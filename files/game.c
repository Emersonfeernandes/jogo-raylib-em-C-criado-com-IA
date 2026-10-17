/* ============================================================
   game.c — Lógica principal do jogo (versão refatorada)

   MUDANÇAS em relação à versão anterior:
   - Variáveis globais (player, tileset, map[][]) removidas.
     Todo o estado agora vive na struct Game passada por ponteiro.
   - Mapa estático substituído por TileMap carregado de CSV.
   - Câmera Camera2D adicionada, com clamping aos limites do mundo.
   - Máquina de estados (ScreenState) controla Update e Draw.
   - Frustum culling no draw: só tiles visíveis são desenhados.
   - Assinatura das funções corrigida para receber (Game *g).
   ============================================================ */

#include "game.h"    /* Game, ScreenState, ClampF, GameInit/Update/Draw/Shutdown. */
#include "input.h"   /* InputRead() → retorna InputState com todas as ações do frame. */
#include <math.h>    /* roundf(), floorf(). */

/* ============================================================
   MoveEntity — Move uma entidade com sweep/slide pixel a pixel,
   verificando colisão contra os tiles sólidos do TileMap.

   Por que pixel a pixel?
   Saltos grandes de posição (ex: velocity * dt direto) podem atravessar
   paredes finas. Mover um pixel de cada vez garante que nenhuma
   colisão seja pulada, mesmo em alta velocidade.

   Parâmetros:
     e   → entidade a mover (pos e remainder são modificados in-place).
     map → mapa consultado para solidez dos tiles.
     dt  → delta time em segundos.
   ============================================================ */
static void MoveEntity(Entity *e, const TileMap *map, float dt)
{
    /* Acumula o deslocamento deste frame no buffer de resto fracionário.
       velocity (px/s) × dt (s) = deslocamento em pixels (pode ser decimal).
       Ex: velocity.x = 200, dt = 0.016 → acumula 3.2 pixels. */
    e->remainder.x += e->velocity.x * dt;
    e->remainder.y += e->velocity.y * dt;

    /* Extrai a parte inteira: quantos pixels inteiros mover agora.
       roundf arredonda para o inteiro mais próximo antes do cast para int.
       roundf(3.2) = 3 | roundf(-0.7) = -1. */
    int moveX = (int)roundf(e->remainder.x);
    int moveY = (int)roundf(e->remainder.y);

    /* Remove os pixels inteiros do acumulador; só o resto decimal permanece.
       Esse decimal será somado nos próximos frames: velocidade sempre precisa. */
    e->remainder.x -= (float)moveX;
    e->remainder.y -= (float)moveY;

    /* ── Movimento Horizontal ── */

    int stepX = (moveX > 0) ? 1 : -1;   /* Direção do passo: +1 (direita) ou -1 (esquerda). */

    while (moveX != 0)
    {
        /* Converte a próxima posição X para índice de tile (pixel / tileSize).
           (int) trunca: 96 / 48 = 2 → tile coluna 2. */
        int nextTileX = (int)((e->pos.x + stepX) / map->tileSize);

        /* Tile Y da entidade — não muda durante deslocamento puramente horizontal. */
        int tileY = (int)(e->pos.y / map->tileSize);

        if (!TileMapIsSolid(map, nextTileX, tileY)) {
            e->pos.x += (float)stepX;   /* Tile livre: move um pixel na horizontal. */
            moveX    -= stepX;           /* Consome um pixel do deslocamento pendente. */
        }
        else break;   /* Parede à frente: para horizontal, mas Y pode continuar (slide). */
    }

    /* ── Movimento Vertical ── */

    int stepY = (moveY > 0) ? 1 : -1;   /* +1 = baixo (Y cresce para baixo), -1 = cima. */

    while (moveY != 0)
    {
        int tileX     = (int)(e->pos.x  / map->tileSize);   /* X de tile não muda no mov. vertical. */
        int nextTileY = (int)((e->pos.y + stepY) / map->tileSize);

        if (!TileMapIsSolid(map, tileX, nextTileY)) {
            e->pos.y += (float)stepY;
            moveY    -= stepY;
        }
        else break;
    }
}

/* ============================================================
   GameInit — Inicializa todos os recursos e o estado inicial do jogo.
   Chamada uma única vez em main() antes do loop principal.
   Parâmetros: ponteiro para Game e dimensões da janela.
   ============================================================ */
void GameInit(Game *g, int screenW, int screenH)
{
    /* Salva as dimensões para uso no clamping da câmera e nos overlays de HUD. */
    g->screenW = screenW;
    g->screenH = screenH;

    /* Máquina de estados começa no menu: o jogador verá a tela inicial. */
    g->state = SCREEN_MENU;

    /* ── Carregamento do mapa ── */

    int tileSize = 48;   /* Cada tile ocupa 48×48 pixels. Ajuste conforme seus assets. */

    /* TileMapLoadCSV: lê o CSV, aloca tiles[] na heap e preenche width/height/tileSize.
       Caminho relativo ao diretório de execução do .exe.
       Retorna false se o arquivo não for encontrado ou estiver malformado. */
    if (!TileMapLoadCSV(&g->map, "assets/map.csv", tileSize)) {
        /* TraceLog imprime uma mensagem no console de debug da Raylib.
           Em produção, considere exibir erro na tela ou abortar. */
        TraceLog(LOG_WARNING, "TILEMAP: Falha ao carregar assets/map.csv");
    }

    /* ── Texturas dos tiles ── */

    /* LoadTexture envia a imagem ao driver de GPU e retorna um handle Texture2D.
       O arquivo PNG deve existir em assets/ relativo ao executável. */
    g->tileWall  = LoadTexture("assets/tile_wall.png");
    g->tileFloor = LoadTexture("assets/tile_floor.png");

    /* ── Câmera 2D ── */

    /* Zera todos os campos antes de configurar os relevantes. */
    g->cam = (Camera2D){0};

    /* zoom 1.0 = sem escala: 1 pixel de mundo = 1 pixel na tela. */
    g->cam.zoom = 1.0f;

    /* offset: ponto fixo na TELA onde .target será renderizado.
       Usar o centro da janela mantém o jogador centralizado na tela. */
    g->cam.offset = (Vector2){ screenW * 0.5f, screenH * 0.5f };

    /* target: ponto do MUNDO que a câmera mira. Atualizado em GameUpdate. */
    g->cam.target = (Vector2){ 0.0f, 0.0f };

    /* ── Jogador ── */

    g->player.pos       = (Vector2){ 2.0f * tileSize, 2.0f * tileSize };   /* Tile (2,2). */
    g->player.size      = (Vector2){ 32.0f, 32.0f };   /* Hitbox 32×32 pixels. */
    g->player.velocity  = (Vector2){ 0.0f, 0.0f };
    g->player.remainder = (Vector2){ 0.0f, 0.0f };
    g->player.speed     = 200.0f;   /* 200 px/s de velocidade máxima. */

    /* Configura a animação do spritesheet do jogador. */
    g->player.anim.tex           = LoadTexture("assets/player.png");
    g->player.anim.frameW        = 32;      /* Largura de cada frame no PNG. */
    g->player.anim.frameH        = 32;      /* Altura de cada frame. */
    g->player.anim.frameCount    = 4;       /* 4 frames de animação de corrida. */
    g->player.anim.currentFrame  = 0;
    g->player.anim.timer         = 0.0f;
    g->player.anim.frameDuration = 0.12f;   /* ~8 FPS de animação. */

    /* ── Inimigo ── */

    g->enemy.pos       = (Vector2){ 8.0f * tileSize, 8.0f * tileSize };
    g->enemy.size      = (Vector2){ 32.0f, 32.0f };
    g->enemy.velocity  = (Vector2){ 0.0f, 0.0f };
    g->enemy.remainder = (Vector2){ 0.0f, 0.0f };
    g->enemy.speed     = 80.0f;   /* Inimigo mais lento que o jogador. */

    g->enemy.anim.tex           = LoadTexture("assets/enemy.png");
    g->enemy.anim.frameW        = 32;
    g->enemy.anim.frameH        = 32;
    g->enemy.anim.frameCount    = 2;      /* Animação simples de 2 frames. */
    g->enemy.anim.currentFrame  = 0;
    g->enemy.anim.timer         = 0.0f;
    g->enemy.anim.frameDuration = 0.3f;   /* Animação lenta: troca a cada 0.3s. */

    /* ── Moeda coletável ── */

    g->coinPos    = (Vector2){ 5.0f * tileSize, 5.0f * tileSize };   /* Tile (5,5). */
    g->coinSize   = (Vector2){ 16.0f, 16.0f };
    g->coinActive = true;   /* Começa presente no mundo. */

    /* ── Métricas iniciais ── */

    g->score = 0;   /* Pontuação zerada. */
    g->hp    = 3;   /* 3 pontos de vida. */
}

/* ============================================================
   GameUpdate — Atualiza a lógica do jogo a cada frame.
   ============================================================ */
void GameUpdate(Game *g, float dt)
{
    /* Lê todo o estado de input de uma vez.
       InputRead encapsula IsKeyDown/IsKeyPressed para cada ação mapeada. */
    InputState inp = InputRead();

    /* Máquina de estados: cada case trata a lógica do estado correspondente. */
    switch (g->state)
    {
        /* ── MENU ── */
        case SCREEN_MENU:
            /* startPressed é true APENAS no frame em que ENTER foi pressionado.
               IsKeyPressed (não IsKeyDown) evita transições repetidas. */
            if (inp.startPressed)
                g->state = SCREEN_GAMEPLAY;
            break;

        /* ── GAMEPLAY ── */
        case SCREEN_GAMEPLAY:

            /* Verifica pausa antes de qualquer lógica de jogo. */
            if (inp.pausePressed) {
                g->state = SCREEN_PAUSE;
                break;   /* Sai do switch: não executa lógica de jogo neste frame. */
            }

            /* ── Movimentação do jogador ──
               inp.move é um Vector2 normalizado (magnitude ≤ 1.0).
               Multiplicar pela speed dá a velocidade em px/s. */
            g->player.velocity.x = inp.move.x * g->player.speed;
            g->player.velocity.y = inp.move.y * g->player.speed;

            /* Sweep/slide pixel a pixel com colisão contra tiles sólidos. */
            MoveEntity(&g->player, &g->map, dt);

            /* ── Animação ──
               Avança os frames de acordo com o tempo acumulado. */
            UpdateAnim(&g->player.anim, dt);
            UpdateAnim(&g->enemy.anim,  dt);

            /* ── Câmera segue o centro do jogador ──
               .pos é o canto superior esquerdo; soma metade do tamanho = centro. */
            {
                Vector2 playerCenter = {
                    g->player.pos.x + g->player.size.x * 0.5f,
                    g->player.pos.y + g->player.size.y * 0.5f
                };
                g->cam.target = playerCenter;

                /* ── Clamping da câmera ──
                   Impede que a câmera mostre área além das bordas do mapa.
                   halfW/halfH = metade da tela em coordenadas de mundo (corrigido pelo zoom). */
                float halfW = (g->screenW * 0.5f) / g->cam.zoom;
                float halfH = (g->screenH * 0.5f) / g->cam.zoom;

                float minX = halfW;
                float maxX = (float)TileMapWorldW(&g->map) - halfW;
                float minY = halfH;
                float maxY = (float)TileMapWorldH(&g->map) - halfH;

                /* Se o mapa for menor que a tela, centraliza no meio do mapa
                   para evitar clamp invertido (min > max → comportamento indefinido). */
                if (maxX < minX) { minX = maxX = TileMapWorldW(&g->map) * 0.5f; }
                if (maxY < minY) { minY = maxY = TileMapWorldH(&g->map) * 0.5f; }

                /* ClampF: mantém o target dentro do intervalo [min, max]. */
                g->cam.target.x = ClampF(g->cam.target.x, minX, maxX);
                g->cam.target.y = ClampF(g->cam.target.y, minY, maxY);
            }

            /* ── Coleta de moeda ──
               CheckCollisionRecs: detecção AABB (Axis-Aligned Bounding Box) simples. */
            if (g->coinActive)
            {
                Rectangle playerRect = { g->player.pos.x, g->player.pos.y,
                                         g->player.size.x, g->player.size.y };
                Rectangle coinRect   = { g->coinPos.x, g->coinPos.y,
                                         g->coinSize.x, g->coinSize.y };

                if (CheckCollisionRecs(playerRect, coinRect)) {
                    g->coinActive = false;   /* Desativa a moeda (não será mais desenhada). */
                    g->score += 10;          /* +10 pontos ao score. */
                }
            }

            /* ── Verificação de derrota ── */
            if (g->hp <= 0)
                g->state = SCREEN_GAMEOVER;

            break;

        /* ── PAUSE ── */
        case SCREEN_PAUSE:
            /* Pressionar P novamente retoma o jogo do ponto onde estava. */
            if (inp.pausePressed)
                g->state = SCREEN_GAMEPLAY;
            break;

        /* ── GAME OVER ── */
        case SCREEN_GAMEOVER:
            /* R reinicia: chama GameInit novamente para resetar TODO o estado.
               As texturas serão recarregadas — otimizável com um GameReset() separado. */
            if (inp.restartPressed)
                GameInit(g, g->screenW, g->screenH);

            /* Q fecha a janela: WindowShouldClose() retornará true no próximo frame. */
            if (inp.quitPressed)
                CloseWindow();
            break;
    }
}

/* ============================================================
   GameDraw — Desenha o frame atual.
   Chamada entre BeginDrawing() e EndDrawing() no main().
   Recebe const Game* porque não deve modificar o estado do jogo.
   ============================================================ */
void GameDraw(const Game *g)
{
    /* Fundo escuro personalizado: (R=18, G=18, B=22, A=255).
       Substitui ClearBackground(BLACK) por uma cor ligeiramente azulada. */
    ClearBackground((Color){18, 18, 22, 255});

    /* =====================================================
       ESPAÇO DO MUNDO — tudo dentro de BeginMode2D/EndMode2D
       é transformado pela câmera (translação + zoom + rotação).
       ===================================================== */
    BeginMode2D(g->cam);

    /* ── Frustum culling: calcula o retângulo de tiles visíveis ──
       Em vez de iterar sobre todos os tiles do mapa (potencialmente milhares),
       calculamos apenas os que estão dentro da janela visível.

       halfW/halfH = metade da tela em coordenadas de MUNDO. */
    float halfW = (g->screenW * 0.5f) / g->cam.zoom;
    float halfH = (g->screenH * 0.5f) / g->cam.zoom;

    /* Bordas do retângulo visível no espaço do mundo. */
    float left   = g->cam.target.x - halfW;
    float right  = g->cam.target.x + halfW;
    float top    = g->cam.target.y - halfH;
    float bottom = g->cam.target.y + halfH;

    /* Converte posições em pixels para índices de tile.
       floorf arredonda para baixo: pixel 47 / 48 = 0 (ainda no tile 0).
       O -1/+1 adiciona margem de 1 tile nas bordas para evitar pop-in visual. */
    int x0 = (int)floorf(left   / g->map.tileSize) - 1;
    int x1 = (int)floorf(right  / g->map.tileSize) + 1;
    int y0 = (int)floorf(top    / g->map.tileSize) - 1;
    int y1 = (int)floorf(bottom / g->map.tileSize) + 1;

    /* Clamp: garante que os índices não saiam dos limites do array de tiles. */
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g->map.width  - 1) x1 = g->map.width  - 1;
    if (y1 > g->map.height - 1) y1 = g->map.height - 1;

    /* ── Desenho dos tiles visíveis ── */
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            /* Consulta o valor do tile: 0=chão, 1=parede (outros valores podem ser adicionados). */
            int t = TileMapGet(&g->map, x, y);

            /* Operador ternário: escolhe textura conforme o tipo de tile. */
            Texture2D tex = (t == 1) ? g->tileWall : g->tileFloor;

            /* DrawTexture: desenha a textura inteira a partir do canto superior esquerdo.
               Posição em pixels = índice de tile × tamanho em pixels. */
            DrawTexture(tex, x * g->map.tileSize, y * g->map.tileSize, WHITE);
        }
    }

    /* ── Moeda coletável ── */
    if (g->coinActive) {
        /* DrawRectangleV: retângulo preenchido na posição e tamanho dados. */
        DrawRectangleV(g->coinPos, g->coinSize, GOLD);
    }

    /* ── Entidades (jogador e inimigo) ──
       DrawAnim recorta o frame atual do spritesheet e escala para `size`. */
    DrawAnim(&g->player.anim, g->player.pos, g->player.size);
    DrawAnim(&g->enemy.anim,  g->enemy.pos,  g->enemy.size);

    EndMode2D();
    /* =====================================================
       ESPAÇO DA TELA — a partir daqui coordenadas são pixels
       fixos na janela, não afetados pela câmera. Ideal para HUD.
       ===================================================== */

    /* ── HUD ── */
    /* TextFormat formata string com printf-style; DrawText renderiza na posição dada. */
    DrawText(TextFormat("SCORE: %d", g->score), 10, 10, 20, RAYWHITE);
    DrawText(TextFormat("HP: %d",    g->hp),    10, 34, 20, RAYWHITE);

    /* ── Overlays por estado ── */
    if (g->state == SCREEN_MENU) {
        /* Overlay semitransparente (alpha=160) escurece o fundo sem cobrir completamente. */
        DrawRectangle(0, 0, g->screenW, g->screenH, (Color){0, 0, 0, 160});
        const char *msg = "PRESSIONE ENTER PARA JOGAR";
        /* MeasureText: retorna a largura em pixels para centralizar o texto. */
        DrawText(msg,
                 g->screenW / 2 - MeasureText(msg, 24) / 2,
                 g->screenH / 2 - 12, 24, WHITE);
    }
    else if (g->state == SCREEN_PAUSE) {
        DrawRectangle(0, 0, g->screenW, g->screenH, (Color){0, 0, 0, 120});
        const char *msg = "PAUSADO  —  P para retomar";
        DrawText(msg,
                 g->screenW / 2 - MeasureText(msg, 24) / 2,
                 g->screenH / 2 - 12, 24, YELLOW);
    }
    else if (g->state == SCREEN_GAMEOVER) {
        DrawRectangle(0, 0, g->screenW, g->screenH, (Color){0, 0, 0, 180});
        const char *title = "GAME OVER";
        DrawText(title,
                 g->screenW / 2 - MeasureText(title, 40) / 2,
                 g->screenH / 2 - 50, 40, RED);
        DrawText(TextFormat("SCORE FINAL: %d", g->score),
                 g->screenW / 2 - 100,
                 g->screenH / 2 + 10, 24, RAYWHITE);
        const char *opts = "R = Reiniciar     Q = Sair";
        DrawText(opts,
                 g->screenW / 2 - MeasureText(opts, 20) / 2,
                 g->screenH / 2 + 50, 20, LIGHTGRAY);
    }
}

/* ============================================================
   GameShutdown — Libera todos os recursos alocados pelo jogo.
   Chamada em main() antes de CloseWindow().
   ============================================================ */
void GameShutdown(Game *g)
{
    /* Libera o array tiles[] da heap e zera os campos da struct TileMap.
       Seguro de chamar mesmo se TileMapLoadCSV falhou (tiles == NULL é tratado). */
    TileMapUnload(&g->map);

    /* Libera texturas dos tiles da memória de vídeo da GPU. */
    UnloadTexture(g->tileWall);
    UnloadTexture(g->tileFloor);

    /* Libera spritesheets das entidades. */
    UnloadTexture(g->player.anim.tex);
    UnloadTexture(g->enemy.anim.tex);
}
