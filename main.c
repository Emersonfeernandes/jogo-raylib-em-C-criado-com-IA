#include "raylib.h"
#include "game.h"

int main(void) {
    const int screenW = 960;
    const int screenH = 540;

    InitWindow(screenW, screenH, "Raylib Prototype - States + Sprites + Tilemap");
    SetTargetFPS(60);

    Game g = {0};
    GameInit(&g, screenW, screenH);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        GameUpdate(&g, dt);

        BeginDrawing();
        GameDraw(&g);
        EndDrawing();
    }

    GameShutdown(&g);
    CloseWindow();
    return 0;
}