#include "Game.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cstdio>
#include <random>

static bool FileExistsPortable(const std::string& path) {
    std::FILE* file = nullptr;

#ifdef _MSC_VER
    if (fopen_s(&file, path.c_str(), "rb") == 0 && file != nullptr) {
        std::fclose(file);
        return true;
    }
#else
    file = std::fopen(path.c_str(), "rb");
    if (file != nullptr) {
        std::fclose(file);
        return true;
    }
#endif

    return false;
}

static std::string FindAssetPath(const std::string& relativePath) {
    const char* candidates[] = {
        "",
        "./",
        "../",
        "../../",
        "../../../",
        "x64/Release/",
        "x64/Debug/",
        "./x64/Release/",
        "./x64/Debug/",
        "../x64/Release/",
        "../x64/Debug/"
    };

    for (const char* prefix : candidates) {
        std::string path = std::string(prefix) + relativePath;
        if (FileExistsPortable(path)) {
            return path;
        }
    }

    return relativePath;
}

Game::Game() {
    std::srand((unsigned int)std::time(nullptr));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE);
    InitWindow(screenW, screenH, "NEON ABYSS // NEXUS SIEGE");

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
    SetTargetFPS(60);

    BuildColorTheme();
    BuildDatabases();
    BuildStars();
    BuildDungeon();
    BuildTileMap();
    LoadAssets();

    camera = { 0 };
    camera.offset = { screenW * 0.5f, screenH * 0.5f };
    camera.target = player.pos;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    ResetRun();
    gameState = GameState::Title;
}

Game::~Game() {
    UnloadAssets();

    if (IsWindowReady()) {
        CloseWindow();
    }
}

void Game::Run() {
    while (!WindowShouldClose()) {
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();

        float rawDt = GetFrameTime();
        if (rawDt > 0.033f) {
            rawDt = 0.033f;
        }

        if (hitStopTimer > 0.0f) {
            hitStopTimer -= rawDt;
            if (hitStopTimer < 0.0f) {
                hitStopTimer = 0.0f;
            }
        }

        float dt = (hitStopTimer > 0.0f) ? 0.0f : rawDt;

        UpdateStars(dt);

        switch (gameState) {
        case GameState::Title:
            UpdateTitle();
            break;
        case GameState::Playing:
            UpdatePlaying(dt);
            break;
        case GameState::GameOver:
            UpdateGameOver(dt);
            break;
        }

        UpdateCamera();
        Draw();
    }
}

void Game::BuildColorTheme() {
    bg = { 193, 221, 113, 255 };
    bg2 = { 160, 199, 84, 255 };
    grid = { 146, 184, 82, 255 };
    panel = { 244, 232, 196, 240 };
    panel2 = { 232, 217, 178, 245 };
    neonCyan = { 59, 146, 209, 255 };
    neonBlue = { 88, 128, 204, 255 };
    neonPink = { 191, 86, 101, 255 };
    neonGold = { 214, 173, 76, 255 };
    softRed = { 182, 72, 62, 255 };
    safeGreen = { 74, 146, 70, 255 };
    bossPurple = { 121, 94, 161, 255 };
}

void Game::BuildDatabases() {
    weaponDB = {
        {"Rusty Pipe", 14, 65.0f, "Common", 0, LIGHTGRAY},
        {"Iron Baton", 18, 72.0f, "Common", 60, GRAY},
        {"Shock Stunner", 24, 82.0f, "Common", 120, neonCyan},
        {"Chain Saw Blade", 34, 88.0f, "Common", 200, ORANGE},
        {"Plasma Cutter", 44, 96.0f, "Rare", 300, SKYBLUE},
        {"Laser Edge", 56, 104.0f, "Rare", 430, BLUE},
        {"Thermal Katana", 70, 112.0f, "Rare", 620, RED},
        {"Sonic Vibroblade", 86, 118.0f, "Rare", 850, LIME},
        {"Pulse Rifle Saber", 102, 126.0f, "Rare", 1100, neonCyan},
        {"Void Reaper", 126, 138.0f, "Legendary", 1450, PURPLE},
        {"Arc Disruptor", 152, 146.0f, "Legendary", 1850, YELLOW},
        {"Singularity Core", 184, 154.0f, "Legendary", 2350, VIOLET},
        {"Hyperion Greatsword", 220, 164.0f, "Legendary", 3000, GOLD},
        {"Nanite Obliterator", 262, 174.0f, "Exotic", 3800, GREEN},
        {"Quantum Slicer", 308, 188.0f, "Exotic", 4800, bossPurple},
        {"Celestial Cleaver", 370, 205.0f, "Exotic", 6200, WHITE},
        {"Omega Decimator", 460, 225.0f, "Exotic", 8200, neonPink}
    };

    monsterTypes = {
        {"Cyber Rat", 45, 8, 2.5f, 12.0f, GRAY, false, 18},
        {"Drone Scout", 60, 10, 3.0f, 14.0f, neonCyan, false, 24},
        {"Syndicate Thug", 88, 14, 2.1f, 18.0f, ORANGE, false, 34},
        {"Enforcer Bot", 130, 18, 1.6f, 22.0f, RED, false, 48},
        {"Corrupted Cyborg", 175, 24, 1.8f, 20.0f, PURPLE, false, 64},
        {"Viper Assassin", 110, 22, 3.4f, 15.0f, LIME, false, 56},
        {"Heavy Automaton", 280, 30, 1.15f, 28.0f, DARKBLUE, false, 80},
        {"Neon Brute", 220, 28, 1.4f, 25.0f, PINK, false, 72},
        {"Pulse Wraith", 145, 20, 2.8f, 17.0f, SKYBLUE, false, 60},
        {"Scrap Spider", 95, 16, 2.9f, 13.0f, BROWN, false, 40},
        {"JUGGERNAUT PRIME", 1100, 36, 1.3f, 42.0f, MAROON, true, 550},
        {"THE ARCHITECT", 1800, 48, 1.05f, 48.0f, bossPurple, true, 900}
    };
}

void Game::BuildStars() {
    stars.clear();

    for (int i = 0; i < 120; ++i) {
        stars.push_back({
            { RandomRange(0.0f, (float)screenW), RandomRange(0.0f, (float)screenH) },
            RandomRange(12.0f, 90.0f),
            RandomRange(1.0f, 3.2f)
            });
    }
}

void Game::BuildDungeon() {
    dungeon = DungeonMap{};

    dungeon.rooms = {
        { { -360.0f, -260.0f, 720.0f, 520.0f }, "NEXUS HUB", true, false },
        { { 620.0f, -360.0f, 860.0f, 720.0f }, "EAST FORGE", false, true },
        { { -1480.0f, -360.0f, 860.0f, 720.0f }, "WEST VAULT", false, true },
        { { -430.0f, -1260.0f, 860.0f, 720.0f }, "NORTH CORE", false, true },
        { { -430.0f, 540.0f, 860.0f, 720.0f }, "SOUTH SECTOR", false, true }
    };

    dungeon.corridors = {
        { 320.0f, -120.0f, 360.0f, 240.0f },
        { -680.0f, -120.0f, 360.0f, 240.0f },
        { -120.0f, -620.0f, 240.0f, 360.0f },
        { -120.0f, 260.0f, 240.0f, 360.0f }
    };

    dungeon.obstacles = {
        { { 930.0f, -250.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { 930.0f, 120.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { 1190.0f, -70.0f, 150.0f, 150.0f }, { 44, 56, 82, 255 } },

        { { -1060.0f, -250.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { -1060.0f, 120.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { -1320.0f, -70.0f, 150.0f, 150.0f }, { 44, 56, 82, 255 } },

        { { -280.0f, -1110.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { 150.0f, -1110.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { -70.0f, -920.0f, 140.0f, 140.0f }, { 44, 56, 82, 255 } },

        { { -280.0f, 760.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { 150.0f, 760.0f, 130.0f, 130.0f }, { 52, 64, 96, 255 } },
        { { -70.0f, 950.0f, 140.0f, 140.0f }, { 44, 56, 82, 255 } }
    };

    safeZone = dungeon.rooms.front().rect;
}

void Game::BuildTileMap() {
    int minX = -2200;
    int minY = -1800;
    int maxX = 2200;
    int maxY = 1800;

    tileMap = {};
    tileMap.tileSize = 64;
    tileMap.originX = minX;
    tileMap.originY = minY;
    tileMap.width = (maxX - minX) / tileMap.tileSize;
    tileMap.height = (maxY - minY) / tileMap.tileSize;
    tileMap.tiles.assign((size_t)tileMap.width * (size_t)tileMap.height, (int)TileType::Grass);

    worldProps.clear();
    decorProps.clear();

    for (int y = 0; y < tileMap.height; ++y) {
        for (int x = 0; x < tileMap.width; ++x) {
            Vector2 center = TileWorldCenter(tileMap, x, y);
            int tile = (int)TileType::Grass;

            for (const auto& corridor : dungeon.corridors) {
                if (CheckCollisionPointRec(center, corridor)) {
                    tile = (int)TileType::Path;
                    break;
                }
            }

            if (tile != (int)TileType::Path) {
                for (const auto& room : dungeon.rooms) {
                    if (CheckCollisionPointRec(center, room.rect)) {
                        tile = ((x + y) % 5 == 0) ? (int)TileType::GrassAlt : (int)TileType::Grass;
                        break;
                    }
                }
            }

            if (tile == (int)TileType::Grass && ((x * 17 + y * 31) % 19 == 0)) {
                tile = (int)TileType::Flowers;
            }

            tileMap.tiles[(size_t)y * (size_t)tileMap.width + (size_t)x] = tile;

            bool insidePlayable = false;
            for (const auto& room : dungeon.rooms) {
                if (CheckCollisionPointRec(center, ExpandRect(room.rect, 28.0f))) {
                    insidePlayable = true;
                    break;
                }
            }
            if (!insidePlayable) {
                for (const auto& corridor : dungeon.corridors) {
                    if (CheckCollisionPointRec(center, ExpandRect(corridor, 24.0f))) {
                        insidePlayable = true;
                        break;
                    }
                }
            }

            int hash = (x * 92821 + y * 68917) & 255;
            if (!insidePlayable) {
                if (hash < 32) decorProps.push_back({ center, 0, 1.15f + (hash % 3) * 0.14f });
                else if (hash < 44) decorProps.push_back({ center, 2, 0.80f + (hash % 3) * 0.10f });
                else if (hash < 58) decorProps.push_back({ center, 1, 0.85f + (hash % 2) * 0.10f });
            }
        }
    }

    Vector2 nexusCenter = SafeZoneCenter(safeZone);
    worldProps.push_back({ nexusCenter, 5, 1.8f });
    worldProps.push_back({ { nexusCenter.x - 180.0f, nexusCenter.y + 70.0f }, 4, 1.05f });
    worldProps.push_back({ { nexusCenter.x + 180.0f, nexusCenter.y + 70.0f }, 4, 1.05f });

    for (int i = 1; i < (int)dungeon.rooms.size(); ++i) {
        Vector2 c = SafeZoneCenter(dungeon.rooms[i].rect);
        if (i == 1 || i == 2) {
            worldProps.push_back({ c, 4, 1.05f });
            worldProps.push_back({ { c.x + (i == 1 ? 170.0f : -170.0f), c.y - 60.0f }, 3, 0.95f });
        }
        else {
            worldProps.push_back({ { c.x - 120.0f, c.y + 20.0f }, 3, 0.92f });
            worldProps.push_back({ { c.x + 145.0f, c.y - 10.0f }, 4, 0.92f });
        }
    }

    for (size_t i = 0; i < dungeon.obstacles.size(); ++i) {
        const auto& obstacle = dungeon.obstacles[i];
        Vector2 center = { obstacle.rect.x + obstacle.rect.width * 0.5f, obstacle.rect.y + obstacle.rect.height * 0.5f };
        int spriteIndex = 2;
        float scale = 1.0f;
        switch (i % 4) {
        case 0: spriteIndex = 0; scale = 1.20f; break;
        case 1: spriteIndex = 2; scale = 0.95f; break;
        case 2: spriteIndex = 3; scale = 0.95f; break;
        default: spriteIndex = 4; scale = 0.92f; break;
        }
        worldProps.push_back({ center, spriteIndex, scale });
    }
}

void Game::LoadAssets() {
    tileAtlas = LoadTexture(FindAssetPath("assets/world_tiles.png").c_str());
    propAtlas = LoadTexture(FindAssetPath("assets/props_atlas.png").c_str());
    actorAtlas = LoadTexture(FindAssetPath("assets/actors_atlas.png").c_str());
}

void Game::UnloadAssets() {
    if (tileAtlas.id != 0) UnloadTexture(tileAtlas);
    if (propAtlas.id != 0) UnloadTexture(propAtlas);
    if (actorAtlas.id != 0) UnloadTexture(actorAtlas);
    tileAtlas = {};
    propAtlas = {};
    actorAtlas = {};
}

Rectangle Game::TileSourceRect(int index) const {
    return { (float)(index * 64), 0.0f, 64.0f, 64.0f };
}

Rectangle Game::PropSourceRect(int index) const {
    const int cell = 96;
    const int cols = 5;
    return { (float)((index % cols) * cell), (float)((index / cols) * cell), (float)cell, (float)cell };
}

Rectangle Game::ActorSourceRect(int index) const {
    const int cell = 64;
    const int cols = 4;
    return { (float)((index % cols) * cell), (float)((index / cols) * cell), (float)cell, (float)cell };
}

Vector2 Game::MoveWithCollision(Vector2 start, Vector2 delta, float radius, int steps) const {
    if (steps < 1) {
        steps = 1;
    }

    Vector2 position = start;
    Vector2 stepDelta = { delta.x / (float)steps, delta.y / (float)steps };
    std::vector<Rectangle> barriers = GetActiveBarrierRects();

    auto BlockedByBarrier = [&](Vector2 candidate) {
        for (const auto& barrier : barriers) {
            if (CircleRectCollision(candidate, radius, barrier)) {
                return true;
            }
        }
        return false;
        };

    for (int i = 0; i < steps; ++i) {
        Vector2 nextX = { position.x + stepDelta.x, position.y };
        if (IsPositionWalkable(nextX, radius, dungeon) && !BlockedByBarrier(nextX)) {
            position.x = nextX.x;
        }

        Vector2 nextY = { position.x, position.y + stepDelta.y };
        if (IsPositionWalkable(nextY, radius, dungeon) && !BlockedByBarrier(nextY)) {
            position.y = nextY.y;
        }
    }

    return position;
}

Vector2 Game::GetSpawnPointInCombatRoom() const {
    const DungeonArea* room = nullptr;

    if (waveTargetRoomIndex >= 0 && waveTargetRoomIndex < (int)dungeon.rooms.size()) {
        room = &dungeon.rooms[waveTargetRoomIndex];
    }

    if (room == nullptr) {
        for (const auto& candidate : dungeon.rooms) {
            if (candidate.isSpawnZone) {
                room = &candidate;
                break;
            }
        }
    }

    if (room == nullptr) {
        return { 700.0f, 0.0f };
    }

    for (int attempt = 0; attempt < 40; ++attempt) {
        Vector2 point = {
            RandomRange(room->rect.x + 70.0f, room->rect.x + room->rect.width - 70.0f),
            RandomRange(room->rect.y + 70.0f, room->rect.y + room->rect.height - 70.0f)
        };

        if (Distance(point, player.pos) < 240.0f) {
            continue;
        }

        if (IsPositionWalkable(point, 18.0f, dungeon)) {
            return point;
        }
    }

    return {
        room->rect.x + room->rect.width * 0.5f,
        room->rect.y + room->rect.height * 0.5f
    };
}

const DungeonArea* Game::GetCurrentArea(Vector2 pos) const {
    return FindDungeonArea(dungeon, pos);
}

std::vector<Rectangle> Game::GetActiveBarrierRects() const {
    std::vector<Rectangle> barriers;

    if (lockedRoomIndex < 0 || lockedRoomIndex >= (int)dungeon.rooms.size()) {
        return barriers;
    }

    switch (lockedRoomIndex) {
    case 1:
        barriers.push_back({ 500.0f - 18.0f, -96.0f, 36.0f, 192.0f });
        break;
    case 2:
        barriers.push_back({ -500.0f - 18.0f, -96.0f, 36.0f, 192.0f });
        break;
    case 3:
        barriers.push_back({ -96.0f, -440.0f - 18.0f, 192.0f, 36.0f });
        break;
    case 4:
        barriers.push_back({ -96.0f, 440.0f - 18.0f, 192.0f, 36.0f });
        break;
    default:
        break;
    }

    return barriers;
}

int Game::CountRelic(RelicType type) const {
    int count = 0;
    for (RelicType relic : relics) {
        if (relic == type) {
            count++;
        }
    }
    return count;
}

float Game::GetMoveSpeed() const {
    return player.speed + 0.42f * (float)CountRelic(RelicType::PhaseBoots);
}

float Game::GetAttackCooldown() const {
    float value = 0.28f - 0.025f * (float)CountRelic(RelicType::OverclockCore);
    if (value < 0.12f) {
        value = 0.12f;
    }
    return value;
}

float Game::GetDashCooldown() const {
    float value = 3.0f - 0.24f * (float)CountRelic(RelicType::PhaseBoots);
    if (value < 1.4f) {
        value = 1.4f;
    }
    return value;
}

float Game::GetCritChance() const {
    return 0.10f + 0.08f * (float)CountRelic(RelicType::RazorPrism);
}

float Game::GetCritMultiplier() const {
    return 1.65f + 0.18f * (float)CountRelic(RelicType::RazorPrism);
}

int Game::GetFlatDamageBonus() const {
    return 8 * CountRelic(RelicType::RazorPrism);
}

int Game::GetEmpDamage() const {
    return 100 + player.wave * 6 + 35 * CountRelic(RelicType::EMPCapacitor);
}

float Game::GetEmpRadius() const {
    return 260.0f + 18.0f * (float)CountRelic(RelicType::EMPCapacitor);
}

int Game::GetTurretDamage() const {
    return 16 + player.wave * 2 + 10 * CountRelic(RelicType::SentryKernel);
}

float Game::GetTurretLifetime() const {
    return 18.0f + 3.5f * (float)CountRelic(RelicType::SentryKernel);
}

int Game::GetHealPickupValue(int baseValue) const {
    return baseValue + 4 * CountRelic(RelicType::NanoforgeHeart);
}

void Game::BuildRewardChoices() {
    rewardChoices.clear();

    std::vector<RelicType> pool = {
        RelicType::OverclockCore,
        RelicType::RazorPrism,
        RelicType::NanoforgeHeart,
        RelicType::PhaseBoots,
        RelicType::SentryKernel,
        RelicType::EMPCapacitor,
        RelicType::BloodCircuit
    };

    static std::mt19937 rng((unsigned int)std::time(nullptr) + 77u);
    std::shuffle(pool.begin(), pool.end(), rng);
    for (int i = 0; i < 3 && i < (int)pool.size(); ++i) {
        rewardChoices.push_back(GetRelicData(pool[i]));
    }
}

void Game::ApplyRelic(RelicType type) {
    relics.push_back(type);
    player.relicsCollected++;

    if (type == RelicType::NanoforgeHeart) {
        player.maxHp += 18;
        player.hp = std::min(player.maxHp, player.hp + 24);
    }
}

bool Game::HasSaveFile() const {
    std::ifstream in("savegame.txt");
    return in.good();
}

void Game::DeleteSave() const {
    std::remove("savegame.txt");
}

void Game::SaveRun() const {
    std::ofstream out("savegame.txt", std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "NEON_ABYSS_SAVE_V2\n";
    out << player.pos.x << ' ' << player.pos.y << ' ' << player.aimDir.x << ' ' << player.aimDir.y << '\n';
    out << player.hp << ' ' << player.maxHp << ' ' << player.xp << ' ' << player.kills << ' '
        << player.wave << ' ' << player.hpUpgradeLevel << ' ' << player.equippedWeaponIdx << ' '
        << player.relicsCollected << '\n';
    out << player.dashCd << ' ' << player.empCd << ' ' << player.turretCd << ' ' << player.attackCd << ' ' << player.hitFlash << '\n';
    out << waveTargetRoomIndex << ' ' << lockedRoomIndex << ' ' << (rewardChestActive ? 1 : 0) << ' '
        << (rewardSelectionOpen ? 1 : 0) << ' ' << rewardChestPos.x << ' ' << rewardChestPos.y << '\n';

    out << shop.ownedWeapons.size();
    for (bool owned : shop.ownedWeapons) {
        out << ' ' << (owned ? 1 : 0);
    }
    out << '\n';

    out << relics.size();
    for (RelicType relic : relics) {
        out << ' ' << (int)relic;
    }
    out << '\n';

    out << monsters.size() << '\n';
    for (const auto& monster : monsters) {
        out << monster.pos.x << ' ' << monster.pos.y << ' '
            << monster.vel.x << ' ' << monster.vel.y << ' '
            << monster.hp << ' ' << monster.maxHp << ' ' << monster.damage << ' ' << monster.xpDrop << ' '
            << monster.speed << ' ' << monster.radius << ' ' << monster.hitFlash << ' ' << monster.attackTimer << ' '
            << (int)monster.color.r << ' ' << (int)monster.color.g << ' ' << (int)monster.color.b << ' ' << (int)monster.color.a << ' '
            << (monster.isBoss ? 1 : 0) << ' ' << (monster.isElite ? 1 : 0) << ' ' << monster.eliteKind << ' ' << monster.typeIndex << '\n';
    }

    out << turrets.size() << '\n';
    for (const auto& turret : turrets) {
        out << turret.pos.x << ' ' << turret.pos.y << ' ' << turret.life << ' ' << turret.fireTimer << '\n';
    }

    out << healthPickups.size() << '\n';
    for (const auto& pickup : healthPickups) {
        out << pickup.pos.x << ' ' << pickup.pos.y << ' ' << pickup.vel.x << ' ' << pickup.vel.y << ' '
            << pickup.healAmount << ' ' << pickup.life << ' ' << pickup.spin << '\n';
    }

    out << orbs.size() << '\n';
    for (const auto& orb : orbs) {
        out << orb.pos.x << ' ' << orb.pos.y << ' ' << orb.vel.x << ' ' << orb.vel.y << ' ' << orb.value << ' '
            << (int)orb.color.r << ' ' << (int)orb.color.g << ' ' << (int)orb.color.b << ' ' << (int)orb.color.a << '\n';
    }
}

bool Game::LoadRun() {
    std::ifstream in("savegame.txt");
    if (!in.is_open()) {
        return false;
    }

    std::string header;
    in >> header;
    if (header != "NEON_ABYSS_SAVE_V2") {
        return false;
    }

    ResetRun();
    monsters.clear();
    orbs.clear();
    healthPickups.clear();
    turrets.clear();
    particles.clear();
    shockwaves.clear();
    beams.clear();
    floatingTexts.clear();

    int rewardChestFlag = 0;
    int rewardSelectionFlag = 0;

    in >> player.pos.x >> player.pos.y >> player.aimDir.x >> player.aimDir.y;
    in >> player.hp >> player.maxHp >> player.xp >> player.kills
        >> player.wave >> player.hpUpgradeLevel >> player.equippedWeaponIdx >> player.relicsCollected;
    in >> player.dashCd >> player.empCd >> player.turretCd >> player.attackCd >> player.hitFlash;
    in >> waveTargetRoomIndex >> lockedRoomIndex >> rewardChestFlag >> rewardSelectionFlag >> rewardChestPos.x >> rewardChestPos.y;

    rewardChestActive = (rewardChestFlag != 0);
    rewardSelectionOpen = false;

    if (player.equippedWeaponIdx < 0 || player.equippedWeaponIdx >= (int)weaponDB.size()) {
        player.equippedWeaponIdx = 0;
    }

    size_t ownedCount = 0;
    in >> ownedCount;
    shop.ownedWeapons.assign(weaponDB.size(), false);
    for (size_t i = 0; i < ownedCount; ++i) {
        int owned = 0;
        in >> owned;
        if (i < shop.ownedWeapons.size()) {
            shop.ownedWeapons[i] = (owned != 0);
        }
    }

    if (!shop.ownedWeapons.empty()) {
        shop.ownedWeapons[0] = true;
    }

    size_t relicCount = 0;
    in >> relicCount;
    relics.clear();
    for (size_t i = 0; i < relicCount; ++i) {
        int relicValue = 0;
        in >> relicValue;
        relics.push_back((RelicType)relicValue);
    }

    size_t monsterCount = 0;
    in >> monsterCount;
    for (size_t i = 0; i < monsterCount; ++i) {
        ActiveMonster monster{};
        int r = 255, g = 255, b = 255, a = 255;
        int isBossInt = 0;
        int isEliteInt = 0;

        in >> monster.pos.x >> monster.pos.y
            >> monster.vel.x >> monster.vel.y
            >> monster.hp >> monster.maxHp >> monster.damage >> monster.xpDrop
            >> monster.speed >> monster.radius >> monster.hitFlash >> monster.attackTimer
            >> r >> g >> b >> a
            >> isBossInt >> isEliteInt >> monster.eliteKind >> monster.typeIndex;

        monster.color = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
        monster.isBoss = (isBossInt != 0);
        monster.isElite = (isEliteInt != 0);
        monster.name = (monster.typeIndex >= 0 && monster.typeIndex < (int)monsterTypes.size())
            ? monsterTypes[monster.typeIndex].name
            : "UNKNOWN";

        if (monster.isElite) {
            if (monster.eliteKind == 0) monster.name = "FRENZIED " + monster.name;
            else if (monster.eliteKind == 1) monster.name = "BULWARK " + monster.name;
            else if (monster.eliteKind == 2) monster.name = "VOLTAIC " + monster.name;
        }

        monsters.push_back(monster);
    }

    size_t turretCount = 0;
    in >> turretCount;
    for (size_t i = 0; i < turretCount; ++i) {
        Turret turret{};
        in >> turret.pos.x >> turret.pos.y >> turret.life >> turret.fireTimer;
        turrets.push_back(turret);
    }

    size_t pickupCount = 0;
    in >> pickupCount;
    for (size_t i = 0; i < pickupCount; ++i) {
        HealthPickup pickup{};
        in >> pickup.pos.x >> pickup.pos.y >> pickup.vel.x >> pickup.vel.y >> pickup.healAmount >> pickup.life >> pickup.spin;
        healthPickups.push_back(pickup);
    }

    size_t orbCount = 0;
    in >> orbCount;
    for (size_t i = 0; i < orbCount; ++i) {
        Orb orb{};
        int r = 255, g = 255, b = 255, a = 255;
        in >> orb.pos.x >> orb.pos.y >> orb.vel.x >> orb.vel.y >> orb.value >> r >> g >> b >> a;
        orb.color = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
        orbs.push_back(orb);
    }

    if (!in.good() && !in.eof()) {
        return false;
    }

    shop.isOpen = false;
    shop.message.clear();
    shop.messageTimer = 0.0f;
    rewardChoices.clear();
    gameState = GameState::Playing;
    announcement = "RUN RESTORED";
    announcementTimer = 2.2f;
    return true;
}

void Game::ResetRun() {
    player = Player{};
    player.xp = 240;

    shop = ShopState{};
    shop.ownedWeapons.assign(weaponDB.size(), false);
    if (!shop.ownedWeapons.empty()) {
        shop.ownedWeapons[0] = true;
    }

    relics.clear();
    rewardChoices.clear();
    rewardChestActive = false;
    rewardSelectionOpen = false;
    rewardChestPos = { 0.0f, 0.0f };
    waveTargetRoomIndex = -1;
    lockedRoomIndex = -1;
    hitStopTimer = 0.0f;
    screenShake = 0.0f;
    nextWaveTimer = 0.0f;
    safeZoneHealBuffer = 0.0f;
    announcement = "NEXUS ONLINE";
    announcementTimer = 2.2f;

    monsters.clear();
    particles.clear();
    orbs.clear();
    healthPickups.clear();
    floatingTexts.clear();
    turrets.clear();
    shockwaves.clear();
    beams.clear();

    SpawnWave(player.wave);
}

void Game::AddFloatingText(Vector2 pos, const std::string& text, Color color) {
    floatingTexts.push_back({ pos, { RandomRange(-0.35f, 0.35f), -1.2f }, text, color, 1.0f });
}

void Game::EmitBurst(Vector2 pos, int count, float speed, Color color, float size) {
    for (int i = 0; i < count; ++i) {
        float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
        float randomSpeed = RandomRange(speed * 0.4f, speed);

        particles.push_back({
            pos,
            { std::cos(angle) * randomSpeed, std::sin(angle) * randomSpeed },
            color,
            RandomRange(0.35f, 0.85f),
            RandomRange(0.35f, 0.85f),
            RandomRange(size * 0.7f, size * 1.35f)
            });
    }
}

void Game::SpawnMonsterByType(int typeIndex, Vector2 pos) {
    const MonsterType& type = monsterTypes[typeIndex];

    ActiveMonster monster{
        pos,
        { 0.0f, 0.0f },
        type.maxHp,
        type.maxHp,
        type.damage,
        type.xpDrop,
        type.speed,
        type.radius,
        0.0f,
        RandomRange(0.15f, 0.8f),
        type.color,
        type.isBoss,
        false,
        -1,
        typeIndex,
        type.name
    };

    if (!type.isBoss) {
        float eliteChance = 0.12f + 0.015f * (float)player.wave;
        if (eliteChance > 0.34f) {
            eliteChance = 0.34f;
        }

        if (RandomRange(0.0f, 1.0f) < eliteChance) {
            monster.isElite = true;
            monster.eliteKind = std::rand() % 3;
            monster.xpDrop += 26;
            monster.radius += 2.0f;

            if (monster.eliteKind == 0) {
                monster.speed *= 1.35f;
                monster.damage += 6;
                monster.color = ORANGE;
                monster.name = "FRENZIED " + monster.name;
            }
            else if (monster.eliteKind == 1) {
                monster.maxHp = (int)(monster.maxHp * 1.75f);
                monster.hp = monster.maxHp;
                monster.damage += 4;
                monster.radius += 4.0f;
                monster.color = GOLD;
                monster.name = "BULWARK " + monster.name;
            }
            else {
                monster.speed *= 1.10f;
                monster.damage += 8;
                monster.color = neonCyan;
                monster.name = "VOLTAIC " + monster.name;
            }
        }
    }

    monsters.push_back(monster);
}

void Game::SpawnWave(int waveNumber) {
    announcement = std::string(TextFormat("WAVE %d // PURGE PROTOCOL", waveNumber));
    announcementTimer = 2.8f;
    rewardChestActive = false;
    rewardSelectionOpen = false;
    rewardChoices.clear();
    lockedRoomIndex = -1;

    std::vector<int> spawnRoomIndices;
    for (int i = 0; i < (int)dungeon.rooms.size(); ++i) {
        if (dungeon.rooms[i].isSpawnZone) {
            spawnRoomIndices.push_back(i);
        }
    }

    if (spawnRoomIndices.empty()) {
        waveTargetRoomIndex = 1;
    }
    else {
        waveTargetRoomIndex = spawnRoomIndices[std::rand() % (int)spawnRoomIndices.size()];
    }

    int normalCount = 5 + waveNumber * 2;
    int maxRegularType = 4 + waveNumber / 2;
    if (maxRegularType > 10) {
        maxRegularType = 10;
    }

    for (int i = 0; i < normalCount; ++i) {
        int typeIndex = std::rand() % maxRegularType;
        SpawnMonsterByType(typeIndex, GetSpawnPointInCombatRoom());
    }

    if (waveNumber % 5 == 0) {
        int bossType = (waveNumber % 10 == 0) ? 11 : 10;
        SpawnMonsterByType(bossType, GetSpawnPointInCombatRoom());
        announcement = monsterTypes[bossType].name + " // BOSS INBOUND";
        announcementTimer = 3.8f;
    }
}

void Game::UpdateTitle() {
    if (IsKeyPressed(KEY_ENTER)) {
        DeleteSave();
        ResetRun();
        gameState = GameState::Playing;
        SaveRun();
    }

    if (HasSaveFile() && IsKeyPressed(KEY_C)) {
        LoadRun();
    }
}

void Game::UpdatePlaying(float dt) {
    player.attackCd = std::max(0.0f, player.attackCd - dt);
    player.dashCd = std::max(0.0f, player.dashCd - dt);
    player.empCd = std::max(0.0f, player.empCd - dt);
    player.turretCd = std::max(0.0f, player.turretCd - dt);
    player.hitFlash = std::max(0.0f, player.hitFlash - dt);
    screenShake = std::max(0.0f, screenShake - dt * 22.0f);
    announcementTimer = std::max(0.0f, announcementTimer - dt);
    shop.messageTimer = std::max(0.0f, shop.messageTimer - dt);

    bool inSafeZone = CheckCollisionPointRec(player.pos, safeZone);
    const DungeonArea* currentArea = GetCurrentArea(player.pos);
    player.speed = GetMoveSpeed();

    if (inSafeZone) {
        safeZoneHealBuffer += (18.0f + 4.0f * (float)CountRelic(RelicType::NanoforgeHeart)) * dt;
        while (safeZoneHealBuffer >= 1.0f) {
            player.hp = std::min(player.maxHp, player.hp + 1);
            safeZoneHealBuffer -= 1.0f;
        }
    }
    else {
        safeZoneHealBuffer = 0.0f;
    }

    if (rewardSelectionOpen) {
        if (IsKeyPressed(KEY_ONE) && rewardChoices.size() > 0) {
            ApplyRelic(rewardChoices[0].type);
            rewardSelectionOpen = false;
            rewardChestActive = false;
            rewardChoices.clear();
            player.wave++;
            SpawnWave(player.wave);
            SaveRun();
        }
        else if (IsKeyPressed(KEY_TWO) && rewardChoices.size() > 1) {
            ApplyRelic(rewardChoices[1].type);
            rewardSelectionOpen = false;
            rewardChestActive = false;
            rewardChoices.clear();
            player.wave++;
            SpawnWave(player.wave);
            SaveRun();
        }
        else if (IsKeyPressed(KEY_THREE) && rewardChoices.size() > 2) {
            ApplyRelic(rewardChoices[2].type);
            rewardSelectionOpen = false;
            rewardChestActive = false;
            rewardChoices.clear();
            player.wave++;
            SpawnWave(player.wave);
            SaveRun();
        }
    }

    if (rewardChestActive && !rewardSelectionOpen && Distance(player.pos, rewardChestPos) < 72.0f && IsKeyPressed(KEY_E)) {
        BuildRewardChoices();
        rewardSelectionOpen = true;
    }

    if (inSafeZone && !rewardSelectionOpen && IsKeyPressed(KEY_E)) {
        shop.isOpen = !shop.isOpen;
        shop.browseWeaponIdx = player.equippedWeaponIdx;
    }
    if (!inSafeZone) {
        shop.isOpen = false;
    }

    if (!rewardChestActive && lockedRoomIndex < 0 && currentArea != nullptr) {
        for (int i = 0; i < (int)dungeon.rooms.size(); ++i) {
            if (i == waveTargetRoomIndex && &dungeon.rooms[i] == currentArea && !dungeon.rooms[i].isSafeZone && !monsters.empty()) {
                Rectangle lockZone = ShrinkRect(dungeon.rooms[i].rect, 96.0f);
                if (lockZone.width < 40.0f || lockZone.height < 40.0f) {
                    lockZone = dungeon.rooms[i].rect;
                }

                if (CheckCollisionPointRec(player.pos, lockZone)) {
                    lockedRoomIndex = i;

                    Vector2 roomCenter = {
                        dungeon.rooms[i].rect.x + dungeon.rooms[i].rect.width * 0.5f,
                        dungeon.rooms[i].rect.y + dungeon.rooms[i].rect.height * 0.5f
                    };
                    Vector2 pushDir = VecNormalizeSafe(VecSub(roomCenter, player.pos));
                    if (pushDir.x != 0.0f || pushDir.y != 0.0f) {
                        player.pos = MoveWithCollision(player.pos, VecScale(pushDir, 42.0f), 18.0f, 8);
                    }

                    announcement = dungeon.rooms[i].name + " // LOCKDOWN";
                    announcementTimer = 2.0f;
                }
                break;
            }
        }
    }

    Vector2 moveInput = { 0.0f, 0.0f };
    if (!shop.isOpen && !rewardSelectionOpen) {
        if (IsKeyDown(KEY_W)) moveInput.y -= 1.0f;
        if (IsKeyDown(KEY_S)) moveInput.y += 1.0f;
        if (IsKeyDown(KEY_A)) moveInput.x -= 1.0f;
        if (IsKeyDown(KEY_D)) moveInput.x += 1.0f;
    }

    if (moveInput.x != 0.0f || moveInput.y != 0.0f) {
        moveInput = VecNormalizeSafe(moveInput);
        player.aimDir = moveInput;
        player.pos = MoveWithCollision(player.pos, VecScale(moveInput, player.speed * 60.0f * dt), 18.0f);

        if ((int)(GetTime() * 18.0) % 2 == 0) {
            particles.push_back({
                VecSub(player.pos, VecScale(player.aimDir, 14.0f)),
                { RandomRange(-0.5f, 0.5f), RandomRange(-0.5f, 0.5f) },
                Fade(neonCyan, 0.8f),
                0.2f,
                0.2f,
                RandomRange(2.0f, 4.5f)
                });
        }
    }

    if (!shop.isOpen && !rewardSelectionOpen && IsKeyPressed(KEY_SPACE) && player.attackCd <= 0.0f) {
        player.attackCd = GetAttackCooldown();
        const Weapon& weapon = weaponDB[player.equippedWeaponIdx];
        bool hitSomething = false;

        shockwaves.push_back({ player.pos, 12.0f, weapon.range + 4.0f * CountRelic(RelicType::RazorPrism), 0.22f, 0.22f, Fade(weapon.color, 0.7f) });
        EmitBurst(player.pos, 16, 4.6f, weapon.color, 4.0f);

        for (auto& monster : monsters) {
            float dist = Distance(player.pos, monster.pos);
            if (dist <= weapon.range) {
                int damage = weapon.damage + GetFlatDamageBonus() + (std::rand() % 7);
                bool isCrit = RandomRange(0.0f, 1.0f) < GetCritChance();
                if (isCrit) {
                    damage = (int)(damage * GetCritMultiplier());
                }

                monster.hp -= damage;
                monster.hitFlash = isCrit ? 0.18f : 0.12f;
                AddFloatingText(monster.pos, (isCrit ? "CRIT -" : "-") + std::to_string(damage), isCrit ? neonGold : weapon.color);
                EmitBurst(monster.pos, isCrit ? 16 : 9, isCrit ? 5.5f : 4.2f, isCrit ? neonGold : weapon.color, isCrit ? 4.4f : 3.2f);
                hitSomething = true;

                if (CountRelic(RelicType::BloodCircuit) > 0) {
                    player.hp = std::min(player.maxHp, player.hp + CountRelic(RelicType::BloodCircuit));
                }

                if (isCrit) {
                    hitStopTimer = std::max(hitStopTimer, 0.045f);
                }
            }
        }

        if (hitSomething) {
            screenShake = std::max(screenShake, 8.0f);
        }
    }

    if (!shop.isOpen && !rewardSelectionOpen && IsKeyPressed(KEY_ONE) && player.dashCd <= 0.0f) {
        player.dashCd = GetDashCooldown();
        Vector2 dashDir = player.aimDir;
        if (dashDir.x == 0.0f && dashDir.y == 0.0f) {
            dashDir = { 1.0f, 0.0f };
        }

        for (int i = 0; i < 7; ++i) {
            Vector2 trailPos = VecSub(player.pos, VecScale(dashDir, (float)i * 26.0f));
            EmitBurst(trailPos, 5, 2.8f, neonCyan, 4.5f);
        }

        player.pos = MoveWithCollision(player.pos, VecScale(dashDir, 210.0f), 18.0f, 12);
        shockwaves.push_back({ player.pos, 8.0f, 95.0f, 0.18f, 0.18f, Fade(neonCyan, 0.8f) });
        screenShake = std::max(screenShake, 6.0f);
    }

    if (!shop.isOpen && !rewardSelectionOpen && IsKeyPressed(KEY_TWO) && player.empCd <= 0.0f) {
        player.empCd = 6.0f;
        int empDamage = GetEmpDamage();
        float empRadius = GetEmpRadius();

        shockwaves.push_back({ player.pos, 20.0f, empRadius, 0.45f, 0.45f, Fade(neonCyan, 0.85f) });
        EmitBurst(player.pos, 36, 7.5f, neonCyan, 5.5f);
        screenShake = std::max(screenShake, 10.0f);
        hitStopTimer = std::max(hitStopTimer, 0.03f);

        for (auto& monster : monsters) {
            float dist = Distance(player.pos, monster.pos);
            if (dist <= empRadius) {
                monster.hp -= empDamage;
                monster.hitFlash = 0.18f;
                AddFloatingText(monster.pos, "EMP " + std::to_string(empDamage), neonCyan);
                EmitBurst(monster.pos, 12, 5.0f, neonCyan, 3.0f);
            }
        }
    }

    if (!shop.isOpen && !rewardSelectionOpen && IsKeyPressed(KEY_THREE) && player.turretCd <= 0.0f) {
        player.turretCd = 12.0f;
        turrets.push_back({ player.pos, GetTurretLifetime(), 0.25f });
        EmitBurst(player.pos, 18, 3.0f, neonBlue, 3.8f);
        AddFloatingText(player.pos, "TURRET ONLINE", neonBlue);
    }

    for (auto& turret : turrets) {
        turret.life -= dt;
        turret.fireTimer -= dt;

        if (turret.fireTimer <= 0.0f && !monsters.empty()) {
            int targetIndex = -1;
            float bestDist = 260.0f;

            for (int i = 0; i < (int)monsters.size(); ++i) {
                float dist = Distance(turret.pos, monsters[i].pos);
                if (dist < bestDist) {
                    bestDist = dist;
                    targetIndex = i;
                }
            }

            if (targetIndex >= 0) {
                ActiveMonster& monster = monsters[targetIndex];
                int turretDamage = GetTurretDamage();
                monster.hp -= turretDamage;
                monster.hitFlash = 0.1f;
                AddFloatingText(monster.pos, "-" + std::to_string(turretDamage), neonBlue);
                EmitBurst(monster.pos, 7, 3.8f, neonBlue, 2.8f);
                beams.push_back({ turret.pos, monster.pos, neonBlue, 3.5f, 0.08f });
                turret.fireTimer = 0.32f;
            }
            else {
                turret.fireTimer = 0.1f;
            }
        }
    }

    turrets.erase(std::remove_if(turrets.begin(), turrets.end(), [](const Turret& turret) {
        return turret.life <= 0.0f;
        }), turrets.end());

    for (auto& monster : monsters) {
        monster.hitFlash = std::max(0.0f, monster.hitFlash - dt);
        monster.attackTimer = std::max(0.0f, monster.attackTimer - dt);

        Vector2 toPlayer = VecSub(player.pos, monster.pos);
        float dist = VecLength(toPlayer);
        Vector2 dir = VecNormalizeSafe(toPlayer);

        bool playerInTargetRoom = (waveTargetRoomIndex >= 0 && waveTargetRoomIndex < (int)dungeon.rooms.size() && currentArea == &dungeon.rooms[waveTargetRoomIndex]);

        if (CheckCollisionPointRec(monster.pos, safeZone)) {
            Vector2 away = VecNormalizeSafe(VecSub(monster.pos, SafeZoneCenter(safeZone)));
            monster.pos = MoveWithCollision(monster.pos, VecScale(away, monster.speed * 80.0f * dt), monster.radius);
        }
        else if (!inSafeZone && dist > 6.0f) {
            if (lockedRoomIndex >= 0 || playerInTargetRoom) {
                monster.vel = VecScale(dir, monster.speed * 72.0f * dt);
                monster.pos = MoveWithCollision(monster.pos, monster.vel, monster.radius);
            }
        }

        if (!inSafeZone && dist <= monster.radius + 18.0f && monster.attackTimer <= 0.0f) {
            player.hp -= monster.damage;
            if (player.hp < 0) {
                player.hp = 0;
            }

            player.hitFlash = 0.18f;
            monster.attackTimer = monster.isBoss ? 0.55f : 0.8f;
            AddFloatingText(player.pos, "-" + std::to_string(monster.damage), softRed);
            EmitBurst(player.pos, 10, 3.0f, softRed, 3.0f);
            screenShake = std::max(screenShake, monster.isBoss ? 12.0f : 5.0f);
        }
    }

    for (auto it = monsters.begin(); it != monsters.end();) {
        if (it->hp <= 0) {
            int orbCount = it->isBoss ? 14 : (it->isElite ? 7 : 4);

            for (int i = 0; i < orbCount; ++i) {
                float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
                float speed = RandomRange(2.0f, 5.0f);
                int value = std::max(1, it->xpDrop / orbCount);

                orbs.push_back({
                    it->pos,
                    { std::cos(angle) * speed, std::sin(angle) * speed },
                    value,
                    it->isBoss ? neonGold : (it->isElite ? neonPink : neonCyan)
                    });
            }

            float healDropChance = it->isBoss ? 1.0f : (it->isElite ? 0.75f : 0.18f);
            if (RandomRange(0.0f, 1.0f) < healDropChance) {
                float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
                healthPickups.push_back({
                    it->pos,
                    { std::cos(angle) * 1.4f, std::sin(angle) * 1.4f },
                    GetHealPickupValue(it->isBoss ? 24 : (it->isElite ? 16 : 10)),
                    16.0f,
                    RandomRange(0.0f, 6.28f)
                    });
            }

            EmitBurst(it->pos, it->isBoss ? 42 : (it->isElite ? 24 : 16), it->isBoss ? 7.0f : 4.0f, it->color, it->isBoss ? 5.0f : 3.2f);
            shockwaves.push_back({ it->pos, 12.0f, it->isBoss ? 220.0f : (it->isElite ? 120.0f : 90.0f), it->isBoss ? 0.55f : 0.22f, it->isBoss ? 0.55f : 0.22f, Fade(it->color, 0.8f) });
            AddFloatingText(it->pos, it->name + " DOWN", it->isBoss ? neonGold : (it->isElite ? neonPink : WHITE));
            player.kills++;
            hitStopTimer = std::max(hitStopTimer, it->isBoss ? 0.08f : (it->isElite ? 0.05f : 0.0f));
            screenShake = std::max(screenShake, it->isBoss ? 14.0f : (it->isElite ? 7.0f : 4.0f));
            it = monsters.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto& orb : orbs) {
        orb.pos = VecAdd(orb.pos, orb.vel);
        orb.vel = VecScale(orb.vel, 0.96f);

        float dist = Distance(orb.pos, player.pos);
        if (dist < 220.0f) {
            Vector2 pull = VecNormalizeSafe(VecSub(player.pos, orb.pos));
            orb.vel = VecAdd(orb.vel, VecScale(pull, 0.55f));
        }
    }

    for (auto it = orbs.begin(); it != orbs.end();) {
        if (Distance(it->pos, player.pos) <= 18.0f) {
            player.xp += it->value;
            if (it->value >= 20) {
                AddFloatingText(player.pos, "+" + std::to_string(it->value) + " XP", neonGold);
            }
            it = orbs.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto& pickup : healthPickups) {
        pickup.spin += dt * 4.0f;
        pickup.life -= dt;
        pickup.pos = VecAdd(pickup.pos, pickup.vel);
        pickup.vel = VecScale(pickup.vel, 0.94f);
    }

    for (auto it = healthPickups.begin(); it != healthPickups.end();) {
        if (Distance(it->pos, player.pos) <= 22.0f) {
            player.hp = std::min(player.maxHp, player.hp + it->healAmount);
            AddFloatingText(player.pos, "+" + std::to_string(it->healAmount) + " HP", safeGreen);
            EmitBurst(it->pos, 10, 3.2f, safeGreen, 2.8f);
            it = healthPickups.erase(it);
        }
        else if (it->life <= 0.0f) {
            it = healthPickups.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto& particle : particles) {
        particle.pos = VecAdd(particle.pos, particle.vel);
        particle.vel = VecScale(particle.vel, 0.96f);
        particle.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& particle) {
        return particle.life <= 0.0f;
        }), particles.end());

    for (auto& shockwave : shockwaves) {
        float t = 1.0f - (shockwave.life / shockwave.maxLife);
        shockwave.radius = LerpFloat(12.0f, shockwave.maxRadius, t);
        shockwave.life -= dt;
    }
    shockwaves.erase(std::remove_if(shockwaves.begin(), shockwaves.end(), [](const Shockwave& shockwave) {
        return shockwave.life <= 0.0f;
        }), shockwaves.end());

    for (auto& beam : beams) {
        beam.life -= dt;
    }
    beams.erase(std::remove_if(beams.begin(), beams.end(), [](const Beam& beam) {
        return beam.life <= 0.0f;
        }), beams.end());

    for (auto& text : floatingTexts) {
        text.pos = VecAdd(text.pos, text.vel);
        text.life -= dt;
    }
    floatingTexts.erase(std::remove_if(floatingTexts.begin(), floatingTexts.end(), [](const FloatingText& text) {
        return text.life <= 0.0f;
        }), floatingTexts.end());

    if (lockedRoomIndex >= 0 && lockedRoomIndex < (int)dungeon.rooms.size()) {
        bool roomStillHot = false;
        for (const auto& monster : monsters) {
            if (CheckCollisionPointRec(monster.pos, dungeon.rooms[lockedRoomIndex].rect)) {
                roomStillHot = true;
                break;
            }
        }

        if (!roomStillHot) {
            Rectangle room = dungeon.rooms[lockedRoomIndex].rect;
            rewardChestActive = true;
            rewardSelectionOpen = false;
            rewardChestPos = { room.x + room.width * 0.5f, room.y + room.height * 0.5f };
            announcement = dungeon.rooms[lockedRoomIndex].name + " // ROOM CLEARED";
            announcementTimer = 2.2f;
            lockedRoomIndex = -1;
            hitStopTimer = std::max(hitStopTimer, 0.06f);
            SaveRun();
        }
    }

    if (shop.isOpen) {
        int hpUpgradeCost = 100 + player.hpUpgradeLevel * 90;

        if (IsKeyPressed(KEY_RIGHT)) {
            shop.browseWeaponIdx = (shop.browseWeaponIdx + 1) % (int)weaponDB.size();
        }
        if (IsKeyPressed(KEY_LEFT)) {
            shop.browseWeaponIdx = (shop.browseWeaponIdx - 1 + (int)weaponDB.size()) % (int)weaponDB.size();
        }

        if (IsKeyPressed(KEY_H)) {
            if (player.xp >= hpUpgradeCost) {
                player.xp -= hpUpgradeCost;
                player.hpUpgradeLevel++;
                player.maxHp += 30;
                player.hp = std::min(player.maxHp, player.hp + 30);
                shop.message = "MAX HP BOOSTED";
                shop.messageColor = safeGreen;
                shop.messageTimer = 1.5f;
                SaveRun();
            }
            else {
                shop.message = "NOT ENOUGH XP";
                shop.messageColor = softRed;
                shop.messageTimer = 1.2f;
            }
        }

        if (IsKeyPressed(KEY_B)) {
            if (shop.ownedWeapons[shop.browseWeaponIdx]) {
                player.equippedWeaponIdx = shop.browseWeaponIdx;
                shop.message = "WEAPON EQUIPPED";
                shop.messageColor = neonCyan;
                shop.messageTimer = 1.2f;
                SaveRun();
            }
            else if (player.xp >= weaponDB[shop.browseWeaponIdx].cost) {
                player.xp -= weaponDB[shop.browseWeaponIdx].cost;
                shop.ownedWeapons[shop.browseWeaponIdx] = true;
                player.equippedWeaponIdx = shop.browseWeaponIdx;
                shop.message = "PURCHASE COMPLETE";
                shop.messageColor = neonGold;
                shop.messageTimer = 1.5f;
                SaveRun();
            }
            else {
                shop.message = "NOT ENOUGH XP";
                shop.messageColor = softRed;
                shop.messageTimer = 1.2f;
            }
        }
    }

    if (player.hp <= 0) {
        DeleteSave();
        gameState = GameState::GameOver;
        announcement = "SIGNAL LOST";
        announcementTimer = 3.0f;
    }
}

void Game::UpdateGameOver(float dt) {
    announcementTimer = std::max(0.0f, announcementTimer - dt);

    if (IsKeyPressed(KEY_ENTER)) {
        ResetRun();
        gameState = GameState::Playing;
    }
}

void Game::UpdateStars(float dt) {
    for (auto& star : stars) {
        star.pos.y += star.speed * dt;
        star.pos.x += std::sin((float)GetTime() * 0.4f + star.speed * 0.01f) * 4.0f * dt;

        if (star.pos.y > screenH + 5.0f) {
            star.pos.y = -5.0f;
            star.pos.x = RandomRange(0.0f, (float)screenW);
        }
    }
}

void Game::UpdateCamera() {
    Vector2 shakeOffset = {
        RandomRange(-screenShake, screenShake),
        RandomRange(-screenShake, screenShake)
    };

    camera.target = VecAdd(player.pos, VecScale(player.aimDir, 36.0f));
    camera.offset = { screenW * 0.5f + shakeOffset.x, screenH * 0.5f + shakeOffset.y };
}

void Game::Draw() const {
    BeginDrawing();
    ClearBackground(bg);

    DrawRectangleGradientV(0, 0, screenW, screenH, bg2, bg);
    for (const auto& star : stars) {
        DrawCircleV(star.pos, star.size, Fade(WHITE, 0.35f + star.size * 0.12f));
    }

    if (gameState == GameState::Title) {
        DrawTitleScreen();
    }
    else {
        DrawWorld();
        DrawHud();

        if (rewardSelectionOpen) {
            DrawRewardOverlay();
        }

        if (shop.isOpen) {
            DrawShop();
        }

        if (gameState == GameState::GameOver) {
            DrawGameOver();
        }
    }

    for (int y = 0; y < screenH; y += 4) {
        DrawRectangle(0, y, screenW, 1, Fade(BLACK, 0.055f));
    }

    EndDrawing();
}

void Game::DrawTitleScreen() const {
    DrawRectangleGradientV(0, 0, screenW, screenH, { 186, 221, 255, 255 }, bg2);

    for (int i = 0; i < screenW; i += 140) {
        DrawTreeProp({ (float)i + 30.0f, 170.0f + std::sin((float)i * 0.03f) * 8.0f }, 1.7f, { 69, 181, 83, 255 });
        DrawTreeProp({ (float)i + 90.0f, 680.0f + std::cos((float)i * 0.02f) * 10.0f }, 1.9f, { 59, 166, 77, 255 });
    }

    DrawCastleProp({ screenW * 0.5f, 250.0f }, 2.1f);
    DrawTowerProp({ screenW * 0.5f - 220.0f, 300.0f }, 1.35f, neonPink);
    DrawTowerProp({ screenW * 0.5f + 220.0f, 300.0f }, 1.35f, neonPink);

    int titleWidth = MeasureText("NEON ABYSS", 74);
    DrawText("NEON ABYSS", screenW / 2 - titleWidth / 2, 82, 74, { 67, 86, 54, 255 });
    DrawText("KINGDOM SIEGE", screenW / 2 - MeasureText("KINGDOM SIEGE", 30) / 2, 158, 30, softRed);
    DrawText("A hand-drawn fantasy action RPG prototype in Raylib", screenW / 2 - MeasureText("A hand-drawn fantasy action RPG prototype in Raylib", 22) / 2, 196, 22, { 79, 74, 63, 255 });

    DrawPanel({ screenW / 2.0f - 360.0f, 360.0f, 720.0f, 190.0f }, panel, { 118, 105, 80, 255 });
    DrawText("WHAT'S IN THE BUILD", screenW / 2 - MeasureText("WHAT'S IN THE BUILD", 24) / 2, 384, 24, { 92, 78, 58, 255 });
    DrawText("- Cartoony overworld presentation with connected sectors", screenW / 2 - 300, 425, 20, { 73, 67, 54, 255 });
    DrawText("- Real-time combat with dash, pulse, EMP and sentry skills", screenW / 2 - 300, 453, 20, { 73, 67, 54, 255 });
    DrawText("- Room lockdown fights, relic rewards and wave progression", screenW / 2 - 300, 481, 20, { 73, 67, 54, 255 });
    DrawText("- Save / continue support while the art style evolves", screenW / 2 - 300, 509, 20, { 73, 67, 54, 255 });

    Color pulse = ((int)(GetTime() * 2.5) % 2 == 0) ? softRed : WHITE;
    DrawText("PRESS ENTER FOR A NEW RUN", screenW / 2 - MeasureText("PRESS ENTER FOR A NEW RUN", 28) / 2, 586, 28, pulse);

    if (HasSaveFile()) {
        DrawText("PRESS C TO CONTINUE SAVED RUN", screenW / 2 - MeasureText("PRESS C TO CONTINUE SAVED RUN", 22) / 2, 622, 22, neonGold);
    }

    DrawText("ESC closes the game", screenW / 2 - MeasureText("ESC closes the game", 16) / 2, 656, 16, { 86, 82, 72, 255 });
}

void Game::DrawWorld() const {
    BeginMode2D(camera);

    auto drawProp = [&](const PropInstance& prop) {
        if (propAtlas.id == 0) {
            return;
        }

        float cell = 96.0f;
        Rectangle src = PropSourceRect(prop.spriteIndex);
        Rectangle dst = { prop.pos.x - (cell * prop.scale) * 0.5f, prop.pos.y - (cell * prop.scale) * 0.78f, cell * prop.scale, cell * prop.scale };
        DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        };

    int minTileX = (int)((camera.target.x - screenW * 0.6f - tileMap.originX) / tileMap.tileSize) - 1;
    int maxTileX = (int)((camera.target.x + screenW * 0.6f - tileMap.originX) / tileMap.tileSize) + 1;
    int minTileY = (int)((camera.target.y - screenH * 0.6f - tileMap.originY) / tileMap.tileSize) - 1;
    int maxTileY = (int)((camera.target.y + screenH * 0.6f - tileMap.originY) / tileMap.tileSize) + 1;

    if (minTileX < 0) minTileX = 0;
    if (minTileY < 0) minTileY = 0;
    if (maxTileX >= tileMap.width) maxTileX = tileMap.width - 1;
    if (maxTileY >= tileMap.height) maxTileY = tileMap.height - 1;

    for (int y = minTileY; y <= maxTileY; ++y) {
        for (int x = minTileX; x <= maxTileX; ++x) {
            Rectangle dst = TileWorldRect(tileMap, x, y);
            Rectangle src = TileSourceRect(TileAt(tileMap, x, y));

            if (tileAtlas.id != 0) {
                DrawTexturePro(tileAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
            }
        }
    }

    for (const auto& prop : decorProps) {
        drawProp(prop);
    }

    for (const auto& room : dungeon.rooms) {
        DrawRectangleLinesEx(room.rect, 2.0f, Fade({ 93, 125, 58, 255 }, 0.30f));
        int labelSize = room.isSafeZone ? 24 : 20;
        int labelX = (int)(room.rect.x + room.rect.width * 0.5f) - MeasureText(room.name.c_str(), labelSize) / 2;
        DrawText(room.name.c_str(), labelX, (int)room.rect.y + 12, labelSize, { 92, 78, 56, 255 });
    }

    std::vector<PropInstance> sortedProps = worldProps;
    std::sort(sortedProps.begin(), sortedProps.end(), [](const PropInstance& a, const PropInstance& b) {
        return a.pos.y < b.pos.y;
        });
    for (const auto& prop : sortedProps) {
        drawProp(prop);
    }

    for (const auto& barrier : GetActiveBarrierRects()) {
        if (propAtlas.id != 0) {
            int spriteIndex = barrier.width > barrier.height ? 7 : 8;
            Rectangle src = PropSourceRect(spriteIndex);
            Rectangle dst = { barrier.x - 8.0f, barrier.y - 8.0f, barrier.width + 16.0f, barrier.height + 16.0f };
            DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
    }

    if (rewardChestActive && propAtlas.id != 0) {
        Rectangle src = PropSourceRect(6);
        Rectangle dst = { rewardChestPos.x - 48.0f, rewardChestPos.y - 60.0f, 96.0f, 96.0f };
        DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    for (const auto& shockwave : shockwaves) {
        float alpha = ClampFloat(shockwave.life / shockwave.maxLife, 0.0f, 1.0f);
        DrawCircleLines((int)shockwave.pos.x, (int)shockwave.pos.y, shockwave.radius, Fade({ 255, 255, 255, 255 }, alpha * 0.45f));
    }

    for (const auto& turret : turrets) {
        if (actorAtlas.id != 0) {
            Rectangle src = ActorSourceRect(6);
            Rectangle dst = { turret.pos.x - 34.0f, turret.pos.y - 46.0f, 68.0f, 68.0f };
            DrawTexturePro(actorAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
        DrawCircleLines((int)turret.pos.x, (int)turret.pos.y, 260.0f, Fade({ 96, 130, 183, 255 }, 0.10f));
    }

    for (const auto& beam : beams) {
        DrawLineEx(beam.start, beam.end, beam.thickness + 2.0f, Fade({ 255, 246, 204, 255 }, beam.life * 3.0f));
        DrawLineEx(beam.start, beam.end, beam.thickness, { 122, 167, 230, 255 });
    }

    for (const auto& orb : orbs) {
        DrawCartoonShadow({ orb.pos.x, orb.pos.y + 6.0f }, 6.0f, 3.0f, 0.12f);
        DrawCircleV(orb.pos, 6.0f, orb.color);
        DrawCircleV({ orb.pos.x - 1.0f, orb.pos.y - 1.0f }, 2.6f, WHITE);
    }

    for (const auto& pickup : healthPickups) {
        Vector2 bobPos = { pickup.pos.x, pickup.pos.y + std::sin(pickup.spin) * 4.0f };
        DrawCartoonShadow({ bobPos.x, bobPos.y + 8.0f }, 8.0f, 4.0f, 0.16f);
        DrawCircleV(bobPos, 8.0f, { 183, 55, 60, 255 });
        DrawCircleV({ bobPos.x, bobPos.y - 7.0f }, 4.0f, { 82, 164, 72, 255 });
        DrawRectangle((int)bobPos.x - 2, (int)bobPos.y - 5, 4, 10, WHITE);
        DrawRectangle((int)bobPos.x - 5, (int)bobPos.y - 2, 10, 4, WHITE);
    }

    for (const auto& monster : monsters) {
        DrawEnemySprite(monster);
        Rectangle back = { monster.pos.x - 24.0f, monster.pos.y - monster.radius - 20.0f, 48.0f, 6.0f };
        DrawRectangleRec(back, Fade(BLACK, 0.35f));
        DrawRectangleRec({ back.x, back.y, back.width * ((float)monster.hp / (float)monster.maxHp), back.height }, monster.isBoss ? neonGold : safeGreen);
    }

    for (const auto& particle : particles) {
        float alpha = ClampFloat(particle.life / particle.maxLife, 0.0f, 1.0f);
        DrawCircleV(particle.pos, particle.size, Fade(particle.color, alpha));
    }

    if (actorAtlas.id != 0) {
        Rectangle src = ActorSourceRect(0);
        Rectangle dst = { player.pos.x - 38.0f, player.pos.y - 52.0f, 76.0f, 76.0f };
        DrawTexturePro(actorAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, weaponDB[player.equippedWeaponIdx].range, Fade({ 98, 130, 190, 255 }, 0.12f));

    for (const auto& text : floatingTexts) {
        DrawText(text.text.c_str(), (int)text.pos.x, (int)text.pos.y, 18, Fade(text.color, text.life));
    }

    EndMode2D();
}

void Game::DrawHud() const {
    DrawPanel({ 18.0f, 18.0f, 420.0f, 158.0f }, panel, neonBlue);
    DrawText("OPERATIVE STATUS", 34, 28, 20, neonCyan);
    DrawText(TextFormat("WEAPON: %s", weaponDB[player.equippedWeaponIdx].name.c_str()), 34, 54, 18, WHITE);
    DrawText(TextFormat("WAVE %d", player.wave), 300, 28, 20, neonGold);
    DrawText(TextFormat("KILLS %d", player.kills), 300, 54, 18, RAYWHITE);
    DrawText(TextFormat("RELICS %d", player.relicsCollected), 300, 78, 18, neonPink);

    DrawRectangle(34, 88, 250, 18, Fade(WHITE, 0.12f));
    DrawRectangle(34, 88, (int)(250.0f * ((float)player.hp / (float)player.maxHp)), 18, player.hp < player.maxHp * 0.3f ? softRed : safeGreen);
    DrawText(TextFormat("HP %d / %d", player.hp, player.maxHp), 44, 88, 16, WHITE);

    DrawRectangle(34, 118, 250, 12, Fade(WHITE, 0.12f));
    DrawRectangle(34, 118, (int)ClampFloat((float)player.xp / 3000.0f * 250.0f, 0.0f, 250.0f), 12, neonGold);
    DrawText(TextFormat("XP BANK %d", player.xp), 294, 112, 16, neonGold);

    DrawPanel({ 18.0f, 188.0f, 300.0f, 118.0f }, panel, neonPink);
    DrawText("ABILITIES", 32, 200, 18, neonPink);
    DrawText(TextFormat("1 DASH    %.1fs", player.dashCd), 32, 226, 18, player.dashCd <= 0.0f ? neonCyan : GRAY);
    DrawText(TextFormat("2 EMP     %.1fs", player.empCd), 32, 250, 18, player.empCd <= 0.0f ? neonCyan : GRAY);
    DrawText(TextFormat("3 TURRET  %.1fs", player.turretCd), 32, 274, 18, player.turretCd <= 0.0f ? neonCyan : GRAY);

    DrawPanel({ screenW - 352.0f, 18.0f, 334.0f, 150.0f }, panel, safeGreen);
    DrawText("FIELD DATA", screenW - 334, 28, 20, safeGreen);
    DrawText(TextFormat("HOSTILES %d", (int)monsters.size()), screenW - 334, 58, 18, WHITE);
    DrawText(TextFormat("TURRETS  %d", (int)turrets.size()), screenW - 334, 82, 18, WHITE);

    bool inSafeZone = CheckCollisionPointRec(player.pos, safeZone);
    const DungeonArea* currentArea = GetCurrentArea(player.pos);
    DrawText(inSafeZone ? "ZONE: SAFE" : "ZONE: HOT", screenW - 334, 106, 18, inSafeZone ? safeGreen : softRed);
    DrawText(TextFormat("AREA: %s", currentArea ? currentArea->name.c_str() : "ACCESS TUNNEL"), screenW - 334, 130, 16, RAYWHITE);

    DrawMiniMap();

    if (rewardChestActive && !rewardSelectionOpen && Distance(player.pos, rewardChestPos) < 72.0f) {
        DrawText("DATA CACHE READY // PRESS E TO CHOOSE A RELIC", 22, screenH - 34, 18, neonGold);
    }
    else if (inSafeZone) {
        DrawText("SAFE ZONE // PRESS E FOR ARSENAL // LEAVE NEXUS THROUGH ANY GLOWING GATE", 22, screenH - 34, 18, safeGreen);
    }
    else {
        DrawText("WASD MOVE   SPACE ATTACK   1 DASH   2 EMP   3 TURRET   CLEAR ROOMS, HOLD ANGLES, DON'T GET PINCHED", 22, screenH - 34, 18, RAYWHITE);
    }

    if (announcementTimer > 0.0f) {
        int fontSize = 28;
        int width = MeasureText(announcement.c_str(), fontSize);
        DrawText(announcement.c_str(), screenW / 2 - width / 2, 24, fontSize, neonGold);
    }
}

void Game::DrawMiniMap() const {
    Rectangle panelRect = { screenW - 250.0f, 184.0f, 232.0f, 232.0f };
    DrawPanel(panelRect, panel, neonBlue);
    DrawText("TACTICAL MAP", (int)panelRect.x + 18, (int)panelRect.y + 14, 18, neonBlue);

    Rectangle mapRect = { panelRect.x + 16.0f, panelRect.y + 40.0f, panelRect.width - 32.0f, panelRect.height - 56.0f };
    DrawRectangleRec(mapRect, Fade(BLACK, 0.28f));
    DrawRectangleLinesEx(mapRect, 1.0f, Fade(WHITE, 0.16f));

    float minX = dungeon.rooms.front().rect.x;
    float minY = dungeon.rooms.front().rect.y;
    float maxX = dungeon.rooms.front().rect.x + dungeon.rooms.front().rect.width;
    float maxY = dungeon.rooms.front().rect.y + dungeon.rooms.front().rect.height;

    for (const auto& room : dungeon.rooms) {
        if (room.rect.x < minX) minX = room.rect.x;
        if (room.rect.y < minY) minY = room.rect.y;
        if (room.rect.x + room.rect.width > maxX) maxX = room.rect.x + room.rect.width;
        if (room.rect.y + room.rect.height > maxY) maxY = room.rect.y + room.rect.height;
    }

    float worldW = maxX - minX;
    float worldH = maxY - minY;
    float scale = std::min(mapRect.width / worldW, mapRect.height / worldH);

    auto WorldToMap = [&](Vector2 worldPos) -> Vector2 {
        return {
            mapRect.x + (worldPos.x - minX) * scale,
            mapRect.y + (worldPos.y - minY) * scale
        };
        };

    for (const auto& corridor : dungeon.corridors) {
        Rectangle mini = {
            mapRect.x + (corridor.x - minX) * scale,
            mapRect.y + (corridor.y - minY) * scale,
            corridor.width * scale,
            corridor.height * scale
        };
        DrawRectangleRec(mini, Fade(neonBlue, 0.16f));
    }

    const DungeonArea* currentArea = GetCurrentArea(player.pos);
    for (int i = 0; i < (int)dungeon.rooms.size(); ++i) {
        const DungeonArea& room = dungeon.rooms[i];
        Rectangle mini = {
            mapRect.x + (room.rect.x - minX) * scale,
            mapRect.y + (room.rect.y - minY) * scale,
            room.rect.width * scale,
            room.rect.height * scale
        };
        Color fill = room.isSafeZone ? Fade(safeGreen, 0.26f) : Fade(neonBlue, 0.12f);
        Color border = room.isSafeZone ? safeGreen : Fade(WHITE, 0.28f);
        if (i == waveTargetRoomIndex && !rewardChestActive) {
            border = neonGold;
            fill = Fade(neonGold, 0.10f);
        }
        if (&room == currentArea) {
            border = neonCyan;
            fill = Fade(neonCyan, 0.18f);
        }
        DrawRectangleRec(mini, fill);
        DrawRectangleLinesEx(mini, 1.0f, border);
    }

    for (const auto& obstacle : dungeon.obstacles) {
        Rectangle mini = {
            mapRect.x + (obstacle.rect.x - minX) * scale,
            mapRect.y + (obstacle.rect.y - minY) * scale,
            obstacle.rect.width * scale,
            obstacle.rect.height * scale
        };
        DrawRectangleRec(mini, Fade(softRed, 0.30f));
    }

    int monsterDots = 0;
    for (const auto& monster : monsters) {
        Vector2 p = WorldToMap(monster.pos);
        DrawCircleV(p, monster.isBoss ? 3.8f : 2.4f, monster.isBoss ? neonGold : softRed);
        monsterDots++;
        if (monsterDots >= 36) {
            break;
        }
    }

    Vector2 playerMini = WorldToMap(player.pos);
    DrawCircleV(playerMini, 4.6f, WHITE);
    DrawCircleV(playerMini, 3.2f, neonCyan);
}

void Game::DrawRewardOverlay() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.62f));

    Rectangle panelRect = { screenW * 0.5f - 470.0f, screenH * 0.5f - 210.0f, 940.0f, 420.0f };
    DrawPanel(panelRect, panel2, neonGold);
    DrawText("DATA CACHE ACQUIRED", (int)panelRect.x + 28, (int)panelRect.y + 22, 30, neonGold);
    DrawText("CHOOSE ONE RELIC", (int)panelRect.x + 30, (int)panelRect.y + 58, 18, RAYWHITE);

    for (int i = 0; i < (int)rewardChoices.size(); ++i) {
        Rectangle card = { panelRect.x + 28.0f + i * 295.0f, panelRect.y + 108.0f, 265.0f, 250.0f };
        const RelicChoice& relic = rewardChoices[i];

        DrawRectangleGradientV((int)card.x, (int)card.y, (int)card.width, (int)card.height, Fade(relic.color, 0.18f), Fade(BLACK, 0.08f));
        DrawRectangleLinesEx(card, 3.0f, relic.color);
        DrawCircleV({ card.x + card.width * 0.5f, card.y + 62.0f }, 26.0f, relic.color);
        DrawCircleV({ card.x + card.width * 0.5f, card.y + 62.0f }, 12.0f, WHITE);
        DrawText(relic.name.c_str(), (int)card.x + 18, (int)card.y + 108, 24, relic.color);
        DrawText(relic.description.c_str(), (int)card.x + 18, (int)card.y + 150, 20, RAYWHITE);
        DrawText(TextFormat("PRESS %d", i + 1), (int)card.x + 18, (int)card.y + 210, 22, WHITE);
    }
}

void Game::DrawShop() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.55f));

    Rectangle panelRect = { screenW / 2.0f - 390.0f, screenH / 2.0f - 240.0f, 780.0f, 480.0f };
    DrawPanel(panelRect, panel2, neonGold);

    DrawText("NEXUS ARSENAL", (int)panelRect.x + 24, (int)panelRect.y + 22, 28, neonGold);
    DrawText(TextFormat("XP AVAILABLE: %d", player.xp), (int)panelRect.x + 540, (int)panelRect.y + 28, 20, safeGreen);

    int hpUpgradeCost = 100 + player.hpUpgradeLevel * 90;
    DrawPanel({ panelRect.x + 24.0f, panelRect.y + 72.0f, 732.0f, 94.0f }, panel, safeGreen);
    DrawText("BIOMETRIC UPGRADES", (int)panelRect.x + 40, (int)panelRect.y + 88, 22, safeGreen);
    DrawText(TextFormat("MAX HP BOOST +30   COST: %d XP   LEVEL: %d", hpUpgradeCost, player.hpUpgradeLevel), (int)panelRect.x + 40, (int)panelRect.y + 122, 20, WHITE);
    DrawText("PRESS H TO PURCHASE", (int)panelRect.x + 520, (int)panelRect.y + 122, 20, neonCyan);

    const Weapon& browseWeapon = weaponDB[shop.browseWeaponIdx];
    DrawPanel({ panelRect.x + 24.0f, panelRect.y + 186.0f, 732.0f, 188.0f }, panel, browseWeapon.color);
    DrawText("WEAPON VAULT", (int)panelRect.x + 40, (int)panelRect.y + 204, 22, browseWeapon.color);
    DrawText(browseWeapon.name.c_str(), (int)panelRect.x + 40, (int)panelRect.y + 238, 30, WHITE);
    DrawText(TextFormat("RARITY: %s", browseWeapon.rarity.c_str()), (int)panelRect.x + 40, (int)panelRect.y + 278, 20, browseWeapon.color);
    DrawText(TextFormat("DAMAGE: %d", browseWeapon.damage), (int)panelRect.x + 260, (int)panelRect.y + 278, 20, WHITE);
    DrawText(TextFormat("RANGE: %.0f", browseWeapon.range), (int)panelRect.x + 430, (int)panelRect.y + 278, 20, WHITE);
    DrawText(TextFormat("COST: %d XP", browseWeapon.cost), (int)panelRect.x + 600, (int)panelRect.y + 278, 20, neonGold);

    std::string stateLabel = shop.ownedWeapons[shop.browseWeaponIdx]
        ? (player.equippedWeaponIdx == shop.browseWeaponIdx ? "EQUIPPED" : "OWNED")
        : "UNOWNED";
    Color stateColor = shop.ownedWeapons[shop.browseWeaponIdx]
        ? (player.equippedWeaponIdx == shop.browseWeaponIdx ? neonCyan : safeGreen)
        : softRed;

    DrawText(TextFormat("STATUS: %s", stateLabel.c_str()), (int)panelRect.x + 40, (int)panelRect.y + 318, 22, stateColor);
    DrawText("LEFT / RIGHT TO BROWSE   B TO BUY OR EQUIP", (int)panelRect.x + 300, (int)panelRect.y + 318, 20, RAYWHITE);

    DrawPanel({ panelRect.x + 24.0f, panelRect.y + 392.0f, 732.0f, 56.0f }, panel, neonBlue);
    DrawText("E CLOSE", (int)panelRect.x + 40, (int)panelRect.y + 408, 20, neonBlue);
    DrawText("BUY ONCE, EQUIP ANYTIME", (int)panelRect.x + 250, (int)panelRect.y + 408, 20, RAYWHITE);

    if (shop.messageTimer > 0.0f) {
        DrawText(shop.message.c_str(), (int)panelRect.x + 560, (int)panelRect.y + 408, 20, shop.messageColor);
    }
}

void Game::DrawGameOver() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.72f));
    DrawText("SIGNAL LOST", screenW / 2 - MeasureText("SIGNAL LOST", 56) / 2, 180, 56, softRed);
    DrawText(TextFormat("WAVE REACHED: %d", player.wave), screenW / 2 - MeasureText(TextFormat("WAVE REACHED: %d", player.wave), 28) / 2, 280, 28, WHITE);
    DrawText(TextFormat("TOTAL KILLS: %d", player.kills), screenW / 2 - MeasureText(TextFormat("TOTAL KILLS: %d", player.kills), 28) / 2, 320, 28, WHITE);
    DrawText(TextFormat("XP BANKED: %d", player.xp), screenW / 2 - MeasureText(TextFormat("XP BANKED: %d", player.xp), 28) / 2, 360, 28, neonGold);
    DrawText("PRESS ENTER TO REBOOT RUN", screenW / 2 - MeasureText("PRESS ENTER TO REBOOT RUN", 26) / 2, 450, 26, neonCyan);
}

void Game::DrawEnemySprite(const ActiveMonster& monster) const {
    float t = (float)GetTime();
    DrawCartoonShadow({ monster.pos.x, monster.pos.y + monster.radius + 6.0f }, monster.radius * 0.85f, monster.radius * 0.30f, 0.20f);

    if (actorAtlas.id != 0) {
        int spriteIndex = monster.isBoss ? 5 : 1 + (monster.typeIndex % 4);
        float size = monster.isBoss ? 96.0f : 72.0f;
        Rectangle src = ActorSourceRect(spriteIndex);
        Rectangle dst = { monster.pos.x - size * 0.5f, monster.pos.y - size * 0.70f, size, size };
        DrawTexturePro(actorAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, (monster.hitFlash > 0.0f) ? WHITE : WHITE);
    }
    else {
        DrawCircleV(monster.pos, monster.radius, (monster.hitFlash > 0.0f) ? WHITE : monster.color);
    }

    if (monster.isElite) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 6.0f + std::sin(t * 5.0f) * 1.5f, Fade(neonPink, 0.30f));
    }

    if (monster.isBoss) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 12.0f + std::sin(t * 3.0f) * 2.0f, Fade(neonGold, 0.34f));
        DrawText(monster.name.c_str(), (int)monster.pos.x - MeasureText(monster.name.c_str(), 16) / 2, (int)(monster.pos.y - monster.radius - 42.0f), 16, neonGold);
    }
}
