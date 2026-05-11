# AtividadesCG

Repositorio para as entregas da Atividade Academica de Computacao Grafica.

## Projeto atual

- Hello3D — cubo e piramide 3D interativos com OpenGL moderno (Core Profile) via GLFW + GLM

## Requisitos (macOS)

- CMake 3.20+
- Xcode Command Line Tools (`xcode-select --install`)
- Git (para FetchContent baixar GLFW e GLM automaticamente)

## Build e execucao

```bash
cmake -S . -B build
cmake --build build
./build/Hello3D
```

> Primeira build baixa GLFW 3.4 e GLM 1.0.1 via CMake FetchContent. Requer conexao com internet.

Para limpar o build:

```bash
rm -rf build
```

## Controles

| Tecla | Acao |
|-------|------|
| `TAB` | Selecionar proximo objeto |
| `X` / `Y` / `Z` | Rotacionar no eixo correspondente |
| `W` / `A` / `S` / `D` | Mover no plano XZ |
| `↑` / `↓` | Mover no eixo Y |
| `]` / `[` | Escala uniforme + / - |
| `Shift` + `X/Y/Z` | Escalar eixo especifico + |
| `Ctrl` + `X/Y/Z` | Escalar eixo especifico - |
| `ESC` | Fechar janela |

## Assets

Modelos OBJ em `assets/`:
- `cube.obj`
- `pyramid.obj`

## Estrutura

```
.
├── CMakeLists.txt
├── src/
│   └── Hello3D.cpp
└── assets/
    ├── cube.obj
    └── pyramid.obj
```
