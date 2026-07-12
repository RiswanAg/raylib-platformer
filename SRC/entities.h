#pragma once
#include "raylib.h"

static constexpr int TILE = 32;

// -------------------- Bullet --------------------
struct Bullet {
    Vector2 pos{};
    Vector2 vel{};
    float   life = 0.0f;   // seconds remaining
    bool    active = false;

    Rectangle Rect(float radius = 6.0f) const {
        return { pos.x - radius, pos.y - radius, radius * 2, radius * 2 };
    }
};

enum class EnemyType { PATROL, CHASE };

// CHASE = "Strong Roamer -> Aggro when shot"
struct Enemy {
    EnemyType type = EnemyType::PATROL;

    Vector2 pos{};
    Vector2 vel{};

    float leftBound = 0.0f;
    float rightBound = 0.0f;
    float speed = 140.0f;

    // Strong chaser behavior
    bool  aggro = false;          // becomes true when shot
    int   hp    = 20;             // hits to kill
    float aggroRange = 520.0f;    // horizontal range to keep chasing (optional)

    // Physics
    bool  onGround = false;
    float jumpCooldown = 0.0f;

    bool alive = true;

    Rectangle Rect() const { return { pos.x - 14, pos.y - 18, 28, 36 }; }
};

struct Player {
    Vector2 pos{};
    Vector2 vel{};
    bool onGround = false;
    bool facingRight = true;
    bool alive = true;

    Rectangle Rect() const { return { pos.x - 14, pos.y - 20, 28, 40 }; }
};
