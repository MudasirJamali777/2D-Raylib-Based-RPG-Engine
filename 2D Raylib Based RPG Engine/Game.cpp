#include "Game.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cstdio>
#include <random>

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

    camera = { 0 };
    camera.offset = { screenW * 0.5f, screenH * 0.5f };
    camera.target = player.pos;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    ResetRun();
    gameState = GameState::Title;
}

Game::~Game() {
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
    bg = { 7, 10, 18, 255 };
    bg2 = { 13, 18, 30, 255 };
    grid = { 22, 31, 52, 255 };
    panel = { 10, 14, 28, 235 };
    panel2 = { 14, 20, 38, 245 };
    neonCyan = { 0, 255, 245, 255 };
    neonBlue = { 72, 145, 255, 255 };
    neonPink = { 255, 65, 185, 255 };
    neonGold = { 255, 210, 80, 255 };
    softRed = { 255, 90, 120, 255 };
    safeGreen = { 90, 255, 160, 255 };
    bossPurple = { 182, 76, 255, 255 };
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
    int titleWidth = MeasureText("NEON ABYSS", 72);
    DrawText("NEON ABYSS", screenW / 2 - titleWidth / 2, 110, 72, neonCyan);
    DrawText("NEXUS SIEGE", screenW / 2 - MeasureText("NEXUS SIEGE", 28) / 2, 190, 28, neonPink);
    DrawText("A top-down cyber action RPG vertical slice", screenW / 2 - MeasureText("A top-down cyber action RPG vertical slice", 22) / 2, 238, 22, RAYWHITE);

    DrawPanel({ screenW / 2.0f - 340.0f, 300.0f, 680.0f, 210.0f }, panel, neonBlue);
    DrawText("FEATURES", screenW / 2 - MeasureText("FEATURES", 24) / 2, 322, 24, neonGold);
    DrawText("- Fast WASD movement with dash, EMP, and sentry turret", screenW / 2 - 285, 365, 20, RAYWHITE);
    DrawText("- Wave survival with elite enemies and boss encounters", screenW / 2 - 285, 395, 20, RAYWHITE);
    DrawText("- 17-weapon arsenal, safe-zone merchant, and permanent upgrades", screenW / 2 - 285, 425, 20, RAYWHITE);
    DrawText("- Multi-room dungeon layout, corridors, collision walls, and room pressure", screenW / 2 - 285, 455, 20, RAYWHITE);

    Color pulse = ((int)(GetTime() * 2.5) % 2 == 0) ? neonCyan : WHITE;
    DrawText("PRESS ENTER TO DEPLOY A NEW RUN", screenW / 2 - MeasureText("PRESS ENTER TO DEPLOY A NEW RUN", 26) / 2, 548, 26, pulse);

    if (HasSaveFile()) {
        DrawText("PRESS C TO CONTINUE SAVED RUN", screenW / 2 - MeasureText("PRESS C TO CONTINUE SAVED RUN", 22) / 2, 586, 22, neonGold);
    }

    DrawText("ESC closes the game", screenW / 2 - MeasureText("ESC closes the game", 16) / 2, 626, 16, GRAY);
}

void Game::DrawWorld() const {
    BeginMode2D(camera);

    const int gridSize = 64;
    int startX = ((int)camera.target.x - screenW) / gridSize * gridSize - gridSize;
    int endX = ((int)camera.target.x + screenW) / gridSize * gridSize + gridSize;
    int startY = ((int)camera.target.y - screenH) / gridSize * gridSize - gridSize;
    int endY = ((int)camera.target.y + screenH) / gridSize * gridSize + gridSize;

    for (int x = startX; x <= endX; x += gridSize) {
        DrawLine(x, startY, x, endY, Fade(grid, x % (gridSize * 4) == 0 ? 0.85f : 0.45f));
    }
    for (int y = startY; y <= endY; y += gridSize) {
        DrawLine(startX, y, endX, y, Fade(grid, y % (gridSize * 4) == 0 ? 0.85f : 0.45f));
    }

    for (const auto& corridor : dungeon.corridors) {
        DrawRectangleGradientV((int)corridor.x, (int)corridor.y, (int)corridor.width, (int)corridor.height, Fade(neonBlue, 0.10f), Fade(BLACK, 0.04f));
        DrawRectangleLinesEx(corridor, 2.0f, Fade(neonBlue, 0.40f));

        if (corridor.width > corridor.height) {
            for (float x = corridor.x + 26.0f; x < corridor.x + corridor.width - 26.0f; x += 54.0f) {
                DrawLineEx({ x, corridor.y + 12.0f }, { x + 18.0f, corridor.y + 12.0f }, 2.0f, Fade(neonBlue, 0.38f));
                DrawLineEx({ x, corridor.y + corridor.height - 12.0f }, { x + 18.0f, corridor.y + corridor.height - 12.0f }, 2.0f, Fade(neonBlue, 0.38f));
            }
        }
        else {
            for (float y = corridor.y + 26.0f; y < corridor.y + corridor.height - 26.0f; y += 54.0f) {
                DrawLineEx({ corridor.x + 12.0f, y }, { corridor.x + 12.0f, y + 18.0f }, 2.0f, Fade(neonBlue, 0.38f));
                DrawLineEx({ corridor.x + corridor.width - 12.0f, y }, { corridor.x + corridor.width - 12.0f, y + 18.0f }, 2.0f, Fade(neonBlue, 0.38f));
            }
        }
    }

    for (const auto& room : dungeon.rooms) {
        Color roomBorder = room.isSafeZone ? safeGreen : neonBlue;
        DrawRectangleGradientV((int)room.rect.x, (int)room.rect.y, (int)room.rect.width, (int)room.rect.height,
            room.isSafeZone ? Fade(safeGreen, 0.11f) : Fade(neonBlue, 0.08f), Fade(BLACK, 0.06f));
        DrawRectangleLinesEx(room.rect, room.isSafeZone ? 3.0f : 2.0f, Fade(roomBorder, 0.86f));

        Rectangle inner = ShrinkRect(room.rect, 18.0f);
        Rectangle frame = ShrinkRect(room.rect, 42.0f);
        if (inner.width > 0.0f && inner.height > 0.0f) {
            DrawRectangleLinesEx(inner, 1.0f, Fade(roomBorder, 0.20f));
        }
        if (frame.width > 0.0f && frame.height > 0.0f) {
            DrawRectangleLinesEx(frame, 1.0f, Fade(roomBorder, 0.10f));
        }

        for (float x = room.rect.x + 44.0f; x < room.rect.x + room.rect.width - 44.0f; x += 72.0f) {
            DrawLineEx({ x, room.rect.y + 20.0f }, { x, room.rect.y + room.rect.height - 20.0f }, 1.0f, Fade(roomBorder, 0.06f));
        }
        for (float y = room.rect.y + 44.0f; y < room.rect.y + room.rect.height - 44.0f; y += 72.0f) {
            DrawLineEx({ room.rect.x + 20.0f, y }, { room.rect.x + room.rect.width - 20.0f, y }, 1.0f, Fade(roomBorder, 0.05f));
        }

        DrawLineEx({ room.rect.x + 10.0f, room.rect.y + 10.0f }, { room.rect.x + 56.0f, room.rect.y + 10.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + 10.0f, room.rect.y + 10.0f }, { room.rect.x + 10.0f, room.rect.y + 56.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + room.rect.width - 10.0f, room.rect.y + 10.0f }, { room.rect.x + room.rect.width - 56.0f, room.rect.y + 10.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + room.rect.width - 10.0f, room.rect.y + 10.0f }, { room.rect.x + room.rect.width - 10.0f, room.rect.y + 56.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + 10.0f, room.rect.y + room.rect.height - 10.0f }, { room.rect.x + 56.0f, room.rect.y + room.rect.height - 10.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + 10.0f, room.rect.y + room.rect.height - 10.0f }, { room.rect.x + 10.0f, room.rect.y + room.rect.height - 56.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + room.rect.width - 10.0f, room.rect.y + room.rect.height - 10.0f }, { room.rect.x + room.rect.width - 56.0f, room.rect.y + room.rect.height - 10.0f }, 3.0f, Fade(roomBorder, 0.6f));
        DrawLineEx({ room.rect.x + room.rect.width - 10.0f, room.rect.y + room.rect.height - 10.0f }, { room.rect.x + room.rect.width - 10.0f, room.rect.y + room.rect.height - 56.0f }, 3.0f, Fade(roomBorder, 0.6f));

        int labelSize = room.isSafeZone ? 24 : 18;
        int labelX = (int)(room.rect.x + room.rect.width * 0.5f) - MeasureText(room.name.c_str(), labelSize) / 2;
        int labelY = (int)(room.rect.y + 18.0f);
        DrawText(room.name.c_str(), labelX, labelY, labelSize, room.isSafeZone ? safeGreen : RAYWHITE);

        for (int i = 0; i < 4; ++i) {
            Vector2 corner = {
                (i == 0 || i == 2) ? room.rect.x + 28.0f : room.rect.x + room.rect.width - 28.0f,
                (i == 0 || i == 1) ? room.rect.y + 28.0f : room.rect.y + room.rect.height - 28.0f
            };
            DrawCircleV(corner, room.isSafeZone ? 7.0f : 5.0f, Fade(roomBorder, room.isSafeZone ? 0.45f : 0.25f));
        }
    }

    for (const auto& obstacle : dungeon.obstacles) {
        DrawRectangleRec(obstacle.rect, obstacle.color);
        DrawRectangleLinesEx(obstacle.rect, 2.0f, Fade(WHITE, 0.18f));

        Rectangle core = ShrinkRect(obstacle.rect, 8.0f);
        if (core.width > 0.0f && core.height > 0.0f) {
            DrawRectangleRec(core, Fade(BLACK, 0.28f));
        }

        if (obstacle.rect.width >= obstacle.rect.height) {
            for (float x = obstacle.rect.x + 12.0f; x < obstacle.rect.x + obstacle.rect.width - 12.0f; x += 24.0f) {
                DrawLineEx({ x, obstacle.rect.y + obstacle.rect.height - 10.0f }, { x + 12.0f, obstacle.rect.y + 10.0f }, 2.0f, Fade(neonGold, 0.25f));
            }
        }
        else {
            for (float y = obstacle.rect.y + 12.0f; y < obstacle.rect.y + obstacle.rect.height - 12.0f; y += 24.0f) {
                DrawLineEx({ obstacle.rect.x + 10.0f, y + 12.0f }, { obstacle.rect.x + obstacle.rect.width - 10.0f, y }, 2.0f, Fade(neonGold, 0.25f));
            }
        }

        Vector2 obstacleCore = { obstacle.rect.x + obstacle.rect.width * 0.5f, obstacle.rect.y + obstacle.rect.height * 0.5f };
        DrawGlowCircle(obstacleCore, 6.0f, Fade(neonCyan, 0.65f));
        DrawCircleV(obstacleCore, 4.0f, neonCyan);
    }

    Vector2 safeCenter = SafeZoneCenter(safeZone);
    float pulse = 150.0f + std::sin((float)GetTime() * 2.0f) * 10.0f;
    DrawCircleLines((int)safeCenter.x, (int)safeCenter.y, pulse, Fade(safeGreen, 0.28f));
    DrawCircleLines((int)safeCenter.x, (int)safeCenter.y, pulse + 32.0f, Fade(safeGreen, 0.16f));
    DrawText("NEXUS", (int)safeCenter.x - 36, (int)safeCenter.y - 10, 22, safeGreen);

    for (const auto& barrier : GetActiveBarrierRects()) {
        DrawRectangleRec(barrier, Fade(softRed, 0.20f));
        DrawRectangleLinesEx(barrier, 3.0f, Fade(softRed, 0.92f));
        for (float i = 8.0f; i < ((barrier.width > barrier.height) ? barrier.width : barrier.height); i += 24.0f) {
            if (barrier.width > barrier.height) {
                DrawLineEx({ barrier.x + i, barrier.y + 5.0f }, { barrier.x + i + 12.0f, barrier.y + barrier.height - 5.0f }, 3.0f, Fade(neonGold, 0.45f));
            }
            else {
                DrawLineEx({ barrier.x + 5.0f, barrier.y + i }, { barrier.x + barrier.width - 5.0f, barrier.y + i + 12.0f }, 3.0f, Fade(neonGold, 0.45f));
            }
        }
    }

    for (const auto& shockwave : shockwaves) {
        float alpha = ClampFloat(shockwave.life / shockwave.maxLife, 0.0f, 1.0f);
        DrawCircleLines((int)shockwave.pos.x, (int)shockwave.pos.y, shockwave.radius, Fade(shockwave.color, alpha));
        DrawCircleLines((int)shockwave.pos.x, (int)shockwave.pos.y, shockwave.radius + 4.0f, Fade(shockwave.color, alpha * 0.45f));
    }

    for (const auto& turret : turrets) {
        DrawGlowCircle(turret.pos, 12.0f, neonBlue);
        DrawCircleV(turret.pos, 12.0f, neonBlue);
        DrawCircleV(turret.pos, 5.0f, WHITE);
        DrawCircleLines((int)turret.pos.x, (int)turret.pos.y, 260.0f, Fade(neonBlue, 0.15f));
    }

    for (const auto& beam : beams) {
        DrawLineEx(beam.start, beam.end, beam.thickness + 3.0f, Fade(beam.color, beam.life * 3.0f));
        DrawLineEx(beam.start, beam.end, beam.thickness, WHITE);
    }

    for (const auto& orb : orbs) {
        DrawGlowCircle(orb.pos, 6.0f, orb.color);
        DrawCircleV(orb.pos, 4.0f, WHITE);
    }

    for (const auto& pickup : healthPickups) {
        Vector2 bobPos = { pickup.pos.x, pickup.pos.y + std::sin(pickup.spin) * 4.0f };
        DrawGlowCircle(bobPos, 10.0f, safeGreen);
        DrawCircleV(bobPos, 9.0f, safeGreen);
        DrawRectangle((int)bobPos.x - 2, (int)bobPos.y - 6, 4, 12, WHITE);
        DrawRectangle((int)bobPos.x - 6, (int)bobPos.y - 2, 12, 4, WHITE);
    }

    if (rewardChestActive) {
        float chestPulse = std::sin((float)GetTime() * 4.0f) * 4.0f;
        DrawGlowCircle(rewardChestPos, 26.0f + chestPulse, neonGold);
        DrawRectangleRounded({ rewardChestPos.x - 24.0f, rewardChestPos.y - 16.0f, 48.0f, 32.0f }, 0.25f, 6, neonGold);
        DrawRectangleRounded({ rewardChestPos.x - 18.0f, rewardChestPos.y - 10.0f, 36.0f, 20.0f }, 0.25f, 6, Fade(BLACK, 0.25f));
        DrawRectangle((int)rewardChestPos.x - 3, (int)rewardChestPos.y - 12, 6, 24, WHITE);
        DrawCircleLines((int)rewardChestPos.x, (int)rewardChestPos.y, 42.0f + chestPulse, Fade(neonGold, 0.35f));
    }

    for (const auto& monster : monsters) {
        DrawEnemySprite(monster);

        Rectangle back = { monster.pos.x - 24.0f, monster.pos.y - monster.radius - 16.0f, 48.0f, 6.0f };
        DrawRectangleRec(back, Fade(BLACK, 0.7f));
        DrawRectangleRec({ back.x, back.y, back.width * ((float)monster.hp / (float)monster.maxHp), back.height }, monster.isBoss ? neonGold : safeGreen);

        if (monster.isBoss) {
            DrawText(monster.name.c_str(), (int)monster.pos.x - MeasureText(monster.name.c_str(), 16) / 2, (int)(monster.pos.y - monster.radius - 38.0f), 16, neonGold);
        }
    }

    for (const auto& particle : particles) {
        float alpha = ClampFloat(particle.life / particle.maxLife, 0.0f, 1.0f);
        DrawCircleV(particle.pos, particle.size + 2.0f, Fade(particle.color, alpha * 0.18f));
        DrawCircleV(particle.pos, particle.size, Fade(particle.color, alpha));
    }

    Color playerColor = (player.hitFlash > 0.0f) ? WHITE : neonCyan;
    float playerPulse = 22.0f + std::sin((float)GetTime() * 6.0f) * 1.8f;
    DrawEllipse((int)player.pos.x, (int)(player.pos.y + 22.0f), 20, 8, Fade(BLACK, 0.26f));
    DrawGlowCircle(player.pos, playerPulse, playerColor);
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, 28.0f, Fade(playerColor, 0.22f));
    DrawCircleV(player.pos, 21.0f, playerColor);
    DrawCircleV(player.pos, 10.0f, WHITE);
    DrawLineEx(player.pos, VecAdd(player.pos, VecScale(player.aimDir, 34.0f)), 5.0f, neonPink);
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, weaponDB[player.equippedWeaponIdx].range, Fade(weaponDB[player.equippedWeaponIdx].color, 0.12f));

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
    Color body = (monster.hitFlash > 0.0f) ? WHITE : monster.color;
    Color core = Fade(WHITE, 0.92f);
    float t = (float)GetTime();
    float wobble = std::sin(t * 4.0f + monster.pos.x * 0.01f) * 2.0f;

    DrawEllipse((int)monster.pos.x, (int)(monster.pos.y + monster.radius + 6.0f), (int)(monster.radius * 0.85f), (int)(monster.radius * 0.34f), Fade(BLACK, 0.28f));
    DrawGlowCircle(monster.pos, monster.radius + (monster.isBoss ? 8.0f : 4.0f), body);

    switch (monster.typeIndex % 4) {
    case 0:
        DrawCircleV(monster.pos, monster.radius + wobble * 0.08f, body);
        DrawCircleV(monster.pos, monster.radius * 0.52f, Fade(BLACK, 0.18f));
        DrawCircleV(monster.pos, monster.radius * 0.34f, core);
        break;
    case 1:
        DrawRectanglePro(
            Rectangle{ monster.pos.x - monster.radius, monster.pos.y - monster.radius, monster.radius * 2.0f, monster.radius * 2.0f },
            { monster.radius, monster.radius },
            45.0f + wobble,
            body
        );
        DrawRectanglePro(
            Rectangle{ monster.pos.x - monster.radius * 0.55f, monster.pos.y - monster.radius * 0.55f, monster.radius * 1.1f, monster.radius * 1.1f },
            { monster.radius * 0.55f, monster.radius * 0.55f },
            45.0f + wobble,
            Fade(BLACK, 0.18f)
        );
        DrawCircleV(monster.pos, monster.radius * 0.26f, core);
        break;
    case 2:
        DrawTriangle(
            { monster.pos.x, monster.pos.y - monster.radius * 1.15f },
            { monster.pos.x - monster.radius * 1.02f, monster.pos.y + monster.radius * 0.92f },
            { monster.pos.x + monster.radius * 1.02f, monster.pos.y + monster.radius * 0.92f },
            body
        );
        DrawTriangle(
            { monster.pos.x, monster.pos.y - monster.radius * 0.50f },
            { monster.pos.x - monster.radius * 0.42f, monster.pos.y + monster.radius * 0.30f },
            { monster.pos.x + monster.radius * 0.42f, monster.pos.y + monster.radius * 0.30f },
            Fade(BLACK, 0.18f)
        );
        DrawCircleV(monster.pos, monster.radius * 0.22f, core);
        break;
    default:
        DrawPoly(monster.pos, 6, monster.radius, t * 45.0f, body);
        DrawPoly(monster.pos, 6, monster.radius * 0.62f, -t * 60.0f, Fade(BLACK, 0.16f));
        DrawCircleV(monster.pos, monster.radius * 0.25f, core);
        break;
    }

    Vector2 leftEye = { monster.pos.x - monster.radius * 0.30f, monster.pos.y - monster.radius * 0.08f };
    Vector2 rightEye = { monster.pos.x + monster.radius * 0.30f, monster.pos.y - monster.radius * 0.08f };
    DrawCircleV(leftEye, monster.isBoss ? 4.0f : 2.6f, WHITE);
    DrawCircleV(rightEye, monster.isBoss ? 4.0f : 2.6f, WHITE);

    if (monster.isElite) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 6.0f + std::sin(t * 5.0f) * 1.5f, Fade(neonPink, 0.45f));
    }

    if (monster.isBoss) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 8.0f + std::sin(t * 3.0f) * 2.0f, Fade(neonGold, 0.45f));
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 18.0f + std::sin(t * 2.0f) * 3.0f, Fade(body, 0.22f));
    }

    DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 1.0f, Fade(WHITE, 0.6f));
}
