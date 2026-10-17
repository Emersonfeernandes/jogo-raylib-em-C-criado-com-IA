/* ============================================================
   entity.c — Implementação de UpdateAnim e DrawAnim.
   ============================================================ */

#include "entity.h"   /* Anim, Entity, UpdateAnim, DrawAnim. */

/* ============================================================
   UpdateAnim — Avança o frame quando o acumulador ultrapassa frameDuration.
   Subtrai em vez de zerar o timer: preserva o excedente para
   que animações rápidas não percam tempo entre frames.
   ============================================================ */
void UpdateAnim(Anim *a, float dt) {
    a->timer += dt;   /* Acumula tempo desde o último avanço. */

    if (a->timer >= a->frameDuration) {
        a->timer -= a->frameDuration;   /* Preserva o excedente (não zera). */
        a->currentFrame++;

        /* Loop da animação: volta ao frame 0 ao passar do último. */
        if (a->currentFrame >= a->frameCount)
            a->currentFrame = 0;
    }
}

/* ============================================================
   DrawAnim — Desenha o frame atual do spritesheet escalado para `size`.
   DrawTexturePro permite recorte de origem e destino com tamanhos diferentes,
   suportando escala e rotação sem precisar alterar a textura.
   ============================================================ */
void DrawAnim(const Anim *a, Vector2 pos, Vector2 size) {
    /* Retângulo de ORIGEM: recorta o frame atual no spritesheet.
       Frames organizados horizontalmente → X = currentFrame * frameW. */
    Rectangle source = {
        (float)(a->currentFrame * a->frameW),   /* X do frame no spritesheet. */
        0.0f,                                    /* Y = 0: todos os frames na linha 0. */
        (float)a->frameW,                        /* Largura do frame original. */
        (float)a->frameH                         /* Altura do frame original. */
    };

    /* Retângulo de DESTINO: onde e com qual tamanho desenhar na tela/mundo.
       `size` pode diferir de frameW/frameH → escala automática pelo DrawTexturePro. */
    Rectangle dest = {
        pos.x,    /* X na tela (ou mundo, se dentro de BeginMode2D). */
        pos.y,    /* Y na tela. */
        size.x,   /* Largura de exibição (pode escalar o frame). */
        size.y    /* Altura de exibição. */
    };

    /* Ponto de pivô para rotação: (0,0) = canto superior esquerdo (sem deslocamento). */
    Vector2 origin = {0.0f, 0.0f};

    /* DrawTexturePro: versão completa de DrawTexture com suporte a recorte,
       escala, rotação e tint. 0.0f = sem rotação. WHITE = sem tint de cor. */
    DrawTexturePro(a->tex, source, dest, origin, 0.0f, WHITE);
}
