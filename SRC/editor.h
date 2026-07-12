#pragma once
#include "level.h"
#include "raylib.h"

enum class EditorTool : int
{
    SOLID   = 1,  
    LAVA    = 2,  
    PATROL  = 3,
    CHASE   = 4,
    SPAWN   = 5,
    FINISH  = 6
};

struct EditorUI
{
    EditorTool tool = EditorTool::SOLID;
    bool showGrid = true;

    // Match My Level system exactly
    char solidChar = '#';
    char lavaChar  = 'H';
    char emptyChar = '.';
};

void EditorUpdate(Level& L, const Camera2D& cam, EditorUI& ui);
void EditorDrawOverlay(const Level& L, const Camera2D& cam, const EditorUI& ui);
