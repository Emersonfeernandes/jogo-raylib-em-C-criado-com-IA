#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>

typedef enum ScreenState {
    SCREEN_MENU = 0,
    SCREEN_GAMEPLAY,
    SCREEN_PAUSE,
    SCREEN_GAMEOVER
} ScreenState;

typedef struct SpriteAnim {
    Texture2D tex;     // spritesheet
    int frameW;        // largura de cada frame
    int frameH;        // altura de cada frame
    int frames;        // quantos frames na linha (1 linha)
    int frame;         // frame atual
    float fps;         // frames por segundo
    float acc;         // acumulador de tempo
} SpriteAnim;

typedef struct Entity {
    Vector2 pos;       // posição (top-left)
    Vector2 size;      // tamanho (w,h) — AABB
    Vector2 vel;       // velocidade
    float speed;       // velocidade base
    SpriteAnim anim;   // animação do sprite
} Entity;

typedef struct Game {
    int screenW, screenH;

    ScreenState state;

    // Tilemap
    int tileSize;
    int mapW, mapH;

    // Entidades
    Entity player;
    Entity enemy;

    // Moeda simples
    Vector2 coinPos;
    Vector2 coinSize;
    bool coinActive;

    int score;
    int hp;

    // Texturas de tiles (pode trocar por sprites reais depois)
    Texture2D tileWall;
    Texture2D tileFloor;
} Game;

void GameInit(Game *g, int screenW, int screenH);
void GameUpdate(Game *g, float dt);
void GameDraw(const Game *g);
void GameShutdown(Game *g);

#endif