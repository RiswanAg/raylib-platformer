#pragma once
#include "raylib.h"
#include <vector>
#include <string>

#include "entities.h"
#include "level.h"
#include "editor.h"

enum class GameState { MENU, PLAYING, EDITOR, PAUSED, GAMEOVER, WIN };
enum class PlayMode  { CAMPAIGN, CUSTOM };

struct Game {
    GameState state = GameState::MENU;
    PlayMode  mode  = PlayMode::CAMPAIGN;

    // levels
    std::vector<Level> campaign;
    int campaignIndex = 0;

    Level custom{};
    std::string customPath; // next to exe

    Level* cur = nullptr;

    // entities
    Player player{};
    std::vector<Bullet> bullets;

    // camera
    Camera2D cam{};
    EditorUI editorUI{};

    // editor camera (fly cam)
    Vector2 editorCamPos{ 0, 0 };
    float   editorZoom = 1.0f;

    // pause
    bool paused = false;
    int  pauseChoice = 0;

    // HUD message
    float msgTimer = 0.0f;
    char  msg[160] = {};

    // audio
    Music bgm{};
    Sound sJump{};
    Sound sShoot{};
    Sound sHit{};

    void Init();
    void Shutdown();
    void ResetToLevel(Level& L);
    void Update(float dt);
    void Draw();
};
