// Protocolo Cronos++ - testes automáticos (sem abrir janela nem tocar áudio)
#include <cstdio>
#include <queue>
#include "../src/audio.h"
#include "../src/calc.h"
#include "../src/jogo.h"

static int falhas = 0, total = 0;
#define CHECK(cond, msg) do { total++; if (!(cond)) { falhas++; std::printf("FALHOU: %s  (linha %d)\n", msg, __LINE__); } } while (0)

static std::wstring calc(const wchar_t* e) { std::wstring r; return avaliar_expressao(e, r) ? r : L"<erro>"; }

static void tecla(Jogo& j, int k) { Evento e; e.tipo = Ev::Tecla; e.tecla = k; j.tratar_evento(e); }
static void digitar(Jogo& j, const std::wstring& s) {
    for (wchar_t c : s) { Evento e; e.tipo = Ev::Char; e.ch = c; j.tratar_evento(e); }
}
static void passos(Jogo& j, int n, float dt = 1.0f / 60) { for (int i = 0; i < n; i++) j.atualizar(dt); }
static void passar_intro(Jogo& j) {
    passos(j, 60);
    tecla(j, K_RETURN);
    passos(j, 40);
}

static void testar_banco() {
    const int esperado[6] = {0, 10, 20, 20, 20, 20};
    std::set<std::wstring> ids;
    for (int f = 1; f <= 5; f++) {
        auto qs = questoes_da_fase(f);
        CHECK((int)qs.size() == esperado[f], "tamanho do banco por fase");
        // regra pedida: fases com meta >= 2 acertos tem 20 questoes
        if (info_fase(f - 1).meta >= 2) CHECK(qs.size() >= 20, "fase com meta >= 2 tem 20 questoes");
        for (auto& q : qs) {
            CHECK(ids.insert(q.id).second, "id unico");
            CHECK(!q.resposta.empty() && q.resposta.find_first_not_of(L"0123456789") == std::wstring::npos, "resposta numerica");
            CHECK(!q.enunciado.empty() && !q.resolucao.empty(), "texto presente");
        }
    }
    int soma = 0;
    for (int i = 0; i < NUM_FASES; i++) soma += info_fase(i).meta;
    CHECK(soma == TOTAL_ACERTOS, "metas somam 12");
}

static void testar_calc() {
    CHECK(calc(L"8!/(3!*5!)") == L"56", "combinacao 8!/(3!5!)");
    CHECK(calc(L"2^10") == L"1024", "potencia");
    CHECK(calc(L"(3+4)*2") == L"14", "parenteses");
    CHECK(calc(L"5!") == L"120", "fatorial");
    CHECK(calc(L"3x4") == L"12", "x como multiplicacao");
    CHECK(calc(L"10/4") == L"2.5", "decimal");
    CHECK(calc(L"1/0") == L"<erro>", "divisao por zero");
    CHECK(calc(L"2+") == L"<erro>", "expressao incompleta");
    CHECK(calc(L"(2+3") == L"<erro>", "parentese aberto");
    CHECK(calc(L"61!") == L"<erro>", "fatorial grande demais");
    CHECK(calc(L"-3+5") == L"2", "unario");
    CHECK(calc(L"10!/(4!*6!)") == L"210", "C(10,4)");
}

static void testar_mapas() {
    for (int i = 0; i < NUM_FASES; i++) {
        Fase f(i);
        bool bloq[ROWS][COLS];
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                Ret tile(c * TILE, r * TILE, TILE, TILE);
                bloq[r][c] = false;
                for (auto& s : f.solidos) if (s.colide(tile)) bloq[r][c] = true;
            }
        bool vis[ROWS][COLS] = {};
        std::queue<std::pair<int, int>> q;
        int sc = f.inicio_x / TILE, sr = f.inicio_y / TILE;
        vis[sr][sc] = true;
        q.push({sr, sc});
        while (!q.empty()) {
            auto p = q.front(); q.pop();
            const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (auto& dd : d) {
                int nr = p.first + dd[0], nc = p.second + dd[1];
                if (nr < 0 || nc < 0 || nr >= ROWS || nc >= COLS || vis[nr][nc] || bloq[nr][nc]) continue;
                vis[nr][nc] = true;
                q.push({nr, nc});
            }
        }
        for (auto& t : f.terminais) {
            bool ok = false;
            Ret zona = t.rect.inflar(34, 34);
            for (int r = 0; r < ROWS && !ok; r++)
                for (int c = 0; c < COLS && !ok; c++)
                    if (vis[r][c] && zona.colide(Ret(c * TILE, r * TILE, TILE, TILE))) ok = true;
            CHECK(ok, "terminal alcancavel");
        }
        int pr = f.porta.rect.y / TILE, pc = f.porta.rect.x / TILE - 1;
        CHECK(vis[pr][pc] || vis[pr + 1][pc], "porta alcancavel");
        CHECK(f.terminais.size() >= (size_t)f.meta(), "terminais suficientes para a meta");
    }
}

static void colocar_perto(Jogo& j, const Terminal& t) {
    j.jogador->rect.x = t.rect.x;
    j.jogador->rect.y = t.rect.baixo() + 2;
    j.jogador->px = (float)(j.jogador->rect.x + 11);
    j.jogador->py = (float)(j.jogador->rect.y + 11);
}

static void responder(Jogo& j, bool certo) {
    std::wstring r = j.desafio->questao()->resposta;
    digitar(j, certo ? r : r + L"7");
    tecla(j, K_RETURN);
}

static void testar_fluxo() {
    Audio som;
    som.iniciar(false);
    Jogo j(som, nullptr);
    CHECK(j.estado == MENU, "comeca no menu");
    tecla(j, K_RETURN);
    CHECK(j.estado == INTRO, "menu -> intro");
    passar_intro(j);
    CHECK(j.estado == JOGANDO, "intro -> jogando");
    // erro: +1 falha, -20 s, nova questao
    colocar_perto(j, j.fase->terminais[0]);
    tecla(j, 'E');
    CHECK(j.desafio != nullptr, "E abre o terminal");
    const Questao* q0 = j.desafio->questao();
    float t0 = j.tempo;
    responder(j, false);
    CHECK(j.erros == 1 && j.tempo <= t0 - PENALIDADE_ERRO + 0.01f, "erro desconta tempo e conta falha");
    CHECK(j.desafio->questao() != q0, "nova questao apos erro");
    tecla(j, K_RETURN);
    CHECK(j.desafio && !j.desafio->feedback, "volta a pergunta apos erro");
    tecla(j, K_ESC);
    CHECK(!j.desafio, "Esc sai do terminal");
    // percorre as 5 fases acertando tudo
    for (int f = 0; f < NUM_FASES; f++) {
        CHECK(j.fase_idx == f, "fase esperada");
        int meta = j.fase->meta();
        int feitos = 0;
        for (size_t k = 0; k < j.fase->terminais.size() && feitos < meta; k++) {
            colocar_perto(j, j.fase->terminais[k]);
            tecla(j, 'E');
            CHECK(j.desafio != nullptr, "abre terminal");
            if (!j.desafio) break;
            responder(j, true);
            tecla(j, K_RETURN);
            feitos++;
        }
        CHECK(j.fase->porta.aberta, "porta abre ao cumprir a meta");
        passos(j, 90);
        j.jogador->rect.x = j.fase->porta.rect.x;
        j.jogador->rect.y = j.fase->porta.rect.y;
        j.jogador->px = (float)(j.jogador->rect.x + 11);
        j.jogador->py = (float)(j.jogador->rect.y + 11);
        passos(j, 2);
        if (f < NUM_FASES - 1) { CHECK(j.estado == INTRO, "passa para a proxima fase"); passar_intro(j); }
    }
    CHECK(j.estado == ESCOLHA_FINAL, "escolha final");
    CHECK(j.acertos_total == TOTAL_ACERTOS, "12 acertos no total");
    tecla(j, '2');
    CHECK(j.estado == CINEMA, "cinematica");
    passos(j, 60 * 6);
    tecla(j, K_RETURN);
    CHECK(j.estado == VITORIA, "vitoria");
    CHECK(j.classificacao().pontos > 0, "pontuacao positiva");
    tecla(j, K_RETURN);
    CHECK(j.estado == INTRO && j.erros == 0 && j.acertos_total == 0, "nova partida reinicia");
}

static void testar_derrota() {
    Audio som;
    som.iniciar(false);
    Jogo j(som, nullptr);
    tecla(j, K_RETURN);
    passar_intro(j);
    for (int i = 0; i < MAX_ERROS; i++) {
        colocar_perto(j, j.fase->terminais[0]);
        tecla(j, 'E');
        responder(j, false);
        tecla(j, K_RETURN);
    }
    CHECK(j.estado == DERROTA, "5 erros = derrota");
    Jogo k(som, nullptr);
    tecla(k, K_RETURN);
    passar_intro(k);
    k.tempo = 0.5f;
    passos(k, 60);
    CHECK(k.estado == DERROTA, "tempo esgotado = derrota");
    // sorteio sem repeticao na fase de 20 questoes
    Baralho b(3);
    std::set<std::wstring> vistos;
    for (int i = 0; i < 20; i++) vistos.insert(b.sortear({})->id);
    CHECK(vistos.size() == 20, "baralho sem repeticao");
}


// Toque (celular): abre o terminal, as notas, calcula e usa o resultado na resposta, tudo por toques.
static void toque(Jogo& j, int x, int y, int dedo = 5) {
    Evento d; d.tipo = Ev::MouseDown; d.x = x; d.y = y; d.dedo = dedo; d.toque = true;
    j.tratar_evento(d);
    j.atualizar(1.0f / 60);
    Evento u = d; u.tipo = Ev::MouseUp;
    j.tratar_evento(u);
    j.atualizar(1.0f / 60);
}
static void toque_tecla(Jogo& j, const wchar_t* valor) {
    for (auto& k : j.notas.teclado.teclas)
        if (k.valor == valor) { toque(j, k.rect.cx(), k.rect.cy()); return; }
    CHECK(false, "tecla do bloco de notas existe");
}

static void testar_toque_notas() {
    Audio som;
    som.iniciar(false);
    Jogo j(som, nullptr);
    tecla(j, K_RETURN);
    passar_intro(j);
    colocar_perto(j, j.fase->terminais[0]);
    toque(j, 1, 1);                       // primeiro toque liga os controles de toque
    tecla(j, 'E');
    CHECK(j.desafio != nullptr, "terminal aberto");
    toque(j, 250, 500);                   // botao "Notas [N]" do terminal
    CHECK(j.notas.aberto, "toque no botao Notas abre o bloco de notas");
    toque_tecla(j, L"8"); toque_tecla(j, L"*"); toque_tecla(j, L"7");
    toque_tecla(j, L"CALCULAR");
    CHECK(j.notas.resultado_inteiro() == L"56", "calculo por toque");
    j.atualizar(1.0f / 60);
    CHECK(j.notas.pode_responder, "notas podem responder");
    toque_tecla(j, L"RESPOSTA");
    CHECK(j.desafio && j.desafio->entrada == L"56", "Usar na resposta coloca 56 no terminal");
    CHECK(!j.notas.aberto, "notas fecham depois de usar a resposta");
}

int main() {
    gfx::iniciar("assets");
    testar_banco();
    testar_calc();
    testar_mapas();
    testar_fluxo();
    testar_derrota();
    testar_toque_notas();
    std::printf("%d verificacoes, %d falhas\n", total, falhas);
    return falhas ? 1 : 0;
}
