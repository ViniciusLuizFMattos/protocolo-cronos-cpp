#include "gfx.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace gfx {

namespace {
struct Alvo { uint32_t* px = nullptr; int w = 0, h = 0; };
Alvo A, TELA;
int TX = 0, TY = 0;
std::vector<uint32_t> buffer_tela;

struct Glifo { std::vector<uint8_t> cov; int w = 0, h = 0, xoff = 0, yoff = 0; float avanco = 0; };
struct FonteTT {
    std::vector<unsigned char> dados;
    stbtt_fontinfo info;
    float escala = 0;
    int ascent = 0, altura = 0;
    std::map<wchar_t, Glifo> glifos;
    bool pronta = false;
};
FonteTT fontes[F_N];

inline void sync() {}

inline void mix(uint32_t* p, Cor c, int a) {
    if (a >= 255) { *p = c; return; }
    if (a <= 0) return;
    uint32_t d = *p;
    int ia = 255 - a;
    int r = (((d >> 16) & 255) * ia + cr(c) * a + 127) / 255;
    int g = (((d >> 8) & 255) * ia + cg(c) * a + 127) / 255;
    int b = ((d & 255) * ia + cb(c) * a + 127) / 255;
    *p = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

inline void plot(int x, int y, Cor c, int a) {
    if ((unsigned)x < (unsigned)A.w && (unsigned)y < (unsigned)A.h) mix(&A.px[(size_t)y * A.w + x], c, a);
}

inline void span(int y, int x0, int x1, Cor c, int a) {   // [x0,x1)
    if ((unsigned)y >= (unsigned)A.h) return;
    if (x0 < 0) x0 = 0;
    if (x1 > A.w) x1 = A.w;
    uint32_t* p = &A.px[(size_t)y * A.w];
    if (a >= 255) { for (int x = x0; x < x1; x++) p[x] = c; }
    else for (int x = x0; x < x1; x++) mix(&p[x], c, a);
}

inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline int cov_a(float cov, int a) { return (int)(cov * a + 0.5f); }

// Retângulo arredondado preenchido (esp<=0) ou apenas a borda (esp>0). Coordenadas já transladadas.
void rret_sdf(int x0, int y0, int w, int h, int raio, int esp, Cor c, int a) {
    if (w <= 0 || h <= 0) return;
    int x1 = x0 + w, y1 = y0 + h;
    raio = std::min(raio, std::min(w, h) / 2);
    float hw = w / 2.0f, hh = h / 2.0f, mx = x0 + hw, my = y0 + hh;
    int ys0 = std::max(y0, 0), ys1 = std::min(y1, A.h);
    for (int y = ys0; y < ys1; y++) {
        bool linha_canto = (y < y0 + raio + 1) || (y >= y1 - raio - 1);
        bool linha_borda = esp > 0 && (y < y0 + esp || y >= y1 - esp);
        if (!linha_canto) {
            if (esp <= 0) span(y, x0, x1, c, a);
            else { span(y, x0, x0 + esp, c, a); span(y, x1 - esp, x1, c, a); }
            continue;
        }
        int xa = x0 + raio + 1, xb = x1 - raio - 1;
        for (int x = x0; x < x1; x++) {
            if (x >= xa && x < xb) {            // trecho reto
                if (esp <= 0 || linha_borda) { span(y, x, xb, c, a); x = xb - 1; }
                else x = xb - 1;
                continue;
            }
            if ((unsigned)x >= (unsigned)A.w) continue;
            float px = std::fabs(x + 0.5f - mx) - (hw - raio);
            float py = std::fabs(y + 0.5f - my) - (hh - raio);
            float d = std::sqrt(std::max(px, 0.f) * std::max(px, 0.f) + std::max(py, 0.f) * std::max(py, 0.f))
                      + std::min(std::max(px, py), 0.f) - raio;
            float cov = clamp01(0.5f - d);
            if (esp > 0) cov -= clamp01(0.5f - (d + esp));
            if (cov > 0) mix(&A.px[(size_t)y * A.w + x], c, cov_a(cov, a));
        }
    }
}
}  // namespace

Cor somar(Cor c, int k) {
    return rgb(limitar(cr(c) + k, 0, 255), limitar(cg(c) + k, 0, 255), limitar(cb(c) + k, 0, 255));
}
Cor lerp(Cor a, Cor b, float k) {
    return rgb((int)(cr(a) + (cr(b) - cr(a)) * k), (int)(cg(a) + (cg(b) - cg(a)) * k), (int)(cb(a) + (cb(b) - cb(a)) * k));
}

static bool ler_arquivo(const std::string& caminho, std::vector<unsigned char>& saida) {
    std::ifstream f(caminho.c_str(), std::ios::binary);
    if (!f) return false;
    saida.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !saida.empty();
}

bool iniciar(const std::string& pasta_assets) {
    if (!buffer_tela.empty()) return true;
    buffer_tela.assign((size_t)LARGURA * ALTURA, 0);
    TELA.px = buffer_tela.data(); TELA.w = LARGURA; TELA.h = ALTURA;
    A = TELA;
    struct Def { int tam; const char* arquivo; };
    const Def defs[F_N] = {{16, "NotoSans-Regular.ttf"}, {21, "NotoSans-Regular.ttf"}, {32, "NotoSans-Bold.ttf"},
                           {24, "NotoSans-Bold.ttf"}, {56, "NotoSans-Bold.ttf"}, {12, "NotoSans-Bold.ttf"},
                           {24, "NotoSans-Bold.ttf"}, {17, "NotoSansMono-Regular.ttf"}};
    bool ok = true;
    for (int i = 0; i < F_N; i++) {
        FonteTT& f = fontes[i];
        // reaproveita os bytes de uma fonte já carregada com o mesmo arquivo
        for (int k = 0; k < i && f.dados.empty(); k++)
            if (std::string(defs[k].arquivo) == defs[i].arquivo) f.dados = fontes[k].dados;
        if (f.dados.empty() && !ler_arquivo(pasta_assets + "/fonts/" + defs[i].arquivo, f.dados)) { ok = false; continue; }
        if (!stbtt_InitFont(&f.info, f.dados.data(), stbtt_GetFontOffsetForIndex(f.dados.data(), 0))) { ok = false; continue; }
        f.escala = stbtt_ScaleForMappingEmToPixels(&f.info, (float)defs[i].tam);
        int asc, desc, gap;
        stbtt_GetFontVMetrics(&f.info, &asc, &desc, &gap);
        f.ascent = (int)std::lround(asc * f.escala);
        f.altura = (int)std::lround((asc - desc) * f.escala);
        f.pronta = true;
    }
    return ok;
}

const uint32_t* pixels() { return TELA.px; }
void alvo_tela() { sync(); A = TELA; }
void alvo(Superficie& s) { sync(); A.px = s.px.data(); A.w = s.w; A.h = s.h; }
void origem(int x, int y) { TX = x; TY = y; }

void limpar(Cor c) {
    sync();
    size_t n = (size_t)A.w * A.h;
    for (size_t i = 0; i < n; i++) A.px[i] = c;
}

void ret(int x, int y, int w, int h, Cor c, int a) {
    sync();
    x += TX; y += TY;
    for (int j = std::max(y, 0); j < std::min(y + h, A.h); j++) span(j, x, x + w, c, a);
}

void ret_borda(int x, int y, int w, int h, int esp, Cor c, int a) {
    if (esp * 2 >= w || esp * 2 >= h) { ret(x, y, w, h, c, a); return; }
    ret(x, y, w, esp, c, a);
    ret(x, y + h - esp, w, esp, c, a);
    ret(x, y + esp, esp, h - 2 * esp, c, a);
    ret(x + w - esp, y + esp, esp, h - 2 * esp, c, a);
}

void rret(int x, int y, int w, int h, int raio, Cor c, int a) { sync(); rret_sdf(x + TX, y + TY, w, h, raio, 0, c, a); }
void rret_borda(int x, int y, int w, int h, int raio, int esp, Cor c, int a) {
    sync();
    rret_sdf(x + TX, y + TY, w, h, raio, esp, c, a);
}

void circ(float cx, float cy, float r, Cor c, int a) {
    sync();
    cx += TX + 0.5f; cy += TY + 0.5f;
    float R = r + 0.5f;
    int y0 = std::max(0, (int)std::floor(cy - R - 1)), y1 = std::min(A.h, (int)std::ceil(cy + R + 1));
    for (int y = y0; y < y1; y++) {
        float dy = y + 0.5f - cy;
        float lim = (R + 1) * (R + 1) - dy * dy;
        if (lim < 0) continue;
        float xo = std::sqrt(lim);
        int x0 = std::max(0, (int)std::floor(cx - xo)), x1 = std::min(A.w, (int)std::ceil(cx + xo));
        for (int x = x0; x < x1; x++) {
            float dx = x + 0.5f - cx;
            float cov = clamp01(R - std::sqrt(dx * dx + dy * dy) + 0.5f);
            if (cov > 0) mix(&A.px[(size_t)y * A.w + x], c, cov_a(cov, a));
        }
    }
}

void circ_borda(float cx, float cy, float r, float esp, Cor c, int a) {
    sync();
    cx += TX + 0.5f; cy += TY + 0.5f;
    float R = r + 0.5f, Ri = R - esp;
    int y0 = std::max(0, (int)std::floor(cy - R - 1)), y1 = std::min(A.h, (int)std::ceil(cy + R + 1));
    float ri_skip = Ri - 1 > 0 ? (Ri - 1) * (Ri - 1) : -1;
    for (int y = y0; y < y1; y++) {
        float dy = y + 0.5f - cy;
        float lim = (R + 1) * (R + 1) - dy * dy;
        if (lim < 0) continue;
        float xo = std::sqrt(lim);
        float xi = (ri_skip > 0 && dy * dy < ri_skip) ? std::sqrt(ri_skip - dy * dy) : 0.f;
        for (int lado = 0; lado < 2; lado++) {
            float ia = lado == 0 ? cx - xo : cx + xi, ib = lado == 0 ? cx - xi : cx + xo;
            int x0 = std::max(0, (int)std::floor(ia)), x1 = std::min(A.w, (int)std::ceil(ib));
            for (int x = x0; x < x1; x++) {
                float dx = x + 0.5f - cx;
                float d = std::sqrt(dx * dx + dy * dy);
                float cov = clamp01(R - d + 0.5f) - clamp01(Ri - d + 0.5f);
                if (cov > 0) mix(&A.px[(size_t)y * A.w + x], c, cov_a(cov, a));
            }
        }
    }
}

void elipse(int x, int y, int w, int h, Cor c, int a) {
    sync();
    float rx = w / 2.0f, ry = h / 2.0f, cx = x + TX + rx, cy = y + TY + ry, m = std::min(rx, ry);
    for (int j = std::max(0, y + TY - 1); j < std::min(A.h, y + TY + h + 1); j++)
        for (int i = std::max(0, x + TX - 1); i < std::min(A.w, x + TX + w + 1); i++) {
            float dx = (i + 0.5f - cx) / rx, dy = (j + 0.5f - cy) / ry;
            float d = std::sqrt(dx * dx + dy * dy);
            float cov = clamp01((1 - d) * m + 0.5f);
            if (cov > 0) mix(&A.px[(size_t)j * A.w + i], c, cov_a(cov, a));
        }
}

void elipse_borda(int x, int y, int w, int h, float esp, Cor c, int a) {
    sync();
    float rx = w / 2.0f, ry = h / 2.0f, cx = x + TX + rx, cy = y + TY + ry, m = std::min(rx, ry);
    for (int j = std::max(0, y + TY - 1); j < std::min(A.h, y + TY + h + 1); j++)
        for (int i = std::max(0, x + TX - 1); i < std::min(A.w, x + TX + w + 1); i++) {
            float dx = (i + 0.5f - cx) / rx, dy = (j + 0.5f - cy) / ry;
            float d = std::sqrt(dx * dx + dy * dy);
            float cov = clamp01((1 - d) * m + 0.5f) - clamp01((1 - d) * m - esp + 0.5f);
            if (cov > 0) mix(&A.px[(size_t)j * A.w + i], c, cov_a(cov, a));
        }
}

void linha(float x0, float y0, float x1, float y1, float esp, Cor c, int a) {
    sync();
    x0 += TX + 0.5f; x1 += TX + 0.5f; y0 += TY + 0.5f; y1 += TY + 0.5f;
    float m = esp / 2 + 1;
    int bx0 = std::max(0, (int)std::floor(std::min(x0, x1) - m)), bx1 = std::min(A.w, (int)std::ceil(std::max(x0, x1) + m));
    int by0 = std::max(0, (int)std::floor(std::min(y0, y1) - m)), by1 = std::min(A.h, (int)std::ceil(std::max(y0, y1) + m));
    float vx = x1 - x0, vy = y1 - y0, l2 = vx * vx + vy * vy;
    for (int y = by0; y < by1; y++)
        for (int x = bx0; x < bx1; x++) {
            float px = x + 0.5f - x0, py = y + 0.5f - y0;
            float t = l2 > 0 ? limitar((px * vx + py * vy) / l2, 0.f, 1.f) : 0.f;
            float dx = px - vx * t, dy = py - vy * t;
            float d = std::sqrt(dx * dx + dy * dy);
            float cov = clamp01(esp / 2 + 0.5f - d);
            if (cov > 0) mix(&A.px[(size_t)y * A.w + x], c, cov_a(cov, a));
        }
}

void poligono(const Pt* p, int n, Cor c, int a) {
    sync();
    if (n < 3) return;
    float ymin = 1e9f, ymax = -1e9f;
    for (int i = 0; i < n; i++) { ymin = std::min(ymin, p[i].y + TY); ymax = std::max(ymax, p[i].y + TY); }
    int ya = std::max(0, (int)std::floor(ymin)), yb = std::min(A.h - 1, (int)std::ceil(ymax));
    std::vector<float> xs;
    for (int y = ya; y <= yb; y++) {
        float yc = y + 0.5f;
        xs.clear();
        for (int i = 0; i < n; i++) {
            Pt s = p[i], e = p[(i + 1) % n];
            float sy = s.y + TY, ey = e.y + TY;
            if ((sy <= yc && ey > yc) || (ey <= yc && sy > yc))
                xs.push_back(s.x + TX + (yc - sy) / (ey - sy) * (e.x - s.x));
        }
        std::sort(xs.begin(), xs.end());
        for (size_t k = 0; k + 1 < xs.size(); k += 2)
            span(y, (int)std::ceil(xs[k] - 0.5f), (int)std::ceil(xs[k + 1] - 0.5f), c, a);
    }
}

void arco(float cx, float cy, float rx, float ry, float ang0, float ang1, float esp, Cor c, int a) {
    const int N = 16;
    for (int i = 0; i < N; i++) {
        float t0 = ang0 + (ang1 - ang0) * i / N, t1 = ang0 + (ang1 - ang0) * (i + 1) / N;
        linha(cx + rx * std::cos(t0), cy - ry * std::sin(t0), cx + rx * std::cos(t1), cy - ry * std::sin(t1), esp, c, a);
    }
}

void blit(const Superficie& s, int x, int y) {
    sync();
    x += TX; y += TY;
    for (int j = 0; j < s.h; j++) {
        int dy = y + j;
        if ((unsigned)dy >= (unsigned)A.h) continue;
        int sx0 = std::max(0, -x), sx1 = std::min(s.w, A.w - x);
        if (sx1 <= sx0) continue;
        std::copy(&s.px[(size_t)j * s.w + sx0], &s.px[(size_t)j * s.w + sx1], &A.px[(size_t)dy * A.w + x + sx0]);
    }
}

void escurecer(const uint8_t* alfa, int w, int h, int x, int y, Cor cor) {
    sync();
    x += TX; y += TY;
    int cr_ = cr(cor), cg_ = cg(cor), cb_ = cb(cor);
    for (int j = 0; j < h; j++) {
        int dy = y + j;
        if ((unsigned)dy >= (unsigned)A.h) continue;
        int i0 = std::max(0, -x), i1 = std::min(w, A.w - x);
        const uint8_t* sa = &alfa[(size_t)j * w];
        uint32_t* d = &A.px[(size_t)dy * A.w + x];
        for (int i = i0; i < i1; i++) {
            int a = sa[i];
            if (!a) continue;
            uint32_t p = d[i];
            int ia = 255 - a;
            int r = (((p >> 16) & 255) * ia + cr_ * a) / 255;
            int g = (((p >> 8) & 255) * ia + cg_ * a) / 255;
            int b = ((p & 255) * ia + cb_ * a) / 255;
            d[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}

// ------------------------------------------------------------------ texto
static const Glifo& glifo(Fonte f, wchar_t c) {
    FonteTT& ft = fontes[f];
    auto it = ft.glifos.find(c);
    if (it != ft.glifos.end()) return it->second;
    Glifo g;
    if (ft.pronta) {
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&ft.info, c, &adv, &lsb);
        g.avanco = adv * ft.escala;
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&ft.info, c, ft.escala, ft.escala, &x0, &y0, &x1, &y1);
        g.w = x1 - x0; g.h = y1 - y0; g.xoff = x0; g.yoff = y0;
        if (g.w > 0 && g.h > 0) {
            g.cov.assign((size_t)g.w * g.h, 0);
            stbtt_MakeCodepointBitmap(&ft.info, g.cov.data(), g.w, g.h, g.w, ft.escala, ft.escala, c);
        }
    }
    return ft.glifos[c] = g;
}

int larg_texto(const std::wstring& t, Fonte f) {
    float x = 0;
    for (wchar_t c : t) x += glifo(f, c).avanco;
    return (int)std::lround(x);
}

int alt_fonte(Fonte f) { return fontes[f].altura; }

static void escrever(const std::wstring& t, Fonte f, Cor c, int x, int y) {
    float cx = (float)x;
    int base = y + fontes[f].ascent;
    for (wchar_t ch : t) {
        const Glifo& g = glifo(f, ch);
        int gx = (int)std::lround(cx) + g.xoff, gy = base + g.yoff;
        for (int j = 0; j < g.h; j++) {
            int py = gy + j;
            if ((unsigned)py >= (unsigned)A.h) continue;
            for (int i = 0; i < g.w; i++) {
                int px = gx + i;
                int a = g.cov[(size_t)j * g.w + i];
                if (a && (unsigned)px < (unsigned)A.w) mix(&A.px[(size_t)py * A.w + px], c, a);
            }
        }
        cx += g.avanco;
    }
}

void texto(const std::wstring& t, Fonte f, Cor c, int x, int y, bool centro, bool sombra) {
    if (t.empty() || A.px != TELA.px) return;
    if (centro) x -= larg_texto(t, f) / 2;
    x += TX; y += TY;
    if (sombra) escrever(t, f, rgb(0, 0, 0), x + 1, y + 2);
    escrever(t, f, c, x, y);
}

void texto_dir(const std::wstring& t, Fonte f, Cor c, int xdir, int y, bool sombra) {
    texto(t, f, c, xdir - larg_texto(t, f), y, false, sombra);
}

const std::vector<std::wstring>& quebrar(const std::wstring& t, Fonte f, int largura) {
    static std::map<std::pair<std::wstring, int>, std::vector<std::wstring>> cache;
    auto chave = std::make_pair(t, (int)f * 10000 + largura);
    auto it = cache.find(chave);
    if (it != cache.end()) return it->second;
    if (cache.size() > 300) cache.clear();
    std::vector<std::wstring> linhas;
    size_t i = 0;
    while (true) {
        size_t fim = t.find(L'\n', i);
        std::wstring par = t.substr(i, fim == std::wstring::npos ? std::wstring::npos : fim - i);
        std::wstring atual;
        size_t k = 0;
        while (true) {
            size_t sp = par.find(L' ', k);
            std::wstring palavra = par.substr(k, sp == std::wstring::npos ? std::wstring::npos : sp - k);
            std::wstring teste = atual.empty() ? palavra : atual + L" " + palavra;
            if (larg_texto(teste, f) <= largura) atual = teste;
            else { if (!atual.empty()) linhas.push_back(atual); atual = palavra; }
            if (sp == std::wstring::npos) break;
            k = sp + 1;
        }
        linhas.push_back(atual);
        if (fim == std::wstring::npos) break;
        i = fim + 1;
    }
    return cache[chave] = linhas;
}

int paragrafo(const std::wstring& t, Fonte f, Cor c, int x, int y, int largura, int espaco, bool centro) {
    for (const auto& l : quebrar(t, f, largura)) {
        if (centro) texto(l, f, c, x + largura / 2, y, true);
        else texto(l, f, c, x, y);
        y += alt_fonte(f) + espaco;
    }
    return y;
}

}  // namespace gfx

namespace gfx {
bool salvar_bmp(const char* caminho) {
    FILE* f = std::fopen(caminho, "wb");
    if (!f) return false;
    unsigned char h[54] = {'B', 'M'};
    auto w32 = [&](int off, uint32_t v) { for (int i = 0; i < 4; i++) h[off + i] = (unsigned char)(v >> (8 * i)); };
    w32(2, 54 + LARGURA * ALTURA * 4);
    w32(10, 54);
    w32(14, 40);
    w32(18, LARGURA);
    w32(22, (uint32_t)(-ALTURA));
    h[26] = 1;
    h[28] = 32;
    std::fwrite(h, 1, 54, f);
    std::fwrite(TELA.px, 4, (size_t)LARGURA * ALTURA, f);
    std::fclose(f);
    return true;
}
}  // namespace gfx
