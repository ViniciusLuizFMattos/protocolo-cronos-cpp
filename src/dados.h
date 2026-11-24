// Protocolo Cronos++ - dados do jogo: fases, mapas, textos e banco de questões
#pragma once
#include <string>
#include <vector>
#include "gfx.h"

struct Questao { int fase; std::wstring id, enunciado, resposta, resolucao, dica; };
const std::vector<Questao>& banco_questoes();
std::vector<Questao> questoes_da_fase(int numero_fase);   // numero_fase: 1 a 5

struct InfoFase { std::wstring nome, nivel; int meta; std::wstring conteudo, historia; };
struct Solido { char ch; int c, r, w, h; };
struct TermCfg { std::wstring nome; int c, r; };
struct MapaCfg { int ic, ir, porta; std::vector<Solido> solidos; std::vector<TermCfg> terminais; };

constexpr int NUM_FASES = 5;
const InfoFase& info_fase(int idx);
const MapaCfg& mapa_cfg(int idx);
struct Paleta { gfx::Cor piso, piso2, parede, parede_luz, destaque; };
const Paleta& paleta(int idx);

extern const std::wstring TEXTO_HISTORIA;
struct Final { std::wstring titulo, corpo, epilogo; std::vector<std::wstring> legendas; };
const Final& final_cfg(int escolha);   // 0 = fugir, 1 = sobrecarregar
