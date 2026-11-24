# -*- coding: utf-8 -*-
"""Questoes extras do Cronos++ (fases com meta >= 2 passam a ter 20 questoes).
Cada item: (fase, id, enunciado, resposta, resolucao, verificador_forca_bruta)."""
from itertools import permutations, combinations


def anag(palavra):            # anagramas distintos (forca bruta)
    return len(set(permutations(palavra)))


def nums(n, cond):            # numeros de n algarismos distintos (1o != 0) que satisfazem cond
    return sum(1 for p in permutations("0123456789", n) if p[0] != "0" and cond(p))


def A(n, p):
    return len(list(permutations(range(n), p)))


def C(n, p):
    return len(list(combinations(range(n), p)))


EXTRAS = [
 # ---------------- FASE 2 ----------------
 (2, "F2-11", "Sequenciador Genético: a senha é um anagrama da palavra REATOR (a letra R aparece 2 vezes). Quantas senhas distintas existem?",
  360, "6 letras com o R repetido 2 vezes: 6!/2! = 720/2 = 360 anagramas.", lambda: anag("REATOR")),
 (2, "F2-12", "O código do módulo de contenção é um anagrama de ABISMO (6 letras todas diferentes). Quantos códigos podem ser formados?",
  720, "Seis letras distintas: 6! = 720 anagramas.", lambda: anag("ABISMO")),
 (2, "F2-13", "A tranca da escotilha aceita qualquer anagrama de CAPSULA (a letra A aparece 2 vezes). Quantos anagramas diferentes existem?",
  2520, "7 letras com A repetida 2 vezes: 7!/2! = 5040/2 = 2520.", lambda: anag("CAPSULA")),
 (2, "F2-14", "A senha do cofre de oxigênio é um anagrama de OXIGENIO (O aparece 2 vezes e I aparece 2 vezes). Quantas senhas distintas existem?",
  10080, "8 letras com duas repetições duplas: 8!/(2!·2!) = 40320/4 = 10080.", lambda: anag("OXIGENIO")),
 (2, "F2-15", "O visor do laboratório mostra números de 4 algarismos formados exatamente pelos dígitos 3, 3, 5 e 5. Quantos números diferentes podem aparecer?",
  6, "4!/(2!·2!) = 24/4 = 6 números.", lambda: anag("3355")),
 (2, "F2-16", "A lista de comandos do bunker tem o código TERMINAL, formado por 8 letras todas diferentes. Quantos anagramas dessa palavra existem?",
  40320, "Oito letras distintas: 8! = 40320.", lambda: anag("TERMINAL")),
 (2, "F2-17", "Seis luzes serão enfileiradas no painel de alarme: 2 vermelhas, 2 azuis, 1 verde e 1 amarela. Luzes da mesma cor são idênticas. Quantas fileiras de cores diferentes existem?",
  180, "6!/(2!·2!) = 720/4 = 180 fileiras.", lambda: anag("RRAAVY")),
 (2, "F2-18", "Um robô-sonda anda numa grade só para a direita (D) ou para cima (C). Para chegar ao destino ele dá 4 passos D e 3 passos C, em qualquer ordem. Quantos caminhos diferentes existem?",
  35, "Os caminhos são anagramas de DDDDCCC: 7!/(4!·3!) = 35.", lambda: anag("DDDDCCC")),
 (2, "F2-19", "O código de emergência do laboratório é um anagrama de RADAR (R aparece 2 vezes e A aparece 2 vezes). Quantos códigos distintos existem?",
  30, "5!/(2!·2!) = 120/4 = 30.", lambda: anag("RADAR")),
 (2, "F2-20", "Dos anagramas da palavra ACHERON (7 letras distintas), quantos COMEÇAM pela letra A?",
  720, "Fixada a letra A na primeira posição, as outras 6 letras se permutam: 6! = 720.", lambda: sum(1 for p in permutations("ACHERON") if p[0] == "A")),
 # ---------------- FASE 3 ----------------
 (3, "F3-11", "Radar de Sonar: 9 cientistas disputam um pódio com 1º, 2º e 3º lugares. Quantos pódios diferentes são possíveis?",
  504, "A(9,3) = 9·8·7 = 504.", lambda: A(9, 3)),
 (3, "F3-12", "A placa de identificação de um drone usa 4 letras distintas escolhidas entre 6 letras disponíveis (A, B, C, D, E, F). Quantas placas existem?",
  360, "A(6,4) = 6·5·4·3 = 360.", lambda: A(6, 4)),
 (3, "F3-13", "Entre 5 sinais captados, o radar escolhe um para ser o sinal principal e outro, diferente, para ser o de apoio. De quantas formas isso pode ser feito?",
  20, "A(5,2) = 5·4 = 20.", lambda: A(5, 2)),
 (3, "F3-14", "Console de Pressão: quantos números de 3 algarismos DISTINTOS existem no sistema decimal (o primeiro algarismo não pode ser zero)?",
  648, "Centena: 9 opções (1 a 9); dezena: 9 (inclui o 0, exclui a centena); unidade: 8. Total 9·9·8 = 648.", lambda: nums(3, lambda p: True)),
 (3, "F3-15", "Entre 15 tripulantes, serão escolhidos um piloto e um copiloto (funções diferentes). De quantas maneiras isso pode ser feito?",
  210, "A(15,2) = 15·14 = 210.", lambda: A(15, 2)),
 (3, "F3-16", "Um painel tem 12 encaixes numerados e 4 módulos diferentes serão instalados, um em cada encaixe escolhido. De quantas formas a instalação pode ser feita?",
  11880, "A(12,4) = 12·11·10·9 = 11880.", lambda: A(12, 4)),
 (3, "F3-17", "Oito mergulhadores disputam uma prova e serão premiados os 4 primeiros colocados (1º ao 4º lugar). Quantas classificações dos premiados são possíveis?",
  1680, "A(8,4) = 8·7·6·5 = 1680.", lambda: A(8, 4)),
 (3, "F3-18", "O terminal do sonar aceita códigos de 3 letras distintas do alfabeto de 26 letras. Quantos códigos existem?",
  15600, "A(26,3) = 26·25·24 = 15600.", lambda: A(26, 3)),
 (3, "F3-19", "Onze equipes de manutenção se revezam em 3 turnos diferentes (manhã, tarde e noite), e cada turno recebe uma equipe distinta. De quantas formas os turnos podem ser preenchidos?",
  990, "A(11,3) = 11·10·9 = 990.", lambda: A(11, 3)),
 (3, "F3-20", "A Central de Sonar tem 10 torpedos diferentes e 5 tubos de lançamento distintos; cada tubo recebe um torpedo. De quantas formas os tubos podem ser carregados?",
  30240, "A(10,5) = 10·9·8·7·6 = 30240.", lambda: A(10, 5)),
 # ---------------- FASE 4 ----------------
 (4, "F4-11", "Câmara do Reator: de 10 válvulas disponíveis, 3 devem ser abertas ao mesmo tempo (a ordem não importa). Quantos grupos de 3 válvulas existem?",
  120, "C(10,3) = 10!/(3!·7!) = 120.", lambda: C(10, 3)),
 (4, "F4-12", "Entre 12 técnicos, forma-se uma equipe de 4 pessoas para a manutenção do núcleo. Quantas equipes diferentes podem ser formadas?",
  495, "C(12,4) = 12!/(4!·8!) = 495.", lambda: C(12, 4)),
 (4, "F4-13", "Um decágono (polígono de 10 lados) é desenhado no mapa do reator. Quantas diagonais ele possui? (Diagonal é o segmento que liga dois vértices não consecutivos.)",
  35, "Todos os segmentos entre 2 dos 10 vértices: C(10,2) = 45. Retirando os 10 lados: 45 − 10 = 35 diagonais.",
  lambda: sum(1 for a, b in combinations(range(10), 2) if (b - a) % 10 not in (1, 9))),
 (4, "F4-14", "Das 8 amostras do estoque, 5 serão enviadas à superfície. De quantas maneiras é possível escolher essas 5 amostras?",
  56, "C(8,5) = C(8,3) = 8!/(3!·5!) = 56.", lambda: C(8, 5)),
 (4, "F4-15", "Uma equipe de reparo será formada por 3 engenheiros escolhidos entre 6 e 2 biólogos escolhidos entre 5. Quantas equipes diferentes podem ser formadas?",
  200, "C(6,3)·C(5,2) = 20·10 = 200.", lambda: C(6, 3) * C(5, 2)),
 (4, "F4-16", "Sete pontos de sensores estão marcados no casco, sem que 3 deles estejam alinhados. Quantos triângulos distintos podem ser desenhados com vértices nesses pontos?",
  35, "Cada triângulo corresponde a um grupo de 3 pontos: C(7,3) = 35.", lambda: C(7, 3)),
 (4, "F4-17", "Onze equipes de resgate fazem um torneio em turno único, em que todas jogam contra todas exatamente uma vez. Quantas partidas serão disputadas?",
  55, "Cada partida é um par de equipes: C(11,2) = 55.", lambda: C(11, 2)),
 (4, "F4-18", "Há 16 baterias de reserva e o reator precisa ligar um par delas em série (a ordem do par não importa). Quantos pares diferentes existem?",
  120, "C(16,2) = 16·15/2 = 120.", lambda: C(16, 2)),
 (4, "F4-19", "Uma comissão será formada por 2 cientistas escolhidos entre 5 e 1 técnico escolhido entre 4. Quantas comissões diferentes são possíveis?",
  40, "C(5,2)·C(4,1) = 10·4 = 40.", lambda: C(5, 2) * C(4, 1)),
 (4, "F4-20", "De 13 tubos de refrigeração, 4 serão inspecionados ao mesmo tempo. Quantas seleções diferentes são possíveis?",
  715, "C(13,4) = 13!/(4!·9!) = 715.", lambda: C(13, 4)),
 # ---------------- FASE 5 ----------------
 (5, "F5-11", "Sala da Cápsula: 6 sobreviventes entram em fila, mas Marta deve ser SEMPRE a primeira. De quantas formas a fila pode ser formada?",
  120, "Marta está fixa na 1ª posição; as outras 5 pessoas se permutam: 5! = 120.", lambda: sum(1 for p in permutations(range(6)) if p[0] == 0)),
 (5, "F5-12", "Quatro homens e três mulheres formam uma fila de 7 pessoas, alternando sexos (começando e terminando por homem: H M H M H M H). De quantas formas a fila pode ser organizada?",
  144, "Homens nas posições 1, 3, 5 e 7: 4! = 24. Mulheres nas posições 2, 4 e 6: 3! = 6. Total 24·6 = 144.",
  lambda: sum(1 for p in permutations(range(7)) if all((x < 4) == (i % 2 == 0) for i, x in enumerate(p)))),
 (5, "F5-13", "Quantos números PARES de 3 algarismos distintos existem? (Use os algarismos de 0 a 9; o primeiro não pode ser zero.)",
  328, "Terminando em 0: 9·8 = 72. Terminando em 2, 4, 6 ou 8 (4 opções): centena tem 8 opções (sem o 0 e sem a unidade) e dezena 8: 4·8·8 = 256. Total 72 + 256 = 328.",
  lambda: nums(3, lambda p: int(p[2]) % 2 == 0)),
 (5, "F5-14", "Uma equipe de 4 pessoas será escolhida entre 5 homens e 5 mulheres, com a exigência de ter PELO MENOS 2 mulheres. Quantas equipes são possíveis?",
  155, "Total: C(10,4) = 210. Sem mulheres: C(5,4) = 5. Com exatamente 1 mulher: 5·C(5,3) = 50. Resposta: 210 − 5 − 50 = 155.",
  lambda: sum(1 for c in combinations(range(10), 4) if sum(1 for x in c if x >= 5) >= 2)),
 (5, "F5-15", "Sete pessoas entram em fila na cápsula, mas Léo e Nina NÃO podem ficar lado a lado. Quantas filas são possíveis?",
  3600, "Total: 7! = 5040. Com Léo e Nina juntos: 6!·2! = 1440. Resposta: 5040 − 1440 = 3600.",
  lambda: sum(1 for p in permutations(range(7)) if abs(p.index(0) - p.index(1)) != 1)),
 (5, "F5-16", "A senha do cofre tem 2 letras DISTINTAS escolhidas entre A, B, C, D e E, seguidas de 3 dígitos DISTINTOS de 0 a 9. Quantas senhas existem?",
  14400, "Letras: A(5,2) = 20. Dígitos: A(10,3) = 720. Total 20·720 = 14400.", lambda: A(5, 2) * A(10, 3)),
 (5, "F5-17", "Quantos números de 4 algarismos DISTINTOS começam pelo algarismo 7?",
  504, "O 7 fica fixo na primeira posição. Para as 3 posições restantes sobram 9 algarismos: A(9,3) = 9·8·7 = 504.",
  lambda: nums(4, lambda p: p[0] == "7")),
 (5, "F5-18", "Uma comissão de 3 pessoas será escolhida entre 6 homens e 5 mulheres, com PELO MENOS 1 mulher. Quantas comissões são possíveis?",
  145, "Total: C(11,3) = 165. Sem mulheres: C(6,3) = 20. Resposta: 165 − 20 = 145.",
  lambda: sum(1 for c in combinations(range(11), 3) if any(x >= 6 for x in c))),
 (5, "F5-19", "Dos anagramas da palavra BUNKER (6 letras distintas), quantos TERMINAM em consoante?",
  480, "Consoantes: B, N, K, R (4 opções para a última posição). As outras 5 letras se permutam: 5! = 120. Total 4·120 = 480.",
  lambda: sum(1 for p in permutations("BUNKER") if p[-1] in "BNKR")),
 (5, "F5-20", "Quantos números de 3 algarismos DISTINTOS e MENORES que 400 existem? (Algarismos de 0 a 9; o primeiro não pode ser zero.)",
  216, "Primeiro algarismo: 1, 2 ou 3 (3 opções). Depois 9 opções e 8 opções: 3·9·8 = 216.",
  lambda: nums(3, lambda p: int(p[0]) < 4)),
]

if __name__ == "__main__":
    for f, i, e, r, res, chk in EXTRAS:
        v = chk()
        assert v == r, (i, v, r)
    print("extras ok:", len(EXTRAS))
