/* ============================================================
   tilemap.h — Cabeçalho do módulo de mapa por tiles
   Define TileMap e declara as funções para carregar, consultar
   e descarregar mapas lidos de arquivos CSV.
   ============================================================ */

#ifndef TILEMAP_H
#define TILEMAP_H

#include "raylib.h"    /* Rectangle (usado em TileMapTileRect). */
#include <stdbool.h>   /* bool. */

/* ============================================================
   TileMap — Representa o mapa do mundo como grid de inteiros.
   Armazenamento row-major linear: tile em (col x, linha y) = tiles[y*width+x].
   Alocado dinamicamente; liberar com TileMapUnload().
   ============================================================ */
typedef struct TileMap {
    int  width;     /* Número de colunas (tiles na horizontal). */
    int  height;    /* Número de linhas (tiles na vertical). */
    int  tileSize;  /* Tamanho de cada tile em pixels (ex: 48 → tiles 48×48 px). */
    int *tiles;     /* Array heap: width*height inteiros em ordem row-major.
                       tiles[y * width + x] = valor do tile na coluna x, linha y.
                       Exemplo: width=20, tile (3,2) → tiles[2*20+3] = tiles[43]. */
} TileMap;

/* Carrega um CSV de números inteiros para tiles[].
   O CSV deve ter linhas com a mesma quantidade de colunas (mapa retangular).
   Retorna true em sucesso, false em falha (arquivo, formato ou alocação). */
bool TileMapLoadCSV(TileMap *m, const char *path, int tileSize);

/* Libera tiles[] da heap e zera todos os campos da struct.
   Seguro chamar mesmo se tiles == NULL (mapa não carregado). */
void TileMapUnload(TileMap *m);

/* Retorna o valor inteiro do tile em (x, y) em espaço de TILE.
   Se fora dos limites, retorna 1 (parede) para bloquear saída do mapa. */
int TileMapGet(const TileMap *m, int x, int y);

/* Retorna true se o tile em (x, y) bloqueia passagem (atualmente: valor == 1). */
bool TileMapIsSolid(const TileMap *m, int x, int y);

/* Retorna o Rectangle em pixels (espaço do mundo) do tile em (x, y).
   Útil para colisão AABB entre entidades e tiles. */
Rectangle TileMapTileRect(const TileMap *m, int x, int y);

/* Largura total do mapa em pixels: width * tileSize. */
int TileMapWorldW(const TileMap *m);

/* Altura total do mapa em pixels: height * tileSize. */
int TileMapWorldH(const TileMap *m);

#endif
