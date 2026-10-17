/* ============================================================
   input.c — Implementação do módulo de entrada
   Centraliza o mapeamento de teclas → ações do jogo.
   Para remapear controles, edite apenas este arquivo.

   MUDANÇAS em relação à versão anterior:
   - InputRead() implementada (antes só declarada em input.h).
   - GetInputVector() mantida e agora delegada por InputRead.
   ============================================================ */

#include "input.h"   /* InputState, GetInputVector. Traz raylib.h e stdbool.h. */

/* ============================================================
   GetInputVector — Lê W/A/S/D e retorna a direção normalizada.
   Função auxiliar; chamada internamente por InputRead().
   ============================================================ */
Vector2 GetInputVector(void)
{
    /* Inicializa o vetor zerado: nenhuma tecla pressionada = sem direção. */
    Vector2 dir = {0};

    /* IsKeyDown retorna true enquanto a tecla estiver MANTIDA pressionada.
       Diferente de IsKeyPressed, que dispara só no primeiro frame. */
    if (IsKeyDown(KEY_D)) dir.x += 1.0f;   /* Direita: incrementa X. */
    if (IsKeyDown(KEY_A)) dir.x -= 1.0f;   /* Esquerda: decrementa X. */
    if (IsKeyDown(KEY_W)) dir.y -= 1.0f;   /* Cima: Y negativo (Y cresce para baixo). */
    if (IsKeyDown(KEY_S)) dir.y += 1.0f;   /* Baixo: Y positivo. */

    /* Normalização: garante magnitude = 1.0 em qualquer direção.
       Sem isso, diagonal (D+S) teria magnitude √2 ≈ 1.41 → 41% mais rápida.
       Vector2Length retorna 0.0 se nenhuma tecla for pressionada (dir = {0,0}).
       Normalizamos só se o vetor não for zero para evitar divisão por zero. */
    if (Vector2Length(dir) > 0.0f)
        dir = Vector2Normalize(dir);

    return dir;
}

/* ============================================================
   InputRead — Lê TODO o estado de entrada do frame atual.
   Preenche e retorna um InputState com movimento e todas as ações.
   Chamada uma vez por frame no início de GameUpdate().
   ============================================================ */
InputState InputRead(void)
{
    /* Inicializa a struct com zeros: todos os campos bool = false,
       Vector2 = {0,0}. Apenas os campos relevantes são sobrescritos abaixo. */
    InputState inp = {0};

    /* ── Direção de movimento ── */
    /* Reutiliza GetInputVector para não duplicar o código de normalização. */
    inp.move = GetInputVector();

    /* ── Ações de pressionar tecla (apenas no frame do press) ──
       IsKeyPressed retorna true SOMENTE no frame em que a tecla foi pressionada,
       não enquanto mantida. Ideal para ações de toggle como pausa e start. */

    /* ENTER inicia o jogo a partir do menu. */
    inp.startPressed = IsKeyPressed(KEY_ENTER);

    /* P alterna pausa / retoma o jogo. */
    inp.pausePressed = IsKeyPressed(KEY_P);

    /* R reinicia na tela de game over. */
    inp.restartPressed = IsKeyPressed(KEY_R);

    /* SPACE executa o dash (esquiva). */
    inp.dashPressed = IsKeyPressed(KEY_SPACE);

    /* Q fecha o jogo na tela de game over ou menu. */
    inp.quitPressed = IsKeyPressed(KEY_Q);

    return inp;   /* Retorna cópia na stack: barato, struct é pequena. */
}
