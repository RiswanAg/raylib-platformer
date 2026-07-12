#include "player_system.h"
#include <cmath>

static constexpr float MOVE_SPEED = 300.0f;
static constexpr float JUMP_VEL   = -650.0f;

void PlayerUpdateInput(Player& p, float /*dt*/) {
    float move = 0.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move += 1.0f;

    p.vel.x = move * MOVE_SPEED;
    if (move < 0) p.facingRight = false;
    if (move > 0) p.facingRight = true;

    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && p.onGround) {
        p.vel.y = JUMP_VEL;
    }
}

void PlayerApplyGravity(Player& p, float dt, float gravity) {
    p.vel.y += gravity * dt;
}

// FIXED COLLISION (center-based pos like your old working file)
void PlayerResolveCollisions(const Level& L, Player& p, float dt) {
    const float halfW = 14.0f; // width 28 / 2
    const float halfH = 20.0f; // height 40 / 2

    // X
    p.pos.x += p.vel.x * dt;
    Rectangle r = p.Rect();

    int left   = (int)floorf(r.x / TILE);
    int right  = (int)floorf((r.x + r.width) / TILE);
    int top    = (int)floorf(r.y / TILE);
    int bottom = (int)floorf((r.y + r.height) / TILE);

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (!IsSolid(GetTile(L, tx, ty))) continue;
            Rectangle tr = TileRect(tx, ty);

            if (CheckCollisionRecs(r, tr)) {
                if (p.vel.x > 0) {
                    // hit tile left side -> place player center halfW away
                    p.pos.x = tr.x - halfW;
                } else if (p.vel.x < 0) {
                    // hit tile right side
                    p.pos.x = tr.x + tr.width + halfW;
                }
                p.vel.x = 0;
                r = p.Rect();
            }
        }
    }

    // Y
    p.pos.y += p.vel.y * dt;
    r = p.Rect();

    left   = (int)floorf(r.x / TILE);
    right  = (int)floorf((r.x + r.width) / TILE);
    top    = (int)floorf(r.y / TILE);
    bottom = (int)floorf((r.y + r.height) / TILE);

    p.onGround = false;

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (!IsSolid(GetTile(L, tx, ty))) continue;
            Rectangle tr = TileRect(tx, ty);

            if (CheckCollisionRecs(r, tr)) {
                if (p.vel.y > 0) {
                    // land on tile
                    p.pos.y = tr.y - halfH;
                    p.vel.y = 0;
                    p.onGround = true;
                } else if (p.vel.y < 0) {
                    // hit underside
                    p.pos.y = tr.y + tr.height + halfH;
                    p.vel.y = 0;
                }
                r = p.Rect();
            }
        }
    }
}
