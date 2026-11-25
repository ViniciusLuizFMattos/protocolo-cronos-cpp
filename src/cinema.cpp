// Protocolo Cronos++ - cinemáticas dos finais.
// Coordenadas do jogo (1024x640). Em telas largas a cena se estende por x < 0 e x > 1024 (E e D abaixo).
#include "cinema.h"
#include <algorithm>
#include "dados.h"
#include "gfx.h"

using namespace gfx;

namespace cinema {
namespace {

const float TAU = 6.2831853f;
float E = 0, D = 1024;      // bordas esquerda e direita visíveis (atualizadas a cada quadro)

// ------------------------------------------------------------------ utilidades
float rnd(int i, int k = 0) {
    uint32_t x = (uint32_t)i * 73856093u ^ (uint32_t)(k + 7) * 19349663u;
    x ^= x >> 13; x *= 0x5bd1e995u; x ^= x >> 15;
    return (x & 0xFFFFFF) / 16777216.0f;
}
float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
float suave(float a, float b, float x) { float u = clamp01((x - a) / (b - a)); return u * u * (3 - 2 * u); }
float sai(float u) { u = clamp01(u); return 1 - (1 - u) * (1 - u) * (1 - u); }      // desacelera no fim
float entra(float u) { u = clamp01(u); return u * u * u; }                         // acelera
float mistura(float a, float b, float u) { return a + (b - a) * u; }
float resto(float v, float m) { float r = std::fmod(v, m); return r < 0 ? r + m : r; }

void quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, Cor c, int a = 255) {
    Pt p[4] = {{x0, y0}, {x1, y1}, {x2, y2}, {x3, y3}};
    poligono(p, 4, c, a);
}

// ------------------------------------------------------------------ ambiente
void mar(float t, Cor topo, Cor fundo, float raios) {
    gradiente((int)E, 0, (int)(D - E) + 1, ALTURA, topo, fundo);
    if (raios > 0)
        for (int k = 0; k < 7; k++) {                        // feixes de luz vindos de cima
            float x = E + (D - E) * (k + 0.5f) / 7 + std::sin(t * 0.35f + k * 1.7f) * 40;
            float larg = 16 + 22 * rnd(k, 1);
            int a = (int)(raios * (10 + 9 * std::sin(t * 0.8f + k)));
            quad(x - larg, 0, x + larg, 0, x + larg * 4 + 90, ALTURA, x - larg + 40, ALTURA, rgb(150, 210, 235), a);
            quad(x - larg * 0.4f, 0, x + larg * 0.4f, 0, x + larg * 2 + 80, ALTURA, x + 55, ALTURA, rgb(190, 235, 250), a);
        }
}

void relevo(float base, float altura, float rolagem, Cor c, int semente) {   // silhueta de montanhas submarinas
    std::vector<Pt> p;
    float passo = 64;
    float ini = E - passo * 2 - resto(rolagem, passo);
    int deslc = (int)std::floor(rolagem / passo);
    int n = 0;
    for (float x = ini; x < D + passo * 2; x += passo, n++) {
        int id = n + deslc;
        p.push_back({x, base - altura * (0.35f + 0.65f * rnd(id, semente))});
    }
    p.push_back({D + passo * 2, (float)ALTURA + 40});
    p.push_back({ini, (float)ALTURA + 40});
    poligono(p.data(), (int)p.size(), c);
}

void neve(float t, int n, float sobe, float prof) {           // "neve marinha" em camadas de profundidade
    float larg = D - E;
    for (int i = 0; i < n; i++) {
        float z = 0.25f + 0.75f * rnd(i, 3);
        float x = E + resto(rnd(i, 1) * larg + std::sin(t * 0.5f + i) * 14 * z + t * 6 * z, larg);
        float y = resto(rnd(i, 2) * ALTURA + t * sobe * z * prof, (float)ALTURA);
        circ(x, y, 0.4f + 1.6f * z * z, rgb(190, 225, 240), (int)(40 + 120 * z));
    }
}

void bolhas(float x, float y, float t, int n, float abertura, float vel, int semente = 0) {
    for (int i = 0; i < n; i++) {
        float vida = resto(t * vel * (0.6f + rnd(i, semente)) / 300 + rnd(i, semente + 1), 1.0f);
        float bx = x + (rnd(i, semente + 2) - 0.5f) * abertura * (0.2f + vida * 1.4f) + std::sin(t * 4 + i) * 6 * vida;
        float by = y + vida * 300;
        float r = 1.2f + 5 * rnd(i, semente + 3) * (0.4f + vida);
        int a = (int)(220 * (1 - vida));
        circ_borda(bx, by, r, 1.2f, rgb(180, 230, 245), a);
        circ(bx - r * 0.3f, by - r * 0.3f, r * 0.2f, rgb(255, 255, 255), a);
    }
}

// ------------------------------------------------------------------ organismo
void tentaculo(float bx, float by, float ang, float comp, float larg, float t, float fase, Cor c, Cor brilho_borda, float luz_ventosa) {
    const int N = 34;
    Pt p[N + 1];
    float dir[N];
    p[0] = {bx, by};
    float passo = comp / N;
    for (int i = 0; i < N; i++) {
        float u = (float)i / N;
        dir[i] = ang + std::sin(t * 1.15f + fase - u * 4.6f) * 0.85f * u + std::sin(t * 0.6f + fase * 2 + u * 3) * 0.25f;
        p[i + 1] = {p[i].x + std::cos(dir[i]) * passo, p[i].y + std::sin(dir[i]) * passo};
    }
    Cor contorno = lerp(c, brilho_borda, 0.55f);
    for (int i = 0; i < N; i++) linha(p[i].x, p[i].y, p[i + 1].x, p[i + 1].y, larg * (1 - 0.9f * i / N) * 2 + 3, contorno);
    for (int i = 0; i < N; i++) linha(p[i].x, p[i].y, p[i + 1].x, p[i + 1].y, larg * (1 - 0.9f * i / N) * 2, c);
    for (int i = 1; i < N - 4; i += 3) {                                // ventosas
        float w = larg * (1 - 0.9f * i / N);
        float vx = p[i + 1].x - std::sin(dir[i]) * w * 0.55f, vy = p[i + 1].y + std::cos(dir[i]) * w * 0.55f;
        circ(vx, vy, std::max(1.0f, w * 0.2f), brilho_borda, (int)(230 * luz_ventosa));
        if (luz_ventosa > 0.4f && i % 6 == 1) luz(vx, vy, w * 1.7f, brilho_borda, 0.4f * luz_ventosa);
    }
}

void olho(float cx, float cy, float R, float aberto, Cor iris, float olhar_x, float t) {
    if (aberto <= 0.01f) {                                         // pálpebra fechada
        arco(cx, cy - R * 0.1f, R, R * 0.22f, TAU * 0.55f, TAU * 0.95f, std::max(1.5f, R * 0.035f), rgb(70, 40, 60));
        return;
    }
    float h = R * 0.62f * aberto;
    luz(cx, cy, R * 2.3f, iris, 0.28f * aberto);
    const int N = 36;
    Pt p[N * 2];
    for (int i = 0; i < N; i++) {
        float u = (float)i / (N - 1), s = std::pow(std::sin(u * TAU / 2), 0.85f);
        p[i] = {cx - R + 2 * R * u, cy - h * s};
        p[N * 2 - 1 - i] = {cx - R + 2 * R * u, cy + h * s * 0.85f};
    }
    poligono(p, N * 2, rgb(18, 8, 14));
    for (int i = 0; i < N * 2; i++) linha(p[i].x, p[i].y, p[(i + 1) % (N * 2)].x, p[(i + 1) % (N * 2)].y, std::max(2.0f, R * 0.025f), lerp(iris, rgb(0, 0, 0), 0.35f), 200);
    float ri = std::min(h * 0.82f, R * 0.46f);
    float ix = cx + olhar_x * R * 0.22f;
    circ(ix, cy, ri, lerp(iris, rgb(0, 0, 0), 0.55f));
    circ(ix, cy, ri * 0.78f, lerp(iris, rgb(0, 0, 0), 0.2f));
    circ(ix, cy, ri * 0.5f, iris);
    for (int i = 0; i < 28; i++) {                                 // estrias da íris
        float a = TAU * i / 28 + std::sin(t * 0.7f) * 0.05f;
        linha(ix + std::cos(a) * ri * 0.3f, cy + std::sin(a) * ri * 0.3f, ix + std::cos(a) * ri * (0.7f + 0.25f * rnd(i, 4)),
              cy + std::sin(a) * ri * (0.7f + 0.25f * rnd(i, 4)), std::max(1.0f, ri * 0.03f), lerp(iris, rgb(255, 255, 220), 0.4f), 110);
    }
    elipse((int)(ix - ri * 0.13f), (int)(cy - ri * 0.78f), (int)(ri * 0.26f) + 1, (int)(ri * 1.56f) + 1, rgb(6, 2, 6));     // pupila em fenda
    circ(ix - ri * 0.33f, cy - ri * 0.34f, ri * 0.13f, rgb(255, 250, 240), 220);
    luz(ix, cy, ri * 1.5f, iris, 0.22f);
}

// ------------------------------------------------------------------ estação e cápsula
void estacao(float cx, float y, float s, int janelas, Cor cor_jan, float nucleo, float t, int apagadas_piscando = 0) {
    auto X = [&](float v) { return cx + v * s; };
    auto Y = [&](float v) { return y + v * s; };
    for (int k = -1; k <= 1; k++) {                              // pernas treliçadas até o leito marinho
        float x = k * 150.0f;
        quad(X(x - 11), Y(80), X(x + 11), Y(80), X(x + 18 + k * 20), Y(330), X(x - 18 + k * 20), Y(330), rgb(10, 17, 28));
        for (int j = 0; j < 5; j++) linha(X(x - 11), Y(100 + j * 44), X(x + 14), Y(140 + j * 44), 2 * s, rgb(26, 40, 58));
    }
    for (int k = -1; k <= 1; k += 2) {                           // cúpulas
        circ(X(k * 128.0f), Y(8), 52 * s, rgb(28, 40, 56));
        circ_borda(X(k * 128.0f), Y(8), 52 * s, 2.5f * s, rgb(70, 100, 130));
        arco(X(k * 128.0f), Y(8), 38 * s, 38 * s, 1.7f, 2.7f, 3 * s, rgb(130, 180, 210), 160);
        luz(X(k * 128.0f), Y(-8), 34 * s, cor_jan, janelas > 0 ? 0.3f : 0.0f);
    }
    rret((int)X(-230), (int)Y(0), (int)(460 * s), (int)(104 * s), (int)(26 * s), rgb(24, 34, 48));
    ret((int)X(-224), (int)Y(5), (int)(448 * s), std::max(1, (int)(9 * s)), rgb(62, 86, 112));
    rret_borda((int)X(-230), (int)Y(0), (int)(460 * s), (int)(104 * s), (int)(26 * s), std::max(1, (int)(2 * s)), rgb(60, 84, 108));
    ret((int)X(-40), (int)Y(-46), (int)(80 * s), (int)(48 * s), rgb(26, 38, 54));
    linha(X(0), Y(-46), X(0), Y(-110), 3 * s, rgb(60, 84, 108));
    bool pisca = ((int)(t * 2)) % 2 == 0;
    circ(X(0), Y(-112), 4 * s, rgb(255, 60, 50), pisca ? 255 : 80);
    if (pisca) luz(X(0), Y(-112), 26 * s, rgb(255, 60, 50), 0.8f);
    for (int i = 0; i < 12; i++) {
        float jx = -198.0f + i * 34;
        bool acesa = i < janelas;
        if (i == janelas && apagadas_piscando && ((int)(t * 14)) % 2 == 0) acesa = true;
        rret((int)X(jx), (int)Y(34), (int)(18 * s) + 1, (int)(12 * s) + 1, 2, acesa ? cor_jan : rgb(10, 16, 24));
        if (acesa) luz(X(jx + 9), Y(40), 20 * s, cor_jan, 0.35f);
    }
    if (nucleo > 0) {
        float r = (14 + 12 * nucleo) * s;
        luz(X(0), Y(74), r * (5 + 3 * nucleo), rgb(255, 130, 40), 0.55f * nucleo);
        circ(X(0), Y(74), r, rgb(255, (int)(130 + 100 * nucleo), 40));
        circ(X(0), Y(74), r * 0.5f, rgb(255, 245, 215));
    }
}

void capsula(float cx, float cy, float s, float ang, float chama, float t) {
    float ca = std::cos(ang), sa = std::sin(ang);
    auto P = [&](float x, float y) { Pt p = {cx + (x * ca - y * sa) * s, cy + (x * sa + y * ca) * s}; return p; };
    if (chama > 0) {
        float c = (22 + 10 * std::sin(t * 41) + 6 * std::sin(t * 67)) * chama;
        Pt f1[3] = {P(-11, 36), P(11, 36), P(0, 36 + c * 1.6f)};
        poligono(f1, 3, rgb(255, 120, 40), 190);
        Pt f2[3] = {P(-6, 36), P(6, 36), P(0, 36 + c)};
        poligono(f2, 3, rgb(255, 230, 150));
        Pt l = P(0, 44);
        luz(l.x, l.y, 62 * s * chama, rgb(255, 150, 60), 0.75f);
    }
    for (int lado = -1; lado <= 1; lado += 2) {                  // aletas
        Pt a[3] = {P(lado * 17.0f, 12), P(lado * 32.0f, 40), P(lado * 12.0f, 32)};
        poligono(a, 3, rgb(210, 110, 20));
    }
    Pt corpo[24];
    for (int i = 0; i < 24; i++) {
        float a = TAU * i / 24;
        float ex = std::cos(a) * 20, ey = std::sin(a) * 38;
        corpo[i] = P(ex, ey);
    }
    poligono(corpo, 24, rgb(196, 208, 224));
    Pt brilho_c[8] = {P(-14, -20), P(-8, -31), P(-4, -32), P(-9, -10), P(-10, 12), P(-12, 24), P(-16, 14), P(-17, -4)};
    poligono(brilho_c, 8, rgb(240, 246, 255), 190);
    for (int i = 0; i < 24; i++) linha(corpo[i].x, corpo[i].y, corpo[(i + 1) % 24].x, corpo[(i + 1) % 24].y, 2 * s, rgb(96, 112, 132));
    Pt faixa[4] = {P(-19, 8), P(19, 8), P(18, 16), P(-18, 16)};
    poligono(faixa, 4, rgb(250, 150, 30));
    Pt v = P(0, -10);
    circ(v.x, v.y, 10 * s, rgb(60, 74, 90));
    circ(v.x, v.y, 7.5f * s, rgb(70, 215, 240));
    Pt r = P(-3, -13);
    circ(r.x, r.y, 2.2f * s, rgb(235, 255, 255));
    luz(v.x, v.y, 26 * s, rgb(70, 215, 240), 0.45f);
}

void mergulhador(float cx, float cy, float s, float braco) {        // visto de costas, diante do console
    elipse((int)(cx - 38 * s), (int)(cy + 70 * s), (int)(76 * s), (int)(18 * s), rgb(0, 0, 0), 130);
    linha(cx - 12 * s, cy + 30 * s, cx - 16 * s, cy + 74 * s, 17 * s, rgb(190, 100, 16));
    linha(cx + 12 * s, cy + 30 * s, cx + 16 * s, cy + 74 * s, 17 * s, rgb(190, 100, 16));
    float mx = cx + 52 * s, my = cy - (6 + 34 * braco) * s;
    linha(cx + 20 * s, cy - 8 * s, cx + 40 * s, cy + (6 - 14 * braco) * s, 14 * s, rgb(235, 140, 30));
    linha(cx + 40 * s, cy + (6 - 14 * braco) * s, mx, my, 12 * s, rgb(235, 140, 30));
    circ(mx, my, 7 * s, rgb(60, 70, 84));
    linha(cx - 20 * s, cy - 8 * s, cx - 36 * s, cy + 26 * s, 13 * s, rgb(220, 128, 26));
    rret((int)(cx - 25 * s), (int)(cy - 24 * s), (int)(50 * s), (int)(62 * s), (int)(18 * s), rgb(250, 150, 30));
    rret((int)(cx - 17 * s), (int)(cy - 16 * s), (int)(34 * s), (int)(48 * s), (int)(12 * s), rgb(70, 84, 100));
    linha(cx, cy - 10 * s, cx, cy + 26 * s, 3 * s, rgb(40, 50, 62));
    circ(cx, cy - 38 * s, 22 * s, rgb(205, 215, 228));
    arco(cx, cy - 38 * s, 17 * s, 17 * s, 1.6f, 2.8f, 3 * s, rgb(255, 255, 255), 200);
    arco(cx, cy - 38 * s, 22 * s, 22 * s, -0.7f, 0.9f, 4 * s, rgb(110, 220, 240), 210);
}

// ------------------------------------------------------------------ superfície
void ceu(float horizonte, Cor topo, Cor base) { gradiente((int)E, 0, (int)(D - E) + 1, (int)horizonte + 1, topo, base); }

void estrelas(float t, float horizonte, float a) {
    for (int i = 0; i < 160; i++) {
        float x = E + rnd(i, 11) * (D - E), y = rnd(i, 12) * horizonte * 0.92f;
        float tw = 0.55f + 0.45f * std::sin(t * (1 + 2 * rnd(i, 13)) + i);
        circ(x, y, 0.3f + 1.1f * rnd(i, 14), rgb(230, 240, 255), (int)(255 * a * tw));
    }
}

void sol(float x, float y, float r, Cor c, float forca) {
    luz(x, y, r * 7, c, 0.34f * forca);
    luz(x, y, r * 2.8f, c, 0.6f * forca);
    circ(x, y, r, lerp(c, rgb(255, 255, 255), 0.65f));
}

void oceano(float horizonte, float t, Cor longe, Cor perto, float reflexo_x, Cor reflexo) {
    gradiente((int)E, (int)horizonte, (int)(D - E) + 1, ALTURA - (int)horizonte, longe, perto);
    for (int c = 0; c < 15; c++) {                              // cristas das ondas em perspectiva
        float u = (c + 1) / 15.0f;
        float y = horizonte + (ALTURA - horizonte) * u * u;
        float comp = 24 + 150 * u, amp = 0.6f + 5 * u;
        float vel = t * (10 + 40 * u) * (c % 2 ? 1 : -1);
        for (float x = E - resto(vel + c * 37, comp * 2) ; x < D; x += comp * 2) {
            float yy = y + std::sin(t * 1.3f + x * 0.02f + c) * amp;
            linha(x, yy, x + comp * 0.7f, yy - amp * 0.4f, 0.8f + 2.4f * u, lerp(longe, rgb(220, 240, 255), 0.35f), (int)(60 + 70 * u));
        }
        float larg = (20 + 90 * u) * (0.6f + 0.4f * std::sin(t * 2.3f + c * 1.9f));
        linha(reflexo_x - larg / 2, y + 2, reflexo_x + larg / 2, y + 2, 1 + 3 * u, reflexo, (int)(170 - 90 * u));
    }
}

void aurora(float t, float a) {
    const float passo = 5;
    for (int faixa = 0; faixa < 3; faixa++) {
        Cor c = faixa == 0 ? rgb(60, 255, 160) : (faixa == 1 ? rgb(70, 200, 255) : rgb(170, 110, 255));
        auto base = [&](float x) { return 150 + faixa * 38 + std::sin(x * 0.007f + t * 0.5f + faixa * 2) * 46 + std::sin(x * 0.021f - t * 0.8f) * 14; };
        auto alt = [&](float x) { return 80 + 45 * std::sin(x * 0.013f + t + faixa); };
        for (float x = E; x < D; x += passo) {
            float y0 = base(x), y1 = base(x + passo), h = alt(x);
            float forca = a * (0.6f + 0.4f * std::sin(x * 0.045f + t * 2 + faixa * 3));
            for (int k = 0; k < 7; k++) {                  // cortina: mais forte embaixo, some para cima
                float u0 = k / 7.0f, u1 = (k + 1) / 7.0f;
                int al = (int)(forca * 46 * (1 - u0) * (1 - u0 * 0.4f));
                quad(x, y0 - h * u1, x + passo, y1 - h * u1, x + passo, y1 - h * u0, x, y0 - h * u0, c, al);
            }
        }
    }
}

// ------------------------------------------------------------------ efeitos
void explosao(float cx, float cy, float te) {        // te = segundos desde a detonação
    if (te < 0) return;
    float some = clamp01(1 - te / 4.0f);
    luz(cx, cy, 120 + 560 * sai(te / 1.6f), rgb(255, 140, 50), 0.9f * some);
    luz(cx, cy, 60 + 260 * sai(te / 1.2f), rgb(255, 230, 170), 1.0f * clamp01(1 - te / 2.4f));
    for (int i = 0; i < 150; i++) {                  // brasas e destroços
        float ang = rnd(i, 21) * TAU, vel = 60 + 360 * rnd(i, 22);
        float dist = vel * (1 - std::exp(-te * 1.4f)) / 1.1f;
        float x = cx + std::cos(ang) * dist, y = cy + std::sin(ang) * dist * 0.8f + 26 * te * te;
        float vida = clamp01(1 - te / (1.6f + 2.4f * rnd(i, 23)));
        if (vida <= 0) continue;
        if (i % 5 == 0) {                            // chapa metálica girando
            float a = te * (2 + 6 * rnd(i, 24)) + i, w = 5 + 14 * rnd(i, 25), h = 3 + 6 * rnd(i, 26);
            float c = std::cos(a), s = std::sin(a);
            quad(x - w * c + h * s, y - w * s - h * c, x + w * c + h * s, y + w * s - h * c, x + w * c - h * s, y + w * s + h * c,
                 x - w * c - h * s, y - w * s + h * c, rgb(58, 66, 80), (int)(255 * clamp01(vida * 2)));
        } else {
            Cor c = lerp(rgb(255, 80, 20), rgb(255, 240, 180), vida);
            linha(x, y, x - std::cos(ang) * 14 * vida, y - std::sin(ang) * 11 * vida, 1 + 2.5f * vida, c, (int)(255 * vida));
            if (i % 4 == 1) luz(x, y, 14 * vida + 3, c, 0.8f * vida);
        }
    }
    for (int k = 0; k < 3; k++) {                    // ondas de choque
        float tk = te - k * 0.22f;
        if (tk <= 0 || tk > 2.2f) continue;
        circ_borda(cx, cy, 30 + 620 * sai(tk / 2.2f), 3 + 12 * (1 - tk / 2.2f), k == 0 ? rgb(255, 235, 200) : rgb(255, 160, 90),
                   (int)(210 * (1 - tk / 2.2f)));
    }
}

void selo(float cx, float cy, float R, float progresso, float t, Cor c) {      // anéis de contenção que se fecham
    for (int k = 0; k < 4; k++) {
        float p = clamp01(progresso * 4 - k);
        if (p <= 0) continue;
        float r = R * (0.45f + 0.22f * k) * (1.5f - 0.5f * sai(p));
        float giro = t * (0.35f + 0.2f * k) * (k % 2 ? -1 : 1);
        int seg = 6 + k * 2;
        for (int i = 0; i < seg; i++) {
            float a0 = giro + TAU * i / seg;
            arco(cx, cy, r, r, a0, a0 + TAU / seg * 0.72f * p, 3.5f, c, (int)(235 * p));
            circ(cx + std::cos(a0) * r, cy - std::sin(a0) * r, 3.5f, rgb(235, 255, 255), (int)(235 * p));
        }
        circ_borda(cx, cy, r + 9, 1, c, (int)(90 * p));
    }
    luz(cx, cy, R * 1.3f, c, 0.35f * clamp01(progresso));
}

// Grafo completo K_n desenhado aresta a aresta: C(n,2) ligações, o tema do jogo.
void constelacao(float cx, float cy, float R, int n, float progresso, float t, Cor c) {
    Pt p[12];
    for (int i = 0; i < n; i++) {
        float a = TAU * i / n + t * 0.12f;
        p[i] = {cx + std::cos(a) * R, cy + std::sin(a) * R * 0.86f};
    }
    int total = n * (n - 1) / 2, k = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++, k++) {
            float u = clamp01(progresso * total - k);
            if (u <= 0) continue;
            linha(p[i].x, p[i].y, mistura(p[i].x, p[j].x, u), mistura(p[i].y, p[j].y, u), 1.4f, c, 120);
        }
    for (int i = 0; i < n; i++) {
        luz(p[i].x, p[i].y, 26, c, 0.8f * clamp01(progresso * 2));
        circ(p[i].x, p[i].y, 4.5f, rgb(235, 255, 255), (int)(255 * clamp01(progresso * 3)));
    }
}

// ================================================================== PLANOS
// u = segundos dentro do plano (0 a DUR_PLANO); T = tempo total (para movimentos contínuos).

// ---------------- Final 0: Ascensão
void asc_escotilha(float u, float T) {
    mar(T, rgb(26, 14, 20), rgb(8, 6, 12), 0);
    float z = 1 + 0.09f * (u / DUR_PLANO);
    float cx = 512, cy = 300;
    for (int i = 0; i < 12; i++) {                                    // anteparas
        float x = E + (D - E) * i / 11.0f;
        ret((int)x - 3, 0, 6, ALTURA, rgb(34, 22, 30));
    }
    float alarme = 0.5f + 0.5f * std::sin(T * 5.2f);
    luz(E + 60, 90, 360, rgb(255, 40, 30), 0.55f * alarme);
    luz(D - 60, 90, 360, rgb(255, 40, 30), 0.55f * (1 - alarme));
    circ(cx, cy, 218 * z, rgb(52, 62, 78));
    circ(cx, cy, 196 * z, rgb(30, 38, 52));
    float fecha = sai(u / 3.2f);
    circ(cx, cy, 168 * z, rgb(10, 16, 26));
    capsula(cx, cy + 6, 3.0f * z, 0, 0, T);
    for (int lado = -1; lado <= 1; lado += 2) {                       // portas deslizando
        float aberta = (1 - fecha) * 175 * z;
        float x0 = lado < 0 ? cx - 170 * z - aberta : cx + aberta, w = 170 * z;
        rret((int)x0, (int)(cy - 170 * z), (int)w, (int)(340 * z), 14, rgb(92, 104, 122));
        rret_borda((int)x0, (int)(cy - 170 * z), (int)w, (int)(340 * z), 14, 4, rgb(48, 58, 74));
        for (int k = 0; k < 8; k++) {
            float y = cy - 150 * z + k * 42 * z;
            linha(x0 + 12, y + 24, x0 + 44, y, 9, rgb(235, 190, 40));
        }
        circ(x0 + w / 2, cy, 26 * z, rgb(50, 62, 80));
        circ_borda(x0 + w / 2, cy, 26 * z, 5, rgb(160, 176, 196));
    }
    circ_borda(cx, cy, 205 * z, 20, rgb(66, 80, 100));
    for (int i = 0; i < 16; i++) {
        float a = TAU * i / 16;
        bool ok = u > 3.2f + i * 0.07f;
        circ(cx + std::cos(a) * 205 * z, cy + std::sin(a) * 205 * z, 6, ok ? rgb(80, 255, 150) : rgb(255, 70, 60));
        if (ok) luz(cx + std::cos(a) * 205 * z, cy + std::sin(a) * 205 * z, 22, rgb(80, 255, 150), 0.7f);
    }
    for (int i = 0; i < 40; i++) {                                    // vapor da vedação
        float vida = resto((u - 3.0f) * 0.7f + rnd(i, 31), 1.0f);
        if (u < 3.1f) break;
        float lado = i % 2 ? 1.f : -1.f;
        circ(cx + lado * (20 + vida * 230) * rnd(i, 32), cy + 172 * z - vida * 70 + rnd(i, 33) * 30, 6 + 22 * vida, rgb(200, 215, 230), (int)(60 * (1 - vida)));
    }
}

void asc_lancamento(float u, float T) {
    mar(T, rgb(8, 30, 52), rgb(1, 5, 12), 0.5f);
    float sobe = entra(u / DUR_PLANO);
    float cam = 190 * sobe;                         // a câmera acompanha a cápsula
    relevo(600 + cam * 0.35f, 130, 30, rgb(6, 18, 32), 3);
    relevo(650 + cam * 0.6f, 100, 200, rgb(4, 11, 22), 4);
    estacao(512, 400 + cam, 1.15f, std::max(0, 12 - (int)(std::max(0.f, u - 1.5f) * 3)), rgb(250, 214, 120), 0, T, 1);
    float y = 360 - 170 * sai(u / 2.5f) - 40 * sobe + std::sin(T * 2.2f) * 3;
    float partida = suave(0.4f, 1.0f, u);
    bolhas(512, y + 40, T, 90, 70, 150);
    luz(512, 390 + cam, 220, rgb(255, 160, 80), 0.5f * partida * clamp01(1 - u / 3));
    capsula(512, y, 1.35f, std::sin(T * 1.3f) * 0.035f, partida, T);
    neve(T, 130, 70, 1 + 3 * sobe);
}

void asc_olho(float u, float T) {
    mar(T, rgb(10, 22, 40), rgb(1, 2, 7), 0.35f);
    float abre = suave(1.1f, 2.4f, u);
    float fundo_y = 70 * sai(u / DUR_PLANO);        // o abismo fica para trás
    relevo(660 + fundo_y, 170, 0, rgb(4, 8, 18), 7);
    for (int i = 0; i < 7; i++) {
        float bx = E + (D - E) * (i + 0.5f) / 7;
        float comp = (230 + 180 * rnd(i, 41)) * (0.35f + 0.65f * sai((u - 0.3f) / 3.5f));
        tentaculo(bx, 690 + fundo_y, -TAU / 4 + (bx - 512) * -0.0006f, comp, 20 + 10 * rnd(i, 42), T, i * 1.9f, rgb(22, 12, 26), rgb(150, 40, 80), 0.6f);
    }
    float olhar = std::sin(u * 1.4f) * 0.5f;
    olho(300, 408 + fundo_y * 0.5f, 118, abre, rgb(255, 60, 40), olhar + 0.4f, T);
    olho(724, 408 + fundo_y * 0.5f, 118, suave(1.3f, 2.6f, u), rgb(255, 60, 40), olhar - 0.4f, T);
    float y = 300 - 210 * suave(1.8f, DUR_PLANO, u);
    float x = 512 + std::sin(u * 1.9f) * 26;
    bolhas(x, y + 36, T, 60, 50, 170, 5);
    capsula(x, y, 0.85f, std::cos(u * 1.9f) * 0.09f, 1, T);
    neve(T, 120, 130, 1.6f);
}

void asc_superficie(float u, float T) {
    float h = 360;
    float amanhece = suave(0, 4.5f, u);
    ceu(h, lerp(rgb(30, 44, 96), rgb(80, 150, 224), amanhece), lerp(rgb(180, 90, 110), rgb(255, 214, 150), amanhece));
    estrelas(T, h, 0.7f * (1 - amanhece));
    float sy = h + 40 - 150 * sai(u / DUR_PLANO);
    sol(660, sy, 44, rgb(255, 180, 90), 1);
    for (int i = 0; i < 6; i++) {                               // nuvens
        float x = E + resto(rnd(i, 51) * (D - E) + T * (6 + 8 * rnd(i, 52)), D - E);
        float y = 70 + rnd(i, 53) * 170;
        for (int k = 0; k < 5; k++)
            elipse((int)(x + k * 28 - 70), (int)(y - 12 - 10 * std::sin(k * 0.8f)), 70, 28, lerp(rgb(120, 90, 130), rgb(255, 240, 230), amanhece), 120);
    }
    oceano(h, T, rgb(40, 96, 140), rgb(6, 34, 66), 660, rgb(255, 214, 140));
    float salto = sai((u - 0.9f) / 1.1f), cai = entra((u - 2.0f) / 0.9f);
    float y = 640 - 380 * salto + 230 * cai;
    if (u > 2.9f) y = 490 + std::sin(T * 2) * 8;
    float inclina = u > 2.9f ? std::sin(T * 1.5f) * 0.13f : 0.12f * std::sin(u * 3);
    capsula(430, y, 1.5f, inclina, u < 2.0f ? 1.f : 0.f, T);
    if (u > 2.9f) {                                              // a parte submersa
        gradiente(360, 515, 140, 70, rgb(24, 74, 112), rgb(14, 52, 86), 235);
        for (int k = 0; k < 4; k++) {
            float r = resto((u - 2.9f) * 60 + k * 45, 180.0f);
            elipse_borda((int)(430 - r), (int)(517 - r * 0.18f), (int)(2 * r), (int)(r * 0.36f), 2, rgb(220, 240, 255), (int)(170 * (1 - r / 180)));
        }
    }
    float splash[2] = {u - 1.0f, u - 2.85f};
    for (int k = 0; k < 2; k++) {                                // respingos
        float s = splash[k];
        if (s < 0 || s > 1.6f) continue;
        for (int i = 0; i < 70; i++) {
            float vx = (rnd(i, 54 + k) - 0.5f) * 300, vy = -(110 + 250 * rnd(i, 56 + k));
            circ(430 + vx * s, 520 + vy * s + 380 * s * s, 1 + 3.5f * rnd(i, 58), rgb(230, 245, 255), (int)(230 * (1 - s / 1.6f)));
        }
    }
    for (int i = 0; i < 5; i++) {                                // gaivotas
        float x = E - 60 + resto(T * 46 + i * 63, D - E + 120), yb = 120 + i * 17 + std::sin(T + i) * 10;
        float asa = std::sin(T * 7 + i) * 5;
        linha(x - 9, yb - asa, x, yb, 2, rgb(30, 34, 52), (int)(220 * amanhece));
        linha(x, yb, x + 9, yb - asa, 2, rgb(30, 34, 52), (int)(220 * amanhece));
    }
}

// ---------------- Final 1: Pulso Cronos
void reator(float cx, float cy, float s, float carga, float T) {
    luz(cx, cy, 330 * s, rgb(255, 120, 30), 0.25f + 0.55f * carga);
    circ(cx, cy, 170 * s, rgb(40, 30, 34));
    circ_borda(cx, cy, 170 * s, 14 * s, rgb(150, 110, 40));
    for (int k = 0; k < 3; k++) {
        float r = (150 - k * 34) * s;
        float giro = T * (0.6f + k * 0.5f) * (1 + 7 * carga * carga) * (k % 2 ? -1 : 1);
        for (int i = 0; i < 5 + k; i++) {
            float a = giro + TAU * i / (5 + k);
            arco(cx, cy, r, r, a, a + TAU / (5 + k) * 0.6f, 11 * s, lerp(rgb(150, 70, 20), rgb(255, 220, 120), carga));
        }
    }
    float pulsa = 1 + 0.09f * std::sin(T * (6 + 22 * carga));
    circ(cx, cy, 46 * s * pulsa, rgb(255, 140, 40));
    circ(cx, cy, 30 * s * pulsa, rgb(255, 215, 120));
    circ(cx, cy, 15 * s * pulsa, rgb(255, 252, 235));
    luz(cx, cy, (90 + 130 * carga) * s, rgb(255, 220, 150), 0.7f);
    for (int i = 0; i < (int)(3 + 9 * carga); i++) {                // arcos elétricos
        int passo = (int)(T * 14) + i * 17;
        float a = rnd(passo, 61) * TAU;
        float x = cx, y = cy;
        for (int j = 1; j <= 7; j++) {
            float d = (25 + 22 * j) * s, aa = a + (rnd(passo + j, 62) - 0.5f) * 0.5f;
            float nx = cx + std::cos(aa) * d, ny = cy + std::sin(aa) * d;
            linha(x, y, nx, ny, 2.2f, rgb(200, 235, 255), (int)(220 * carga));
            x = nx; y = ny;
        }
        luz(x, y, 34 * s, rgb(120, 190, 255), 0.7f * carga);
    }
}

void pulso_nucleo(float u, float T) {
    mar(T, rgb(30, 14, 12), rgb(8, 4, 8), 0);
    float carga = entra(u / DUR_PLANO);
    for (int i = 0; i < 10; i++) {
        float x = E + (D - E) * i / 9.0f;
        ret((int)x - 6, 0, 12, ALTURA, rgb(40, 24, 24));
        linha(x, 0, 512, 310, 2, rgb(255, 140, 50), (int)(30 + 90 * carga));
    }
    reator(512, 300, 1 + 0.18f * u / DUR_PLANO, carga, T);
    for (int i = 0; i < 90; i++) {                               // partículas sugadas pelo núcleo
        float vida = resto(T * (0.4f + rnd(i, 63)) * (1 + carga) + rnd(i, 64), 1.0f);
        float a = rnd(i, 65) * TAU, d = (1 - vida) * 520 + 40;
        circ(512 + std::cos(a) * d, 300 + std::sin(a) * d, 1 + 2.5f * vida, rgb(255, 190, 90), (int)(220 * vida));
    }
}

void pulso_fuga(float u, float T) {
    mar(T, rgb(16, 22, 40), rgb(4, 3, 8), 0.3f);
    float carga = 0.4f + 0.6f * u / DUR_PLANO;
    relevo(590, 150, 10, rgb(8, 12, 24), 3);
    relevo(645, 100, 90, rgb(5, 7, 16), 4);
    for (int i = 0; i < 6; i++) {
        float bx = E + (D - E) * (i + 0.5f) / 6;
        tentaculo(bx, 660, -TAU / 4 + (512 - bx) * 0.0013f, 330 + 130 * rnd(i, 66), 22, T * 1.6f, i * 2.3f, rgb(44, 18, 40), rgb(235, 70, 90), 1);
    }
    float pisca = 0.65f + 0.35f * std::fabs(std::sin(T * (5 + u * 2)));
    estacao(512, 420, 1.1f, 12, lerp(rgb(255, 120, 50), rgb(255, 235, 190), pisca), carga * pisca, T);
    float parte = entra((u - 0.7f) / 4.0f);
    float x = 512 + 250 * parte, y = 350 - 280 * parte;
    bolhas(x - 20 * parte, y + 40, T, 70, 40, 190, 9);
    capsula(x, y, 1.0f - 0.3f * parte, 0.55f * suave(0.7f, 2.4f, u), suave(0.5f, 0.9f, u), T);
    neve(T, 100, 40, 1);
}

void pulso_explosao(float u, float T) {
    mar(T, rgb(16, 24, 44), rgb(4, 4, 10), 0.2f);
    float te = u - 0.9f;
    relevo(600, 150, 10, rgb(8, 12, 24), 3);
    if (te < 0.15f) {
        estacao(512, 400, 0.95f, 12, rgb(255, 250, 230), 1, T);
        luz(512, 470, 200 + 300 * suave(0, 0.9f, u), rgb(255, 220, 170), suave(0, 1.05f, u));
    }
    float dissolve = suave(0.5f, 3.8f, te);
    for (int i = 0; i < 6; i++) {                                // o organismo se desfaz em partículas
        float bx = E + (D - E) * (i + 0.5f) / 6;
        if (dissolve < 0.85f)
            tentaculo(bx, 690 + 200 * dissolve, -TAU / 4 + (bx - 512) * 0.0016f, 300 * (1 - dissolve * 0.6f), 18 * (1 - dissolve), T * 3, i * 2.3f,
                      lerp(rgb(24, 12, 26), rgb(255, 150, 90), dissolve), rgb(255, 120, 80), 1);
        for (int k = 0; k < 18; k++) {
            if (te < 0.6f) break;
            float v = clamp01((te - 0.6f - rnd(i * 31 + k, 67)) / 2.6f);
            if (v <= 0 || v >= 1) continue;
            float px = bx + (rnd(i * 31 + k, 68) - 0.5f) * 150 + std::sin(T * 2 + k) * 12;
            float py = 640 - rnd(i * 31 + k, 69) * 240 - 210 * v;
            circ(px, py, 3.5f * (1 - v), rgb(255, 170, 110), (int)(255 * (1 - v)));
        }
    }
    explosao(512, 470, te);
    float x = 760 + 40 * u, y = 190 - 30 * u + std::sin(T * 9) * 6 * clamp01(1 - te / 2) * (te > 0.25f);
    capsula(x, y, 0.62f, 0.7f + 0.2f * std::sin(T * 7) * (te > 0.25f ? clamp01(1 - te / 2.5f) : 0.f), 1, T);
    neve(T, 110, 30, 1);
    if (te > 0 && te < 1.0f) ret((int)E - 20, -20, (int)(D - E) + 40, ALTURA + 40, rgb(255, 250, 235), (int)(255 * (1 - te)));
}

void pulso_calmaria(float u, float T) {
    float h = 370, amanhece = suave(0, 5, u);
    ceu(h, lerp(rgb(18, 24, 60), rgb(100, 120, 200), amanhece), lerp(rgb(120, 60, 90), rgb(255, 180, 140), amanhece));
    estrelas(T, h, 0.9f * (1 - amanhece));
    sol(512, h + 20 - 80 * amanhece, 36, rgb(255, 150, 90), 0.9f);
    oceano(h, T * 0.6f, rgb(60, 70, 120), rgb(10, 16, 44), 512, rgb(255, 170, 130));
    float y = 470 + std::sin(T * 1.7f) * 7;
    float x = 330 + u * 8;
    capsula(x, y, 1.4f, 0.28f + std::sin(T * 1.3f) * 0.09f, 0, T);
    gradiente((int)x - 66, (int)(y + 28), 132, 50, rgb(36, 40, 84), rgb(24, 28, 66), 238);
    for (int k = 0; k < 3; k++) {
        float r = resto(T * 34 + k * 55, 165.0f);
        elipse_borda((int)(x - r), (int)(y + 30 - r * 0.16f), (int)(2 * r), (int)(r * 0.32f), 2, rgb(230, 220, 255), (int)(150 * (1 - r / 165)));
    }
    bool sinal = ((int)(T * 2.5f)) % 2 == 0;                     // farol de resgate
    circ(x, y - 60, 4, rgb(255, 80, 60), sinal ? 255 : 60);
    if (sinal) luz(x, y - 60, 46, rgb(255, 80, 60), 0.9f);
}

// ---------------- Final 2: Guardião
void sala_controle(float T, Cor tom, float forca) {
    mar(T, rgb(14, 22, 38), rgb(5, 7, 14), 0);
    for (int i = 0; i < 9; i++) {
        float x = E + (D - E) * i / 8.0f;
        ret((int)x - 5, 0, 10, ALTURA, rgb(24, 34, 52));
    }
    rret(262, 80, 500, 230, 18, rgb(8, 14, 24));
    rret_borda(262, 80, 500, 230, 18, 6, rgb(54, 74, 100));
    luz(512, 195, 330, tom, 0.28f * forca);
}

void console_painel(float T, Cor c, float ativo) {
    quad(240, 340, 784, 340, 830, 420, 194, 420, rgb(30, 42, 60));
    quad(194, 420, 830, 420, 830, 640, 194, 640, rgb(18, 26, 40));
    linha(240, 340, 784, 340, 3, rgb(90, 120, 150));
    for (int i = 0; i < 36; i++) {
        float x = 272 + (i % 12) * 43.0f, y = 356 + (i / 12) * 20.0f;
        bool on = rnd(i, 71) < ativo && std::sin(T * (2 + 4 * rnd(i, 72)) + i) > -0.5f;
        rret((int)x, (int)y, 24, 9, 3, on ? c : rgb(42, 54, 72));
        if (on) luz(x + 12, y + 4, 24, c, 0.4f);
    }
}

void guard_partida(float u, float T) {
    sala_controle(T, rgb(80, 180, 255), 1);
    // visor: a cápsula vai embora, sozinha, levando os dados
    gradiente(268, 86, 488, 218, rgb(10, 46, 74), rgb(2, 10, 22));
    float p = entra(u / DUR_PLANO);
    float x = 512 + 130 * p, y = 262 - 160 * p;
    bolhas(x, y + 18, T, 26, 18, 90, 13);
    capsula(x, y, 0.8f - 0.55f * p, 0.25f, 1, T);
    for (int i = 0; i < 40; i++) circ(268 + rnd(i, 73) * 488, 86 + resto(rnd(i, 74) * 218 + T * 12, 218.0f), 1.2f, rgb(190, 225, 240), 110);
    rret_borda(262, 80, 500, 230, 18, 6, rgb(54, 74, 100));
    linha(512, 86, 512, 304, 2, rgb(54, 74, 100), 130);
    console_painel(T, rgb(90, 220, 255), 0.55f);
    mergulhador(430, 400, 1.55f, 0);
}

void guard_selo(float u, float T) {
    sala_controle(T, rgb(255, 80, 90), 1);
    gradiente(268, 86, 488, 218, rgb(40, 10, 30), rgb(8, 2, 10));
    float prog = suave(1.4f, 4.6f, u);
    for (int i = 0; i < 5; i++)
        tentaculo(320 + i * 96.0f, 330, -TAU / 4 + (i - 2) * 0.12f, 190 * (1 - prog * 0.55f), 11, T * (2.4f - 2 * prog), i * 1.7f,
                  rgb(26, 10, 24), lerp(rgb(190, 40, 70), rgb(80, 220, 255), prog), 0.8f);
    olho(512, 180, 62 - 8 * prog, 1 - suave(4.0f, 5.0f, u), lerp(rgb(255, 60, 40), rgb(80, 200, 255), prog), std::sin(T * 3) * (1 - prog), T);
    selo(512, 195, 118, prog, T, rgb(90, 220, 255));
    rret_borda(262, 80, 500, 230, 18, 6, rgb(54, 74, 100));
    ret((int)E, 312, (int)(D - E) + 1, 328, rgb(0, 0, 0), 25);
    console_painel(T, lerp(rgb(255, 120, 60), rgb(90, 255, 170), prog), 0.4f + 0.6f * prog);
    float alavanca = sai((u - 0.8f) / 0.7f);
    mergulhador(660, 400, 1.55f, alavanca);
    float ax = 764, ay = 392;                                    // alavanca
    float ang = -2.15f + 1.2f * alavanca;
    linha(ax, ay, ax + std::cos(ang) * 78, ay + std::sin(ang) * 78, 9, rgb(150, 162, 178));
    circ(ax + std::cos(ang) * 78, ay + std::sin(ang) * 78, 14, rgb(255, 80, 60));
    circ(ax, ay, 16, rgb(70, 84, 104));
    if (u > 1.5f && u < 2.0f) ret((int)E, 0, (int)(D - E) + 1, ALTURA, rgb(190, 240, 255), (int)(190 * (1 - (u - 1.5f) / 0.5f)));
}

void guard_repouso(float u, float T) {
    mar(T, rgb(10, 40, 66), rgb(1, 6, 14), 0.9f);
    relevo(580, 140, 15, rgb(7, 22, 38), 3);
    float acende = suave(0.6f, 4.0f, u);
    estacao(512, 235, 0.9f, (int)(12 * acende), rgb(110, 230, 255), 0, T);
    relevo(660, 90, 120, rgb(4, 12, 22), 4);
    selo(512, 452, 80, 1, T * 0.35f, rgb(80, 200, 240));
    olho(482, 452, 24, 0, rgb(80, 200, 255), 0, T);
    olho(542, 452, 24, 0, rgb(80, 200, 255), 0, T);
    for (int i = 0; i < 7; i++) {                                // cardume voltando
        float x = E - 80 + resto(T * 55 + i * 29 + (i % 3) * 14, D - E + 160), y = 190 + i * 11 + std::sin(T * 2 + i) * 9;
        elipse((int)x - 10, (int)y - 4, 20, 8, rgb(150, 220, 235), (int)(210 * acende));
        Pt rabo[3] = {{x - 9, y}, {x - 17, y - 6}, {x - 17, y + 6}};
        poligono(rabo, 3, rgb(150, 220, 235), (int)(210 * acende));
    }
    neve(T, 150, -14, 1);
}

void guard_sinal(float u, float T) {
    mar(T, rgb(4, 10, 20), rgb(1, 3, 8), 0.2f);
    float cx = 512, cy = 285;
    for (int k = 1; k <= 4; k++) circ_borda(cx, cy, k * 55.0f, 1.5f, rgb(60, 190, 160), 90);
    linha(cx - 230, cy, cx + 230, cy, 1, rgb(60, 190, 160), 90);
    linha(cx, cy - 230, cx, cy + 230, 1, rgb(60, 190, 160), 90);
    float varre = T * 1.7f;
    for (int i = 0; i < 26; i++) {
        float a = varre - i * 0.035f;
        linha(cx, cy, cx + std::cos(a) * 220, cy - std::sin(a) * 220, 3, rgb(80, 255, 200), (int)(120 * (1 - i / 26.0f)));
    }
    circ(cx, cy, 7, rgb(255, 170, 60));
    luz(cx, cy, 70, rgb(255, 170, 60), 0.6f + 0.4f * std::sin(T * 3));
    float resposta = suave(2.2f, 3.0f, u);                        // um sinal vindo da superfície responde
    float pulsa = 0.5f + 0.5f * std::sin(T * 6);
    circ(cx + 104, cy - 150, 6, rgb(120, 220, 255), (int)(255 * resposta));
    circ_borda(cx + 104, cy - 150, 10 + 22 * resto(T, 1.0f), 2, rgb(120, 220, 255), (int)(200 * resposta * (1 - resto(T, 1.0f))));
    luz(cx + 104, cy - 150, 50, rgb(120, 220, 255), 0.8f * resposta * pulsa);
}

// ---------------- Final 3: Protocolo Perfeito (secreto)
void perf_ressonancia(float u, float T) {
    mar(T, rgb(8, 10, 34), rgb(2, 2, 10), 0.4f);
    float p = suave(0.2f, 4.8f, u);
    constelacao(512, 300, 195 + 12 * std::sin(T), 9, p, T, rgb(90, 220, 255));
    constelacao(512, 300, 92, 5, suave(1.2f, 5.0f, u), -T * 2, rgb(255, 205, 100));
    luz(512, 300, 110 + 80 * p, rgb(200, 240, 255), 0.6f * p);
    circ(512, 300, 10 + 8 * p, rgb(240, 252, 255), (int)(255 * p));
    neve(T, 110, -30, 1);
}

void perf_despertar(float u, float T) {
    mar(T, rgb(6, 24, 46), rgb(1, 3, 10), 0.4f);
    float muda = suave(1.8f, 3.8f, u);
    Cor iris = lerp(rgb(255, 70, 50), rgb(60, 240, 220), muda);
    for (int i = 0; i < 10; i++) {
        float a = TAU * i / 10 + 0.2f;
        float bx = 512 + std::cos(a) * 430, by = 300 + std::sin(a) * 340;
        tentaculo(bx, by, a + 3.14159f + 0.9f * muda, 230, 20, T * (1.4f - 0.7f * muda), i * 1.3f, lerp(rgb(24, 12, 26), rgb(10, 40, 52), muda),
                  lerp(rgb(170, 40, 80), rgb(60, 240, 220), muda), 0.5f + 0.5f * muda);
    }
    float aberto = suave(0.3f, 1.4f, u) - suave(3.9f, 4.05f, u) + suave(4.15f, 4.4f, u);   // piscada tranquila
    olho(512, 300, 210 * (1 + 0.07f * u / DUR_PLANO), clamp01(aberto), iris, std::sin(u * 1.2f) * 0.35f * (1 - muda), T);
    neve(T, 100, -20, 1);
}

void perf_subida(float u, float T) {
    float clareia = suave(1.0f, DUR_PLANO, u);
    mar(T, lerp(rgb(8, 36, 70), rgb(60, 170, 200), clareia), lerp(rgb(1, 4, 12), rgb(8, 50, 90), clareia), 0.4f + clareia);
    float p = u / DUR_PLANO;
    float cx = 512, cy = 430 - 250 * suave(0, 1, p);
    for (int fita = 0; fita < 4; fita++) {                       // espirais bioluminescentes
        float px = 0, py = 0;
        for (int i = 0; i < 60; i++) {
            float v = i / 59.0f;
            float a = T * 2.4f + fita * TAU / 4 + v * 7;
            float x = cx + std::cos(a) * (50 + 150 * v), y = cy + v * 420 + std::sin(a) * 22;
            if (i) linha(px, py, x, y, 7 * (1 - v) + 1, fita % 2 ? rgb(80, 255, 220) : rgb(120, 180, 255), (int)(210 * (1 - v)));
            if (i % 9 == 0) luz(x, y, 30, rgb(80, 240, 230), 0.55f * (1 - v));
            px = x; py = y;
        }
    }
    luz(cx, cy, 150, rgb(100, 255, 230), 0.4f);
    capsula(cx, cy, 1.15f, std::sin(T * 1.1f) * 0.06f, 0, T);
    bolhas(cx, cy + 40, T, 60, 90, 130, 17);
    neve(T, 140, 160, 1.5f);
}

void perf_aurora(float u, float T) {
    float h = 385;
    ceu(h, rgb(4, 8, 30), rgb(26, 56, 92));
    estrelas(T, h, 1);
    aurora(T, suave(0, 2, u));
    sol(790, 110, 22, rgb(190, 215, 255), 0.5f);
    oceano(h, T * 0.5f, rgb(20, 60, 90), rgb(3, 12, 30), 512, rgb(90, 240, 200));
    // o guardião emerge ao longe e se despede
    float sobe = sai((u - 1.2f) / 2.4f);
    for (int i = 0; i < 5; i++) {
        float bx = 690 + i * 42.0f;
        tentaculo(bx, 470, -TAU / 4 + (i - 2) * 0.22f, 130 * sobe, 9, T * 1.2f, i * 1.9f, rgb(8, 30, 44), rgb(80, 250, 220), 1);
    }
    gradiente(660, 462, 260, 30, rgb(14, 40, 64), rgb(8, 26, 46), 220);
    float y = 478 + std::sin(T * 1.8f) * 6;
    capsula(360, y, 1.35f, std::sin(T * 1.4f) * 0.11f, 0, T);
    gradiente(300, (int)y + 26, 120, 48, rgb(14, 44, 68), rgb(6, 24, 46), 238);
    constelacao(512, 150, 66, 6, suave(2.5f, 5.0f, u), T, rgb(200, 240, 255));
    for (int i = 0; i < 40; i++) {                               // vaga-lumes do mar
        float x = E + rnd(i, 81) * (D - E), yy = h + 40 + rnd(i, 82) * 200;
        float tw = 0.5f + 0.5f * std::sin(T * 2 + i);
        circ(x, yy, 1.6f, rgb(110, 255, 210), (int)(220 * tw));
        if (i % 4 == 0) luz(x, yy, 14, rgb(110, 255, 210), 0.6f * tw);
    }
}

typedef void (*Plano)(float, float);
const Plano PLANOS[NUM_FINAIS][NUM_PLANOS] = {
    {asc_escotilha, asc_lancamento, asc_olho, asc_superficie},
    {pulso_nucleo, pulso_fuga, pulso_explosao, pulso_calmaria},
    {guard_partida, guard_selo, guard_repouso, guard_sinal},
    {perf_ressonancia, perf_despertar, perf_subida, perf_aurora}};

float tremor(int final, float t) {
    if (final == 0) return t < DUR_PLANO && t > 3.2f && t < 3.8f ? 7.f : 0.f;
    if (final == 1) {
        if (t < DUR_PLANO) return 9 * entra(t / DUR_PLANO);
        float te = t - (2 * DUR_PLANO + 0.9f);
        if (te > 0 && te < 2.5f) return 22 * (1 - te / 2.5f);
    }
    if (final == 2 && t > DUR_PLANO + 1.5f && t < DUR_PLANO + 4.6f) return 5;
    return 0;
}

void atualizar_bordas() { E = (float)-base_x(); D = (float)(LARGURA + base_x()); }

}  // namespace

void sons(Audio& som, int final, float antes, float depois) {
    struct Marca { int final; float t; const char* nome; float vol; };
    const float P = DUR_PLANO;
    static const Marca marcas[] = {
        {0, 0.1f, "porta", 0.7f}, {0, 3.2f, "porta", 0.5f}, {0, P + 0.4f, "explosao", 0.3f}, {0, 2 * P + 1.0f, "rugido", 0.8f},
        {0, 3 * P + 0.95f, "respingo", 0.7f}, {0, 3 * P + 2.85f, "respingo", 0.5f},
        {1, 0.3f, "carga", 0.6f}, {1, P + 0.5f, "porta", 0.4f}, {1, P + 1.4f, "rugido", 0.6f}, {1, 2 * P + 0.9f, "explosao", 1.0f},
        {1, 2 * P + 1.2f, "rugido", 0.8f}, {1, 3 * P + 1.5f, "sino", 0.5f},
        {2, 1.0f, "porta", 0.4f}, {2, P + 1.0f, "porta", 0.7f}, {2, P + 1.5f, "carga", 0.4f}, {2, P + 2.0f, "rugido", 0.7f},
        {2, 3 * P + 0.5f, "bipe", 0.25f}, {2, 3 * P + 2.4f, "sino", 0.6f},
        {3, 0.4f, "sino", 0.6f}, {3, 2.2f, "sino", 0.5f}, {3, P + 0.5f, "rugido", 0.5f}, {3, P + 3.0f, "sino", 0.8f},
        {3, 3 * P + 1.2f, "respingo", 0.3f}, {3, 3 * P + 2.8f, "sino", 0.7f}};
    for (const auto& m : marcas)
        if (m.final == final && antes < m.t && depois >= m.t) som.tocar(m.nome, m.vol);
    if (antes < INICIO_CARTAO && depois >= INICIO_CARTAO) som.tocar("fase", 0.35f);
}

void desenhar(int final, float t, int descobertos) {
    final = limitar(final, 0, NUM_FINAIS - 1);
    atualizar_bordas();
    const float vt = std::min(t, INICIO_CARTAO + 60.0f);
    int plano = std::min(NUM_PLANOS - 1, (int)(vt / DUR_PLANO));
    float u = vt - plano * DUR_PLANO;
    float uc = std::min(u, DUR_PLANO + 0.0f);                      // no cartão, o último plano continua vivo
    if (plano == NUM_PLANOS - 1) uc = u;

    float amp = tremor(final, vt);
    int ox = 0, oy = 0;
    if (amp > 0) {
        ox = (int)(std::sin(vt * 71) * amp * 0.6f + std::sin(vt * 113) * amp * 0.4f);
        oy = (int)(std::cos(vt * 89) * amp * 0.7f);
    }
    origem(0, 0);
    ret((int)E, 0, (int)(D - E) + 1, ALTURA, rgb(0, 0, 0));
    origem(ox, oy);
    PLANOS[final][plano](std::min(uc, DUR_PLANO + 3.0f), vt);
    origem(0, 0);
    vinheta(0.85f);

    // transições: entra e sai do preto a cada corte
    float escuro = 1 - suave(0, 0.7f, u);
    if (plano < NUM_PLANOS - 1) escuro = std::max(escuro, suave(DUR_PLANO - 0.5f, DUR_PLANO, u));
    if (plano > 0 || vt < 1) {
        if (escuro > 0) ret((int)E, 0, (int)(D - E) + 1, ALTURA, rgb(0, 0, 0), (int)(255 * escuro));
    }

    // faixas de cinema
    int barra = (int)(58 * sai(t / 1.2f));
    ret((int)E, 0, (int)(D - E) + 1, barra, rgb(0, 0, 0));
    ret((int)E, ALTURA - barra, (int)(D - E) + 1, barra, rgb(0, 0, 0));

    const Final& fin = final_cfg(final);
    if (t < INICIO_CARTAO) {
        int al = (int)(255 * suave(0.5f, 1.3f, u) * (1 - suave(DUR_PLANO - 0.7f, DUR_PLANO - 0.2f, u)));
        const auto& linhas = quebrar(fin.legendas[plano], F_TEXTO, 820);
        int n = (int)linhas.size();
        int y0 = ALTURA - 58 - 14 - n * 30;
        gradiente((int)E, y0 - 46, (int)(D - E) + 1, 46, rgb(0, 0, 0), rgb(0, 0, 0), al * 70 / 255);
        ret((int)E, y0, (int)(D - E) + 1, ALTURA - 58 - y0, rgb(0, 0, 0), al * 120 / 255);
        for (int i = 0; i < n; i++) texto_cine(linhas[i], F_TEXTO, rgb(238, 243, 248), 512, y0 + 6 + i * 30, al);
        texto_cine(maiusc(fin.titulo), F_HUD_V, rgb(250, 200, 90), 512, 15, (int)(255 * suave(0.8f, 2.0f, t)), 4);
    } else {                                                       // cartão final
        float c = t - INICIO_CARTAO;
        ret((int)E, barra, (int)(D - E) + 1, ALTURA - 2 * barra, rgb(0, 4, 10), (int)(185 * suave(0, 1.4f, c)));
        float abre = sai(c / 1.8f);
        std::wstring nome = fin.nome_curto;
        texto_cine(L"FINAL " + a_wstr(final + 1) + L" DE " + a_wstr(NUM_FINAIS), F_HUD_V, rgb(130, 200, 220), 512, 172,
                   (int)(255 * suave(0.2f, 1.0f, c)), 5);
        luz(512, 250, 330 * abre, final == 3 ? rgb(120, 255, 220) : rgb(70, 170, 220), 0.22f);
        texto_cine(nome, F_GIGANTE, rgb(245, 250, 255), 512, 214, (int)(255 * suave(0.4f, 1.6f, c)), (int)(2 + 8 * abre));
        int meia = (int)(230 * abre);
        ret(512 - meia, 296, 2 * meia, 2, rgb(250, 190, 70), (int)(255 * suave(0.5f, 1.2f, c)));
        int a2 = (int)(255 * suave(1.1f, 2.1f, c));
        int y = 320;
        for (const auto& l : quebrar(fin.epilogo, F_TEXTO, 760)) {
            texto_cine(l, F_TEXTO, rgb(196, 218, 232), 512, y, a2);
            y += 31;
        }
        texto_cine(L"Finais descobertos: " + a_wstr(descobertos) + L" de " + a_wstr(NUM_FINAIS), F_PEQ, rgb(250, 200, 90), 512, y + 22, a2);
    }
}

void miniatura(int final, int x, int y, int w, int h, float t) {
    // pequenas ilustrações desenhadas direto no cartão (sem sair da área)
    float cx = x + w / 2.0f, cy = y + h / 2.0f;
    if (final == 0) {
        gradiente(x, y, w, h, rgb(40, 120, 170), rgb(3, 14, 30));
        for (int i = 0; i < 12; i++) {
            float v = resto(t * 0.45f + rnd(i, 91), 1.0f);
            circ_borda(cx + (rnd(i, 92) - 0.5f) * 26, cy + 22 + v * (h / 2.0f - 28), 1.5f + 2 * v, 1, rgb(190, 235, 250), (int)(220 * (1 - v)));
        }
        capsula(cx, cy - 8 + std::sin(t * 2) * 4, 0.85f, std::sin(t) * 0.06f, 0.55f, t);
    } else if (final == 1) {
        gradiente(x, y, w, h, rgb(60, 22, 14), rgb(12, 5, 8));
        float s = std::min(w, h) / 420.0f;
        circ_borda(cx, cy, 130 * s, 3, rgb(150, 110, 40));
        for (int k = 0; k < 2; k++)
            for (int i = 0; i < 5; i++) {
                float a = t * (1.4f + k) * (k ? -1 : 1) + TAU * i / 5;
                float r = (105 - k * 40) * s;
                arco(cx, cy, r, r, a, a + 0.8f, 4, rgb(255, 180, 70));
            }
        circ(cx, cy, 22 * s * (1 + 0.15f * std::sin(t * 8)), rgb(255, 230, 160));
        luz(cx, cy, 46, rgb(255, 140, 40), 0.8f);
    } else if (final == 2) {
        gradiente(x, y, w, h, rgb(12, 36, 62), rgb(3, 8, 18));
        selo(cx, cy, std::min(w, h) * 0.32f, 1, t, rgb(90, 220, 255));
        mergulhador(cx, cy - 4, 0.5f, 0);
    } else {
        gradiente(x, y, w, h, rgb(14, 20, 56), rgb(3, 4, 14));
        constelacao(cx, cy, std::min(w, h) * 0.36f, 7, 1, t, rgb(110, 240, 230));
        olho(cx, cy, 30, 0.85f, rgb(60, 240, 220), std::sin(t) * 0.4f, t);
    }
}

}  // namespace cinema
