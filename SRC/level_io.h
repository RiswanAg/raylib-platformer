#pragma once
#include "level.h"
#include <string>

bool SaveLevelToFile(const Level& L, const char* filePath);

// Returns true on success. If false, errorOut contains the exact reason.
bool LoadLevelFromFile(Level& L, const char* filePath, std::string* errorOut);
