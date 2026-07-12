#include "enemy_system.h"
#include "bullet_system.h"
#include <cmath>
#include <algorithm>

static constexpr float GRAVITY = 1800.0f;

// Strong chaser tuning
static constexpr int   CHASER_HP       = 20;
static constexpr float CHASER_SPEED    = 190.0f;
static constexpr float CHASER_JUMP_VEL = -620.0f;
static constexpr float CHASER_JUMP_CD  = 0.35f;

static bool CollidesSolidAt(const Level& L, const Rectangle& r) {
    int left   = (int)floorf(r.x / TILE);
    int right  = (int)floorf((r.x + r.width) / TILE);
    int top    = (int)floorf(r.y / TILE);
    int bottom = (int)floorf((r.y + r.height) / TILE);

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (!IsSolid(GetTile(L, tx, ty))) continue;
            if (CheckCollisionRecs(r, TileRect(tx, ty))) return true;
        }
    }
    return false;
}

// FIXED: center-based enemy collision (matches Enemy::Rect())
static void EnemyResolveCollisions2D(const Level& L, Enemy& e, float dt) {
    const float halfW = e.Rect().width * 0.5f;   // 28/2 = 14
    const float halfH = e.Rect().height * 0.5f;  // 36/2 = 18

    // X
    e.pos.x += e.vel.x * dt;
    Rectangle r = e.Rect();

    int left   = (int)floorf(r.x / TILE);
    int right  = (int)floorf((r.x + r.width) / TILE);
    int top    = (int)floorf(r.y / TILE);
    int bottom = (int)floorf((r.y + r.height) / TILE);

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (!IsSolid(GetTile(L, tx, ty))) continue;
            Rectangle tr = TileRect(tx, ty);

            if (CheckCollisionRecs(r, tr)) {
                if (e.vel.x > 0) e.pos.x = tr.x - halfW;
                else if (e.vel.x < 0) e.pos.x = tr.x + tr.width + halfW;
                e.vel.x = 0;
                r = e.Rect();
            }
        }
    }

    // Y
    e.pos.y += e.vel.y * dt;
    r = e.Rect();

    left   = (int)floorf(r.x / TILE);
    right  = (int)floorf((r.x + r.width) / TILE);
    top    = (int)floorf(r.y / TILE);
    bottom = (int)floorf((r.y + r.height) / TILE);

    e.onGround = false;

    for (int ty = top; ty <= bottom; ty++) {
        for (int tx = left; tx <= right; tx++) {
            if (!IsSolid(GetTile(L, tx, ty))) continue;
            Rectangle tr = TileRect(tx, ty);

            if (CheckCollisionRecs(r, tr)) {
                if (e.vel.y > 0) {
                    e.pos.y = tr.y - halfH;
                    e.vel.y = 0;
                    e.onGround = true;
                } else if (e.vel.y < 0) {
                    e.pos.y = tr.y + tr.height + halfH;
                    e.vel.y = 0;
                }
                r = e.Rect();
            }
        }
    }
}

static bool WallAhead(const Level& L, const Enemy& e, float dir) {
    Rectangle base = e.Rect();
    Rectangle probe{};
    probe.width = 6.0f;
    probe.height = base.height * 0.5f;
    probe.y = base.y + base.height * 0.25f;
    probe.x = (dir > 0) ? (base.x + base.width + 1.0f) : (base.x - probe.width - 1.0f);
    return CollidesSolidAt(L, probe);
}

static void EnsureChaserDefaults(Enemy& e) {
    if (e.type != EnemyType::CHASE) return;
    if (e.hp <= 0) e.hp = CHASER_HP;
    if (e.speed <= 0) e.speed = CHASER_SPEED;
    if (e.aggroRange <= 0) e.aggroRange = 520.0f;
}

EnemyUpdateResult EnemiesUpdate(Level& L, Player& player, std::vector<Bullet>& bullets, float dt) {
    EnemyUpdateResult res{};

    for (auto& e : L.enemies) {
        if (!e.alive) continue;

        EnsureChaserDefaults(e);

        if (e.jumpCooldown > 0.0f) e.jumpCooldown -= dt;
        e.vel.y += GRAVITY * dt;

        if (e.type == EnemyType::PATROL) {
            if (e.vel.x == 0) e.vel.x = e.speed;

            if (e.rightBound > e.leftBound + 1.0f) {
                if (e.pos.x < e.leftBound)  { e.pos.x = e.leftBound;  e.vel.x = fabsf(e.speed); }
                if (e.pos.x > e.rightBound) { e.pos.x = e.rightBound; e.vel.x = -fabsf(e.speed); }
            } else {
                float dir = (e.vel.x >= 0) ? 1.0f : -1.0f;
                if (WallAhead(L, e, dir)) e.vel.x = -e.vel.x;
            }
        } else {
            if (!e.aggro) {
                if (e.vel.x == 0) e.vel.x = e.speed;

                if (e.rightBound > e.leftBound + 1.0f) {
                    if (e.pos.x < e.leftBound)  { e.pos.x = e.leftBound;  e.vel.x = fabsf(e.speed); }
                    if (e.pos.x > e.rightBound) { e.pos.x = e.rightBound; e.vel.x = -fabsf(e.speed); }
                } else {
                    float dir = (e.vel.x >= 0) ? 1.0f : -1.0f;
                    if (WallAhead(L, e, dir)) e.vel.x = -e.vel.x;
                }
            } else {
                float dx = player.pos.x - e.pos.x;
                float dir = (dx >= 0) ? 1.0f : -1.0f;
                e.vel.x = dir * e.speed;

                bool playerAbove = (player.pos.y + 6.0f) < e.pos.y;
                bool closeX = fabsf(dx) < 150.0f;
                bool wall = WallAhead(L, e, dir);

                if (e.onGround && e.jumpCooldown <= 0.0f && (wall || (playerAbove && closeX))) {
                    e.vel.y = CHASER_JUMP_VEL;
                    e.jumpCooldown = CHASER_JUMP_CD;
                }
            }
        }

        EnemyResolveCollisions2D(L, e, dt);

        // Bullet hits
        for (auto& b : bullets) {
            if (!b.active) continue;
            Rectangle br = b.Rect(BULLET_RADIUS);

            if (CheckCollisionRecs(br, e.Rect())) {
                b.active = false;

                if (e.type == EnemyType::CHASE) {
                    e.aggro = true;
                    e.hp--;
                    if (e.hp <= 0) e.alive = false;
                } else {
                    e.alive = false;
                }
            }
        }

        // Player contact
        if (e.alive && CheckCollisionRecs(player.Rect(), e.Rect())) {
            res.playerKilled = true;
        }
    }

    return res;
}
