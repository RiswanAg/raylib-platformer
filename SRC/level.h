#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include "entities.h"

struct Level {
    std::vector<std::string> tiles; // ASCII tilemap
    int width = 0;
    int height = 0;
    Vector2 spawn{};
    Rectangle finishRect{};
    std::vector<Enemy> enemies;
};

Level MakeLevel1();
Level MakeLevel2();
void DrawLevel(const Level& L);

char GetTile(const Level& L, int tx, int ty);
bool IsSolid(char c);
bool IsHazard(char c);
Rectangle TileRect(int tx, int ty);
bool TouchingHazard(const Level& L, const Rectangle& r);
