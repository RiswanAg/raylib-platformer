#pragma once
#include "entities.h"
#include "level.h"
#include <vector>

struct EnemyUpdateResult {
    bool playerKilled = false;
};

EnemyUpdateResult EnemiesUpdate(Level& L, Player& player, std::vector<Bullet>& bullets, float dt);
