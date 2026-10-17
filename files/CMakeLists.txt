# ============================================================
# CMakeLists.txt — Script de build do projeto com CMake
# ============================================================

# Versão mínima do CMake. 3.20+ garante suporte a C11 e vcpkg.
cmake_minimum_required(VERSION 3.20)

# Nome do projeto e linguagem.
# ${PROJECT_NAME} = "jogo_raylib" em todo o script abaixo.
project(jogo_raylib C)

# Padrão C11 obrigatório: usa strtok_r, bool, compound literals, etc.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Cria o executável listando TODOS os arquivos .c do projeto.
# MUDANÇA: adicionados tilemap.c e entity.c em relação à versão anterior.
add_executable(${PROJECT_NAME}
    main.c        # Loop principal: InitWindow → loop → CloseWindow.
    game.c        # Lógica: estados, câmera, colisão, draw.
    input.c       # Mapeamento de teclas → InputState.
    tilemap.c     # Carregamento de CSV e consulta de tiles.
    entity.c      # Animação de spritesheet (UpdateAnim/DrawAnim).
)

# Adiciona o diretório raiz ao caminho de busca de headers.
# Permite #include "game.h" sem caminhos relativos nos arquivos .c.
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Linka a biblioteca de matemática (math.h: roundf, floorf).
# Necessário no Linux e MinGW (-lm). No MSVC já está no CRT e este link é ignorado.
target_link_libraries(${PROJECT_NAME} PRIVATE m)

# Localiza a Raylib instalada (via vcpkg ou instalação manual).
# CONFIG = usa RaylibConfig.cmake gerado pela instalação da Raylib.
# REQUIRED = aborta o build com erro claro se não encontrar.
find_package(raylib CONFIG REQUIRED)

# Linka a Raylib ao executável.
# PRIVATE = dependência de implementação, não propagada para outros targets.
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
