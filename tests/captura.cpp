// Gera capturas (BMP) de cada tela do jogo para conferência visual: captura.exe <pasta>
#include <cstdio>
#include <string>
#include "../src/audio.h"
#include "../src/jogo.h"

static void tecla(Jogo& j, int k) { Evento e; e.tipo = Ev::Tecla; e.tecla = k; j.tratar_evento(e); }
static void passos(Jogo& j, int n) { for (int i = 0; i < n; i++) j.atualizar(1.0f / 60); }
static void foto(Jogo& j, const std::string& pasta, const char* nome) {
    j.desenhar();
    gfx::salvar_bmp((pasta + "/" + nome + ".bmp").c_str());
}

int main(int argc, char** argv) {
    std::string pasta = argc > 1 ? argv[1] : ".";
    gfx::iniciar("assets");
    Audio som;
    som.iniciar(false);
    Jogo j(som, nullptr);
    passos(j, 30);
    foto(j, pasta, "01_menu");
    tecla(j, K_RETURN);
    passos(j, 60);
    foto(j, pasta, "02_intro");
    tecla(j, K_RETURN);
    passos(j, 40);
    entrada.tec['D'] = true;
    passos(j, 50);
    entrada.tec['D'] = false;
    foto(j, pasta, "03_jogo");
    const Terminal& t = j.fase->terminais[0];
    j.jogador->rect.x = t.rect.x; j.jogador->rect.y = t.rect.baixo() + 2;
    j.jogador->px = (float)(j.jogador->rect.x + 11); j.jogador->py = (float)(j.jogador->rect.y + 11);
    passos(j, 5);
    foto(j, pasta, "04_perto_terminal");
    tecla(j, 'E');
    foto(j, pasta, "05_terminal");
    tecla(j, 'N');
    foto(j, pasta, "06_notas");
    tecla(j, 'N');
    tecla(j, K_ESC);
    // fase 3 e 5 para ver os mapas
    j.carregar_fase(2);
    passos(j, 10);
    foto(j, pasta, "07_fase3");
    j.carregar_fase(3);
    passos(j, 10);
    foto(j, pasta, "08_fase4");
    j.estado = ESCOLHA_FINAL;
    foto(j, pasta, "09_escolha");
    tecla(j, '2');
    passos(j, 60 * 5);
    foto(j, pasta, "10_cinema_explosao");
    passos(j, 60 * 8);
    foto(j, pasta, "11_cinema_fim");
    tecla(j, K_RETURN);
    passos(j, 120);
    foto(j, pasta, "12_vitoria");
    j.encerrar(DERROTA, L"Falhas críticas demais: o sistema do bunker colapsou.");
    foto(j, pasta, "13_derrota");
    std::printf("capturas em %s\n", pasta.c_str());
    return 0;
}
