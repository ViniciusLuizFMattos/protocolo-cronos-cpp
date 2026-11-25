#include "dados.h"

using gfx::rgb;

const std::vector<Questao>& banco_questoes() {
    static const std::vector<Questao> b = {
#include "questoes_dados.inc"
    };
    return b;
}

std::vector<Questao> questoes_da_fase(int n) {
    std::vector<Questao> r;
    for (const auto& q : banco_questoes()) if (q.fase == n) r.push_back(q);
    return r;
}

static const std::vector<InfoFase> FASES = {
    {L"Câmara de Eclusa", L"Fácil", 1, L"Princípio Fundamental da Contagem",
     L"Você acorda na Eclusa de Descompressão da Estação Acheron-4. As luzes de emergência piscam. "
     L"A porta hidráulica só cede se o Painel de Fusíveis for religado."},
    {L"Laboratório Genético", L"Básico", 2, L"Permutação Simples e com Repetição",
     L"Aqui o organismo abissal foi estudado. A equipe anterior trancou o setor com "
     L"sequências de DNA embaralhadas: reordene-as para prosseguir."},
    {L"Central de Sonar", L"Intermediário", 2, L"Arranjo Simples",
     L"Algo enorme se move no escuro lá fora. O radar e o console de pressão só liberam "
     L"a passagem para quem dominar a ordem dos sinais."},
    {L"Câmara do Reator", L"Avançado", 3, L"Combinação Simples",
     L"O núcleo do reator ruge. Três válvulas precisam ser acionadas, mas aqui a ordem "
     L"não importa: apenas quais grupos você escolhe."},
    {L"Sala da Cápsula", L"Difícil", 4, L"Restrições e Integração de Conceitos",
     L"A última barreira. A cápsula de fuga espera do outro lado, mas o protocolo final "
     L"combina TUDO o que você aprendeu. Não há margem para erro."},
};

// solidos: (caractere, coluna, linha, largura, altura) em tiles.
//   '#' parede   'M' mesa   'B' tanque   'K' caixote   'R' núcleo do reator
static const std::vector<MapaCfg> MAPAS = {
    {2, 8, 8,
     {{'M', 5, 2, 4, 2}, {'M', 5, 13, 4, 2}, {'B', 12, 2, 1, 1}, {'B', 14, 2, 1, 1},
      {'K', 24, 3, 2, 2}, {'K', 22, 13, 2, 2}, {'B', 28, 14, 1, 1}, {'M', 17, 12, 3, 2}},
     {{L"Painel de Fusíveis", 14, 7}}},
    {2, 8, 8,
     {{'#', 15, 1, 1, 6}, {'#', 15, 11, 1, 6},
      {'M', 3, 2, 4, 2}, {'M', 3, 13, 4, 2}, {'M', 20, 2, 5, 2}, {'M', 20, 13, 5, 2},
      {'B', 10, 4, 1, 1}, {'B', 10, 13, 1, 1}, {'B', 27, 8, 2, 2}},
     {{L"Sequenciador Genético", 6, 8}, {L"Válvula de Contenção", 22, 8}}},
    {2, 9, 12,
     {{'#', 8, 1, 1, 8}, {'#', 8, 12, 1, 5}, {'#', 20, 1, 1, 10},
      {'M', 11, 2, 4, 2}, {'M', 11, 13, 4, 2}, {'K', 3, 3, 2, 2}, {'K', 3, 13, 2, 2},
      {'B', 17, 6, 1, 1}, {'M', 24, 3, 4, 2}},
     {{L"Radar de Sonar", 11, 8}, {L"Console de Pressão", 25, 9}}},
    {2, 9, 8,
     {{'R', 13, 6, 5, 4}, {'#', 24, 1, 1, 5}, {'#', 24, 12, 1, 5},
      {'K', 3, 3, 2, 2}, {'K', 3, 13, 2, 2}, {'M', 19, 13, 4, 2},
      {'B', 20, 3, 1, 1}, {'B', 22, 3, 1, 1}},
     {{L"Válvula do Reator A", 8, 3}, {L"Válvula do Reator B", 11, 13}, {L"Válvula do Reator C", 26, 5}}},
    {2, 8, 8,
     {{'B', 10, 3, 1, 1}, {'B', 10, 14, 1, 1}, {'M', 17, 3, 4, 2}, {'M', 17, 13, 4, 2},
      {'K', 28, 3, 2, 2}, {'K', 28, 13, 2, 2}, {'K', 6, 8, 2, 2}},
     {{L"Núcleo de Energia", 4, 3}, {L"Console da Cápsula", 4, 13},
      {L"Painel de Contenção", 14, 8}, {L"Computador Central", 24, 8}}},
};

static const Paleta PALETAS[5] = {
    {rgb(52, 62, 74), rgb(44, 53, 64), rgb(28, 36, 48), rgb(62, 78, 96), rgb(80, 150, 190)},
    {rgb(46, 66, 66), rgb(38, 56, 56), rgb(22, 42, 42), rgb(50, 90, 88), rgb(80, 210, 170)},
    {rgb(38, 50, 82), rgb(32, 43, 72), rgb(20, 28, 56), rgb(50, 70, 118), rgb(90, 140, 250)},
    {rgb(74, 58, 48), rgb(64, 50, 42), rgb(46, 30, 26), rgb(100, 66, 52), rgb(255, 150, 60)},
    {rgb(66, 44, 56), rgb(56, 38, 48), rgb(40, 22, 34), rgb(92, 50, 70), rgb(255, 80, 110)},
};

const InfoFase& info_fase(int i) { return FASES[i]; }
const MapaCfg& mapa_cfg(int i) { return MAPAS[i]; }
const Paleta& paleta(int i) { return PALETAS[i]; }

const std::wstring TEXTO_HISTORIA =
    L"Ano 2089. A Estação Submarina ACHERON-4, a 3.000 metros de profundidade, sofreu um colapso estrutural. "
    L"Você é o único sobrevivente e acorda em meio ao alarme.\n"
    L"Antes de morrer, a equipe anterior trancou cada setor com PROTOCOLOS CRIPTOGRÁFICOS baseados em "
    L"Análise Combinatória para conter um organismo abissal. Resolva os desafios dos terminais, abra as "
    L"portas hidráulicas e chegue à cápsula de fuga antes que o oxigênio, ou a paciência do organismo, acabe.";

static const Final FINAIS[4] = {
    {L"Final A: Ascensão", L"ASCENSÃO", L"Lançar a cápsula agora e deixar o organismo lacrado.",
     L"Você sela a escotilha e dispara a cápsula Cronos. Acheron-4 some nas trevas enquanto você sobe, levando os dados dos "
     L"protocolos. O organismo permanece contido pelas travas que você destravou com Matemática. Você sobreviveu.",
     L"Os dados chegam à superfície. A Acheron-4 vira zona proibida, e lá embaixo o organismo continua à espera.",
     {L"A escotilha se fecha. Dezesseis travas confirmam a vedação.",
      L"A cápsula Cronos se desprende. Uma a uma, as luzes de Acheron-4 se apagam.",
      L"Mas algo desperta no abismo. As travas que você quebrou com Matemática agora o mantêm preso.",
      L"Três mil metros acima, o céu amanhece. Você chegou à superfície."}},
    {L"Final B: Pulso Cronos", L"PULSO CRONOS", L"Sobrecarregar o reator, destruir o organismo e fugir no último segundo.",
     L"Você redireciona toda a energia ao núcleo. O reator sobrecarrega e o pulso aniquila o organismo e as portas que o "
     L"prendiam. Na última contagem, a cápsula escapa. Acheron-4 deixa de existir, e a ameaça também.",
     L"Nenhum vestígio do organismo é encontrado. Contar possibilidades foi o que salvou a missão.",
     {L"Toda a energia flui para o núcleo. O reator começa a gritar.",
      L"A cápsula parte enquanto os tentáculos cercam a estação.",
      L"O Pulso Cronos explode. A luz varre o abismo e desfaz tudo o que ele alcança.",
      L"Na superfície, o mar se acalma. A ameaça acabou, e você sobreviveu."}},
    {L"Final C: O Guardião", L"O GUARDIÃO", L"Ficar na estação, enviar os dados e selar o organismo para sempre.",
     L"Você coloca os dados na cápsula e a envia sozinha. Com os protocolos que aprendeu, fecha o último selo por dentro. "
     L"Alguém precisava ficar de guarda, e você escolheu ser essa pessoa.",
     L"Meses depois, um sinal responde das alturas. Estão vindo buscar você.",
     {L"A cápsula parte vazia, levando tudo o que você descobriu.",
      L"Você puxa a alavanca. Anel após anel, o selo se fecha sobre o organismo.",
      L"O abismo dorme. Acheron-4 volta a brilhar, calma, no fundo do mar.",
      L"No sonar, seu farol continua aceso. E então... alguém responde."}},
    {L"Final secreto: Protocolo Perfeito", L"PROTOCOLO PERFEITO", L"Responder ao chamado: você não errou nenhuma vez.",
     L"Sem uma única falha, os protocolos entram em ressonância. A criatura nunca foi um monstro: era a guardiã do abismo, "
     L"presa por humanos que não sabiam conversar com ela. Você falou a língua certa.",
     L"O relatório oficial não menciona a aurora, nem a despedida. Você nunca precisou que acreditassem.",
     {L"Doze respostas, nenhuma falha. Os terminais passam a pulsar em harmonia.",
      L"O olho se abre. A fúria se apaga e dá lugar a algo que se parece com reconhecimento.",
      L"Ela não tenta impedir você. Ela o carrega para cima.",
      L"Sob a aurora, a guardiã se despede. Cada ligação possível, enfim, foi feita."}},
};
const Final& final_cfg(int e) { return FINAIS[e]; }
