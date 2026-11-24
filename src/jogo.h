// Protocolo Cronos++ - máquina de estados, regras, interface e laço de jogo
#pragma once
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "audio.h"
#include "dados.h"
#include "entrada.h"
#include "gfx.h"
#include "mundo.h"

enum Estado { MENU, INTRO, JOGANDO, ESCOLHA_FINAL, VITORIA, DERROTA, CINEMA };

// Recordes e preferências salvos entre partidas.
struct Dados {
    int melhor_pontos = 0;
    int melhor_tempo = 0;       // tempo restante da melhor partida (s)
    std::wstring melhor_letra = L"-";
    int partidas = 0, vitorias = 0;
    float volume = 0.7f;
    bool mudo = false;
};
// Onde os recordes ficam guardados (arquivo no desktop, localStorage no navegador).
struct Armazenamento {
    virtual ~Armazenamento() {}
    virtual bool ler(std::string& texto) = 0;
    virtual void gravar(const std::string& texto) = 0;
};
void carregar_dados(Armazenamento* a, Dados& d);
void salvar_dados(Armazenamento* a, const Dados& d);

std::wstring expressao_para_texto(int v);                    // inteiro -> texto
std::wstring formatar_tempo(int segundos);                   // mm:ss

class Baralho {
public:
    explicit Baralho(int numero_fase);
    const Questao* sortear(const std::vector<std::wstring>& em_uso);
private:
    int numero;
    std::vector<const Questao*> restantes;
    void reabastecer(const std::vector<std::wstring>& excluir);
};

struct Botao {
    gfx::Ret rect;
    std::wstring rot;
    gfx::Cor cor;
    Botao() : cor(0) {}
    Botao(gfx::Ret r, std::wstring t, gfx::Cor c = gfx::rgb(20, 90, 120)) : rect(r), rot(std::move(t)), cor(c) {}
    bool clicado(const Evento& e) const { return e.tipo == Ev::MouseDown && e.botao == 1 && rect.contem(e.x, e.y); }
    void desenhar(gfx::Fonte f) const;
};

class TecladoTela {
public:
    struct Def { std::wstring rotulo, valor; int cols; };
    TecladoTela() {}
    TecladoTela(std::vector<std::vector<Def>> linhas, int tw = 44, int th = 38, int espaco = 4);
    void tamanho(int& w, int& h) const;
    void posicionar(int x, int y);
    std::wstring tocar(int px, int py, float agora);
    void desenhar(gfx::Fonte f, gfx::Cor fundo, gfx::Cor borda, gfx::Cor texto, const std::set<std::wstring>& apagadas, float agora) const;
    struct Tecla { gfx::Ret rect; std::wstring rotulo, valor; };
    std::vector<Tecla> teclas;
private:
    std::vector<std::vector<Def>> linhas;
    int tw = 44, th = 38, espaco = 4;
    std::wstring apertada;
    float apertada_t = -10;
};

class BlocoNotas {
public:
    BlocoNotas();
    static gfx::Ret RECT() { return gfx::Ret(LARGURA - 276, 72, 260, 500); }
    bool aberto = false;
    bool pode_responder = false;
    TecladoTela teclado;
    std::vector<std::pair<std::wstring, int>> linhas;     // (texto, tipo) 0 = digitado, 1 = resultado, 2 = erro
    void reiniciar();
    std::wstring ultimo_resultado() const;
    std::wstring resultado_inteiro() const;               // "" se não for inteiro não negativo
    void calcular();
    void apagar();
    void digitar(wchar_t ch);
    // Devolve true quando o jogador pede para usar o resultado no terminal.
    bool evento(const Evento& e, float agora);
    void desenhar(float t) const;
};

class Jogo;
class Desafio {
public:
    Desafio(Jogo* j, Terminal* t);
    Jogo* jogo;
    Terminal* terminal;
    std::wstring entrada;
    bool feedback = false;
    bool acertou = false;
    const Questao* resolvida = nullptr;
    const Questao* questao() const { return terminal->questao; }
    void evento(const Evento& e);
    void enviar();
    void novo_enunciado();
    void desenhar(float t);
    gfx::Ret painel_rect() const;
private:
    gfx::Ret btn, btn_notas, btn_sair;
    TecladoTela teclado;
    void layout();
    void botao(const gfx::Ret& r, const std::wstring& txt, gfx::Cor cor) const;
};

// Joystick virtual + botões de ação (celular/tablet). Cada dedo é rastreado separadamente.
struct ControlesToque {
    enum Acao { NENHUMA, E, N, JOY };
    bool ativo = false;
    int dedo_joy = -2;
    float vx = 0, vy = 0, kx = 130, ky = ALTURA - 130;
    const float raio = 74, bx = 130, by = ALTURA - 130;
    const float ex = LARGURA - 120, ey = ALTURA - 130, re = 58;
    const float nx = LARGURA - 250, ny = ALTURA - 80, rn = 38;
    void soltar_tudo();
    Acao apertar(int dedo, int x, int y);
    void mover(int dedo, int x, int y);
    void soltar(int dedo);
    void desenhar(bool perto_terminal) const;
};

struct Particula { float x, y, vx, vy, vida; gfx::Cor cor; };
struct Destroco { float x, y, vx, vy, w, h, ang; };

class Jogo {
public:
    Jogo(Audio& som, Armazenamento* armazenamento);

    // --- entrada / ciclo ---
    void tratar_evento(const Evento& e);
    void atualizar(float dt);
    void desenhar();
    void perdeu_foco();      // pausa automaticamente ao sair da janela
    bool rodando = true;

    // --- estado (público para os testes automáticos) ---
    Audio& som;
    Estado estado = MENU;
    float t = 0;
    float tempo = TEMPO_TOTAL;
    int erros = 0, acertos_total = 0, fase_idx = 0;
    std::vector<Baralho> baralhos;
    std::unique_ptr<Fase> fase;
    std::unique_ptr<Jogador> jogador;
    std::unique_ptr<Desafio> desafio;
    BlocoNotas notas;
    Iluminacao luz;
    bool pausado = false;
    int final_escolhido = -1;
    std::wstring derrota_pendente, motivo_derrota;
    float flash_verm = 0, flash_verde = 0, tremor = 0;
    std::wstring aviso;
    float aviso_t = 0;
    float intro_t = 0, intro_saida = -1;
    float cinema_t = 0;
    Dados dados;
    Armazenamento* arm = nullptr;
    ControlesToque toque;
    bool novo_recorde = false;

    void novo_jogo(bool iniciar = true);
    void carregar_fase(int idx);
    void processar_resposta(Terminal* t, bool acertou);
    void continuar_desafio(Desafio* d);
    void encerrar(Estado e, const std::wstring& motivo = L"");
    void interagir();
    Terminal* terminal_proximo();
    void avisar(const std::wstring& txt, float dur = 3.0f) { aviso = txt; aviso_t = dur; }
    struct Classif { std::wstring letra, titulo; gfx::Cor cor; int pontos; };
    Classif classificacao() const;

private:
    friend class Desafio;
    std::vector<Particula> bolhas;       // x, y, raio, velocidade (vx=raio, vy=vel)
    std::vector<Particula> fogos;
    std::vector<Destroco> destrocos;
    std::vector<Particula> cinema_bolhas;
    std::set<std::string> cinema_eventos;
    Botao btn_iniciar, btn_novo, btn_fugir, btn_sobrecarga, btn_continuar, btn_menu;
    gfx::Ret r_som, r_pausa;
    bool arrastando_vol = false;
    int dedo_vol = -3;
    bool apertar(int dedo, int x, int y);
    float passo_t = 0;
    float luz_dx = 1, luz_dy = 0;
    bool perto_terminal = false;

    void atualizar_jogo(float dt);
    void atualizar_cinema(float dt);
    void iniciar_cinema(int final);
    float estacao_y() const { return 430 + cinema_t * 15; }
    void capsula_pos(float& x, float& y) const;
    void salvar();

    void desenhar_fundo_menu();
    void desenhar_menu();
    void desenhar_intro();
    void desenhar_hud();
    void desenhar_jogo();
    void desenhar_botoes_sistema();
    void desenhar_volume();
    void desenhar_escolha();
    void desenhar_vitoria();
    void desenhar_fim();
    void desenhar_cinema();
    void desenhar_estacao(float sy, int luzes, float brilho);
    void desenhar_capsula(float cx, float cy);
    void rotulo(const std::wstring& txt, int cx, int cy, gfx::Cor cor, bool pequeno = false);
    bool r_volume(gfx::Ret& r) const;
    void ajustar_volume(int x);
};

// Utilitário de interface: painel arredondado translúcido com borda.
void painel(const gfx::Ret& r, gfx::Cor cor = gfx::rgb(10, 20, 32), gfx::Cor borda = gfx::rgb(70, 220, 235), int alfa = 235, int raio = 12);
void veu(int alfa);
