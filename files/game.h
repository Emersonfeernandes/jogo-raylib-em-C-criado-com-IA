/* ============================================================
   game.h — Cabeçalho principal do jogo (versão refatorada)
   Define constantes, o enum ScreenState, a struct Game completa,
   e declara as funções públicas do módulo game.c.

   MUDANÇAS em relação à versão anterior:
   - Removidas: constantes MAP_WIDTH/MAP_HEIGHT (mapa agora é dinâmico, lido de CSV).
   - Adicionadas: struct Game, enum ScreenState, Camera2D, TileMap, Entity.
   - Assinatura das funções corrigida: todas recebem (Game*, ...) em vez de (void).
   ============================================================ */

#ifndef GAME_H   /* Include guard: impede que este header seja processado mais de uma vez. */
#define GAME_H

#include "raylib.h"    /* Tipos da Raylib: Vector2, Rectangle, Texture2D, Camera2D etc. */
#include "tilemap.h"   /* Tipo TileMap: mapa dinâmico carregado de CSV. */
#include "entity.h"    /* Tipos Entity e Anim: jogador, inimigo e animações de spritesheet. */
#include <stdbool.h>   /* Tipo bool (true/false). */

/* ============================================================
   ScreenState — Estados possíveis da máquina de estados do jogo.
   Determina qual lógica/visual está ativa a cada momento.
   Usar enum (em vez de #define com ints) dá tipagem forte:
   o compilador avisa se um switch/case não cobre todos os estados.
   ============================================================ */
typedef enum ScreenState {
    SCREEN_MENU,       /* Tela inicial/título: aguarda o jogador pressionar START. */
    SCREEN_GAMEPLAY,   /* Jogo rodando: atualiza física, colisões e animações. */
    SCREEN_PAUSE,      /* Pausa: congela a lógica do jogo, exibe overlay de pausa. */
    SCREEN_GAMEOVER    /* Fim de jogo: exibe placar final e opção de reiniciar. */
} ScreenState;

/* ============================================================
   ClampF — Utilitário inline: limita `v` ao intervalo [lo, hi].
   `static inline`: o compilador insere o corpo diretamente no ponto
   de chamada (sem overhead de função). `static` evita símbolo
   duplicado no linker quando múltiplos .c incluem este header.
   ============================================================ */
static inline float ClampF(float v, float lo, float hi) {
    if (v < lo) return lo;   /* Valor abaixo do mínimo: retorna o limite inferior. */
    if (v > hi) return hi;   /* Valor acima do máximo: retorna o limite superior. */
    return v;                /* Dentro do intervalo: retorna sem modificação. */
}

/* ============================================================
   Game — Estrutura central que agrupa TODO o estado mutável do jogo.
   Criada em main() e passada por ponteiro (Game *g) para todas as
   funções, eliminando variáveis globais e tornando o estado explícito.
   ============================================================ */
typedef struct Game {

    /* --- Dimensões da janela --- */
    int screenW;   /* Largura da janela em pixels (setada em main(), passada a GameInit). */
    int screenH;   /* Altura da janela em pixels. */

    /* --- Máquina de estados --- */
    ScreenState state;   /* Estado atual da tela; controla GameUpdate e GameDraw. */

    /* --- Mundo / Mapa --- */
    TileMap map;   /* Mapa carregado do arquivo CSV.
                      Contém: tiles[] (heap), width, height, tileSize.
                      Deve ser liberado com TileMapUnload em GameShutdown. */

    /* --- Câmera 2D --- */
    Camera2D cam;  /* Câmera da Raylib que transforma coordenadas de mundo em tela.
                      .target   → ponto do MUNDO que fica no centro visual (segue o player).
                      .offset   → ponto na TELA onde .target é renderizado (ex: centro da tela).
                      .zoom     → fator de escala: 1.0 = tamanho real, 2.0 = tela dobrada.
                      .rotation → rotação em graus da câmera inteira (0 = normal). */

    /* --- Entidades do jogo --- */
    Entity player;   /* Jogador controlado: posição, velocidade, hitbox e animação. */
    Entity enemy;    /* Inimigo simples com a mesma estrutura Entity. */

    /* --- Moeda coletável --- */
    Vector2 coinPos;    /* Posição da moeda no espaço do mundo (canto superior esquerdo). */
    Vector2 coinSize;   /* Tamanho da moeda em pixels (largura × altura). */
    bool coinActive;    /* true → moeda existe no mundo; false → já coletada. */

    /* --- Métricas de jogo --- */
    int score;   /* Pontuação acumulada. Aumenta ao coletar moedas. */
    int hp;      /* Pontos de vida. Ao atingir 0 → muda para SCREEN_GAMEOVER. */

    /* --- Texturas dos tiles --- */
    Texture2D tileWall;    /* Textura para tiles sólidos (valor 1 no CSV). */
    Texture2D tileFloor;   /* Textura para tiles de chão (valor 0 no CSV). */

} Game;

/* ============================================================
   Declarações das funções públicas — implementadas em game.c.
   ============================================================ */

/* Inicializa recursos: carrega texturas/mapa, configura câmera e spawna entidades.
   screenW e screenH vêm do main() e são necessários para centralizar o offset da câmera. */
void GameInit(Game *g, int screenW, int screenH);

/* Atualiza a lógica do jogo a cada frame.
   dt = delta time em segundos (tempo desde o último frame, ex: ~0.016 a 60 FPS). */
void GameUpdate(Game *g, float dt);

/* Desenha o frame atual: mapa, entidades, HUD e overlays.
   `const Game *g` → GameDraw lê o estado mas não o modifica. */
void GameDraw(const Game *g);

/* Libera todos os recursos alocados (texturas, array de tiles). Chamada ao fechar. */
void GameShutdown(Game *g);

#endif   /* Fecha o include guard iniciado com #ifndef GAME_H. */
