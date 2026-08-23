#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>

static float ClampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static float LerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

static float RandomRange(float minValue, float maxValue) {
    return minValue + ((float)std::rand() / (float)RAND_MAX) * (maxValue - minValue);
}

static Vector2 VecAdd(Vector2 a, Vector2 b) {
    return { a.x + b.x, a.y + b.y };
}

static Vector2 VecSub(Vector2 a, Vector2 b) {
    return { a.x - b.x, a.y - b.y };
}

static Vector2 VecScale(Vector2 v, float s) {
    return { v.x * s, v.y * s };
}

static float VecLength(Vector2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static Vector2 VecNormalizeSafe(Vector2 v) {
    float len = VecLength(v);
    if (len <= 0.0001f) return { 0.0f, 0.0f };
    return { v.x / len, v.y / len };
}

static float Distance(Vector2 a, Vector2 b) {
    return VecLength(VecSub(a, b));
}

static Vector2 SafeZoneCenter(Rectangle rec) {
    return { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
}

static void DrawGlowCircle(Vector2 pos, float radius, Color color) {
    for (int i = 4; i >= 1; --i) {
        DrawCircleV(pos, radius + i * 6.0f, Fade(color, 0.045f * (float)i));
    }
    DrawCircleV(pos, radius, color);
}

static void DrawPanel(Rectangle rect, Color fill, Color border) {
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 2.0f, border);
}

struct Weapon {
    std::string name;
    int damage;
    float range;
    std::string rarity;
    int cost;
    Color color;
};

struct MonsterType {
    std::string name;
    int maxHp;
    int damage;
    float speed;
    float radius;
    Color color;
    bool isBoss;
    int xpDrop;
};

struct ActiveMonster {
    Vector2 pos;
    Vector2 vel;
    int hp;
    int maxHp;
    int damage;
    int xpDrop;
    float speed;
    float radius;
    float hitFlash;
    float attackTimer;
    Color color;
    bool isBoss;
    int typeIndex;
    std::string name;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    float maxLife;
    float size;
};

struct Orb {
    Vector2 pos;
    Vector2 vel;
    int value;
    Color color;
};

struct FloatingText {
    Vector2 pos;
    Vector2 vel;
    std::string text;
    Color color;
    float life;
};

struct Turret {
    Vector2 pos;
    float life;
    float fireTimer;
};

struct Shockwave {
    Vector2 pos;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
    Color color;
};

struct Beam {
    Vector2 start;
    Vector2 end;
    Color color;
    float thickness;
    float life;
};

struct Star {
    Vector2 pos;
    float speed;
    float size;
};

enum class GameState {
    Title,
    Playing,
    GameOver
};

static void DrawEnemySprite(const ActiveMonster& m) {
    Color body = (m.hitFlash > 0.0f) ? WHITE : m.color;
    Color core = Fade(WHITE, 0.9f);

    DrawGlowCircle(m.pos, m.radius + (m.isBoss ? 5.0f : 0.0f), body);

    switch (m.typeIndex % 4) {
    case 0:
        DrawCircleV(m.pos, m.radius, body);
        DrawCircleV(m.pos, m.radius * 0.35f, core);
        break;
    case 1:
        DrawRectanglePro(
            Rectangle{ m.pos.x - m.radius, m.pos.y - m.radius, m.radius * 2.0f, m.radius * 2.0f },
            { m.radius, m.radius },
            45.0f,
            body
        );
        DrawCircleV(m.pos, m.radius * 0.28f, core);
        break;
    case 2:
        DrawTriangle(
            { m.pos.x, m.pos.y - m.radius * 1.1f },
            { m.pos.x - m.radius, m.pos.y + m.radius },
            { m.pos.x + m.radius, m.pos.y + m.radius },
            body
        );
        DrawCircleV(m.pos, m.radius * 0.22f, core);
        break;
    default:
        DrawPoly(m.pos, 6, m.radius, GetTime() * 45.0, body);
        DrawCircleV(m.pos, m.radius * 0.25f, core);
        break;
    }

    DrawCircleLines((int)m.pos.x, (int)m.pos.y, m.radius + 1.0f, Fade(WHITE, 0.6f));
}

int main() {
    int screenW = 1280;
    int screenH = 720;

    std::srand((unsigned int)std::time(nullptr));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE);
    InitWindow(screenW, screenH, "NEON ABYSS // NEXUS SIEGE");

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
    SetTargetFPS(60);

    const Color BG = { 7, 10, 18, 255 };
    const Color BG_2 = { 13, 18, 30, 255 };
    const Color GRID = { 22, 31, 52, 255 };
    const Color PANEL = { 10, 14, 28, 235 };
    const Color PANEL_2 = { 14, 20, 38, 245 };
    const Color NEON_CYAN = { 0, 255, 245, 255 };
    const Color NEON_BLUE = { 72, 145, 255, 255 };
    const Color NEON_PINK = { 255, 65, 185, 255 };
    const Color NEON_GOLD = { 255, 210, 80, 255 };
    const Color SOFT_RED = { 255, 90, 120, 255 };
    const Color SAFE_GREEN = { 90, 255, 160, 255 };
    const Color BOSS_PURPLE = { 182, 76, 255, 255 };

    std::vector<Weapon> weaponDB = {
        {"Rusty Pipe", 14, 65.0f, "Common", 0, LIGHTGRAY},
        {"Iron Baton", 18, 72.0f, "Common", 60, GRAY},
        {"Shock Stunner", 24, 82.0f, "Common", 120, NEON_CYAN},
        {"Chain Saw Blade", 34, 88.0f, "Common", 200, ORANGE},
        {"Plasma Cutter", 44, 96.0f, "Rare", 300, SKYBLUE},
        {"Laser Edge", 56, 104.0f, "Rare", 430, BLUE},
        {"Thermal Katana", 70, 112.0f, "Rare", 620, RED},
        {"Sonic Vibroblade", 86, 118.0f, "Rare", 850, LIME},
        {"Pulse Rifle Saber", 102, 126.0f, "Rare", 1100, NEON_CYAN},
        {"Void Reaper", 126, 138.0f, "Legendary", 1450, PURPLE},
        {"Arc Disruptor", 152, 146.0f, "Legendary", 1850, YELLOW},
        {"Singularity Core", 184, 154.0f, "Legendary", 2350, VIOLET},
        {"Hyperion Greatsword", 220, 164.0f, "Legendary", 3000, GOLD},
        {"Nanite Obliterator", 262, 174.0f, "Exotic", 3800, GREEN},
        {"Quantum Slicer", 308, 188.0f, "Exotic", 4800, BOSS_PURPLE},
        {"Celestial Cleaver", 370, 205.0f, "Exotic", 6200, WHITE},
        {"Omega Decimator", 460, 225.0f, "Exotic", 8200, NEON_PINK}
    };

    std::vector<MonsterType> monsterTypes = {
        {"Cyber Rat", 45, 8, 2.5f, 12.0f, GRAY, false, 18},
        {"Drone Scout", 60, 10, 3.0f, 14.0f, NEON_CYAN, false, 24},
        {"Syndicate Thug", 88, 14, 2.1f, 18.0f, ORANGE, false, 34},
        {"Enforcer Bot", 130, 18, 1.6f, 22.0f, RED, false, 48},
        {"Corrupted Cyborg", 175, 24, 1.8f, 20.0f, PURPLE, false, 64},
        {"Viper Assassin", 110, 22, 3.4f, 15.0f, LIME, false, 56},
        {"Heavy Automaton", 280, 30, 1.15f, 28.0f, DARKBLUE, false, 80},
        {"Neon Brute", 220, 28, 1.4f, 25.0f, PINK, false, 72},
        {"Pulse Wraith", 145, 20, 2.8f, 17.0f, SKYBLUE, false, 60},
        {"Scrap Spider", 95, 16, 2.9f, 13.0f, BROWN, false, 40},
        {"JUGGERNAUT PRIME", 1100, 36, 1.3f, 42.0f, MAROON, true, 550},
        {"THE ARCHITECT", 1800, 48, 1.05f, 48.0f, BOSS_PURPLE, true, 900}
    };

    std::vector<Star> stars;
    for (int i = 0; i < 120; ++i) {
        stars.push_back({
            { RandomRange(0.0f, (float)screenW), RandomRange(0.0f, (float)screenH) },
            RandomRange(12.0f, 90.0f),
            RandomRange(1.0f, 3.2f)
            });
    }

    GameState gameState = GameState::Title;

    Vector2 playerPos = { 0.0f, 0.0f };
    Vector2 aimDir = { 1.0f, 0.0f };
    float playerSpeed = 4.6f;
    int playerHp = 160;
    int playerMaxHp = 160;
    int xp = 0;
    int kills = 0;
    int wave = 1;
    int hpUpgradeLevel = 0;
    int equippedWeaponIdx = 0;
    int browseWeaponIdx = 0;
    float dashCd = 0.0f;
    float empCd = 0.0f;
    float turretCd = 0.0f;
    float attackCd = 0.0f;
    float playerHitFlash = 0.0f;
    float screenShake = 0.0f;
    float nextWaveTimer = 0.0f;
    float announcementTimer = 0.0f;
    float shopMessageTimer = 0.0f;
    bool inShopUI = false;
    std::string announcement = "";
    std::string shopMessage = "";
    Color shopMessageColor = WHITE;

    std::vector<bool> ownedWeapons(weaponDB.size(), false);
    ownedWeapons[0] = true;

    std::vector<ActiveMonster> monsters;
    std::vector<Particle> particles;
    std::vector<Orb> orbs;
    std::vector<FloatingText> floatingTexts;
    std::vector<Turret> turrets;
    std::vector<Shockwave> shockwaves;
    std::vector<Beam> beams;

    Rectangle safeZone = { -280.0f, -220.0f, 560.0f, 440.0f };

    Camera2D camera = { 0 };
    camera.offset = { screenW * 0.5f, screenH * 0.5f };
    camera.target = playerPos;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    auto AddFloatingText = [&](Vector2 pos, const std::string& text, Color color) {
        floatingTexts.push_back({ pos, { RandomRange(-0.35f, 0.35f), -1.2f }, text, color, 1.0f });
        };

    auto EmitBurst = [&](Vector2 pos, int count, float speed, Color color, float size) {
        for (int i = 0; i < count; ++i) {
            float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
            float s = RandomRange(speed * 0.4f, speed);
            particles.push_back({
                pos,
                { std::cos(angle) * s, std::sin(angle) * s },
                color,
                RandomRange(0.35f, 0.85f),
                RandomRange(0.35f, 0.85f),
                RandomRange(size * 0.7f, size * 1.35f)
                });
        }
        };

    auto SpawnMonsterByType = [&](int typeIndex, Vector2 pos) {
        const MonsterType& t = monsterTypes[typeIndex];
        monsters.push_back({
            pos,
            { 0.0f, 0.0f },
            t.maxHp,
            t.maxHp,
            t.damage,
            t.xpDrop,
            t.speed,
            t.radius,
            0.0f,
            RandomRange(0.15f, 0.8f),
            t.color,
            t.isBoss,
            typeIndex,
            t.name
            });
        };

    auto SpawnWave = [&](int waveNumber) {
        announcement = TextFormat("WAVE %d // PURGE PROTOCOL", waveNumber);
        announcementTimer = 2.8f;

        int normalCount = 5 + waveNumber * 2;
        int maxRegularType = 4 + waveNumber / 2;
        if (maxRegularType > 10) maxRegularType = 10;

        for (int i = 0; i < normalCount; ++i) {
            float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
            float dist = RandomRange(620.0f, 980.0f + waveNumber * 20.0f);
            Vector2 pos = { playerPos.x + std::cos(angle) * dist, playerPos.y + std::sin(angle) * dist };
            int typeIndex = std::rand() % maxRegularType;
            SpawnMonsterByType(typeIndex, pos);
        }

        if (waveNumber % 5 == 0) {
            int bossType = (waveNumber % 10 == 0) ? 11 : 10;
            float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
            Vector2 pos = { playerPos.x + std::cos(angle) * 1150.0f, playerPos.y + std::sin(angle) * 1150.0f };
            SpawnMonsterByType(bossType, pos);
            announcement = monsterTypes[bossType].name + std::string(" // BOSS INBOUND");
            announcementTimer = 3.8f;
        }
        };

    auto ResetRun = [&]() {
        playerPos = { 0.0f, 0.0f };
        aimDir = { 1.0f, 0.0f };
        playerSpeed = 4.6f;
        playerHp = 160;
        playerMaxHp = 160;
        xp = 240;
        kills = 0;
        wave = 1;
        hpUpgradeLevel = 0;
        equippedWeaponIdx = 0;
        browseWeaponIdx = 0;
        dashCd = 0.0f;
        empCd = 0.0f;
        turretCd = 0.0f;
        attackCd = 0.0f;
        playerHitFlash = 0.0f;
        screenShake = 0.0f;
        nextWaveTimer = 0.0f;
        announcement = "NEXUS ONLINE";
        announcementTimer = 2.2f;
        shopMessage = "";
        shopMessageTimer = 0.0f;
        inShopUI = false;

        ownedWeapons.assign(weaponDB.size(), false);
        ownedWeapons[0] = true;

        monsters.clear();
        particles.clear();
        orbs.clear();
        floatingTexts.clear();
        turrets.clear();
        shockwaves.clear();
        beams.clear();

        SpawnWave(wave);
        };

    ResetRun();
    gameState = GameState::Title;

    while (!WindowShouldClose()) {
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();

        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        for (auto& s : stars) {
            s.pos.y += s.speed * dt;
            s.pos.x += std::sin((float)GetTime() * 0.4f + s.speed * 0.01f) * 4.0f * dt;
            if (s.pos.y > screenH + 5.0f) {
                s.pos.y = -5.0f;
                s.pos.x = RandomRange(0.0f, (float)screenW);
            }
        }

        if (gameState == GameState::Title) {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetRun();
                gameState = GameState::Playing;
            }
        }
        else if (gameState == GameState::Playing) {
            attackCd = std::max(0.0f, attackCd - dt);
            dashCd = std::max(0.0f, dashCd - dt);
            empCd = std::max(0.0f, empCd - dt);
            turretCd = std::max(0.0f, turretCd - dt);
            playerHitFlash = std::max(0.0f, playerHitFlash - dt);
            screenShake = std::max(0.0f, screenShake - dt * 22.0f);
            announcementTimer = std::max(0.0f, announcementTimer - dt);
            shopMessageTimer = std::max(0.0f, shopMessageTimer - dt);

            bool inSafeZone = CheckCollisionPointRec(playerPos, safeZone);
            if (inSafeZone) {
                playerHp = std::min(playerMaxHp, playerHp + (int)(12.0f * dt));
            }

            if (inSafeZone && IsKeyPressed(KEY_E)) {
                inShopUI = !inShopUI;
                browseWeaponIdx = equippedWeaponIdx;
            }
            if (!inSafeZone) {
                inShopUI = false;
            }

            Vector2 moveInput = { 0.0f, 0.0f };
            if (!inShopUI) {
                if (IsKeyDown(KEY_W)) moveInput.y -= 1.0f;
                if (IsKeyDown(KEY_S)) moveInput.y += 1.0f;
                if (IsKeyDown(KEY_A)) moveInput.x -= 1.0f;
                if (IsKeyDown(KEY_D)) moveInput.x += 1.0f;
            }

            if (moveInput.x != 0.0f || moveInput.y != 0.0f) {
                moveInput = VecNormalizeSafe(moveInput);
                aimDir = moveInput;
                playerPos = VecAdd(playerPos, VecScale(moveInput, playerSpeed * 60.0f * dt));
                if ((int)(GetTime() * 18.0) % 2 == 0) {
                    particles.push_back({
                        VecSub(playerPos, VecScale(aimDir, 14.0f)),
                        { RandomRange(-0.5f, 0.5f), RandomRange(-0.5f, 0.5f) },
                        Fade(NEON_CYAN, 0.8f),
                        0.2f,
                        0.2f,
                        RandomRange(2.0f, 4.5f)
                        });
                }
            }

            if (!inShopUI && IsKeyPressed(KEY_SPACE) && attackCd <= 0.0f) {
                attackCd = 0.28f;
                const Weapon& w = weaponDB[equippedWeaponIdx];
                bool hitSomething = false;

                shockwaves.push_back({ playerPos, 12.0f, w.range, 0.22f, 0.22f, Fade(w.color, 0.7f) });
                EmitBurst(playerPos, 16, 4.6f, w.color, 4.0f);

                for (auto& m : monsters) {
                    float d = Distance(playerPos, m.pos);
                    if (d <= w.range) {
                        int damage = w.damage + (std::rand() % 7);
                        m.hp -= damage;
                        m.hitFlash = 0.12f;
                        AddFloatingText(m.pos, "-" + std::to_string(damage), w.color);
                        EmitBurst(m.pos, 9, 4.2f, w.color, 3.2f);
                        hitSomething = true;
                    }
                }

                if (hitSomething) {
                    screenShake = std::max(screenShake, 8.0f);
                }
            }

            if (!inShopUI && IsKeyPressed(KEY_ONE) && dashCd <= 0.0f) {
                dashCd = 3.0f;
                Vector2 dashDir = aimDir;
                if (dashDir.x == 0.0f && dashDir.y == 0.0f) dashDir = { 1.0f, 0.0f };

                for (int i = 0; i < 7; ++i) {
                    Vector2 trailPos = VecSub(playerPos, VecScale(dashDir, (float)i * 26.0f));
                    EmitBurst(trailPos, 5, 2.8f, NEON_CYAN, 4.5f);
                }

                playerPos = VecAdd(playerPos, VecScale(dashDir, 210.0f));
                shockwaves.push_back({ playerPos, 8.0f, 95.0f, 0.18f, 0.18f, Fade(NEON_CYAN, 0.8f) });
                screenShake = std::max(screenShake, 6.0f);
            }

            if (!inShopUI && IsKeyPressed(KEY_TWO) && empCd <= 0.0f) {
                empCd = 6.0f;
                int empDamage = 100 + wave * 6;
                shockwaves.push_back({ playerPos, 20.0f, 260.0f, 0.45f, 0.45f, Fade(NEON_CYAN, 0.85f) });
                EmitBurst(playerPos, 36, 7.5f, NEON_CYAN, 5.5f);
                screenShake = std::max(screenShake, 10.0f);

                for (auto& m : monsters) {
                    float d = Distance(playerPos, m.pos);
                    if (d <= 260.0f) {
                        m.hp -= empDamage;
                        m.hitFlash = 0.18f;
                        AddFloatingText(m.pos, "EMP " + std::to_string(empDamage), NEON_CYAN);
                        EmitBurst(m.pos, 12, 5.0f, NEON_CYAN, 3.0f);
                    }
                }
            }

            if (!inShopUI && IsKeyPressed(KEY_THREE) && turretCd <= 0.0f) {
                turretCd = 12.0f;
                turrets.push_back({ playerPos, 18.0f, 0.25f });
                EmitBurst(playerPos, 18, 3.0f, NEON_BLUE, 3.8f);
                AddFloatingText(playerPos, "TURRET ONLINE", NEON_BLUE);
            }

            for (auto& t : turrets) {
                t.life -= dt;
                t.fireTimer -= dt;

                if (t.fireTimer <= 0.0f && !monsters.empty()) {
                    int targetIndex = -1;
                    float bestDist = 260.0f;
                    for (int i = 0; i < (int)monsters.size(); ++i) {
                        float d = Distance(t.pos, monsters[i].pos);
                        if (d < bestDist) {
                            bestDist = d;
                            targetIndex = i;
                        }
                    }

                    if (targetIndex >= 0) {
                        ActiveMonster& m = monsters[targetIndex];
                        int turretDamage = 16 + wave * 2;
                        m.hp -= turretDamage;
                        m.hitFlash = 0.1f;
                        AddFloatingText(m.pos, "-" + std::to_string(turretDamage), NEON_BLUE);
                        EmitBurst(m.pos, 7, 3.8f, NEON_BLUE, 2.8f);
                        beams.push_back({ t.pos, m.pos, NEON_BLUE, 3.5f, 0.08f });
                        t.fireTimer = 0.32f;
                    }
                    else {
                        t.fireTimer = 0.1f;
                    }
                }
            }

            turrets.erase(std::remove_if(turrets.begin(), turrets.end(), [](const Turret& t) {
                return t.life <= 0.0f;
                }), turrets.end());

            for (auto& m : monsters) {
                m.hitFlash = std::max(0.0f, m.hitFlash - dt);
                m.attackTimer = std::max(0.0f, m.attackTimer - dt);

                Vector2 toPlayer = VecSub(playerPos, m.pos);
                float dist = VecLength(toPlayer);
                Vector2 dir = VecNormalizeSafe(toPlayer);

                if (CheckCollisionPointRec(m.pos, safeZone)) {
                    Vector2 away = VecNormalizeSafe(VecSub(m.pos, SafeZoneCenter(safeZone)));
                    m.pos = VecAdd(m.pos, VecScale(away, m.speed * 80.0f * dt));
                }
                else if (!inSafeZone && dist > 6.0f) {
                    m.vel = VecScale(dir, m.speed * 72.0f * dt);
                    m.pos = VecAdd(m.pos, m.vel);
                }

                if (!inSafeZone && dist <= m.radius + 18.0f && m.attackTimer <= 0.0f) {
                    playerHp -= m.damage;
                    if (playerHp < 0) playerHp = 0;
                    playerHitFlash = 0.18f;
                    m.attackTimer = m.isBoss ? 0.55f : 0.8f;
                    AddFloatingText(playerPos, "-" + std::to_string(m.damage), SOFT_RED);
                    EmitBurst(playerPos, 10, 3.0f, SOFT_RED, 3.0f);
                    screenShake = std::max(screenShake, m.isBoss ? 12.0f : 5.0f);
                }
            }

            for (auto it = monsters.begin(); it != monsters.end();) {
                if (it->hp <= 0) {
                    int orbCount = it->isBoss ? 14 : 4;
                    for (int i = 0; i < orbCount; ++i) {
                        float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
                        float speed = RandomRange(2.0f, 5.0f);
                        int value = std::max(1, it->xpDrop / orbCount);
                        orbs.push_back({
                            it->pos,
                            { std::cos(angle) * speed, std::sin(angle) * speed },
                            value,
                            it->isBoss ? NEON_GOLD : NEON_CYAN
                            });
                    }

                    EmitBurst(it->pos, it->isBoss ? 42 : 16, it->isBoss ? 7.0f : 4.0f, it->color, it->isBoss ? 5.0f : 3.2f);
                    shockwaves.push_back({ it->pos, 12.0f, it->isBoss ? 220.0f : 90.0f, it->isBoss ? 0.55f : 0.22f, it->isBoss ? 0.55f : 0.22f, Fade(it->color, 0.8f) });
                    AddFloatingText(it->pos, it->name + " DOWN", it->isBoss ? NEON_GOLD : WHITE);
                    kills++;
                    screenShake = std::max(screenShake, it->isBoss ? 14.0f : 4.0f);
                    it = monsters.erase(it);
                }
                else {
                    ++it;
                }
            }

            for (auto& o : orbs) {
                o.pos = VecAdd(o.pos, o.vel);
                o.vel = VecScale(o.vel, 0.96f);
                float d = Distance(o.pos, playerPos);
                if (d < 220.0f) {
                    Vector2 pull = VecNormalizeSafe(VecSub(playerPos, o.pos));
                    o.vel = VecAdd(o.vel, VecScale(pull, 0.55f));
                }
            }

            for (auto it = orbs.begin(); it != orbs.end();) {
                if (Distance(it->pos, playerPos) <= 18.0f) {
                    xp += it->value;
                    if (it->value >= 20) AddFloatingText(playerPos, "+" + std::to_string(it->value) + " XP", NEON_GOLD);
                    it = orbs.erase(it);
                }
                else {
                    ++it;
                }
            }

            for (auto& p : particles) {
                p.pos = VecAdd(p.pos, p.vel);
                p.vel = VecScale(p.vel, 0.96f);
                p.life -= dt;
            }
            particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) {
                return p.life <= 0.0f;
                }), particles.end());

            for (auto& s : shockwaves) {
                float t = 1.0f - (s.life / s.maxLife);
                s.radius = LerpFloat(12.0f, s.maxRadius, t);
                s.life -= dt;
            }
            shockwaves.erase(std::remove_if(shockwaves.begin(), shockwaves.end(), [](const Shockwave& s) {
                return s.life <= 0.0f;
                }), shockwaves.end());

            for (auto& b : beams) {
                b.life -= dt;
            }
            beams.erase(std::remove_if(beams.begin(), beams.end(), [](const Beam& b) {
                return b.life <= 0.0f;
                }), beams.end());

            for (auto& text : floatingTexts) {
                text.pos = VecAdd(text.pos, text.vel);
                text.life -= dt;
            }
            floatingTexts.erase(std::remove_if(floatingTexts.begin(), floatingTexts.end(), [](const FloatingText& t) {
                return t.life <= 0.0f;
                }), floatingTexts.end());

            if (monsters.empty()) {
                nextWaveTimer -= dt;
                if (nextWaveTimer <= 0.0f) {
                    wave++;
                    SpawnWave(wave);
                    nextWaveTimer = 2.2f;
                }
            }
            else {
                nextWaveTimer = 1.5f;
            }

            if (inShopUI) {
                int hpUpgradeCost = 100 + hpUpgradeLevel * 90;

                if (IsKeyPressed(KEY_RIGHT)) {
                    browseWeaponIdx = (browseWeaponIdx + 1) % (int)weaponDB.size();
                }
                if (IsKeyPressed(KEY_LEFT)) {
                    browseWeaponIdx = (browseWeaponIdx - 1 + (int)weaponDB.size()) % (int)weaponDB.size();
                }

                if (IsKeyPressed(KEY_H)) {
                    if (xp >= hpUpgradeCost) {
                        xp -= hpUpgradeCost;
                        hpUpgradeLevel++;
                        playerMaxHp += 30;
                        playerHp = std::min(playerMaxHp, playerHp + 30);
                        shopMessage = "MAX HP BOOSTED";
                        shopMessageColor = SAFE_GREEN;
                        shopMessageTimer = 1.5f;
                    }
                    else {
                        shopMessage = "NOT ENOUGH XP";
                        shopMessageColor = SOFT_RED;
                        shopMessageTimer = 1.2f;
                    }
                }

                if (IsKeyPressed(KEY_B)) {
                    if (ownedWeapons[browseWeaponIdx]) {
                        equippedWeaponIdx = browseWeaponIdx;
                        shopMessage = "WEAPON EQUIPPED";
                        shopMessageColor = NEON_CYAN;
                        shopMessageTimer = 1.2f;
                    }
                    else if (xp >= weaponDB[browseWeaponIdx].cost) {
                        xp -= weaponDB[browseWeaponIdx].cost;
                        ownedWeapons[browseWeaponIdx] = true;
                        equippedWeaponIdx = browseWeaponIdx;
                        shopMessage = "PURCHASE COMPLETE";
                        shopMessageColor = NEON_GOLD;
                        shopMessageTimer = 1.5f;
                    }
                    else {
                        shopMessage = "NOT ENOUGH XP";
                        shopMessageColor = SOFT_RED;
                        shopMessageTimer = 1.2f;
                    }
                }
            }

            if (playerHp <= 0) {
                gameState = GameState::GameOver;
                announcement = "SIGNAL LOST";
                announcementTimer = 3.0f;
            }
        }
        else if (gameState == GameState::GameOver) {
            announcementTimer = std::max(0.0f, announcementTimer - dt);
            if (IsKeyPressed(KEY_ENTER)) {
                ResetRun();
                gameState = GameState::Playing;
            }
        }

        Vector2 shakeOffset = { RandomRange(-screenShake, screenShake), RandomRange(-screenShake, screenShake) };
        camera.target = VecAdd(playerPos, VecScale(aimDir, 36.0f));
        camera.offset = { screenW * 0.5f + shakeOffset.x, screenH * 0.5f + shakeOffset.y };

        BeginDrawing();
        ClearBackground(BG);

        DrawRectangleGradientV(0, 0, screenW, screenH, BG_2, BG);
        for (const auto& s : stars) {
            DrawCircleV(s.pos, s.size, Fade(WHITE, 0.35f + s.size * 0.12f));
        }

        if (gameState == GameState::Title) {
            int titleW = MeasureText("NEON ABYSS", 72);
            DrawText("NEON ABYSS", screenW / 2 - titleW / 2, 110, 72, NEON_CYAN);
            DrawText("NEXUS SIEGE", screenW / 2 - MeasureText("NEXUS SIEGE", 28) / 2, 190, 28, NEON_PINK);
            DrawText("A top-down cyber action RPG vertical slice", screenW / 2 - MeasureText("A top-down cyber action RPG vertical slice", 22) / 2, 238, 22, RAYWHITE);

            DrawPanel({ screenW / 2.0f - 340.0f, 300.0f, 680.0f, 210.0f }, PANEL, NEON_BLUE);
            DrawText("FEATURES", screenW / 2 - MeasureText("FEATURES", 24) / 2, 322, 24, NEON_GOLD);
            DrawText("- Fast WASD movement with dash, EMP, and sentry turret", screenW / 2 - 285, 365, 20, RAYWHITE);
            DrawText("- Wave survival with elite enemies and boss encounters", screenW / 2 - 285, 395, 20, RAYWHITE);
            DrawText("- 17-weapon arsenal, safe-zone merchant, and permanent upgrades", screenW / 2 - 285, 425, 20, RAYWHITE);
            DrawText("- Neon particles, screen shake, XP orbs, and combat feedback", screenW / 2 - 285, 455, 20, RAYWHITE);

            Color pulse = ((int)(GetTime() * 2.5) % 2 == 0) ? NEON_CYAN : WHITE;
            DrawText("PRESS ENTER TO DEPLOY", screenW / 2 - MeasureText("PRESS ENTER TO DEPLOY", 26) / 2, 560, 26, pulse);
            DrawText("ESC closes the game", screenW / 2 - MeasureText("ESC closes the game", 16) / 2, 602, 16, GRAY);
        }
        else {
            BeginMode2D(camera);

            const int gridSize = 64;
            int startX = ((int)camera.target.x - screenW) / gridSize * gridSize - gridSize;
            int endX = ((int)camera.target.x + screenW) / gridSize * gridSize + gridSize;
            int startY = ((int)camera.target.y - screenH) / gridSize * gridSize - gridSize;
            int endY = ((int)camera.target.y + screenH) / gridSize * gridSize + gridSize;

            for (int x = startX; x <= endX; x += gridSize) {
                DrawLine(x, startY, x, endY, Fade(GRID, x % (gridSize * 4) == 0 ? 0.85f : 0.45f));
            }
            for (int y = startY; y <= endY; y += gridSize) {
                DrawLine(startX, y, endX, y, Fade(GRID, y % (gridSize * 4) == 0 ? 0.85f : 0.45f));
            }

            DrawRectangleRec(safeZone, Fade(SAFE_GREEN, 0.08f));
            DrawRectangleLinesEx(safeZone, 3.0f, SAFE_GREEN);

            Vector2 safeCenter = SafeZoneCenter(safeZone);
            float pulse = 150.0f + std::sin((float)GetTime() * 2.0f) * 10.0f;
            DrawCircleLines((int)safeCenter.x, (int)safeCenter.y, pulse, Fade(SAFE_GREEN, 0.28f));
            DrawCircleLines((int)safeCenter.x, (int)safeCenter.y, pulse + 32.0f, Fade(SAFE_GREEN, 0.16f));
            DrawText("NEXUS", (int)safeCenter.x - 36, (int)safeCenter.y - 10, 22, SAFE_GREEN);

            for (const auto& s : shockwaves) {
                float alpha = ClampFloat(s.life / s.maxLife, 0.0f, 1.0f);
                DrawCircleLines((int)s.pos.x, (int)s.pos.y, s.radius, Fade(s.color, alpha));
                DrawCircleLines((int)s.pos.x, (int)s.pos.y, s.radius + 4.0f, Fade(s.color, alpha * 0.45f));
            }

            for (const auto& t : turrets) {
                DrawGlowCircle(t.pos, 12.0f, NEON_BLUE);
                DrawCircleV(t.pos, 12.0f, NEON_BLUE);
                DrawCircleV(t.pos, 5.0f, WHITE);
                DrawCircleLines((int)t.pos.x, (int)t.pos.y, 260.0f, Fade(NEON_BLUE, 0.15f));
            }

            for (const auto& beam : beams) {
                DrawLineEx(beam.start, beam.end, beam.thickness + 3.0f, Fade(beam.color, beam.life * 3.0f));
                DrawLineEx(beam.start, beam.end, beam.thickness, WHITE);
            }

            for (const auto& orb : orbs) {
                DrawGlowCircle(orb.pos, 6.0f, orb.color);
                DrawCircleV(orb.pos, 4.0f, WHITE);
            }

            for (const auto& m : monsters) {
                DrawEnemySprite(m);

                Rectangle back = { m.pos.x - 24.0f, m.pos.y - m.radius - 16.0f, 48.0f, 6.0f };
                DrawRectangleRec(back, Fade(BLACK, 0.7f));
                DrawRectangleRec({ back.x, back.y, back.width * ((float)m.hp / (float)m.maxHp), back.height }, m.isBoss ? NEON_GOLD : SAFE_GREEN);

                if (m.isBoss) {
                    DrawText(m.name.c_str(), (int)m.pos.x - MeasureText(m.name.c_str(), 16) / 2, (int)(m.pos.y - m.radius - 38.0f), 16, NEON_GOLD);
                }
            }

            for (const auto& p : particles) {
                float alpha = ClampFloat(p.life / p.maxLife, 0.0f, 1.0f);
                DrawCircleV(p.pos, p.size + 2.0f, Fade(p.color, alpha * 0.18f));
                DrawCircleV(p.pos, p.size, Fade(p.color, alpha));
            }

            Color playerColor = (playerHitFlash > 0.0f) ? WHITE : NEON_CYAN;
            DrawGlowCircle(playerPos, 20.0f, playerColor);
            DrawCircleV(playerPos, 18.0f, playerColor);
            DrawCircleV(playerPos, 8.0f, WHITE);
            DrawLineEx(playerPos, VecAdd(playerPos, VecScale(aimDir, 28.0f)), 4.0f, NEON_PINK);
            DrawCircleLines((int)playerPos.x, (int)playerPos.y, weaponDB[equippedWeaponIdx].range, Fade(weaponDB[equippedWeaponIdx].color, 0.12f));

            for (const auto& text : floatingTexts) {
                DrawText(text.text.c_str(), (int)text.pos.x, (int)text.pos.y, 18, Fade(text.color, text.life));
            }

            EndMode2D();

            DrawPanel({ 18.0f, 18.0f, 390.0f, 146.0f }, PANEL, NEON_BLUE);
            DrawText("OPERATIVE STATUS", 34, 28, 20, NEON_CYAN);
            DrawText(TextFormat("WEAPON: %s", weaponDB[equippedWeaponIdx].name.c_str()), 34, 54, 18, WHITE);
            DrawText(TextFormat("WAVE %d", wave), 280, 28, 20, NEON_GOLD);
            DrawText(TextFormat("KILLS %d", kills), 280, 54, 18, RAYWHITE);

            DrawRectangle(34, 86, 220, 16, Fade(WHITE, 0.12f));
            DrawRectangle(34, 86, (int)(220.0f * ((float)playerHp / (float)playerMaxHp)), 16, playerHp < playerMaxHp * 0.3f ? SOFT_RED : SAFE_GREEN);
            DrawText(TextFormat("HP %d / %d", playerHp, playerMaxHp), 42, 84, 16, WHITE);

            DrawRectangle(34, 112, 220, 12, Fade(WHITE, 0.12f));
            DrawRectangle(34, 112, (int)ClampFloat((float)xp / 3000.0f * 220.0f, 0.0f, 220.0f), 12, NEON_GOLD);
            DrawText(TextFormat("XP BANK %d", xp), 262, 108, 16, NEON_GOLD);

            DrawPanel({ 18.0f, 176.0f, 265.0f, 118.0f }, PANEL, NEON_PINK);
            DrawText("ABILITIES", 32, 188, 18, NEON_PINK);
            DrawText(TextFormat("1 DASH    %.1fs", dashCd), 32, 214, 18, dashCd <= 0.0f ? NEON_CYAN : GRAY);
            DrawText(TextFormat("2 EMP     %.1fs", empCd), 32, 238, 18, empCd <= 0.0f ? NEON_CYAN : GRAY);
            DrawText(TextFormat("3 TURRET  %.1fs", turretCd), 32, 262, 18, turretCd <= 0.0f ? NEON_CYAN : GRAY);

            DrawPanel({ screenW - 292.0f, 18.0f, 274.0f, 118.0f }, PANEL, SAFE_GREEN);
            DrawText("FIELD DATA", screenW - 274, 28, 20, SAFE_GREEN);
            DrawText(TextFormat("HOSTILES %d", (int)monsters.size()), screenW - 274, 58, 18, WHITE);
            DrawText(TextFormat("TURRETS  %d", (int)turrets.size()), screenW - 274, 82, 18, WHITE);
            DrawText(CheckCollisionPointRec(playerPos, safeZone) ? "ZONE: SAFE" : "ZONE: HOT", screenW - 274, 106, 18,
                CheckCollisionPointRec(playerPos, safeZone) ? SAFE_GREEN : SOFT_RED);

            if (CheckCollisionPointRec(playerPos, safeZone)) {
                DrawText("SAFE ZONE // PRESS E TO ACCESS ARSENAL", 22, screenH - 34, 18, SAFE_GREEN);
            }
            else {
                DrawText("WASD MOVE   SPACE ATTACK   1 DASH   2 EMP   3 TURRET", 22, screenH - 34, 18, RAYWHITE);
            }

            if (announcementTimer > 0.0f) {
                int fontSize = 28;
                int width = MeasureText(announcement.c_str(), fontSize);
                DrawText(announcement.c_str(), screenW / 2 - width / 2, 24, fontSize, NEON_GOLD);
            }

            if (inShopUI) {
                DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.55f));
                Rectangle panel = { screenW / 2.0f - 390.0f, screenH / 2.0f - 240.0f, 780.0f, 480.0f };
                DrawPanel(panel, PANEL_2, NEON_GOLD);

                DrawText("NEXUS ARSENAL", (int)panel.x + 24, (int)panel.y + 22, 28, NEON_GOLD);
                DrawText(TextFormat("XP AVAILABLE: %d", xp), (int)panel.x + 540, (int)panel.y + 28, 20, SAFE_GREEN);

                int hpUpgradeCost = 100 + hpUpgradeLevel * 90;
                DrawPanel({ panel.x + 24.0f, panel.y + 72.0f, 732.0f, 94.0f }, PANEL, SAFE_GREEN);
                DrawText("BIOMETRIC UPGRADES", (int)panel.x + 40, (int)panel.y + 88, 22, SAFE_GREEN);
                DrawText(TextFormat("MAX HP BOOST +30   COST: %d XP   LEVEL: %d", hpUpgradeCost, hpUpgradeLevel), (int)panel.x + 40, (int)panel.y + 122, 20, WHITE);
                DrawText("PRESS H TO PURCHASE", (int)panel.x + 520, (int)panel.y + 122, 20, NEON_CYAN);

                const Weapon& browse = weaponDB[browseWeaponIdx];
                DrawPanel({ panel.x + 24.0f, panel.y + 186.0f, 732.0f, 188.0f }, PANEL, browse.color);
                DrawText("WEAPON VAULT", (int)panel.x + 40, (int)panel.y + 204, 22, browse.color);
                DrawText(browse.name.c_str(), (int)panel.x + 40, (int)panel.y + 238, 30, WHITE);
                DrawText(TextFormat("RARITY: %s", browse.rarity.c_str()), (int)panel.x + 40, (int)panel.y + 278, 20, browse.color);
                DrawText(TextFormat("DAMAGE: %d", browse.damage), (int)panel.x + 260, (int)panel.y + 278, 20, WHITE);
                DrawText(TextFormat("RANGE: %.0f", browse.range), (int)panel.x + 430, (int)panel.y + 278, 20, WHITE);
                DrawText(TextFormat("COST: %d XP", browse.cost), (int)panel.x + 600, (int)panel.y + 278, 20, NEON_GOLD);

                std::string stateLabel = ownedWeapons[browseWeaponIdx] ? (equippedWeaponIdx == browseWeaponIdx ? "EQUIPPED" : "OWNED") : "UNOWNED";
                Color stateColor = ownedWeapons[browseWeaponIdx] ? (equippedWeaponIdx == browseWeaponIdx ? NEON_CYAN : SAFE_GREEN) : SOFT_RED;
                DrawText(TextFormat("STATUS: %s", stateLabel.c_str()), (int)panel.x + 40, (int)panel.y + 318, 22, stateColor);
                DrawText("LEFT / RIGHT TO BROWSE   B TO BUY OR EQUIP", (int)panel.x + 300, (int)panel.y + 318, 20, RAYWHITE);

                DrawPanel({ panel.x + 24.0f, panel.y + 392.0f, 732.0f, 56.0f }, PANEL, NEON_BLUE);
                DrawText("E CLOSE", (int)panel.x + 40, (int)panel.y + 408, 20, NEON_BLUE);
                DrawText("BUY ONCE, EQUIP ANYTIME", (int)panel.x + 250, (int)panel.y + 408, 20, RAYWHITE);

                if (shopMessageTimer > 0.0f) {
                    DrawText(shopMessage.c_str(), (int)panel.x + 560, (int)panel.y + 408, 20, shopMessageColor);
                }
            }

            if (gameState == GameState::GameOver) {
                DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.72f));
                DrawText("SIGNAL LOST", screenW / 2 - MeasureText("SIGNAL LOST", 56) / 2, 180, 56, SOFT_RED);
                DrawText(TextFormat("WAVE REACHED: %d", wave), screenW / 2 - MeasureText(TextFormat("WAVE REACHED: %d", wave), 28) / 2, 280, 28, WHITE);
                DrawText(TextFormat("TOTAL KILLS: %d", kills), screenW / 2 - MeasureText(TextFormat("TOTAL KILLS: %d", kills), 28) / 2, 320, 28, WHITE);
                DrawText(TextFormat("XP BANKED: %d", xp), screenW / 2 - MeasureText(TextFormat("XP BANKED: %d", xp), 28) / 2, 360, 28, NEON_GOLD);
                DrawText("PRESS ENTER TO REBOOT RUN", screenW / 2 - MeasureText("PRESS ENTER TO REBOOT RUN", 26) / 2, 450, 26, NEON_CYAN);
            }
        }

        for (int y = 0; y < screenH; y += 4) {
            DrawRectangle(0, y, screenW, 1, Fade(BLACK, 0.055f));
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
