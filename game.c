#include "game.h"
#include "input.h"
#include <math.h>
#include <string.h>

// =======================
//  TILEMAP (simples)
//  0 = chão, 1 = parede
// =======================
enum {
    MAP_W = 20,
    MAP_H = 12
};

// Mapa “handmade”: bordas sólidas + alguns blocos
static const int MAP_DATA[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1,1,1,0,0,0,1,0,0,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,1,0,1,0,1,0,1,0,0,0,1},
    {1,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,1},
    {1,0,0,1,0,0,0,1,0,0,0,1,1,1,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,0,0,0,0,1,1,1,0,1,0,1,1,1,0,1},
    {1,0,0,0,1,0,1,1,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// =======================
// Helpers básicos
// =======================
static float ClampF(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }

static Rectangle EntRect(const Entity *e) {
    return (Rectangle){ e->pos.x, e->pos.y, e->size.x, e->size.y };
}

static bool RectIntersects(Rectangle a, Rectangle b) {
    return CheckCollisionRecs(a, b);
}

static Rectangle TileRect(int tileSize, int x, int y) {
    return (Rectangle){ (float)(x * tileSize), (float)(y * tileSize), (float)tileSize, (float)tileSize };
}

static bool TileIsWall(int x, int y) {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return true; // fora do mapa = sólido
    return MAP_DATA[y][x] == 1;
}

// =======================
// Spritesheet “fake” (runtime)
// - Pra não depender de arquivos PNG agora.
// - Depois você troca por LoadTexture("assets/player.png").
// =======================
static SpriteAnim MakeAnim(Texture2D tex, int frameW, int frameH, int frames, float fps) {
    SpriteAnim a = {0};
    a.tex = tex;
    a.frameW = frameW;
    a.frameH = frameH;
    a.frames = frames;
    a.fps = fps;
    a.frame = 0;
    a.acc = 0.0f;
    return a;
}

static void AnimUpdate(SpriteAnim *a, float dt) {
    // Animação simples: avança frame baseado em fps
    a->acc += dt;
    float spf = (a->fps <= 0.0f) ? 9999.0f : (1.0f / a->fps);
    while (a->acc >= spf) {
        a->acc -= spf;
        a->frame = (a->frame + 1) % a->frames;
    }
}

static void DrawAnim(const SpriteAnim *a, Vector2 pos, Vector2 size) {
    // Recorte do frame atual no spritesheet
    Rectangle src = { (float)(a->frame * a->frameW), 0.0f, (float)a->frameW, (float)a->frameH };
    Rectangle dst = { pos.x, pos.y, size.x, size.y };
    DrawTexturePro(a->tex, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

// =======================
// Colisão com paredes (tilemap)
// Estratégia: mover em X e resolver colisões; depois mover em Y e resolver.
// Isso evita “atolar” e é simples/robusto.
// =======================
static void ResolveCollisionsAxis(Entity *e, int tileSize, bool moveX) {
    Rectangle r = EntRect(e);

    // Descobre o range de tiles potencialmente tocados pela entidade
    int left   = (int)floorf(r.x / tileSize) - 1;
    int right  = (int)floorf((r.x + r.width) / tileSize) + 1;
    int top    = (int)floorf(r.y / tileSize) - 1;
    int bottom = (int)floorf((r.y + r.height) / tileSize) + 1;

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (!TileIsWall(tx, ty)) continue;

            Rectangle t = TileRect(tileSize, tx, ty);

            if (RectIntersects(r, t)) {
                // Empurra para fora dependendo do eixo do movimento
                if (moveX) {
                    if (e->vel.x > 0) {
                        // indo pra direita: encosta na esquerda do tile
                        e->pos.x = t.x - r.width;
                    } else if (e->vel.x < 0) {
                        // indo pra esquerda: encosta na direita do tile
                        e->pos.x = t.x + t.width;
                    }
                    e->vel.x = 0;
                } else {
                    if (e->vel.y > 0) {
                        // indo pra baixo: encosta no topo do tile
                        e->pos.y = t.y - r.height;
                    } else if (e->vel.y < 0) {
                        // indo pra cima: encosta na base do tile
                        e->pos.y = t.y + t.height;
                    }
                    e->vel.y = 0;
                }

                r = EntRect(e); // atualiza retângulo após ajuste
            }
        }
    }
}

static void MoveWithTileCollisions(Entity *e, float dt, int tileSize) {
    // Move em X
    e->pos.x += e->vel.x * dt;
    ResolveCollisionsAxis(e, tileSize, true);

    // Move em Y
    e->pos.y += e->vel.y * dt;
    ResolveCollisionsAxis(e, tileSize, false);
}

// =======================
// Gameplay helpers
// =======================
static Vector2 NormalizeSafe(Vector2 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y);
    if (len > 0.0001f) { v.x /= len; v.y /= len; }
    return v;
}

static void SpawnCoin(Game *g) {
    // Coloca a moeda num tile de chão aleatório (bem simples)
    for (int tries = 0; tries < 200; tries++) {
        int x = GetRandomValue(1, g->mapW - 2);
        int y = GetRandomValue(1, g->mapH - 2);
        if (!TileIsWall(x, y)) {
            g->coinPos = (Vector2){ x * g->tileSize + 8.0f, y * g->tileSize + 8.0f };
            g->coinActive = true;
            return;
        }
    }
    // fallback
    g->coinPos = (Vector2){ 2 * g->tileSize, 2 * g->tileSize };
    g->coinActive = true;
}

static void ResetGameplay(Game *g) {
    g->score = 0;
    g->hp = 3;

    // player no “safe zone”
    g->player.pos  = (Vector2){ 2 * g->tileSize + 6.0f, 2 * g->tileSize + 6.0f };
    g->player.vel  = (Vector2){ 0, 0 };

    // enemy distante
    g->enemy.pos   = (Vector2){ (g->mapW - 3) * g->tileSize + 6.0f, (g->mapH - 3) * g->tileSize + 6.0f };
    g->enemy.vel   = (Vector2){ 0, 0 };

    SpawnCoin(g);
}

// =======================
// Init / Shutdown
// =======================
void GameInit(Game *g, int screenW, int screenH) {
    g->screenW = screenW;
    g->screenH = screenH;

    g->tileSize = 48;           // tile grande pra visualizar bem
    g->mapW = MAP_W;
    g->mapH = MAP_H;

    // =========
    // Texturas “fake” (runtime)
    // =========
    // Player spritesheet: 2 frames (azuis) num image 64x32 (cada frame 32x32)
    {
        Image img = GenImageColor(64, 32, BLANK);
        // frame 0
        ImageDrawRectangle(&img, 0, 0, 32, 32, (Color){ 80, 180, 255, 255 });
        ImageDrawRectangle(&img, 10, 8, 12, 16, (Color){ 30, 90, 160, 255 }); // detalhe
        // frame 1 (leve variação)
        ImageDrawRectangle(&img, 32, 0, 32, 32, (Color){ 60, 200, 255, 255 });
        ImageDrawRectangle(&img, 42, 10, 12, 16, (Color){ 30, 90, 160, 255 });

        Texture2D t = LoadTextureFromImage(img);
        UnloadImage(img);

        g->player.anim = MakeAnim(t, 32, 32, 2, 8.0f); // 8 fps
    }

    // Enemy spritesheet: 2 frames vermelhos
    {
        Image img = GenImageColor(64, 32, BLANK);
        ImageDrawRectangle(&img, 0, 0, 32, 32, (Color){ 220, 60, 60, 255 });
        ImageDrawRectangle(&img, 8, 8, 16, 16, (Color){ 120, 20, 20, 255 });
        ImageDrawRectangle(&img, 32, 0, 32, 32, (Color){ 240, 80, 80, 255 });
        ImageDrawRectangle(&img, 40, 10, 16, 16, (Color){ 120, 20, 20, 255 });

        Texture2D t = LoadTextureFromImage(img);
        UnloadImage(img);

        g->enemy.anim = MakeAnim(t, 32, 32, 2, 6.0f); // 6 fps
    }

    // Tile floor / wall (texturas simples)
    {
        // Criamos o retângulo uma vez para reutilizar
        Rectangle rec = { 0, 0, (float)g->tileSize, (float)g->tileSize };

        Image floor = GenImageColor(g->tileSize, g->tileSize, (Color){ 26, 26, 32, 255 });
        ImageDrawRectangleLines(&floor, rec, 1, (Color){ 40, 40, 55, 255 });
        g->tileFloor = LoadTextureFromImage(floor);
        UnloadImage(floor);

        Image wall = GenImageColor(g->tileSize, g->tileSize, (Color){ 70, 70, 90, 255 });
        ImageDrawRectangleLines(&wall, rec, 1, (Color){ 100, 100, 120, 255 });
        g->tileWall = LoadTextureFromImage(wall);
        UnloadImage(wall);
    }

    // Entidades (tamanho 32x32 “sprite scaled”)
    g->player.size  = (Vector2){ 32.0f, 32.0f };
    g->player.speed = 240.0f;

    g->enemy.size   = (Vector2){ 32.0f, 32.0f };
    g->enemy.speed  = 160.0f;

    // Moeda (sprite fake desenhada como retângulo amarelo)
    g->coinSize = (Vector2){ 16.0f, 16.0f };
    g->coinActive = true;

    g->state = SCREEN_MENU;
    SetRandomSeed((unsigned int)GetTime());

    ResetGameplay(g);
}

void GameShutdown(Game *g) {
    // libera texturas (muito importante em C)
    UnloadTexture(g->player.anim.tex);
    UnloadTexture(g->enemy.anim.tex);
    UnloadTexture(g->tileWall);
    UnloadTexture(g->tileFloor);
}

// =======================
// Update — com máquina de estados
// =======================
void GameUpdate(Game *g, float dt) {
    InputState in = InputRead();

    // =========
    // MENU
    // =========
    if (g->state == SCREEN_MENU) {
        if (in.startPressed) {
            ResetGameplay(g);          // garante gameplay limpo ao iniciar
            g->state = SCREEN_GAMEPLAY;
        }
        if (in.quitPressed) {
            // atalho: fecha a janela (equivalente a clicar no X)
            CloseWindow();
        }
        return;
    }

    // =========
    // PAUSE
    // =========
    if (g->state == SCREEN_PAUSE) {
        if (in.pausePressed) g->state = SCREEN_GAMEPLAY;  // volta do pause
        if (in.restartPressed) { ResetGameplay(g); g->state = SCREEN_GAMEPLAY; }
        return;
    }

    // =========
    // GAMEOVER
    // =========
    if (g->state == SCREEN_GAMEOVER) {
        if (in.restartPressed) { ResetGameplay(g); g->state = SCREEN_GAMEPLAY; }
        if (in.quitPressed) g->state = SCREEN_MENU;
        return;
    }

    // =========
    // GAMEPLAY
    // =========
    if (in.pausePressed) {
        g->state = SCREEN_PAUSE;
        return;
    }

    // Player: define velocidade a partir do input
    float pSpeed = g->player.speed;
    if (in.dashPressed) pSpeed *= 1.6f; // dash “instantâneo” simples

    g->player.vel = (Vector2){ in.move.x * pSpeed, in.move.y * pSpeed };

    // Atualiza animação (mesmo parado; você pode condicionar ao movimento)
    AnimUpdate(&g->player.anim, dt);

    // Move e colide com tilemap
    MoveWithTileCollisions(&g->player, dt, g->tileSize);

    // Enemy: segue o player (vetor direção)
    Vector2 toP = (Vector2){ (g->player.pos.x - g->enemy.pos.x), (g->player.pos.y - g->enemy.pos.y) };
    Vector2 dir = NormalizeSafe(toP);
    g->enemy.vel = (Vector2){ dir.x * g->enemy.speed, dir.y * g->enemy.speed };

    AnimUpdate(&g->enemy.anim, dt);
    MoveWithTileCollisions(&g->enemy, dt, g->tileSize);

    // Colisão player x coin
    Rectangle pr = EntRect(&g->player);
    Rectangle cr = (Rectangle){ g->coinPos.x, g->coinPos.y, g->coinSize.x, g->coinSize.y };

    if (g->coinActive && RectIntersects(pr, cr)) {
        g->score += 10;
        SpawnCoin(g);
    }

    // Colisão player x enemy
    Rectangle er = EntRect(&g->enemy);
    if (RectIntersects(pr, er)) {
        g->hp -= 1;

        // “Knockback”: empurra o player pra trás (simples)
        Vector2 away = NormalizeSafe((Vector2){ pr.x - er.x, pr.y - er.y });
        g->player.pos.x += away.x * 18.0f;
        g->player.pos.y += away.y * 18.0f;

        // reposiciona inimigo um pouco pra não drenar HP instantâneo
        g->enemy.pos = (Vector2){
            (float)GetRandomValue((g->mapW/2) * g->tileSize, (g->mapW - 2) * g->tileSize),
            (float)GetRandomValue(2 * g->tileSize, (g->mapH - 2) * g->tileSize)
        };

        if (g->hp <= 0) {
            g->state = SCREEN_GAMEOVER;
        }
    }
}

// =======================
// Draw — desenha baseado no estado
// =======================
void GameDraw(const Game *g) {
    ClearBackground((Color){ 18, 18, 22, 255 });

    // Desenha tilemap (sempre, pra manter contexto em pause/gameover)
    for (int y = 0; y < g->mapH; y++) {
        for (int x = 0; x < g->mapW; x++) {
            Texture2D t = (MAP_DATA[y][x] == 1) ? g->tileWall : g->tileFloor;
            DrawTexture(t, x * g->tileSize, y * g->tileSize, WHITE);
        }
    }

    // HUD comum
    DrawText(TextFormat("SCORE: %d", g->score), 10, 10, 20, RAYWHITE);
    DrawText(TextFormat("HP: %d", g->hp), 10, 34, 20, RAYWHITE);

    // MENU
    if (g->state == SCREEN_MENU) {
        const char *t = "MENU";
        DrawText(t, g->screenW/2 - MeasureText(t, 50)/2, 120, 50, RAYWHITE);

        DrawText("ENTER = Start", g->screenW/2 - MeasureText("ENTER = Start", 24)/2, 220, 24, RAYWHITE);
        DrawText("Q = Quit",      g->screenW/2 - MeasureText("Q = Quit", 24)/2,      260, 24, RAYWHITE);

        DrawText("WASD/Arrows move | P/ESC pause | SPACE dash | R restart",
                 g->screenW/2 - MeasureText("WASD/Arrows move | P/ESC pause | SPACE dash | R restart", 18)/2,
                 g->screenH - 40, 18, (Color){ 170,170,190,255 });
        return;
    }

    // Desenha moeda
    if (g->coinActive) {
        // sprite fake: retângulo amarelo com borda
        DrawRectangleV(g->coinPos, g->coinSize, GOLD);
        DrawRectangleLines((int)g->coinPos.x, (int)g->coinPos.y, (int)g->coinSize.x, (int)g->coinSize.y, (Color){ 140, 120, 30, 255 });
    }

    // Desenha entidades com sprites
    DrawAnim(&g->player.anim, g->player.pos, g->player.size);
    DrawAnim(&g->enemy.anim,  g->enemy.pos,  g->enemy.size);

    // PAUSE overlay
    if (g->state == SCREEN_PAUSE) {
        DrawRectangle(0, 0, g->screenW, g->screenH, (Color){ 0,0,0,120 });
        const char *p = "PAUSED";
        DrawText(p, g->screenW/2 - MeasureText(p, 50)/2, g->screenH/2 - 60, 50, RAYWHITE);
        DrawText("P/ESC = Resume | R = Restart", g->screenW/2 - MeasureText("P/ESC = Resume | R = Restart", 24)/2,
                 g->screenH/2, 24, RAYWHITE);
        return;
    }

    // GAMEOVER overlay
    if (g->state == SCREEN_GAMEOVER) {
        DrawRectangle(0, 0, g->screenW, g->screenH, (Color){ 0,0,0,150 });
        const char *go = "GAME OVER";
        DrawText(go, g->screenW/2 - MeasureText(go, 50)/2, g->screenH/2 - 70, 50, (Color){255,220,220,255});
        DrawText("R = Restart | Q = Menu", g->screenW/2 - MeasureText("R = Restart | Q = Menu", 24)/2,
                 g->screenH/2, 24, RAYWHITE);
        return;
    }

    // Dica no gameplay
    DrawText("P/ESC pause | SPACE dash | R restart", 10, g->screenH - 28, 18, (Color){ 170,170,190,255 });
}