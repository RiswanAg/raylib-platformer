#include "bullet_system.h"
#include <cmath>

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

void BulletFire(std::vector<Bullet>& bullets, Vector2 origin, Vector2 dir, float speed, float lifeSeconds) {
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
    if (len < 0.001f) { dir = {1,0}; len = 1.0f; }
    dir.x /= len; dir.y /= len;

    for (auto& b : bullets) {
        if (!b.active) {
            b.active = true;
            b.pos = origin;
            b.vel = { dir.x * speed, dir.y * speed };
            b.life = lifeSeconds;
            return;
        }
    }
}

void BulletsUpdate(const Level& L, std::vector<Bullet>& bullets, float dt) {
    for (auto& b : bullets) {
        if (!b.active) continue;

        b.life -= dt;
        if (b.life <= 0.0f) { b.active = false; continue; }

        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;

        Rectangle br = b.Rect(BULLET_RADIUS);
        if (CollidesSolidAt(L, br)) b.active = false;
    }
}
