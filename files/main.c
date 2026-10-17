/* ============================================================
   main.c — Ponto de entrada do programa (versão corrigida)
   Inicializa a janela, roda o loop principal e encerra o jogo.

   CORREÇÃO em relação à versão anterior:
   - Game g declarado corretamente: game.h agora define typedef struct Game.
   - GameInit/Update/Draw/Shutdown chamados com (Game*) como esperado por game.c.
   ============================================================ */

#include "raylib.h"   /* InitWindow, SetTargetFPS, WindowShouldClose,
                         GetFrameTime, BeginDrawing, EndDrawing, CloseWindow. */
#include "game.h"     /* Game, GameInit, GameUpdate, GameDraw, GameShutdown. */

int main(void)
{
    /* Resolução da janela em pixels.
       960×540 é um submúltiplo de 1920×1080 (Full HD), conveniente para testes. */
    const int screenW = 960;
    const int screenH = 540;

    /* Cria a janela do SO e inicializa o contexto de renderização OpenGL.
       Deve ser chamada ANTES de qualquer outra função da Raylib. */
    InitWindow(screenW, screenH, "Raylib Prototype - States + Sprites + Tilemap");

    /* Limita a 60 FPS: sem isso o jogo consumiria 100% da CPU/GPU desnecessariamente.
       A Raylib insere sleeps automáticos para manter o tempo de frame estável. */
    SetTargetFPS(60);

    /* Declara a struct Game na stack e zera todos os campos com = {0}.
       Isso garante que nenhum campo fique com lixo de memória antes de GameInit.
       Game é definido em game.h como typedef struct Game { ... } Game. */
    Game g = {0};

    /* Inicializa todo o estado do jogo: carrega texturas/mapa, configura câmera,
       spawna entidades. Passa o endereço de `g` e as dimensões da janela. */
    GameInit(&g, screenW, screenH);

    /* ============================================================
       Loop principal: executa até o usuário fechar a janela (X) ou
       pressionar ESC (comportamento padrão da Raylib).
       WindowShouldClose() também retorna true após CloseWindow() em game.c.
       ============================================================ */
    while (!WindowShouldClose())
    {
        /* GetFrameTime: tempo em segundos desde o último frame.
           A 60 FPS estável: dt ≈ 0.0167s.
           Garante "frame rate independent movement": a mesma distância
           por segundo, independente de quantos frames ocorreram. */
        float dt = GetFrameTime();

        /* Atualiza lógica: input, movimento, colisões, máquina de estados. */
        GameUpdate(&g, dt);

        /* BeginDrawing: prepara o back-buffer para receber comandos de draw. */
        BeginDrawing();

            /* Desenha mapa, entidades, HUD e overlays de estado no back-buffer. */
            GameDraw(&g);

        /* EndDrawing: apresenta o back-buffer na tela (swap de buffers)
           e aguarda para manter 60 FPS (via sleep interno da Raylib). */
        EndDrawing();
    }

    /* Libera recursos do jogo (texturas, tiles) ANTES de CloseWindow,
       pois o contexto OpenGL ainda precisa existir para UnloadTexture. */
    GameShutdown(&g);

    /* Fecha a janela e destrói o contexto OpenGL. Última chamada à Raylib. */
    CloseWindow();

    return 0;   /* Convenção: 0 = programa encerrou sem erros. */
}
