#include "raylib.h"
#include "game.h"

int main() {
    const int SCREEN_W = 1280;
    const int SCREEN_H = 720;

    InitWindow(SCREEN_W, SCREEN_H, "Raylib Platformer");
    SetTargetFPS(60);

    Game game;
    game.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        game.Update(dt);

        BeginDrawing();
        ClearBackground(BLACK);
        game.Draw();
        EndDrawing();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
