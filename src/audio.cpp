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
            float vol = v.musica ? VOL_MUSICA * volume * ganho_musica : v.vol;
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

void Audio::musica_de_fundo(float ganho) { ganho_musica = limitar(ganho, 0.f, 1.f); }

void Audio::parar_efeitos() {
    if (!ativo) return;
    im->lock();
    vozes.erase(std::remove_if(vozes.begin(), vozes.end(), [](const Voz& v) { return !v.musica; }), vozes.end());
    im->unlock();
}

void Audio::tocar_trilha(int final) {
    std::string nome = "trilha" + std::string(1, (char)('0' + final));
    if (!sons.count(nome)) gerar_trilha(final);
    tocar(nome, 0.85f);
}

namespace {
// Soma uma nota com ataque e liberação suaves numa onda (usada para montar os acordes das trilhas).
void nota(Onda& o, float freq, float inicio, float dur, float vol, float ataque, float solta, int timbre) {
    int i0 = (int)(inicio * SR), n = (int)(dur * SR);
    double f1 = TAU * freq / SR, f2 = TAU * (freq * 1.004) / SR;
    for (int i = 0; i < n && i0 + i < (int)o.size(); i++) {
        float t = (float)i / SR;
        float env = std::min(1.0f, t / ataque) * std::min(1.0f, (dur - t) / solta);
        float s;
        if (timbre == 0) {                       // cordas/pad: duas senoides levemente desafinadas + harmônicos
            s = (float)(std::sin(f1 * i) + std::sin(f2 * i)) * 0.5f + (float)std::sin(f1 * 2 * i) * 0.18f + (float)std::sin(f1 * 3 * i) * 0.06f;
        } else {                                 // sino: parciais que decaem
            s = (float)std::sin(f1 * i) * std::exp(-t * 2.2f) + (float)std::sin(f1 * 2.76 * i) * 0.35f * std::exp(-t * 4.5f);
        }
        o[i0 + i] += s * vol * env;
    }
}
float hz(int semitons) { return 220.0f * std::pow(2.0f, semitons / 12.0f); }    // 0 = lá 220 Hz
}  // namespace

void Audio::gerar_trilha(int final) {
    // Quatro acordes (um por plano, 5,5 s cada) e um acorde final que sustenta o cartão de título.
    static const int ACORDES[4][5][3] = {
        {{-9, -5, -2}, {-2, 2, 5}, {0, 3, 7}, {-4, 0, 3}, {3, 7, 10}},        // Ascensão: Dó, Sol, Lá m, Fá, Dó
        {{-7, -4, 0}, {-11, -7, -4}, {-14, -11, -7}, {-12, -8, -5}, {-7, -3, 0}},  // Pulso: Ré m, Si b, Sol m, Lá, Ré
        {{0, 3, 7}, {-4, 0, 3}, {-9, -5, -2}, {-5, -1, 2}, {-12, -9, -5}},     // Guardião: Lá m, Fá, Dó, Mi, Lá m
        {{-7, -3, 0}, {0, 4, 7}, {2, 5, 9}, {-2, 2, 5}, {5, 9, 12}}};          // Perfeito: Ré, Lá, Si m, Sol, Ré
    Onda o((size_t)(SR * 29), 0.f);
    const int (*ac)[3] = ACORDES[final];
    for (int k = 0; k < 5; k++) {
        float ini = k * 5.5f, dur = k == 4 ? 6.8f : 6.4f;
        for (int v = 0; v < 3; v++) nota(o, hz(ac[k][v]), ini, dur, 0.16f, 1.1f, 1.6f, 0);
        nota(o, hz(ac[k][0] - 12), ini, dur, 0.22f, 0.8f, 1.5f, 0);            // baixo
        // arpejo de sinos: mais denso nos finais luminosos
        int passos = (final == 0 || final == 3) ? 8 : 4;
        for (int p = 0; p < passos; p++) {
            int nt = ac[k][p % 3] + 12 + ((p / 3) % 2) * 12;
            if (k >= 1 || p % 2 == 0) nota(o, hz(nt), ini + p * (5.5f / passos), 1.8f, 0.07f + 0.015f * k, 0.01f, 0.5f, 1);
        }
    }
    sons["trilha" + std::string(1, (char)('0' + final))] = criar(o);
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
    sons["respingo"] = criar(somar({ruido(1.6f, 0.7f, 0.55f, 2.6f), tom(120, 0.5f, 0.4f, SIN, 45, 1.5f)}));
    sons["rugido"] = criar(somar({tom(55, 2.6f, 0.7f, SUAVE, 38, 0.9f), ruido(2.6f, 0.25f, 0.96f, 0.8f)}));
    sons["carga"] = criar(tom(110, 4.2f, 0.45f, SUAVE, 880, 0.25f));
    sons["sino"] = criar(somar({tom(880, 2.2f, 0.4f, SIN, 0, 3.0f), tom(1320, 2.2f, 0.2f, SIN, 0, 4.0f), tom(1760, 2.2f, 0.1f, SIN, 0, 5.0f)}));
    sons["explosao"] = criar(somar({tom(70, 2.2f, 0.8f, SIN, 25, 1.4f), ruido(2.2f, 0.55f, 0.97f, 2.0f)}));
    const float notas[8][2] = {{523, .18f}, {659, .18f}, {784, .18f}, {1047, .36f}, {988, .18f}, {784, .18f}, {880, .45f}, {1047, .9f}};
    Onda mel;
    for (auto& p : notas) mel += tom(p[0], p[1], 0.55f, SUAVE, 0, 0.9f);
    Onda base = tom(131, (float)mel.size() / SR, 0.35f, SIN, 0, 0.6f);
    sons["tema"] = criar(somar({mel, base}));
}
