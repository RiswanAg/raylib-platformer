#pragma once
#include "entities.h"
#include "level.h"
#include <vector>

static constexpr float BULLET_RADIUS = 6.0f;

void BulletsUpdate(const Level& L, std::vector<Bullet>& bullets, float dt);
void BulletFire(std::vector<Bullet>& bullets, Vector2 origin, Vector2 dir, float speed, float lifeSeconds);
