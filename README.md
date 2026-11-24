# Protocolo Cronos++: O Bunker Submerso

Reescrita em **C++** do jogo *Protocolo Cronos* (Escape Room 2D de Análise Combinatória). O projeto Python original
(`../protocolo-cronos`) continua intacto. Gráficos e sons são 100% procedurais; só as fontes (Noto Sans, licença OFL) ficam
em `assets/fonts`.

## Arquitetura portátil

```
núcleo C++ (regras, questões, calculadora, cenário, renderizador por software, síntese de áudio)
        └── src/plataforma_sdl.cpp  (SDL2: janela, teclado, mouse, toque, áudio)
              ├── Windows / Linux / macOS (desktop)
              └── Emscripten → WebAssembly (navegador do PC e do celular)
```

O núcleo só escreve num framebuffer 1024×640 e gera amostras de áudio, sem nenhuma API de sistema. Isso permite
novas plataformas (Android, ESP32) trocando apenas a camada de plataforma.

## Novidades em relação à versão Python

- **Banco maior:** toda fase com 2 ou mais acertos tem **20 questões** (fases 2 a 5); a fase 1 mantém 10. Total: **90**.
  As 40 novas estão em `tools/questoes_extras.py`, cada resposta conferida por força bruta.
- **Celular:** joystick virtual, botões E/N e teclado na tela; no navegador entra em tela cheia no primeiro toque.
- Renderizador com bordas suavizadas, iluminação suave, lanterna que acompanha o personagem com suavização.
- Janela redimensionável (F11 = tela cheia), **Shift** corre, pausa ao perder o foco.
- **Recordes, volume e mudo salvos** (arquivo no desktop, `localStorage` no navegador).
- **Testes automáticos** (`teste.exe`): banco, calculadora, mapas, fluxo completo, derrotas, sorteio sem repetição.

## Regras (iguais às do original)

| Fase | Sala | Nível | Conteúdo | Acertos | Banco |
|------|------|-------|----------|:------:|:-----:|
| 1 | Câmara de Eclusa | Fácil | Princípio Fundamental da Contagem | 1 | 10 |
| 2 | Laboratório Genético | Básico | Permutação Simples e com Repetição | 2 | 20 |
| 3 | Central de Sonar | Intermediário | Arranjo Simples | 2 | 20 |
| 4 | Câmara do Reator | Avançado | Combinação Simples | 3 | 20 |
| 5 | Sala da Cápsula | Difícil | Restrições e Integração | 4 | 20 |

12 acertos para vencer, 12 minutos, 5 falhas críticas; cada erro custa 20 s e sorteia outra questão inédita do mesmo nível.

## Controles

`WASD`/setas mover · `Shift` correr · `E` terminal · `N` bloco de notas (calculadora com `! ^ ( )`) · `Enter` confirmar ·
`Esc`/`P` pausar · `M` mudo · `-`/`+` volume · `F11` tela cheia · `T` liga/desliga os controles de toque.
No celular: joystick à esquerda, botões **E** (acessar) e **N** (notas) à direita.

## Compilar

**Windows com MinGW** (SDL2 já vem em `third_party/`):

```bat
build.bat
```

Gera `CronosPP.exe` (precisa de `SDL2.dll` e da pasta `assets` ao lado) e `teste.exe`, e roda os testes.

**Web (WebAssembly):** instale o [emsdk](https://github.com/emscripten-core/emsdk) ao lado do projeto e rode

```bat
build_web.bat
```

Serve `build-web/` com qualquer servidor estático (`py -3.11 -m http.server -d build-web`).

**Linux / macOS / Windows via CMake:** `cmake -S . -B build && cmake --build build && ctest --test-dir build`
(precisa de SDL2). O workflow `.github/workflows/build.yml` faz isso nos três sistemas, compila a versão web e publica
no GitHub Pages; esse workflow ainda não foi executado.

## Estrutura

```
src/gfx.*        renderizador por software + texto (stb_truetype)   src/audio.*   síntese procedural + mixer
src/calc.*       calculadora segura do bloco de notas               src/dados.*   fases, mapas, textos, questões
src/mundo.*      cenário, terminais, porta, jogador, luz            src/jogo.*    estados, regras, interface, toque
src/plataforma_sdl.cpp  janela/entrada/áudio via SDL2               src/questoes_dados.inc  gerado por tools/
tests/           testes automáticos e geração de capturas           web/shell.html  página do jogo no navegador
third_party/     SDL2 (Windows i686) e stb_truetype                 assets/fonts   Noto Sans (OFL)
```
