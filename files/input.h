/* ============================================================
   input.h — Cabeçalho do módulo de entrada (input)
   Define a estrutura InputState e declara as funções públicas.

   MUDANÇAS em relação à versão anterior:
   - InputRead() agora é a função principal (não apenas declarada).
   - GetInputVector() mantida para compatibilidade com código legado.
   ============================================================ */

#ifndef INPUT_H
#define INPUT_H

#include "raylib.h"    /* Vector2, IsKeyDown, IsKeyPressed. */
#include <stdbool.h>   /* bool, true, false. */

/* ============================================================
   InputState — Agrupa todas as ações de entrada do frame atual.
   Preenchida por InputRead() a cada frame e consumida por GameUpdate().
   Centralizar o input assim facilita remapear teclas: basta editar input.c.
   ============================================================ */
typedef struct InputState {

    Vector2 move;        /* Direção de movimento normalizada (magnitude ≤ 1.0).
                            move.x: -1.0 = esquerda, +1.0 = direita, 0 = parado.
                            move.y: -1.0 = cima,     +1.0 = baixo,   0 = parado.
                            Normalizado: diagonal tem a mesma velocidade que reto. */

    bool startPressed;   /* true APENAS no frame em que ENTER foi pressionado.
                            Usado para iniciar o jogo a partir do menu. */

    bool pausePressed;   /* true APENAS no frame em que P foi pressionado.
                            Alterna entre SCREEN_GAMEPLAY e SCREEN_PAUSE. */

    bool restartPressed; /* true APENAS no frame em que R foi pressionado.
                            Reinicia o jogo na tela de game over. */

    bool dashPressed;    /* true APENAS no frame em que SPACE foi pressionado.
                            Reservado para habilidade de esquiva. */

    bool quitPressed;    /* true APENAS no frame em que Q foi pressionado.
                            Fecha o jogo a partir do game over. */

} InputState;

/* ============================================================
   InputRead — Lê o estado do teclado e retorna um InputState preenchido.
   Deve ser chamada UMA VEZ por frame no início de GameUpdate.
   Retorna por valor (cópia barata: struct pequena, toda na stack).
   ============================================================ */
InputState InputRead(void);

/* ============================================================
   GetInputVector — Versão simplificada: retorna apenas o vetor
   de direção de movimento normalizado (W/A/S/D).
   Mantida para compatibilidade; internamente delegada por InputRead.
   ============================================================ */
Vector2 GetInputVector(void);

#endif   /* Fecha o include guard. */
