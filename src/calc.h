// Protocolo Cronos++ - calculadora segura do bloco de notas (+ - * / ^ ( ) e fatorial)
#pragma once
#include <string>

// Avalia a expressão. Devolve false se for inválida; senão preenche `saida` (inteiro ou decimal).
bool avaliar_expressao(const std::wstring& texto, std::wstring& saida);
