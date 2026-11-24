// Protocolo Cronos++ - camada de plataforma SDL2 (Windows, Linux, macOS, Android e navegador via Emscripten).
// O núcleo do jogo só desenha num framebuffer e gera amostras de áudio; aqui ele é ligado à janela, ao som e à entrada.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include "audio.h"
#include "gfx.h"
#include "jogo.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {

// ---------------------------------------------------------------- armazenamento
#ifdef __EMSCRIPTEN__
struct ArmazenamentoWeb : Armazenamento {          // localStorage do navegador
    bool ler(std::string& t) override {
        char* p = (char*)EM_ASM_INT({
            var v = localStorage.getItem('cronos_dados');
            if (v === null) return 0;
            var n = lengthBytesUTF8(v) + 1;
            var m = _malloc(n);
            stringToUTF8(v, m, n);
            return m;
        });
        if (!p) return false;
        t = p;
        std::free(p);
        return true;
    }
    void gravar(const std::string& t) override { EM_ASM({ localStorage.setItem('cronos_dados', UTF8ToString($0)); }, t.c_str()); }
};
#else
struct ArmazenamentoArquivo : Armazenamento {      // arquivo na pasta de preferências do usuário
    std::string caminho;
    explicit ArmazenamentoArquivo(std::string c) : caminho(std::move(c)) {}
    bool ler(std::string& t) override {
        std::ifstream f(caminho.c_str(), std::ios::binary);
        if (!f) return false;
        t.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        return true;
    }
    void gravar(const std::string& t) override {
        std::ofstream f(caminho.c_str(), std::ios::binary | std::ios::trunc);
        if (f) f << t;
    }
};
#endif

// ---------------------------------------------------------------- aplicação
struct App {
    SDL_Window* janela = nullptr;
    SDL_Renderer* render = nullptr;
    SDL_Texture* textura = nullptr;
    SDL_AudioDeviceID dispositivo = 0;
    Audio som;
    std::unique_ptr<Armazenamento> arm;
    std::unique_ptr<Jogo> jogo;
    Uint64 anterior = 0;
    Uint32 ultimo_toque = 0;      // ms; ignora o mouse "fantasma" que alguns navegadores geram depois de um toque
    bool encerrar = false;
};

int traduzir_tecla(SDL_Keycode k) {
    if (k >= 'a' && k <= 'z') return (int)(k - 'a' + 'A');
    if (k >= '0' && k <= '9') return (int)k;
    switch (k) {
    case SDLK_RETURN: case SDLK_KP_ENTER: return K_RETURN;
    case SDLK_ESCAPE: return K_ESC;
    case SDLK_SPACE: return K_SPACE;
    case SDLK_BACKSPACE: return K_BACK;
    case SDLK_LEFT: return K_LEFT;
    case SDLK_RIGHT: return K_RIGHT;
    case SDLK_UP: return K_UP;
    case SDLK_DOWN: return K_DOWN;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return K_SHIFT;
    case SDLK_MINUS: return K_MENOS;
    case SDLK_KP_MINUS: return K_KP_MENOS;
    case SDLK_EQUALS: case SDLK_PLUS: return K_MAIS;
    case SDLK_KP_PLUS: return K_KP_MAIS;
    case SDLK_F11: return K_F11;
    default: return 0;
    }
}

void audio_callback(void* dados, Uint8* fluxo, int bytes) {
    static_cast<Audio*>(dados)->mixar(reinterpret_cast<short*>(fluxo), bytes / 2);
}

SDL_Surface* criar_icone() {          // capacete de mergulho desenhado por código
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return nullptr;
    Uint32* px = (Uint32*)s->pixels;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            float dx = x + 0.5f - 16, dy = y + 0.5f - 16, d = std::sqrt(dx * dx + dy * dy);
            float vx = x + 0.5f - 19, vy = y + 0.5f - 15, dv = std::sqrt(vx * vx + vy * vy);
            Uint32 c = 0;
            if (d <= 15) c = 0xFFFA961E;
            if (d <= 11) c = 0xFFCDD7E1;
            if (dv <= 6) c = 0xFF3CC8E6;
            px[y * (s->pitch / 4) + x] = c;
        }
    return s;
}

void enviar_texto(Jogo& jogo, const char* utf8) {       // UTF-8 -> eventos de caractere
    for (const unsigned char* p = (const unsigned char*)utf8; *p;) {
        unsigned cp;
        int n;
        if (*p < 0x80) { cp = *p; n = 1; }
        else if ((*p >> 5) == 6) { cp = *p & 0x1F; n = 2; }
        else if ((*p >> 4) == 14) { cp = *p & 0x0F; n = 3; }
        else { cp = *p & 0x07; n = 4; }
        for (int i = 1; i < n && p[i]; i++) cp = (cp << 6) | (p[i] & 0x3F);
        p += n;
        if (cp >= 32 && cp < 0xFFFF && cp != 127) {
            Evento e;
            e.tipo = Ev::Char;
            e.ch = (wchar_t)cp;
            jogo.tratar_evento(e);
        }
    }
}

void alternar_tela_cheia(App& a) {
#ifdef __EMSCRIPTEN__
    (void)a;      // no navegador a tela cheia é pedida pelo JavaScript da página (web/shell.html)
    return;
#endif
    bool cheia = (SDL_GetWindowFlags(a.janela) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
    SDL_SetWindowFullscreen(a.janela, cheia ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void tratar_eventos(App& a) {
    Jogo& jogo = *a.jogo;
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        Evento e;
        switch (ev.type) {
        case SDL_QUIT:
            e.tipo = Ev::Sair;
            jogo.tratar_evento(e);
            break;
        case SDL_KEYDOWN: case SDL_KEYUP: {
            int k = traduzir_tecla(ev.key.keysym.sym);
            if (!k) break;
            bool desce = ev.type == SDL_KEYDOWN;
            if (desce && k == K_F11) { if (!ev.key.repeat) alternar_tela_cheia(a); break; }
            entrada.tec[k & 255] = desce;
            if (desce && (!ev.key.repeat || k == K_BACK)) {
                e.tipo = Ev::Tecla;
                e.tecla = k;
                jogo.tratar_evento(e);
            }
            break;
        }
        case SDL_TEXTINPUT:
            enviar_texto(jogo, ev.text.text);
            break;
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
            if (SDL_GetTicks() - a.ultimo_toque < 700 && a.ultimo_toque) break;
            if (ev.button.which == SDL_TOUCH_MOUSEID) break;
            e.tipo = ev.type == SDL_MOUSEBUTTONDOWN ? Ev::MouseDown : Ev::MouseUp;
            e.botao = ev.button.button;
            e.x = ev.button.x; e.y = ev.button.y;
            entrada.mx = e.x; entrada.my = e.y;
            jogo.tratar_evento(e);
            break;
        case SDL_MOUSEMOTION:
            if (SDL_GetTicks() - a.ultimo_toque < 700 && a.ultimo_toque) break;
            if (ev.motion.which == SDL_TOUCH_MOUSEID) break;
            e.tipo = Ev::MouseMove;
            e.x = ev.motion.x; e.y = ev.motion.y;
            entrada.mx = e.x; entrada.my = e.y;
            jogo.tratar_evento(e);
            break;
        case SDL_FINGERDOWN: case SDL_FINGERMOTION: case SDL_FINGERUP: {
            a.ultimo_toque = SDL_GetTicks() ? SDL_GetTicks() : 1;
            int w, h;
            SDL_GetWindowSize(a.janela, &w, &h);
            float lx, ly;
            SDL_RenderWindowToLogical(a.render, (int)(ev.tfinger.x * w), (int)(ev.tfinger.y * h), &lx, &ly);
            e.tipo = ev.type == SDL_FINGERDOWN ? Ev::MouseDown : (ev.type == SDL_FINGERUP ? Ev::MouseUp : Ev::MouseMove);
            e.x = (int)lx; e.y = (int)ly;
            e.dedo = (int)(ev.tfinger.fingerId & 0x7FFFFFFF);
            e.toque = true;
            entrada.mx = e.x; entrada.my = e.y;
            jogo.tratar_evento(e);
            break;
        }
        case SDL_WINDOWEVENT:
            if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                std::fill(std::begin(entrada.tec), std::end(entrada.tec), false);
                jogo.perdeu_foco();
            }
            break;
        default:
            break;
        }
    }
    if (!jogo.rodando) a.encerrar = true;
}

void um_quadro(void* p) {
    App& a = *static_cast<App*>(p);
    tratar_eventos(a);
    Uint64 agora = SDL_GetPerformanceCounter();
    float dt = (float)((agora - a.anterior) / (double)SDL_GetPerformanceFrequency());
    a.anterior = agora;
    if (dt > 0.05f) dt = 0.05f;
    a.jogo->atualizar(dt);
    a.jogo->desenhar();
    SDL_UpdateTexture(a.textura, nullptr, gfx::pixels(), LARGURA * 4);
    SDL_RenderClear(a.render);
    SDL_RenderCopy(a.render, a.textura, nullptr, nullptr);
    SDL_RenderPresent(a.render);
#ifndef __EMSCRIPTEN__
    // garante ~60 quadros/s mesmo se o vsync do driver estiver desligado
    double gasto = (SDL_GetPerformanceCounter() - agora) / (double)SDL_GetPerformanceFrequency();
    if (gasto < 0.004) SDL_Delay(4);
#endif
}

}  // namespace

int main(int, char**) {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }
    App app;

    std::string assets = "assets";
#ifndef __EMSCRIPTEN__
    if (char* base = SDL_GetBasePath()) { assets = std::string(base) + "assets"; SDL_free(base); }
#endif
    if (!gfx::iniciar(assets)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Protocolo Cronos++", "Não foi possível carregar as fontes (pasta assets/fonts).", nullptr);
        return 1;
    }

    int jw = LARGURA, jh = ALTURA;
    SDL_Rect util;
    if (SDL_GetDisplayUsableBounds(0, &util) == 0 && (jw > util.w * 9 / 10 || jh > util.h * 9 / 10)) {
        float e = std::min(util.w * 0.9f / LARGURA, util.h * 0.9f / ALTURA);
        jw = (int)(LARGURA * e); jh = (int)(ALTURA * e);
    }
    app.janela = SDL_CreateWindow("Protocolo Cronos++: O Bunker Submerso", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, jw, jh,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!app.janela) { std::fprintf(stderr, "janela: %s\n", SDL_GetError()); return 1; }
    if (SDL_Surface* ic = criar_icone()) { SDL_SetWindowIcon(app.janela, ic); SDL_FreeSurface(ic); }
    app.render = SDL_CreateRenderer(app.janela, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.render) app.render = SDL_CreateRenderer(app.janela, -1, SDL_RENDERER_SOFTWARE);
    if (!app.render) { std::fprintf(stderr, "renderer: %s\n", SDL_GetError()); return 1; }
    SDL_RenderSetLogicalSize(app.render, LARGURA, ALTURA);       // letterbox automático + coordenadas do mouse do jogo
    app.textura = SDL_CreateTexture(app.render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

#ifdef __EMSCRIPTEN__
    app.arm.reset(new ArmazenamentoWeb());
#else
    if (char* pref = SDL_GetPrefPath("ProtocoloCronos", "CronosPP")) {
        app.arm.reset(new ArmazenamentoArquivo(std::string(pref) + "dados.cfg"));
        SDL_free(pref);
    }
#endif

    app.som.iniciar(true);
    SDL_AudioSpec quero;
    SDL_zero(quero);
    quero.freq = Audio::TAXA;
    quero.format = AUDIO_S16SYS;
    quero.channels = 1;
    quero.samples = 1024;
    quero.callback = audio_callback;
    quero.userdata = &app.som;
    SDL_AudioSpec obtido;
    app.dispositivo = SDL_OpenAudioDevice(nullptr, 0, &quero, &obtido, 0);
    if (app.dispositivo) SDL_PauseAudioDevice(app.dispositivo, 0);
    else app.som.ativo = false;

    app.jogo.reset(new Jogo(app.som, app.arm.get()));
    app.anterior = SDL_GetPerformanceCounter();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(um_quadro, &app, 0, 1);
#else
    while (!app.encerrar) um_quadro(&app);
#endif

    if (app.dispositivo) SDL_CloseAudioDevice(app.dispositivo);
    app.jogo.reset();
    SDL_DestroyTexture(app.textura);
    SDL_DestroyRenderer(app.render);
    SDL_DestroyWindow(app.janela);
    SDL_Quit();
    return 0;
}
