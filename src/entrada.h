// Protocolo Cronos++ - eventos e estado de entrada (independente da plataforma)
#pragma once

// Códigos de tecla = códigos virtuais do Windows. Letras e dígitos: ASCII maiúsculo ('A'..'Z', '0'..'9').
enum Tecla {
    K_BACK = 8, K_RETURN = 13, K_SHIFT = 16, K_ESC = 27, K_SPACE = 32,
    K_LEFT = 37, K_UP = 38, K_RIGHT = 39, K_DOWN = 40,
    K_KP_MAIS = 0x6B, K_KP_MENOS = 0x6D, K_F11 = 0x7A, K_MAIS = 0xBB, K_MENOS = 0xBD
};

enum class Ev { Tecla, Char, MouseDown, MouseUp, MouseMove, Sair };

struct Evento {
    Ev tipo = Ev::Sair;
    int tecla = 0;        // Ev::Tecla
    wchar_t ch = 0;       // Ev::Char
    int x = 0, y = 0;     // coordenadas do jogo (1024x640)
    int botao = 1;
    int dedo = -1;        // -1 = mouse; >= 0 = identificador do dedo (toque)
    bool toque = false;   // veio de uma tela sensível ao toque
};

struct Entrada {
    bool tec[256] = {};   // teclas pressionadas agora
    float joy_x = 0, joy_y = 0;   // direcional analógico do controle (-1 a 1)
    int mx = 0, my = 0;   // posição do mouse em coordenadas do jogo
};
extern Entrada entrada;
