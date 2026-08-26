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
    InitWindow(screenW, screenH, "CROWNHEART // KINGDOM SIEGE");

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
    SetTargetFPS(60);

    BuildColorTheme();
    BuildDatabases();
    LoadProfile();
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
        {"Rusty Cudgel", 14, 65.0f, "Common", 0, LIGHTGRAY},
        {"Iron Mace", 18, 72.0f, "Common", 60, GRAY},
        {"Pilgrim's Hatchet", 24, 82.0f, "Common", 120, neonCyan},
        {"Raider Flail", 34, 88.0f, "Common", 200, ORANGE},
        {"Hearthblade", 44, 96.0f, "Rare", 300, SKYBLUE},
        {"Knight Saber", 56, 104.0f, "Rare", 430, BLUE},
        {"Sunsteel Blade", 70, 112.0f, "Rare", 620, RED},
        {"Storm Fang", 86, 118.0f, "Rare", 850, LIME},
        {"Raven Pike", 102, 126.0f, "Rare", 1100, neonCyan},
        {"Grave Reaper", 126, 138.0f, "Legendary", 1450, PURPLE},
        {"Arc Halberd", 152, 146.0f, "Legendary", 1850, YELLOW},
        {"Starforged Brand", 184, 154.0f, "Legendary", 2350, VIOLET},
        {"King's Greatsword", 220, 164.0f, "Legendary", 3000, GOLD},
        {"Wyrmtooth Cleaver", 262, 174.0f, "Exotic", 3800, GREEN},
        {"Moonveil Edge", 308, 188.0f, "Exotic", 4800, bossPurple},
        {"Celestial Cleaver", 370, 205.0f, "Exotic", 6200, WHITE},
        {"Crownfall", 460, 225.0f, "Exotic", 8200, neonPink}
    };

    monsterTypes = {
        {"Briar Rat", 45, 8, 2.5f, 12.0f, GRAY, false, 18},
        {"Marsh Sprite", 60, 10, 3.0f, 14.0f, neonCyan, false, 24},
        {"Road Brigand", 88, 14, 2.1f, 18.0f, ORANGE, false, 34},
        {"Redfang Marauder", 130, 18, 1.6f, 22.0f, RED, false, 48},
        {"Grave Warden", 175, 24, 1.8f, 20.0f, PURPLE, false, 64},
        {"Hollow Stalker", 110, 22, 3.4f, 15.0f, LIME, false, 56},
        {"Stonehide Ogre", 280, 30, 1.15f, 28.0f, DARKBLUE, false, 80},
        {"Ashen Brute", 220, 28, 1.4f, 25.0f, PINK, false, 72},
        {"Mire Wisp", 145, 20, 2.8f, 17.0f, SKYBLUE, false, 60},
        {"Burrow Skitter", 95, 16, 2.9f, 13.0f, BROWN, false, 40},
        {"THE ASHEN WYRM", 1100, 36, 1.3f, 42.0f, MAROON, true, 550},
        {"THE THORN KING", 1800, 48, 1.05f, 48.0f, bossPurple, true, 900}
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
        { { -480.0f, -360.0f, 960.0f, 720.0f }, "CROWNHEART KEEP", true, false },
        { { 760.0f, -300.0f, 760.0f, 600.0f }, "SUNFORGE COURT", false, true },
        { { -1520.0f, -300.0f, 760.0f, 600.0f }, "BRAMBLE WARD", false, true },
        { { -360.0f, -1360.0f, 720.0f, 560.0f }, "MOONSPRING ASCENT", false, true },
        { { -360.0f, 800.0f, 720.0f, 560.0f }, "EMBER ROAD", false, true },
        { { 760.0f, -1260.0f, 760.0f, 520.0f }, "WYVERN WATCH", false, true },
        { { 760.0f, 740.0f, 760.0f, 520.0f }, "GILDED FIELDS", false, true },
        { { -1520.0f, -1260.0f, 760.0f, 520.0f }, "WHISPER GROVE", false, true },
        { { -1520.0f, 740.0f, 760.0f, 520.0f }, "ROOTGLEN HOLD", false, true },
        { { 180.0f, -820.0f, 320.0f, 240.0f }, "PILGRIM'S NOOK", false, false },
        { { -560.0f, 470.0f, 320.0f, 220.0f }, "MOSSY CELLAR", false, false },
        { { 1560.0f, -1040.0f, 300.0f, 220.0f }, "FALCONER'S LEDGE", false, false },
        { { -1860.0f, 900.0f, 300.0f, 220.0f }, "SMUGGLER'S COVE", false, false }
    };

    dungeon.corridors = {
        { 420.0f, -110.0f, 420.0f, 220.0f },
        { -840.0f, -110.0f, 420.0f, 220.0f },
        { -110.0f, -860.0f, 220.0f, 560.0f },
        { -110.0f, 300.0f, 220.0f, 560.0f },
        { 1040.0f, -860.0f, 200.0f, 620.0f },
        { 1040.0f, 240.0f, 200.0f, 620.0f },
        { -1240.0f, -860.0f, 200.0f, 620.0f },
        { -1240.0f, 240.0f, 200.0f, 620.0f },
        { 110.0f, -710.0f, 110.0f, 120.0f },
        { -240.0f, 520.0f, 130.0f, 120.0f },
        { 1490.0f, -970.0f, 140.0f, 80.0f },
        { -1600.0f, 970.0f, 100.0f, 80.0f }
    };

    auto AddObstacle = [&](Rectangle rect, Color color, int spriteIndex, float spriteScale, std::initializer_list<Rectangle> colliders) {
        DungeonObstacle obstacle{};
        obstacle.rect = rect;
        obstacle.color = color;
        obstacle.spriteIndex = spriteIndex;
        obstacle.spriteScale = spriteScale;
        obstacle.colliders.assign(colliders.begin(), colliders.end());
        dungeon.obstacles.push_back(obstacle);
        };

    auto TowerCollider = [&](Rectangle rect) {
        return Rectangle{ rect.x + rect.width * 0.31f, rect.y + rect.height * 0.60f, rect.width * 0.38f, rect.height * 0.28f };
        };
    auto SignCollider = [&](Rectangle rect) {
        return Rectangle{ rect.x + rect.width * 0.38f, rect.y + rect.height * 0.48f, rect.width * 0.18f, rect.height * 0.34f };
        };
    auto TreeTrunk = [&](Rectangle rect) {
        return Rectangle{ rect.x + rect.width * 0.39f, rect.y + rect.height * 0.58f, rect.width * 0.20f, rect.height * 0.28f };
        };
    auto TreeRoots = [&](Rectangle rect) {
        return Rectangle{ rect.x + rect.width * 0.31f, rect.y + rect.height * 0.72f, rect.width * 0.36f, rect.height * 0.12f };
        };
    auto RockCollider = [&](Rectangle rect) {
        return Rectangle{ rect.x + rect.width * 0.22f, rect.y + rect.height * 0.46f, rect.width * 0.54f, rect.height * 0.24f };
        };
    auto HutCollider = [&](Rectangle rect) {
        return Rectangle{ rect.x + rect.width * 0.27f, rect.y + rect.height * 0.48f, rect.width * 0.42f, rect.height * 0.28f };
        };

    AddObstacle({ -96.0f, -44.0f, 192.0f, 168.0f }, { 114, 101, 85, 255 }, 5, 1.82f, {
        { -72.0f, 34.0f, 30.0f, 60.0f },
        { -22.0f, 54.0f, 44.0f, 42.0f },
        { 42.0f, 34.0f, 30.0f, 60.0f }
        });
    AddObstacle({ -258.0f, 46.0f, 76.0f, 118.0f }, { 116, 104, 91, 255 }, 4, 1.06f, { TowerCollider({ -258.0f, 46.0f, 76.0f, 118.0f }) });
    AddObstacle({ 182.0f, 46.0f, 76.0f, 118.0f }, { 116, 104, 91, 255 }, 4, 1.06f, { TowerCollider({ 182.0f, 46.0f, 76.0f, 118.0f }) });
    AddObstacle({ -328.0f, 136.0f, 42.0f, 62.0f }, { 120, 100, 62, 255 }, 9, 0.92f, { SignCollider({ -328.0f, 136.0f, 42.0f, 62.0f }) });
    AddObstacle({ 286.0f, 136.0f, 42.0f, 62.0f }, { 120, 100, 62, 255 }, 9, 0.92f, { SignCollider({ 286.0f, 136.0f, 42.0f, 62.0f }) });

    AddObstacle({ 930.0f, -186.0f, 92.0f, 126.0f }, { 69, 125, 70, 255 }, 0, 1.18f, { TreeTrunk({ 930.0f, -186.0f, 92.0f, 126.0f }), TreeRoots({ 930.0f, -186.0f, 92.0f, 126.0f }) });
    AddObstacle({ 1162.0f, -34.0f, 96.0f, 78.0f }, { 116, 112, 104, 255 }, 2, 0.92f, { RockCollider({ 1162.0f, -34.0f, 96.0f, 78.0f }) });
    AddObstacle({ 1272.0f, 92.0f, 110.0f, 104.0f }, { 118, 98, 78, 255 }, 3, 0.98f, { HutCollider({ 1272.0f, 92.0f, 110.0f, 104.0f }) });

    AddObstacle({ -1282.0f, -186.0f, 92.0f, 126.0f }, { 69, 125, 70, 255 }, 0, 1.18f, { TreeTrunk({ -1282.0f, -186.0f, 92.0f, 126.0f }), TreeRoots({ -1282.0f, -186.0f, 92.0f, 126.0f }) });
    AddObstacle({ -1058.0f, 42.0f, 96.0f, 78.0f }, { 116, 112, 104, 255 }, 2, 0.92f, { RockCollider({ -1058.0f, 42.0f, 96.0f, 78.0f }) });
    AddObstacle({ -1410.0f, 88.0f, 110.0f, 104.0f }, { 118, 98, 78, 255 }, 3, 0.98f, { HutCollider({ -1410.0f, 88.0f, 110.0f, 104.0f }) });

    AddObstacle({ -202.0f, -1216.0f, 88.0f, 122.0f }, { 69, 125, 70, 255 }, 0, 1.14f, { TreeTrunk({ -202.0f, -1216.0f, 88.0f, 122.0f }), TreeRoots({ -202.0f, -1216.0f, 88.0f, 122.0f }) });
    AddObstacle({ 94.0f, -1078.0f, 96.0f, 78.0f }, { 116, 112, 104, 255 }, 2, 0.92f, { RockCollider({ 94.0f, -1078.0f, 96.0f, 78.0f }) });
    AddObstacle({ -26.0f, -970.0f, 82.0f, 128.0f }, { 116, 104, 91, 255 }, 4, 1.00f, { TowerCollider({ -26.0f, -970.0f, 82.0f, 128.0f }) });

    AddObstacle({ -198.0f, 904.0f, 88.0f, 122.0f }, { 69, 125, 70, 255 }, 0, 1.14f, { TreeTrunk({ -198.0f, 904.0f, 88.0f, 122.0f }), TreeRoots({ -198.0f, 904.0f, 88.0f, 122.0f }) });
    AddObstacle({ 90.0f, 1040.0f, 96.0f, 78.0f }, { 116, 112, 104, 255 }, 2, 0.92f, { RockCollider({ 90.0f, 1040.0f, 96.0f, 78.0f }) });
    AddObstacle({ -30.0f, 1126.0f, 82.0f, 128.0f }, { 116, 104, 91, 255 }, 4, 1.00f, { TowerCollider({ -30.0f, 1126.0f, 82.0f, 128.0f }) });

    AddObstacle({ 906.0f, -1112.0f, 92.0f, 124.0f }, { 69, 125, 70, 255 }, 0, 1.16f, { TreeTrunk({ 906.0f, -1112.0f, 92.0f, 124.0f }), TreeRoots({ 906.0f, -1112.0f, 92.0f, 124.0f }) });
    AddObstacle({ 1228.0f, -976.0f, 112.0f, 100.0f }, { 118, 98, 78, 255 }, 3, 0.98f, { HutCollider({ 1228.0f, -976.0f, 112.0f, 100.0f }) });

    AddObstacle({ 904.0f, 852.0f, 92.0f, 124.0f }, { 69, 125, 70, 255 }, 0, 1.16f, { TreeTrunk({ 904.0f, 852.0f, 92.0f, 124.0f }), TreeRoots({ 904.0f, 852.0f, 92.0f, 124.0f }) });
    AddObstacle({ 1224.0f, 976.0f, 112.0f, 100.0f }, { 118, 98, 78, 255 }, 3, 0.98f, { HutCollider({ 1224.0f, 976.0f, 112.0f, 100.0f }) });

    AddObstacle({ -1374.0f, -1112.0f, 92.0f, 124.0f }, { 69, 125, 70, 255 }, 0, 1.16f, { TreeTrunk({ -1374.0f, -1112.0f, 92.0f, 124.0f }), TreeRoots({ -1374.0f, -1112.0f, 92.0f, 124.0f }) });
    AddObstacle({ -1098.0f, -974.0f, 112.0f, 100.0f }, { 118, 98, 78, 255 }, 3, 0.98f, { HutCollider({ -1098.0f, -974.0f, 112.0f, 100.0f }) });

    AddObstacle({ -1378.0f, 854.0f, 92.0f, 124.0f }, { 69, 125, 70, 255 }, 0, 1.16f, { TreeTrunk({ -1378.0f, 854.0f, 92.0f, 124.0f }), TreeRoots({ -1378.0f, 854.0f, 92.0f, 124.0f }) });
    AddObstacle({ -1102.0f, 974.0f, 112.0f, 100.0f }, { 118, 98, 78, 255 }, 3, 0.98f, { HutCollider({ -1102.0f, 974.0f, 112.0f, 100.0f }) });

    AddObstacle({ 286.0f, -748.0f, 92.0f, 124.0f }, { 69, 125, 70, 255 }, 0, 1.10f, { TreeTrunk({ 286.0f, -748.0f, 92.0f, 124.0f }), TreeRoots({ 286.0f, -748.0f, 92.0f, 124.0f }) });
    AddObstacle({ -432.0f, 540.0f, 112.0f, 100.0f }, { 118, 98, 78, 255 }, 3, 0.94f, { HutCollider({ -432.0f, 540.0f, 112.0f, 100.0f }) });
    AddObstacle({ 1678.0f, -976.0f, 86.0f, 120.0f }, { 116, 104, 91, 255 }, 4, 0.98f, { TowerCollider({ 1678.0f, -976.0f, 86.0f, 120.0f }) });
    AddObstacle({ -1768.0f, 964.0f, 96.0f, 78.0f }, { 116, 112, 104, 255 }, 2, 0.90f, { RockCollider({ -1768.0f, 964.0f, 96.0f, 78.0f }) });

    safeZone = dungeon.rooms.front().rect;
}

void Game::BuildTileMap() {
    int minX = 1000000;
    int minY = 1000000;
    int maxX = -1000000;
    int maxY = -1000000;

    auto includeRect = [&](Rectangle rect) {
        minX = std::min(minX, (int)rect.x);
        minY = std::min(minY, (int)rect.y);
        maxX = std::max(maxX, (int)(rect.x + rect.width));
        maxY = std::max(maxY, (int)(rect.y + rect.height));
        };

    for (const auto& room : dungeon.rooms) includeRect(room.rect);
    for (const auto& corridor : dungeon.corridors) includeRect(corridor);

    minX = (int)(std::floor((minX - 256) / 64.0f) * 64.0f);
    minY = (int)(std::floor((minY - 256) / 64.0f) * 64.0f);
    maxX = (int)(std::ceil((maxX + 256) / 64.0f) * 64.0f);
    maxY = (int)(std::ceil((maxY + 256) / 64.0f) * 64.0f);

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
            const DungeonArea* roomAtPoint = FindDungeonArea(dungeon, center);
            bool inCorridor = false;
            for (const auto& corridor : dungeon.corridors) {
                if (CheckCollisionPointRec(center, corridor)) {
                    inCorridor = true;
                    break;
                }
            }

            int tile = (int)TileType::Grass;
            if (inCorridor) {
                tile = (int)TileType::Path;
            }
            else if (roomAtPoint != nullptr) {
                Vector2 roomCenter = SafeZoneCenter(roomAtPoint->rect);
                tile = ((x + y) % 5 == 0) ? (int)TileType::GrassAlt : (int)TileType::Grass;

                if (roomAtPoint->isSafeZone) {
                    if (std::fabs(center.x - roomCenter.x) < 120.0f || std::fabs(center.y - roomCenter.y) < 120.0f || Distance(center, roomCenter) < 148.0f) {
                        tile = (int)TileType::Path;
                    }
                }
                else {
                    if (Distance(center, roomCenter) < 96.0f) {
                        tile = (int)TileType::Path;
                    }
                    else if (((x * 11 + y * 7) % 9) == 0) {
                        tile = (int)TileType::GrassAlt;
                    }
                }
            }
            else {
                int hash = (x * 17 + y * 31) & 31;
                if (hash < 3) tile = (int)TileType::Flowers;
                else if (hash < 6) tile = (int)TileType::GrassAlt;
            }

            tileMap.tiles[(size_t)y * (size_t)tileMap.width + (size_t)x] = tile;

            bool insidePlayable = false;
            for (const auto& room : dungeon.rooms) {
                if (CheckCollisionPointRec(center, ExpandRect(room.rect, 36.0f))) {
                    insidePlayable = true;
                    break;
                }
            }
            if (!insidePlayable) {
                for (const auto& corridor : dungeon.corridors) {
                    if (CheckCollisionPointRec(center, ExpandRect(corridor, 30.0f))) {
                        insidePlayable = true;
                        break;
                    }
                }
            }

            if (!insidePlayable) {
                int hash = (x * 92821 + y * 68917) & 255;
                if (hash < 8) decorProps.push_back({ center, 0, 1.04f + (hash % 2) * 0.10f });
                else if (hash < 12) decorProps.push_back({ center, 2, 0.76f + (hash % 2) * 0.08f });
                else if (hash < 16) decorProps.push_back({ center, 1, 0.80f + (hash % 2) * 0.06f });
            }
        }
    }

    worldProps.push_back({ { 0.0f, -520.0f }, 9, 0.90f });
    worldProps.push_back({ { 0.0f, 520.0f }, 9, 0.90f });
    worldProps.push_back({ { 650.0f, 0.0f }, 9, 0.90f });
    worldProps.push_back({ { -650.0f, 0.0f }, 9, 0.90f });
    worldProps.push_back({ { 126.0f, -636.0f }, 9, 0.82f });
    worldProps.push_back({ { -148.0f, 592.0f }, 9, 0.82f });
    worldProps.push_back({ { 1544.0f, -930.0f }, 9, 0.82f });
    worldProps.push_back({ { -1618.0f, 1008.0f }, 9, 0.82f });

    for (size_t i = 0; i < dungeon.obstacles.size(); ++i) {
        const auto& obstacle = dungeon.obstacles[i];
        Vector2 center = { obstacle.rect.x + obstacle.rect.width * 0.5f, obstacle.rect.y + obstacle.rect.height * 0.5f };

        int spriteIndex = obstacle.spriteIndex;
        float scale = obstacle.spriteScale;
        if (spriteIndex < 0) {
            switch (i % 4) {
            case 0: spriteIndex = 0; scale = 1.20f; break;
            case 1: spriteIndex = 2; scale = 0.95f; break;
            case 2: spriteIndex = 3; scale = 0.95f; break;
            default: spriteIndex = 4; scale = 0.92f; break;
            }
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

    std::vector<Rectangle> barriers = GetActiveBarrierRects();
    auto CanOccupy = [&](Vector2 candidate) {
        if (!IsPositionWalkable(candidate, radius, dungeon)) {
            return false;
        }

        for (const auto& barrier : barriers) {
            if (CircleRectCollision(candidate, radius, barrier)) {
                return false;
            }
        }

        return true;
        };

    Vector2 position = start;
    if (!CanOccupy(position)) {
        bool found = false;
        for (float dist = 2.0f; dist <= 96.0f && !found; dist += 2.0f) {
            for (int i = 0; i < 24; ++i) {
                float angle = (6.28318530718f * (float)i) / 24.0f;
                Vector2 probe = {
                    start.x + std::cos(angle) * dist,
                    start.y + std::sin(angle) * dist
                };
                if (CanOccupy(probe)) {
                    position = probe;
                    found = true;
                    break;
                }
            }
        }
    }

    int autoSteps = (int)std::ceil(std::max(std::fabs(delta.x), std::fabs(delta.y)) / 3.0f);
    if (autoSteps > steps) {
        steps = autoSteps;
    }

    Vector2 stepDelta = { delta.x / (float)steps, delta.y / (float)steps };

    auto MoveAxisPrecisely = [&](bool moveX, float amount) {
        if (std::fabs(amount) <= 0.0001f) {
            return;
        }

        Vector2 target = position;
        if (moveX) target.x += amount;
        else target.y += amount;

        if (CanOccupy(target)) {
            position = target;
            return;
        }

        float low = 0.0f;
        float high = 1.0f;
        float best = 0.0f;
        for (int i = 0; i < 12; ++i) {
            float mid = (low + high) * 0.5f;
            Vector2 probe = position;
            if (moveX) probe.x += amount * mid;
            else probe.y += amount * mid;

            if (CanOccupy(probe)) {
                best = mid;
                low = mid;
            }
            else {
                high = mid;
            }
        }

        if (best > 0.0f) {
            if (moveX) position.x += amount * best;
            else position.y += amount * best;
        }
        };

    for (int i = 0; i < steps; ++i) {
        MoveAxisPrecisely(true, stepDelta.x);
        MoveAxisPrecisely(false, stepDelta.y);
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

    Rectangle room = dungeon.rooms[lockedRoomIndex].rect;
    Vector2 roomCenter = SafeZoneCenter(room);

    for (const auto& corridor : dungeon.corridors) {
        float overlapLeft = std::max(room.x, corridor.x);
        float overlapTop = std::max(room.y, corridor.y);
        float overlapRight = std::min(room.x + room.width, corridor.x + corridor.width);
        float overlapBottom = std::min(room.y + room.height, corridor.y + corridor.height);
        float overlapW = overlapRight - overlapLeft;
        float overlapH = overlapBottom - overlapTop;

        if (overlapW <= 26.0f || overlapH <= 26.0f) {
            continue;
        }

        const float thickness = 40.0f;
        const float inset = 64.0f;

        if (corridor.width > corridor.height) {
            bool leftSide = (corridor.x + corridor.width * 0.5f) < roomCenter.x;
            float barrierX = leftSide
                ? room.x + inset - thickness * 0.5f
                : room.x + room.width - inset - thickness * 0.5f;
            barriers.push_back({
                barrierX,
                overlapTop + 18.0f,
                thickness,
                std::max(56.0f, overlapH - 36.0f)
                });
        }
        else {
            bool topSide = (corridor.y + corridor.height * 0.5f) < roomCenter.y;
            float barrierY = topSide
                ? room.y + inset - thickness * 0.5f
                : room.y + room.height - inset - thickness * 0.5f;
            barriers.push_back({
                overlapLeft + 18.0f,
                barrierY,
                std::max(56.0f, overlapW - 36.0f),
                thickness
                });
        }
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
    return 4.6f + 0.16f * (float)CountRelic(RelicType::PhaseBoots);
}

float Game::GetAttackCooldown() const {
    float value = 0.28f - 0.025f * (float)CountRelic(RelicType::OverclockCore);
    if (value < 0.12f) {
        value = 0.12f;
    }
    return value;
}

float Game::GetDashCooldown() const {
    float value = 3.0f - 0.12f * (float)CountRelic(RelicType::PhaseBoots);
    if (value < 1.8f) {
        value = 1.8f;
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
    return 45 + player.wave * 3 + 18 * CountRelic(RelicType::EMPCapacitor);
}

float Game::GetEmpRadius() const {
    return 170.0f + 10.0f * (float)CountRelic(RelicType::EMPCapacitor);
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

bool Game::LoadProfile() {
    persistentOwnedWeapons.assign(weaponDB.size(), false);
    if (!persistentOwnedWeapons.empty()) {
        persistentOwnedWeapons[0] = true;
    }
    persistentHpUpgradeLevel = 0;
    persistentEquippedWeaponIdx = 0;
    legacyRenown = 0;

    std::ifstream in("profile.txt");
    if (!in.is_open()) {
        return false;
    }

    std::string header;
    in >> header;
    if (header != "CROWNHEART_PROFILE_V1") {
        return false;
    }

    in >> legacyRenown >> persistentHpUpgradeLevel >> persistentEquippedWeaponIdx;
    if (legacyRenown < 0) legacyRenown = 0;
    if (persistentHpUpgradeLevel < 0) persistentHpUpgradeLevel = 0;
    if (persistentEquippedWeaponIdx < 0 || persistentEquippedWeaponIdx >= (int)weaponDB.size()) {
        persistentEquippedWeaponIdx = 0;
    }

    size_t ownedCount = 0;
    in >> ownedCount;
    for (size_t i = 0; i < ownedCount; ++i) {
        int owned = 0;
        in >> owned;
        if (i < persistentOwnedWeapons.size()) {
            persistentOwnedWeapons[i] = (owned != 0);
        }
    }

    if (!persistentOwnedWeapons.empty()) {
        persistentOwnedWeapons[0] = true;
        if (!persistentOwnedWeapons[persistentEquippedWeaponIdx]) {
            persistentEquippedWeaponIdx = 0;
        }
    }

    return true;
}

void Game::SaveProfile() const {
    std::ofstream out("profile.txt", std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "CROWNHEART_PROFILE_V1\n";
    out << legacyRenown << ' ' << persistentHpUpgradeLevel << ' ' << persistentEquippedWeaponIdx << '\n';
    out << persistentOwnedWeapons.size();
    for (bool owned : persistentOwnedWeapons) {
        out << ' ' << (owned ? 1 : 0);
    }
    out << '\n';
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
            else if (monster.eliteKind == 2) monster.name = "STORMBOUND " + monster.name;
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
    announcement = "CHRONICLE RESTORED";
    announcementTimer = 2.2f;
    return true;
}

void Game::ResetRun() {
    player = Player{};
    player.pos = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.72f };
    player.aimDir = { 0.0f, -1.0f };
    player.hpUpgradeLevel = persistentHpUpgradeLevel;
    player.maxHp = 160 + persistentHpUpgradeLevel * 30;
    player.hp = player.maxHp;
    player.xp = 240 + legacyRenown;
    legacyRenown = 0;

    shop = ShopState{};
    shop.ownedWeapons = persistentOwnedWeapons;
    if (shop.ownedWeapons.size() != weaponDB.size()) {
        shop.ownedWeapons.assign(weaponDB.size(), false);
    }
    if (!shop.ownedWeapons.empty()) {
        shop.ownedWeapons[0] = true;
    }
    player.equippedWeaponIdx = persistentEquippedWeaponIdx;
    if (player.equippedWeaponIdx < 0 || player.equippedWeaponIdx >= (int)weaponDB.size() || !shop.ownedWeapons[player.equippedWeaponIdx]) {
        player.equippedWeaponIdx = 0;
    }
    persistentEquippedWeaponIdx = player.equippedWeaponIdx;
    SaveProfile();

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
    announcement = "CROWNHEART AWAITS";
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
                monster.name = "STORMBOUND " + monster.name;
            }
        }
    }

    monsters.push_back(monster);
}

void Game::SpawnWave(int waveNumber) {
    announcement = std::string(TextFormat("WAVE %d // HUNT BEGINS", waveNumber));
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
        announcement = monsterTypes[bossType].name + " // BOSS STIRS";
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

                    announcement = dungeon.rooms[i].name + " // GATES SEALED";
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
        player.empCd = 30.0f;
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
                AddFloatingText(monster.pos, "NOVA " + std::to_string(empDamage), neonCyan);
                EmitBurst(monster.pos, 12, 5.0f, neonCyan, 3.0f);
            }
        }
    }

    if (!shop.isOpen && !rewardSelectionOpen && IsKeyPressed(KEY_THREE) && player.turretCd <= 0.0f) {
        player.turretCd = 12.0f;
        turrets.push_back({ player.pos, GetTurretLifetime(), 0.25f });
        EmitBurst(player.pos, 18, 3.0f, neonBlue, 3.8f);
        AddFloatingText(player.pos, "TOTEM RAISED", neonBlue);
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
            announcement = dungeon.rooms[lockedRoomIndex].name + " // PATH SECURED";
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
                persistentHpUpgradeLevel = player.hpUpgradeLevel;
                shop.message = "VIGOR RAISED";
                shop.messageColor = safeGreen;
                shop.messageTimer = 1.5f;
                SaveProfile();
                SaveRun();
            }
            else {
                shop.message = "NOT ENOUGH RENOWN";
                shop.messageColor = softRed;
                shop.messageTimer = 1.2f;
            }
        }

        if (IsKeyPressed(KEY_B)) {
            if (shop.ownedWeapons[shop.browseWeaponIdx]) {
                player.equippedWeaponIdx = shop.browseWeaponIdx;
                persistentEquippedWeaponIdx = player.equippedWeaponIdx;
                shop.message = "ARMAMENT READIED";
                shop.messageColor = neonCyan;
                shop.messageTimer = 1.2f;
                SaveProfile();
                SaveRun();
            }
            else if (player.xp >= weaponDB[shop.browseWeaponIdx].cost) {
                player.xp -= weaponDB[shop.browseWeaponIdx].cost;
                shop.ownedWeapons[shop.browseWeaponIdx] = true;
                player.equippedWeaponIdx = shop.browseWeaponIdx;
                persistentOwnedWeapons = shop.ownedWeapons;
                persistentEquippedWeaponIdx = player.equippedWeaponIdx;
                shop.message = "BARGAIN SEALED";
                shop.messageColor = neonGold;
                shop.messageTimer = 1.5f;
                SaveProfile();
                SaveRun();
            }
            else {
                shop.message = "NOT ENOUGH RENOWN";
                shop.messageColor = softRed;
                shop.messageTimer = 1.2f;
            }
        }
    }

    if (player.hp <= 0) {
        persistentOwnedWeapons = shop.ownedWeapons;
        persistentHpUpgradeLevel = player.hpUpgradeLevel;
        persistentEquippedWeaponIdx = player.equippedWeaponIdx;
        legacyRenown = std::max(legacyRenown, (int)std::round((float)player.xp * 0.65f));
        SaveProfile();
        DeleteSave();
        gameState = GameState::GameOver;
        announcement = "FALLEN IN BATTLE";
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

    int titleWidth = MeasureText("CROWNHEART", 74);
    DrawText("CROWNHEART", screenW / 2 - titleWidth / 2, 82, 74, { 67, 86, 54, 255 });
    DrawText("KINGDOM SIEGE", screenW / 2 - MeasureText("KINGDOM SIEGE", 30) / 2, 158, 30, softRed);
    DrawText("A hand-drawn fantasy action RPG in Raylib", screenW / 2 - MeasureText("A hand-drawn fantasy action RPG in Raylib", 22) / 2, 196, 22, { 79, 74, 63, 255 });

    DrawPanel({ screenW / 2.0f - 360.0f, 360.0f, 720.0f, 190.0f }, panel, { 118, 105, 80, 255 });
    DrawText("WHAT'S IN THE BUILD", screenW / 2 - MeasureText("WHAT'S IN THE BUILD", 24) / 2, 384, 24, { 92, 78, 58, 255 });
    DrawText("- A kingdom crossroads with keeps, side paths and hidden pockets", screenW / 2 - 300, 425, 20, { 73, 67, 54, 255 });
    DrawText("- Real-time combat with dash, nova burst and guardian totems", screenW / 2 - 300, 453, 20, { 73, 67, 54, 255 });
    DrawText("- Sealed-court battles, relic blessings and rising hunt waves", screenW / 2 - 300, 481, 20, { 73, 67, 54, 255 });
    DrawText("- Save and continue support across your campaign", screenW / 2 - 300, 509, 20, { 73, 67, 54, 255 });

    Color pulse = ((int)(GetTime() * 2.5) % 2 == 0) ? softRed : WHITE;
    DrawText("PRESS ENTER TO BEGIN A NEW QUEST", screenW / 2 - MeasureText("PRESS ENTER TO BEGIN A NEW QUEST", 28) / 2, 586, 28, pulse);

    if (HasSaveFile()) {
        DrawText("PRESS C TO CONTINUE YOUR CHRONICLE", screenW / 2 - MeasureText("PRESS C TO CONTINUE YOUR CHRONICLE", 22) / 2, 622, 22, neonGold);
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

    auto drawFenceStrip = [&](bool horizontal, float fixed, float start, float end) {
        if (end - start < 24.0f) {
            return;
        }

        Color wallShade = { 119, 95, 66, 255 };
        if (horizontal) {
            DrawLineEx({ start, fixed + 6.0f }, { end, fixed + 6.0f }, 7.0f, Fade(wallShade, 0.24f));
        }
        else {
            DrawLineEx({ fixed + 6.0f, start }, { fixed + 6.0f, end }, 7.0f, Fade(wallShade, 0.24f));
        }

        if (propAtlas.id == 0) {
            if (horizontal) DrawLineEx({ start, fixed }, { end, fixed }, 4.0f, wallShade);
            else DrawLineEx({ fixed, start }, { fixed, end }, 4.0f, wallShade);
            return;
        }

        Rectangle src = PropSourceRect(horizontal ? 7 : 8);
        for (float p = start; p < end; p += 88.0f) {
            float length = std::min(88.0f, end - p);
            Rectangle dst = horizontal
                ? Rectangle{ p, fixed - 48.0f, length, 96.0f }
            : Rectangle{ fixed - 48.0f, p, 96.0f, length };
            DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
        };

    auto drawFenceWithGaps = [&](bool horizontal, float fixed, float spanStart, float spanEnd, std::vector<Vector2> gaps) {
        std::sort(gaps.begin(), gaps.end(), [](const Vector2& a, const Vector2& b) { return a.x < b.x; });
        float cursor = spanStart;

        for (const auto& gap : gaps) {
            float gapStart = ClampFloat(gap.x - 22.0f, spanStart, spanEnd);
            float gapEnd = ClampFloat(gap.y + 22.0f, spanStart, spanEnd);
            if (gapStart > cursor) {
                drawFenceStrip(horizontal, fixed, cursor, gapStart);
            }
            if (gapEnd > cursor) {
                cursor = gapEnd;
            }
        }

        if (cursor < spanEnd) {
            drawFenceStrip(horizontal, fixed, cursor, spanEnd);
        }
        };

    auto drawRoomEdgeFences = [&](const DungeonArea& room) {
        std::vector<Vector2> leftGaps;
        std::vector<Vector2> rightGaps;
        std::vector<Vector2> topGaps;
        std::vector<Vector2> bottomGaps;
        Vector2 roomCenter = SafeZoneCenter(room.rect);

        for (const auto& corridor : dungeon.corridors) {
            float overlapLeft = std::max(room.rect.x, corridor.x);
            float overlapTop = std::max(room.rect.y, corridor.y);
            float overlapRight = std::min(room.rect.x + room.rect.width, corridor.x + corridor.width);
            float overlapBottom = std::min(room.rect.y + room.rect.height, corridor.y + corridor.height);
            float overlapW = overlapRight - overlapLeft;
            float overlapH = overlapBottom - overlapTop;

            if (overlapW <= 18.0f || overlapH <= 18.0f) {
                continue;
            }

            if (corridor.width > corridor.height) {
                if (corridor.x + corridor.width * 0.5f < roomCenter.x) leftGaps.push_back({ overlapTop, overlapBottom });
                else rightGaps.push_back({ overlapTop, overlapBottom });
            }
            else {
                if (corridor.y + corridor.height * 0.5f < roomCenter.y) topGaps.push_back({ overlapLeft, overlapRight });
                else bottomGaps.push_back({ overlapLeft, overlapRight });
            }
        }

        drawFenceWithGaps(true, room.rect.y, room.rect.x + 20.0f, room.rect.x + room.rect.width - 20.0f, topGaps);
        drawFenceWithGaps(true, room.rect.y + room.rect.height, room.rect.x + 20.0f, room.rect.x + room.rect.width - 20.0f, bottomGaps);
        drawFenceWithGaps(false, room.rect.x, room.rect.y + 20.0f, room.rect.y + room.rect.height - 20.0f, leftGaps);
        drawFenceWithGaps(false, room.rect.x + room.rect.width, room.rect.y + 20.0f, room.rect.y + room.rect.height - 20.0f, rightGaps);
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

    for (const auto& corridor : dungeon.corridors) {
        DrawRectangleLinesEx(corridor, 2.0f, Fade({ 160, 126, 78, 255 }, 0.18f));
    }

    for (const auto& prop : decorProps) {
        drawProp(prop);
    }

    for (const auto& room : dungeon.rooms) {
        DrawRectangleLinesEx(room.rect, 2.0f, Fade({ 93, 125, 58, 255 }, 0.18f));
        drawRoomEdgeFences(room);
        int labelSize = room.isSafeZone ? 24 : 20;
        int labelX = (int)(room.rect.x + room.rect.width * 0.5f) - MeasureText(room.name.c_str(), labelSize) / 2;
        DrawText(room.name.c_str(), labelX, (int)room.rect.y + 14, labelSize, { 92, 78, 56, 255 });
    }

    std::vector<PropInstance> sortedProps = worldProps;
    std::sort(sortedProps.begin(), sortedProps.end(), [](const PropInstance& a, const PropInstance& b) {
        return a.pos.y < b.pos.y;
        });
    for (const auto& prop : sortedProps) {
        drawProp(prop);
    }

    for (const auto& barrier : GetActiveBarrierRects()) {
        DrawRectangleRounded(barrier, 0.12f, 4, Fade({ 97, 72, 48, 255 }, 0.18f));
        if (barrier.width > barrier.height) drawFenceStrip(true, barrier.y + barrier.height * 0.5f, barrier.x, barrier.x + barrier.width);
        else drawFenceStrip(false, barrier.x + barrier.width * 0.5f, barrier.y, barrier.y + barrier.height);
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
    DrawText("ADVENTURER'S KIT", 34, 28, 20, neonCyan);
    DrawText(TextFormat("WEAPON: %s", weaponDB[player.equippedWeaponIdx].name.c_str()), 34, 54, 18, WHITE);
    DrawText(TextFormat("WAVE %d", player.wave), 300, 28, 20, neonGold);
    DrawText(TextFormat("KILLS %d", player.kills), 300, 54, 18, RAYWHITE);
    DrawText(TextFormat("RELICS %d", player.relicsCollected), 300, 78, 18, neonPink);

    DrawRectangle(34, 88, 250, 18, Fade(WHITE, 0.12f));
    DrawRectangle(34, 88, (int)(250.0f * ((float)player.hp / (float)player.maxHp)), 18, player.hp < player.maxHp * 0.3f ? softRed : safeGreen);
    DrawText(TextFormat("HP %d / %d", player.hp, player.maxHp), 44, 88, 16, WHITE);

    DrawRectangle(34, 118, 250, 12, Fade(WHITE, 0.12f));
    DrawRectangle(34, 118, (int)ClampFloat((float)player.xp / 3000.0f * 250.0f, 0.0f, 250.0f), 12, neonGold);
    DrawText(TextFormat("RENOWN %d", player.xp), 294, 112, 16, neonGold);

    DrawPanel({ 18.0f, 188.0f, 300.0f, 118.0f }, panel, neonPink);
    DrawText("ABILITIES", 32, 200, 18, neonPink);
    DrawText(TextFormat("1 DASH    %.1fs", player.dashCd), 32, 226, 18, player.dashCd <= 0.0f ? neonCyan : GRAY);
    DrawText(TextFormat("2 NOVA    %.1fs", player.empCd), 32, 250, 18, player.empCd <= 0.0f ? neonCyan : GRAY);
    DrawText(TextFormat("3 TOTEM   %.1fs", player.turretCd), 32, 274, 18, player.turretCd <= 0.0f ? neonCyan : GRAY);

    DrawPanel({ screenW - 352.0f, 18.0f, 334.0f, 150.0f }, panel, safeGreen);
    DrawText("REGION READOUT", screenW - 334, 28, 20, safeGreen);
    DrawText(TextFormat("FOES %d", (int)monsters.size()), screenW - 334, 58, 18, WHITE);
    DrawText(TextFormat("TOTEMS %d", (int)turrets.size()), screenW - 334, 82, 18, WHITE);

    bool inSafeZone = CheckCollisionPointRec(player.pos, safeZone);
    const DungeonArea* currentArea = GetCurrentArea(player.pos);
    DrawText(inSafeZone ? "ZONE: SAFE" : "ZONE: HOT", screenW - 334, 106, 18, inSafeZone ? safeGreen : softRed);
    DrawText(TextFormat("AREA: %s", currentArea ? currentArea->name.c_str() : "STONE ROAD"), screenW - 334, 130, 16, RAYWHITE);

    DrawMiniMap();

    if (rewardChestActive && !rewardSelectionOpen && Distance(player.pos, rewardChestPos) < 72.0f) {
        DrawText("RELIC CHEST READY // PRESS E TO CLAIM A BLESSING", 22, screenH - 34, 18, neonGold);
    }
    else if (inSafeZone) {
        DrawText("SAFE COURTYARD // PRESS E FOR THE KEEP ARMORY // RIDE OUT THROUGH ANY OPEN ROAD", 22, screenH - 34, 18, safeGreen);
    }
    else {
        DrawText("WASD MOVE   SPACE SWING   1 DASH   2 NOVA   3 TOTEM   HOLD THE ROADS, CLEAR THE COURTS", 22, screenH - 34, 18, RAYWHITE);
    }

    if (announcementTimer > 0.0f) {
        int fontSize = 28;
        int width = MeasureText(announcement.c_str(), fontSize);
        DrawText(announcement.c_str(), screenW / 2 - width / 2, 24, fontSize, neonGold);
    }
}

void Game::DrawMiniMap() const {
    Rectangle panelRect = { screenW - 292.0f, 176.0f, 274.0f, 274.0f };
    DrawPanel(panelRect, panel, neonBlue);
    DrawText("KINGDOM MAP", (int)panelRect.x + 20, (int)panelRect.y + 16, 18, neonBlue);

    Rectangle mapRect = { panelRect.x + 16.0f, panelRect.y + 42.0f, panelRect.width - 32.0f, panelRect.height - 60.0f };
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
        if (obstacle.colliders.empty()) {
            Rectangle mini = {
                mapRect.x + (obstacle.rect.x - minX) * scale,
                mapRect.y + (obstacle.rect.y - minY) * scale,
                obstacle.rect.width * scale,
                obstacle.rect.height * scale
            };
            DrawRectangleRec(mini, Fade(obstacle.color, 0.18f));
        }
        else {
            for (const auto& collider : obstacle.colliders) {
                Rectangle mini = {
                    mapRect.x + (collider.x - minX) * scale,
                    mapRect.y + (collider.y - minY) * scale,
                    collider.width * scale,
                    collider.height * scale
                };
                DrawRectangleRec(mini, Fade(obstacle.color, 0.18f));
            }
        }
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

    if (rewardChestActive) {
        Vector2 chestMini = WorldToMap(rewardChestPos);
        DrawCircleV(chestMini, 4.2f, neonGold);
        DrawCircleLines((int)chestMini.x, (int)chestMini.y, 7.0f, WHITE);
    }

    Vector2 playerMini = WorldToMap(player.pos);
    DrawCircleV(playerMini, 4.6f, WHITE);
    DrawCircleV(playerMini, 3.2f, neonCyan);
}

void Game::DrawRewardOverlay() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.62f));

    Rectangle panelRect = { screenW * 0.5f - 470.0f, screenH * 0.5f - 210.0f, 940.0f, 420.0f };
    DrawPanel(panelRect, panel2, neonGold);
    DrawText("RELIC CHEST OPENED", (int)panelRect.x + 28, (int)panelRect.y + 22, 30, neonGold);
    DrawText("CHOOSE ONE BLESSING", (int)panelRect.x + 30, (int)panelRect.y + 58, 18, RAYWHITE);

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

    DrawText("KEEP ARMORY", (int)panelRect.x + 24, (int)panelRect.y + 22, 28, neonGold);
    DrawText(TextFormat("RENOWN HELD: %d", player.xp), (int)panelRect.x + 518, (int)panelRect.y + 28, 20, safeGreen);

    int hpUpgradeCost = 100 + player.hpUpgradeLevel * 90;
    DrawPanel({ panelRect.x + 24.0f, panelRect.y + 72.0f, 732.0f, 94.0f }, panel, safeGreen);
    DrawText("HEARTH BLESSINGS", (int)panelRect.x + 40, (int)panelRect.y + 88, 22, safeGreen);
    DrawText(TextFormat("VIGOR +30   COST: %d RENOWN   RANK: %d", hpUpgradeCost, player.hpUpgradeLevel), (int)panelRect.x + 40, (int)panelRect.y + 122, 20, WHITE);
    DrawText("PRESS H TO INVEST", (int)panelRect.x + 536, (int)panelRect.y + 122, 20, neonCyan);

    const Weapon& browseWeapon = weaponDB[shop.browseWeaponIdx];
    DrawPanel({ panelRect.x + 24.0f, panelRect.y + 186.0f, 732.0f, 188.0f }, panel, browseWeapon.color);
    DrawText("ARMORY RACK", (int)panelRect.x + 40, (int)panelRect.y + 204, 22, browseWeapon.color);
    DrawText(browseWeapon.name.c_str(), (int)panelRect.x + 40, (int)panelRect.y + 238, 30, WHITE);
    DrawText(TextFormat("RARITY: %s", browseWeapon.rarity.c_str()), (int)panelRect.x + 40, (int)panelRect.y + 278, 20, browseWeapon.color);
    DrawText(TextFormat("DAMAGE: %d", browseWeapon.damage), (int)panelRect.x + 260, (int)panelRect.y + 278, 20, WHITE);
    DrawText(TextFormat("RANGE: %.0f", browseWeapon.range), (int)panelRect.x + 430, (int)panelRect.y + 278, 20, WHITE);
    DrawText(TextFormat("COST: %d XP", browseWeapon.cost), (int)panelRect.x + 600, (int)panelRect.y + 278, 20, neonGold);

    std::string stateLabel = shop.ownedWeapons[shop.browseWeaponIdx]
        ? (player.equippedWeaponIdx == shop.browseWeaponIdx ? "READIED" : "CLAIMED")
        : "LOCKED";
    Color stateColor = shop.ownedWeapons[shop.browseWeaponIdx]
        ? (player.equippedWeaponIdx == shop.browseWeaponIdx ? neonCyan : safeGreen)
        : softRed;

    DrawText(TextFormat("STATUS: %s", stateLabel.c_str()), (int)panelRect.x + 40, (int)panelRect.y + 318, 22, stateColor);
    DrawText("LEFT / RIGHT TO BROWSE   B TO CLAIM OR READY", (int)panelRect.x + 246, (int)panelRect.y + 318, 20, RAYWHITE);

    DrawPanel({ panelRect.x + 24.0f, panelRect.y + 392.0f, 732.0f, 56.0f }, panel, neonBlue);
    DrawText("E LEAVE", (int)panelRect.x + 40, (int)panelRect.y + 408, 20, neonBlue);
    DrawText("CLAIM ONCE, BEAR IT FOREVER", (int)panelRect.x + 220, (int)panelRect.y + 408, 20, RAYWHITE);

    if (shop.messageTimer > 0.0f) {
        DrawText(shop.message.c_str(), (int)panelRect.x + 560, (int)panelRect.y + 408, 20, shop.messageColor);
    }
}

void Game::DrawGameOver() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.72f));
    DrawText("FALLEN IN BATTLE", screenW / 2 - MeasureText("FALLEN IN BATTLE", 56) / 2, 180, 56, softRed);
    DrawText(TextFormat("WAVE REACHED: %d", player.wave), screenW / 2 - MeasureText(TextFormat("WAVE REACHED: %d", player.wave), 28) / 2, 280, 28, WHITE);
    DrawText(TextFormat("TOTAL KILLS: %d", player.kills), screenW / 2 - MeasureText(TextFormat("TOTAL KILLS: %d", player.kills), 28) / 2, 320, 28, WHITE);
    DrawText(TextFormat("RENOWN CARRIED FORWARD: %d", legacyRenown), screenW / 2 - MeasureText(TextFormat("RENOWN CARRIED FORWARD: %d", legacyRenown), 28) / 2, 360, 28, neonGold);
    DrawText("WEAPONS AND VIGOR UPGRADES ARE KEPT", screenW / 2 - MeasureText("WEAPONS AND VIGOR UPGRADES ARE KEPT", 22) / 2, 404, 22, RAYWHITE);
    DrawText("PRESS ENTER TO RIDE OUT AGAIN", screenW / 2 - MeasureText("PRESS ENTER TO RIDE OUT AGAIN", 26) / 2, 456, 26, neonCyan);
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
