#include "editor.h"
#include "entities.h"
#include <cmath>
#include <string>
#include <algorithm>
#include <cstdio>

static void MouseToTile(const Camera2D& cam, int& outTx, int& outTy)
{
    Vector2 w = GetScreenToWorld2D(GetMousePosition(), cam);
    outTx = (int)floorf(w.x / (float)TILE);
    outTy = (int)floorf(w.y / (float)TILE);
}

static bool InBounds(const Level& L, int tx, int ty)
{
    return tx >= 0 && ty >= 0 && tx < L.width && ty < L.height;
}

static float DistSq(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

static void EnsureTilesShape(Level& L, char emptyChar)
{
    // If tiles already exist, keep as much as possible.
    if ((int)L.tiles.size() != L.height)
    {
        int newH = (int)L.tiles.size();
        L.height = newH;
        L.width  = (newH > 0) ? (int)L.tiles[0].size() : 0;
    }

    // If width/height are 0 but tiles exist, derive them.
    if (L.height == 0) L.height = (int)L.tiles.size();
    if (L.width == 0 && L.height > 0) L.width = (int)L.tiles[0].size();

    if (L.height <= 0 || L.width <= 0) return;

    // Normalize all rows to width
    for (int y = 0; y < (int)L.tiles.size(); y++)
    {
        if ((int)L.tiles[y].size() < L.width) L.tiles[y].append(L.width - (int)L.tiles[y].size(), emptyChar);
        if ((int)L.tiles[y].size() > L.width) L.tiles[y].resize(L.width);
    }

    // If tiles vector size doesn't match height, fix it
    if ((int)L.tiles.size() < L.height)
        L.tiles.resize(L.height, std::string(L.width, emptyChar));
    if ((int)L.tiles.size() > L.height)
        L.tiles.resize(L.height);
}

static void SetTileChar(Level& L, int tx, int ty, char c, char emptyChar)
{
    EnsureTilesShape(L, emptyChar);
    if (!InBounds(L, tx, ty)) return;
    L.tiles[ty][tx] = c;
}

static int FindEnemyAt(Level& L, Vector2 worldPos)
{
    for (int i = (int)L.enemies.size() - 1; i >= 0; --i)
    {
        Rectangle r = L.enemies[i].Rect();
        // easier to hit
        r.x -= 6; r.y -= 6; r.width += 12; r.height += 12;
        if (CheckCollisionPointRec(worldPos, r)) return i;
    }
    return -1;
}

static const char* ToolName(EditorTool t)
{
    switch (t)
    {
        case EditorTool::SOLID:  return "1: Solid (#)";
        case EditorTool::LAVA:   return "2: Lava (H)";
        case EditorTool::PATROL: return "3: Enemy Patrol";
        case EditorTool::CHASE:  return "4: Enemy Chase";
        case EditorTool::SPAWN:  return "5: Spawn";
        case EditorTool::FINISH: return "6: Finish";
        default: return "Unknown";
    }
}

void EditorUpdate(Level& L, const Camera2D& cam, EditorUI& ui)
{
    EnsureTilesShape(L, ui.emptyChar);

    // tool selection (press once)
    if (IsKeyPressed(KEY_ONE))   ui.tool = EditorTool::SOLID;
    if (IsKeyPressed(KEY_TWO))   ui.tool = EditorTool::LAVA;
    if (IsKeyPressed(KEY_THREE)) ui.tool = EditorTool::PATROL;
    if (IsKeyPressed(KEY_FOUR))  ui.tool = EditorTool::CHASE;
    if (IsKeyPressed(KEY_FIVE))  ui.tool = EditorTool::SPAWN;
    if (IsKeyPressed(KEY_SIX))   ui.tool = EditorTool::FINISH;

    if (IsKeyPressed(KEY_G)) ui.showGrid = !ui.showGrid;

    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), cam);
    int tx = 0, ty = 0;
    MouseToTile(cam, tx, ty);

    bool lmbDown    = IsMouseButtonDown(MOUSE_BUTTON_LEFT);     // drag paint
    bool lmbPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);  // click once

    // RMB delete ANYTHING
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        // 1) delete enemy
        int idx = FindEnemyAt(L, mouseWorld);
        if (idx >= 0) L.enemies.erase(L.enemies.begin() + idx);

        // 2) clear tile (solid/lava/etc) -> empty '.'
        if (InBounds(L, tx, ty)) SetTileChar(L, tx, ty, ui.emptyChar, ui.emptyChar);

        // 3) clear spawn if close
        if (DistSq(mouseWorld, L.spawn) < (20.0f * 20.0f))
        {
            // move to a default safe spot
            L.spawn = { 64, 64 };
        }

        // 4) clear finish if clicked inside
        if (CheckCollisionPointRec(mouseWorld, L.finishRect))
        {
            // park off-map
            L.finishRect = { -9999.0f, -9999.0f, (float)TILE, (float)TILE };
        }
    }

    // HOLD LMB = paint tiles/lava continuously
    if (ui.tool == EditorTool::SOLID && lmbDown)
    {
        if (InBounds(L, tx, ty)) SetTileChar(L, tx, ty, ui.solidChar, ui.emptyChar); // '#'
        return;
    }

    if (ui.tool == EditorTool::LAVA && lmbDown)
    {
        if (InBounds(L, tx, ty)) SetTileChar(L, tx, ty, ui.lavaChar, ui.emptyChar); // 'H'
        return;
    }

    // Click once tools
    if (!lmbPressed) return;

    if (ui.tool == EditorTool::SPAWN)
    {
        if (!InBounds(L, tx, ty)) return;
        L.spawn = { tx * (float)TILE + TILE/2.0f, ty * (float)TILE + TILE/2.0f };
        return;
    }

    if (ui.tool == EditorTool::FINISH)
    {
        if (!InBounds(L, tx, ty)) return;
        L.finishRect = { tx * (float)TILE, ty * (float)TILE, (float)TILE, (float)TILE };
        return;
    }

    if (ui.tool == EditorTool::PATROL || ui.tool == EditorTool::CHASE)
    {
        if (!InBounds(L, tx, ty)) return;

        Enemy e{};
        e.pos  = { tx * (float)TILE + TILE/2.0f, ty * (float)TILE + TILE/2.0f };
        e.alive = true;

        if (ui.tool == EditorTool::PATROL)
        {
            e.type = EnemyType::PATROL;
            e.speed = 120.0f;
            e.vel  = { e.speed, 0.0f };
            e.leftBound  = std::max(0, tx - 6) * (float)TILE + TILE/2.0f;
            e.rightBound = std::min(L.width-1, tx + 6) * (float)TILE + TILE/2.0f;
        }
        else
        {
            e.type = EnemyType::CHASE;
            e.speed = 170.0f;
            e.vel   = { 0.0f, 0.0f };
            e.aggroRange = 320.0f;
        }

        L.enemies.push_back(e);
        return;
    }
}

void EditorDrawOverlay(const Level& L, const Camera2D& cam, const EditorUI& ui)
{
    DrawText("EDITOR MODE", 20, 20, 28, RAYWHITE);
    DrawText("Hold LMB: paint tiles/lava | Click: enemies/spawn/finish | RMB: delete ANYTHING", 20, 55, 18, GRAY);
    DrawText("CTRL+S save | CTRL+L load | ESC menu", 20, 75, 18, GRAY);

    DrawText("TOOLS:", 20, 105, 18, RAYWHITE);
    DrawText("1 = Solid Tile (#)",  20, 125, 18, (ui.tool == EditorTool::SOLID)  ? YELLOW : GRAY);
    DrawText("2 = Lava (H)",        20, 145, 18, (ui.tool == EditorTool::LAVA)   ? YELLOW : GRAY);
    DrawText("3 = Enemy Patrol",    20, 165, 18, (ui.tool == EditorTool::PATROL) ? YELLOW : GRAY);
    DrawText("4 = Enemy Chase",     20, 185, 18, (ui.tool == EditorTool::CHASE)  ? YELLOW : GRAY);
    DrawText("5 = Spawn",           20, 205, 18, (ui.tool == EditorTool::SPAWN)  ? YELLOW : GRAY);
    DrawText("6 = Finish",          20, 225, 18, (ui.tool == EditorTool::FINISH) ? YELLOW : GRAY);

    std::string cur = std::string("Selected: ") + ToolName(ui.tool);
    DrawText(cur.c_str(), 20, 255, 18, YELLOW);

    // World markers (always visible)
    BeginMode2D(cam);

    // Spawn marker
    DrawCircleV(L.spawn, 10.0f, SKYBLUE);
    DrawLineV({L.spawn.x - 14, L.spawn.y}, {L.spawn.x + 14, L.spawn.y}, SKYBLUE);
    DrawLineV({L.spawn.x, L.spawn.y - 14}, {L.spawn.x, L.spawn.y + 14}, SKYBLUE);

    // Finish marker
    DrawRectangleLinesEx(L.finishRect, 2.0f, GOLD);

    // Enemy markers
    for (const auto& e : L.enemies)
    {
        Rectangle r = e.Rect();
        Color c = (e.type == EnemyType::PATROL) ? ORANGE : PURPLE;
        DrawRectangleRec(r, c);
        DrawRectangleLinesEx(r, 2.0f, WHITE);

        if (e.type == EnemyType::PATROL)
            DrawLine((int)e.leftBound, (int)e.pos.y, (int)e.rightBound, (int)e.pos.y, c);
    }

    EndMode2D();

    char buf[128];
    std::snprintf(buf, sizeof(buf), "Tile codes: SOLID='%c'  LAVA='%c'  EMPTY='%c'",
                  ui.solidChar, ui.lavaChar, ui.emptyChar);
    DrawText(buf, 20, 285, 18, GRAY);
}
