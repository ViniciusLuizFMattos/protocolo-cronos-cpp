// Protocolo Cronos++ - cinemáticas dos finais. Cada quadro é função apenas do final escolhido e do tempo.
#pragma once
#include "audio.h"

namespace cinema {

constexpr int NUM_FINAIS = 4;          // 0 Ascensão, 1 Pulso Cronos, 2 Guardião, 3 Protocolo Perfeito (secreto)
constexpr float DUR_PLANO = 5.5f;      // cada final tem 4 planos
constexpr int NUM_PLANOS = 4;
constexpr float INICIO_CARTAO = DUR_PLANO * NUM_PLANOS;
constexpr float DURACAO = INICIO_CARTAO + 3.0f;     // a partir daqui espera o jogador

// Dispara os sons cujo instante foi cruzado entre antes e depois (em segundos).
void sons(Audio& som, int final, float antes, float depois);

// Desenha o quadro do instante t (a tela inteira, inclusive faixas laterais). `descobertos` aparece no cartão final.
void desenhar(int final, float t, int descobertos);

// Ilustração animada pequena de um final, para a tela de escolha.
void miniatura(int final, int x, int y, int w, int h, float t);

}  // namespace cinema
