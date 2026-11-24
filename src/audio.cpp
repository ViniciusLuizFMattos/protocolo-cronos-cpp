#include "audio.h"
#include <atomic>
#include <algorithm>
#include <cmath>
#include "comum.h"

namespace {
constexpr int SR = 44100;
constexpr float GANHO = 0.42f;
constexpr float TAU = 6.28318530718f;
typedef std::vector<float> Onda;

Rng rs(12345);

enum Forma { SIN, SUAVE, SERRA, QUADRADA };

Onda tom(float f0, float dur, float vol = 0.4f, Forma forma = SIN, float f1 = 0, float decaimento = 1.5f) {
    int n = (int)(SR * dur);
    Onda out(n);
    double fase = 0;
    if (f1 <= 0) f1 = f0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR;
        fase += TAU * (f0 + (f1 - f0) * t / dur) / SR;
        float s;
        switch (forma) {
            case SIN: s = (float)std::sin(fase); break;
            case SUAVE: s = (float)(std::sin(fase) * 0.8 + std::sin(fase * 2) * 0.25 + std::sin(fase * 3) * 0.12); break;
            case SERRA: s = (float)(2 * std::fmod(fase / TAU, 1.0) - 1); break;
            default: s = std::sin(fase) >= 0 ? 1.f : -1.f;
        }
        float ataque = std::min(1.0f, t / 0.008f);
        out[i] = s * vol * ataque * std::pow(1 - t / dur, decaimento);
    }
    return out;
}

Onda ruido(float dur, float vol = 0.3f, float suav = 0.5f, float decaimento = 3.0f) {
    int n = (int)(SR * dur);
    Onda out(n);
    float ant = 0;
    for (int i = 0; i < n; i++) {
        ant = ant * suav + rs.entre(-1, 1) * (1 - suav);
        out[i] = ant * vol * std::pow(1 - (float)i / n, decaimento);
    }
    return out;
}

Onda somar(std::initializer_list<Onda> ls) {
    size_t n = 0;
    for (auto& l : ls) n = std::max(n, l.size());
    Onda out(n, 0.f);
    for (auto& l : ls) for (size_t i = 0; i < l.size(); i++) out[i] += l[i];
    return out;
}

Onda& operator+=(Onda& a, const Onda& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

std::vector<short> criar(const Onda& o) {
    float mx = 1.0f;
    for (float v : o) mx = std::max(mx, std::fabs(v));
    float k = 32000 * GANHO / mx;
    std::vector<short> r(o.size());
    for (size_t i = 0; i < o.size(); i++) r[i] = (short)(o[i] * k);
    return r;
}
}  // namespace

// Trava leve (a thread de áudio da plataforma chama mixar() enquanto o jogo dispara sons).
struct Audio::Impl {
    std::atomic_flag trava = ATOMIC_FLAG_INIT;
    void lock() { while (trava.test_and_set(std::memory_order_acquire)) {} }
    void unlock() { trava.clear(std::memory_order_release); }
};

Audio::Audio() : im(new Impl()) {}
Audio::~Audio() { delete im; }

void Audio::iniciar(bool real) {
    gerar();
    ativo = real;
    if (ativo) iniciar_musica();
}

void Audio::mixar(short* saida, int n) {
    im->lock();
    for (int i = 0; i < n; i++) {
        float acc = 0;
        for (auto& v : vozes) {
            if (v.pos >= v.dados->size()) {
                if (!v.loop) continue;
                v.pos = 0;
            }
            float vol = v.musica ? VOL_MUSICA * volume : v.vol;
            acc += (*v.dados)[v.pos++] * vol;
        }
        saida[i] = (short)limitar(acc, -32767.f, 32767.f);
    }
    vozes.erase(std::remove_if(vozes.begin(), vozes.end(),
                               [](const Voz& v) { return !v.loop && v.pos >= v.dados->size(); }),
                vozes.end());
    im->unlock();
}

void Audio::tocar(const std::string& nome, float vol) {
    if (!ativo || mudo || volume <= 0) return;
    auto it = sons.find(nome);
    if (it == sons.end()) return;
    im->lock();
    if (vozes.size() < 24) vozes.push_back({&it->second, 0, vol * volume, false, false});
    im->unlock();
}

void Audio::iniciar_musica() {
    if (!ativo || mudo) return;
    im->lock();
    vozes.push_back({&sons["sonar"], 0, 1.f, true, true});
    im->unlock();
}

bool Audio::alternar_mudo() {
    mudo = !mudo;
    if (ativo) {
        im->lock();
        if (mudo) vozes.clear();
        im->unlock();
        if (!mudo) iniciar_musica();
    }
    return mudo;
}

void Audio::definir_volume(float v) { volume = limitar(v, 0.f, 1.f); }

void Audio::gerar() {
    sons["bipe"] = criar(tom(880, 0.09f, 0.5f, SIN, 0, 1.2f));
    sons["passo"] = criar(ruido(0.07f, 0.6f, 0.9f, 4.0f));
    Onda acerto;
    const float fa[3][2] = {{523, 0.11f}, {659, 0.11f}, {784, 0.11f}};
    for (auto& p : fa) acerto += tom(p[0], p[1], 0.6f);
    acerto += tom(1047, 0.30f, 0.6f, SIN, 0, 2.0f);
    sons["acerto"] = criar(acerto);
    Onda alarme;
    for (int k = 0; k < 4; k++) alarme += tom(k % 2 == 0 ? 330.f : 247.f, 0.16f, 0.7f, SUAVE, 0, 0.8f);
    sons["erro"] = criar(alarme);
    Onda fase = tom(220, 0.5f, 0.6f, SIN, 440);
    fase += tom(440, 0.4f, 0.6f, SIN, 660);
    sons["fase"] = criar(fase);
    Onda trava(200, 0.f);
    trava += tom(260, 0.12f, 0.35f, SUAVE, 120);
    sons["porta"] = criar(somar({tom(90, 1.4f, 0.6f, SIN, 38, 0.8f), ruido(1.4f, 0.2f, 0.93f, 1.2f), trava}));
    sons["derrota"] = criar(tom(220, 1.2f, 0.7f, SUAVE, 55, 0.7f));
    Onda vit;
    const float fv[3][2] = {{392, 0.15f}, {523, 0.15f}, {659, 0.15f}};
    for (auto& p : fv) vit += tom(p[0], p[1], 0.6f);
    vit += tom(784, 0.5f, 0.6f, SIN, 0, 1.2f);
    sons["vitoria"] = criar(vit);
    // Música: drone de sonar em loop de 4 s (ciclos inteiros de 55 Hz, 82,5 Hz e 0,5 Hz)
    int n = SR * 4, meio = n / 2;
    Onda pulso(meio);
    for (int i = 0; i < meio; i++) {
        float d = (float)i / SR;
        pulso[i] = std::sin(TAU * 900 * d) * 0.10f * std::exp(-d * 3.2f) + std::sin(TAU * 48 * d) * 0.30f * std::exp(-d * 6.0f);
    }
    Onda sonar(n);
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float s = (std::sin(TAU * 55 * t) * 0.30f + std::sin(TAU * 82.5f * t) * 0.14f) * (0.75f + 0.25f * std::sin(TAU * 0.5f * t));
        sonar[i] = s + pulso[i % meio];
    }
    sons["sonar"] = criar(sonar);
    sons["explosao"] = criar(somar({tom(70, 2.2f, 0.8f, SIN, 25, 1.4f), ruido(2.2f, 0.55f, 0.97f, 2.0f)}));
    const float notas[8][2] = {{523, .18f}, {659, .18f}, {784, .18f}, {1047, .36f}, {988, .18f}, {784, .18f}, {880, .45f}, {1047, .9f}};
    Onda mel;
    for (auto& p : notas) mel += tom(p[0], p[1], 0.55f, SUAVE, 0, 0.9f);
    Onda base = tom(131, (float)mel.size() / SR, 0.35f, SIN, 0, 0.6f);
    sons["tema"] = criar(somar({mel, base}));
}
