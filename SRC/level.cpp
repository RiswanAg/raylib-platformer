#include "level.h"
#include <cmath>
#include <algorithm>

bool IsSolid(char c) { return c == '#'; }
bool IsHazard(char c) { return c == 'H'; }

char GetTile(const Level& L, int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= L.width || ty >= L.height) return '#';
    return L.tiles[ty][tx];
}

Rectangle TileRect(int tx, int ty) {
    return {(float)(tx * TILE), (float)(ty * TILE), (float)TILE, (float)TILE};
}

static void BuildLevel(Level& L) {
    L.height = (int)L.tiles.size();
    L.width  = L.height > 0 ? (int)L.tiles[0].size() : 0;
    L.enemies.clear();
    L.spawn = { 64, 64 };
    L.finishRect = { (float)(L.width*TILE - 2*TILE), (float)(2*TILE), (float)TILE, (float)TILE };

    for (int y = 0; y < L.height; y++) {
        for (int x = 0; x < L.width; x++) {
            char c = L.tiles[y][x];
            if (c == 'S') {
                L.spawn = { x * (float)TILE + TILE/2.0f, y * (float)TILE + TILE/2.0f };
                L.tiles[y][x] = '.';
            } else if (c == 'F') {
                L.finishRect = { x * (float)TILE, y * (float)TILE, (float)TILE, (float)TILE };
                L.tiles[y][x] = '.';
            } else if (c == 'P') {
                Enemy e;
                e.type = EnemyType::PATROL;
                e.pos  = { x * (float)TILE + TILE/2.0f, y * (float)TILE + TILE/2.0f };
                e.vel  = { e.speed, 0 };
                e.leftBound  = std::max(0, x - 6) * (float)TILE + TILE/2.0f;
                e.rightBound = std::min(L.width-1, x + 6) * (float)TILE + TILE/2.0f;
                L.enemies.push_back(e);
                L.tiles[y][x] = '.';
            } else if (c == 'C') {
                Enemy e;
                e.type = EnemyType::CHASE;
                e.pos  = { x * (float)TILE + TILE/2.0f, y * (float)TILE + TILE/2.0f };
                e.vel  = { 0, 0 };
                e.speed = 170;
                e.aggroRange = 320;
                L.enemies.push_back(e);
                L.tiles[y][x] = '.';
            }
        }
    }
}

Level MakeLevel1() {
    Level L;
    L.tiles = {
        "############################################################",
        "#..........................................................#",
        "#.........................C................................#",
        "#..........................................................#",
        "#............#####.........................................#",
        "#....................................................F.....#",
        "#..........................................................#",
        "#..S...............####..................P.................#",
        "#.........................####.............................#",
        "#..........................................................#",
        "#..........................................................#",
        "#..........................................................#",
        "#..........................................................#",
        "#..........................................................#",
        "############################################################",
        "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
    };
    BuildLevel(L);
    return L;
}

Level MakeLevel2() {
    Level L;
    L.tiles = {
        "############################################################",
        "#..........................................................#",
        "#..............#####.......................................#",
        "#..................#............................C..........#",
        "#..S...............#.......................................#",
        "#..................#.............#####.....................#",
        "#..................#............................P..........#",
        "#..................#.......................................#",
        "#..........###########################.....................#",
        "#..........................................................#",
        "#.....................................................F....#",
        "#..........................................................#",
        "#..........................................................#",
        "#..........................................................#",
        "############################################################",
        "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"
    };
    BuildLevel(L);
    return L;
}

void DrawLevel(const Level& L) {
    for (int y = 0; y < L.height; y++) {
        for (int x = 0; x < L.width; x++) {
            char c = L.tiles[y][x];
            Rectangle tr = TileRect(x, y);
            if (c == '#') DrawRectangleRec(tr, DARKGRAY);
            else if (c == 'H') DrawRectangleRec(tr, RED);
        }
    }
    DrawRectangleRec(L.finishRect, GOLD);
}

bool TouchingHazard(const Level& L, const Rectangle& r) {
    int left   = (int)floorf(r.x / TILE);
    int right  = (int)floorf((r.x + r.width) / TILE);
    int top    = (int)floorf(r.y / TILE);
    int bottom = (int)floorf((r.y + r.height) / TILE);

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (IsHazard(GetTile(L, tx, ty))) {
                if (CheckCollisionRecs(r, TileRect(tx, ty))) return true;
            }
        }
    }
    return false;
}
