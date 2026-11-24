// Protocolo Cronos++ - constantes e utilidades comuns
#pragma once
#include <cmath>
#include <cstdint>
#include <string>

constexpr int LARGURA = 1024, ALTURA = 640;
constexpr int HUD_H = 64;                 // altura da barra superior
constexpr int TILE = 32;
constexpr int COLS = 32, ROWS = 18;       // 32*32 = 1024 ; 18*32 = 576 ; 576 + 64 = 640
constexpr int TEMPO_TOTAL = 12 * 60;      // tempo limite global (s)
constexpr int MAX_ERROS = 5;              // falhas críticas permitidas
constexpr int PENALIDADE_ERRO = 20;       // segundos descontados a cada erro
constexpr int TOTAL_ACERTOS = 12;
constexpr int PENALIDADE_DICA = 10;      // segundos descontados ao pedir a dica de uma pergunta (1ª vez)
constexpr float VEL_JOGADOR = 175.f;      // pixels por segundo
constexpr float FATOR_CORRIDA = 1.45f;    // Shift (novo no C++)
constexpr float PI_ = 3.14159265358979f;

// Gerador pseudoaleatório rápido (xorshift32).
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed = 2463534242u) : s(seed ? seed : 1u) {}
    uint32_t prox() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float f() { return (prox() >> 8) * (1.0f / 16777216.0f); }            // [0,1)
    float entre(float a, float b) { return a + (b - a) * f(); }
    int ent(int a, int b) { return a + (int)(prox() % (uint32_t)(b - a + 1)); }   // inclusivo
};
Rng& rng_global();

template <class T> inline T limitar(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

// std::to_wstring não existe no MinGW 6.3 usado aqui.
inline std::wstring a_wstr(long long v) {
    if (v == 0) return L"0";
    bool neg = v < 0;
    unsigned long long u = neg ? 0ull - (unsigned long long)v : (unsigned long long)v;
    std::wstring s;
    while (u) { s.insert(s.begin(), (wchar_t)(L'0' + u % 10)); u /= 10; }
    return neg ? L"-" + s : s;
}

// Maiúsculas para texto em português (towupper do MinGW só cobre ASCII).
inline std::wstring maiusc(std::wstring s) {
    for (auto& c : s) {
        if (c >= L'a' && c <= L'z') c = (wchar_t)(c - 32);
        else if (c >= 0xE0 && c <= 0xFE && c != 0xF7) c = (wchar_t)(c - 32);
    }
    return s;
}
