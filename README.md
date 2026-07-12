# Raylib Platformer

A 2D platformer built in C++ with [raylib](https://www.raylib.com/), made for **BITE 3713 – Multiplatform Game** (UTeM, Semester 1 2025/2026).

Team of 4: Danish Irsyad, Riswan Hamua, Mohammad Hafiz, Hmam Nooreldeen.

## Features

- Player movement & physics (gravity, jump)
- Enemy AI — patrol and chase behaviour, collision detection
- Shooting mechanic (bullet system)
- Hazard tiles (lava / void detection)
- Campaign system with multiple levels loaded from text files
- Built-in level editor (paint tiles, lava, enemy patrol/chase, spawn, finish) with save/load

## Build

Requires [raylib](https://www.raylib.com/) (w64devkit toolchain on Windows):

```
g++ SRC/*.cpp -o game.exe -std=c++17 -I <raylib_include> -L <raylib_lib> -lraylib -lopengl32 -lgdi32 -lwinmm
```

Or use the included VS Code task (`.vscode/tasks.json`) — adjust the raylib paths for your machine.

## Level editor

Set `DEV_MODE 1` in `SRC/config.h` and rebuild to enable the in-game level editor (`E` to toggle).
