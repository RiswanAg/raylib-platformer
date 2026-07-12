#include "level_io.h"
#include <fstream>
#include <sstream>
#include <cmath>

static void SetErr(std::string* out, const std::string& msg) {
    if (out) *out = msg;
}

static void RecomputeFinishRectFromTile(Level& L, int fx, int fy) {
    L.finishRect = { fx * (float)TILE, fy * (float)TILE, (float)TILE, (float)TILE };
}

static std::string NormalizeRow(const std::string& src, int width) {
    // Keep only first 'width' chars; if shorter, pad with '0'
    std::string r = src;
    if ((int)r.size() < width) r.append(width - (int)r.size(), '0');
    if ((int)r.size() > width) r.resize(width);
    return r;
}

bool SaveLevelToFile(const Level& L, const char* filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << L.width << " " << L.height << "\n";
    out << L.spawn.x << " " << L.spawn.y << "\n";

    int fx = (int)floorf(L.finishRect.x / TILE);
    int fy = (int)floorf(L.finishRect.y / TILE);
    out << fx << " " << fy << "\n";

    out << (int)L.enemies.size() << "\n";
    for (const auto& e : L.enemies) {
        int t = (e.type == EnemyType::PATROL) ? 0 : 1;
        out << t << " "
            << e.pos.x << " " << e.pos.y << " "
            << e.leftBound << " " << e.rightBound << " "
            << e.speed << " " << e.aggroRange << " "
            << (e.alive ? 1 : 0) << "\n";
    }

    // IMPORTANT: always write exactly width chars per row
    for (int y = 0; y < L.height; y++) {
        std::string row = (y >= 0 && y < (int)L.tiles.size()) ? L.tiles[y] : "";
        out << NormalizeRow(row, L.width) << "\n";
    }

    return true;
}

bool LoadLevelFromFile(Level& L, const char* filePath, std::string* errorOut) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        SetErr(errorOut, "Cannot open file (not found or permission).");
        return false;
    }

    int w = 0, h = 0;
    if (!(in >> w >> h)) { SetErr(errorOut, "Bad header: missing width/height."); return false; }
    if (w <= 0 || h <= 0) { SetErr(errorOut, "Bad header: width/height must be > 0."); return false; }
    if (w > 5000 || h > 5000) { SetErr(errorOut, "Bad header: level too large (corrupt)."); return false; }

    Vector2 spawn{};
    if (!(in >> spawn.x >> spawn.y)) { SetErr(errorOut, "Bad header: missing spawnX spawnY."); return false; }

    int fx = 0, fy = 0;
    if (!(in >> fx >> fy)) { SetErr(errorOut, "Bad header: missing finishTileX finishTileY."); return false; }

    int enemyCount = 0;
    if (!(in >> enemyCount)) { SetErr(errorOut, "Bad header: missing enemyCount."); return false; }
    if (enemyCount < 0 || enemyCount > 5000) { SetErr(errorOut, "enemyCount is insane (corrupt)."); return false; }

    std::string dummy;
    std::getline(in, dummy); // consume endline

    std::vector<Enemy> enemies;
    enemies.reserve((size_t)enemyCount);

    int enemiesRead = 0;
    while (enemiesRead < enemyCount && in.good()) {
        std::string line;
        std::getline(in, line);
        if (line.empty()) continue;

        std::istringstream ss(line);
        int typeInt = 0, aliveInt = 1;
        Enemy e{};

        if (!(ss >> typeInt
                 >> e.pos.x >> e.pos.y
                 >> e.leftBound >> e.rightBound
                 >> e.speed >> e.aggroRange
                 >> aliveInt)) {
            SetErr(errorOut, "Enemy parse failed at index " + std::to_string(enemiesRead) + ".");
            return false;
        }

        e.type = (typeInt == 0) ? EnemyType::PATROL : EnemyType::CHASE;
        e.alive = (aliveInt != 0);
        e.vel = { (e.type == EnemyType::PATROL) ? e.speed : 0.0f, 0.0f };

        enemies.push_back(e);
        enemiesRead++;
    }

    if (enemiesRead != enemyCount) {
        SetErr(errorOut, "File ended early: expected " + std::to_string(enemyCount) +
                         " enemies, got " + std::to_string(enemiesRead) + ".");
        return false;
    }

    // Read exactly h rows (blank lines allowed; they become zeros)
    std::vector<std::string> rows;
    rows.reserve(h);

    while ((int)rows.size() < h && in.good()) {
        std::string row;
        std::getline(in, row);

        // allow blank -> all zeros
        rows.push_back(NormalizeRow(row, w));
    }

    if ((int)rows.size() != h) {
        SetErr(errorOut, "File ended early: expected " + std::to_string(h) +
                         " map rows, got " + std::to_string((int)rows.size()) + ".");
        return false;
    }

    L.width = w;
    L.height = h;
    L.tiles = rows;
    L.spawn = spawn;
    L.enemies = enemies;
    RecomputeFinishRectFromTile(L, fx, fy);

    SetErr(errorOut, "OK");
    return true;
}
