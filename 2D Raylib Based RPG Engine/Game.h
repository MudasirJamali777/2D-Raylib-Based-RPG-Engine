#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include "Player.h"
#include "Weapon.h"
#include "Monster.h"
#include "Particle.h"
#include "Shop.h"
#include "Dungeon.h"
#include "Relic.h"

enum class GameState {
    Title,
    Playing,
    GameOver
};

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    int screenW = 1280;
    int screenH = 720;

    Color bg{};
    Color bg2{};
    Color grid{};
    Color panel{};
    Color panel2{};
    Color neonCyan{};
    Color neonBlue{};
    Color neonPink{};
    Color neonGold{};
    Color softRed{};
    Color safeGreen{};
    Color bossPurple{};

    GameState gameState = GameState::Title;
    Player player;
    ShopState shop;
    Camera2D camera{};
    Rectangle safeZone{};
    DungeonMap dungeon;

    float screenShake = 0.0f;
    float nextWaveTimer = 0.0f;
    float announcementTimer = 0.0f;
    float safeZoneHealBuffer = 0.0f;
    float hitStopTimer = 0.0f;
    std::string announcement;

    int waveTargetRoomIndex = -1;
    int lockedRoomIndex = -1;
    bool rewardChestActive = false;
    bool rewardSelectionOpen = false;
    Vector2 rewardChestPos = { 0.0f, 0.0f };
    std::vector<RelicType> relics;
    std::vector<RelicChoice> rewardChoices;

    std::vector<Weapon> weaponDB;
    std::vector<MonsterType> monsterTypes;
    std::vector<Star> stars;
    std::vector<ActiveMonster> monsters;
    std::vector<Particle> particles;
    std::vector<Orb> orbs;
    std::vector<HealthPickup> healthPickups;
    std::vector<FloatingText> floatingTexts;
    std::vector<Turret> turrets;
    std::vector<Shockwave> shockwaves;
    std::vector<Beam> beams;

    void BuildColorTheme();
    void BuildDatabases();
    void BuildStars();
    void BuildDungeon();
    void ResetRun();

    Vector2 MoveWithCollision(Vector2 start, Vector2 delta, float radius, int steps = 1) const;
    Vector2 GetSpawnPointInCombatRoom() const;
    const DungeonArea* GetCurrentArea(Vector2 pos) const;
    std::vector<Rectangle> GetActiveBarrierRects() const;
    int CountRelic(RelicType type) const;
    float GetMoveSpeed() const;
    float GetAttackCooldown() const;
    float GetDashCooldown() const;
    float GetCritChance() const;
    float GetCritMultiplier() const;
    int GetFlatDamageBonus() const;
    int GetEmpDamage() const;
    float GetEmpRadius() const;
    int GetTurretDamage() const;
    float GetTurretLifetime() const;
    int GetHealPickupValue(int baseValue) const;
    void BuildRewardChoices();
    void ApplyRelic(RelicType type);
    bool HasSaveFile() const;
    bool LoadRun();
    void SaveRun() const;
    void DeleteSave() const;

    void AddFloatingText(Vector2 pos, const std::string& text, Color color);
    void EmitBurst(Vector2 pos, int count, float speed, Color color, float size);
    void SpawnMonsterByType(int typeIndex, Vector2 pos);
    void SpawnWave(int waveNumber);

    void UpdateTitle();
    void UpdatePlaying(float dt);
    void UpdateGameOver(float dt);
    void UpdateStars(float dt);
    void UpdateCamera();

    void Draw() const;
    void DrawTitleScreen() const;
    void DrawWorld() const;
    void DrawHud() const;
    void DrawMiniMap() const;
    void DrawRewardOverlay() const;
    void DrawShop() const;
    void DrawGameOver() const;
    void DrawEnemySprite(const ActiveMonster& monster) const;
};
