#ifndef INPUT_H
#define INPUT_H

#include "raylib.h"
#include <stdbool.h>

typedef struct InputState {
    Vector2 move;            // direção normalizada (-1..1)
    bool startPressed;       // ENTER
    bool pausePressed;       // P ou ESC
    bool restartPressed;     // R
    bool dashPressed;        // SPACE (só pra exemplo)
    bool quitPressed;        // Q (menu/gameover)
} InputState;

InputState InputRead(void);

#endif