# -*- coding: utf-8 -*-
"""Gera src/questoes_dados.inc: questoes do Python original + extras do Cronos++.
Uso: py -3.11 -X utf8 tools/gerar_questoes.py <questoes.py original> <saida.inc>"""
import sys, os, importlib.util
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from questoes_extras import EXTRAS
from dicas import DICAS
import re
spec = importlib.util.spec_from_file_location("q", sys.argv[1])
q = importlib.util.module_from_spec(spec); spec.loader.exec_module(q)

def lit(s):
    assert ")CRONOS\"" not in s
    return 'LR"CRONOS(' + s + ')CRONOS"'

itens = []
for fase, lista in sorted(q.BANCO_QUESTOES.items()):
    for it in lista:
        itens.append((fase, it["id"], it["enunciado"], it["resposta"], it["resolucao"]))
for f, i, e, r, res, _ in EXTRAS:
    itens.append((f, i, e, str(r), res))
ids = [x[1] for x in itens]
assert len(ids) == len(set(ids)), "ids repetidos"
assert set(ids) == set(DICAS), ("dicas faltando/sobrando", set(ids) ^ set(DICAS))
for f, i, e, r, res in itens:       # a dica mostra o metodo, nunca o resultado
    assert not re.search(r"(?<![\d.])" + re.escape(r) + r"(?![\d.])", DICAS[i]), ("dica contem a resposta", i, r)
with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as out:
    out.write("// Gerado por tools/gerar_questoes.py - nao editar a mao.\n")
    for f, i, e, r, res in itens:
        out.write("{%d, %s, %s, %s, %s, %s},\n" % (f, lit(i), lit(e), lit(r), lit(res), lit(DICAS[i])))
print("ok", len(itens), {f: sum(1 for x in itens if x[0] == f) for f in range(1, 6)})
