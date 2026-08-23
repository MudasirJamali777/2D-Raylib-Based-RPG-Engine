#include "Game.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

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

    safeZone = { -280.0f, -220.0f, 560.0f, 440.0f };

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

        float dt = GetFrameTime();
        if (dt > 0.033f) {
            dt = 0.033f;
        }

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

void Game::ResetRun() {
    player = Player{};
    player.xp = 240;

    shop = ShopState{};
    shop.ownedWeapons.assign(weaponDB.size(), false);
    if (!shop.ownedWeapons.empty()) {
        shop.ownedWeapons[0] = true;
    }

    screenShake = 0.0f;
    nextWaveTimer = 0.0f;
    safeZoneHealBuffer = 0.0f;
    announcement = "NEXUS ONLINE";
    announcementTimer = 2.2f;

    monsters.clear();
    particles.clear();
    orbs.clear();
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

    monsters.push_back({
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
        typeIndex,
        type.name
        });
}

void Game::SpawnWave(int waveNumber) {
    announcement = std::string(TextFormat("WAVE %d // PURGE PROTOCOL", waveNumber));
    announcementTimer = 2.8f;

    int normalCount = 5 + waveNumber * 2;
    int maxRegularType = 4 + waveNumber / 2;
    if (maxRegularType > 10) {
        maxRegularType = 10;
    }

    for (int i = 0; i < normalCount; ++i) {
        float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
        float dist = RandomRange(620.0f, 980.0f + waveNumber * 20.0f);
        Vector2 pos = {
            player.pos.x + std::cos(angle) * dist,
            player.pos.y + std::sin(angle) * dist
        };

        int typeIndex = std::rand() % maxRegularType;
        SpawnMonsterByType(typeIndex, pos);
    }

    if (waveNumber % 5 == 0) {
        int bossType = (waveNumber % 10 == 0) ? 11 : 10;
        float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
        Vector2 pos = {
            player.pos.x + std::cos(angle) * 1150.0f,
            player.pos.y + std::sin(angle) * 1150.0f
        };

        SpawnMonsterByType(bossType, pos);
        announcement = monsterTypes[bossType].name + " // BOSS INBOUND";
        announcementTimer = 3.8f;
    }
}

void Game::UpdateTitle() {
    if (IsKeyPressed(KEY_ENTER)) {
        ResetRun();
        gameState = GameState::Playing;
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

    if (inSafeZone) {
        safeZoneHealBuffer += 18.0f * dt;
        while (safeZoneHealBuffer >= 1.0f) {
            player.hp = std::min(player.maxHp, player.hp + 1);
            safeZoneHealBuffer -= 1.0f;
        }
    }
    else {
        safeZoneHealBuffer = 0.0f;
    }

    if (inSafeZone && IsKeyPressed(KEY_E)) {
        shop.isOpen = !shop.isOpen;
        shop.browseWeaponIdx = player.equippedWeaponIdx;
    }
    if (!inSafeZone) {
        shop.isOpen = false;
    }

    Vector2 moveInput = { 0.0f, 0.0f };
    if (!shop.isOpen) {
        if (IsKeyDown(KEY_W)) moveInput.y -= 1.0f;
        if (IsKeyDown(KEY_S)) moveInput.y += 1.0f;
        if (IsKeyDown(KEY_A)) moveInput.x -= 1.0f;
        if (IsKeyDown(KEY_D)) moveInput.x += 1.0f;
    }

    if (moveInput.x != 0.0f || moveInput.y != 0.0f) {
        moveInput = VecNormalizeSafe(moveInput);
        player.aimDir = moveInput;
        player.pos = VecAdd(player.pos, VecScale(moveInput, player.speed * 60.0f * dt));

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

    if (!shop.isOpen && IsKeyPressed(KEY_SPACE) && player.attackCd <= 0.0f) {
        player.attackCd = 0.28f;
        const Weapon& weapon = weaponDB[player.equippedWeaponIdx];
        bool hitSomething = false;

        shockwaves.push_back({ player.pos, 12.0f, weapon.range, 0.22f, 0.22f, Fade(weapon.color, 0.7f) });
        EmitBurst(player.pos, 16, 4.6f, weapon.color, 4.0f);

        for (auto& monster : monsters) {
            float dist = Distance(player.pos, monster.pos);
            if (dist <= weapon.range) {
                int damage = weapon.damage + (std::rand() % 7);
                monster.hp -= damage;
                monster.hitFlash = 0.12f;
                AddFloatingText(monster.pos, "-" + std::to_string(damage), weapon.color);
                EmitBurst(monster.pos, 9, 4.2f, weapon.color, 3.2f);
                hitSomething = true;
            }
        }

        if (hitSomething) {
            screenShake = std::max(screenShake, 8.0f);
        }
    }

    if (!shop.isOpen && IsKeyPressed(KEY_ONE) && player.dashCd <= 0.0f) {
        player.dashCd = 3.0f;
        Vector2 dashDir = player.aimDir;
        if (dashDir.x == 0.0f && dashDir.y == 0.0f) {
            dashDir = { 1.0f, 0.0f };
        }

        for (int i = 0; i < 7; ++i) {
            Vector2 trailPos = VecSub(player.pos, VecScale(dashDir, (float)i * 26.0f));
            EmitBurst(trailPos, 5, 2.8f, neonCyan, 4.5f);
        }

        player.pos = VecAdd(player.pos, VecScale(dashDir, 210.0f));
        shockwaves.push_back({ player.pos, 8.0f, 95.0f, 0.18f, 0.18f, Fade(neonCyan, 0.8f) });
        screenShake = std::max(screenShake, 6.0f);
    }

    if (!shop.isOpen && IsKeyPressed(KEY_TWO) && player.empCd <= 0.0f) {
        player.empCd = 6.0f;
        int empDamage = 100 + player.wave * 6;

        shockwaves.push_back({ player.pos, 20.0f, 260.0f, 0.45f, 0.45f, Fade(neonCyan, 0.85f) });
        EmitBurst(player.pos, 36, 7.5f, neonCyan, 5.5f);
        screenShake = std::max(screenShake, 10.0f);

        for (auto& monster : monsters) {
            float dist = Distance(player.pos, monster.pos);
            if (dist <= 260.0f) {
                monster.hp -= empDamage;
                monster.hitFlash = 0.18f;
                AddFloatingText(monster.pos, "EMP " + std::to_string(empDamage), neonCyan);
                EmitBurst(monster.pos, 12, 5.0f, neonCyan, 3.0f);
            }
        }
    }

    if (!shop.isOpen && IsKeyPressed(KEY_THREE) && player.turretCd <= 0.0f) {
        player.turretCd = 12.0f;
        turrets.push_back({ player.pos, 18.0f, 0.25f });
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
                int turretDamage = 16 + player.wave * 2;
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

        if (CheckCollisionPointRec(monster.pos, safeZone)) {
            Vector2 away = VecNormalizeSafe(VecSub(monster.pos, SafeZoneCenter(safeZone)));
            monster.pos = VecAdd(monster.pos, VecScale(away, monster.speed * 80.0f * dt));
        }
        else if (!inSafeZone && dist > 6.0f) {
            monster.vel = VecScale(dir, monster.speed * 72.0f * dt);
            monster.pos = VecAdd(monster.pos, monster.vel);
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
            int orbCount = it->isBoss ? 14 : 4;

            for (int i = 0; i < orbCount; ++i) {
                float angle = RandomRange(0.0f, 360.0f) * DEG2RAD;
                float speed = RandomRange(2.0f, 5.0f);
                int value = std::max(1, it->xpDrop / orbCount);

                orbs.push_back({
                    it->pos,
                    { std::cos(angle) * speed, std::sin(angle) * speed },
                    value,
                    it->isBoss ? neonGold : neonCyan
                    });
            }

            EmitBurst(it->pos, it->isBoss ? 42 : 16, it->isBoss ? 7.0f : 4.0f, it->color, it->isBoss ? 5.0f : 3.2f);
            shockwaves.push_back({ it->pos, 12.0f, it->isBoss ? 220.0f : 90.0f, it->isBoss ? 0.55f : 0.22f, it->isBoss ? 0.55f : 0.22f, Fade(it->color, 0.8f) });
            AddFloatingText(it->pos, it->name + " DOWN", it->isBoss ? neonGold : WHITE);
            player.kills++;
            screenShake = std::max(screenShake, it->isBoss ? 14.0f : 4.0f);
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

    if (monsters.empty()) {
        nextWaveTimer -= dt;
        if (nextWaveTimer <= 0.0f) {
            player.wave++;
            SpawnWave(player.wave);
            nextWaveTimer = 2.2f;
        }
    }
    else {
        nextWaveTimer = 1.5f;
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
            }
            else if (player.xp >= weaponDB[shop.browseWeaponIdx].cost) {
                player.xp -= weaponDB[shop.browseWeaponIdx].cost;
                shop.ownedWeapons[shop.browseWeaponIdx] = true;
                player.equippedWeaponIdx = shop.browseWeaponIdx;
                shop.message = "PURCHASE COMPLETE";
                shop.messageColor = neonGold;
                shop.messageTimer = 1.5f;
            }
            else {
                shop.message = "NOT ENOUGH XP";
                shop.messageColor = softRed;
                shop.messageTimer = 1.2f;
            }
        }
    }

    if (player.hp <= 0) {
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
    DrawText("- Neon particles, screen shake, XP orbs, and combat feedback", screenW / 2 - 285, 455, 20, RAYWHITE);

    Color pulse = ((int)(GetTime() * 2.5) % 2 == 0) ? neonCyan : WHITE;
    DrawText("PRESS ENTER TO DEPLOY", screenW / 2 - MeasureText("PRESS ENTER TO DEPLOY", 26) / 2, 560, 26, pulse);
    DrawText("ESC closes the game", screenW / 2 - MeasureText("ESC closes the game", 16) / 2, 602, 16, GRAY);
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

    DrawRectangleRec(safeZone, Fade(safeGreen, 0.08f));
    DrawRectangleLinesEx(safeZone, 3.0f, safeGreen);

    Vector2 safeCenter = SafeZoneCenter(safeZone);
    float pulse = 150.0f + std::sin((float)GetTime() * 2.0f) * 10.0f;
    DrawCircleLines((int)safeCenter.x, (int)safeCenter.y, pulse, Fade(safeGreen, 0.28f));
    DrawCircleLines((int)safeCenter.x, (int)safeCenter.y, pulse + 32.0f, Fade(safeGreen, 0.16f));
    DrawText("NEXUS", (int)safeCenter.x - 36, (int)safeCenter.y - 10, 22, safeGreen);

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
    DrawGlowCircle(player.pos, 20.0f, playerColor);
    DrawCircleV(player.pos, 18.0f, playerColor);
    DrawCircleV(player.pos, 8.0f, WHITE);
    DrawLineEx(player.pos, VecAdd(player.pos, VecScale(player.aimDir, 28.0f)), 4.0f, neonPink);
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, weaponDB[player.equippedWeaponIdx].range, Fade(weaponDB[player.equippedWeaponIdx].color, 0.12f));

    for (const auto& text : floatingTexts) {
        DrawText(text.text.c_str(), (int)text.pos.x, (int)text.pos.y, 18, Fade(text.color, text.life));
    }

    EndMode2D();
}

void Game::DrawHud() const {
    DrawPanel({ 18.0f, 18.0f, 390.0f, 146.0f }, panel, neonBlue);
    DrawText("OPERATIVE STATUS", 34, 28, 20, neonCyan);
    DrawText(TextFormat("WEAPON: %s", weaponDB[player.equippedWeaponIdx].name.c_str()), 34, 54, 18, WHITE);
    DrawText(TextFormat("WAVE %d", player.wave), 280, 28, 20, neonGold);
    DrawText(TextFormat("KILLS %d", player.kills), 280, 54, 18, RAYWHITE);

    DrawRectangle(34, 86, 220, 16, Fade(WHITE, 0.12f));
    DrawRectangle(34, 86, (int)(220.0f * ((float)player.hp / (float)player.maxHp)), 16, player.hp < player.maxHp * 0.3f ? softRed : safeGreen);
    DrawText(TextFormat("HP %d / %d", player.hp, player.maxHp), 42, 84, 16, WHITE);

    DrawRectangle(34, 112, 220, 12, Fade(WHITE, 0.12f));
    DrawRectangle(34, 112, (int)ClampFloat((float)player.xp / 3000.0f * 220.0f, 0.0f, 220.0f), 12, neonGold);
    DrawText(TextFormat("XP BANK %d", player.xp), 262, 108, 16, neonGold);

    DrawPanel({ 18.0f, 176.0f, 265.0f, 118.0f }, panel, neonPink);
    DrawText("ABILITIES", 32, 188, 18, neonPink);
    DrawText(TextFormat("1 DASH    %.1fs", player.dashCd), 32, 214, 18, player.dashCd <= 0.0f ? neonCyan : GRAY);
    DrawText(TextFormat("2 EMP     %.1fs", player.empCd), 32, 238, 18, player.empCd <= 0.0f ? neonCyan : GRAY);
    DrawText(TextFormat("3 TURRET  %.1fs", player.turretCd), 32, 262, 18, player.turretCd <= 0.0f ? neonCyan : GRAY);

    DrawPanel({ screenW - 292.0f, 18.0f, 274.0f, 118.0f }, panel, safeGreen);
    DrawText("FIELD DATA", screenW - 274, 28, 20, safeGreen);
    DrawText(TextFormat("HOSTILES %d", (int)monsters.size()), screenW - 274, 58, 18, WHITE);
    DrawText(TextFormat("TURRETS  %d", (int)turrets.size()), screenW - 274, 82, 18, WHITE);

    bool inSafeZone = CheckCollisionPointRec(player.pos, safeZone);
    DrawText(inSafeZone ? "ZONE: SAFE" : "ZONE: HOT", screenW - 274, 106, 18, inSafeZone ? safeGreen : softRed);

    if (inSafeZone) {
        DrawText("SAFE ZONE // PRESS E TO ACCESS ARSENAL", 22, screenH - 34, 18, safeGreen);
    }
    else {
        DrawText("WASD MOVE   SPACE ATTACK   1 DASH   2 EMP   3 TURRET", 22, screenH - 34, 18, RAYWHITE);
    }

    if (announcementTimer > 0.0f) {
        int fontSize = 28;
        int width = MeasureText(announcement.c_str(), fontSize);
        DrawText(announcement.c_str(), screenW / 2 - width / 2, 24, fontSize, neonGold);
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
    Color core = Fade(WHITE, 0.9f);

    DrawGlowCircle(monster.pos, monster.radius + (monster.isBoss ? 5.0f : 0.0f), body);

    switch (monster.typeIndex % 4) {
    case 0:
        DrawCircleV(monster.pos, monster.radius, body);
        DrawCircleV(monster.pos, monster.radius * 0.35f, core);
        break;
    case 1:
        DrawRectanglePro(
            Rectangle{ monster.pos.x - monster.radius, monster.pos.y - monster.radius, monster.radius * 2.0f, monster.radius * 2.0f },
            { monster.radius, monster.radius },
            45.0f,
            body
        );
        DrawCircleV(monster.pos, monster.radius * 0.28f, core);
        break;
    case 2:
        DrawTriangle(
            { monster.pos.x, monster.pos.y - monster.radius * 1.1f },
            { monster.pos.x - monster.radius, monster.pos.y + monster.radius },
            { monster.pos.x + monster.radius, monster.pos.y + monster.radius },
            body
        );
        DrawCircleV(monster.pos, monster.radius * 0.22f, core);
        break;
    default:
        DrawPoly(monster.pos, 6, monster.radius, (float)GetTime() * 45.0f, body);
        DrawCircleV(monster.pos, monster.radius * 0.25f, core);
        break;
    }

    DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 1.0f, Fade(WHITE, 0.6f));
}
