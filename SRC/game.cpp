#include "game.h"
#include "config.h"

#ifndef DEV_MODE
#error "DEV_MODE is not defined. Put #define DEV_MODE 1 or 0 in SRC/config.h"
#endif

#if (DEV_MODE != 0)
    #define DEV_ENABLED 1
#else
    #define DEV_ENABLED 0
#endif

#include "level_io.h"
#include "editor.h"

#include "player_system.h"
#include "bullet_system.h"
#include "enemy_system.h"
#include "ui.h"

#include <cstring>
#include <algorithm>
#include <string>

static constexpr int   SCREEN_W = 1280;
static constexpr int   SCREEN_H = 720;

static constexpr float GRAVITY  = 1800.0f;
static constexpr float BULLET_SPEED = 900.0f;
static constexpr float BULLET_LIFE  = 1.10f;
static constexpr float CAM_LERP = 10.0f;

// Campaign limit (safe + clean)
static constexpr int CAMPAIGN_MAX_LEVELS = 4;

static bool CtrlDown() {
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

static void SetMsg(Game& g, const char* text, float seconds = 4.0f) {
    if (!text) text = "";
    std::strncpy(g.msg, text, sizeof(g.msg) - 1);
    g.msg[sizeof(g.msg) - 1] = '\0';
    g.msgTimer = seconds;
}

static std::string AppFile(const char* name) {
    return std::string(GetApplicationDirectory()) + name;
}

static std::string CampaignFilePath(int idx) {
    // idx: 0..3 -> campaign_01..campaign_04
    char buf[64];
    std::snprintf(buf, sizeof(buf), "campaign_%02d.txt", idx + 1);
    return AppFile(buf);
}

static void EditorCameraFly(Game& g, float dt) {
    float speed = 750.0f * dt / std::max(0.25f, g.editorZoom);

    if (IsKeyDown(KEY_W)) g.editorCamPos.y -= speed;
    if (IsKeyDown(KEY_S)) g.editorCamPos.y += speed;
    if (IsKeyDown(KEY_A)) g.editorCamPos.x -= speed;
    if (IsKeyDown(KEY_D)) g.editorCamPos.x += speed;

    if (CtrlDown()) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            g.editorZoom *= (wheel > 0 ? 1.1f : 0.9f);
            g.editorZoom = std::clamp(g.editorZoom, 0.25f, 3.0f);
        }
    }

    g.cam.target = g.editorCamPos;
    g.cam.zoom   = g.editorZoom;
}

static void EnsureCampaignSize(Game& g) {
    if ((int)g.campaign.size() < CAMPAIGN_MAX_LEVELS) {
        // Build base templates:
        // 1 -> MakeLevel1, 2 -> MakeLevel2, 3/4 -> copies of MakeLevel2 (template)
        g.campaign.clear();
        g.campaign.reserve(CAMPAIGN_MAX_LEVELS);
        g.campaign.push_back(MakeLevel1());
        g.campaign.push_back(MakeLevel2());
        for (int i = 2; i < CAMPAIGN_MAX_LEVELS; i++) g.campaign.push_back(MakeLevel2());
    }
}

static void LoadCampaignEdits(Game& g) {
    EnsureCampaignSize(g);

    std::string err;
    for (int i = 0; i < CAMPAIGN_MAX_LEVELS; i++) {
        std::string path = CampaignFilePath(i);
        if (FileExists(path.c_str())) {
            LoadLevelFromFile(g.campaign[i], path.c_str(), &err);
            err.clear();
        }
    }
}

static void RebuildCampaign(Game& g) {
    EnsureCampaignSize(g);
    LoadCampaignEdits(g);
}

static void EnterEditor(Game& g, bool editingCampaign, int campaignIdx) {
    g.paused = false;

    if (editingCampaign) {
        g.mode = PlayMode::CAMPAIGN;
        g.campaignIndex = std::clamp(campaignIdx, 0, CAMPAIGN_MAX_LEVELS - 1);
        g.cur = &g.campaign[g.campaignIndex];
        SetMsg(g, "DEV Campaign Editor: N next | B back | CTRL+S save | CTRL+L load | M menu", 8.0f);
    } else {
        g.mode = PlayMode::CUSTOM;
        g.cur  = &g.custom;
        SetMsg(g, "Custom Editor: CTRL+S save custom_01.txt | CTRL+L load | M menu", 6.0f);
    }

    g.state = GameState::EDITOR;
    g.editorCamPos = g.cur->spawn;
    g.cam.target   = g.editorCamPos;
    g.cam.zoom     = g.editorZoom;
}

void Game::Init() {
    cam.offset = { SCREEN_W * 0.5f, SCREEN_H * 0.5f };
    cam.target = { 0, 0 };
    cam.rotation = 0;
    cam.zoom = 1.0f;

    editorCamPos = cam.target;
    editorZoom   = 1.0f;

    RebuildCampaign(*this);
    campaignIndex = 0;

    customPath = AppFile("custom_01.txt");

    bullets.assign(128, Bullet{});

    InitAudioDevice();
    if (FileExists("bgm.mp3"))   bgm    = LoadMusicStream("bgm.mp3");
    if (FileExists("jump.wav"))  sJump  = LoadSound("jump.wav");
    if (FileExists("shoot.wav")) sShoot = LoadSound("shoot.wav");
    if (FileExists("hit.wav"))   sHit   = LoadSound("hit.wav");

    if (bgm.ctxData) {
        SetMusicVolume(bgm, 0.4f);
        PlayMusicStream(bgm);
    }

    state = GameState::MENU;
    mode  = PlayMode::CAMPAIGN;

    paused = false;
    pauseChoice = 0;

    msgTimer = 0;
    msg[0] = '\0';

    editorUI = {};

    if (DEV_ENABLED)
        SetMsg(*this, "DEV=1 | ENTER=Campaign | C=Play Custom | TAB=Edit Custom | E=Edit Campaign (4 levels)", 8.0f);
    else
        SetMsg(*this, "DEV=0 | ENTER=Campaign | C=Play Custom | TAB=Edit Custom", 8.0f);
}

void Game::Shutdown() {
    if (bgm.ctxData) { StopMusicStream(bgm); UnloadMusicStream(bgm); }
    if (sJump.frameCount)  UnloadSound(sJump);
    if (sShoot.frameCount) UnloadSound(sShoot);
    if (sHit.frameCount)   UnloadSound(sHit);
    CloseAudioDevice();
}

void Game::ResetToLevel(Level& L) {
    cur = &L;

    player = {};
    player.pos = cur->spawn;
    player.alive = true;

    for (auto& e : cur->enemies) {
        e.alive = true;
        e.vel = { 0,0 };
        e.onGround = false;
        e.jumpCooldown = 0;
        if (e.type == EnemyType::CHASE) e.hp = 20;
    }

    for (auto& b : bullets) b.active = false;

    cam.target = player.pos;
    editorCamPos = cam.target;
    state = GameState::PLAYING;
}

void Game::Update(float dt) {
    dt = std::min(dt, 1.0f / 30.0f);

    if (bgm.ctxData) UpdateMusicStream(bgm);
    if (msgTimer > 0) msgTimer -= dt;

    // DEV HOTKEY: open campaign editor anytime
    if (IsKeyPressed(KEY_E)) {
        if (!DEV_ENABLED) {
            SetMsg(*this, "E pressed but DEV=0 (set DEV_MODE=1 in SRC/config.h then rebuild)", 6.0f);
        } else {
            RebuildCampaign(*this);
            EnterEditor(*this, true, 0);
        }
        return;
    }

    // ===== MENU =====
    if (state == GameState::MENU) {
        if (IsKeyPressed(KEY_ENTER)) {
            mode = PlayMode::CAMPAIGN;
            campaignIndex = 0;
            RebuildCampaign(*this);
            ResetToLevel(campaign[campaignIndex]);
        }

        if (IsKeyPressed(KEY_C)) {
            mode = PlayMode::CUSTOM;
            std::string err;
            if (LoadLevelFromFile(custom, customPath.c_str(), &err)) ResetToLevel(custom);
            else SetMsg(*this, "No custom_01.txt found. Press TAB to create.", 6.0f);
        }

        if (IsKeyPressed(KEY_TAB)) {
            mode = PlayMode::CUSTOM;

            std::string err;
            if (FileExists(customPath.c_str())) {
                if (!LoadLevelFromFile(custom, customPath.c_str(), &err)) {
                    custom = MakeLevel1();
                    SetMsg(*this, "Custom load failed -> template. CTRL+S to save.", 6.0f);
                } else {
                    SetMsg(*this, "Editing custom_01.txt | CTRL+S save | M menu", 6.0f);
                }
            } else {
                custom = MakeLevel1();
                SetMsg(*this, "New custom level (template). CTRL+S to save.", 6.0f);
            }

            EnterEditor(*this, false, 0);
            return;
        }

        return;
    }

    // ===== EDITOR =====
    if (state == GameState::EDITOR) {
        // Menu
        if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
            state = GameState::MENU;
            if (DEV_ENABLED)
                SetMsg(*this, "DEV=1 | ENTER=Campaign | C=Play Custom | TAB=Edit Custom | E=Edit Campaign (4 levels)", 8.0f);
            else
                SetMsg(*this, "DEV=0 | ENTER=Campaign | C=Play Custom | TAB=Edit Custom", 8.0f);
            return;
        }

        // Campaign navigation while editing (N/B + [ ])
        if (DEV_ENABLED && mode == PlayMode::CAMPAIGN) {
            bool next = IsKeyPressed(KEY_N) || IsKeyPressed(KEY_RIGHT_BRACKET);
            bool prev = IsKeyPressed(KEY_B) || IsKeyPressed(KEY_LEFT_BRACKET);

            if (next) {
                if (campaignIndex < CAMPAIGN_MAX_LEVELS - 1) {
                    campaignIndex++;
                    cur = &campaign[campaignIndex];
                    editorCamPos = cur->spawn;
                    SetMsg(*this, "Next campaign level", 1.5f);
                } else {
                    SetMsg(*this, "Already at last campaign level (4)", 2.5f);
                }
            }

            if (prev) {
                if (campaignIndex > 0) {
                    campaignIndex--;
                    cur = &campaign[campaignIndex];
                    editorCamPos = cur->spawn;
                    SetMsg(*this, "Previous campaign level", 1.5f);
                } else {
                    SetMsg(*this, "Already at first campaign level", 2.5f);
                }
            }
        }

        EditorCameraFly(*this, dt);
        EditorUpdate(*cur, cam, editorUI);

        // Save / Load
        if (CtrlDown() && IsKeyPressed(KEY_S)) {
            bool ok = false;

            if (mode == PlayMode::CUSTOM) {
                ok = SaveLevelToFile(*cur, customPath.c_str());
                SetMsg(*this, ok ? "Saved custom_01.txt" : "Save failed", 3.0f);
            } else {
                std::string path = CampaignFilePath(campaignIndex);
                ok = SaveLevelToFile(*cur, path.c_str());
                SetMsg(*this, ok ? "Saved campaign file" : "Save failed", 3.0f);
            }
        }

        if (CtrlDown() && IsKeyPressed(KEY_L)) {
            std::string err;
            bool ok = false;

            if (mode == PlayMode::CUSTOM) {
                ok = LoadLevelFromFile(*cur, customPath.c_str(), &err);
                SetMsg(*this, ok ? "Loaded custom_01.txt" : ("Load failed: " + err).c_str(), 5.0f);
            } else {
                std::string path = CampaignFilePath(campaignIndex);
                ok = LoadLevelFromFile(*cur, path.c_str(), &err);
                SetMsg(*this, ok ? "Loaded campaign file" : ("Load failed: " + err).c_str(), 5.0f);
            }
        }

        return;
    }

    // ===== PAUSE =====
    if (state == GameState::PLAYING && IsKeyPressed(KEY_P)) {
        paused = !paused;
        pauseChoice = 0;
    }

    if (paused) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))   pauseChoice = std::max(pauseChoice - 1, 0);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) pauseChoice = std::min(pauseChoice + 1, 2);

        if (IsKeyPressed(KEY_ENTER)) {
            if (pauseChoice == 0) paused = false;
            else if (pauseChoice == 1) { paused = false; state = GameState::MENU; }
            else if (pauseChoice == 2) CloseWindow();
        }
        return;
    }

    // ===== GAME OVER / WIN =====
    if (state == GameState::GAMEOVER || state == GameState::WIN) {
        if (IsKeyPressed(KEY_R)) {
            if (mode == PlayMode::CAMPAIGN) {
                RebuildCampaign(*this);
                ResetToLevel(campaign[campaignIndex]);
            } else {
                std::string err;
                if (LoadLevelFromFile(custom, customPath.c_str(), &err)) ResetToLevel(custom);
                else state = GameState::MENU;
            }
        }

        if (IsKeyPressed(KEY_M)) state = GameState::MENU;
        return;
    }

    // ===== GAMEPLAY =====
    PlayerUpdateInput(player, dt);
    PlayerApplyGravity(player, dt, GRAVITY);
    PlayerResolveCollisions(*cur, player, dt);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetScreenToWorld2D(GetMousePosition(), cam);
        BulletFire(bullets, player.pos, { m.x - player.pos.x, m.y - player.pos.y },
                   BULLET_SPEED, BULLET_LIFE);
        if (sShoot.frameCount) PlaySound(sShoot);
    }

    BulletsUpdate(*cur, bullets, dt);

    if (EnemiesUpdate(*cur, player, bullets, dt).playerKilled) {
        state = GameState::GAMEOVER;
        return;
    }

    if (TouchingHazard(*cur, player.Rect()))
        state = GameState::GAMEOVER;

    if (CheckCollisionRecs(player.Rect(), cur->finishRect)) {
        if (mode == PlayMode::CAMPAIGN && ++campaignIndex < CAMPAIGN_MAX_LEVELS)
            ResetToLevel(campaign[campaignIndex]);
        else
            state = GameState::WIN;
    }

    cam.target.x += (player.pos.x - player.pos.x) * 0.0f; // keep x handled by lerp below (no-op but safe)
    cam.target.x += (player.pos.x - cam.target.x) * std::min(1.0f, CAM_LERP * dt);
    cam.target.y = player.pos.y;
}

void Game::Draw() {
    DrawText(DEV_ENABLED ? "DEV=1" : "DEV=0", 10, 10, 20, DEV_ENABLED ? GREEN : RED);

    if (state == GameState::MENU) { DrawMenuUI(*this); return; }
    if (paused) { DrawPauseUI(*this, SCREEN_W, SCREEN_H); return; }

    BeginMode2D(cam);
    DrawLevel(*cur);

    if (state == GameState::PLAYING)
        DrawRectangleRec(player.Rect(), SKYBLUE);

    for (auto& e : cur->enemies)
        if (e.alive) DrawRectangleRec(e.Rect(), e.type == EnemyType::PATROL ? ORANGE : PURPLE);

    for (auto& b : bullets)
        if (b.active) DrawCircleV(b.pos, BULLET_RADIUS, YELLOW);

    EndMode2D();

    if (state == GameState::EDITOR) {
        EditorDrawOverlay(*cur, cam, editorUI);
        DrawText("Campaign: N next | B back | CTRL+S save | CTRL+L load | M menu", 20, 330, 18, GRAY);
        if (mode == PlayMode::CAMPAIGN) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Editing campaign level %d/%d", campaignIndex + 1, CAMPAIGN_MAX_LEVELS);
            DrawText(buf, 20, 355, 18, YELLOW);
        }
    } else {
        DrawEndUI(*this);
    }

    if (msgTimer > 0 && msg[0] != '\0') DrawText(msg, 20, 100, 22, YELLOW);
}
