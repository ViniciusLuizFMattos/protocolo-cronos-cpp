@echo off
rem Compila o Protocolo Cronos++ para desktop (MinGW + SDL2 em third_party) e roda os testes automaticos.
setlocal
if not exist build mkdir build
set SDL=third_party\SDL2\i686-w64-mingw32
set FLAGS=-std=gnu++14 -O2 -Wall -Wextra -I%SDL%\include\SDL2
set OBJS=build\gfx.o build\audio.o build\calc.o build\dados.o build\mundo.o build\jogo.o
for %%f in (gfx audio calc dados mundo jogo plataforma_sdl) do (
  g++ %FLAGS% -c src\%%f.cpp -o build\%%f.o || exit /b 1
)
g++ %OBJS% build\plataforma_sdl.o -o CronosPP.exe -L%SDL%\lib -lSDL2 -mwindows || exit /b 1
g++ %FLAGS% tests\teste.cpp %OBJS% -o teste.exe -L%SDL%\lib -lSDL2 || exit /b 1
copy /y %SDL%\bin\SDL2.dll . >nul
.\teste.exe || exit /b 1
echo.
echo Pronto: CronosPP.exe
