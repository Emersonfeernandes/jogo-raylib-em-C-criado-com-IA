/* ============================================================
   tilemap.c — Implementação do módulo de mapa por tiles

   CORREÇÃO em relação ao código original:
   - strtok_s (MSVC) substituído por strtok_r (POSIX/GCC/MinGW/Linux/macOS).
     strtok_s não existe em compiladores GCC/MinGW sem extensões MSVC,
     causando erro de compilação. strtok_r é o equivalente thread-safe padrão.
   ============================================================ */

#include "tilemap.h"   /* TileMap e declarações das funções. */
#include <stdio.h>     /* FILE, fopen, fgets, fclose, rewind. */
#include <stdlib.h>    /* malloc, free, atoi. */
#include <string.h>    /* memset, strtok_r. */

/* ── Auxiliar interna: zera todos os campos da struct ── */
static void TileMapZero(TileMap *m) {
    m->width    = 0;
    m->height   = 0;
    m->tileSize = 0;
    m->tiles    = NULL;   /* NULL garante que free(m->tiles) será seguro. */
}

/* ============================================================
   CountColumns — Conta números separados por vírgula em uma linha CSV.
   Algoritmo de flag: ao entrar em um número (dígito ou '-'), incrementa
   o contador; ao sair (vírgula ou outro char), reseta a flag.
   Retorna o total de valores numéricos encontrados.
   ============================================================ */
static int CountColumns(const char *line) {
    int cols      = 0;
    const char *p = line;
    bool inNumber = false;

    while (*p) {
        if ((*p >= '0' && *p <= '9') || *p == '-') {
            /* Início ou continuação de um número. */
            if (!inNumber) { cols++; inNumber = true; }
        } else if (*p == ',') {
            inNumber = false;   /* Vírgula separa tokens: próximo será novo número. */
        }
        /* \n, \r, espaço: ignorados silenciosamente. */
        p++;
    }
    return cols;
}

/* ============================================================
   TileMapLoadCSV — Carrega um arquivo CSV e preenche a TileMap.
   Duas passagens:
     1ª: conta linhas e colunas (descobre width e height).
     2ª: lê os valores inteiros de cada célula.
   ============================================================ */
bool TileMapLoadCSV(TileMap *m, const char *path, int tileSize) {
    TileMapUnload(m);   /* Libera dados anteriores para evitar memory leak. */
    TileMapZero(m);     /* Estado limpo antes de preencher. */

    /* "rb" = modo binário: evita conversão automática de \r\n em \n no Windows,
       o que poderia confundir a contagem de colunas. */
    FILE *f = fopen(path, "rb");
    if (!f) return false;   /* Arquivo não encontrado ou sem permissão de leitura. */

    /* ── 1ª PASSADA: descobre dimensões ── */
    char line[8192];   /* Buffer para uma linha. 8192 chars suporta mapas muito largos. */
    int width  = -1;   /* -1 = ainda não encontramos a primeira linha válida. */
    int height = 0;

    while (fgets(line, sizeof(line), f)) {
        /* fgets lê até \n ou EOF. Ignora linhas vazias ou com só whitespace. */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;

        int cols = CountColumns(line);
        if (cols <= 0) continue;   /* Linha sem números: ignora (ex: comentário). */

        if (width < 0) {
            width = cols;            /* 1ª linha válida define a largura do mapa. */
        } else if (cols != width) {
            /* Linhas com colunas diferentes quebram o layout retangular → falha. */
            fclose(f);
            return false;
        }
        height++;
    }

    if (width <= 0 || height <= 0) { fclose(f); return false; }

    /* ── Alocação do array de tiles ──
       sizeof(int) * width * height bytes contíguos na heap. */
    int *tiles = (int*)malloc(sizeof(int) * width * height);
    if (!tiles) { fclose(f); return false; }   /* Falha de alocação (memória esgotada). */

    /* Preenche com zeros por segurança antes de sobrescrever com os valores do CSV. */
    memset(tiles, 0, sizeof(int) * width * height);

    /* ── 2ª PASSADA: lê valores ── */
    rewind(f);   /* Volta ao início do arquivo. */
    int y = 0;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') continue;

        int    x    = 0;
        char *save  = NULL;   /* Estado interno do strtok_r (thread-safe, por isso usa save). */

        /* ✅ FIX: strtok_r (POSIX/GCC/MinGW) em vez de strtok_s (MSVC exclusivo).
           Divide `line` em tokens separados por ",", "\r" ou "\n".
           Na primeira chamada: passa `line`. Nas seguintes: passa NULL. */
        char *tok = strtok_r(line, ",\r\n", &save);

        while (tok && x < width) {
            /* atoi("1") = 1, atoi("0") = 0. Armazena em row-major: linha y, coluna x. */
            tiles[y * width + x] = atoi(tok);
            x++;
            tok = strtok_r(NULL, ",\r\n", &save);   /* Próximo token da mesma linha. */
        }

        if (x == width) y++;     /* Linha completa: avança de linha. */
        if (y >= height) break;  /* Leu todas as linhas esperadas: para. */
    }

    fclose(f);

    /* Preenche a struct com os dados carregados. */
    m->width    = width;
    m->height   = height;
    m->tileSize = tileSize;
    m->tiles    = tiles;   /* Transfere propriedade do ponteiro: TileMap "possui" o array. */
    return true;
}

/* Libera tiles[] e zera a struct. */
void TileMapUnload(TileMap *m) {
    if (m->tiles) free(m->tiles);   /* free(NULL) seria seguro, mas o if é explícito. */
    TileMapZero(m);
}

/* Retorna valor do tile em (x, y). Fora dos limites → 1 (parede). */
int TileMapGet(const TileMap *m, int x, int y) {
    if (!m || !m->tiles) return 1;
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return 1;
    return m->tiles[y * m->width + x];   /* Row-major: linha y, coluna x. */
}

/* true se tile == 1 (parede). Delega para TileMapGet (que faz bounds check). */
bool TileMapIsSolid(const TileMap *m, int x, int y) {
    return TileMapGet(m, x, y) == 1;
}

/* Rectangle em pixels do tile (x, y) no espaço do mundo. */
Rectangle TileMapTileRect(const TileMap *m, int x, int y) {
    return (Rectangle){
        (float)(x * m->tileSize),   /* Borda esquerda em pixels. */
        (float)(y * m->tileSize),   /* Borda superior em pixels. */
        (float)m->tileSize,         /* Largura = tileSize. */
        (float)m->tileSize          /* Altura  = tileSize. */
    };
}

/* Tamanho total do mundo em pixels. */
int TileMapWorldW(const TileMap *m) { return m->width  * m->tileSize; }
int TileMapWorldH(const TileMap *m) { return m->height * m->tileSize; }
