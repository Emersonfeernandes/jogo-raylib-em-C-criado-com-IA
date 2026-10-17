#include "input.h"
#include <math.h>

static Vector2 NormalizeIfNeeded(Vector2 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y);
    if (len > 0.0001f) { v.x /= len; v.y /= len; }
    return v;
}

InputState InputRead(void) {
    InputState in = {0};

    // Movimento (WASD / setas)
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  in.move.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) in.move.x += 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    in.move.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  in.move.y += 1.0f;

    in.move = NormalizeIfNeeded(in.move);

    // Inputs de estados
    in.startPressed   = IsKeyPressed(KEY_ENTER);
    in.pausePressed   = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
    in.restartPressed = IsKeyPressed(KEY_R);
    in.dashPressed    = IsKeyPressed(KEY_SPACE);
    in.quitPressed    = IsKeyPressed(KEY_Q);

    return in;
}