// Protocolo Cronos++ - renderizador por software (framebuffer 32 bits) + texto via GDI
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "comum.h"

namespace gfx {

typedef uint32_t Cor;   // 0x00RRGGBB
constexpr Cor rgb(int r, int g, int b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b; }
inline int cr(Cor c) { return (c >> 16) & 255; }
inline int cg(Cor c) { return (c >> 8) & 255; }
inline int cb(Cor c) { return c & 255; }
Cor somar(Cor c, int k);                 // soma k a cada canal (com limite)
Cor lerp(Cor a, Cor b, float k);

struct Ret {
    int x = 0, y = 0, w = 0, h = 0;
    Ret() {}
    Ret(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    int dir() const { return x + w; }
    int baixo() const { return y + h; }
    int cx() const { return x + w / 2; }
    int cy() const { return y + h / 2; }
    bool contem(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    Ret inflar(int dx, int dy) const { return Ret(x - dx / 2, y - dy / 2, w + dx, h + dy); }
    Ret mover(int dx, int dy) const { return Ret(x + dx, y + dy, w, h); }
    bool colide(const Ret& o) const { return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h; }
};

struct Pt { float x, y; };

struct Superficie {
    int w, h;
    std::vector<uint32_t> px;
    Superficie(int w_ = 0, int h_ = 0) : w(w_), h(h_), px((size_t)w_ * h_, 0) {}
};

enum Fonte { F_PEQ, F_TEXTO, F_GRANDE, F_TITULO, F_GIGANTE, F_HUD_L, F_HUD_V, F_MONO, F_N };

bool iniciar(const std::string& pasta_assets);   // cria o buffer da tela e carrega as fontes de <pasta>/fonts
const uint32_t* pixels();             // framebuffer 1024x640, 0x00RRGGBB (a plataforma apresenta isto)
void alvo_tela();                     // desenha na tela principal
void alvo(Superficie& s);             // desenha numa superfície própria (sem texto)
void origem(int x, int y);            // translação aplicada a todas as primitivas
void limpar(Cor c);

void ret(int x, int y, int w, int h, Cor c, int a = 255);
inline void ret(const Ret& r, Cor c, int a = 255) { ret(r.x, r.y, r.w, r.h, c, a); }
void ret_borda(int x, int y, int w, int h, int esp, Cor c, int a = 255);
inline void ret_borda(const Ret& r, int esp, Cor c, int a = 255) { ret_borda(r.x, r.y, r.w, r.h, esp, c, a); }
void rret(int x, int y, int w, int h, int raio, Cor c, int a = 255);
inline void rret(const Ret& r, int raio, Cor c, int a = 255) { rret(r.x, r.y, r.w, r.h, raio, c, a); }
void rret_borda(int x, int y, int w, int h, int raio, int esp, Cor c, int a = 255);
inline void rret_borda(const Ret& r, int raio, int esp, Cor c, int a = 255) { rret_borda(r.x, r.y, r.w, r.h, raio, esp, c, a); }
void circ(float cx, float cy, float r, Cor c, int a = 255);
void circ_borda(float cx, float cy, float r, float esp, Cor c, int a = 255);
void elipse(int x, int y, int w, int h, Cor c, int a = 255);
void elipse_borda(int x, int y, int w, int h, float esp, Cor c, int a = 255);
void linha(float x0, float y0, float x1, float y1, float esp, Cor c, int a = 255);
void poligono(const Pt* p, int n, Cor c, int a = 255);
void arco(float cx, float cy, float rx, float ry, float ang0, float ang1, float esp, Cor c, int a = 255);
void blit(const Superficie& s, int x, int y);
// Aplica "escuridão" (alfa 0-255 por pixel, w*h) sobre a tela na posição (x,y) com a cor dada.
bool salvar_bmp(const char* caminho);   // grava a tela atual em um .bmp (depuração/capturas)
void escurecer(const uint8_t* alfa, int w, int h, int x, int y, Cor cor);

// --- texto ---
int larg_texto(const std::wstring& t, Fonte f);
int alt_fonte(Fonte f);
// Desenha texto; (x,y) é o canto superior esquerdo (ou o topo-centro se centro=true).
void texto(const std::wstring& t, Fonte f, Cor c, int x, int y, bool centro = false, bool sombra = true);
void texto_dir(const std::wstring& t, Fonte f, Cor c, int xdir, int y, bool sombra = true);
const std::vector<std::wstring>& quebrar(const std::wstring& t, Fonte f, int largura);
// Desenha com quebra de linha; devolve o y logo abaixo do bloco.
int paragrafo(const std::wstring& t, Fonte f, Cor c, int x, int y, int largura, int espaco = 4, bool centro = false);

}  // namespace gfx
