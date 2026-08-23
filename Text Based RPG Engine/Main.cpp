#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

const Color NEON_CYAN = { 0, 255, 255, 255 };

struct Weapon {
    std::string name;
    int damage;
    float range;
    std::string rarity;
    int cost;
};

struct MonsterType {
    std::string name;
    int maxHp;
    int damage;
    float speed;
    float radius;
    Color color;
    bool isBoss;
};

struct ActiveMonster {
    Vector2 pos;
    int hp;
    int maxHp;
    int damage;
    float speed;
    float radius;
    Color color;
    bool isBoss;
    std::string name;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float alpha;
    float size;
};

struct Turret {
    Vector2 pos;
    float timer;
};

int main() {
    const int screenW = 1280;
    const int screenH = 720;

    std::srand((unsigned int)std::time(nullptr));

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenW, screenH, "CYBER ENGINE 2D // EXPANDED CORE");
    SetTargetFPS(60);

    std::vector<Weapon> weaponDB = {
        {"Rusty Pipe", 10, 60.0f, "Common", 0},
        {"Iron Baton", 15, 65.0f, "Common", 50},
        {"Shock Stunner", 22, 70.0f, "Common", 100},
        {"Chain Saw blade", 30, 75.0f, "Common", 175},
        {"Plasma Cutter", 40, 80.0f, "Rare", 250},
        {"Laser Edge", 52, 85.0f, "Rare", 350},
        {"Thermal Katana", 65, 90.0f, "Rare", 500},
        {"Sonic Vibroblade", 80, 95.0f, "Rare", 700},
        {"Pulse Rifle Saber", 98, 100.0f, "Rare", 950},
        {"Void Reaper", 120, 110.0f, "Legendary", 1300},
        {"Arc Disruptor", 145, 115.0f, "Legendary", 1700},
        {"Singularity Core", 175, 120.0f, "Legendary", 2200},
        {"Hyperion Greatsword", 210, 130.0f, "Legendary", 2800},
        {"Nanite Obliterator", 250, 140.0f, "Exotic", 3500},
        {"Quantum Slicer", 300, 150.0f, "Exotic", 4500},
        {"Celestial Cleaver", 360, 160.0f, "Exotic", 6000},
        {"Omega Decimator", 450, 180.0f, "Exotic", 8000}
    };

    std::vector<MonsterType> monsterTypes = {
        {"Cyber Rat", 30, 5, 2.5f, 12.0f, GRAY, false},
        {"Drone Scout", 45, 8, 3.2f, 14.0f, YELLOW, false},
        {"Syndicate Thug", 70, 14, 2.0f, 18.0f, ORANGE, false},
        {"Enforcer Bot", 110, 20, 1.6f, 22.0f, RED, false},
        {"Corrupted Cyborg", 160, 28, 1.8f, 20.0f, PURPLE, false},
        {"Viper Assassin", 90, 35, 3.5f, 15.0f, LIME, false},
        {"Heavy Automaton", 260, 42, 1.2f, 28.0f, DARKBLUE, false},
        {"JUGGERNAUT PRIME", 750, 60, 1.4f, 40.0f, MAROON, true},
        {"THE ARCHITECT", 1400, 85, 1.1f, 45.0f, VIOLET, true}
    };

    const int weaponCount = (int)weaponDB.size();

    Vector2 playerPos = { 0.0f, 0.0f };
    float playerSpeed = 4.5f;
    int playerHp = 150;
    int maxHp = 150;
    int xp = 200;
    int equippedWeaponIdx = 0;

    float dashCd = 0.0f;
    float empCd = 0.0f;
    float turretCd = 0.0f;

    std::vector<ActiveMonster> monsters;
    std::vector<Particle> particles;
    std::vector<Turret> turrets;

    Rectangle safeZone = { -250.0f, -250.0f, 500.0f, 500.0f };
    bool inShopUI = false;

    Camera2D camera = { 0 };
    camera.offset = { screenW / 2.0f, screenH / 2.0f };
    camera.target = playerPos;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    auto spawnMonsters = [&](int count) {
        for (int i = 0; i < count; i++) {
            int typeIdx = std::rand() % 7;
            const MonsterType& t = monsterTypes[typeIdx];
            float angle = (float)(std::rand() % 360) * DEG2RAD;
            float dist = 400.0f + (float)(std::rand() % 800);

            monsters.push_back({
                { std::cos(angle) * dist, std::sin(angle) * dist },
                t.maxHp,
                t.maxHp,
                t.damage,
                t.speed,
                t.radius,
                t.color,
                t.isBoss,
                t.name
                });
        }

        monsters.push_back({
            { 1200.0f, 1200.0f },
            monsterTypes[7].maxHp,
            monsterTypes[7].maxHp,
            monsterTypes[7].damage,
            monsterTypes[7].speed,
            monsterTypes[7].radius,
            monsterTypes[7].color,
            true,
            monsterTypes[7].name
            });

        monsters.push_back({
            { -1500.0f, -1500.0f },
            monsterTypes[8].maxHp,
            monsterTypes[8].maxHp,
            monsterTypes[8].damage,
            monsterTypes[8].speed,
            monsterTypes[8].radius,
            monsterTypes[8].color,
            true,
            monsterTypes[8].name
            });
        };

    spawnMonsters(18);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        dashCd = std::max(0.0f, dashCd - dt);
        empCd = std::max(0.0f, empCd - dt);
        turretCd = std::max(0.0f, turretCd - dt);

        bool inSafeZone = CheckCollisionPointRec(playerPos, safeZone);

        if (inSafeZone && IsKeyPressed(KEY_E)) {
            inShopUI = !inShopUI;
        }
        if (!inSafeZone) {
            inShopUI = false;
        }

        if (!inShopUI) {
            Vector2 move = { 0.0f, 0.0f };
            if (IsKeyDown(KEY_W)) move.y -= 1.0f;
            if (IsKeyDown(KEY_S)) move.y += 1.0f;
            if (IsKeyDown(KEY_A)) move.x -= 1.0f;
            if (IsKeyDown(KEY_D)) move.x += 1.0f;

            if (move.x != 0.0f || move.y != 0.0f) {
                float len = std::sqrt(move.x * move.x + move.y * move.y);
                playerPos.x += (move.x / len) * playerSpeed;
                playerPos.y += (move.y / len) * playerSpeed;
            }

            if (IsKeyPressed(KEY_ONE) && dashCd <= 0.0f) {
                if (move.x != 0.0f || move.y != 0.0f) {
                    float len = std::sqrt(move.x * move.x + move.y * move.y);
                    playerPos.x += (move.x / len) * 160.0f;
                    playerPos.y += (move.y / len) * 160.0f;
                    dashCd = 3.0f;
                }
            }

            if (IsKeyPressed(KEY_TWO) && empCd <= 0.0f) {
                empCd = 6.0f;
                for (auto& m : monsters) {
                    float dx = playerPos.x - m.pos.x;
                    float dy = playerPos.y - m.pos.y;
                    float d = std::sqrt(dx * dx + dy * dy);
                    if (d < 220.0f) {
                        m.hp -= 80;
                    }
                }

                for (int i = 0; i < 36; i++) {
                    float angle = (float)i * 10.0f * DEG2RAD;
                    particles.push_back({
                        playerPos,
                        { std::cos(angle) * 8.0f, std::sin(angle) * 8.0f },
                        NEON_CYAN,
                        1.0f,
                        6.0f
                        });
                }
            }

            if (IsKeyPressed(KEY_THREE) && turretCd <= 0.0f) {
                turrets.push_back({ playerPos, 10.0f });
                turretCd = 12.0f;
            }

            if (IsKeyPressed(KEY_SPACE)) {
                Weapon w = weaponDB[equippedWeaponIdx];
                for (auto& m : monsters) {
                    float dx = playerPos.x - m.pos.x;
                    float dy = playerPos.y - m.pos.y;
                    float d = std::sqrt(dx * dx + dy * dy);
                    if (d <= w.range) {
                        m.hp -= w.damage;
                    }
                }

                for (int i = 0; i < 15; i++) {
                    float angle = (float)(std::rand() % 360) * DEG2RAD;
                    particles.push_back({
                        playerPos,
                        { std::cos(angle) * 4.0f, std::sin(angle) * 4.0f },
                        MAGENTA,
                        1.0f,
                        4.0f
                        });
                }
            }

            for (auto& t : turrets) {
                t.timer -= dt;
                for (auto& m : monsters) {
                    float dx = t.pos.x - m.pos.x;
                    float dy = t.pos.y - m.pos.y;
                    float d = std::sqrt(dx * dx + dy * dy);
                    if (d < 200.0f) {
                        m.hp -= 1;
                    }
                }
            }

            turrets.erase(
                std::remove_if(turrets.begin(), turrets.end(), [](const Turret& t) { return t.timer <= 0.0f; }),
                turrets.end()
            );

            for (auto& m : monsters) {
                if (!inSafeZone) {
                    float dx = playerPos.x - m.pos.x;
                    float dy = playerPos.y - m.pos.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist > 5.0f) {
                        m.pos.x += (dx / dist) * m.speed;
                        m.pos.y += (dy / dist) * m.speed;
                    }

                    if (dist < m.radius + 15.0f) {
                        playerHp -= m.damage / 20;
                        if (playerHp < 0) {
                            playerHp = 0;
                        }
                    }
                }
            }

            for (auto it = monsters.begin(); it != monsters.end();) {
                if (it->hp <= 0) {
                    xp += it->isBoss ? 500 : 45;
                    it = monsters.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        for (auto& p : particles) {
            p.pos.x += p.vel.x;
            p.pos.y += p.vel.y;
            p.alpha -= 0.04f;
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.alpha <= 0.0f; }),
            particles.end()
        );

        camera.target = playerPos;

        BeginDrawing();
        ClearBackground(GetColor(0x0A0A12FF));
        BeginMode2D(camera);

        for (int x = -2500; x <= 2500; x += 100) {
            DrawLine(x, -2500, x, 2500, GetColor(0x161626FF));
        }
        for (int y = -2500; y <= 2500; y += 100) {
            DrawLine(-2500, y, 2500, y, GetColor(0x161626FF));
        }

        DrawRectangleRec(safeZone, GetColor(0x00FF881A));
        DrawRectangleLinesEx(safeZone, 3.0f, LIME);
        DrawText("SAFE ZONE (NEXUS)", -110, -10, 20, LIME);

        for (const auto& t : turrets) {
            DrawCircleV(t.pos, 12.0f, SKYBLUE);
            DrawCircleLines((int)t.pos.x, (int)t.pos.y, 200.0f, Fade(SKYBLUE, 0.15f));
        }

        for (const auto& m : monsters) {
            DrawCircleV(m.pos, m.radius, m.color);
            DrawCircleLines((int)m.pos.x, (int)m.pos.y, m.radius, WHITE);

            DrawRectangleRec(
                Rectangle{ m.pos.x - 20.0f, m.pos.y - m.radius - 12.0f, 40.0f, 5.0f },
                RED
            );

            DrawRectangleRec(
                Rectangle{
                    m.pos.x - 20.0f,
                    m.pos.y - m.radius - 12.0f,
                    40.0f * ((float)m.hp / (float)m.maxHp),
                    5.0f
                },
                GREEN
            );
        }

        for (const auto& p : particles) {
            DrawCircleV(p.pos, p.size, Fade(p.color, p.alpha));
        }

        DrawCircleV(playerPos, 18.0f, SKYBLUE);
        DrawCircleLines((int)playerPos.x, (int)playerPos.y, 18.0f, WHITE);
        DrawCircleLines((int)playerPos.x, (int)playerPos.y, weaponDB[equippedWeaponIdx].range, Fade(MAGENTA, 0.2f));

        EndMode2D();

        DrawRectangle(15, 15, 360, 145, GetColor(0x121220EE));
        DrawRectangleLines(15, 15, 360, 145, SKYBLUE);

        DrawText(TextFormat("HP: %d / %d", playerHp, maxHp), 30, 25, 16, GREEN);
        DrawText(TextFormat("XP: %d DATA", xp), 200, 25, 16, GOLD);
        DrawText(TextFormat("WEAPON: %s (%d DMG)", weaponDB[equippedWeaponIdx].name.c_str(), weaponDB[equippedWeaponIdx].damage), 30, 50, 15, WHITE);

        DrawText(TextFormat("1: Dash [%.1fs]", dashCd), 30, 75, 14, dashCd <= 0.0f ? NEON_CYAN : GRAY);
        DrawText(TextFormat("2: EMP [%.1fs]", empCd), 150, 75, 14, empCd <= 0.0f ? NEON_CYAN : GRAY);
        DrawText(TextFormat("3: Sentry [%.1fs]", turretCd), 260, 75, 14, turretCd <= 0.0f ? NEON_CYAN : GRAY);

        if (inSafeZone) {
            DrawText(">>> SAFE ZONE ACTIVE <<<", 30, 105, 15, LIME);
            DrawText("Press 'E' to open Arsenal Merchant", 30, 125, 14, YELLOW);
        }
        else {
            DrawText("WASD: Move | SPACE: Attack", 30, 115, 14, RAYWHITE);
        }

        if (inShopUI) {
            DrawRectangle(screenW / 2 - 350, screenH / 2 - 250, 700, 500, GetColor(0x0A0A18F5));
            DrawRectangleLines(screenW / 2 - 350, screenH / 2 - 250, 700, 500, GOLD);

            DrawText("NEXUS ARSENAL MERCHANT", screenW / 2 - 160, screenH / 2 - 230, 22, GOLD);
            DrawText(TextFormat("AVAILABLE XP: %d", xp), screenW / 2 - 80, screenH / 2 - 200, 16, GREEN);

            DrawText("UPGRADE MAX HP (+25) [Cost: 100 XP]", screenW / 2 - 300, screenH / 2 - 150, 16, WHITE);
            DrawText("Press 'H' to upgrade Health", screenW / 2 + 100, screenH / 2 - 150, 14, GRAY);

            if (IsKeyPressed(KEY_H) && xp >= 100) {
                xp -= 100;
                maxHp += 25;
                playerHp += 25;
                if (playerHp > maxHp) {
                    playerHp = maxHp;
                }
            }

            DrawText(TextFormat("< WEAPON SELECTION > : %s", weaponDB[equippedWeaponIdx].name.c_str()), screenW / 2 - 300, screenH / 2 - 90, 18, SKYBLUE);
            DrawText(TextFormat("Rarity: %s | Damage: %d | Cost: %d XP", weaponDB[equippedWeaponIdx].rarity.c_str(), weaponDB[equippedWeaponIdx].damage, weaponDB[equippedWeaponIdx].cost), screenW / 2 - 300, screenH / 2 - 65, 14, RAYWHITE);

            DrawText("Press 'LEFT' / 'RIGHT' Arrow Keys to browse equipment", screenW / 2 - 300, screenH / 2 - 30, 14, YELLOW);
            DrawText("Press 'B' to Buy/Equip Selected Weapon", screenW / 2 - 300, screenH / 2, 16, LIME);

            if (IsKeyPressed(KEY_RIGHT)) {
                equippedWeaponIdx = (equippedWeaponIdx + 1) % weaponCount;
            }
            if (IsKeyPressed(KEY_LEFT)) {
                equippedWeaponIdx = (equippedWeaponIdx - 1 + weaponCount) % weaponCount;
            }

            if (IsKeyPressed(KEY_B)) {
                Weapon w = weaponDB[equippedWeaponIdx];
                if (xp >= w.cost) {
                    xp -= w.cost;
                }
            }

            DrawText("Press 'E' to Exit Merchant UI", screenW / 2 - 110, screenH / 2 + 200, 16, RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
