#include <algorithm>
#include "mundo.h"

using namespace gfx;

namespace {
const Cor PRETO = rgb(0, 0, 0);
}

// ------------------------------------------------------------------ Terminal
void Terminal::desenhar(float t) const {
    const Ret& r = rect;
    rret(r.mover(2, 3), 5, PRETO);
    rret(r, 5, rgb(26, 32, 42));
    rret_borda(r, 5, 2, rgb(88, 100, 118));
    Ret tela = r.inflar(-14, -12);
    rret(tela, 3, feita ? rgb(14, 70, 36) : rgb(14, 52, 66));
    Cor cor_linha = feita ? rgb(90, 240, 140) : rgb(80, 200, 225);
    for (int i = 0; i < 3; i++) {
        int larg = tela.w - 6 - (i * 9 % 14);
        linha((float)tela.x + 3, (float)tela.y + 3 + i * 4, (float)tela.x + 3 + larg, (float)tela.y + 3 + i * 4, 1, cor_linha);
    }
    float pulso = 0.55f + 0.45f * std::sin(t * 4.5f + r.x);
    Cor cor = feita ? rgb(70, 255, 120) : rgb(255, 60, 50);
    circ((float)r.dir() - 2, (float)r.y + 6, 17, cor, (int)(90 * pulso));
    circ((float)r.dir() - 2, (float)r.y + 6, 10, cor, (int)(93 * pulso));
    circ((float)r.dir() - 2, (float)r.y + 6, 4, cor);
    circ((float)r.dir() - 3, (float)r.y + 5, 1, rgb(255, 255, 255));
}

// ------------------------------------------------------------------ Porta
void Porta::desenhar(float t) const {
    const Ret& r = rect;
    ret(r, rgb(8, 10, 14));
    if (prog > 0) ret(r.inflar(-6, -2), rgb(180, 255, 220));
    float meia = TILE * (1 - prog);
    for (int topo = 1; topo >= 0; topo--) {
        Ret p = topo ? Ret(r.x, r.y, TILE, (int)meia) : Ret(r.x, (int)(r.baixo() - meia), TILE, (int)meia);
        if (p.h <= 0) continue;
        ret(p, rgb(96, 108, 122));
        for (int k = -10; k < TILE; k += 10) {        // listras de aviso (recortadas na folha)
            float x0 = (float)(p.x + k);
            float t0 = std::max(0.f, (p.x - x0) / 10), t1 = std::min(1.f, (p.dir() - x0) / 10);
            if (t0 >= t1) continue;
            float ya = (float)p.baixo(), yb = (float)p.y;
            linha(x0 + 10 * t0, ya + (yb - ya) * t0, x0 + 10 * t1, ya + (yb - ya) * t1, 3, rgb(230, 190, 40));
        }
        ret_borda(p, 2, rgb(40, 48, 58));
    }
    ret_borda(r, 3, rgb(60, 68, 80));
    Cor cor = aberta ? rgb(70, 255, 120) : rgb(255, 60, 50);
    if (!aberta && (int)(t * 2) % 2 == 0) cor = rgb(140, 30, 30);
    circ((float)r.x - 4, (float)r.cy(), 4, cor);
}

// ------------------------------------------------------------------ Fase
Fase::Fase(int i) : idx(i), estatico(LARGURA, ROWS * TILE) {
    const MapaCfg& cfg = mapa_cfg(idx);
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) grade[r][c] = (r == 0 || r == ROWS - 1 || c == 0 || c == COLS - 1) ? '#' : '.';
        grade[r][COLS] = 0;
    }
    for (const auto& s : cfg.solidos) {
        if (s.ch == '#') {
            for (int rr = s.r; rr < s.r + s.h; rr++)
                for (int cc = s.c; cc < s.c + s.w; cc++) grade[rr][cc] = '#';
        } else {
            objetos.push_back({s.ch, Ret(s.c * TILE, s.r * TILE, s.w * TILE, s.h * TILE)});
        }
    }
    porta.rect = Ret(COLS * TILE - TILE, cfg.porta * TILE, TILE, TILE * 2);
    for (int dr = 0; dr < 2; dr++) grade[cfg.porta + dr][COLS - 1] = 'D';
    for (const auto& t : cfg.terminais) {
        Terminal term;
        term.nome = t.nome;
        term.rect = Ret(t.c * TILE, t.r * TILE, TILE * 2, TILE);
        terminais.push_back(term);
    }
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (grade[r][c] == '#') solidos.push_back(Ret(c * TILE, r * TILE, TILE, TILE));
    for (auto& o : objetos) {
        solidos.push_back(o.second);
        if (o.first == 'M') mesas.push_back(o.second);
    }
    for (auto& t : terminais) solidos.push_back(t.rect);
    inicio_x = cfg.ic * TILE + TILE / 2;
    inicio_y = cfg.ir * TILE + TILE / 2;
    renderizar();
}

int Fase::acertos() const {
    int n = 0;
    for (auto& t : terminais) n += t.feita;
    return n;
}

std::vector<Ret> Fase::colisores() const {
    std::vector<Ret> v = solidos;
    if (porta.solida()) v.push_back(porta.rect);
    return v;
}

void Fase::renderizar() {
    alvo(estatico);
    origem(0, 0);
    Rng rng(idx * 131 + 7);
    const Paleta& pal = paleta(idx);
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int x = c * TILE, y = r * TILE;
            if (grade[r][c] == '#') {
                ret(x, y, TILE, TILE, pal.parede);
                ret_borda(x + 1, y + 1, TILE - 2, TILE - 2, 1, pal.parede_luz);
                if (r + 1 < ROWS && grade[r + 1][c] != '#') {
                    ret(x, y + TILE - 6, TILE, 6, pal.parede_luz);
                    ret(x, y + TILE - 1, TILE, 1, pal.destaque);
                }
                const int dots[4][2] = {{4, 4}, {TILE - 5, 4}, {4, TILE - 12}, {TILE - 5, TILE - 12}};
                for (auto& d : dots) circ((float)x + d[0], (float)y + d[1], 1, somar(pal.parede, 18));
            } else {
                int v = rng.ent(-5, 5);
                Cor base = ((r + c) % 2 == 0) ? pal.piso : pal.piso2;
                ret(x, y, TILE, TILE, somar(base, v));
                ret_borda(x, y, TILE, TILE, 1, somar(base, -12));
                const int dots[4][2] = {{3, 3}, {TILE - 4, 3}, {3, TILE - 4}, {TILE - 4, TILE - 4}};
                for (auto& d : dots) circ((float)x + d[0], (float)y + d[1], 1, somar(base, 22));
                if (rng.f() < 0.08f)                       // grade de ventilação
                    for (int k = 6; k < TILE - 4; k += 5)
                        linha((float)x + 6, (float)y + k, (float)x + TILE - 7, (float)y + k, 2, somar(base, -22));
                if (rng.f() < 0.05f)                       // arranhões
                    linha((float)x + rng.ent(2, 28), (float)y + rng.ent(2, 28), (float)x + rng.ent(2, 28), (float)y + rng.ent(2, 28), 1, somar(base, 14));
                if (r > 0 && grade[r - 1][c] == '#') ret(x, y, TILE, 8, PRETO, 70);     // sombra sob a parede
            }
        }
    }
    int pl = mapa_cfg(idx).porta;                        // faixa de aviso diante da porta
    for (int dr = 0; dr < 2; dr++)
        for (int c = COLS - 3; c <= COLS - 2; c++) {
            for (int k = 0; k < TILE; k += 8)
                linha((float)c * TILE + k, (float)(pl + dr) * TILE + TILE, (float)c * TILE + k + 8, (float)(pl + dr) * TILE, 3, rgb(200, 160, 30));
            ret_borda(c * TILE, (pl + dr) * TILE, TILE, TILE, 1, rgb(30, 30, 30));
        }
    for (auto& o : objetos) desenhar_objeto(o.first, o.second, rng);
    alvo_tela();
}

void Fase::desenhar_objeto(char ch, const Ret& r, Rng& rng) {
    Cor destaque = paleta(idx).destaque;
    rret(r.mover(3, 4), 4, PRETO);
    if (ch == 'M') {                                                   // mesa de laboratório
        rret(r, 4, rgb(84, 78, 72));
        rret(r.inflar(-6, -6), 3, rgb(112, 106, 98));
        rret_borda(r, 4, 2, rgb(50, 46, 42));
        const Cor cores[3] = {rgb(90, 220, 200), rgb(240, 190, 70), rgb(230, 90, 110)};
        for (int i = 0; i < std::max(1, r.w / 40); i++) {
            int bx = r.x + 12 + i * 38;
            circ((float)bx, (float)r.y + 14, 6, cores[rng.ent(0, 2)]);
            circ((float)bx - 2, (float)r.y + 12, 2, rgb(240, 250, 255));
        }
        Ret mon(r.dir() - 30, r.baixo() - 20, 20, 12);
        ret(mon, rgb(20, 30, 40));
        ret(mon.inflar(-4, -4), destaque);
    } else if (ch == 'B') {                                            // tanque cilíndrico
        float cx = (float)r.cx(), cy = (float)r.cy();
        int raio = std::min(r.w, r.h) / 2 - 2;
        circ(cx, cy, (float)raio, rgb(70, 80, 92));
        circ(cx, cy, (float)raio - 4, rgb(40, 130, 150));
        circ(cx, cy, (float)raio - 9, rgb(90, 200, 215));
        circ(cx - raio / 3, cy - raio / 3, (float)std::max(2, raio / 5), rgb(210, 250, 255));
        for (int i = 0; i < 4; i++)
            circ(cx + rng.ent(-raio / 2, raio / 2), cy + rng.ent(-raio / 2, raio / 2), 1, rgb(200, 245, 255));
    } else if (ch == 'K') {                                            // caixote
        rret(r, 3, rgb(118, 88, 52));
        rret_borda(r, 3, 3, rgb(86, 62, 34));
        linha((float)r.x, (float)r.y, (float)r.dir(), (float)r.baixo(), 3, rgb(86, 62, 34));
        linha((float)r.dir(), (float)r.y, (float)r.x, (float)r.baixo(), 3, rgb(86, 62, 34));
    } else if (ch == 'R') {                                            // núcleo do reator
        rret(r, 10, rgb(46, 40, 40));
        rret_borda(r, 10, 3, rgb(200, 150, 40));
        float cx = (float)r.cx(), cy = (float)r.cy();
        const struct { int raio; Cor cor; } an[4] = {{54, rgb(90, 50, 20)}, {42, rgb(180, 80, 20)}, {28, rgb(255, 140, 40)}, {14, rgb(255, 230, 150)}};
        for (auto& a : an) circ(cx, cy, (float)a.raio, a.cor);
        for (int k = 0; k < TILE * 5; k += 10) {
            linha((float)r.x + k, (float)r.y, (float)r.x + k + 6, (float)r.y, 3, rgb(230, 190, 40));
            linha((float)r.x + k, (float)r.baixo() - 1, (float)r.x + k + 6, (float)r.baixo() - 1, 3, rgb(230, 190, 40));
        }
    }
}

// ------------------------------------------------------------------ Jogador
Jogador::Jogador(int cx, int cy) : rect(cx - 11, cy - 11, 22, 22), px((float)cx), py((float)cy) {}

void Jogador::atualizar(float dt, const std::vector<Ret>& colisores, bool pode_mover) {
    float dx = 0, dy = 0;
    if (pode_mover) {
        const bool* k = entrada.tec;
        dx = (float)((k['D'] || k[K_RIGHT]) - (k['A'] || k[K_LEFT]));
        dy = (float)((k['S'] || k[K_DOWN]) - (k['W'] || k[K_UP]));
        dx += entrada.joy_x;
        dy += entrada.joy_y;
    }
    float n = std::hypot(dx, dy);
    movendo = n > 0.15f;
    if (!movendo) { anim *= 0.8f; correndo = false; return; }
    correndo = pode_mover && entrada.tec[K_SHIFT];
    float forca = std::min(1.0f, n);
    dx /= n; dy /= n;
    dir_x = dx; dir_y = dy;
    float fator = correndo ? FATOR_CORRIDA : 1.0f;
    anim += dt * 11 * forca * fator;
    float vel = VEL_JOGADOR * forca * fator;
    // eixos X e Y separados para deslizar nas paredes
    px += dx * vel * dt;
    rect.x = (int)std::lround(px) - 11;
    for (const auto& s : colisores)
        if (rect.colide(s)) {
            if (dx > 0) rect.x = s.x - rect.w;
            else if (dx < 0) rect.x = s.dir();
            px = (float)(rect.x + 11);
        }
    py += dy * vel * dt;
    rect.y = (int)std::lround(py) - 11;
    for (const auto& s : colisores)
        if (rect.colide(s)) {
            if (dy > 0) rect.y = s.y - rect.h;
            else if (dy < 0) rect.y = s.baixo();
            py = (float)(rect.y + 11);
        }
}

void Jogador::desenhar() const {
    float cx = (float)(rect.x + 11), cy = (float)(rect.y + 11);
    float fx = dir_x, fy = dir_y, px_ = -fy, py_ = fx;
    float balanco = movendo ? std::sin(anim) : 0.f;
    elipse((int)cx - 13, (int)cy - 6, 26, 20, PRETO, 150);                               // sombra
    for (int lado = -1; lado <= 1; lado += 2) {                                          // pernas
        float o = balanco * lado * 5;
        circ(cx + px_ * lado * 6 + fx * o, cy + py_ * lado * 6 + fy * o, 5, rgb(210, 120, 20));
    }
    circ(cx - fx * 9, cy - fy * 9, 7, rgb(60, 70, 82));                                  // cilindro de ar
    circ_borda(cx - fx * 9, cy - fy * 9, 7, 1, rgb(110, 125, 140));
    for (int lado = -1; lado <= 1; lado += 2) {                                          // braços
        float o = -balanco * lado * 5;
        circ(cx + px_ * lado * 11 + fx * o, cy + py_ * lado * 11 + fy * o, 4, rgb(235, 140, 30));
    }
    circ(cx, cy, 11, rgb(250, 150, 30));                                                 // torso
    circ_borda(cx, cy, 11, 2, rgb(170, 90, 10));
    float hx = cx + fx * 3, hy = cy + fy * 3;
    circ(hx, hy, 8, rgb(205, 215, 225));                                                 // capacete
    circ(hx + fx * 4, hy + fy * 4, 5, rgb(60, 200, 230));                                // visor
    circ(hx + fx * 5 - px_ * 1.5f, hy + fy * 5 - py_ * 1.5f, 1, rgb(230, 255, 255));
}

// ------------------------------------------------------------------ Iluminação
Iluminacao::Luz::Luz(int raio, int alfa_max) : r(raio), lut((size_t)raio * raio * 4, 0) {
    for (int y = 0; y < 2 * raio; y++)
        for (int x = 0; x < 2 * raio; x++) {
            float dx = x + 0.5f - raio, dy = y + 0.5f - raio;
            float d = std::sqrt(dx * dx + dy * dy) / raio;
            if (d < 1) lut[(size_t)y * 2 * raio + x] = (uint8_t)(alfa_max * std::pow(1 - d * d, 0.9f));
        }
}

Iluminacao::Iluminacao() : lanterna(250, 240), ambiente(70, 240), brilho(64, 220), alfa((size_t)LARGURA * ROWS * TILE) {}

void Iluminacao::subtrair(const Luz& l, int cx, int cy) {
    int x0 = cx - l.r, y0 = cy - l.r, W = LARGURA, H = ROWS * TILE;
    for (int j = 0; j < 2 * l.r; j++) {
        int y = y0 + j;
        if ((unsigned)y >= (unsigned)H) continue;
        int i0 = std::max(0, -x0), i1 = std::min(2 * l.r, W - x0);
        const uint8_t* s = &l.lut[(size_t)j * 2 * l.r];
        uint8_t* d = &alfa[(size_t)y * W + x0];
        for (int i = i0; i < i1; i++) d[i] = (uint8_t)std::max(0, (int)d[i] - (int)s[i]);
    }
}

void Iluminacao::aplicar(const Jogador& j, const Fase& f, float luz_dx, float luz_dy, float t) {
    std::fill(alfa.begin(), alfa.end(), (uint8_t)238);
    int cx = j.rect.cx(), cy = j.rect.cy();
    // lanterna (círculo deslocado para a frente = cone aproximado) + luz ambiente do capacete
    subtrair(lanterna, (int)(cx + luz_dx * 60), (int)(cy + luz_dy * 60));
    subtrair(ambiente, cx, cy);
    for (const auto& term : f.terminais) subtrair(brilho, term.rect.cx(), term.rect.cy());
    for (const auto& mesa : f.mesas) subtrair(brilho, mesa.cx(), mesa.cy());
    subtrair(brilho, f.porta.rect.cx(), f.porta.rect.cy());
    (void)t;
    escurecer(alfa.data(), LARGURA, ROWS * TILE, 0, 0, rgb(0, 4, 10));
}
