/* ============================================================
   entity.h — Tipos Anim e Entity, compartilhados entre módulos.
   Separado de game.h para facilitar reuso em inimigos, NPCs, projéteis.
   ============================================================ */

#ifndef ENTITY_H
#define ENTITY_H

#include "raylib.h"   /* Texture2D, Vector2, Rectangle, DrawTexturePro. */

/* ============================================================
   Anim — Controla a animação de um spritesheet.
   Um spritesheet é uma imagem com vários frames lado a lado na horizontal.
   Esta struct sabe qual frame exibir agora e quando avançar para o próximo.
   ============================================================ */
typedef struct Anim {
    Texture2D tex;        /* Handle da textura na GPU (carregada com LoadTexture). */
    int frameW;           /* Largura de cada frame em pixels (ex: 32). */
    int frameH;           /* Altura de cada frame em pixels (ex: 32). */
    int frameCount;       /* Total de frames na animação (ex: 4). */
    int currentFrame;     /* Índice do frame atual, de 0 a frameCount-1. */
    float timer;          /* Acumulador de tempo em segundos desde o último avanço. */
    float frameDuration;  /* Quanto tempo (s) cada frame é exibido antes de avançar.
                             Ex: 0.1 → 10 FPS de animação, independente do FPS do jogo. */
} Anim;

/* ============================================================
   Entity — Representa qualquer objeto com física e visual no mundo.
   Usada para jogador, inimigo e qualquer entidade móvel.
   ============================================================ */
typedef struct Entity {
    Vector2 pos;        /* Posição no mundo em pixels (canto superior esquerdo da hitbox). */
    Vector2 size;       /* Tamanho da hitbox em pixels (largura × altura). */
    Vector2 velocity;   /* Velocidade atual em pixels/segundo. */
    Vector2 remainder;  /* Acúmulo de movimento fracionário (sweep/slide pixel a pixel).
                           Evita perda de velocidade ao truncar para inteiros. */
    float speed;        /* Velocidade máxima em pixels/segundo. */
    Anim  anim;         /* Dados de animação do spritesheet desta entidade. */
} Entity;

/* ============================================================
   UpdateAnim — Avança o frame de animação com base no tempo.
   Deve ser chamada todo frame com o delta time do jogo.
   ============================================================ */
void UpdateAnim(Anim *a, float dt);

/* ============================================================
   DrawAnim — Desenha o frame atual do spritesheet em `pos`, escalado para `size`.
   Usa DrawTexturePro para suportar qualquer tamanho de destino.
   ============================================================ */
void DrawAnim(const Anim *a, Vector2 pos, Vector2 size);

#endif
