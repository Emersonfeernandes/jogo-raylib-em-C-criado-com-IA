Isso agora é **erro de linkedição** (não é mais includePath). O VS Code está compilando **só o `main.c`** e **não está linkando**:

1. a **raylib** (`InitWindow`, `BeginDrawing` etc.)
2. seus arquivos do jogo (`GameInit`, `GameUpdate`, `GameDraw`, `GameShutdown`)

Por isso dá `undefined reference to ...`.

## ✅ Como corrigir (sem CMake, do jeito rápido)

Você tem que compilar **todos os .c** e linkar as libs:

No terminal **UCRT64** (ou no task do VS Code), rode:

```bash
gcc main.c game.c input.c -o jogo.exe -I C:/msys64/ucrt64/include -L C:/msys64/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

> Ajuste o caminho se seu MSYS2 não estiver em `C:\msys64`.

Isso resolve **tanto** as funções da raylib quanto as do seu `game.c/input.c`.

---

# ✅ Jeito “profissional” (com CMake) — recomendo

O seu build atual está usando o comando automático do VS Code: **“gcc build active file”**. Ele sempre compila só o arquivo aberto. Para projetos com múltiplos arquivos + libs, use CMake.

## 1) Estrutura

Se seus arquivos estão em `.../game/` como no log, faça assim:

```
superTrunfoTema2/
  game/
    CMakeLists.txt
    main.c
    game.c
    game.h
    input.c
    input.h
```

## 2) Crie `game/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(jogo_raylib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    main.c
    game.c
    input.c
)

# Para achar headers locais (game.h, input.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Raylib instalada no MSYS2 (pacman)
find_package(raylib CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
```

## 3) Build (no terminal UCRT64)

Entre na pasta `game/`:

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/jogo_raylib.exe
```

---

# ✅ Se você quer continuar usando o botão “Run/Build” do VS Code

Você precisa trocar do “build active file” por um **tasks.json** que compile tudo.

Crie `.vscode/tasks.json` dentro da pasta que você abriu no VS Code (provavelmente `.../game/.vscode/tasks.json`):

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build Raylib Game (MSYS2 UCRT64)",
      "type": "shell",
      "command": "C:/msys64/ucrt64/bin/gcc.exe",
      "args": [
        "main.c",
        "game.c",
        "input.c",
        "-o",
        "jogo.exe",
        "-I",
        "C:/msys64/ucrt64/include",
        "-L",
        "C:/msys64/ucrt64/lib",
        "-lraylib",
        "-lopengl32",
        "-lgdi32",
        "-lwinmm"
      ],
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": ["$gcc"]
    }
  ]
}
```

Depois use: **Terminal → Run Build Task (Ctrl+Shift+B)**.

---

Perfeito 👍 agora o erro é simples:

> ❌ `cmake` não é reconhecido

Isso significa que o **CMake não está instalado** (ou não está no PATH).

---

# ✅ SOLUÇÃO 1 (RECOMENDADO) — Instalar CMake pelo MSYS2

Como você já usa MSYS2 UCRT64, faça assim:

### 1️⃣ Abra o terminal **UCRT64** (não PowerShell)

Menu Iniciar → MSYS2 → **UCRT64**

### 2️⃣ Instale o CMake

```bash
pacman -S mingw-w64-ucrt-x86_64-cmake
```

### 3️⃣ Teste

```bash
cmake --version
```

Se aparecer a versão → pronto ✅

---

# 🚨 IMPORTANTE

Você tentou rodar no **PowerShell do Windows**:

```
cmake -S . -B build -G "MinGW Makefiles"
```

Mas o correto é rodar no:

> MSYS2 UCRT64 terminal

Ou então usar o caminho completo:

```bash
C:\msys64\ucrt64\bin\cmake.exe -S . -B build -G "MinGW Makefiles"
```

---

# ✅ SOLUÇÃO 2 — Instalar CMake global no Windows

Se quiser instalar no Windows inteiro:

1. Baixe em:
   [https://cmake.org/download/](https://cmake.org/download/)

2. Durante instalação marque:
   ✔ Add CMake to system PATH

Depois reinicie o VS Code.

---

# 🔥 O jeito mais limpo (recomendado para você)

Como você já está usando:

```
C:\msys64\ucrt64\bin\gcc.exe
```

O ideal é:

✔ Instalar CMake pelo MSYS2
✔ Sempre usar o terminal UCRT64
✔ Buildar por lá

---

Boa — CMake instalado ✅ (`4.2.1`). Agora falta só **configurar e buildar do jeito certo** (e evitar aquele “build active file” que compila só o `main.c`).

Vou assumir que seus arquivos estão em:
`C:\Users\emers\OneDrive\Documentos\ESTACIO\superTrunfoTema2\game\`

---

## 1) Crie o `CMakeLists.txt` dentro da pasta `game/`

Crie o arquivo:

`.../superTrunfoTema2/game/CMakeLists.txt`

Com este conteúdo:

```cmake
cmake_minimum_required(VERSION 3.20)
project(jogo_raylib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    main.c
    game.c
    input.c
)

# Headers locais (game.h, input.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Raylib do MSYS2
find_package(raylib CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
```

---

## 2) Build (no terminal **UCRT64**)

Abra o terminal **UCRT64** e rode:

```bash
cd "/c/Users/emers/OneDrive/Documentos/ESTACIO/superTrunfoTema2/game"
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/jogo_raylib.exe
```

> Repare que no MSYS2 o caminho vira `/c/Users/...`

---

## 3) Se der erro “não acha raylib” (bem comum)

Aí é porque o CMake não está achando o “pacote” da raylib (o arquivo `raylibConfig.cmake`).
Nesse caso, a solução mais “à prova de bala” é linkar via `pkg-config`.

### Instale pkg-config (se não tiver):

```bash
pacman -S mingw-w64-ucrt-x86_64-pkgconf
```

### Troque o trecho do CMakeLists por este (substitui `find_package(raylib...)`):

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)

target_include_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARIES})

# libs extras no Windows (normalmente já vêm, mas ajuda)
if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE opengl32 gdi32 winmm)
endif()
```

Depois rode os comandos do build de novo.

---

## 4) No VS Code: pare de usar “gcc build active file”

Quando você clica no build padrão, ele faz:

* compila só `main.c`
* não linka `game.c`, `input.c`
* não linka raylib

Com CMake, você usa a extensão **CMake Tools** (recomendado) ou roda no terminal UCRT64.

---