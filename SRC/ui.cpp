#include "ui.h"
#include "raylib.h"
#include "config.h"

#ifndef DEV_MODE
#define DEV_MODE 0
#endif

static void ShadowText(const char* t, int x, int y, int s, Color c) {
    DrawText(t, x+2, y+2, s, Fade(BLACK, 0.6f));
    DrawText(t, x, y, s, c);
}

void DrawMenuUI(const Game& g) {
    ShadowText("2D PLATFORMER (raylib)", 70, 80, 50, RAYWHITE);

    ShadowText("ENTER = Play Campaign", 70, 170, 28, RAYWHITE);
    ShadowText("C     = Play Custom",   70, 210, 28, RAYWHITE);
    ShadowText("TAB   = Edit Custom",   70, 250, 28, RAYWHITE);

#if DEV_MODE
    ShadowText("E     = Edit Campaign (DEV)", 70, 290, 28, YELLOW);
    ShadowText("Campaign saves: campaign_01.txt / campaign_02.txt", 70, 330, 20, GRAY);
#endif

    // Group Members Section
    ShadowText("GROUP MEMBERS:", 70, 420, 24, SKYBLUE);
    ShadowText("1. HMAM NOORELDEEN ABDELLA ALTOOM", 90, 455, 20, RAYWHITE);
    ShadowText("2. DANISH IRSYAD BIN MUHAMMAD RIDHA", 90, 480, 20, RAYWHITE);
    ShadowText("3. RISWAN BIN HAMUA", 90, 505, 20, RAYWHITE);
    ShadowText("4. MOHAMMAD HAFIZ BIN IDRIS", 90, 530, 20, RAYWHITE);

    if (g.msgTimer > 0.0f && g.msg[0] != '\0') {
        ShadowText(g.msg, 70, 590, 22, YELLOW);
    }
}

void DrawPauseUI(const Game& g, int /*screenW*/, int /*screenH*/) {
    ShadowText("PAUSED", 70, 70, 60, RAYWHITE);

    const char* items[3] = {"Resume","Main Menu","Quit Game"};
    int y = 160;
    for (int i = 0; i < 3; i++) {
        Color c = (i == g.pauseChoice) ? YELLOW : RAYWHITE;
        ShadowText(items[i], 90, y, 34, c);
        y += 52;
    }

    ShadowText("W/S or Up/Down then ENTER", 70, 360, 20, GRAY);
    ShadowText("P = toggle pause",         70, 385, 20, GRAY);
}

void DrawEndUI(const Game& g) {
    if (g.state == GameState::GAMEOVER) {
        ShadowText("GAME OVER", 60, 60, 52, RED);
        ShadowText("R = Restart | M = Menu", 60, 120, 24, RAYWHITE);
    } else if (g.state == GameState::WIN) {
        ShadowText("YOU WIN!", 60, 60, 52, GOLD);
        ShadowText("M = Menu", 60, 120, 24, RAYWHITE);
    }
}