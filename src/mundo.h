// Protocolo Cronos++ - cenário: terminais, porta, fase, jogador e iluminação
#pragma once
#include <algorithm>
#include <vector>
#include "dados.h"
#include "entrada.h"
#include "gfx.h"

using gfx::Ret;

struct Terminal {
    std::wstring nome;
    Ret rect;
    bool feita = false;
    const Questao* questao = nullptr;
    void desenhar(float t) const;
};

struct Porta {
    Ret rect;
    bool aberta = false;
    float prog = 0;
    void abrir() { aberta = true; }
    void atualizar(float dt) { if (aberta) prog = std::min(1.0f, prog + dt / 1.2f); }
    bool solida() const { return !(aberta && prog > 0.55f); }
    bool passavel() const { return aberta && prog >= 0.55f; }
    void desenhar(float t) const;
};

class Fase {
public:
    explicit Fase(int idx);
    int idx;
    Porta porta;
    std::vector<Terminal> terminais;
    std::vector<Ret> solidos;
    int inicio_x, inicio_y;
    char grade[ROWS][COLS + 1];
    gfx::Superficie estatico;
    int acertos() const;
    int meta() const { return info_fase(idx).meta; }
    std::vector<Ret> colisores() const;

private:
    std::vector<std::pair<char, Ret>> objetos;
    void renderizar();
    void desenhar_objeto(char ch, const Ret& r, Rng& rng);
};

class Jogador {
public:
    explicit Jogador(int cx, int cy);
    Ret rect;
    float px, py;
    float dir_x = 1, dir_y = 0;
    float anim = 0;
    bool movendo = false, correndo = false;
    void atualizar(float dt, const std::vector<Ret>& colisores, bool pode_mover);
    void desenhar() const;
};

class Iluminacao {
public:
    Iluminacao();
    // Escurece o mundo (coordenadas do mundo, com a origem gfx já posicionada).
    void aplicar(const Jogador& j, const Fase& f, float luz_dx, float luz_dy, float t);

private:
    struct Luz { int r; std::vector<uint8_t> lut; Luz(int raio, int alfa_max); };
    Luz lanterna, ambiente, brilho;
    std::vector<uint8_t> alfa;
    void subtrair(const Luz& l, int cx, int cy);
};
