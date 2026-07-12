#pragma once
#include "entities.h"
#include "level.h"

void PlayerUpdateInput(Player& p, float dt);
void PlayerApplyGravity(Player& p, float dt, float gravity);
void PlayerResolveCollisions(const Level& L, Player& p, float dt);
