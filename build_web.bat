@echo off
rem Compila a versao web (WebAssembly) com o Emscripten. Uso: build_web.bat [caminho do emsdk]
rem (o Python do PATH pode ser o atalho da Microsoft Store; por isso usamos o Python/Node que vem dentro do emsdk)
setlocal
set EMROOT=%~1
if "%EMROOT%"=="" set EMROOT=%~dp0..\emsdk
for /d %%d in ("%EMROOT%\python\*") do set PATH=%%d;%PATH%
for /d %%d in ("%EMROOT%\node\*") do set PATH=%%d\bin;%%d;%PATH%
set PATH=%EMROOT%\upstream\emscripten;%PATH%
if not exist build-web mkdir build-web
call em++.exe -std=gnu++14 -O2 -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web ^
  src/gfx.cpp src/audio.cpp src/calc.cpp src/dados.cpp src/mundo.cpp src/cinema.cpp src/jogo.cpp src/plataforma_sdl.cpp ^
  --preload-file assets@/assets --shell-file web/shell.html -o build-web/index.html || exit /b 1
echo Pronto: build-web\index.html
