#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include "Player.h"
#include "Weapon.h"
#include "Monster.h"
#include "Particle.h"
#include "Shop.h"
#include "Dungeon.h"
#include "Relic.h"
#include "Tilemap.h"
#include "Quest.h"
#include "Pet.h"

enum class WorldId {
    Crownheart = 0,
    Frostveil = 1,
    Sunscar = 2,
    Mirethorn = 3
};

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
    bool questBoardOpen = false;
    bool realmMapOpen = false;
    Vector2 rewardChestPos = { 0.0f, 0.0f };
    std::vector<RelicType> relics;
    std::vector<RelicChoice> rewardChoices;
    int euro = 0;
    WorldId currentWorld = WorldId::Crownheart;
    int mainQuestIndex = 0;
    int mainQuestProgress = 0;
    bool mainQuestReady = false;

    std::vector<Weapon> weaponDB;
    std::vector<MonsterType> monsterTypes;
    std::vector<QuestDefinition> mainQuestDB;
    std::vector<QuestDefinition> sideQuestDB;
    std::vector<PetDefinition> petDB;
    std::vector<int> sideQuestOfferIndices;
    std::vector<int> sideQuestProgress;
    std::vector<bool> sideQuestAccepted;
    std::vector<bool> sideQuestReady;
    std::vector<Star> stars;
    std::vector<bool> persistentOwnedWeapons;
    std::vector<bool> realmSignatureClaimed;
    std::vector<bool> persistentOwnedPets;
    int persistentHpUpgradeLevel = 0;
    int persistentEquippedWeaponIdx = 0;
    int persistentEquippedPetIdx = -1;
    int legacyRenown = 0;
    TileMap tileMap;
    Texture2D tileAtlas{};
    Texture2D propAtlas{};
    Texture2D actorAtlas{};
    Texture2D enemyAtlas{};
    Texture2D weaponAtlas{};
    Texture2D petAtlas{};
    std::vector<PropInstance> worldProps;
    std::vector<PropInstance> decorProps;
    std::vector<ActiveMonster> monsters;
    std::vector<Particle> particles;
    std::vector<Orb> orbs;
    std::vector<HealthPickup> healthPickups;
    std::vector<FloatingText> floatingTexts;
    std::vector<Turret> turrets;
    std::vector<Shockwave> shockwaves;
    std::vector<Beam> beams;
    ActivePet pet;

    void BuildColorTheme();
    void BuildDatabases();
    void BuildQuestData();
    void BuildStars();
    void BuildDungeon();
    void BuildTileMap();
    void LoadAssets();
    void UnloadAssets();
    void ResetRun();

    Vector2 MoveWithCollision(Vector2 start, Vector2 delta, float radius, int steps = 1) const;
    Vector2 GetSpawnPointInCombatRoom() const;
    const DungeonArea* GetCurrentArea(Vector2 pos) const;
    std::vector<Rectangle> GetActiveBarrierRects() const;
    int CountRelic(RelicType type) const;
    float GetMoveSpeed() const;
    float GetAttackCooldown() const;
    float GetWeaponAttackCooldown(const Weapon& weapon) const;
    float GetDashCooldown() const;
    float GetCritChance() const;
    float GetCritMultiplier() const;
    int GetFlatDamageBonus() const;
    int GetWeaponDamageAgainst(const Weapon& weapon, const ActiveMonster& monster) const;
    int GetEmpDamage() const;
    float GetEmpRadius() const;
    int GetTurretDamage() const;
    float GetTurretLifetime() const;
    int GetHealPickupValue(int baseValue) const;
    void BuildRewardChoices();
    void ApplyRelic(RelicType type);
    void AdvanceQuestObjective(QuestObjective objective, int amount = 1);
    void RefreshSideQuestOffer(int slot);
    bool IsWorldUnlocked(WorldId world) const;
    void TravelToWorld(WorldId world);
    void GrantRealmSignatureIfNeeded(WorldId world);
    bool HasSaveFile() const;
    bool LoadRun();
    bool LoadProfile();
    void SaveRun() const;
    void SaveProfile() const;
    void DeleteSave() const;

    void AddFloatingText(Vector2 pos, const std::string& text, Color color);
    void EmitBurst(Vector2 pos, int count, float speed, Color color, float size);
    void SpawnMonsterByType(int typeIndex, Vector2 pos);
    void SpawnWave(int waveNumber);
    void SyncPetState(bool snapToPlayer);
    void ApplyWeaponHitEffect(const Weapon& weapon, ActiveMonster& monster, int damage, bool isCrit);

    void UpdateTitle();
    void UpdatePlaying(float dt);
    void UpdateGameOver(float dt);
    void UpdateStars(float dt);
    void UpdateCamera();

    Rectangle TileSourceRect(int index) const;
    Rectangle PropSourceRect(int index) const;
    Rectangle ActorSourceRect(int index) const;
    Rectangle EnemySourceRect(int index) const;
    Rectangle WeaponSourceRect(int index) const;
    Rectangle PetSourceRect(int index) const;

    void Draw() const;
    void DrawTitleScreen() const;
    void DrawWorld() const;
    void DrawHud() const;
    void DrawMiniMap() const;
    void DrawRewardOverlay() const;
    void DrawShop() const;
    void DrawQuestBoard() const;
    void DrawRealmMap() const;
    void DrawGameOver() const;
    void DrawEnemySprite(const ActiveMonster& monster) const;
    void DrawPlayerWeapon() const;
    void DrawPetSprite() const;
};
