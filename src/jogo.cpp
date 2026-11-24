#include "jogo.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "calc.h"

using namespace gfx;

Entrada entrada;
Rng& rng_global() {
    static Rng r((uint32_t)std::rand() * 2654435761u + 12345u);
    return r;
}

namespace {
const Cor BRANCO = rgb(235, 240, 245), CINZA = rgb(140, 155, 170), CIANO = rgb(70, 220, 235),
          VERDE = rgb(70, 235, 130), VERMELHO = rgb(245, 70, 60), AMARELO = rgb(250, 200, 60),
          FUNDO = rgb(6, 12, 20), PRETO = rgb(0, 0, 0);
const float CINEMA_TOTAL = 16.0f;

bool tec(const Evento& e, int k) { return e.tipo == Ev::Tecla && e.tecla == k; }
bool tec2(const Evento& e, int a, int b) { return e.tipo == Ev::Tecla && (e.tecla == a || e.tecla == b); }
std::wstring num2(int v) { return (v < 10 ? L"0" : L"") + a_wstr(v); }
Cor pisca(float t, Cor a, Cor b) { return ((int)(t * 2)) % 2 == 0 ? a : b; }
}  // namespace

std::wstring formatar_tempo(int s) { s = std::max(0, s); return num2(s / 60) + L":" + num2(s % 60); }

// ------------------------------------------------------------------ persistência
void carregar_dados(Armazenamento* a, Dados& d) {
    std::string txt;
    if (!a || !a->ler(txt)) return;
    size_t i = 0;
    while (i < txt.size()) {
        size_t fim = txt.find('\n', i);
        if (fim == std::string::npos) fim = txt.size();
        std::string linha = txt.substr(i, fim - i);
        i = fim + 1;
        size_t eq = linha.find('=');
        if (eq == std::string::npos) continue;
        std::string k = linha.substr(0, eq), v = linha.substr(eq + 1);
        if (k == "melhor_pontos") d.melhor_pontos = std::atoi(v.c_str());
        else if (k == "melhor_tempo") d.melhor_tempo = std::atoi(v.c_str());
        else if (k == "partidas") d.partidas = std::atoi(v.c_str());
        else if (k == "vitorias") d.vitorias = std::atoi(v.c_str());
        else if (k == "volume") d.volume = limitar((float)std::atof(v.c_str()), 0.f, 1.f);
        else if (k == "mudo") d.mudo = std::atoi(v.c_str()) != 0;
        else if (k == "melhor_letra" && !v.empty() && v[0] >= 'A' && v[0] <= 'Z') d.melhor_letra = std::wstring(1, (wchar_t)v[0]);
    }
}

void salvar_dados(Armazenamento* a, const Dados& d) {
    if (!a) return;
    char buf[256];
    std::snprintf(buf, sizeof buf, "melhor_pontos=%d\nmelhor_tempo=%d\nmelhor_letra=%c\npartidas=%d\nvitorias=%d\nvolume=%.2f\nmudo=%d\n",
                  d.melhor_pontos, d.melhor_tempo, (char)d.melhor_letra[0], d.partidas, d.vitorias, d.volume, d.mudo ? 1 : 0);
    a->gravar(buf);
}

// ------------------------------------------------------------------ controles de toque
void ControlesToque::layout(int faixa) {
    lateral = faixa >= 150;
    if (lateral) {                // controles nas faixas laterais, fora do mapa (coordenadas do jogo: x < 0 e x > 1024)
        // mais perto do mapa do que do meio da faixa: longe das bordas do aparelho (cantos arredondados, gestos do sistema)
        float perto = faixa * 0.36f;
        raio = std::min(80.0f, perto + 6);
        bx = -perto; by = ALTURA - 190;
        re = std::min(64.0f, perto + 6);
        ex = LARGURA + perto; ey = ALTURA - 250;
        nx = ex; ny = ALTURA - 100; rn = std::min(44.0f, re * 0.7f);
    } else {                      // sobre os cantos do mapa
        raio = 74; bx = 130; by = ALTURA - 130;
        ex = LARGURA - 120; ey = ALTURA - 130; re = 58;
        nx = LARGURA - 250; ny = ALTURA - 80; rn = 38;
    }
    soltar_tudo();
}

void ControlesToque::soltar_tudo() { dedo_joy = -2; vx = vy = 0; kx = bx; ky = by; }

ControlesToque::Acao ControlesToque::apertar(int dedo, int x, int y) {
    auto dentro = [&](float cx, float cy, float r) { return std::hypot(x - cx, y - cy) <= r; };
    if (dentro(ex, ey, re)) return E;
    if (dentro(nx, ny, rn)) return N;
    if (dentro(bx, by, raio * 1.6f)) { dedo_joy = dedo; mover(dedo, x, y); return JOY; }
    return NENHUMA;
}

void ControlesToque::mover(int dedo, int x, int y) {
    if (dedo != dedo_joy) return;
    float dx = x - bx, dy = y - by, d = std::hypot(dx, dy);
    if (d > raio) { dx = dx / d * raio; dy = dy / d * raio; }
    kx = bx + dx; ky = by + dy;
    vx = dx / raio; vy = dy / raio;
}

void ControlesToque::soltar(int dedo) { if (dedo == dedo_joy) soltar_tudo(); }

void ControlesToque::desenhar(bool perto) const {
    circ(bx, by, raio, CIANO, 50);
    circ_borda(bx, by, raio, 3, CIANO, 160);
    circ(kx, ky, raio * 0.4f, rgb(200, 245, 255), 190);
    circ(ex, ey, re, perto ? AMARELO : rgb(120, 140, 160), perto ? 220 : 150);
    circ_borda(ex, ey, re, 3, rgb(255, 255, 255), 190);
    circ(nx, ny, rn, rgb(238, 228, 190), 170);
    circ_borda(nx, ny, rn, 3, rgb(255, 255, 255), 190);
    texto(L"E", F_GRANDE, rgb(20, 20, 20), (int)ex, (int)ey - 28, true, false);
    texto(L"ACESSAR", F_PEQ, rgb(20, 20, 20), (int)ex, (int)ey + 8, true, false);
    texto(L"N", F_TITULO, rgb(60, 40, 20), (int)nx, (int)ny - 22, true, false);
    texto(L"NOTAS", F_HUD_L, rgb(60, 40, 20), (int)nx, (int)ny + 6, true, false);
}

// ------------------------------------------------------------------ utilitários de interface
void painel(const Ret& r, Cor cor, Cor borda, int alfa, int raio) {
    rret(r, raio, cor, alfa);
    rret_borda(r, raio, 2, borda);
}

void veu(int alfa) { ret(0, 0, LARGURA, ALTURA, PRETO, alfa); }

void Botao::desenhar(Fonte f) const {
    bool sobre = rect.contem(entrada.mx, entrada.my);
    Cor c = sobre ? somar(cor, 40) : cor;
    rret(rect, 10, c);
    rret_borda(rect, 10, 2, CIANO);
    gfx::texto(rot, f, BRANCO, rect.cx(), rect.y + 10, true);
}

// ------------------------------------------------------------------ Baralho
Baralho::Baralho(int n) : numero(n) { reabastecer({}); }

void Baralho::reabastecer(const std::vector<std::wstring>& excluir) {
    restantes.clear();
    static std::vector<Questao> todas;
    if (todas.empty()) todas = banco_questoes();
    for (const auto& q : todas)
        if (q.fase == numero && std::find(excluir.begin(), excluir.end(), q.id) == excluir.end()) restantes.push_back(&q);
    Rng& r = rng_global();
    for (int i = (int)restantes.size() - 1; i > 0; i--) std::swap(restantes[i], restantes[r.ent(0, i)]);
}

const Questao* Baralho::sortear(const std::vector<std::wstring>& em_uso) {
    if (restantes.empty()) reabastecer(em_uso);
    const Questao* q = restantes.back();
    restantes.pop_back();
    return q;
}

// ------------------------------------------------------------------ TecladoTela
TecladoTela::TecladoTela(std::vector<std::vector<Def>> l, int tw_, int th_, int esp) : linhas(std::move(l)), tw(tw_), th(th_), espaco(esp) {}

void TecladoTela::tamanho(int& w, int& h) const {
    int cols = 0;
    for (auto& l : linhas) {
        int c = 0;
        for (auto& d : l) c += d.cols;
        cols = std::max(cols, c);
    }
    w = cols * tw + (cols - 1) * espaco;
    h = (int)linhas.size() * th + ((int)linhas.size() - 1) * espaco;
}

void TecladoTela::posicionar(int x, int y) {
    teclas.clear();
    for (size_t i = 0; i < linhas.size(); i++) {
        int cx = x;
        for (auto& d : linhas[i]) {
            int w = d.cols * tw + (d.cols - 1) * espaco;
            teclas.push_back({Ret(cx, y + (int)i * (th + espaco), w, th), d.rotulo, d.valor});
            cx += w + espaco;
        }
    }
}

std::wstring TecladoTela::tocar(int px, int py, float agora) {
    for (auto& k : teclas)
        if (k.rect.contem(px, py)) { apertada = k.valor; apertada_t = agora; return k.valor; }
    return L"";
}

void TecladoTela::desenhar(Fonte f, Cor fundo, Cor borda, Cor txt, const std::set<std::wstring>& apagadas, float agora) const {
    bool recente = agora - apertada_t < 0.15f;
    for (auto& k : teclas) {
        Cor cor = (recente && k.valor == apertada) ? somar(fundo, 60) : fundo;
        bool desligada = apagadas.count(k.valor) > 0;
        if (desligada) cor = rgb((cr(fundo) + 200) / 2, (cg(fundo) + 200) / 2, (cb(fundo) + 200) / 2);
        rret(k.rect, 6, cor);
        rret_borda(k.rect, 6, 2, borda);
        Cor ct = desligada ? rgb(150, 140, 120) : txt;
        texto(k.rotulo, f, ct, k.rect.cx(), k.rect.cy() - alt_fonte(f) / 2, true, false);
    }
}

static std::vector<TecladoTela::Def> K(std::initializer_list<const wchar_t*> rotulos) {
    std::vector<TecladoTela::Def> v;
    for (auto r : rotulos) v.push_back({r, r, 1});
    return v;
}

// ------------------------------------------------------------------ BlocoNotas
BlocoNotas::BlocoNotas() {
    std::vector<TecladoTela::Def> l4 = K({L"0", L".", L"^", L"+"});
    l4.push_back({L"<-", L"APAGAR", 1});
    teclado = TecladoTela({
        K({L"7", L"8", L"9", L"/", L"("}),
        K({L"4", L"5", L"6", L"*", L")"}),
        K({L"1", L"2", L"3", L"-", L"!"}),
        l4,
        {{L"CE", L"LIMPAR", 1}, {L"=", L"CALCULAR", 2}, {L"Fechar", L"FECHAR", 2}},
        {{L"Usar na resposta", L"RESPOSTA", 5}},
    });
    int w, h;
    teclado.tamanho(w, h);
    Ret r = RECT();
    teclado.posicionar(r.cx() - w / 2, r.baixo() - h - 12);
    reiniciar();
}

void BlocoNotas::reiniciar() { linhas.assign(1, {L"", 0}); }

std::wstring BlocoNotas::ultimo_resultado() const {
    for (auto it = linhas.rbegin(); it != linhas.rend(); ++it)
        if (it->second == 1) return it->first.substr(2);
    return L"";
}

std::wstring BlocoNotas::resultado_inteiro() const {
    std::wstring r = ultimo_resultado();
    if (r.empty()) return L"";
    for (wchar_t c : r) if (c < L'0' || c > L'9') return L"";
    return r;
}

void BlocoNotas::calcular() {
    const std::wstring& atual = linhas.back().first;
    if (atual.find_first_not_of(L' ') == std::wstring::npos) return;     // '=' numa linha vazia não cria linhas
    std::wstring res;
    if (avaliar_expressao(atual, res)) linhas.push_back({L"= " + res, 1});
    else linhas.push_back({L"= erro na expressão", 2});
    linhas.push_back({L"", 0});
    if (linhas.size() > 40) linhas.erase(linhas.begin(), linhas.end() - 40);
}

void BlocoNotas::apagar() {
    std::wstring& txt = linhas.back().first;
    if (!txt.empty()) txt.pop_back();
    else if (linhas.size() > 1) {
        linhas.pop_back();
        if (linhas.back().second != 0) linhas.pop_back();
    }
}

void BlocoNotas::digitar(wchar_t ch) {
    std::wstring atual = linhas.back().first;
    // Continuar a conta: operador logo após um resultado usa o resultado (como numa calculadora).
    if (atual.empty() && std::wstring(L"+-*/^!x×").find(ch) != std::wstring::npos && linhas.size() >= 2 &&
        linhas[linhas.size() - 2].second == 1)
        atual = linhas[linhas.size() - 2].first.substr(2);
    std::wstring txt = atual + ch;
    if (larg_texto(txt, F_MONO) <= RECT().w - 34) linhas.back() = {txt, 0};
}

bool BlocoNotas::evento(const Evento& e, float agora) {
    if (e.tipo == Ev::MouseDown && e.botao == 1) {
        std::wstring v = teclado.tocar(e.x, e.y, agora);
        if (v == L"FECHAR") aberto = false;
        else if (v == L"CALCULAR") calcular();
        else if (v == L"APAGAR") apagar();
        else if (v == L"LIMPAR") linhas.back() = {L"", 0};
        else if (v == L"RESPOSTA") return pode_responder && !resultado_inteiro().empty();
        else if (!v.empty()) digitar(v[0]);
        return false;
    }
    if (e.tipo == Ev::Tecla) {
        if (e.tecla == 'N' || e.tecla == K_ESC) aberto = false;
        else if (e.tecla == K_RETURN) calcular();
        else if (e.tecla == K_BACK) apagar();
    } else if (e.tipo == Ev::Char) {
        if (std::wstring(L"0123456789+-*/^()!.,=xX×÷").find(e.ch) != std::wstring::npos && e.ch != L'=') digitar(e.ch);
        else if (e.ch == L'=') calcular();
    }
    return false;
}

void BlocoNotas::desenhar(float t) const {
    Ret r = RECT();
    painel(r, rgb(238, 228, 190), rgb(120, 90, 40), 245, 8);
    Cor tinta = rgb(40, 36, 30);
    texto(L"BLOCO DE NOTAS", F_MONO, rgb(120, 60, 20), r.cx(), r.y + 8, true, false);
    linha((float)r.x + 12, (float)r.y + 34, (float)r.dir() - 12, (float)r.y + 34, 2, rgb(190, 120, 90));
    int y = r.y + 42;
    size_t ini = linhas.size() > 7 ? linhas.size() - 7 : 0;
    const Cor cores[3] = {tinta, rgb(20, 110, 60), rgb(170, 40, 30)};
    for (size_t i = ini; i < linhas.size(); i++) {
        texto(linhas[i].first, F_MONO, cores[linhas[i].second], r.x + 14, y, false, false);
        y += 23;
    }
    if ((int)(t * 2) % 2 == 0) {
        int cx = r.x + 14 + larg_texto(linhas.back().first, F_MONO) + 1;
        int yy = r.y + 42 + (int)(linhas.size() - ini - 1) * 23;
        linha((float)cx, (float)yy + 2, (float)cx, (float)yy + 19, 2, tinta);
    }
    std::wstring dica = ultimo_resultado().empty() ? L"Ex: 8!/(3!*5!) e =" : L"Após o =, use * + - /";
    texto(dica, F_MONO, rgb(110, 90, 60), r.x + 14, teclado.teclas[0].rect.y - 26, false, false);
    bool usar = pode_responder && !resultado_inteiro().empty();
    std::set<std::wstring> apagadas;
    if (!usar) apagadas.insert(L"RESPOSTA");
    teclado.desenhar(F_MONO, rgb(214, 200, 160), rgb(140, 110, 60), tinta, apagadas, t);
    if (usar)
        for (auto& k : teclado.teclas)
            if (k.valor == L"RESPOSTA") rret_borda(k.rect, 6, 3, rgb(30, 140, 70));
}

// ------------------------------------------------------------------ Desafio
Desafio::Desafio(Jogo* j, Terminal* t) : jogo(j), terminal(t) {
    btn = Ret(0, 0, 240, 48);
    btn_notas = Ret(0, 0, 150, 42);
    btn_sair = Ret(0, 0, 150, 42);
    teclado = TecladoTela({K({L"7", L"8", L"9"}), K({L"4", L"5", L"6"}), K({L"1", L"2", L"3"}),
                           {{L"<-", L"APAGAR", 1}, {L"0", L"0", 1}, {L"OK", L"OK", 1}}}, 66, 50, 6);
    layout();
}

Ret Desafio::painel_rect() const {
    int w = 720;
    int x = jogo->notas.aberto ? 16 : (LARGURA - w) / 2;
    return Ret(x, 46, w, 548);
}

void Desafio::layout() {
    Ret r = painel_rect();
    int w, h;
    teclado.tamanho(w, h);
    teclado.posicionar(r.dir() - 30 - w, r.baixo() - 30 - h);
    btn_notas.x = r.x + 30; btn_notas.y = r.baixo() - 108;
    btn_sair.x = r.x + 192; btn_sair.y = r.baixo() - 108;
    btn.x = r.cx() - btn.w / 2; btn.y = r.baixo() - 18 - btn.h;
}

void Desafio::evento(const Evento& e) {
    layout();
    bool clique = e.tipo == Ev::MouseDown && e.botao == 1;
    if (!feedback) {
        if (clique) {
            std::wstring v = teclado.tocar(e.x, e.y, jogo->t);
            if (v == L"APAGAR") { if (!entrada.empty()) entrada.pop_back(); }
            else if (v == L"OK") { if (!entrada.empty()) enviar(); }
            else if (!v.empty()) { if (entrada.size() < 9) entrada += v; }
            else if (btn_notas.contem(e.x, e.y)) jogo->notas.aberto = !jogo->notas.aberto;
            else if (btn_sair.contem(e.x, e.y)) jogo->desafio.reset();
        } else if (e.tipo == Ev::Tecla) {
            if (e.tecla == K_ESC) jogo->desafio.reset();
            else if (e.tecla == K_BACK) { if (!entrada.empty()) entrada.pop_back(); }
            else if (e.tecla == K_RETURN) { if (!entrada.empty()) enviar(); }
        } else if (e.tipo == Ev::Char) {
            if (e.ch >= L'0' && e.ch <= L'9' && entrada.size() < 9) entrada += e.ch;
        }
    } else {
        if (tec2(e, K_RETURN, K_SPACE) || (clique && btn.contem(e.x, e.y))) jogo->continuar_desafio(this);
    }
}

void Desafio::enviar() {
    jogo->notas.aberto = false;           // respondeu: o bloco de notas fecha sozinho
    resolvida = questao();
    acertou = entrada == resolvida->resposta;
    feedback = true;
    jogo->processar_resposta(terminal, acertou);
}

void Desafio::novo_enunciado() { entrada.clear(); feedback = false; }

void Desafio::botao(const Ret& r, const std::wstring& txt, Cor cor) const {
    bool sobre = r.contem(::entrada.mx, ::entrada.my);
    rret(r, 8, sobre ? rgb(30, 110, 140) : rgb(20, 70, 100));
    rret_borda(r, 8, 2, cor);
    texto(txt, F_TEXTO, BRANCO, r.cx(), r.cy() - alt_fonte(F_TEXTO) / 2, true);
}

void Desafio::desenhar(float t) {
    layout();
    veu(170);
    Ret r = painel_rect();
    Cor cor = !feedback ? CIANO : (acertou ? VERDE : VERMELHO);
    painel(r, rgb(10, 20, 32), cor);
    int x = r.x + 30, larg = r.w - 60;
    const Fase& f = *jogo->fase;
    const Questao* q = feedback ? resolvida : questao();
    texto(L"TERMINAL: " + maiusc(terminal->nome), F_TITULO, cor, x, r.y + 16);
    texto(L"Fase " + a_wstr(f.idx + 1) + L" · " + info_fase(f.idx).conteudo + L" · Questão " + q->id,
          F_PEQ, CINZA, x, r.y + 52);
    linha((float)x, (float)r.y + 78, (float)r.dir() - 30, (float)r.y + 78, 1, rgb(50, 80, 100));
    if (!feedback) {
        paragrafo(q->enunciado, F_TEXTO, BRANCO, x, r.y + 92, larg, 5);
        Ret caixa(x, r.baixo() - 178, 312, 54);
        texto(L"Sua resposta (apenas números):", F_PEQ, CINZA, x, caixa.y - 24);
        rret(caixa, 6, rgb(4, 10, 16));
        rret_borda(caixa, 6, 2, cor);
        std::wstring cursor = (int)(t * 2) % 2 == 0 ? L"_" : L" ";
        texto(entrada + cursor, F_GRANDE, BRANCO, caixa.x + 14, caixa.y + 6);
        botao(btn_notas, L"Notas [N]", CIANO);
        botao(btn_sair, L"Sair [Esc]", CIANO);
        texto(L"Erros: " + a_wstr(jogo->erros) + L"/" + a_wstr(MAX_ERROS) + L"  (cada erro: +1 falha e -" +
                  a_wstr(PENALIDADE_ERRO) + L"s)", F_PEQ, AMARELO, x, r.baixo() - 50);
        teclado.desenhar(F_GRANDE, rgb(16, 44, 60), cor, BRANCO, {}, jogo->t);
    } else {
        std::wstring msg;
        if (acertou) {
            texto(L"ACESSO CONCEDIDO", F_GRANDE, VERDE, x, r.y + 90);
            msg = L"Protocolo quebrado. O terminal ficou VERDE.";
        } else {
            texto(L"SOBRECARGA NO TERMINAL!", F_GRANDE, VERMELHO, x, r.y + 90);
            msg = L"Você respondeu " + entrada + L". A resposta correta era " + q->resposta + L". +1 falha crítica e -" +
                  a_wstr(PENALIDADE_ERRO) + L"s. Uma nova questão foi sorteada.";
        }
        int y = paragrafo(msg, F_TEXTO, BRANCO, x, r.y + 136, larg, 4);
        texto(L"Resolução:", F_TEXTO, AMARELO, x, y + 8);
        paragrafo(q->resolucao, F_TEXTO, rgb(210, 230, 240), x, y + 38, larg, 4);
        botao(btn, L"Continuar [Enter]", cor);
    }
}

// ------------------------------------------------------------------ Jogo
Jogo::Jogo(Audio& s, Armazenamento* armazenamento) : som(s), arm(armazenamento) {
    carregar_dados(arm, dados);
    som.definir_volume(dados.volume);
    if (dados.mudo && !som.mudo) som.alternar_mudo();
    for (int i = 0; i < 40; i++) {
        Rng& r = rng_global();
        bolhas.push_back({r.entre(0, LARGURA), r.entre(0, ALTURA), r.entre(2, 7), r.entre(10, 40), 0, 0});
    }
    btn_iniciar = Botao(Ret(LARGURA / 2 - 150, 560, 300, 56), L"INICIAR MISSÃO");
    btn_novo = Botao(Ret(LARGURA / 2 - 170, 526, 340, 56), L"INICIAR NOVA PARTIDA");
    btn_fugir = Botao(Ret(90, 360, 400, 120), L"1 · FUGIR NA CÁPSULA", rgb(20, 80, 60));
    btn_sobrecarga = Botao(Ret(534, 360, 400, 120), L"2 · SOBRECARREGAR O REATOR", rgb(110, 40, 30));
    btn_continuar = Botao(Ret(LARGURA / 2 - 230, 400, 220, 56), L"CONTINUAR");
    btn_menu = Botao(Ret(LARGURA / 2 + 10, 400, 220, 56), L"MENU", rgb(90, 50, 30));
    r_som = Ret(LARGURA - 48, 12, 40, 40);
    r_pausa = Ret(LARGURA - 94, 12, 40, 40);
    novo_jogo(false);
}

void Jogo::salvar() {
    dados.volume = som.volume;
    dados.mudo = som.mudo;
    salvar_dados(arm, dados);
}

void Jogo::novo_jogo(bool iniciar) {
    tempo = (float)TEMPO_TOTAL;
    erros = acertos_total = 0;
    baralhos.clear();
    for (int i = 1; i <= NUM_FASES; i++) baralhos.emplace_back(i);
    fase_idx = 0;
    desafio.reset();
    notas.reiniciar();
    notas.aberto = false;
    flash_verm = flash_verde = tremor = 0;
    aviso.clear(); aviso_t = 0;
    derrota_pendente.clear();
    motivo_derrota.clear();
    final_escolhido = -1;
    passo_t = 0;
    pausado = false;
    fogos.clear();
    cinema_t = 0;
    novo_recorde = false;
    luz_dx = 1; luz_dy = 0;
    carregar_fase(0);
    if (iniciar) {
        estado = INTRO;
        intro_t = 0; intro_saida = -1;
        som.tocar("fase");
    }
}

void Jogo::carregar_fase(int idx) {
    fase_idx = idx;
    desafio.reset();
    fase.reset(new Fase(idx));
    jogador.reset(new Jogador(fase->inicio_x, fase->inicio_y));
}

// ---------------------------------------------------------------- regras
void Jogo::processar_resposta(Terminal* term, bool acertou) {
    if (acertou) {
        term->feita = true;
        acertos_total++;
        flash_verde = 0.6f;
        som.tocar("acerto");
        if (fase->acertos() >= fase->meta() && !fase->porta.aberta) {
            fase->porta.abrir();
            som.tocar("porta", 0.6f);
            avisar(L"Meta cumprida! A porta hidráulica está se abrindo.", 4.0f);
        }
    } else {
        erros++;
        tempo -= PENALIDADE_ERRO;
        flash_verm = 0.7f;
        tremor = 0.6f;
        som.tocar("erro");
        std::vector<std::wstring> em_uso;
        for (auto& t : fase->terminais) if (t.questao) em_uso.push_back(t.questao->id);
        term->questao = baralhos[fase_idx].sortear(em_uso);
        if (erros >= MAX_ERROS) derrota_pendente = L"Falhas críticas demais: o sistema do bunker colapsou.";
        else if (tempo <= 0) derrota_pendente = L"O tempo acabou: o oxigênio da estação se esgotou.";
    }
}

void Jogo::continuar_desafio(Desafio* d) {
    if (!derrota_pendente.empty()) encerrar(DERROTA, derrota_pendente);
    else if (d->acertou) desafio.reset();
    else d->novo_enunciado();
}

Jogo::Classif Jogo::classificacao() const {
    int t = std::max(0, (int)tempo), e = erros;
    int pontos = std::max(0, TOTAL_ACERTOS * 100 + t * 2 - e * 150);
    if (e == 0 && t >= 300) return {L"S", L"Mestre(a) da Combinatória", AMARELO, pontos};
    if (e <= 1 && t >= 180) return {L"A", L"Engenheiro(a) do Protocolo", VERDE, pontos};
    if (e <= 3) return {L"B", L"Sobrevivente Calculista", CIANO, pontos};
    return {L"C", L"Escapou por um triz", rgb(200, 160, 255), pontos};
}

void Jogo::encerrar(Estado e, const std::wstring& motivo) {
    estado = e;
    motivo_derrota = motivo;
    desafio.reset();
    notas.aberto = false;
    tremor = 0;
    dados.partidas++;
    if (e == DERROTA) {
        som.tocar("derrota");
    } else {
        fogos.clear();
        dados.vitorias++;
        Classif c = classificacao();
        if (c.pontos > dados.melhor_pontos) {
            dados.melhor_pontos = c.pontos;
            dados.melhor_tempo = std::max(0, (int)tempo);
            dados.melhor_letra = c.letra;
            novo_recorde = true;
        }
        som.tocar(som.tem("tema") ? "tema" : "vitoria", 0.6f);
    }
    salvar();
}

Terminal* Jogo::terminal_proximo() {
    for (auto& t : fase->terminais)
        if (!t.feita && t.rect.inflar(34, 34).colide(jogador->rect)) return &t;
    return nullptr;
}

void Jogo::interagir() {
    Terminal* t = terminal_proximo();
    if (!t) return;
    if (!t->questao) {
        std::vector<std::wstring> em_uso;
        for (auto& x : fase->terminais) if (x.questao) em_uso.push_back(x.questao->id);
        t->questao = baralhos[fase_idx].sortear(em_uso);
    }
    desafio.reset(new Desafio(this, t));
    som.tocar("bipe");
}

// ---------------------------------------------------------------- volume
bool Jogo::r_volume(Ret& r) const {
    if (estado == JOGANDO) {
        if (!pausado) return false;
        r = Ret(LARGURA / 2 - 200, 486, 400, 44);
        return true;
    }
    if (estado == INTRO || estado == CINEMA) return false;
    r = Ret(LARGURA - 8 - 160, 58, 160, 38);
    return true;
}

void Jogo::ajustar_volume(int x) {
    Ret r;
    if (!r_volume(r)) return;
    som.definir_volume((float)(x - (r.x + 14)) / (r.w - 28));
    if (som.mudo && som.volume > 0) som.alternar_mudo();
}

// ---------------------------------------------------------------- eventos
// Botões de sistema e controles de toque. Devolve true se o clique/toque foi consumido.
bool Jogo::apertar(int dedo, int x, int y) {
    if (r_som.contem(x, y)) { som.alternar_mudo(); salvar(); return true; }
    Ret rv;
    if (r_volume(rv) && rv.inflar(24, 20).contem(x, y)) { arrastando_vol = true; dedo_vol = dedo; ajustar_volume(x); return true; }
    if (estado != JOGANDO || pausado) return false;
    if (r_pausa.contem(x, y)) { pausado = true; toque.soltar_tudo(); return true; }
    if (toque.ativo && !desafio && !notas.aberto) {
        ControlesToque::Acao a = toque.apertar(dedo, x, y);
        if (a == ControlesToque::E) interagir();
        else if (a == ControlesToque::N) { notas.aberto = true; toque.soltar_tudo(); }
        return a != ControlesToque::NENHUMA;
    }
    return false;
}

void Jogo::tratar_evento(const Evento& e) {
    if (e.tipo == Ev::Sair) { rodando = false; return; }
    if (e.toque) toque.ativo = true;
    if (e.tipo == Ev::MouseDown && e.botao == 1) {
        if (apertar(e.dedo, e.x, e.y)) return;
    } else if (e.tipo == Ev::MouseMove) {
        if (arrastando_vol && dedo_vol == e.dedo) ajustar_volume(e.x);
        toque.mover(e.dedo, e.x, e.y);
        return;
    } else if (e.tipo == Ev::MouseUp) {
        if (arrastando_vol && dedo_vol == e.dedo) { arrastando_vol = false; salvar(); }
        toque.soltar(e.dedo);
        return;
    }
    // --- atalhos globais ---
    if (e.tipo == Ev::Tecla && !notas.aberto) {
        if (e.tecla == 'M') { som.alternar_mudo(); salvar(); return; }
        if (e.tecla == 'T' && !desafio) { toque.ativo = !toque.ativo; toque.soltar_tudo(); return; }
        if (tec2(e, K_MENOS, K_KP_MENOS)) { som.definir_volume(som.volume - 0.1f); salvar(); return; }
        if (tec2(e, K_MAIS, K_KP_MAIS)) {
            som.definir_volume(som.volume + 0.1f);
            if (som.mudo) som.alternar_mudo();
            salvar();
            return;
        }
    }
    bool enter_ou_espaco = tec2(e, K_RETURN, K_SPACE);
    bool clique = e.tipo == Ev::MouseDown && e.botao == 1;
    switch (estado) {
    case MENU:
        if (enter_ou_espaco || btn_iniciar.clicado(e)) novo_jogo();
        else if (tec(e, K_ESC)) rodando = false;
        break;
    case INTRO:
        if ((enter_ou_espaco || clique) && intro_t > 0.5f && intro_saida < 0) intro_saida = 0;
        break;
    case JOGANDO:
        if (pausado) {
            if (tec(e, K_ESC) || tec(e, 'P') || tec(e, K_RETURN) || btn_continuar.clicado(e)) pausado = false;
            else if (tec(e, 'Q') || btn_menu.clicado(e)) { pausado = false; estado = MENU; }
        } else if (notas.aberto) {
            if (desafio && clique && !BlocoNotas::RECT().contem(e.x, e.y)) desafio->evento(e);
            else if (notas.evento(e, t) && desafio) {
                desafio->entrada = notas.resultado_inteiro().substr(0, 9);
                notas.aberto = false;
            }
        } else if (tec(e, 'P')) pausado = true;
        else if (!desafio && tec(e, K_ESC)) pausado = true;
        else if (tec(e, 'N')) notas.aberto = true;
        else if (desafio) desafio->evento(e);
        else if (tec(e, 'E')) interagir();
        break;
    case ESCOLHA_FINAL:
        if (tec(e, '1') || btn_fugir.clicado(e)) iniciar_cinema(0);
        else if (tec(e, '2') || btn_sobrecarga.clicado(e)) iniciar_cinema(1);
        break;
    case CINEMA:
        if ((enter_ou_espaco || tec(e, K_ESC) || clique) && cinema_t > 1.0f) encerrar(VITORIA);
        break;
    case VITORIA:
    case DERROTA:
        if (tec2(e, K_RETURN, 'R') || btn_novo.clicado(e)) novo_jogo();
        break;
    }
}

// ---------------------------------------------------------------- atualização
void Jogo::perdeu_foco() {
    if (estado == JOGANDO && !desafio && !notas.aberto) pausado = true;
}

void Jogo::atualizar(float dt) {
    t += dt;
    notas.pode_responder = desafio && !desafio->feedback;
    for (auto& b : bolhas) {
        b.y -= b.vy * dt;
        b.x += std::sin(t + b.vx) * 8 * dt;
        if (b.y < -10) { b.x = rng_global().entre(0, LARGURA); b.y = ALTURA + 10; }
    }
    if (estado == INTRO) {
        intro_t += dt;
        if (intro_saida >= 0) {
            intro_saida += dt;
            if (intro_saida >= 0.5f) estado = JOGANDO;
        }
    } else if (estado == JOGANDO && !pausado) {
        atualizar_jogo(dt);
    } else if (estado == CINEMA) {
        atualizar_cinema(dt);
    } else if (estado == VITORIA) {
        for (auto& f : fogos) { f.x += f.vx * dt; f.y += f.vy * dt; f.vy += 40 * dt; f.vida -= dt; }
        fogos.erase(std::remove_if(fogos.begin(), fogos.end(), [](const Particula& f) { return f.vida <= 0; }), fogos.end());
        Rng& r = rng_global();
        if (r.f() < 0.08f) {                       // brilhos subindo na tela de vitória
            float cx = r.entre(120, LARGURA - 120);
            const Cor cores[4] = {VERDE, AMARELO, CIANO, rgb(255, 140, 200)};
            Cor cor = cores[r.ent(0, 3)];
            for (int i = 0; i < 18; i++) {
                float ang = r.entre(0, 2 * PI_), v = r.entre(40, 130);
                fogos.push_back({cx, r.entre(40, 200), std::cos(ang) * v, std::sin(ang) * v, r.entre(0.8f, 1.6f), cor});
            }
        }
    }
}

void Jogo::atualizar_jogo(float dt) {
    tempo -= dt;
    if (tempo <= 0 && derrota_pendente.empty()) {
        tempo = 0;
        encerrar(DERROTA, L"O tempo acabou: o oxigênio da estação se esgotou.");
        return;
    }
    Fase& f = *fase;
    bool bloqueado = desafio || notas.aberto;
    entrada.joy_x = toque.ativo ? toque.vx : 0.f;
    entrada.joy_y = toque.ativo ? toque.vy : 0.f;
    jogador->atualizar(dt, f.colisores(), !bloqueado);
    // a lanterna acompanha a direção do mergulhador com suavização
    float k = 1 - std::exp(-12 * dt);
    luz_dx += (jogador->dir_x - luz_dx) * k;
    luz_dy += (jogador->dir_y - luz_dy) * k;
    if (jogador->movendo && !bloqueado) {
        passo_t += dt;
        if (passo_t > (jogador->correndo ? 0.2f : 0.3f)) { passo_t = 0; som.tocar("passo", 0.3f); }
    }
    f.porta.atualizar(dt);
    flash_verm = std::max(0.f, flash_verm - dt);
    flash_verde = std::max(0.f, flash_verde - dt);
    tremor = std::max(0.f, tremor - dt);
    aviso_t = std::max(0.f, aviso_t - dt);
    if (f.porta.passavel() && jogador->rect.colide(f.porta.rect)) {
        if (fase_idx == NUM_FASES - 1) {
            estado = ESCOLHA_FINAL;
            som.tocar("fase");
        } else {
            carregar_fase(fase_idx + 1);
            estado = INTRO;
            intro_t = 0; intro_saida = -1;
            som.tocar("fase");
        }
    } else if (!f.porta.aberta && jogador->rect.inflar(10, 10).colide(f.porta.rect) && aviso_t <= 0) {
        avisar(L"Porta selada. Terminais restantes: " + a_wstr(f.meta() - f.acertos()) + L".", 2.0f);
    }
}

// ---------------------------------------------------------------- cinemática
void Jogo::iniciar_cinema(int final) {
    final_escolhido = final;
    estado = CINEMA;
    cinema_t = 0;
    cinema_eventos.clear();
    cinema_bolhas.clear();
    destrocos.clear();
    tremor = 0;
    desafio.reset();
    notas.aberto = false;
    som.tocar("porta", 0.45f);
}

void Jogo::capsula_pos(float& x, float& y) const {
    float y0 = 390;
    float u = std::min(1.0f, cinema_t / 5.0f);
    float k = 1 - (1 - u) * (1 - u) * (1 - u);
    x = 512;
    y = y0 + (250 - y0) * k + std::sin(cinema_t * 2.2f) * 4;
}

void Jogo::atualizar_cinema(float dt) {
    cinema_t += dt;
    float tt = cinema_t;
    bool sobrecarga = final_escolhido == 1;
    Rng& r = rng_global();
    if (sobrecarga && tt >= 4.5f && !cinema_eventos.count("boom")) {
        cinema_eventos.insert("boom");
        som.tocar("explosao", 0.7f);
        tremor = 1.3f;
        float sy = estacao_y();
        destrocos.clear();
        for (int i = 0; i < 26; i++)
            destrocos.push_back({512 + r.entre(-160, 160), sy + r.entre(0, 80), r.entre(-260, 260), r.entre(-220, 40),
                                 (float)r.ent(8, 26), (float)r.ent(5, 14), r.entre(0, 6)});
    }
    if (!sobrecarga && tt >= 6.5f && !cinema_eventos.count("ping")) { cinema_eventos.insert("ping"); som.tocar("bipe", 0.25f); }
    if (tt >= 11.5f && !cinema_eventos.count("sol")) { cinema_eventos.insert("sol"); som.tocar("fase", 0.4f); }
    tremor = std::max(0.f, tremor - dt);
    for (auto& d : destrocos) {
        d.x += d.vx * dt;
        d.y += d.vy * dt + 26 * dt;
        d.vy += 90 * dt;
        d.ang += dt * 3;
    }
    float cx, cy;
    capsula_pos(cx, cy);
    if (r.f() < 0.7f) cinema_bolhas.push_back({cx + r.entre(-7, 7), cy + 34, r.entre(2, 5), 0, 0, 0});
    for (auto& b : cinema_bolhas) { b.y += 95 * dt; b.x += std::sin(tt * 3 + b.vx) * 10 * dt; }
    cinema_bolhas.erase(std::remove_if(cinema_bolhas.begin(), cinema_bolhas.end(), [](const Particula& b) { return b.y >= ALTURA + 10; }),
                        cinema_bolhas.end());
    if (cinema_bolhas.size() > 140) cinema_bolhas.erase(cinema_bolhas.begin(), cinema_bolhas.end() - 140);
}

void Jogo::desenhar_estacao(float sy, int luzes, float brilho) {
    int y = (int)sy;
    for (int x : {360, 512, 664}) ret(x - 9, y + 90, 18, 400, rgb(14, 20, 30));          // pilares até o fundo
    for (int x : {402, 622}) {                                                            // cúpulas
        circ((float)x, (float)y + 8, 44, rgb(30, 40, 54));
        circ_borda((float)x, (float)y + 8, 44, 2, rgb(46, 60, 78));
    }
    Ret corpo(512 - 220, y, 440, 110);
    rret(corpo, 20, rgb(26, 34, 46));
    rret_borda(corpo, 20, 2, rgb(52, 68, 88));
    for (int i = 0; i < 12; i++)                                                           // janelas
        rret(corpo.x + 28 + i * 33, y + 34, 16, 10, 2, i < luzes ? rgb(250, 214, 120) : rgb(16, 22, 30));
    if (brilho > 0) {                                                                      // núcleo do reator
        float r = 16 + 10 * brilho;
        circ(512, (float)y + 74, r, rgb(255, (int)(120 + 100 * brilho), 40));
        circ(512, (float)y + 74, std::max(4.f, r / 2), rgb(255, 240, 200));
    }
}

void Jogo::desenhar_capsula(float cx, float cy) {
    float chama = 10 + 6 * std::fabs(std::sin(cinema_t * 30));
    Pt fogo[3] = {{cx - 8, cy + 30}, {cx + 8, cy + 30}, {cx, cy + 30 + chama}};
    poligono(fogo, 3, rgb(255, 170, 60));
    elipse((int)cx - 18, (int)cy - 32, 36, 64, rgb(210, 220, 232));
    elipse_borda((int)cx - 18, (int)cy - 32, 36, 64, 2, rgb(120, 135, 150));
    ret((int)cx - 18, (int)cy + 4, 36, 6, rgb(250, 150, 30));
    circ(cx, cy - 10, 8, rgb(60, 200, 230));
    circ(cx - 3, cy - 13, 2, rgb(220, 255, 255));
}

// ---------------------------------------------------------------- desenho
void Jogo::desenhar_fundo_menu() {
    for (int y = 0; y < ALTURA; y += 4) {
        float k = (float)y / ALTURA;
        ret(0, y, LARGURA, 4, rgb((int)(4 + 8 * k), (int)(14 + 28 * k), (int)(26 + 40 * k)));
    }
    for (int i = 0; i < 4; i++) {                              // anéis de sonar
        float raio = std::fmod(t * 90 + i * 160, 640.f);
        int alfa = std::max(0, 90 - (int)(raio / 7));
        if (raio > 2) circ_borda(LARGURA / 2, ALTURA + 40, raio, 2, CIANO, alfa);
    }
    for (auto& b : bolhas) circ_borda(b.x, b.y, b.vx, 1, rgb(120, 200, 230));
}

void Jogo::desenhar_menu() {
    desenhar_fundo_menu();
    texto(L"PROTOCOLO CRONOS++", F_GIGANTE, CIANO, LARGURA / 2, 24, true);
    texto(L"O Bunker Submerso", F_GRANDE, BRANCO, LARGURA / 2, 90, true);
    texto(L"Escape Room de Análise Combinatória", F_TEXTO, CINZA, LARGURA / 2, 132, true);
    Ret r(60, 170, LARGURA - 120, 210);
    painel(r);
    paragrafo(TEXTO_HISTORIA, F_TEXTO, BRANCO, r.x + 20, r.y + 14, r.w - 40, 3);
    Ret c(60, 388, LARGURA - 120, 150);
    painel(c, rgb(10, 20, 32), rgb(60, 100, 130));
    texto(L"CONTROLES", F_TITULO, AMARELO, c.x + 20, c.y + 10);
    const wchar_t* linhas[3] = {
        L"WASD / Setas: mover   Shift: correr   E: terminal   N: notas   P / Esc: pausar   M: mudo   - / +: volume   F11: tela cheia",
        L"Meta: 12 acertos nas 5 fases (1 + 2 + 2 + 3 + 4).  Tempo: 12:00.  Limite: 5 falhas críticas.",
        L"Errou? Você mantém seus acertos, perde 20 s e o terminal sorteia OUTRA questão da mesma fase."};
    for (int i = 0; i < 3; i++) texto(linhas[i], F_PEQ, BRANCO, c.x + 20, c.y + 46 + i * 28);
    if (dados.melhor_pontos > 0)
        texto(L"Recorde: " + a_wstr(dados.melhor_pontos) + L" pts (classe " + dados.melhor_letra + L")  ·  Vitórias: " +
                  a_wstr(dados.vitorias) + L"/" + a_wstr(dados.partidas),
              F_PEQ, VERDE, LARGURA / 2, 540, true);
    btn_iniciar.desenhar(F_TITULO);
}

void Jogo::desenhar_intro() {
    ret(0, 0, LARGURA, ALTURA, FUNDO);
    for (auto& b : bolhas) circ_borda(b.x, b.y, b.vx, 1, rgb(50, 100, 130));
    const InfoFase& info = info_fase(fase_idx);
    texto(L"FASE " + a_wstr(fase_idx + 1) + L" DE " + a_wstr(NUM_FASES), F_GRANDE, AMARELO, LARGURA / 2, 120, true);
    texto(maiusc(info.nome), F_GIGANTE, CIANO, LARGURA / 2, 168, true);
    texto(L"Nível " + info.nivel + L"  ·  " + info.conteudo, F_TITULO, BRANCO, LARGURA / 2, 246, true);
    paragrafo(info.historia, F_TEXTO, CINZA, 180, 310, LARGURA - 360, 4, true);
    texto(L"Acertos necessários para abrir a porta: " + a_wstr(info.meta), F_TITULO, VERDE, LARGURA / 2, 440, true);
    int restante = 0;
    for (int i = 0; i < fase_idx; i++) restante += info_fase(i).meta;
    texto(L"Progresso global: " + a_wstr(restante) + L"/" + a_wstr(TOTAL_ACERTOS), F_TEXTO, CINZA, LARGURA / 2, 480, true);
    texto(L"Banco desta fase: " + a_wstr(questoes_da_fase(fase_idx + 1).size()) + L" questões sorteadas sem repetição",
          F_PEQ, CINZA, LARGURA / 2, 516, true);
    if (intro_t > 0.5f)
        texto(toque.ativo ? L"Toque na tela para começar" : L"Clique ou aperte [Enter] para começar", F_TITULO, pisca(t, BRANCO, CINZA), LARGURA / 2, 560, true);
    int a = 0;
    if (intro_saida >= 0) a = (int)(255 * std::min(1.0f, intro_saida / 0.5f));
    else if (intro_t < 0.7f) a = (int)(255 * (1 - intro_t / 0.7f));
    if (a > 0) veu(a);
}

void Jogo::desenhar_hud() {
    ret(0, 0, LARGURA, HUD_H, rgb(8, 16, 26));
    ret(0, HUD_H - 2, LARGURA, 2, CIANO);
    int tt = std::max(0, (int)std::ceil(tempo));
    Cor cor_tempo = (tt <= 60 && (int)(t * 2) % 2 == 0) ? VERMELHO : (tt <= 120 ? AMARELO : BRANCO);
    struct Campo { int x; std::wstring rotulo, valor; Cor cor; };
    Campo campos[5] = {
        {18, L"FASE ATUAL", a_wstr(fase_idx + 1) + L"/" + a_wstr(NUM_FASES) + L"  " + info_fase(fase_idx).nome, BRANCO},
        {340, L"ACERTOS NA FASE", a_wstr(fase->acertos()) + L"/" + a_wstr(fase->meta()), VERDE},
        {500, L"PROGRESSO TOTAL", a_wstr(acertos_total) + L"/" + a_wstr(TOTAL_ACERTOS), CIANO},
        {680, L"TEMPO", formatar_tempo(tt), cor_tempo},
        {800, L"ERROS", a_wstr(erros) + L"/" + a_wstr(MAX_ERROS), erros >= MAX_ERROS - 1 ? VERMELHO : AMARELO}};
    for (auto& c : campos) {
        texto(c.rotulo, F_HUD_L, CINZA, c.x, 8, false, false);
        texto(c.valor, F_HUD_V, c.cor, c.x, 26);
    }
    for (int i = 0; i < MAX_ERROS; i++) circ((float)862 + i * 14, 42, 5, i < erros ? VERMELHO : rgb(50, 30, 30));
}

void Jogo::rotulo(const std::wstring& txt, int cx, int cy, Cor cor, bool pequeno) {
    Fonte f = pequeno ? F_HUD_L : F_PEQ;
    int w = larg_texto(txt, f), h = alt_fonte(f);
    Ret r(cx - w / 2, cy - h / 2, w, h);
    rret(r.inflar(10, 4), 4, PRETO, 170);
    texto(txt, f, cor, r.x, r.y, false, false);
}

void Jogo::desenhar_jogo() {
    Fase& f = *fase;
    ret(0, 0, LARGURA, ALTURA, FUNDO);
    int ox = 0, oy = 0;
    if (tremor > 0) {
        int amp = (int)(9 * tremor);
        ox = rng_global().ent(-amp, amp);
        oy = rng_global().ent(-amp, amp);
    }
    origem(ox, HUD_H + oy);
    blit(f.estatico, 0, 0);
    for (auto& tm : f.terminais) tm.desenhar(t);
    f.porta.desenhar(t);
    for (int i = 0; i < 25; i++)                                       // bolhas/poeira flutuando
        circ(bolhas[i].x, bolhas[i].y - HUD_H, std::max(1.f, bolhas[i].vx / 3), rgb(150, 210, 235));
    jogador->desenhar();
    luz.aplicar(*jogador, f, luz_dx, luz_dy, t);
    // prompts (acima da escuridão para ficarem legíveis)
    Terminal* prox = (!desafio && !notas.aberto) ? terminal_proximo() : nullptr;
    if (prox) {
        rotulo(prox->nome, prox->rect.cx(), prox->rect.y - 46, AMARELO);
        rotulo(toque.ativo ? L"Toque em E para acessar" : L"[E] Acessar Terminal", prox->rect.cx(), prox->rect.y - 26, BRANCO);
    }
    for (auto& tm : f.terminais)
        if (tm.feita) rotulo(L"CONCLUÍDO", tm.rect.cx(), tm.rect.y - 22, VERDE, true);
    if (f.porta.aberta) rotulo(L"PORTA ABERTA >>", f.porta.rect.x - 50, f.porta.rect.y - 22, VERDE, true);
    origem(0, 0);
    if (flash_verm > 0) ret(0, 0, LARGURA, ALTURA, rgb(255, 20, 20), (int)(150 * flash_verm / 0.7f));
    if (flash_verde > 0) ret(0, 0, LARGURA, ALTURA, rgb(30, 255, 120), (int)(110 * flash_verde / 0.6f));
    desenhar_hud();
    if (aviso_t > 0) {
        Ret r(LARGURA / 2 - 320, HUD_H + 10, 640, 40);
        painel(r, rgb(10, 20, 32), AMARELO);
        texto(aviso, F_TEXTO, AMARELO, r.cx(), r.y + 7, true);
    }
    perto_terminal = prox != nullptr;
    if (desafio) desafio->desenhar(t);
    if (notas.aberto) notas.desenhar(t);
    if (pausado) {
        veu(190);
        texto(L"PAUSADO", F_GIGANTE, CIANO, LARGURA / 2, 210, true);
        texto(L"O cronômetro está parado.", F_TEXTO, BRANCO, LARGURA / 2, 290, true);
        texto(L"[Esc / P] Continuar     [Q] Menu     [M] Mudo     [- / +] Volume", F_PEQ, CINZA, LARGURA / 2, 340, true);
        btn_continuar.desenhar(F_TITULO);
        btn_menu.desenhar(F_TITULO);
    }
}

void Jogo::desenhar_botoes_sistema() {
    bool com_pausa = estado == JOGANDO && !pausado;
    std::vector<Ret> botoes = {r_som};
    if (com_pausa) botoes.push_back(r_pausa);
    for (auto& r : botoes) {
        rret(r, 8, rgb(16, 32, 46));
        rret_borda(r, 8, 2, CIANO);
    }
    float cx = (float)r_som.cx() - 5, cy = (float)r_som.cy();
    ret((int)cx - 9, (int)cy - 5, 6, 10, BRANCO);
    Pt cone[4] = {{cx - 3, cy - 5}, {cx + 4, cy - 11}, {cx + 4, cy + 11}, {cx - 3, cy + 5}};
    poligono(cone, 4, BRANCO);
    if (som.mudo) {
        linha(cx + 8, cy - 6, cx + 16, cy + 6, 3, VERMELHO);
        linha(cx + 16, cy - 6, cx + 8, cy + 6, 3, VERMELHO);
    } else {
        if (som.volume > 0.02f) arco(cx + 7, cy, 5, 6, -1.1f, 1.1f, 2, BRANCO);
        if (som.volume > 0.5f) arco(cx + 11, cy, 9, 11, -1.1f, 1.1f, 2, BRANCO);
    }
    if (com_pausa) {
        ret(r_pausa.cx() - 6, r_pausa.cy() - 9, 4, 18, BRANCO);
        ret(r_pausa.cx() + 3, r_pausa.cy() - 9, 4, 18, BRANCO);
    }
    desenhar_volume();
}

void Jogo::desenhar_volume() {
    Ret r;
    if (!r_volume(r)) return;
    bool grande = r.w > 300;
    Fonte f = grande ? F_TEXTO : F_HUD_L;
    float v = som.volume;
    bool ativo = !som.mudo;
    Cor cor = ativo ? CIANO : CINZA;
    int x0 = r.x + 14, x1 = r.dir() - 14, y = r.baixo() - (grande ? 14 : 10);
    std::wstring txt = L"Volume: " + a_wstr((int)std::lround(v * 100)) + L"%" + (ativo ? L"" : L"  (mudo)");
    texto(txt, f, grande ? BRANCO : CINZA, r.cx(), r.y - (grande ? 22 : -2), true, grande);
    float esp = grande ? 8.f : 6.f;
    linha((float)x0, (float)y, (float)x1, (float)y, esp, rgb(40, 56, 72));
    int xk = (int)(x0 + (x1 - x0) * v);
    if (xk > x0) linha((float)x0, (float)y, (float)xk, (float)y, esp, cor);
    float rk = grande ? 14.f : 9.f;
    circ((float)xk, (float)y, rk, BRANCO);
    circ_borda((float)xk, (float)y, rk, 3, cor);
}

void Jogo::desenhar_escolha() {
    desenhar_fundo_menu();
    texto(L"A CÁPSULA ESTÁ PRONTA", F_GIGANTE, VERDE, LARGURA / 2, 40, true);
    paragrafo(L"Todos os protocolos foram quebrados. O reator de Acheron-4 está instável e o organismo "
              L"ainda se agita do outro lado das paredes. Você tem apenas uma decisão a tomar:",
              F_TEXTO, BRANCO, 120, 150, LARGURA - 240, 4, true);
    btn_fugir.desenhar(F_TITULO);
    btn_sobrecarga.desenhar(F_TITULO);
    paragrafo(L"Lançar a cápsula agora e deixar a estação para trás, com o organismo lacrado.", F_PEQ, CINZA,
              btn_fugir.rect.x + 20, btn_fugir.rect.y + 60, 360, 4, true);
    paragrafo(L"Aplicar toda a energia ao núcleo para destruir o organismo, e fugir no último segundo.", F_PEQ, CINZA,
              btn_sobrecarga.rect.x + 20, btn_sobrecarga.rect.y + 60, 360, 4, true);
    texto(L"Escolha com [1] ou [2], ou clique em uma opção", F_PEQ, CINZA, LARGURA / 2, 520, true);
}

void Jogo::desenhar_vitoria() {
    desenhar_fundo_menu();
    for (int k = 0; k < 5; k++) {                               // raios de luz
        float x = 140 + k * 190 + std::sin(t * 0.5f + k) * 25;
        Pt p[4] = {{x - 25, 0}, {x + 25, 0}, {x + 120, ALTURA}, {x + 30, ALTURA}};
        poligono(p, 4, rgb(255, 255, 220), 22);
    }
    for (auto& f : fogos) circ(f.x, f.y, f.vida > 0.5f ? 3.f : 2.f, f.cor);
    const Final& fin = final_cfg(final_escolhido);
    float pulso = 0.5f + 0.5f * std::sin(t * 3);
    texto(L"VITÓRIA!", F_GIGANTE, lerp(VERDE, rgb(200, 255, 220), pulso), LARGURA / 2, 12, true);
    texto(fin.titulo, F_GRANDE, AMARELO, LARGURA / 2, 80, true);
    int y = paragrafo(fin.corpo, F_TEXTO, BRANCO, 70, 126, LARGURA - 140, 0, true);
    paragrafo(fin.epilogo, F_PEQ, rgb(180, 205, 220), 110, y + 4, LARGURA - 220, 0, true);
    Classif cl = classificacao();
    Ret r(80, 300, 540, 190);
    painel(r, rgb(10, 20, 32), VERDE);
    int tt = std::max(0, (int)tempo), gasto = TEMPO_TOTAL - tt;
    std::pair<std::wstring, std::wstring> linhas[5] = {
        {L"Acertos", a_wstr(acertos_total) + L"/" + a_wstr(TOTAL_ACERTOS)},
        {L"Falhas críticas", a_wstr(erros) + L"/" + a_wstr(MAX_ERROS)},
        {L"Tempo restante", formatar_tempo(tt)},
        {L"Tempo usado", formatar_tempo(gasto)},
        {L"Pontuação", a_wstr(cl.pontos)}};
    for (int i = 0; i < 5; i++) {
        int yy = r.y + 12 + i * 34;
        texto(linhas[i].first, F_TITULO, CINZA, r.x + 30, yy);
        texto_dir(linhas[i].second, F_TITULO, i == 4 ? AMARELO : BRANCO, r.dir() - 30, yy);
    }
    Ret rc(644, 300, 300, 190);
    painel(rc, rgb(10, 20, 32), cl.cor);
    texto(L"CLASSIFICAÇÃO", F_HUD_L, CINZA, rc.cx(), rc.y + 14, true);
    float cx = (float)rc.cx(), cy = (float)rc.y + 86;
    circ(cx, cy, 52, rgb(10, 20, 32));
    circ_borda(cx, cy, 52, 4, cl.cor);
    texto(cl.letra, F_GIGANTE, cl.cor, (int)cx, (int)cy - alt_fonte(F_GIGANTE) / 2, true);
    paragrafo(cl.titulo, F_TITULO, BRANCO, rc.x + 10, rc.y + 146, rc.w - 20, 0, true);
    if (novo_recorde) texto(L"NOVO RECORDE!", F_TITULO, pisca(t, AMARELO, BRANCO), LARGURA / 2, 494, true);
    else texto(L"Recorde: " + a_wstr(dados.melhor_pontos) + L" pts", F_PEQ, CINZA, LARGURA / 2, 498, true);
    btn_novo.desenhar(F_TITULO);
    texto(L"Protocolo Cronos++ · Projeto 2 - Escape Room de Análise Combinatória · Obrigado por jogar!", F_PEQ, CINZA, LARGURA / 2, 604, true);
}

void Jogo::desenhar_fim() {
    if (estado == VITORIA) { desenhar_vitoria(); return; }
    desenhar_fundo_menu();
    texto(L"DERROTA", F_GIGANTE, VERMELHO, LARGURA / 2, 36, true);
    texto(L"Protocolo Cronos falhou", F_GRANDE, AMARELO, LARGURA / 2, 106, true);
    paragrafo(motivo_derrota, F_TEXTO, BRANCO, 130, 170, LARGURA - 260, 4, true);
    Ret r(LARGURA / 2 - 260, 330, 520, 170);
    painel(r, rgb(10, 20, 32), VERMELHO);
    int tt = std::max(0, (int)tempo), gasto = TEMPO_TOTAL - tt;
    std::wstring linhas[4] = {
        L"Acertos: " + a_wstr(acertos_total) + L"/" + a_wstr(TOTAL_ACERTOS),
        L"Fase alcançada: " + a_wstr(fase_idx + 1) + L"/" + a_wstr(NUM_FASES),
        L"Erros: " + a_wstr(erros) + L"/" + a_wstr(MAX_ERROS),
        L"Tempo restante: " + formatar_tempo(tt) + L"   (usado: " + formatar_tempo(gasto) + L")"};
    for (int i = 0; i < 4; i++) texto(linhas[i], F_TITULO, BRANCO, r.x + 30, r.y + 18 + i * 36);
    btn_novo.desenhar(F_TITULO);
}

void Jogo::desenhar_cinema() {
    float tt = cinema_t;
    bool sobrecarga = final_escolhido == 1;
    float sup = limitar((tt - 9.0f) / 6.0f, 0.f, 1.f);
    int ox = 0, oy = 0;
    if (tremor > 0) {
        int amp = (int)(14 * tremor);
        ox = rng_global().ent(-amp, amp);
        oy = rng_global().ent(-amp, amp);
    }
    ret(0, 0, LARGURA, ALTURA, PRETO);
    origem(ox, oy);
    Cor topo = lerp(rgb(6, 16, 32), rgb(70, 160, 210), sup), fundo = lerp(rgb(1, 4, 10), rgb(16, 60, 110), sup);
    for (int i = 0; i < ALTURA; i += 20) ret(0, i, LARGURA, 20, lerp(topo, fundo, (float)i / ALTURA));
    if (sup > 0)                                                  // raios de sol
        for (int k = 0; k < 6; k++) {
            float x = 120 + k * 170 + std::sin(tt * 0.6f + k) * 30;
            Pt p[4] = {{x - 30, 0}, {x + 30, 0}, {x + 140, ALTURA}, {x + 40, ALTURA}};
            poligono(p, 4, rgb(255, 255, 230), (int)(38 * sup));
        }
    float sy = estacao_y();
    bool explodiu = sobrecarga && tt >= 4.5f;
    if (!explodiu && sy < ALTURA + 120) {
        int luzes = sobrecarga ? 12 : std::max(0, 12 - (int)(std::max(0.f, tt - 3.0f) * 2.2f));
        float brilho = sobrecarga ? std::min(1.0f, tt / 4.5f) * (0.6f + 0.4f * std::fabs(std::sin(tt * (4 + 6 * tt / 4.5f)))) : 0.f;
        desenhar_estacao(sy, luzes, brilho);
    }
    if (!sobrecarga && tt >= 6.0f && tt <= 12.0f) {               // olhos do organismo no escuro
        float a = std::sin((tt - 6.0f) / 6.0f * PI_);
        if ((int)(tt * 3) % 7 != 0) {
            const int olhos[4][2] = {{150, 470}, {178, 466}, {860, 500}, {884, 504}};
            for (auto& o : olhos) {
                circ((float)o[0], (float)o[1], 12, rgb(255, 40, 40), (int)(90 * a));
                circ((float)o[0], (float)o[1], 4, rgb(255, 120, 90), (int)(230 * a));
            }
        }
    }
    for (auto& b : cinema_bolhas) circ_borda(b.x, b.y, b.vx, 1, rgb(170, 220, 240));
    if (explodiu) {
        float te = tt - 4.5f;
        for (auto& d : destrocos) {
            float c = std::cos(d.ang), s = std::sin(d.ang), hw = d.w / 2, hh = d.h / 2, mx = d.x + hw, my = d.y + hh;
            Pt p[4];
            const float q[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
            for (int i = 0; i < 4; i++) p[i] = {mx + q[i][0] * hw * c - q[i][1] * hh * s, my + q[i][0] * hw * s + q[i][1] * hh * c};
            poligono(p, 4, rgb(60, 66, 78));
        }
        if (te < 3.0f) {
            const struct { float k; Cor cor; } ondas[2] = {{1.0f, rgb(255, 220, 160)}, {0.7f, rgb(255, 140, 60)}};
            for (auto& o : ondas) {
                float raio = te * 620 * o.k;
                if (raio > 4) circ_borda(512, sy + 60, raio, 10, o.cor, (int)(200 * std::max(0.f, 1 - te / 3)));
            }
            float br = std::max(0.f, 1 - te / 2.0f);
            if (br > 0) circ(512, sy + 60, 60 + 220 * te, rgb(255, 190, 90), (int)(160 * br));
        }
        if (te < 0.9f) ret(0, 0, LARGURA, ALTURA, rgb(255, 255, 255), (int)(255 * (1 - te / 0.9f)));
    }
    float cx, cy;
    capsula_pos(cx, cy);
    desenhar_capsula(cx, cy);
    origem(0, 0);
    // legenda com efeito de máquina de escrever
    const Final& fin = final_cfg(final_escolhido);
    int idx = std::min((int)fin.legendas.size() - 1, (int)(tt / 3.9f));
    float local = tt - idx * 3.9f;
    size_t n = (size_t)std::max(0.f, local * 48);
    std::wstring txt = fin.legendas[idx].substr(0, n);
    Ret r(90, 56, LARGURA - 180, 84);
    painel(r, rgb(10, 20, 32), sobrecarga ? AMARELO : CIANO, 200);
    paragrafo(txt, F_TEXTO, BRANCO, r.x + 24, r.y + 12, r.w - 48, 3);
    texto(fin.titulo, F_TITULO, AMARELO, LARGURA / 2, 14, true);
    if (tt >= CINEMA_TOTAL) texto(toque.ativo ? L"Toque na tela para ver o resultado" : L"Clique ou aperte [Enter] para ver o resultado", F_TITULO, pisca(t, BRANCO, AMARELO), LARGURA / 2, ALTURA - 52, true);
    else if (tt > 1.0f) texto(L"[Enter] ou clique para pular", F_PEQ, CINZA, LARGURA / 2, ALTURA - 30, true);
}

void Jogo::desenhar() {
    alvo_tela();
    origem(0, 0);
    if (base_x() > 0) {                      // tela mais larga que 16:10: faixas laterais escuras com divisórias
        limpar(rgb(4, 9, 16));
        ret(-2, 0, 2, ALTURA, rgb(30, 70, 90));
        ret(LARGURA, 0, 2, ALTURA, rgb(30, 70, 90));
    }
    switch (estado) {
    case MENU: desenhar_menu(); break;
    case INTRO: desenhar_intro(); break;
    case JOGANDO: desenhar_jogo(); break;
    case ESCOLHA_FINAL: desenhar_escolha(); break;
    case CINEMA: desenhar_cinema(); break;
    default: desenhar_fim(); break;
    }
    desenhar_botoes_sistema();
    if (toque.ativo && estado == JOGANDO && !(desafio || notas.aberto || pausado)) toque.desenhar(perto_terminal);
    if (debug_toque) {
        circ_borda((float)dbg_x, (float)dbg_y, 22, 3, dbg_vivo ? rgb(255, 60, 60) : rgb(255, 255, 0));
        texto(a_wstr(dbg_x) + L"," + a_wstr(dbg_y) + L"  faixa=" + a_wstr(base_x()) + L" W=" + a_wstr(largura_tela()), F_PEQ,
              rgb(255, 255, 0), -base_x() + 6, ALTURA - 24);
    }
}
