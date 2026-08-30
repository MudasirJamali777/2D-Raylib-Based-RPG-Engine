#include "Game.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cstdio>
#include <random>

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall PlaySoundA(const char* pszSound, void* hmod, unsigned int fdwSound);
#pragma comment(lib, "winmm.lib")

static constexpr unsigned int SND_ASYNC_FALLBACK = 0x0001u;
static constexpr unsigned int SND_NODEFAULT_FALLBACK = 0x0002u;
static constexpr unsigned int SND_FILENAME_FALLBACK = 0x00020000u;
#endif

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
        "Release/",
        "Debug/",
        "./Release/",
        "./Debug/",
        "../",
        "../Release/",
        "../Debug/",
        "../../",
        "../../../",
        "x64/Release/",
        "x64/Debug/",
        "./x64/Release/",
        "./x64/Debug/",
        "../x64/Release/",
        "../x64/Debug/",
        "Release/assets/../",
        "Debug/assets/../"
    };

    for (const char* prefix : candidates) {
        std::string path = std::string(prefix) + relativePath;
        if (FileExistsPortable(path)) {
            return path;
        }
    }

    return relativePath;
}

enum SfxId : unsigned int {
    SFX_NONE = 0,
    SFX_UI_MOVE,
    SFX_UI_ACCEPT,
    SFX_UI_DENY,
    SFX_SWORD_LIGHT,
    SFX_SWORD_HEAVY,
    SFX_HIT,
    SFX_CRIT,
    SFX_DASH,
    SFX_BURST,
    SFX_BANNER,
    SFX_QUEST,
    SFX_PET,
    SFX_HEAL,
    SFX_BOSS_INTRO,
    SFX_TRAVEL,
    SFX_REWARD,
    SFX_GAME_OVER
};

static const char* GetSfxPath(unsigned int id) {
    switch (id) {
    case SFX_UI_MOVE: return "assets/sfx/ui_move.wav";
    case SFX_UI_ACCEPT: return "assets/sfx/ui_accept.wav";
    case SFX_UI_DENY: return "assets/sfx/ui_deny.wav";
    case SFX_SWORD_LIGHT: return "assets/sfx/sword_light.wav";
    case SFX_SWORD_HEAVY: return "assets/sfx/sword_heavy.wav";
    case SFX_HIT: return "assets/sfx/hit.wav";
    case SFX_CRIT: return "assets/sfx/crit.wav";
    case SFX_DASH: return "assets/sfx/dash.wav";
    case SFX_BURST: return "assets/sfx/burst.wav";
    case SFX_BANNER: return "assets/sfx/banner.wav";
    case SFX_QUEST: return "assets/sfx/quest.wav";
    case SFX_PET: return "assets/sfx/pet.wav";
    case SFX_HEAL: return "assets/sfx/heal.wav";
    case SFX_BOSS_INTRO: return "assets/sfx/boss_intro.wav";
    case SFX_TRAVEL: return "assets/sfx/travel.wav";
    case SFX_REWARD: return "assets/sfx/reward.wav";
    case SFX_GAME_OVER: return "assets/sfx/game_over.wav";
    default: return nullptr;
    }
}

static bool SoundLoaded(const Sound& sound) {
    return sound.frameCount != 0;
}

static void PlaySoundSafe(const Sound& sound) {
#ifdef _WIN32
    const char* rel = GetSfxPath(sound.frameCount);
    if (!rel) {
        return;
    }

    std::string path = FindAssetPath(rel);
    if (FileExistsPortable(path)) {
        PlaySoundA(path.c_str(), nullptr,
            SND_FILENAME_FALLBACK | SND_ASYNC_FALLBACK | SND_NODEFAULT_FALLBACK);
    }
#else
    (void)sound;
#endif
}

static void UnloadSoundSafe(Sound& sound) {
    sound = {};
}

static const char* WorldLabel(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "CROWNHEART KEEP";
    case WorldId::Frostveil: return "FROSTVEIL PASS";
    case WorldId::Sunscar: return "SUNSCAR DUNES";
    case WorldId::Mirethorn: return "MIRETHORN HOLLOW";
    }
    return "UNKNOWN REALM";
}

static unsigned char ClampByte(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

static Color ScaleColor(Color color, float factor) {
    return {
        ClampByte((int)std::round((float)color.r * factor)),
        ClampByte((int)std::round((float)color.g * factor)),
        ClampByte((int)std::round((float)color.b * factor)),
        color.a
    };
}

static Color MixColor(Color a, Color b, float t) {
    t = ClampFloat(t, 0.0f, 1.0f);
    return {
        ClampByte((int)std::round((float)a.r + ((float)b.r - (float)a.r) * t)),
        ClampByte((int)std::round((float)a.g + ((float)b.g - (float)a.g) * t)),
        ClampByte((int)std::round((float)a.b + ((float)b.b - (float)a.b) * t)),
        ClampByte((int)std::round((float)a.a + ((float)b.a - (float)a.a) * t))
    };
}

static void DrawSoftGlow(Vector2 pos, float radius, Color color, float alpha) {
    for (int i = 5; i >= 1; --i) {
        float scale = 1.0f + 0.42f * (float)i;
        DrawCircleV(pos, radius * scale, Fade(color, alpha * 0.05f * (float)i));
    }
}

static void DrawHorizontalFade(int x, int y, int width, int height, Color color, float alphaStart, float alphaEnd) {
    if (width <= 0 || height <= 0) {
        return;
    }

    for (int i = 0; i < width; ++i) {
        float t = (width > 1) ? ((float)i / (float)(width - 1)) : 0.0f;
        float alpha = LerpFloat(alphaStart, alphaEnd, t);
        DrawRectangle(x + i, y, 1, height, Fade(color, alpha));
    }
}

static int MoodHash(int x, int y, int seed) {
    unsigned int n = (unsigned int)(x * 374761393) ^ (unsigned int)(y * 668265263) ^ (unsigned int)(seed * 362437);
    n = (n ^ (n >> 13)) * 1274126177u;
    return (int)(n ^ (n >> 16));
}

static const char* WorldBlurb(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "Ruined courts, chapel roads, and a keep still breathing through grief.";
    case WorldId::Frostveil: return "Frozen shrines and silver passes where old vows linger in the cold.";
    case WorldId::Sunscar: return "Ash-blown roads and split stone where oaths were burned beneath red suns.";
    case WorldId::Mirethorn: return "Bog courts and thorn-choked causeways drowned beneath patient fog.";
    }
    return "";
}

static Color WorldGrassTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 95, 104, 96, 255 };
    case WorldId::Frostveil: return { 102, 114, 128, 255 };
    case WorldId::Sunscar: return { 123, 102, 78, 255 };
    case WorldId::Mirethorn: return { 78, 92, 71, 255 };
    }
    return { 95, 104, 96, 255 };
}

static Color WorldRoadTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 136, 126, 116, 255 };
    case WorldId::Frostveil: return { 150, 156, 176, 255 };
    case WorldId::Sunscar: return { 166, 136, 94, 255 };
    case WorldId::Mirethorn: return { 116, 106, 82, 255 };
    }
    return { 136, 126, 116, 255 };
}

static Color WorldAccentTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 118, 126, 169, 255 };
    case WorldId::Frostveil: return { 148, 176, 210, 255 };
    case WorldId::Sunscar: return { 192, 138, 84, 255 };
    case WorldId::Mirethorn: return { 110, 138, 98, 255 };
    }
    return { 118, 126, 169, 255 };
}

static Color WorldShadowTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 24, 28, 34, 255 };
    case WorldId::Frostveil: return { 18, 26, 38, 255 };
    case WorldId::Sunscar: return { 32, 20, 16, 255 };
    case WorldId::Mirethorn: return { 18, 24, 18, 255 };
    }
    return { 24, 28, 34, 255 };
}

static Color WorldFogTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 122, 130, 156, 255 };
    case WorldId::Frostveil: return { 166, 184, 210, 255 };
    case WorldId::Sunscar: return { 154, 112, 86, 255 };
    case WorldId::Mirethorn: return { 94, 122, 98, 255 };
    }
    return { 122, 130, 156, 255 };
}

static Color WorldGlowTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 188, 170, 122, 255 };
    case WorldId::Frostveil: return { 206, 230, 255, 255 };
    case WorldId::Sunscar: return { 224, 166, 92, 255 };
    case WorldId::Mirethorn: return { 156, 184, 124, 255 };
    }
    return { 188, 170, 122, 255 };
}

static Color WorldPropTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 228, 226, 232, 255 };
    case WorldId::Frostveil: return { 220, 234, 245, 255 };
    case WorldId::Sunscar: return { 236, 226, 206, 255 };
    case WorldId::Mirethorn: return { 218, 228, 214, 255 };
    }
    return WHITE;
}

static Color WorldSkyTopTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 18, 22, 32, 255 };
    case WorldId::Frostveil: return { 15, 24, 38, 255 };
    case WorldId::Sunscar: return { 28, 16, 20, 255 };
    case WorldId::Mirethorn: return { 14, 22, 20, 255 };
    }
    return { 18, 22, 32, 255 };
}

static Color WorldSkyBottomTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 54, 44, 48, 255 };
    case WorldId::Frostveil: return { 56, 64, 82, 255 };
    case WorldId::Sunscar: return { 88, 58, 38, 255 };
    case WorldId::Mirethorn: return { 46, 52, 34, 255 };
    }
    return { 54, 44, 48, 255 };
}

static Color WorldRoomLabelTint(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return { 203, 189, 164, 255 };
    case WorldId::Frostveil: return { 210, 224, 236, 255 };
    case WorldId::Sunscar: return { 226, 194, 142, 255 };
    case WorldId::Mirethorn: return { 188, 206, 160, 255 };
    }
    return { 203, 189, 164, 255 };
}

static float WorldFogStrength(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return 0.30f;
    case WorldId::Frostveil: return 0.36f;
    case WorldId::Sunscar: return 0.20f;
    case WorldId::Mirethorn: return 0.42f;
    }
    return 0.30f;
}

static float WorldAmbientMotion(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return 0.70f;
    case WorldId::Frostveil: return 0.60f;
    case WorldId::Sunscar: return 0.85f;
    case WorldId::Mirethorn: return 0.52f;
    }
    return 0.70f;
}

static int WorldIndex(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return 0;
    case WorldId::Frostveil: return 1;
    case WorldId::Sunscar: return 2;
    case WorldId::Mirethorn: return 3;
    }
    return 0;
}

static WorldId WeaponOriginWorld(int weaponIndex) {
    switch (weaponIndex) {
    case 29:
    case 33:
    case 34:
    case 38:
        return WorldId::Crownheart;
    case 30:
    case 35:
        return WorldId::Frostveil;
    case 31:
    case 36:
        return WorldId::Sunscar;
    case 32:
    case 37:
        return WorldId::Mirethorn;
    default:
        break;
    }

    if (weaponIndex >= 25) return WorldId::Mirethorn;
    if (weaponIndex >= 22) return WorldId::Sunscar;
    if (weaponIndex >= 19) return WorldId::Frostveil;
    return WorldId::Crownheart;
}

static bool WeaponIsRealmSignature(int weaponIndex) {
    return weaponIndex == 17 || weaponIndex == 21 || weaponIndex == 24 || weaponIndex == 27;
}

static int SignatureWeaponIndex(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return 17;
    case WorldId::Frostveil: return 21;
    case WorldId::Sunscar: return 24;
    case WorldId::Mirethorn: return 27;
    }
    return 17;
}

static const char* WeaponSourceLabel(int weaponIndex) {
    switch (weaponIndex) {
    case 29: return "OATHKEEP FORGE";
    case 30: return "GLASS SHRINE CACHE";
    case 31: return "ASHEN CARAVAN VAULT";
    case 32: return "BLACKBRIAR ARMORY";
    case 33: return "BROKEN CROWN RELIQUARY";
    case 34: return "VESPER FORGE";
    case 35: return "CHAPELGLASS VAULT";
    case 36: return "SUNSCAR OATHVAULT";
    case 37: return "THORNMIRE ARMORY";
    case 38: return "BROKEN THRONE RELIQUARY";
    default:
        break;
    }

    if (weaponIndex < 17) {
        return "KEEP FORGE";
    }

    switch (WeaponOriginWorld(weaponIndex)) {
    case WorldId::Crownheart: return "CROWNHEART FORGE";
    case WorldId::Frostveil: return "FROSTVEIL CACHE";
    case WorldId::Sunscar: return "SUNSCAR RELIQUARY";
    case WorldId::Mirethorn: return "MIRETHORN ARMORY";
    }

    return "KEEP FORGE";
}

static const char* WeaponIdentityDesc(int weaponIndex) {
    switch (weaponIndex) {
    case 29: return "OATH // bites deeper into elites and bosses.";
    case 30: return "VOW // tears harder through chilled prey.";
    case 31: return "EMBER // crits spread harsher fire.";
    case 32: return "BRIAR // poisoned thrusts steady your blood.";
    case 33: return "SORROW // gains edge when the knight is wounded.";
    case 34: return "VESPER // punishes healthier foes and opens crowds.";
    case 35: return "GLASS // duelist steel peaks while the knight stands clean.";
    case 36: return "OATH // burning elites suffer the axe's second sentence.";
    case 37: return "GRIEF // longer reach, crueler to weakened prey.";
    case 38: return "LAMENT // crownsteel turns desperation into judgment.";
    default: return "";
    }
}

static const char* WorldTileAtlasRelativePath(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "assets/world_tiles_crownheart.png";
    case WorldId::Frostveil: return "assets/world_tiles_frostveil.png";
    case WorldId::Sunscar: return "assets/world_tiles_sunscar.png";
    case WorldId::Mirethorn: return "assets/world_tiles_mirethorn.png";
    }
    return "assets/world_tiles.png";
}

static const char* WorldPropAtlasRelativePath(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "assets/props_atlas_crownheart.png";
    case WorldId::Frostveil: return "assets/props_atlas_frostveil.png";
    case WorldId::Sunscar: return "assets/props_atlas_sunscar.png";
    case WorldId::Mirethorn: return "assets/props_atlas_mirethorn.png";
    }
    return "assets/props_atlas.png";
}

static const char* WorldEnemyAtlasRelativePath(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "assets/enemy_atlas_crownheart.png";
    case WorldId::Frostveil: return "assets/enemy_atlas_frostveil.png";
    case WorldId::Sunscar: return "assets/enemy_atlas_sunscar.png";
    case WorldId::Mirethorn: return "assets/enemy_atlas_mirethorn.png";
    }
    return "assets/enemy_atlas_crownheart.png";
}

static const char* WeaponTraitLabel(WeaponTrait trait) {
    switch (trait) {
    case WeaponTrait::Balanced: return "Balanced";
    case WeaponTrait::Swift: return "Swift";
    case WeaponTrait::Heavy: return "Heavy";
    case WeaponTrait::Frost: return "Frostbite";
    case WeaponTrait::Chain: return "Storm Arc";
    case WeaponTrait::Sunfire: return "Sunfire";
    case WeaponTrait::Venom: return "Venom";
    case WeaponTrait::Guardian: return "Guardian";
    case WeaponTrait::Executioner: return "Executioner";
    case WeaponTrait::Royal: return "Royal";
    }
    return "Balanced";
}

static const char* WeaponTraitDesc(WeaponTrait trait) {
    switch (trait) {
    case WeaponTrait::Balanced: return "Steady steel with no drawback.";
    case WeaponTrait::Swift: return "Faster swings and sharp tempo.";
    case WeaponTrait::Heavy: return "Hits harder but winds up slower.";
    case WeaponTrait::Frost: return "Strikes chill and slow foes.";
    case WeaponTrait::Chain: return "Arc damage jumps to nearby targets.";
    case WeaponTrait::Sunfire: return "Leaves burning damage behind.";
    case WeaponTrait::Venom: return "Poisons foes over time.";
    case WeaponTrait::Guardian: return "Restores a little life on hit.";
    case WeaponTrait::Executioner: return "Punishes wounded enemies.";
    case WeaponTrait::Royal: return "Adds splash and higher crit pressure.";
    }
    return "Steady steel with no drawback.";
}

static const char* WorldBlurbLine1(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "Ruined courts and chapel roads.";
    case WorldId::Frostveil: return "Frozen shrines and silver passes.";
    case WorldId::Sunscar: return "Ash roads and broken caravan stone.";
    case WorldId::Mirethorn: return "Bog causeways and thorn-choked bridges.";
    }
    return "";
}

static const char* WorldBlurbLine2(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return "The keep endures through grief.";
    case WorldId::Frostveil: return "Old vows still haunt the cold.";
    case WorldId::Sunscar: return "Burned oaths linger in the dust.";
    case WorldId::Mirethorn: return "Fog and roots swallow every lane.";
    }
    return "";
}

static int MainQuestUnlockRequirement(WorldId world) {
    switch (world) {
    case WorldId::Crownheart: return 0;
    case WorldId::Frostveil: return 3;
    case WorldId::Sunscar: return 6;
    case WorldId::Mirethorn: return 9;
    }
    return 0;
}

static WorldId MainQuestWorld(int questIndex) {
    if (questIndex < 3) return WorldId::Crownheart;
    if (questIndex < 6) return WorldId::Frostveil;
    if (questIndex < 9) return WorldId::Sunscar;
    return WorldId::Mirethorn;
}

static const char* QuestObjectiveVerb(QuestObjective objective) {
    switch (objective) {
    case QuestObjective::SlayFoes: return "SLAY";
    case QuestObjective::ClearCourts: return "CLEAR";
    case QuestObjective::ClaimBlessings: return "CLAIM";
    case QuestObjective::DefeatElites: return "HUNT";
    case QuestObjective::DefeatBosses: return "DEFEAT";
    }
    return "DO";
}

static constexpr const char* kProfileSaveHeaderV6 = "CROWNHEART_PROFILE_V6";
static constexpr const char* kRunSaveHeaderV4 = "CROWNHEART_SAVE_V4";
static constexpr const char* kLegacyRunSaveHeaderV2 = "NEON_ABYSS_SAVE_V2";
static constexpr const char* kLegacyRunSaveHeaderV3 = "NEON_ABYSS_SAVE_V3";

Game::Game() {
    std::srand((unsigned int)std::time(nullptr));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenW, screenH, "CROWNHEART // KINGDOM SIEGE");
    SetWindowMinSize(960, 540);

    int monitor = GetCurrentMonitor();
    int monitorW = GetMonitorWidth(monitor);
    int monitorH = GetMonitorHeight(monitor);
    int targetW = std::max(1100, std::min(monitorW - 120, 1720));
    int targetH = std::max(640, std::min(monitorH - 140, 980));
    SetWindowSize(targetW, targetH);
    SetWindowPosition(std::max(0, (monitorW - targetW) / 2), std::max(0, (monitorH - targetH) / 2));

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();
    SetTargetFPS(60);

    BuildColorTheme();
    BuildDatabases();
    BuildQuestData();
    LoadProfile();
    BuildStars();
    BuildDungeon();
    BuildTileMap();
    LoadAssets();
    LoadAudio();

    camera = { 0 };
    camera.offset = { screenW * 0.5f, screenH * 0.5f };
    camera.target = player.pos;
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    ResetRun();
    gameState = GameState::Title;
}

Game::~Game() {
    UnloadAudio();
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
    bg = { 14, 16, 22, 255 };
    bg2 = { 42, 32, 38, 255 };
    grid = { 70, 62, 56, 255 };
    panel = { 18, 20, 28, 236 };
    panel2 = { 26, 24, 34, 244 };
    neonCyan = { 138, 168, 196, 255 };
    neonBlue = { 110, 126, 172, 255 };
    neonPink = { 152, 76, 86, 255 };
    neonGold = { 188, 156, 96, 255 };
    softRed = { 176, 82, 74, 255 };
    safeGreen = { 108, 148, 112, 255 };
    bossPurple = { 120, 98, 150, 255 };
}

void Game::BuildDatabases() {
    weaponDB = {
        {"Rusty Cudgel", 16, 64.0f, "Common", 0, LIGHTGRAY, 0, WeaponTrait::Heavy, 1.14f},
        {"Iron Mace", 21, 72.0f, "Common", 180, GRAY, 1, WeaponTrait::Heavy, 1.13f},
        {"Pilgrim's Hatchet", 19, 78.0f, "Common", 260, neonCyan, 2, WeaponTrait::Swift, 0.88f},
        {"Raider Flail", 24, 86.0f, "Common", 420, ORANGE, 3, WeaponTrait::Chain, 0.98f},
        {"Hearthblade", 29, 92.0f, "Rare", 620, SKYBLUE, 4, WeaponTrait::Guardian, 0.96f},
        {"Knight Saber", 34, 100.0f, "Rare", 860, BLUE, 5, WeaponTrait::Balanced, 0.94f},
        {"Sunsteel Blade", 40, 108.0f, "Rare", 1120, RED, 6, WeaponTrait::Sunfire, 0.96f},
        {"Storm Fang", 46, 116.0f, "Rare", 1450, LIME, 7, WeaponTrait::Chain, 0.92f},
        {"Raven Pike", 52, 126.0f, "Rare", 1780, neonCyan, 8, WeaponTrait::Swift, 0.90f},
        {"Grave Reaper", 60, 134.0f, "Legendary", 2150, PURPLE, 9, WeaponTrait::Executioner, 1.02f},
        {"Arc Halberd", 68, 142.0f, "Legendary", 2580, YELLOW, 10, WeaponTrait::Chain, 1.00f},
        {"Starforged Brand", 74, 150.0f, "Legendary", 3120, VIOLET, 11, WeaponTrait::Royal, 0.98f},
        {"King's Greatsword", 84, 158.0f, "Legendary", 3720, GOLD, 12, WeaponTrait::Heavy, 1.12f},
        {"Wyrmtooth Cleaver", 92, 166.0f, "Exotic", 4380, GREEN, 13, WeaponTrait::Executioner, 1.06f},
        {"Moonveil Edge", 98, 174.0f, "Exotic", 5120, bossPurple, 14, WeaponTrait::Frost, 0.94f},
        {"Celestial Cleaver", 108, 182.0f, "Exotic", 6080, WHITE, 15, WeaponTrait::Guardian, 1.00f},
        {"Crownfall", 118, 190.0f, "Exotic", 7240, neonPink, 16, WeaponTrait::Royal, 1.04f},
        {"Crownsent Pike", 42, 122.0f, "Realmforged", 980, neonBlue, 17, WeaponTrait::Guardian, 0.94f},
        {"Gatekeeper Hammer", 48, 130.0f, "Realmforged", 1320, { 100, 110, 128, 255 }, 18, WeaponTrait::Heavy, 1.10f},
        {"Hailhook", 56, 132.0f, "Realmforged", 1750, { 132, 182, 236, 255 }, 19, WeaponTrait::Frost, 0.94f},
        {"Winterglass Rapier", 54, 140.0f, "Legendary", 2280, { 202, 236, 255, 255 }, 20, WeaponTrait::Swift, 0.80f},
        {"Whiteout Halberd", 70, 152.0f, "Exotic", 2840, { 150, 208, 255, 255 }, 21, WeaponTrait::Frost, 0.98f},
        {"Dune Carver", 60, 138.0f, "Realmforged", 1920, { 194, 143, 80, 255 }, 22, WeaponTrait::Sunfire, 0.92f},
        {"Sirocco Saber", 68, 146.0f, "Legendary", 2460, { 226, 182, 86, 255 }, 23, WeaponTrait::Swift, 0.86f},
        {"Pharaoh's Hookblade", 80, 156.0f, "Exotic", 3250, { 235, 188, 73, 255 }, 24, WeaponTrait::Sunfire, 0.98f},
        {"Boghook", 58, 140.0f, "Realmforged", 2060, { 98, 122, 72, 255 }, 25, WeaponTrait::Venom, 0.96f},
        {"Witchreed Glaive", 72, 150.0f, "Legendary", 2720, { 118, 164, 108, 255 }, 26, WeaponTrait::Guardian, 0.98f},
        {"Hollowroot Scythe", 86, 164.0f, "Exotic", 3560, { 164, 208, 132, 255 }, 27, WeaponTrait::Venom, 1.02f},
        {"Marsh Lantern Spear", 76, 156.0f, "Legendary", 2980, { 160, 178, 118, 255 }, 28, WeaponTrait::Guardian, 0.94f},
        {"Oathkeeper Blade", 64, 148.0f, "Knightly", 2380, { 126, 170, 232, 255 }, 5, WeaponTrait::Balanced, 0.90f},
        {"Widowglass Thorn", 78, 154.0f, "Knightly", 3080, { 206, 232, 255, 255 }, 20, WeaponTrait::Frost, 0.92f},
        {"Ashsworn Falchion", 90, 162.0f, "Knightly", 3840, { 230, 154, 86, 255 }, 23, WeaponTrait::Sunfire, 0.95f},
        {"Blackrose Pike", 84, 170.0f, "Knightly", 4360, { 176, 118, 144, 255 }, 28, WeaponTrait::Venom, 0.90f},
        {"Sword Without Crown", 104, 178.0f, "Relic", 5180, { 222, 178, 92, 255 }, 16, WeaponTrait::Royal, 1.00f},
        {"Vesper Greatblade", 88, 164.0f, "Knightly", 3520, { 172, 164, 204, 255 }, 12, WeaponTrait::Heavy, 1.08f},
        {"Chapelglass Estoc", 74, 156.0f, "Knightly", 3360, { 198, 226, 245, 255 }, 20, WeaponTrait::Swift, 0.82f},
        {"Suncleft Oathaxe", 94, 168.0f, "Knightly", 4280, { 240, 176, 92, 255 }, 13, WeaponTrait::Sunfire, 0.98f},
        {"Griefthorn Partisan", 82, 172.0f, "Knightly", 4620, { 140, 174, 122, 255 }, 8, WeaponTrait::Venom, 0.90f},
        {"King's Lament", 110, 184.0f, "Relic", 5980, { 216, 186, 118, 255 }, 11, WeaponTrait::Royal, 1.02f}
    };

    petDB = {
        {"Ember Fox", "Spits cinders that burn hunted foes.", 24, 8, 1.00f, 32.0f, { 214, 118, 62, 255 }, 0},
        {"Frost Finch", "Pecks icy shards that slow enemies.", 40, 10, 1.10f, 34.0f, { 164, 214, 255, 255 }, 1},
        {"Dune Scarab", "Launches bright darts in quick bursts.", 62, 12, 0.86f, 30.0f, { 224, 186, 76, 255 }, 2},
        {"Mireling", "Spits venom globs that poison targets.", 88, 15, 1.18f, 36.0f, { 122, 166, 104, 255 }, 3},
        {"Lantern Moth", "Sends guiding light and mends you in battle.", 122, 11, 1.00f, 34.0f, { 244, 222, 138, 255 }, 4},
        {"Stone Pup", "Charges with blunt force and staggers packs.", 168, 18, 1.24f, 38.0f, { 166, 158, 172, 255 }, 5}
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

void Game::BuildQuestData() {
    mainQuestDB = {
        { "Crownheart Roadwatch", "Thin the raiders and beasts pressing the keep roads.", QuestObjective::SlayFoes, 10, 12 },
        { "Keep Court Sweep", "Clear the first battle courts around Crownheart.", QuestObjective::ClearCourts, 2, 16 },
        { "Redfang Hunt Order", "Bring down elite warband leaders near the keep.", QuestObjective::DefeatElites, 2, 20 },

        { "Frostveil Scout March", "Secure the icy approach and reopen the pass.", QuestObjective::ClearCourts, 2, 22 },
        { "Shrineglass Recovery", "Claim blessings from the pale shrines of Frostveil.", QuestObjective::ClaimBlessings, 2, 26 },
        { "White Hunt Writ", "Defeat elite hunters stalking the frozen roads.", QuestObjective::DefeatElites, 3, 30 },

        { "Sunscar Caravan Guard", "Cut down marauders scattered through the dunes.", QuestObjective::SlayFoes, 14, 32 },
        { "Amber Gate Break", "Clear the burning courts around the trade lanes.", QuestObjective::ClearCourts, 3, 38 },
        { "Ash Tyrant Bounty", "Defeat a boss that rules the Sunscar hunt.", QuestObjective::DefeatBosses, 1, 46 },

        { "Fen Lantern Patrol", "Push through the mire and reclaim the swamp courts.", QuestObjective::ClearCourts, 3, 42 },
        { "Rot Altar Recovery", "Claim blessings from drowned shrines in the fog.", QuestObjective::ClaimBlessings, 2, 48 },
        { "Thorn Crown Warrant", "Defeat a Mirethorn boss and end the charter line.", QuestObjective::DefeatBosses, 1, 60 }
    };

    sideQuestDB = {
        { "Roadside Cull", "Slay loose raiders around Crownheart.", QuestObjective::SlayFoes, 7, 6 },
        { "Gate Court Notice", "Clear one Crownheart court for the keep guard.", QuestObjective::ClearCourts, 1, 8 },
        { "Banner Blessing", "Bring one blessing back to the keep chapel.", QuestObjective::ClaimBlessings, 1, 8 },
        { "Redfang Bounty", "Hunt one elite prowling the green roads.", QuestObjective::DefeatElites, 1, 10 },

        { "Snowline Sweep", "Slay a pack roaming Frostveil lanes.", QuestObjective::SlayFoes, 9, 9 },
        { "Ice Chapel Watch", "Clear one Frostveil court for the scouts.", QuestObjective::ClearCourts, 1, 11 },
        { "Shrineglass Tithe", "Recover one blessing from the frozen chapels.", QuestObjective::ClaimBlessings, 1, 12 },
        { "White Hunter Mark", "Bring down one elite Frostveil hunter.", QuestObjective::DefeatElites, 1, 14 },

        { "Caravan Road Sweep", "Slay dune marauders near the trade stones.", QuestObjective::SlayFoes, 11, 12 },
        { "Brass Gate Contract", "Clear two Sunscar courts for the caravans.", QuestObjective::ClearCourts, 2, 15 },
        { "Sun Reliquary Claim", "Take one blessing from a Sunscar reliquary.", QuestObjective::ClaimBlessings, 1, 15 },
        { "Wyrm Watch", "Defeat one boss haunting the desert lanes.", QuestObjective::DefeatBosses, 1, 20 },

        { "Fenline Purge", "Slay rotkin along the flooded paths.", QuestObjective::SlayFoes, 12, 14 },
        { "Bog Lantern Route", "Clear two Mirethorn courts for the ferrymen.", QuestObjective::ClearCourts, 2, 18 },
        { "Drowned Reliquary", "Recover one blessing from the black water shrines.", QuestObjective::ClaimBlessings, 1, 18 },
        { "Thorn Trophy", "Bring down a Mirethorn boss for the keep board.", QuestObjective::DefeatBosses, 1, 24 }
    };

    if (sideQuestOfferIndices.size() != 3) {
        sideQuestOfferIndices.assign(3, -1);
        sideQuestProgress.assign(3, 0);
        sideQuestAccepted.assign(3, false);
        sideQuestReady.assign(3, false);
    }
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

    switch (currentWorld) {
    case WorldId::Crownheart:
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

        AddObstacle({ -96.0f, -44.0f, 192.0f, 168.0f }, { 114, 101, 85, 255 }, 5, 1.82f, {
            { -72.0f, 34.0f, 30.0f, 60.0f }, { -22.0f, 54.0f, 44.0f, 42.0f }, { 42.0f, 34.0f, 30.0f, 60.0f }
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
        break;

    case WorldId::Frostveil:
        dungeon.rooms = {
            { { -460.0f, -340.0f, 920.0f, 680.0f }, "WHITEGATE CAMP", true, false },
            { { 760.0f, -260.0f, 720.0f, 540.0f }, "GLACIER YARD", false, true },
            { { -1480.0f, -260.0f, 720.0f, 540.0f }, "WOLFPINE RISE", false, true },
            { { -340.0f, -1320.0f, 680.0f, 520.0f }, "CHAPEL APPROACH", false, true },
            { { -340.0f, 780.0f, 680.0f, 520.0f }, "ICEFLOW CROSSING", false, true },
            { { 740.0f, -1180.0f, 720.0f, 460.0f }, "HAILWATCH OVERLOOK", false, true },
            { { -1480.0f, 740.0f, 720.0f, 460.0f }, "WHITEBARROW", false, true },
            { { 180.0f, -820.0f, 300.0f, 220.0f }, "PILGRIM'S HEARTH", false, false },
            { { -520.0f, 500.0f, 300.0f, 220.0f }, "SNOWMELT CACHE", false, false }
        };
        dungeon.corridors = {
            { 400.0f, -100.0f, 400.0f, 200.0f },
            { -840.0f, -100.0f, 400.0f, 200.0f },
            { -100.0f, -840.0f, 200.0f, 500.0f },
            { -100.0f, 300.0f, 200.0f, 520.0f },
            { 1020.0f, -780.0f, 180.0f, 520.0f },
            { -1240.0f, 240.0f, 180.0f, 520.0f },
            { 110.0f, -720.0f, 110.0f, 120.0f },
            { -220.0f, 560.0f, 120.0f, 100.0f }
        };

        AddObstacle({ -84.0f, -26.0f, 168.0f, 148.0f }, { 143, 150, 168, 255 }, 5, 1.58f, {
            { -56.0f, 38.0f, 26.0f, 56.0f }, { -14.0f, 54.0f, 28.0f, 36.0f }, { 30.0f, 38.0f, 26.0f, 56.0f }
            });
        AddObstacle({ -236.0f, 42.0f, 74.0f, 116.0f }, { 162, 166, 182, 255 }, 4, 1.00f, { TowerCollider({ -236.0f, 42.0f, 74.0f, 116.0f }) });
        AddObstacle({ 162.0f, 42.0f, 74.0f, 116.0f }, { 162, 166, 182, 255 }, 4, 1.00f, { TowerCollider({ 162.0f, 42.0f, 74.0f, 116.0f }) });
        AddObstacle({ 930.0f, -118.0f, 94.0f, 126.0f }, { 136, 154, 164, 255 }, 0, 1.12f, { TreeTrunk({ 930.0f, -118.0f, 94.0f, 126.0f }), TreeRoots({ 930.0f, -118.0f, 94.0f, 126.0f }) });
        AddObstacle({ 1180.0f, -16.0f, 96.0f, 78.0f }, { 168, 174, 186, 255 }, 2, 0.94f, { RockCollider({ 1180.0f, -16.0f, 96.0f, 78.0f }) });
        AddObstacle({ -1260.0f, -126.0f, 94.0f, 126.0f }, { 136, 154, 164, 255 }, 0, 1.12f, { TreeTrunk({ -1260.0f, -126.0f, 94.0f, 126.0f }), TreeRoots({ -1260.0f, -126.0f, 94.0f, 126.0f }) });
        AddObstacle({ -1030.0f, 24.0f, 96.0f, 78.0f }, { 168, 174, 186, 255 }, 2, 0.94f, { RockCollider({ -1030.0f, 24.0f, 96.0f, 78.0f }) });
        AddObstacle({ -150.0f, -1170.0f, 90.0f, 122.0f }, { 136, 154, 164, 255 }, 0, 1.08f, { TreeTrunk({ -150.0f, -1170.0f, 90.0f, 122.0f }), TreeRoots({ -150.0f, -1170.0f, 90.0f, 122.0f }) });
        AddObstacle({ 70.0f, -1010.0f, 98.0f, 80.0f }, { 168, 174, 186, 255 }, 2, 0.94f, { RockCollider({ 70.0f, -1010.0f, 98.0f, 80.0f }) });
        AddObstacle({ -156.0f, 918.0f, 90.0f, 122.0f }, { 136, 154, 164, 255 }, 0, 1.08f, { TreeTrunk({ -156.0f, 918.0f, 90.0f, 122.0f }), TreeRoots({ -156.0f, 918.0f, 90.0f, 122.0f }) });
        AddObstacle({ 60.0f, 1050.0f, 98.0f, 80.0f }, { 168, 174, 186, 255 }, 2, 0.94f, { RockCollider({ 60.0f, 1050.0f, 98.0f, 80.0f }) });
        AddObstacle({ 1020.0f, -1040.0f, 108.0f, 104.0f }, { 170, 160, 150, 255 }, 3, 0.96f, { HutCollider({ 1020.0f, -1040.0f, 108.0f, 104.0f }) });
        AddObstacle({ -1280.0f, 890.0f, 108.0f, 104.0f }, { 170, 160, 150, 255 }, 3, 0.96f, { HutCollider({ -1280.0f, 890.0f, 108.0f, 104.0f }) });
        AddObstacle({ 298.0f, -760.0f, 42.0f, 62.0f }, { 186, 176, 142, 255 }, 9, 0.90f, { SignCollider({ 298.0f, -760.0f, 42.0f, 62.0f }) });
        AddObstacle({ -416.0f, 560.0f, 42.0f, 62.0f }, { 186, 176, 142, 255 }, 9, 0.90f, { SignCollider({ -416.0f, 560.0f, 42.0f, 62.0f }) });
        break;

    case WorldId::Sunscar:
        dungeon.rooms = {
            { { -460.0f, -340.0f, 920.0f, 680.0f }, "SAFFRON OASIS", true, false },
            { { 760.0f, -260.0f, 740.0f, 560.0f }, "CARAVAN GATE", false, true },
            { { -1500.0f, -260.0f, 740.0f, 560.0f }, "DUNE GRAVE", false, true },
            { { -340.0f, -1320.0f, 700.0f, 520.0f }, "SUNSTEP TERRACE", false, true },
            { { -340.0f, 780.0f, 700.0f, 520.0f }, "CINDER BASIN", false, true },
            { { 760.0f, 760.0f, 700.0f, 460.0f }, "AMBER BAZAAR", false, true },
            { { -1480.0f, -1180.0f, 700.0f, 460.0f }, "SCORPION HOLLOW", false, true },
            { { 200.0f, 500.0f, 280.0f, 220.0f }, "WIND SHRINE", false, false },
            { { -520.0f, -800.0f, 280.0f, 220.0f }, "BROKEN CISTERN", false, false }
        };
        dungeon.corridors = {
            { 400.0f, -100.0f, 400.0f, 200.0f },
            { -860.0f, -100.0f, 400.0f, 200.0f },
            { -100.0f, -840.0f, 200.0f, 500.0f },
            { -100.0f, 300.0f, 200.0f, 520.0f },
            { 1020.0f, 240.0f, 180.0f, 520.0f },
            { -1240.0f, -780.0f, 180.0f, 520.0f },
            { 160.0f, 500.0f, 80.0f, 120.0f },
            { -300.0f, -720.0f, 80.0f, 120.0f }
        };

        AddObstacle({ -80.0f, -18.0f, 160.0f, 140.0f }, { 146, 126, 82, 255 }, 3, 1.10f, { HutCollider({ -80.0f, -18.0f, 160.0f, 140.0f }) });
        AddObstacle({ -250.0f, 54.0f, 42.0f, 62.0f }, { 140, 112, 70, 255 }, 9, 0.92f, { SignCollider({ -250.0f, 54.0f, 42.0f, 62.0f }) });
        AddObstacle({ 208.0f, 54.0f, 42.0f, 62.0f }, { 140, 112, 70, 255 }, 9, 0.92f, { SignCollider({ 208.0f, 54.0f, 42.0f, 62.0f }) });
        AddObstacle({ 930.0f, -78.0f, 112.0f, 100.0f }, { 158, 134, 94, 255 }, 3, 0.98f, { HutCollider({ 930.0f, -78.0f, 112.0f, 100.0f }) });
        AddObstacle({ 1182.0f, 38.0f, 96.0f, 78.0f }, { 168, 152, 122, 255 }, 2, 0.96f, { RockCollider({ 1182.0f, 38.0f, 96.0f, 78.0f }) });
        AddObstacle({ -1270.0f, -64.0f, 112.0f, 100.0f }, { 158, 134, 94, 255 }, 3, 0.98f, { HutCollider({ -1270.0f, -64.0f, 112.0f, 100.0f }) });
        AddObstacle({ -1024.0f, 52.0f, 96.0f, 78.0f }, { 168, 152, 122, 255 }, 2, 0.96f, { RockCollider({ -1024.0f, 52.0f, 96.0f, 78.0f }) });
        AddObstacle({ -64.0f, -1180.0f, 96.0f, 78.0f }, { 168, 152, 122, 255 }, 2, 0.96f, { RockCollider({ -64.0f, -1180.0f, 96.0f, 78.0f }) });
        AddObstacle({ -8.0f, 930.0f, 96.0f, 78.0f }, { 168, 152, 122, 255 }, 2, 0.96f, { RockCollider({ -8.0f, 930.0f, 96.0f, 78.0f }) });
        AddObstacle({ 1050.0f, 884.0f, 110.0f, 104.0f }, { 158, 134, 94, 255 }, 3, 0.98f, { HutCollider({ 1050.0f, 884.0f, 110.0f, 104.0f }) });
        AddObstacle({ -1240.0f, -1040.0f, 90.0f, 124.0f }, { 170, 148, 118, 255 }, 0, 1.10f, { TreeTrunk({ -1240.0f, -1040.0f, 90.0f, 124.0f }), TreeRoots({ -1240.0f, -1040.0f, 90.0f, 124.0f }) });
        AddObstacle({ 298.0f, 562.0f, 74.0f, 116.0f }, { 152, 120, 82, 255 }, 4, 0.98f, { TowerCollider({ 298.0f, 562.0f, 74.0f, 116.0f }) });
        AddObstacle({ -420.0f, -744.0f, 74.0f, 116.0f }, { 152, 120, 82, 255 }, 4, 0.98f, { TowerCollider({ -420.0f, -744.0f, 74.0f, 116.0f }) });
        break;

    case WorldId::Mirethorn:
        dungeon.rooms = {
            { { -460.0f, -340.0f, 920.0f, 680.0f }, "LANTERN FEN", true, false },
            { { 760.0f, -260.0f, 720.0f, 560.0f }, "REEDWATCH", false, true },
            { { -1480.0f, -260.0f, 720.0f, 560.0f }, "ROTROOT STAND", false, true },
            { { -340.0f, -1320.0f, 700.0f, 520.0f }, "WITCHLIGHT CAUSEWAY", false, true },
            { { -340.0f, 780.0f, 700.0f, 520.0f }, "MUDGRAVE BEND", false, true },
            { { 760.0f, -1180.0f, 700.0f, 460.0f }, "CANKER BRIDGE", false, true },
            { { -1480.0f, 760.0f, 700.0f, 460.0f }, "HOLLOWMERE", false, true },
            { { 180.0f, 500.0f, 300.0f, 220.0f }, "CROAKING NOOK", false, false },
            { { -520.0f, -820.0f, 300.0f, 220.0f }, "BOGKEEPER'S SHED", false, false }
        };
        dungeon.corridors = {
            { 400.0f, -100.0f, 400.0f, 200.0f },
            { -840.0f, -100.0f, 400.0f, 200.0f },
            { -100.0f, -840.0f, 200.0f, 500.0f },
            { -100.0f, 300.0f, 200.0f, 520.0f },
            { 1020.0f, -780.0f, 180.0f, 520.0f },
            { -1240.0f, 240.0f, 180.0f, 540.0f },
            { 130.0f, 520.0f, 110.0f, 120.0f },
            { -300.0f, -740.0f, 100.0f, 120.0f }
        };

        AddObstacle({ -80.0f, -16.0f, 158.0f, 138.0f }, { 110, 120, 84, 255 }, 3, 1.05f, { HutCollider({ -80.0f, -16.0f, 158.0f, 138.0f }) });
        AddObstacle({ -250.0f, 56.0f, 42.0f, 62.0f }, { 108, 96, 64, 255 }, 9, 0.92f, { SignCollider({ -250.0f, 56.0f, 42.0f, 62.0f }) });
        AddObstacle({ 208.0f, 56.0f, 42.0f, 62.0f }, { 108, 96, 64, 255 }, 9, 0.92f, { SignCollider({ 208.0f, 56.0f, 42.0f, 62.0f }) });
        AddObstacle({ 942.0f, -132.0f, 92.0f, 126.0f }, { 74, 112, 72, 255 }, 0, 1.16f, { TreeTrunk({ 942.0f, -132.0f, 92.0f, 126.0f }), TreeRoots({ 942.0f, -132.0f, 92.0f, 126.0f }) });
        AddObstacle({ 1178.0f, 12.0f, 96.0f, 78.0f }, { 116, 124, 102, 255 }, 2, 0.92f, { RockCollider({ 1178.0f, 12.0f, 96.0f, 78.0f }) });
        AddObstacle({ -1260.0f, -132.0f, 92.0f, 126.0f }, { 74, 112, 72, 255 }, 0, 1.16f, { TreeTrunk({ -1260.0f, -132.0f, 92.0f, 126.0f }), TreeRoots({ -1260.0f, -132.0f, 92.0f, 126.0f }) });
        AddObstacle({ -1028.0f, 18.0f, 96.0f, 78.0f }, { 116, 124, 102, 255 }, 2, 0.92f, { RockCollider({ -1028.0f, 18.0f, 96.0f, 78.0f }) });
        AddObstacle({ -134.0f, -1160.0f, 92.0f, 126.0f }, { 74, 112, 72, 255 }, 0, 1.14f, { TreeTrunk({ -134.0f, -1160.0f, 92.0f, 126.0f }), TreeRoots({ -134.0f, -1160.0f, 92.0f, 126.0f }) });
        AddObstacle({ 52.0f, -1034.0f, 110.0f, 104.0f }, { 110, 120, 84, 255 }, 3, 0.94f, { HutCollider({ 52.0f, -1034.0f, 110.0f, 104.0f }) });
        AddObstacle({ -144.0f, 932.0f, 92.0f, 126.0f }, { 74, 112, 72, 255 }, 0, 1.14f, { TreeTrunk({ -144.0f, 932.0f, 92.0f, 126.0f }), TreeRoots({ -144.0f, 932.0f, 92.0f, 126.0f }) });
        AddObstacle({ 54.0f, 1048.0f, 110.0f, 104.0f }, { 110, 120, 84, 255 }, 3, 0.94f, { HutCollider({ 54.0f, 1048.0f, 110.0f, 104.0f }) });
        AddObstacle({ 1030.0f, -1036.0f, 74.0f, 116.0f }, { 122, 112, 92, 255 }, 4, 0.98f, { TowerCollider({ 1030.0f, -1036.0f, 74.0f, 116.0f }) });
        AddObstacle({ -1260.0f, 886.0f, 74.0f, 116.0f }, { 122, 112, 92, 255 }, 4, 0.98f, { TowerCollider({ -1260.0f, 886.0f, 74.0f, 116.0f }) });
        AddObstacle({ 300.0f, 556.0f, 42.0f, 62.0f }, { 108, 96, 64, 255 }, 9, 0.90f, { SignCollider({ 300.0f, 556.0f, 42.0f, 62.0f }) });
        AddObstacle({ -420.0f, -756.0f, 42.0f, 62.0f }, { 108, 96, 64, 255 }, 9, 0.90f, { SignCollider({ -420.0f, -756.0f, 42.0f, 62.0f }) });
        break;
    }

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

    switch (currentWorld) {
    case WorldId::Crownheart:
        worldProps.push_back({ { 0.0f, -520.0f }, 9, 0.90f });
        worldProps.push_back({ { 0.0f, 520.0f }, 9, 0.90f });
        worldProps.push_back({ { 650.0f, 0.0f }, 9, 0.90f });
        worldProps.push_back({ { -650.0f, 0.0f }, 9, 0.90f });
        worldProps.push_back({ { 126.0f, -636.0f }, 9, 0.82f });
        worldProps.push_back({ { -148.0f, 592.0f }, 9, 0.82f });
        worldProps.push_back({ { 1544.0f, -930.0f }, 9, 0.82f });
        worldProps.push_back({ { -1618.0f, 1008.0f }, 9, 0.82f });
        break;
    case WorldId::Frostveil:
        worldProps.push_back({ { 0.0f, -508.0f }, 4, 0.88f });
        worldProps.push_back({ { 0.0f, 510.0f }, 4, 0.88f });
        worldProps.push_back({ { 642.0f, 0.0f }, 9, 0.84f });
        worldProps.push_back({ { -642.0f, 0.0f }, 9, 0.84f });
        worldProps.push_back({ { 112.0f, -636.0f }, 2, 0.88f });
        worldProps.push_back({ { -146.0f, 600.0f }, 2, 0.88f });
        break;
    case WorldId::Sunscar:
        worldProps.push_back({ { 0.0f, -522.0f }, 3, 0.92f });
        worldProps.push_back({ { 0.0f, 520.0f }, 3, 0.92f });
        worldProps.push_back({ { 660.0f, 0.0f }, 9, 0.84f });
        worldProps.push_back({ { -660.0f, 0.0f }, 9, 0.84f });
        worldProps.push_back({ { 182.0f, 612.0f }, 4, 0.86f });
        worldProps.push_back({ { -360.0f, -710.0f }, 2, 0.88f });
        break;
    case WorldId::Mirethorn:
        worldProps.push_back({ { 0.0f, -520.0f }, 9, 0.88f });
        worldProps.push_back({ { 0.0f, 520.0f }, 9, 0.88f });
        worldProps.push_back({ { 652.0f, 0.0f }, 0, 1.02f });
        worldProps.push_back({ { -652.0f, 0.0f }, 0, 1.02f });
        worldProps.push_back({ { 180.0f, 602.0f }, 1, 0.94f });
        worldProps.push_back({ { -360.0f, -730.0f }, 1, 0.94f });
        break;
    }

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
    if (tileAtlas.id != 0) {
        UnloadTexture(tileAtlas);
        tileAtlas = {};
    }
    if (propAtlas.id != 0) {
        UnloadTexture(propAtlas);
        propAtlas = {};
    }
    if (enemyAtlas.id != 0) {
        UnloadTexture(enemyAtlas);
        enemyAtlas = {};
    }

    std::string tilePath = FindAssetPath(WorldTileAtlasRelativePath(currentWorld));
    std::string propPath = FindAssetPath(WorldPropAtlasRelativePath(currentWorld));
    std::string enemyPath = FindAssetPath(WorldEnemyAtlasRelativePath(currentWorld));

    if (!FileExistsPortable(tilePath)) {
        tilePath = FindAssetPath("assets/world_tiles.png");
    }
    if (!FileExistsPortable(propPath)) {
        propPath = FindAssetPath("assets/props_atlas.png");
    }
    if (!FileExistsPortable(enemyPath)) {
        enemyPath = FindAssetPath("assets/enemy_atlas_crownheart.png");
    }

    tileAtlas = LoadTexture(tilePath.c_str());
    propAtlas = LoadTexture(propPath.c_str());
    enemyAtlas = LoadTexture(enemyPath.c_str());

    if (actorAtlas.id == 0) {
        actorAtlas = LoadTexture(FindAssetPath("assets/actors_atlas.png").c_str());
    }
    if (weaponAtlas.id == 0) {
        weaponAtlas = LoadTexture(FindAssetPath("assets/weapons_atlas.png").c_str());
    }
    if (petAtlas.id == 0) {
        petAtlas = LoadTexture(FindAssetPath("assets/pets_atlas.png").c_str());
    }
}

void Game::LoadAudio() {
    if (audioReady) {
        return;
    }

    loadedSoundCount = 0;
    audioDebugStatus = "WINMM INIT";
    audioReady = true;

    auto registerSfx = [&](Sound& sound, unsigned int id, const char* relativePath) {
        std::string path = FindAssetPath(relativePath);
        if (!FileExistsPortable(path)) {
            sound = {};
            return;
        }

        sound = {};
        sound.frameCount = id;
        loadedSoundCount++;
        };

    registerSfx(sfxUiMove, SFX_UI_MOVE, "assets/sfx/ui_move.wav");
    registerSfx(sfxUiAccept, SFX_UI_ACCEPT, "assets/sfx/ui_accept.wav");
    registerSfx(sfxUiDeny, SFX_UI_DENY, "assets/sfx/ui_deny.wav");
    registerSfx(sfxSwordLight, SFX_SWORD_LIGHT, "assets/sfx/sword_light.wav");
    registerSfx(sfxSwordHeavy, SFX_SWORD_HEAVY, "assets/sfx/sword_heavy.wav");
    registerSfx(sfxHit, SFX_HIT, "assets/sfx/hit.wav");
    registerSfx(sfxCrit, SFX_CRIT, "assets/sfx/crit.wav");
    registerSfx(sfxDash, SFX_DASH, "assets/sfx/dash.wav");
    registerSfx(sfxBurst, SFX_BURST, "assets/sfx/burst.wav");
    registerSfx(sfxBanner, SFX_BANNER, "assets/sfx/banner.wav");
    registerSfx(sfxQuest, SFX_QUEST, "assets/sfx/quest.wav");
    registerSfx(sfxPet, SFX_PET, "assets/sfx/pet.wav");
    registerSfx(sfxHeal, SFX_HEAL, "assets/sfx/heal.wav");
    registerSfx(sfxBossIntro, SFX_BOSS_INTRO, "assets/sfx/boss_intro.wav");
    registerSfx(sfxTravel, SFX_TRAVEL, "assets/sfx/travel.wav");
    registerSfx(sfxReward, SFX_REWARD, "assets/sfx/reward.wav");
    registerSfx(sfxGameOver, SFX_GAME_OVER, "assets/sfx/game_over.wav");

    audioDebugStatus = loadedSoundCount > 0
        ? ("WINMM READY " + std::to_string(loadedSoundCount) + "/17")
        : "NO SFX LOADED";
}

void Game::UnloadAudio() {
    UnloadSoundSafe(sfxUiMove);
    UnloadSoundSafe(sfxUiAccept);
    UnloadSoundSafe(sfxUiDeny);
    UnloadSoundSafe(sfxSwordLight);
    UnloadSoundSafe(sfxSwordHeavy);
    UnloadSoundSafe(sfxHit);
    UnloadSoundSafe(sfxCrit);
    UnloadSoundSafe(sfxDash);
    UnloadSoundSafe(sfxBurst);
    UnloadSoundSafe(sfxBanner);
    UnloadSoundSafe(sfxQuest);
    UnloadSoundSafe(sfxPet);
    UnloadSoundSafe(sfxHeal);
    UnloadSoundSafe(sfxBossIntro);
    UnloadSoundSafe(sfxTravel);
    UnloadSoundSafe(sfxReward);
    UnloadSoundSafe(sfxGameOver);

#ifdef _WIN32
    PlaySoundA(nullptr, nullptr, 0);
#endif

    audioReady = false;
    loadedSoundCount = 0;
    audioDebugStatus = "AUDIO OFF";
}

void Game::UnloadAssets() {
    if (tileAtlas.id != 0) UnloadTexture(tileAtlas);
    if (propAtlas.id != 0) UnloadTexture(propAtlas);
    if (actorAtlas.id != 0) UnloadTexture(actorAtlas);
    if (enemyAtlas.id != 0) UnloadTexture(enemyAtlas);
    if (weaponAtlas.id != 0) UnloadTexture(weaponAtlas);
    if (petAtlas.id != 0) UnloadTexture(petAtlas);
    tileAtlas = {};
    propAtlas = {};
    actorAtlas = {};
    enemyAtlas = {};
    weaponAtlas = {};
    petAtlas = {};
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

Rectangle Game::EnemySourceRect(int index) const {
    const int cell = 64;
    const int cols = 4;
    return { (float)((index % cols) * cell), (float)((index / cols) * cell), (float)cell, (float)cell };
}

Rectangle Game::WeaponSourceRect(int index) const {
    const int cell = 64;
    const int cols = 6;
    return { (float)((index % cols) * cell), (float)((index / cols) * cell), (float)cell, (float)cell };
}

Rectangle Game::PetSourceRect(int index) const {
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
    float value = 0.40f - 0.02f * (float)CountRelic(RelicType::OverclockCore);
    if (value < 0.22f) {
        value = 0.22f;
    }
    return value;
}

float Game::GetWeaponAttackCooldown(const Weapon& weapon) const {
    float value = GetAttackCooldown() * weapon.cooldownScale;
    if (value < 0.18f) {
        value = 0.18f;
    }
    return value;
}

int Game::GetHpUpgradeCost() const {
    return 180 + player.hpUpgradeLevel * 180 + player.hpUpgradeLevel * player.hpUpgradeLevel * 30;
}

float Game::GetDashCooldown() const {
    float value = 3.0f - 0.12f * (float)CountRelic(RelicType::PhaseBoots);
    if (value < 1.8f) {
        value = 1.8f;
    }
    return value;
}

float Game::GetCritChance() const {
    return 0.06f + 0.05f * (float)CountRelic(RelicType::RazorPrism);
}

float Game::GetCritMultiplier() const {
    return 1.50f + 0.14f * (float)CountRelic(RelicType::RazorPrism);
}

int Game::GetFlatDamageBonus() const {
    return 5 * CountRelic(RelicType::RazorPrism);
}

int Game::GetWeaponDamageAgainst(const Weapon& weapon, const ActiveMonster& monster) const {
    int damage = weapon.damage + GetFlatDamageBonus() + (std::rand() % 5);

    switch (weapon.trait) {
    case WeaponTrait::Heavy:
        damage += 4;
        break;
    case WeaponTrait::Guardian:
        damage += 1;
        break;
    case WeaponTrait::Royal:
        damage += 4;
        break;
    case WeaponTrait::Executioner:
        if (monster.hp <= monster.maxHp / 3) {
            damage = (int)std::round((float)damage * 1.18f);
        }
        break;
    default:
        break;
    }

    if (weapon.name == "Oathkeeper Blade") {
        if (monster.isElite || monster.isBoss) {
            damage += 10;
        }
        if (player.hp * 2 <= player.maxHp) {
            damage += 6;
        }
    }
    else if (weapon.name == "Widowglass Thorn") {
        if (monster.slowTimer > 0.0f) {
            damage += 9;
        }
    }
    else if (weapon.name == "Ashsworn Falchion") {
        if (monster.burnTimer > 0.0f) {
            damage += 7;
        }
    }
    else if (weapon.name == "Blackrose Pike") {
        if (monster.poisonTimer > 0.0f) {
            damage += 8;
        }
        if (monster.isElite) {
            damage += 4;
        }
    }
    else if (weapon.name == "Sword Without Crown") {
        if (monster.isBoss) {
            damage += 14;
        }
        else if (monster.isElite) {
            damage += 8;
        }
        if (player.hp * 2 <= player.maxHp) {
            damage += 5;
        }
    }
    else if (weapon.name == "Vesper Greatblade") {
        if (monster.hp >= (monster.maxHp * 7) / 10) {
            damage += 9;
        }
        if (monster.isElite || monster.isBoss) {
            damage += 4;
        }
    }
    else if (weapon.name == "Chapelglass Estoc") {
        if (player.hp >= player.maxHp) {
            damage += 8;
        }
    }
    else if (weapon.name == "Suncleft Oathaxe") {
        if (monster.burnTimer > 0.0f) {
            damage += 10;
        }
        if ((monster.isElite || monster.isBoss) && monster.burnTimer > 0.0f) {
            damage += 6;
        }
    }
    else if (weapon.name == "Griefthorn Partisan") {
        if (monster.hp <= monster.maxHp / 2) {
            damage += 9;
        }
        if (monster.poisonTimer > 0.0f) {
            damage += 6;
        }
    }
    else if (weapon.name == "King's Lament") {
        if (monster.isBoss) {
            damage += 12;
        }
        else if (monster.isElite) {
            damage += 7;
        }
        if (player.hp * 2 <= player.maxHp) {
            damage += 10;
        }
    }

    if (damage < 1) {
        damage = 1;
    }
    return damage;
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

int Game::GetPetBondLevel(int petIndex) const {
    if (petIndex < 0 || petIndex >= (int)persistentPetBondXp.size()) {
        return 0;
    }

    int xp = persistentPetBondXp[petIndex];
    if (xp >= 75) return 4;
    if (xp >= 45) return 3;
    if (xp >= 22) return 2;
    if (xp >= 8) return 1;
    return 0;
}

int Game::GetPetTrainingEuroCost() const {
    return 40 + persistentPetTrainingLevel * 24 + persistentPetTrainingLevel * persistentPetTrainingLevel * 8;
}

int Game::GetPetDamage(int petIndex) const {
    if (petIndex < 0 || petIndex >= (int)petDB.size()) {
        return 0;
    }

    return petDB[petIndex].damage + GetPetBondLevel(petIndex) * 3 + persistentPetTrainingLevel * 2 + player.wave / 2;
}

float Game::GetPetAttackCooldown(int petIndex) const {
    if (petIndex < 0 || petIndex >= (int)petDB.size()) {
        return 1.0f;
    }

    float value = petDB[petIndex].attackCooldown - persistentPetTrainingLevel * 0.02f;
    if (value < 0.68f) {
        value = 0.68f;
    }
    return value;
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

    AdvanceQuestObjective(QuestObjective::ClaimBlessings, 1);
}

void Game::AdvanceQuestObjective(QuestObjective objective, int amount) {
    bool changed = false;

    if (mainQuestIndex >= 0 && mainQuestIndex < (int)mainQuestDB.size() && !mainQuestReady) {
        const QuestDefinition& mainQuest = mainQuestDB[mainQuestIndex];
        if (mainQuest.objective == objective) {
            int oldProgress = mainQuestProgress;
            mainQuestProgress = std::min(mainQuest.target, mainQuestProgress + amount);
            if (mainQuestProgress != oldProgress) {
                changed = true;
                if (mainQuestProgress >= mainQuest.target) {
                    mainQuestReady = true;
                    announcement = mainQuest.title + " // READY TO TURN IN";
                    announcementTimer = 2.2f;
                }
            }
        }
    }

    for (size_t i = 0; i < sideQuestOfferIndices.size(); ++i) {
        int defIndex = sideQuestOfferIndices[i];
        if (defIndex < 0 || defIndex >= (int)sideQuestDB.size() || !sideQuestAccepted[i] || sideQuestReady[i]) {
            continue;
        }

        const QuestDefinition& quest = sideQuestDB[defIndex];
        if (quest.objective != objective) {
            continue;
        }

        int oldProgress = sideQuestProgress[i];
        sideQuestProgress[i] = std::min(quest.target, sideQuestProgress[i] + amount);
        if (sideQuestProgress[i] != oldProgress) {
            changed = true;
            if (sideQuestProgress[i] >= quest.target) {
                sideQuestReady[i] = true;
                announcement = quest.title + " // BOUNTY READY";
                announcementTimer = 1.8f;
            }
        }
    }

    if (changed) {
        SaveProfile();
    }
}

void Game::RefreshSideQuestOffer(int slot) {
    if (slot < 0 || slot >= (int)sideQuestOfferIndices.size() || sideQuestDB.empty()) {
        return;
    }

    std::vector<int> pool;
    switch (currentWorld) {
    case WorldId::Crownheart: pool = { 0, 1, 2, 3 }; break;
    case WorldId::Frostveil: pool = { 4, 5, 6, 7 }; break;
    case WorldId::Sunscar: pool = { 8, 9, 10, 11 }; break;
    case WorldId::Mirethorn: pool = { 12, 13, 14, 15 }; break;
    }

    if (pool.empty()) {
        for (int i = 0; i < (int)sideQuestDB.size(); ++i) {
            pool.push_back(i);
        }
    }

    int candidate = pool[std::rand() % (int)pool.size()];
    for (int attempt = 0; attempt < 32; ++attempt) {
        int test = pool[std::rand() % (int)pool.size()];
        bool alreadyUsed = false;
        for (int i = 0; i < (int)sideQuestOfferIndices.size(); ++i) {
            if (i != slot && sideQuestOfferIndices[i] == test) {
                alreadyUsed = true;
                break;
            }
        }
        if (!alreadyUsed) {
            candidate = test;
            break;
        }
    }

    sideQuestOfferIndices[slot] = candidate;
    sideQuestProgress[slot] = 0;
    sideQuestAccepted[slot] = false;
    sideQuestReady[slot] = false;
}

bool Game::IsWorldUnlocked(WorldId world) const {
    if (world == WorldId::Crownheart) {
        return true;
    }
    return mainQuestIndex >= MainQuestUnlockRequirement(world);
}

void Game::TravelToWorld(WorldId world) {
    if (!IsWorldUnlocked(world)) {
        announcement = std::string(WorldLabel(world)) + " // STILL SEALED";
        announcementTimer = 2.0f;
        return;
    }

    currentWorld = world;
    BuildDungeon();
    BuildTileMap();
    LoadAssets();

    player.pos = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.72f };
    player.aimDir = { 0.0f, -1.0f };
    shop.isOpen = false;
    questBoardOpen = false;
    realmMapOpen = false;
    rewardChestActive = false;
    rewardSelectionOpen = false;
    waveTargetRoomIndex = -1;
    lockedRoomIndex = -1;
    safeZoneHealBuffer = 0.0f;

    monsters.clear();
    particles.clear();
    orbs.clear();
    healthPickups.clear();
    floatingTexts.clear();
    turrets.clear();
    shockwaves.clear();
    beams.clear();

    for (int i = 0; i < (int)sideQuestOfferIndices.size(); ++i) {
        if (!sideQuestAccepted[i]) {
            RefreshSideQuestOffer(i);
        }
    }

    SyncPetState(true);

    announcement = std::string(WorldLabel(world)) + " // ROADS OPEN";
    announcementTimer = 2.4f;
    PlaySoundSafe(sfxTravel);
    SpawnWave(player.wave);
    int worldSlot = WorldIndex(world);
    if (worldSlot >= 0 && worldSlot < 4 && !realmIntroSeen[worldSlot]) {
        realmIntroSeen[worldSlot] = true;
        StartRealmIntroCutscene(world);
    }
    SaveRun();
    SaveProfile();
}

void Game::GrantRealmSignatureIfNeeded(WorldId world) {
    if (weaponDB.empty()) {
        return;
    }

    if (realmSignatureClaimed.size() < 4) {
        realmSignatureClaimed.assign(4, false);
    }

    int worldSlot = WorldIndex(world);
    if (worldSlot < 0 || worldSlot >= (int)realmSignatureClaimed.size() || realmSignatureClaimed[worldSlot]) {
        return;
    }

    int weaponIndex = SignatureWeaponIndex(world);
    if (weaponIndex < 0 || weaponIndex >= (int)weaponDB.size()) {
        return;
    }

    if (persistentOwnedWeapons.size() != weaponDB.size()) {
        persistentOwnedWeapons.resize(weaponDB.size(), false);
    }
    if (shop.ownedWeapons.size() != weaponDB.size()) {
        shop.ownedWeapons.resize(weaponDB.size(), false);
    }

    realmSignatureClaimed[worldSlot] = true;
    persistentOwnedWeapons[weaponIndex] = true;
    shop.ownedWeapons[weaponIndex] = true;
    player.equippedWeaponIdx = weaponIndex;
    persistentEquippedWeaponIdx = weaponIndex;

    announcement = std::string(WorldLabel(world)) + " // " + weaponDB[weaponIndex].name + " CLAIMED";
    announcementTimer = 2.8f;
    AddFloatingText(player.pos, "NEW ARMAMENT", neonGold);
    SaveProfile();
    SaveRun();
}

bool Game::LoadProfile() {
    persistentOwnedWeapons.assign(weaponDB.size(), false);
    if (!persistentOwnedWeapons.empty()) {
        persistentOwnedWeapons[0] = true;
    }
    realmSignatureClaimed.assign(4, false);
    persistentOwnedPets.assign(petDB.size(), false);
    persistentPetBondXp.assign(petDB.size(), 0);
    persistentPetTrainingLevel = 0;
    persistentHpUpgradeLevel = 0;
    persistentEquippedWeaponIdx = 0;
    persistentEquippedPetIdx = -1;
    legacyRenown = 0;
    euro = 0;
    mainQuestIndex = 0;
    mainQuestProgress = 0;
    mainQuestReady = false;
    sideQuestOfferIndices.assign(3, -1);
    sideQuestProgress.assign(3, 0);
    sideQuestAccepted.assign(3, false);
    sideQuestReady.assign(3, false);

    std::ifstream in("profile.txt");
    if (!in.is_open()) {
        for (int i = 0; i < 3; ++i) RefreshSideQuestOffer(i);
        return false;
    }

    std::string header;
    in >> header;
    if (header == "CROWNHEART_PROFILE_V1") {
        in >> legacyRenown >> persistentHpUpgradeLevel >> persistentEquippedWeaponIdx;

        size_t ownedCount = 0;
        in >> ownedCount;
        for (size_t i = 0; i < ownedCount; ++i) {
            int owned = 0;
            in >> owned;
            if (i < persistentOwnedWeapons.size()) {
                persistentOwnedWeapons[i] = (owned != 0);
            }
        }

        for (int i = 0; i < 3; ++i) RefreshSideQuestOffer(i);
    }
    else if (header == "CROWNHEART_PROFILE_V2" || header == "CROWNHEART_PROFILE_V3" || header == "CROWNHEART_PROFILE_V4" || header == "CROWNHEART_PROFILE_V5" || header == "CROWNHEART_PROFILE_V6") {
        in >> legacyRenown >> persistentHpUpgradeLevel >> persistentEquippedWeaponIdx >> euro;

        size_t ownedCount = 0;
        in >> ownedCount;
        for (size_t i = 0; i < ownedCount; ++i) {
            int owned = 0;
            in >> owned;
            if (i < persistentOwnedWeapons.size()) {
                persistentOwnedWeapons[i] = (owned != 0);
            }
        }

        in >> mainQuestIndex >> mainQuestProgress;
        int mainReadyInt = 0;
        in >> mainReadyInt;
        mainQuestReady = (mainReadyInt != 0);

        size_t offerCount = 0;
        in >> offerCount;
        for (size_t i = 0; i < offerCount; ++i) {
            int value = -1;
            in >> value;
            if (i < sideQuestOfferIndices.size()) sideQuestOfferIndices[i] = value;
        }

        size_t progressCount = 0;
        in >> progressCount;
        for (size_t i = 0; i < progressCount; ++i) {
            int value = 0;
            in >> value;
            if (i < sideQuestProgress.size()) sideQuestProgress[i] = value;
        }

        size_t acceptedCount = 0;
        in >> acceptedCount;
        for (size_t i = 0; i < acceptedCount; ++i) {
            int value = 0;
            in >> value;
            if (i < sideQuestAccepted.size()) sideQuestAccepted[i] = (value != 0);
        }

        size_t readyCount = 0;
        in >> readyCount;
        for (size_t i = 0; i < readyCount; ++i) {
            int value = 0;
            in >> value;
            if (i < sideQuestReady.size()) sideQuestReady[i] = (value != 0);
        }

        if (header == "CROWNHEART_PROFILE_V3" || header == "CROWNHEART_PROFILE_V4" || header == "CROWNHEART_PROFILE_V5" || header == "CROWNHEART_PROFILE_V6") {
            size_t signatureCount = 0;
            in >> signatureCount;
            for (size_t i = 0; i < signatureCount; ++i) {
                int value = 0;
                in >> value;
                if (i < realmSignatureClaimed.size()) {
                    realmSignatureClaimed[i] = (value != 0);
                }
            }
        }

        if (header == "CROWNHEART_PROFILE_V4" || header == "CROWNHEART_PROFILE_V5" || header == "CROWNHEART_PROFILE_V6") {
            in >> persistentEquippedPetIdx;

            size_t petOwnedCount = 0;
            in >> petOwnedCount;
            for (size_t i = 0; i < petOwnedCount; ++i) {
                int value = 0;
                in >> value;
                if (i < persistentOwnedPets.size()) {
                    persistentOwnedPets[i] = (value != 0);
                }
            }
        }

        if (header == "CROWNHEART_PROFILE_V5" || header == "CROWNHEART_PROFILE_V6") {
            size_t petBondCount = 0;
            in >> petBondCount;
            for (size_t i = 0; i < petBondCount; ++i) {
                int value = 0;
                in >> value;
                if (i < persistentPetBondXp.size()) {
                    persistentPetBondXp[i] = std::max(0, value);
                }
            }
        }

        if (header == "CROWNHEART_PROFILE_V6") {
            in >> persistentPetTrainingLevel;
        }
    }
    else {
        for (int i = 0; i < 3; ++i) RefreshSideQuestOffer(i);
        return false;
    }

    if (legacyRenown < 0) legacyRenown = 0;
    if (euro < 0) euro = 0;
    if (persistentHpUpgradeLevel < 0) persistentHpUpgradeLevel = 0;
    if (persistentEquippedWeaponIdx < 0 || persistentEquippedWeaponIdx >= (int)weaponDB.size()) {
        persistentEquippedWeaponIdx = 0;
    }
    if (!persistentOwnedWeapons.empty()) {
        persistentOwnedWeapons[0] = true;
        if (!persistentOwnedWeapons[persistentEquippedWeaponIdx]) {
            persistentEquippedWeaponIdx = 0;
        }
    }
    if (mainQuestIndex < 0) mainQuestIndex = 0;
    if (mainQuestIndex >= (int)mainQuestDB.size()) {
        mainQuestIndex = (int)mainQuestDB.size();
        mainQuestProgress = 0;
        mainQuestReady = false;
    }
    else if (!mainQuestDB.empty()) {
        mainQuestProgress = std::min(mainQuestProgress, mainQuestDB[mainQuestIndex].target);
        if (mainQuestProgress < mainQuestDB[mainQuestIndex].target) {
            mainQuestReady = false;
        }
    }

    for (int i = 0; i < 3; ++i) {
        if (sideQuestOfferIndices[i] < 0 || sideQuestOfferIndices[i] >= (int)sideQuestDB.size()) {
            RefreshSideQuestOffer(i);
            continue;
        }

        sideQuestProgress[i] = std::max(0, sideQuestProgress[i]);
        int target = sideQuestDB[sideQuestOfferIndices[i]].target;
        if (sideQuestProgress[i] > target) sideQuestProgress[i] = target;
        if (!sideQuestAccepted[i]) sideQuestReady[i] = false;
        if (sideQuestAccepted[i] && sideQuestProgress[i] >= target) sideQuestReady[i] = true;
    }

    for (WorldId world : { WorldId::Crownheart, WorldId::Frostveil, WorldId::Sunscar, WorldId::Mirethorn }) {
        int signatureIndex = SignatureWeaponIndex(world);
        if (signatureIndex >= 0 && signatureIndex < (int)persistentOwnedWeapons.size() && persistentOwnedWeapons[signatureIndex]) {
            realmSignatureClaimed[WorldIndex(world)] = true;
        }
    }

    if (persistentEquippedPetIdx < -1 || persistentEquippedPetIdx >= (int)petDB.size()) {
        persistentEquippedPetIdx = -1;
    }
    if (persistentOwnedPets.size() != petDB.size()) {
        persistentOwnedPets.resize(petDB.size(), false);
    }
    if (persistentPetBondXp.size() != petDB.size()) {
        persistentPetBondXp.resize(petDB.size(), 0);
    }
    for (int& value : persistentPetBondXp) {
        if (value < 0) {
            value = 0;
        }
    }
    if (persistentPetTrainingLevel < 0) {
        persistentPetTrainingLevel = 0;
    }
    if (persistentEquippedPetIdx >= 0 && !persistentOwnedPets[persistentEquippedPetIdx]) {
        persistentEquippedPetIdx = -1;
    }

    SaveProfile();
    return true;
}

void Game::SaveProfile() const {
    std::ofstream out("profile.txt", std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << kProfileSaveHeaderV6 << '\n';
    out << legacyRenown << ' ' << persistentHpUpgradeLevel << ' ' << persistentEquippedWeaponIdx << ' ' << euro << '\n';

    out << persistentOwnedWeapons.size();
    for (bool owned : persistentOwnedWeapons) {
        out << ' ' << (owned ? 1 : 0);
    }
    out << '\n';

    out << mainQuestIndex << ' ' << mainQuestProgress << ' ' << (mainQuestReady ? 1 : 0) << '\n';

    out << sideQuestOfferIndices.size();
    for (int value : sideQuestOfferIndices) out << ' ' << value;
    out << '\n';

    out << sideQuestProgress.size();
    for (int value : sideQuestProgress) out << ' ' << value;
    out << '\n';

    out << sideQuestAccepted.size();
    for (bool value : sideQuestAccepted) out << ' ' << (value ? 1 : 0);
    out << '\n';

    out << sideQuestReady.size();
    for (bool value : sideQuestReady) out << ' ' << (value ? 1 : 0);
    out << '\n';

    out << realmSignatureClaimed.size();
    for (bool value : realmSignatureClaimed) out << ' ' << (value ? 1 : 0);
    out << '\n';

    out << persistentEquippedPetIdx << '\n';
    out << persistentOwnedPets.size();
    for (bool value : persistentOwnedPets) out << ' ' << (value ? 1 : 0);
    out << '\n';

    out << persistentPetBondXp.size();
    for (int value : persistentPetBondXp) out << ' ' << value;
    out << '\n';

    out << persistentPetTrainingLevel << '\n';
}

bool Game::HasSaveFile() const {
    std::ifstream in("savegame.txt");
    return in.good();
}

void Game::DeleteSave() const {
    std::remove("savegame.txt");
}

void Game::DeleteProfile() const {
    std::remove("profile.txt");
}

void Game::SaveRun() const {
    std::ofstream out("savegame.txt", std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << kRunSaveHeaderV4 << '\n';
    out << (int)currentWorld << '\n';
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

    out << (shopkeeperCutsceneSeen ? 1 : 0) << ' ' << shopkeeperStoryStage << ' ' << (blessingMemorySeen ? 1 : 0) << '\n';
    for (int i = 0; i < 4; ++i) {
        if (i > 0) out << ' ';
        out << (realmIntroSeen[i] ? 1 : 0);
    }
    out << '\n';
    for (int i = 0; i < 4; ++i) {
        if (i > 0) out << ' ';
        out << (bossIntroSeen[i] ? 1 : 0);
    }
    out << '\n';
    for (int i = 0; i < 4; ++i) {
        if (i > 0) out << ' ';
        out << (blessingMemorySeenByWorld[i] ? 1 : 0);
    }
    out << '\n';
    for (int i = 0; i < 4; ++i) {
        if (i > 0) out << ' ';
        out << (bossAftermathSeen[i] ? 1 : 0);
    }
    out << '\n';
}

bool Game::LoadRun() {
    std::ifstream in("savegame.txt");
    if (!in.is_open()) {
        return false;
    }

    std::string header;
    in >> header;
    bool isV2 = (header == kLegacyRunSaveHeaderV2);
    bool isV3 = (header == kLegacyRunSaveHeaderV3);
    bool isV4 = (header == kRunSaveHeaderV4);
    if (!isV2 && !isV3 && !isV4) {
        return false;
    }

    ResetRun();

    if (isV3 || isV4) {
        int worldValue = 0;
        in >> worldValue;
        if (worldValue >= 0 && worldValue <= 3) {
            currentWorld = (WorldId)worldValue;
            BuildDungeon();
            BuildTileMap();
            LoadAssets();
        }
    }
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
    questBoardOpen = false;
    realmMapOpen = false;

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

    in >> std::ws;
    if (in.peek() != EOF) {
        int shopkeeperSeenInt = 0;
        in >> shopkeeperSeenInt >> shopkeeperStoryStage >> blessingMemorySeen;
        shopkeeperCutsceneSeen = (shopkeeperSeenInt != 0);
        for (int i = 0; i < 4; ++i) {
            int value = 0;
            in >> value;
            realmIntroSeen[i] = (value != 0);
        }
        for (int i = 0; i < 4; ++i) {
            int value = 0;
            in >> value;
            bossIntroSeen[i] = (value != 0);
        }
        for (int i = 0; i < 4; ++i) {
            int value = 0;
            in >> value;
            blessingMemorySeenByWorld[i] = (value != 0);
        }
        in >> std::ws;
        if (in.peek() != EOF) {
            for (int i = 0; i < 4; ++i) {
                int value = 0;
                in >> value;
                bossAftermathSeen[i] = (value != 0);
            }
        }
    }
    if (in.fail() && !in.eof()) {
        return false;
    }
    in.clear();

    if (!in.good() && !in.eof()) {
        return false;
    }

    persistentOwnedWeapons = shop.ownedWeapons;
    persistentHpUpgradeLevel = player.hpUpgradeLevel;
    persistentEquippedWeaponIdx = player.equippedWeaponIdx;
    SaveProfile();

    shop.isOpen = false;
    shop.message.clear();
    shop.messageTimer = 0.0f;
    rewardChoices.clear();
    SyncPetState(true);
    gameState = GameState::Playing;
    announcement = "CHRONICLE RESTORED";
    announcementTimer = 2.2f;
    return true;
}

void Game::ResetRun() {
    currentWorld = WorldId::Crownheart;
    BuildDungeon();
    BuildTileMap();
    LoadAssets();

    if (persistentOwnedWeapons.size() < weaponDB.size()) {
        persistentOwnedWeapons.resize(weaponDB.size(), false);
    }
    if (!persistentOwnedWeapons.empty()) {
        persistentOwnedWeapons[0] = true;
    }
    if (realmSignatureClaimed.size() < 4) {
        realmSignatureClaimed.assign(4, false);
    }

    player = Player{};
    player.pos = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.72f };
    player.aimDir = { 0.0f, -1.0f };
    persistentHpUpgradeLevel = std::max(0, std::min(persistentHpUpgradeLevel, 8));
    persistentPetTrainingLevel = std::max(0, std::min(persistentPetTrainingLevel, 6));
    player.hpUpgradeLevel = persistentHpUpgradeLevel;
    player.maxHp = 160 + persistentHpUpgradeLevel * 18;
    player.hp = player.maxHp;
    player.xp = 40 + legacyRenown / 4;
    legacyRenown = 0;

    shop = ShopState{};
    shop.ownedWeapons = persistentOwnedWeapons;
    if (shop.ownedWeapons.size() < weaponDB.size()) {
        shop.ownedWeapons.resize(weaponDB.size(), false);
    }
    if (!shop.ownedWeapons.empty()) {
        shop.ownedWeapons[0] = true;
    }
    shop.browsePetIdx = 0;
    player.equippedWeaponIdx = 0;
    persistentEquippedWeaponIdx = 0;
    if (persistentOwnedPets.size() != petDB.size()) {
        persistentOwnedPets.resize(petDB.size(), false);
    }
    if (persistentPetBondXp.size() != petDB.size()) {
        persistentPetBondXp.resize(petDB.size(), 0);
    }
    persistentEquippedPetIdx = -1;
    SaveProfile();

    relics.clear();
    rewardChoices.clear();
    rewardChestActive = false;
    rewardSelectionOpen = false;
    questBoardOpen = false;
    realmMapOpen = false;
    rewardChestPos = { 0.0f, 0.0f };
    waveTargetRoomIndex = -1;
    lockedRoomIndex = -1;
    cutscene = {};
    for (int i = 0; i < 4; ++i) { realmIntroSeen[i] = false; bossIntroSeen[i] = false; bossAftermathSeen[i] = false; blessingMemorySeenByWorld[i] = false; }
    realmIntroSeen[0] = true;
    shopkeeperCutsceneSeen = false;
    shopkeeperStoryStage = -1;
    blessingMemorySeen = false;
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
    SyncPetState(true);

    SpawnWave(player.wave);
}

void Game::StartCutscene(const std::string& sceneName, const std::vector<CutsceneStep>& steps) {
    cutscene = {};
    cutscene.active = true;
    cutscene.sceneName = sceneName;
    cutscene.steps = steps;
    cutscene.cameraTarget = player.pos;
    cutscene.cameraStart = player.pos;
    cutscene.stepOrigin = player.pos;
    cutscene.fadeAlpha = 1.0f;
    cutscene.lockCamera = true;
    cutscene.sceneTitleMax = 1.10f;
    if (sceneName.find("BOSS //") != std::string::npos) cutscene.sceneTitleMax = 1.85f;
    else if (sceneName.find("FINAL VOW") != std::string::npos) cutscene.sceneTitleMax = 1.65f;
    else if (sceneName.find("CHAPTER //") != std::string::npos) cutscene.sceneTitleMax = 1.45f;
    else if (sceneName.find("REALM ENTRY") != std::string::npos) cutscene.sceneTitleMax = 1.30f;
    cutscene.sceneTitleTimer = cutscene.sceneTitleMax;
    cutscene.speakerColor = neonGold;
    for (const auto& step : steps) {
        if (step.type == CutsceneStepType::Dialogue) {
            cutscene.speakerColor = step.color;
            break;
        }
    }
    announcementTimer = 0.0f;
    shop.isOpen = false;
    rewardSelectionOpen = false;
    questBoardOpen = false;
    realmMapOpen = false;
    AdvanceCutsceneStep();
}

void Game::StartIntroCutscene() {
    Vector2 gateLook = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.18f };
    Vector2 courtCenter = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.38f };
    Vector2 knightMark = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.68f };

    player.pos = knightMark;
    player.aimDir = { 0.0f, -1.0f };

    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 1.10f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Wait, 0.18f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 1.15f, gateLook, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, gateLook, "THE CHRONICLE", "On the night the bells died, Crownheart burned by oath, by fire, and by betrayal.", neonGold, true, true });
    steps.push_back({ CutsceneStepType::PanCamera, 1.00f, courtCenter, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, courtCenter, "THE CHRONICLE", "Princess Seralyne vanished. Prince Vaelor raised a rival banner. The keep learned grief before dawn.", softRed, true, true });
    steps.push_back({ CutsceneStepType::PanCamera, 1.05f, knightMark, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, knightMark, "THE KNIGHT", "If she yet lives, I will find her. If the crown is ash, I will carry its sorrow through every realm.", neonCyan, false, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, knightMark, "THE LAST BELL", "Ride now. Cleanse the sealed courts. Let the broken throne remember your name.", neonGold, true, true });
    StartCutscene("PROLOGUE // THE LAST VOW", steps);
}

void Game::StartRealmIntroCutscene(WorldId world) {
    std::string line1;
    std::string line2;
    std::string speaker = "THE CHRONICLE";
    Color color = neonGold;

    switch (world) {
    case WorldId::Crownheart:
        line1 = "Crownheart still stands, but every stone carries the weight of a dying oath.";
        line2 = "The keep is sanctuary only for those willing to ride back into the dark.";
        color = neonGold;
        break;
    case WorldId::Frostveil:
        line1 = "Frostveil Pass sleeps beneath silver snow, chapel glass, and vows that never thawed.";
        line2 = "Step softly. Even the cold here remembers who failed the crown.";
        color = WorldAccentTint(world);
        break;
    case WorldId::Sunscar:
        line1 = "Sunscar burns with caravan ash, split stone, and the heat of broken banners.";
        line2 = "What was once a king's road is now a furnace for the living and the condemned.";
        color = WorldAccentTint(world);
        break;
    case WorldId::Mirethorn:
        line1 = "Mirethorn drowns its courts in thorn, rot, and black water older than mercy.";
        line2 = "Do not trust the fog. It keeps the names of the dead and lends them voices.";
        color = WorldAccentTint(world);
        break;
    }

    Vector2 gateLook = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.28f };
    Vector2 knightMark = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.68f };
    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.45f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.80f, gateLook, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, gateLook, speaker, line1, color, true, true });
    steps.push_back({ CutsceneStepType::PanCamera, 0.85f, knightMark, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, knightMark, "THE KNIGHT", line2, neonCyan, false, true });
    StartCutscene(std::string("REALM ENTRY // ") + WorldLabel(world), steps);
}

void Game::StartChapterUnlockCutscene(WorldId world) {
    Vector2 boardLook = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.46f };
    std::string sceneName = "CHAPTER // A NEW ROAD";
    std::string line1;
    std::string line2;
    std::string line3;

    switch (world) {
    case WorldId::Frostveil:
        sceneName = "CHAPTER // FROSTVEIL UNSEALED";
        line1 = "Maerwyn lays a torn ribbon on the oathboard. Princess Seralyne wore chapel-blue on the night she vanished.";
        line2 = "Scouts found the cloth half-buried in Frostveil snow. If hope still breathes, it breathes there.";
        line3 = "Then I ride north before the cold buries her trail.";
        break;
    case WorldId::Sunscar:
        sceneName = "CHAPTER // SUNSCAR UNSEALED";
        line1 = "A red seal is hammered into the board. Prince Vaelor marches beneath ash banners through Sunscar.";
        line2 = "He buys loyalty with hunger, gold, and the promise that Crownheart is already dead.";
        line3 = "Then I will answer him in the language of steel.";
        break;
    case WorldId::Mirethorn:
        sceneName = "CHAPTER // MIRETHORN UNSEALED";
        line1 = "The ferrymen speak at last. A royal prayer-song was heard drifting through Mirethorn fog.";
        line2 = "If Seralyne was taken alive, the black water may be the last place her memory still walks.";
        line3 = "Then I follow the song into the mire, even if it leads me to a grave.";
        break;
    default:
        return;
    }

    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.32f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.62f, boardLook, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "MAERWYN", line1, safeGreen, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "MAERWYN", line2, neonGold, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "THE KNIGHT", line3, neonCyan, false, true });
    StartCutscene(sceneName, steps);
}

void Game::StartFinalCharterCutscene() {
    Vector2 boardLook = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.46f };
    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.34f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.72f, boardLook, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "MAERWYN", "The charter line is ended. No more boards, no more petitions. Only the truth that waits beyond the broken throne.", safeGreen, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "PRINCESS SERALYNE", "When crowns begin to rot, seek not the loudest banner, but the wound hidden beneath it.", softRed, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "MAERWYN", "Vaelor will guard the throne room with every lie he owns. Go anyway. Seralyne would have.", neonGold, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, boardLook, "THE KNIGHT", "Then I ride for the wound itself, and for the princess he failed to break.", neonCyan, false, true });
    StartCutscene("FINAL VOW // THE BROKEN THRONE", steps);
}

void Game::StartShopkeeperCutscene(int stage) {
    Vector2 hallLook = { safeZone.x + safeZone.width * 0.5f, safeZone.y + safeZone.height * 0.52f };
    std::string line1;
    std::string line2;
    std::string line3;
    std::string sceneName = "BLACK HALL // MAERWYN";

    if (stage <= 0) {
        line1 = "I am Maerwyn, last armorer of the Black Hall. Bring me renown, and I will return grief to the world as steel.";
        line2 = "Do not chase every bright blade. Choose the weapon whose sorrow you can carry without breaking.";
        line3 = "Keep the forge lit, Maerwyn. This kingdom is not done asking blood of me.";
    }
    else if (stage == 1) {
        line1 = "Frostveil does not forgive. The snow keeps prints, prayers, and the shape of those who fled too late.";
        line2 = "If Princess Seralyne left a trail, the cold may have preserved what fire could not.";
        line3 = "Then I will search the frozen chapels until the silence answers me.";
    }
    else if (stage == 2) {
        line1 = "Sunscar is Prince Vaelor's country now. His red banners drink coin from caravans and hope from men.";
        line2 = "He wants the realm to believe Crownheart is already a memory. Cut that lie out of the dunes.";
        line3 = "Let him wait beneath his banners. I am coming.";
    }
    else if (stage == 3) {
        line1 = "Mirethorn keeps what other realms lose: crowns, corpses, confessions. Nothing sinks there without a witness.";
        line2 = "If the kingdom still hides its deepest wound, you will find it where the fog refuses to speak plainly.";
        line3 = "Then let the mire hear me clearly.";
    }
    else {
        line1 = "Seralyne never feared the truth of steel. Vaelor did. That is why one vanished and the other hid behind banners.";
        line2 = "If you reach the broken throne, choose the living heart of the kingdom over the easier legend. Bring her home if home still exists.";
        line3 = "I will. And if Vaelor stands in that last hall, he answers for every bell that died.";
    }

    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.35f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.55f, hallLook, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, hallLook, "MAERWYN", line1, safeGreen, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, hallLook, "MAERWYN", line2, neonGold, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, hallLook, "THE KNIGHT", line3, neonCyan, false, true });
    StartCutscene(sceneName, steps);
}

void Game::StartBlessingMemoryCutscene(WorldId world) {
    Vector2 reliquaryLook = rewardChestPos;
    std::string line1;
    std::string line2;
    std::string title = std::string("RELIQUARY MEMORY // ") + WorldLabel(world);

    switch (world) {
    case WorldId::Crownheart:
        line1 = "Before the bells died, Princess Seralyne lit the chapel lamps herself and said the keep should always look warm from the road.";
        line2 = "Take what grace remains. A kingdom is not saved by steel alone.";
        break;
    case WorldId::Frostveil:
        line1 = "Seralyne laughed once in Frostveil snowfall, palms open, as if winter were gentler than courtly vows.";
        line2 = "If you find only silence here, listen harder. Love leaves marks even in the cold.";
        break;
    case WorldId::Sunscar:
        line1 = "Sunscar remembers the day Prince Vaelor bowed with a smile and promised peace he never meant to keep.";
        line2 = "Do not mistake charm for mercy. Some betrayals arrive perfumed and crowned.";
        break;
    case WorldId::Mirethorn:
        line1 = "Black water carries a prayer she once sang in secret. The melody broke, but it did not drown.";
        line2 = "If you still seek me, follow what the dead were too frightened to bury.";
        break;
    }

    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.40f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.60f, reliquaryLook, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, reliquaryLook, "THE RELIQUARY", line1, neonGold, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, reliquaryLook, "PRINCESS SERALYNE", line2, softRed, true, true });
    StartCutscene(title, steps);
}

void Game::StartBossIntroCutscene(const ActiveMonster& monster) {
    std::string line1;
    std::string line2;
    if (monster.name == "THE ASHEN WYRM") {
        line1 = "The Ashen Wyrm wakes beneath scorched stone, carrying oath-fire in its lungs.";
        line2 = "Then let it choke on steel before it reaches the keep.";
    }
    else if (monster.name == "THE THORN KING") {
        line1 = "The Thorn King rises from black water and briar like a crown the dead refused to bury.";
        line2 = "Then I will cut that crown apart.";
    }
    else {
        line1 = monster.name + " stands within the sealed court and dares the kingdom to kneel.";
        line2 = "I did not come to kneel.";
    }

    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.28f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.75f, monster.pos, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, monster.pos, monster.name, line1, monster.color, true, true });
    steps.push_back({ CutsceneStepType::PanCamera, 0.55f, player.pos, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, player.pos, "THE KNIGHT", line2, neonCyan, false, true });
    StartCutscene(std::string("BOSS // ") + monster.name, steps);
}

void Game::StartBossAftermathCutscene(WorldId world, const std::string& bossName, Vector2 bossPos) {
    std::string line1;
    std::string line2;
    std::string line3;

    switch (world) {
    case WorldId::Crownheart:
        line1 = "The first great beast falls, and the keep roads breathe again beneath the night.";
        line2 = "One wound closes, yet the kingdom's deeper sorrow still rides beyond these walls.";
        line3 = "Then I press on. A living crown is not saved by stopping at the first victory.";
        break;
    case WorldId::Frostveil:
        line1 = "Frozen beneath the bloodied chapel glass lies a ribbon marked with Princess Seralyne's seal.";
        line2 = "She passed through Frostveil alive. The cold kept what the fire tried to erase.";
        line3 = "Then her trail still lives. I ride after it before the snow swallows the last sign.";
        break;
    case WorldId::Sunscar:
        line1 = "The desert carrion scatters. Word will reach Prince Vaelor that Crownheart still answers in steel.";
        line2 = "Let Vaelor's red banners tremble. False kings rule loudly because they fear the return of the rightful dead.";
        line3 = "Then I will make him hear my coming across every mile of ash road.";
        break;
    case WorldId::Mirethorn:
        line1 = "The black water gives back a prayer-song, and somewhere beneath the briars the broken throne begins to stir.";
        line2 = "The deepest wound is close now. If Seralyne yet breathes, the kingdom is no longer hiding her grief from you.";
        line3 = "Then let the last road open. I will meet the truth where the crown was buried.";
        break;
    }

    std::vector<CutsceneStep> steps;
    steps.push_back({ CutsceneStepType::Fade, 0.22f, { 0.0f, 0.0f }, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::PanCamera, 0.62f, bossPos, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, bossPos, bossName, line1, softRed, true, true });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, bossPos, "THE CHRONICLE", line2, neonGold, true, true });
    steps.push_back({ CutsceneStepType::PanCamera, 0.48f, player.pos, "", "", WHITE, false, false });
    steps.push_back({ CutsceneStepType::Dialogue, 0.0f, player.pos, "THE KNIGHT", line3, neonCyan, false, true });
    StartCutscene(std::string("AFTERMATH // ") + bossName, steps);
}

void Game::AdvanceCutsceneStep() {
    if (!cutscene.active) {
        return;
    }

    cutscene.currentStep++;
    cutscene.stepTimer = 0.0f;
    cutscene.stepOrigin = player.pos;
    cutscene.cameraStart = cutscene.cameraTarget;
    cutscene.fadeStart = cutscene.fadeAlpha;
    cutscene.awaitingAdvance = false;
    cutscene.showPortrait = false;
    cutscene.speaker.clear();
    cutscene.text.clear();

    if (cutscene.currentStep < 0 || cutscene.currentStep >= (int)cutscene.steps.size()) {
        std::string endAnnouncement;
        float endAnnouncementTimer = 2.2f;
        if (cutscene.sceneName.find("PROLOGUE") != std::string::npos) {
            endAnnouncement = "PROLOGUE ENDED // THE HUNT BEGINS";
            endAnnouncementTimer = 2.6f;
            PlaySoundSafe(sfxBanner);
        }
        else if (cutscene.sceneName.find("BLACK HALL") != std::string::npos) {
            endAnnouncement = "MAERWYN WAITS // PRESS E TO ENTER THE ARMORY";
            PlaySoundSafe(sfxUiAccept);
        }
        else if (cutscene.sceneName.find("RELIQUARY MEMORY") != std::string::npos) {
            endAnnouncement = "RELIQUARY AWAKENED // PRESS E TO CLAIM A BLESSING";
            PlaySoundSafe(sfxReward);
        }
        else if (cutscene.sceneName.find("REALM ENTRY") != std::string::npos) {
            endAnnouncement = std::string(WorldLabel(currentWorld)) + " // ENTER THE HUNT";
            PlaySoundSafe(sfxTravel);
        }
        else if (cutscene.sceneName.find("FINAL VOW") != std::string::npos) {
            endAnnouncement = "FINAL VOW AWAKENED // THE KINGDOM REMEMBERS";
            endAnnouncementTimer = 2.8f;
            PlaySoundSafe(sfxQuest);
        }
        else if (cutscene.sceneName.find("AFTERMATH //") != std::string::npos) {
            endAnnouncement = "THE ROAD DEEPENS // RIDE ON";
            endAnnouncementTimer = 2.4f;
            PlaySoundSafe(sfxQuest);
        }
        else if (cutscene.sceneName.find("CHAPTER //") != std::string::npos) {
            endAnnouncement = "A NEW ROAD STIRS // USE THE GATE OF REALMS";
            PlaySoundSafe(sfxQuest);
        }
        else if (cutscene.sceneName.find("BOSS //") != std::string::npos) {
            endAnnouncement = "THE COURT SHUDDERS // STAND FAST";
            PlaySoundSafe(sfxBossIntro);
        }
        cutscene.active = false;
        cutscene.lockCamera = false;
        cutscene.fadeAlpha = 0.0f;
        cutscene.letterbox = 0.0f;
        cutscene.sceneTitleTimer = 0.0f;
        cutscene.sceneTitleMax = 0.0f;
        if (!endAnnouncement.empty()) {
            announcement = endAnnouncement;
            announcementTimer = endAnnouncementTimer;
        }
        SaveRun();
        return;
    }

    const CutsceneStep& step = cutscene.steps[cutscene.currentStep];
    if (step.type == CutsceneStepType::Dialogue) {
        cutscene.lockCamera = true;
        if (step.target.x != 0.0f || step.target.y != 0.0f) {
            cutscene.cameraTarget = step.target;
        }
        cutscene.speaker = step.speaker;
        cutscene.text = step.text;
        cutscene.speakerColor = step.color;
        cutscene.showPortrait = step.portraitMode;
        cutscene.awaitingAdvance = step.requireAdvance;
        PlaySoundSafe(sfxUiMove);
    }
    else if (step.type == CutsceneStepType::PanCamera) {
        cutscene.lockCamera = true;
    }
    else if (step.type == CutsceneStepType::MovePlayer) {
        cutscene.lockCamera = true;
        PlaySoundSafe(sfxTravel);
    }
}

void Game::UpdateCutscene(float dt) {
    if (!cutscene.active || cutscene.currentStep < 0 || cutscene.currentStep >= (int)cutscene.steps.size()) {
        return;
    }

    if (IsKeyPressed(KEY_F)) {
        cutscene.active = false;
        cutscene.lockCamera = false;
        cutscene.fadeAlpha = 0.0f;
        cutscene.letterbox = 0.0f;
        cutscene.sceneTitleTimer = 0.0f;
        cutscene.sceneTitleMax = 0.0f;
        cutscene.text.clear();
        cutscene.speaker.clear();
        announcement = "CUTSCENE SKIPPED // THE HUNT BEGINS";
        announcementTimer = 2.0f;
        SaveRun();
        return;
    }

    cutscene.stepTimer += dt;
    cutscene.sceneTitleTimer = std::max(0.0f, cutscene.sceneTitleTimer - dt);
    const CutsceneStep& step = cutscene.steps[cutscene.currentStep];

    switch (step.type) {
    case CutsceneStepType::Wait:
        if (cutscene.stepTimer >= step.duration) {
            AdvanceCutsceneStep();
        }
        break;
    case CutsceneStepType::Fade: {
        float t = (step.duration > 0.0f) ? ClampFloat(cutscene.stepTimer / step.duration, 0.0f, 1.0f) : 1.0f;
        float targetAlpha = ClampFloat(step.target.x, 0.0f, 1.0f);
        cutscene.fadeAlpha = LerpFloat(cutscene.fadeStart, targetAlpha, t);
        if (cutscene.stepTimer >= step.duration) {
            cutscene.fadeAlpha = targetAlpha;
            AdvanceCutsceneStep();
        }
        break;
    }
    case CutsceneStepType::PanCamera: {
        float t = (step.duration > 0.0f) ? ClampFloat(cutscene.stepTimer / step.duration, 0.0f, 1.0f) : 1.0f;
        cutscene.cameraTarget = {
            LerpFloat(cutscene.cameraStart.x, step.target.x, t),
            LerpFloat(cutscene.cameraStart.y, step.target.y, t)
        };
        if (cutscene.stepTimer >= step.duration) {
            cutscene.cameraTarget = step.target;
            AdvanceCutsceneStep();
        }
        break;
    }
    case CutsceneStepType::MovePlayer: {
        float t = (step.duration > 0.0f) ? ClampFloat(cutscene.stepTimer / step.duration, 0.0f, 1.0f) : 1.0f;
        Vector2 nextPos = {
            LerpFloat(cutscene.stepOrigin.x, step.target.x, t),
            LerpFloat(cutscene.stepOrigin.y, step.target.y, t)
        };
        Vector2 face = VecNormalizeSafe(VecSub(step.target, player.pos));
        if (face.x != 0.0f || face.y != 0.0f) {
            player.aimDir = face;
        }
        player.pos = nextPos;
        cutscene.cameraTarget = VecAdd(player.pos, VecScale(player.aimDir, 18.0f));
        if (cutscene.stepTimer >= step.duration) {
            player.pos = step.target;
            AdvanceCutsceneStep();
        }
        break;
    }
    case CutsceneStepType::Dialogue:
        cutscene.lockCamera = true;
        if (step.target.x != 0.0f || step.target.y != 0.0f) {
            cutscene.cameraTarget = step.target;
        }
        if (step.requireAdvance) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_E)) {
                PlaySoundSafe(sfxUiAccept);
                AdvanceCutsceneStep();
            }
        }
        else if (step.duration > 0.0f && cutscene.stepTimer >= step.duration) {
            AdvanceCutsceneStep();
        }
        break;
    }

    float desiredLetterbox = (!cutscene.speaker.empty() || cutscene.showPortrait) ? 58.0f : 34.0f;
    cutscene.letterbox = LerpFloat(cutscene.letterbox, desiredLetterbox, ClampFloat(dt * 6.0f, 0.0f, 1.0f));
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
    PlaySoundSafe(sfxBanner);
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

    std::vector<int> worldPool;
    int bossType = 10;
    switch (currentWorld) {
    case WorldId::Crownheart:
        worldPool = { 0, 2, 3, 4, 7, 9 };
        bossType = 10;
        break;
    case WorldId::Frostveil:
        worldPool = { 1, 4, 5, 6, 8 };
        bossType = 11;
        break;
    case WorldId::Sunscar:
        worldPool = { 0, 2, 3, 5, 7, 9 };
        bossType = 10;
        break;
    case WorldId::Mirethorn:
        worldPool = { 0, 1, 4, 5, 8, 9 };
        bossType = 11;
        break;
    }

    int realmTier = WorldIndex(currentWorld);
    int normalCount = 6 + waveNumber * 2 + realmTier * 2;
    int allowedTypes = std::min((int)worldPool.size(), 2 + waveNumber / 2 + realmTier / 2);
    if (allowedTypes < 1) {
        allowedTypes = 1;
    }

    for (int i = 0; i < normalCount; ++i) {
        int typeIndex = worldPool[std::rand() % allowedTypes];
        SpawnMonsterByType(typeIndex, GetSpawnPointInCombatRoom());
    }

    if (waveNumber >= 3 + realmTier) {
        int pressureAdds = 1 + realmTier;
        for (int i = 0; i < pressureAdds; ++i) {
            int typeIndex = worldPool[(allowedTypes - 1 + i) % allowedTypes];
            SpawnMonsterByType(typeIndex, GetSpawnPointInCombatRoom());
        }
    }

    if (waveNumber % 5 == 0) {
        SpawnMonsterByType(bossType, GetSpawnPointInCombatRoom());
        announcement = std::string(WorldLabel(currentWorld)) + " // " + monsterTypes[bossType].name + " STIRS";
        announcementTimer = 3.8f;
        PlaySoundSafe(sfxBossIntro);
    }
}

void Game::SyncPetState(bool snapToPlayer) {
    if (persistentOwnedPets.size() != petDB.size()) {
        persistentOwnedPets.resize(petDB.size(), false);
    }
    if (persistentPetBondXp.size() != petDB.size()) {
        persistentPetBondXp.resize(petDB.size(), 0);
    }

    if (persistentEquippedPetIdx < 0 || persistentEquippedPetIdx >= (int)petDB.size()) {
        pet = ActivePet{};
        return;
    }

    if (!persistentOwnedPets[persistentEquippedPetIdx]) {
        pet = ActivePet{};
        return;
    }

    bool wasInactive = !pet.active || pet.petIndex != persistentEquippedPetIdx;
    pet.active = true;
    pet.petIndex = persistentEquippedPetIdx;
    if (wasInactive) {
        pet.fireTimer = 0.20f;
        pet.orbitAngle = 0.0f;
        pet.bob = 0.0f;
    }
    if (snapToPlayer || wasInactive) {
        pet.pos = VecAdd(player.pos, { 26.0f, -20.0f });
    }
}

void Game::AwardPetBond(int amount) {
    if (!pet.active || pet.petIndex < 0 || pet.petIndex >= (int)petDB.size()) {
        return;
    }

    if (persistentPetBondXp.size() != petDB.size()) {
        persistentPetBondXp.resize(petDB.size(), 0);
    }

    int oldLevel = GetPetBondLevel(pet.petIndex);
    persistentPetBondXp[pet.petIndex] = std::max(0, persistentPetBondXp[pet.petIndex] + amount);
    int newLevel = GetPetBondLevel(pet.petIndex);

    if (newLevel > oldLevel) {
        announcement = petDB[pet.petIndex].name + " // BOND RANK " + std::to_string(newLevel);
        announcementTimer = 2.2f;
        AddFloatingText(pet.pos, "BOND UP", petDB[pet.petIndex].color);
        SaveProfile();
    }
    else if ((persistentPetBondXp[pet.petIndex] % 5) == 0) {
        SaveProfile();
    }
}

void Game::ApplyWeaponHitEffect(const Weapon& weapon, ActiveMonster& monster, int damage, bool isCrit) {
    switch (weapon.trait) {
    case WeaponTrait::Frost:
        monster.slowTimer = std::max(monster.slowTimer, 1.8f);
        EmitBurst(monster.pos, 5, 2.8f, Fade({ 206, 236, 255, 255 }, 0.95f), 2.2f);
        break;
    case WeaponTrait::Chain: {
        int arcs = 0;
        for (auto& other : monsters) {
            if (&other == &monster || other.hp <= 0) {
                continue;
            }
            if (WithinRange(other.pos, monster.pos, 84.0f)) {
                int arcDamage = std::max(4, damage / 4);
                other.hp -= arcDamage;
                other.hitFlash = 0.08f;
                beams.push_back({ monster.pos, other.pos, weapon.color, 2.8f, 0.10f });
                AddFloatingText(other.pos, "ARC " + std::to_string(arcDamage), weapon.color);
                arcs++;
                if (arcs >= 1) {
                    break;
                }
            }
        }
        break;
    }
    case WeaponTrait::Sunfire:
        monster.burnTimer = std::max(monster.burnTimer, 2.6f);
        monster.burnTickTimer = 0.18f;
        EmitBurst(monster.pos, 4, 2.2f, { 236, 170, 78, 255 }, 2.4f);
        break;
    case WeaponTrait::Venom:
        monster.poisonTimer = std::max(monster.poisonTimer, 3.3f);
        monster.poisonTickTimer = 0.18f;
        EmitBurst(monster.pos, 4, 2.0f, { 150, 206, 106, 255 }, 2.4f);
        break;
    case WeaponTrait::Guardian:
        if (player.hp < player.maxHp) {
            player.hp = std::min(player.maxHp, player.hp + 1);
        }
        break;
    case WeaponTrait::Heavy:
        hitStopTimer = std::max(hitStopTimer, 0.05f);
        screenShake = std::max(screenShake, 9.0f);
        break;
    case WeaponTrait::Royal:
        shockwaves.push_back({ monster.pos, 14.0f, 92.0f, 0.18f, 0.18f, Fade(weapon.color, 0.72f) });
        for (auto& other : monsters) {
            if (&other == &monster || other.hp <= 0) {
                continue;
            }
            if (WithinRange(other.pos, monster.pos, 58.0f)) {
                int splashDamage = std::max(3, damage / 7);
                other.hp -= splashDamage;
                other.hitFlash = 0.08f;
            }
        }
        break;
    default:
        break;
    }

    if (weapon.name == "Oathkeeper Blade") {
        if ((monster.isElite || monster.isBoss) && player.hp < player.maxHp) {
            player.hp = std::min(player.maxHp, player.hp + 1);
        }
    }
    else if (weapon.name == "Widowglass Thorn") {
        if (isCrit) {
            monster.slowTimer = std::max(monster.slowTimer, 3.4f);
        }
    }
    else if (weapon.name == "Ashsworn Falchion") {
        monster.burnTimer = std::max(monster.burnTimer, isCrit ? 4.2f : 3.2f);
        if (isCrit) {
            for (auto& other : monsters) {
                if (&other == &monster || other.hp <= 0) {
                    continue;
                }
                if (WithinRange(other.pos, monster.pos, 56.0f)) {
                    other.burnTimer = std::max(other.burnTimer, 2.4f);
                    other.burnTickTimer = 0.18f;
                    other.hitFlash = 0.08f;
                }
            }
        }
    }
    else if (weapon.name == "Blackrose Pike") {
        if (player.hp < player.maxHp) {
            player.hp = std::min(player.maxHp, player.hp + 1);
        }
    }
    else if (weapon.name == "Sword Without Crown") {
        shockwaves.push_back({ monster.pos, 16.0f, 112.0f, 0.20f, 0.20f, Fade(weapon.color, 0.60f) });
        if ((monster.isElite || monster.isBoss) && player.hp < player.maxHp) {
            player.hp = std::min(player.maxHp, player.hp + (monster.isBoss ? 2 : 1));
        }
    }
    else if (weapon.name == "Vesper Greatblade") {
        if (isCrit) {
            shockwaves.push_back({ monster.pos, 12.0f, 86.0f, 0.16f, 0.16f, Fade(weapon.color, 0.60f) });
        }
    }
    else if (weapon.name == "Chapelglass Estoc") {
        if (isCrit && player.hp >= player.maxHp) {
            player.dashCd = std::max(0.0f, player.dashCd - 0.45f);
        }
    }
    else if (weapon.name == "Suncleft Oathaxe") {
        monster.burnTimer = std::max(monster.burnTimer, isCrit ? 4.4f : 3.4f);
        if (isCrit) {
            for (auto& other : monsters) {
                if (&other == &monster || other.hp <= 0) continue;
                if (WithinRange(other.pos, monster.pos, 64.0f)) {
                    other.burnTimer = std::max(other.burnTimer, 2.6f);
                    other.burnTickTimer = 0.18f;
                    other.hitFlash = 0.08f;
                }
            }
        }
    }
    else if (weapon.name == "Griefthorn Partisan") {
        if (monster.hp <= monster.maxHp / 2 && player.hp < player.maxHp) {
            player.hp = std::min(player.maxHp, player.hp + 2);
        }
    }
    else if (weapon.name == "King's Lament") {
        shockwaves.push_back({ monster.pos, 18.0f, 124.0f, 0.20f, 0.20f, Fade(weapon.color, 0.55f) });
        if ((monster.isElite || monster.isBoss) && player.hp < player.maxHp) {
            player.hp = std::min(player.maxHp, player.hp + (monster.isBoss ? 3 : 2));
        }
    }

    if (isCrit) {
        EmitBurst(monster.pos, 4, 2.8f, neonGold, 2.2f);
    }
}

void Game::UpdateTitle() {
    if (IsKeyPressed(KEY_M)) {
        if (!SoundLoaded(sfxUiAccept)) {
            announcement = "SFX NOT LOADED";
            announcementTimer = 2.0f;
        }
        else {
            PlaySoundSafe(sfxUiAccept);
            announcement = "AUDIO TEST // CHIME";
            announcementTimer = 2.0f;
        }
    }

    if (IsKeyPressed(KEY_F)) {
        DeleteSave();
        DeleteProfile();
        LoadProfile();
        ResetRun();
        gameState = GameState::Playing;
        StartIntroCutscene();
        SaveRun();
        PlaySoundSafe(sfxUiAccept);
    }
    else if (IsKeyPressed(KEY_ENTER)) {
        DeleteSave();
        ResetRun();
        gameState = GameState::Playing;
        StartIntroCutscene();
        SaveRun();
        PlaySoundSafe(sfxUiAccept);
    }

    if (HasSaveFile() && IsKeyPressed(KEY_C)) {
        LoadRun();
        PlaySoundSafe(sfxUiAccept);
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

    if (cutscene.active) {
        UpdateCutscene(dt);
        return;
    }

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
            PlaySoundSafe(sfxUiAccept);
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
        int blessingWorldSlot = WorldIndex(currentWorld);
        if (blessingWorldSlot >= 0 && blessingWorldSlot < 4 && !blessingMemorySeenByWorld[blessingWorldSlot]) {
            blessingMemorySeen = true;
            blessingMemorySeenByWorld[blessingWorldSlot] = true;
            StartBlessingMemoryCutscene(currentWorld);
            return;
        }
        BuildRewardChoices();
        rewardSelectionOpen = true;
        questBoardOpen = false;
        realmMapOpen = false;
        shop.isOpen = false;
        PlaySoundSafe(sfxReward);
    }

    if (inSafeZone && !rewardSelectionOpen && !realmMapOpen && IsKeyPressed(KEY_Q)) {
        questBoardOpen = !questBoardOpen;
        if (questBoardOpen) {
            shop.isOpen = false;
        }
        PlaySoundSafe(sfxUiMove);
    }

    if (inSafeZone && !rewardSelectionOpen && !questBoardOpen && IsKeyPressed(KEY_R)) {
        realmMapOpen = !realmMapOpen;
        if (realmMapOpen) {
            shop.isOpen = false;
        }
    }

    if (realmMapOpen) {
        if (IsKeyPressed(KEY_ONE)) TravelToWorld(WorldId::Crownheart);
        else if (IsKeyPressed(KEY_TWO)) TravelToWorld(WorldId::Frostveil);
        else if (IsKeyPressed(KEY_THREE)) TravelToWorld(WorldId::Sunscar);
        else if (IsKeyPressed(KEY_FOUR)) TravelToWorld(WorldId::Mirethorn);
    }

    if (questBoardOpen) {
        if (mainQuestIndex >= 0 && mainQuestIndex < (int)mainQuestDB.size() && mainQuestReady && IsKeyPressed(KEY_E)) {
            bool frostWasUnlocked = IsWorldUnlocked(WorldId::Frostveil);
            bool sunWasUnlocked = IsWorldUnlocked(WorldId::Sunscar);
            bool mireWasUnlocked = IsWorldUnlocked(WorldId::Mirethorn);
            euro += mainQuestDB[mainQuestIndex].euroReward;
            mainQuestIndex++;
            mainQuestProgress = 0;
            mainQuestReady = false;
            if (!frostWasUnlocked && IsWorldUnlocked(WorldId::Frostveil)) {
                SaveProfile();
                SaveRun();
                PlaySoundSafe(sfxQuest);
                StartChapterUnlockCutscene(WorldId::Frostveil);
                return;
            }
            else if (!sunWasUnlocked && IsWorldUnlocked(WorldId::Sunscar)) {
                SaveProfile();
                SaveRun();
                PlaySoundSafe(sfxQuest);
                StartChapterUnlockCutscene(WorldId::Sunscar);
                return;
            }
            else if (!mireWasUnlocked && IsWorldUnlocked(WorldId::Mirethorn)) {
                SaveProfile();
                SaveRun();
                PlaySoundSafe(sfxQuest);
                StartChapterUnlockCutscene(WorldId::Mirethorn);
                return;
            }
            else {
                if (mainQuestIndex >= (int)mainQuestDB.size()) {
                    SaveProfile();
                    SaveRun();
                    PlaySoundSafe(sfxQuest);
                    StartFinalCharterCutscene();
                    return;
                }
                announcement = "CHARTER FULFILLED // EURO CLAIMED";
                announcementTimer = 2.2f;
                SaveProfile();
                SaveRun();
                PlaySoundSafe(sfxQuest);
            }
        }

        const int sideKeys[3] = { KEY_ONE, KEY_TWO, KEY_THREE };
        for (int i = 0; i < 3; ++i) {
            if (!IsKeyPressed(sideKeys[i])) {
                continue;
            }

            int defIndex = sideQuestOfferIndices[i];
            if (defIndex < 0 || defIndex >= (int)sideQuestDB.size()) {
                continue;
            }

            const QuestDefinition& quest = sideQuestDB[defIndex];
            if (!sideQuestAccepted[i]) {
                sideQuestAccepted[i] = true;
                sideQuestProgress[i] = 0;
                sideQuestReady[i] = false;
                announcement = quest.title + " // CONTRACT TAKEN";
                announcementTimer = 2.0f;
                SaveProfile();
                PlaySoundSafe(sfxUiAccept);
            }
            else if (sideQuestReady[i]) {
                euro += quest.euroReward;
                announcement = quest.title + " // EURO CLAIMED";
                announcementTimer = 2.0f;
                RefreshSideQuestOffer(i);
                SaveProfile();
                PlaySoundSafe(sfxReward);
            }
        }
    }

    if (inSafeZone && !rewardSelectionOpen && !questBoardOpen && !realmMapOpen && IsKeyPressed(KEY_E)) {
        int desiredShopkeeperStage = 0;
        if (mainQuestIndex >= (int)mainQuestDB.size()) desiredShopkeeperStage = 4;
        else if (mainQuestIndex >= 9) desiredShopkeeperStage = 3;
        else if (mainQuestIndex >= 6) desiredShopkeeperStage = 2;
        else if (mainQuestIndex >= 3) desiredShopkeeperStage = 1;

        if (!shopkeeperCutsceneSeen || shopkeeperStoryStage < desiredShopkeeperStage) {
            shopkeeperCutsceneSeen = true;
            shopkeeperStoryStage = desiredShopkeeperStage;
            StartShopkeeperCutscene(desiredShopkeeperStage);
            return;
        }
        shop.isOpen = !shop.isOpen;
        shop.browseWeaponIdx = player.equippedWeaponIdx;
        if (!petDB.empty()) {
            shop.browsePetIdx = persistentEquippedPetIdx >= 0 ? persistentEquippedPetIdx : shop.browsePetIdx % (int)petDB.size();
        }
        PlaySoundSafe(sfxUiMove);
    }
    if (!inSafeZone) {
        shop.isOpen = false;
        questBoardOpen = false;
        realmMapOpen = false;
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

    if (!inSafeZone && lockedRoomIndex >= 0) {
        int worldSlot = WorldIndex(currentWorld);
        if (worldSlot >= 0 && worldSlot < 4 && !bossIntroSeen[worldSlot]) {
            for (const auto& monster : monsters) {
                if (monster.isBoss) {
                    bossIntroSeen[worldSlot] = true;
                    StartBossIntroCutscene(monster);
                    return;
                }
            }
        }
    }

    Vector2 moveInput = { 0.0f, 0.0f };
    if (!shop.isOpen && !rewardSelectionOpen && !questBoardOpen && !realmMapOpen) {
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

    if (pet.active && pet.petIndex >= 0 && pet.petIndex < (int)petDB.size()) {
        const PetDefinition& petInfo = petDB[pet.petIndex];
        pet.orbitAngle += dt * 2.4f;
        pet.bob += dt * 4.0f;
        Vector2 desired = {
            player.pos.x + std::cos(pet.orbitAngle) * petInfo.orbitRadius,
            player.pos.y - 22.0f + std::sin(pet.bob) * 5.0f
        };
        pet.pos = VecAdd(pet.pos, VecScale(VecSub(desired, pet.pos), ClampFloat(dt * 8.0f, 0.0f, 1.0f)));
        pet.fireTimer -= dt;

        if (!inSafeZone && !monsters.empty() && pet.fireTimer <= 0.0f) {
            int targetIndex = -1;
            float bestDistSq = 250.0f * 250.0f;
            for (int i = 0; i < (int)monsters.size(); ++i) {
                float distSq = DistanceSqr(pet.pos, monsters[i].pos);
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    targetIndex = i;
                }
            }

            if (targetIndex >= 0) {
                ActiveMonster& target = monsters[targetIndex];
                int petDamage = GetPetDamage(pet.petIndex);
                PlaySoundSafe(sfxPet);
                target.hp -= petDamage;
                target.hitFlash = 0.10f;
                beams.push_back({ pet.pos, target.pos, petInfo.color, 2.4f, 0.10f });
                AddFloatingText(target.pos, petInfo.name + " -" + std::to_string(petDamage), petInfo.color);
                EmitBurst(target.pos, 6, 3.0f, petInfo.color, 2.4f);

                if (pet.petIndex == 0) {
                    target.burnTimer = std::max(target.burnTimer, 2.2f);
                    target.burnTickTimer = 0.15f;
                }
                else if (pet.petIndex == 1) {
                    target.slowTimer = std::max(target.slowTimer, 1.5f);
                }
                else if (pet.petIndex == 2) {
                    for (auto& other : monsters) {
                        if (&other == &target || other.hp <= 0) continue;
                        if (Distance(other.pos, target.pos) <= 54.0f) {
                            other.hp -= std::max(5, petDamage / 2);
                            other.hitFlash = 0.08f;
                        }
                    }
                }
                else if (pet.petIndex == 3) {
                    target.poisonTimer = std::max(target.poisonTimer, 3.0f);
                    target.poisonTickTimer = 0.15f;
                }

                pet.fireTimer = GetPetAttackCooldown(pet.petIndex);
            }
        }
    }

    if (!shop.isOpen && !rewardSelectionOpen && !questBoardOpen && !realmMapOpen && IsKeyPressed(KEY_SPACE) && player.attackCd <= 0.0f) {
        const Weapon& weapon = weaponDB[player.equippedWeaponIdx];
        player.attackCd = GetWeaponAttackCooldown(weapon);
        bool hitSomething = false;
        bool landedCrit = false;
        float swingRange = weapon.range + 4.0f * CountRelic(RelicType::RazorPrism);
        if (weapon.name == "Blackrose Pike") {
            swingRange += 10.0f;
        }
        else if (weapon.name == "Sword Without Crown") {
            swingRange += 6.0f;
        }
        else if (weapon.name == "Griefthorn Partisan") {
            swingRange += 12.0f;
        }
        else if (weapon.name == "King's Lament") {
            swingRange += 8.0f;
        }

        PlaySoundSafe((weapon.trait == WeaponTrait::Heavy || weapon.trait == WeaponTrait::Royal) ? sfxSwordHeavy : sfxSwordLight);
        shockwaves.push_back({ player.pos, 12.0f, swingRange, 0.22f, 0.22f, Fade(weapon.color, 0.7f) });
        EmitBurst(player.pos, 16, 4.6f, weapon.color, 4.0f);

        for (auto& monster : monsters) {
            float dist = Distance(player.pos, monster.pos);
            if (dist <= swingRange) {
                int damage = GetWeaponDamageAgainst(weapon, monster);
                float critChance = GetCritChance();
                if (weapon.trait == WeaponTrait::Swift) critChance += 0.02f;
                if (weapon.trait == WeaponTrait::Royal) critChance += 0.03f;
                if (weapon.trait == WeaponTrait::Heavy) critChance -= 0.02f;
                if (weapon.name == "Oathkeeper Blade" && (monster.isElite || monster.isBoss)) critChance += 0.03f;
                if (weapon.name == "Widowglass Thorn" && monster.slowTimer > 0.0f) critChance += 0.05f;
                if (weapon.name == "Sword Without Crown" && player.hp * 2 <= player.maxHp) critChance += 0.08f;
                if (weapon.name == "Chapelglass Estoc" && player.hp >= player.maxHp) critChance += 0.06f;
                if (weapon.name == "Vesper Greatblade" && monster.hp >= (monster.maxHp * 7) / 10) critChance += 0.03f;
                if (weapon.name == "Griefthorn Partisan" && monster.hp <= monster.maxHp / 2) critChance += 0.04f;
                if (weapon.name == "King's Lament" && player.hp * 2 <= player.maxHp) critChance += 0.06f;
                if (critChance < 0.02f) critChance = 0.02f;
                if (critChance > 0.55f) critChance = 0.55f;

                bool isCrit = RandomRange(0.0f, 1.0f) < critChance;
                if (isCrit) {
                    damage = (int)(damage * GetCritMultiplier());
                    landedCrit = true;
                }

                monster.hp -= damage;
                monster.hitFlash = isCrit ? 0.18f : 0.12f;
                AddFloatingText(monster.pos, (isCrit ? "CRIT -" : "-") + std::to_string(damage), isCrit ? neonGold : weapon.color);
                EmitBurst(monster.pos, isCrit ? 16 : 9, isCrit ? 5.5f : 4.2f, isCrit ? neonGold : weapon.color, isCrit ? 4.4f : 3.2f);
                ApplyWeaponHitEffect(weapon, monster, damage, isCrit);
                hitSomething = true;

                if (CountRelic(RelicType::BloodCircuit) > 0 && hitSomething) {
                    player.hp = std::min(player.maxHp, player.hp + 1);
                }

                if (isCrit) {
                    hitStopTimer = std::max(hitStopTimer, 0.045f);
                }
            }
        }

        if (hitSomething) {
            screenShake = std::max(screenShake, 8.0f);
            PlaySoundSafe(landedCrit ? sfxCrit : sfxHit);
        }
    }

    if (!shop.isOpen && !rewardSelectionOpen && !questBoardOpen && !realmMapOpen && IsKeyPressed(KEY_ONE) && player.dashCd <= 0.0f) {
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
        PlaySoundSafe(sfxDash);
    }

    if (!shop.isOpen && !rewardSelectionOpen && !questBoardOpen && !realmMapOpen && IsKeyPressed(KEY_TWO) && player.empCd <= 0.0f) {
        player.empCd = 30.0f;
        int empDamage = GetEmpDamage();
        float empRadius = GetEmpRadius();

        shockwaves.push_back({ player.pos, 20.0f, empRadius, 0.45f, 0.45f, Fade(neonCyan, 0.85f) });
        EmitBurst(player.pos, 36, 7.5f, neonCyan, 5.5f);
        screenShake = std::max(screenShake, 10.0f);
        hitStopTimer = std::max(hitStopTimer, 0.03f);
        PlaySoundSafe(sfxBurst);

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

    if (!shop.isOpen && !rewardSelectionOpen && !questBoardOpen && !realmMapOpen && IsKeyPressed(KEY_THREE) && player.turretCd <= 0.0f) {
        player.turretCd = 12.0f;
        turrets.push_back({ player.pos, GetTurretLifetime(), 0.25f });
        EmitBurst(player.pos, 18, 3.0f, neonBlue, 3.8f);
        AddFloatingText(player.pos, "TOTEM RAISED", neonBlue);
        PlaySoundSafe(sfxBanner);
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
        monster.slowTimer = std::max(0.0f, monster.slowTimer - dt);

        if (monster.burnTimer > 0.0f) {
            monster.burnTimer = std::max(0.0f, monster.burnTimer - dt);
            monster.burnTickTimer -= dt;
            if (monster.burnTickTimer <= 0.0f) {
                int burnDamage = std::max(5, monster.maxHp / 24);
                monster.hp -= burnDamage;
                monster.burnTickTimer = 0.55f;
                AddFloatingText(monster.pos, "BURN " + std::to_string(burnDamage), { 236, 170, 78, 255 });
            }
        }

        if (monster.poisonTimer > 0.0f) {
            monster.poisonTimer = std::max(0.0f, monster.poisonTimer - dt);
            monster.poisonTickTimer -= dt;
            if (monster.poisonTickTimer <= 0.0f) {
                int poisonDamage = std::max(4, monster.maxHp / 28);
                monster.hp -= poisonDamage;
                monster.poisonTickTimer = 0.62f;
                AddFloatingText(monster.pos, "VENOM " + std::to_string(poisonDamage), { 150, 206, 106, 255 });
            }
        }

        Vector2 toPlayer = VecSub(player.pos, monster.pos);
        float dist = VecLength(toPlayer);
        Vector2 dir = VecNormalizeSafe(toPlayer);
        float moveScale = (monster.slowTimer > 0.0f) ? 0.64f : 1.0f;

        bool playerInTargetRoom = (waveTargetRoomIndex >= 0 && waveTargetRoomIndex < (int)dungeon.rooms.size() && currentArea == &dungeon.rooms[waveTargetRoomIndex]);

        if (CheckCollisionPointRec(monster.pos, safeZone)) {
            Vector2 away = VecNormalizeSafe(VecSub(monster.pos, SafeZoneCenter(safeZone)));
            monster.pos = MoveWithCollision(monster.pos, VecScale(away, monster.speed * 80.0f * moveScale * dt), monster.radius);
        }
        else if (!inSafeZone && dist > 6.0f) {
            if (lockedRoomIndex >= 0 || playerInTargetRoom) {
                monster.vel = VecScale(dir, monster.speed * 72.0f * moveScale * dt);
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

    bool triggerBossAftermath = false;
    std::string bossAftermathName;
    Vector2 bossAftermathPos = { 0.0f, 0.0f };
    WorldId bossAftermathWorld = currentWorld;

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
            AwardPetBond(it->isBoss ? 6 : (it->isElite ? 3 : 1));
            AdvanceQuestObjective(QuestObjective::SlayFoes, 1);
            if (it->isElite) {
                AdvanceQuestObjective(QuestObjective::DefeatElites, 1);
            }
            if (it->isBoss) {
                AdvanceQuestObjective(QuestObjective::DefeatBosses, 1);
                int worldSlot = WorldIndex(currentWorld);
                if (worldSlot >= 0 && worldSlot < 4 && !bossAftermathSeen[worldSlot]) {
                    bossAftermathSeen[worldSlot] = true;
                    triggerBossAftermath = true;
                    bossAftermathName = it->name;
                    bossAftermathPos = it->pos;
                    bossAftermathWorld = currentWorld;
                }
            }
            hitStopTimer = std::max(hitStopTimer, it->isBoss ? 0.08f : (it->isElite ? 0.05f : 0.0f));
            screenShake = std::max(screenShake, it->isBoss ? 14.0f : (it->isElite ? 7.0f : 4.0f));
            it = monsters.erase(it);
        }
        else {
            ++it;
        }
    }

    if (triggerBossAftermath) {
        SaveProfile();
        SaveRun();
        StartBossAftermathCutscene(bossAftermathWorld, bossAftermathName, bossAftermathPos);
        return;
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
            AdvanceQuestObjective(QuestObjective::ClearCourts, 1);
            WorldId clearedWorld = currentWorld;
            lockedRoomIndex = -1;
            hitStopTimer = std::max(hitStopTimer, 0.06f);
            GrantRealmSignatureIfNeeded(clearedWorld);
            SaveRun();
        }
    }

    if (shop.isOpen) {
        int hpUpgradeCost = GetHpUpgradeCost();
        const int armoryCols = 8;

        if (IsKeyPressed(KEY_RIGHT)) {
            shop.browseWeaponIdx = (shop.browseWeaponIdx + 1) % (int)weaponDB.size();
        }
        if (IsKeyPressed(KEY_LEFT)) {
            shop.browseWeaponIdx = (shop.browseWeaponIdx - 1 + (int)weaponDB.size()) % (int)weaponDB.size();
        }
        if (IsKeyPressed(KEY_UP)) {
            shop.browseWeaponIdx -= armoryCols;
            if (shop.browseWeaponIdx < 0) {
                shop.browseWeaponIdx = (shop.browseWeaponIdx % (int)weaponDB.size() + (int)weaponDB.size()) % (int)weaponDB.size();
            }
        }
        if (IsKeyPressed(KEY_DOWN)) {
            shop.browseWeaponIdx += armoryCols;
            if (shop.browseWeaponIdx >= (int)weaponDB.size()) {
                shop.browseWeaponIdx %= (int)weaponDB.size();
            }
        }

        WorldId browseWorld = WeaponOriginWorld(shop.browseWeaponIdx);
        bool browseRealmUnlocked = IsWorldUnlocked(browseWorld);
        int browseWorldSlot = WorldIndex(browseWorld);
        bool browseSignatureLocked = WeaponIsRealmSignature(shop.browseWeaponIdx)
            && !shop.ownedWeapons[shop.browseWeaponIdx]
            && !(browseWorldSlot >= 0 && browseWorldSlot < (int)realmSignatureClaimed.size() ? realmSignatureClaimed[browseWorldSlot] : false);

        if (IsKeyPressed(KEY_H)) {
            if (player.hpUpgradeLevel >= 8) {
                shop.message = "VIGOR MASTERED";
                shop.messageColor = neonGold;
                shop.messageTimer = 1.3f;
            }
            else if (player.xp >= hpUpgradeCost) {
                player.xp -= hpUpgradeCost;
                player.hpUpgradeLevel++;
                player.maxHp += 18;
                player.hp = std::min(player.maxHp, player.hp + 18);
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
            else if (!browseRealmUnlocked) {
                shop.message = std::string(WorldLabel(browseWorld)) + " SEALED";
                shop.messageColor = softRed;
                shop.messageTimer = 1.5f;
            }
            else if (browseSignatureLocked) {
                shop.message = "CLEAR A COURT IN THIS REALM";
                shop.messageColor = neonGold;
                shop.messageTimer = 1.7f;
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

        if (!petDB.empty() && IsKeyPressed(KEY_P)) {
            shop.browsePetIdx = (shop.browsePetIdx + 1) % (int)petDB.size();
        }

        if (!petDB.empty() && IsKeyPressed(KEY_N)) {
            const PetDefinition& petInfo = petDB[shop.browsePetIdx];
            if (persistentOwnedPets.size() != petDB.size()) {
                persistentOwnedPets.resize(petDB.size(), false);
            }

            if (persistentOwnedPets[shop.browsePetIdx]) {
                persistentEquippedPetIdx = shop.browsePetIdx;
                SyncPetState(true);
                shop.message = "COMPANION READIED";
                shop.messageColor = petInfo.color;
                shop.messageTimer = 1.4f;
                SaveProfile();
                SaveRun();
            }
            else if (euro >= petInfo.euroCost) {
                euro -= petInfo.euroCost;
                persistentOwnedPets[shop.browsePetIdx] = true;
                persistentEquippedPetIdx = shop.browsePetIdx;
                SyncPetState(true);
                shop.message = "BOND FORGED";
                shop.messageColor = petInfo.color;
                shop.messageTimer = 1.5f;
                SaveProfile();
                SaveRun();
            }
            else {
                shop.message = "NOT ENOUGH EURO";
                shop.messageColor = softRed;
                shop.messageTimer = 1.2f;
            }
        }
    }

    if (player.hp <= 0) {
        persistentOwnedWeapons = shop.ownedWeapons;
        persistentHpUpgradeLevel = player.hpUpgradeLevel;
        persistentEquippedWeaponIdx = player.equippedWeaponIdx;
        legacyRenown = std::max(legacyRenown, (int)std::round((float)player.xp * 0.30f));
        if (persistentPetTrainingLevel < 0) {
            persistentPetTrainingLevel = 0;
        }
        SaveProfile();
        DeleteSave();
        gameState = GameState::GameOver;
        announcement = "FALLEN IN BATTLE";
        announcementTimer = 3.0f;
        PlaySoundSafe(sfxGameOver);
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

    if (cutscene.active && cutscene.lockCamera) {
        camera.target = cutscene.cameraTarget;
    }
    else {
        camera.target = VecAdd(player.pos, VecScale(player.aimDir, 36.0f));
    }
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

        if (cutscene.active) {
            DrawCutsceneOverlay();
        }
        else {
            DrawHud();

            if (rewardSelectionOpen) {
                DrawRewardOverlay();
            }

            if (shop.isOpen) {
                DrawShop();
            }

            if (questBoardOpen) {
                DrawQuestBoard();
            }

            if (realmMapOpen) {
                DrawRealmMap();
            }
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
    float t = (float)GetTime();
    Color skyTop = { 12, 16, 26, 255 };
    Color skyBottom = { 58, 36, 42, 255 };
    Color moon = { 194, 208, 226, 255 };
    Color mist = { 126, 136, 162, 255 };
    Color parchment = { 224, 212, 188, 255 };
    Color iron = { 120, 105, 86, 255 };
    Color sorrowRed = { 154, 78, 86, 255 };

    DrawRectangleGradientV(0, 0, screenW, screenH, skyTop, skyBottom);
    DrawSoftGlow({ screenW * 0.78f, 112.0f }, 44.0f, moon, 0.28f);
    DrawCircleV({ screenW * 0.78f, 112.0f }, 44.0f, moon);

    for (const auto& star : stars) {
        float shimmer = 0.20f + star.size * 0.08f + std::sin(t * 1.8f + star.pos.x * 0.01f) * 0.04f;
        DrawCircleV(star.pos, star.size * 0.9f, Fade(WHITE, ClampFloat(shimmer, 0.10f, 0.36f)));
    }

    for (int i = -120; i < screenW + 120; i += 120) {
        DrawTreeProp({ (float)i + 36.0f, 226.0f + std::sin((float)i * 0.028f) * 10.0f }, 1.65f, { 30, 48, 34, 255 });
        DrawTreeProp({ (float)i + 84.0f, 678.0f + std::cos((float)i * 0.019f) * 10.0f }, 2.05f, { 22, 38, 28, 255 });
    }

    DrawRectangleGradientV(0, 180, screenW, 250, Fade(BLACK, 0.00f), Fade(BLACK, 0.16f));
    DrawCastleProp({ screenW * 0.5f, 264.0f }, 2.18f);
    DrawTowerProp({ screenW * 0.5f - 238.0f, 316.0f }, 1.38f, sorrowRed);
    DrawTowerProp({ screenW * 0.5f + 238.0f, 316.0f }, 1.38f, sorrowRed);
    DrawRectangle(0, 138, screenW, 260, Fade(BLACK, 0.18f));

    for (int i = 0; i < 5; ++i) {
        float drift = std::sin(t * 0.22f + (float)i * 1.4f) * 28.0f;
        DrawEllipse((int)(screenW * (0.12f + i * 0.19f) + drift), 488 + i * 34, 240 + i * 32, 40 + i * 8, Fade(mist, 0.07f - i * 0.008f));
    }

    for (int i = 0; i < 7; ++i) {
        DrawEllipse((int)(i * 220.0f + 110.0f), screenH - 22, 180.0f + (i % 2) * 34.0f, 56.0f + (i % 3) * 10.0f, Fade(BLACK, 0.54f));
    }

    int titleWidth = MeasureText("CROWNHEART", 78);
    DrawText("CROWNHEART", screenW / 2 - titleWidth / 2 + 2, 82, 78, Fade(BLACK, 0.45f));
    DrawText("CROWNHEART", screenW / 2 - titleWidth / 2, 80, 78, parchment);
    DrawText("THE FALLEN KINGDOM", screenW / 2 - MeasureText("THE FALLEN KINGDOM", 28) / 2, 164, 28, sorrowRed);
    DrawText("A tragic knight action RPG in a ruined sacred realm", screenW / 2 - MeasureText("A tragic knight action RPG in a ruined sacred realm", 22) / 2, 198, 22, { 168, 161, 151, 255 });

    Rectangle lorePanel = { screenW / 2.0f - 372.0f, 352.0f, 744.0f, 202.0f };
    DrawPanel(lorePanel, { 20, 22, 28, 232 }, iron);
    DrawText("WHAT WAITS IN THIS CHRONICLE", screenW / 2 - MeasureText("WHAT WAITS IN THIS CHRONICLE", 24) / 2, 376, 24, { 196, 176, 132, 255 });
    DrawText("- Four broken realms threaded by roads, shrines, secrets and sealed courts", screenW / 2 - 314, 420, 20, { 197, 191, 179, 255 });
    DrawText("- Real-time steel, dash, nova burst and guardian totems under siege", screenW / 2 - 314, 448, 20, { 197, 191, 179, 255 });
    DrawText("- Relics, quests, boss hunts, companion pets and escalating night waves", screenW / 2 - 314, 476, 20, { 197, 191, 179, 255 });
    DrawText("- A growing armory of realm-forged weapons with distinct identities", screenW / 2 - 314, 504, 20, { 197, 191, 179, 255 });

    if (loadedSoundCount < 17) {
        Color debugColor = (loadedSoundCount > 0) ? Color{ 126, 168, 126, 255 } : sorrowRed;
        DrawText(audioDebugStatus.c_str(), screenW / 2 - MeasureText(audioDebugStatus.c_str(), 20) / 2, 536, 20, debugColor);
        DrawText("PRESS M TO TEST SOUND", screenW / 2 - MeasureText("PRESS M TO TEST SOUND", 18) / 2, 560, 18, { 206, 178, 118, 255 });
    }

    Color pulse = ((int)(GetTime() * 2.3) % 2 == 0) ? Color{ 228, 220, 202, 255 } : Color{ 196, 142, 132, 255 };
    DrawText("PRESS ENTER TO BEGIN A NEW CHRONICLE", screenW / 2 - MeasureText("PRESS ENTER TO BEGIN A NEW CHRONICLE", 28) / 2, 588, 28, pulse);
    DrawText("YOUR PERMANENT UNLOCKS REMAIN, BUT EACH CHRONICLE BEGINS AS A LOWLY KNIGHT", screenW / 2 - MeasureText("YOUR PERMANENT UNLOCKS REMAIN, BUT EACH CHRONICLE BEGINS AS A LOWLY KNIGHT", 17) / 2, 620, 17, { 164, 156, 148, 255 });

    if (HasSaveFile()) {
        DrawText("PRESS C TO RESUME THE CURRENT CHRONICLE", screenW / 2 - MeasureText("PRESS C TO RESUME THE CURRENT CHRONICLE", 22) / 2, 648, 22, { 206, 178, 118, 255 });
    }

    DrawText("PRESS F TO BURN THE OLD CHRONICLE", screenW / 2 - MeasureText("PRESS F TO BURN THE OLD CHRONICLE", 20) / 2, 678, 20, sorrowRed);
    DrawText("ESC closes the game", screenW / 2 - MeasureText("ESC closes the game", 16) / 2, 704, 16, { 132, 129, 124, 255 });

    if (loadedSoundCount <= 0) {
        DrawText("SFX FILES NOT FOUND // RUN build_sound_assets.py", screenW / 2 - MeasureText("SFX FILES NOT FOUND // RUN build_sound_assets.py", 18) / 2, 730, 18, sorrowRed);
    }

    DrawHorizontalFade(0, 0, screenW / 5, screenH, BLACK, 0.34f, 0.0f);
    DrawHorizontalFade(screenW - screenW / 5, 0, screenW / 5, screenH, BLACK, 0.0f, 0.34f);
    DrawRectangleGradientV(0, 0, screenW, screenH / 4, Fade(BLACK, 0.22f), Fade(BLACK, 0.0f));
    DrawRectangleGradientV(0, screenH - screenH / 3, screenW, screenH / 3, Fade(BLACK, 0.0f), Fade(BLACK, 0.24f));
}

void Game::DrawWorld() const {
    BeginMode2D(camera);

    Color grassTint = WorldGrassTint(currentWorld);
    Color roadTint = WorldRoadTint(currentWorld);
    Color accentTint = WorldAccentTint(currentWorld);
    Color shadowTint = WorldShadowTint(currentWorld);
    Color fogTint = WorldFogTint(currentWorld);
    Color glowTint = WorldGlowTint(currentWorld);
    Color propTint = WorldPropTint(currentWorld);
    Color labelTint = WorldRoomLabelTint(currentWorld);
    float fogStrength = WorldFogStrength(currentWorld);
    float ambientMotion = WorldAmbientMotion(currentWorld);
    float worldTime = (float)GetTime();
    int worldSeed = WorldIndex(currentWorld) * 97 + 11;

    auto drawProp = [&](const PropInstance& prop) {
        if (propAtlas.id == 0) {
            return;
        }

        float cell = 96.0f;
        Rectangle src = PropSourceRect(prop.spriteIndex);
        Rectangle dst = { prop.pos.x - (cell * prop.scale) * 0.5f, prop.pos.y - (cell * prop.scale) * 0.78f, cell * prop.scale, cell * prop.scale };
        DrawEllipse((int)prop.pos.x, (int)(prop.pos.y + 18.0f * prop.scale), 18.0f * prop.scale, 7.0f * prop.scale, Fade(BLACK, 0.22f));
        DrawEllipse((int)prop.pos.x, (int)(prop.pos.y + 15.0f * prop.scale), 28.0f * prop.scale, 9.0f * prop.scale, Fade(shadowTint, 0.10f));
        DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, propTint);
        };

    auto drawFenceStrip = [&](bool horizontal, float fixed, float startSpan, float endSpan) {
        if (endSpan - startSpan < 24.0f) {
            return;
        }

        Color wallShade = MixColor(roadTint, shadowTint, 0.45f);
        if (horizontal) {
            DrawLineEx({ startSpan, fixed + 7.0f }, { endSpan, fixed + 7.0f }, 9.0f, Fade(BLACK, 0.15f));
            DrawLineEx({ startSpan, fixed + 5.0f }, { endSpan, fixed + 5.0f }, 7.0f, Fade(wallShade, 0.26f));
        }
        else {
            DrawLineEx({ fixed + 7.0f, startSpan }, { fixed + 7.0f, endSpan }, 9.0f, Fade(BLACK, 0.15f));
            DrawLineEx({ fixed + 5.0f, startSpan }, { fixed + 5.0f, endSpan }, 7.0f, Fade(wallShade, 0.26f));
        }

        if (propAtlas.id == 0) {
            if (horizontal) DrawLineEx({ startSpan, fixed }, { endSpan, fixed }, 4.0f, wallShade);
            else DrawLineEx({ fixed, startSpan }, { fixed, endSpan }, 4.0f, wallShade);
            return;
        }

        Rectangle src = PropSourceRect(horizontal ? 7 : 8);
        for (float p = startSpan; p < endSpan; p += 88.0f) {
            float length = std::min(88.0f, endSpan - p);
            Rectangle dst = horizontal
                ? Rectangle{ p, fixed - 48.0f, length, 96.0f }
            : Rectangle{ fixed - 48.0f, p, 96.0f, length };
            DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, propTint);
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
            int tileIndex = TileAt(tileMap, x, y);
            Rectangle src = TileSourceRect(tileIndex);
            int hash = MoodHash(x, y, worldSeed);
            Color baseTint = grassTint;
            if (tileIndex == (int)TileType::Path) {
                baseTint = roadTint;
            }
            else if (tileIndex == (int)TileType::GrassAlt) {
                baseTint = MixColor(grassTint, accentTint, 0.16f);
            }
            else if (tileIndex == (int)TileType::Flowers) {
                baseTint = MixColor(grassTint, glowTint, 0.22f);
            }

            float shade = 0.88f + (float)(hash & 7) * 0.022f;
            if (tileIndex == (int)TileType::Path && ((hash >> 4) & 1) == 0) {
                shade -= 0.04f;
            }
            Color tint = ScaleColor(baseTint, shade);

            if (tileAtlas.id != 0) {
                DrawTexturePro(tileAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, tint);
            }
            else {
                DrawRectangleRec(dst, tint);
            }

            DrawRectangleRec(dst, Fade(shadowTint, tileIndex == (int)TileType::Path ? 0.035f : 0.055f));

            if (tileIndex == (int)TileType::Path && (hash % 6) == 0) {
                DrawLineEx({ dst.x + 10.0f, dst.y + 18.0f }, { dst.x + dst.width - 12.0f, dst.y + dst.height - 15.0f }, 1.6f, Fade(BLACK, 0.08f));
            }
        }
    }

    int fogMinX = minTileX / 4 - 1;
    int fogMaxX = maxTileX / 4 + 1;
    int fogMinY = minTileY / 3 - 1;
    int fogMaxY = maxTileY / 3 + 1;
    for (int gy = fogMinY; gy <= fogMaxY; ++gy) {
        for (int gx = fogMinX; gx <= fogMaxX; ++gx) {
            int seed = MoodHash(gx, gy, worldSeed + 4000);
            float baseX = tileMap.originX + gx * tileMap.tileSize * 4.0f + 96.0f + (float)(seed & 31) * 4.0f;
            float baseY = tileMap.originY + gy * tileMap.tileSize * 3.0f + 72.0f + (float)((seed >> 5) & 31) * 4.0f;
            float phase = worldTime * ambientMotion * (0.18f + 0.02f * (float)(seed & 3)) + (float)(seed & 255) * 0.03f;
            float driftX = std::sin(phase) * (18.0f + (float)(seed & 15));
            float driftY = std::cos(phase * 0.83f) * (10.0f + (float)((seed >> 8) & 7));
            float radiusX = 48.0f + (float)((seed >> 11) & 31) * 2.0f;
            float radiusY = 18.0f + (float)((seed >> 16) & 15) * 1.25f;
            float alpha = fogStrength * (0.13f + (float)((seed >> 20) & 7) * 0.01f);
            DrawEllipse((int)(baseX + driftX), (int)(baseY + driftY), radiusX, radiusY, Fade(fogTint, alpha));

            if ((seed & 7) == 0) {
                Vector2 motePos = {
                    baseX + std::cos(phase * 1.28f) * (18.0f + (float)((seed >> 9) & 15)),
                    baseY + std::sin(phase * 1.14f) * (12.0f + (float)((seed >> 13) & 11))
                };
                DrawCircleV(motePos, 1.6f + (float)((seed >> 18) & 3) * 0.4f, Fade(glowTint, 0.12f));
            }
        }
    }

    for (const auto& corridor : dungeon.corridors) {
        DrawRectangleRec(corridor, Fade(shadowTint, 0.06f));
        DrawLineEx({ corridor.x, corridor.y + corridor.height }, { corridor.x + corridor.width, corridor.y + corridor.height }, 8.0f, Fade(BLACK, 0.12f));
        DrawRectangleLinesEx(corridor, 2.0f, Fade(accentTint, 0.14f));
    }

    for (const auto& prop : decorProps) {
        drawProp(prop);
    }

    for (const auto& room : dungeon.rooms) {
        DrawRectangleRec(room.rect, Fade(room.isSafeZone ? glowTint : shadowTint, room.isSafeZone ? 0.05f : 0.085f));
        DrawLineEx({ room.rect.x, room.rect.y + room.rect.height }, { room.rect.x + room.rect.width, room.rect.y + room.rect.height }, 8.0f, Fade(BLACK, 0.14f));
        DrawRectangleLinesEx(room.rect, 2.0f, Fade(accentTint, 0.16f));
        drawRoomEdgeFences(room);
        int labelSize = room.isSafeZone ? 24 : 20;
        int labelX = (int)(room.rect.x + room.rect.width * 0.5f) - MeasureText(room.name.c_str(), labelSize) / 2;
        int labelY = (int)room.rect.y + 14;
        Color roomLabel = room.isSafeZone ? MixColor(labelTint, glowTint, 0.20f) : labelTint;
        DrawText(room.name.c_str(), labelX + 2, labelY + 2, labelSize, Fade(BLACK, 0.42f));
        DrawText(room.name.c_str(), labelX, labelY, labelSize, roomLabel);
    }

    std::vector<PropInstance> sortedProps = worldProps;
    std::sort(sortedProps.begin(), sortedProps.end(), [](const PropInstance& a, const PropInstance& b) {
        return a.pos.y < b.pos.y;
        });
    for (const auto& prop : sortedProps) {
        drawProp(prop);
    }

    for (const auto& barrier : GetActiveBarrierRects()) {
        DrawRectangleRounded(barrier, 0.12f, 4, Fade(shadowTint, 0.20f));
        if (barrier.width > barrier.height) drawFenceStrip(true, barrier.y + barrier.height * 0.5f, barrier.x, barrier.x + barrier.width);
        else drawFenceStrip(false, barrier.x + barrier.width * 0.5f, barrier.y, barrier.y + barrier.height);
    }

    if (rewardChestActive) {
        DrawSoftGlow({ rewardChestPos.x, rewardChestPos.y - 8.0f }, 28.0f, glowTint, 0.22f);
        if (propAtlas.id != 0) {
            Rectangle src = PropSourceRect(6);
            Rectangle dst = { rewardChestPos.x - 48.0f, rewardChestPos.y - 60.0f, 96.0f, 96.0f };
            DrawTexturePro(propAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, propTint);
        }
    }

    for (const auto& shockwave : shockwaves) {
        float alpha = ClampFloat(shockwave.life / shockwave.maxLife, 0.0f, 1.0f);
        DrawCircleLines((int)shockwave.pos.x, (int)shockwave.pos.y, shockwave.radius, Fade(glowTint, alpha * 0.30f));
        DrawCircleLines((int)shockwave.pos.x, (int)shockwave.pos.y, shockwave.radius - 4.0f, Fade({ 255, 255, 255, 255 }, alpha * 0.20f));
    }

    for (const auto& turret : turrets) {
        if (actorAtlas.id != 0) {
            Rectangle src = ActorSourceRect(6);
            Rectangle dst = { turret.pos.x - 34.0f, turret.pos.y - 46.0f, 68.0f, 68.0f };
            DrawTexturePro(actorAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
        DrawCircleLines((int)turret.pos.x, (int)turret.pos.y, 260.0f, Fade(accentTint, 0.08f));
    }

    for (const auto& beam : beams) {
        DrawLineEx(beam.start, beam.end, beam.thickness + 2.0f, Fade({ 255, 246, 204, 255 }, beam.life * 2.8f));
        DrawLineEx(beam.start, beam.end, beam.thickness, ScaleColor(accentTint, 1.10f));
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
        DrawRectangleRec(back, Fade(BLACK, 0.42f));
        DrawRectangleRec({ back.x, back.y, back.width * ((float)monster.hp / (float)monster.maxHp), back.height }, monster.isBoss ? neonGold : safeGreen);
    }

    for (const auto& particle : particles) {
        float alpha = ClampFloat(particle.life / particle.maxLife, 0.0f, 1.0f);
        DrawCircleV(particle.pos, particle.size, Fade(particle.color, alpha));
    }

    DrawPetSprite();

    if (actorAtlas.id != 0) {
        Rectangle src = ActorSourceRect(0);
        Rectangle dst = { player.pos.x - 38.0f, player.pos.y - 52.0f, 76.0f, 76.0f };
        DrawTexturePro(actorAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
    DrawPlayerWeapon();
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, weaponDB[player.equippedWeaponIdx].range, Fade(accentTint, 0.10f));

    for (const auto& text : floatingTexts) {
        DrawText(text.text.c_str(), (int)text.pos.x, (int)text.pos.y, 18, Fade(text.color, text.life));
    }

    EndMode2D();

    DrawRectangleGradientV(0, 0, screenW, screenH / 3, Fade(WorldSkyTopTint(currentWorld), 0.20f), Fade(BLACK, 0.0f));
    DrawSoftGlow({ screenW * 0.82f, 92.0f }, 40.0f, glowTint, 0.10f);
    DrawRectangleGradientV(0, screenH - screenH / 3, screenW, screenH / 3, Fade(BLACK, 0.0f), Fade(WorldSkyBottomTint(currentWorld), 0.18f));
    DrawHorizontalFade(0, 0, screenW / 5, screenH, BLACK, 0.30f, 0.0f);
    DrawHorizontalFade(screenW - screenW / 5, 0, screenW / 5, screenH, BLACK, 0.0f, 0.28f);
    DrawRectangleGradientV(0, 0, screenW, screenH / 5, Fade(BLACK, 0.16f), Fade(BLACK, 0.0f));
}

void Game::DrawCutsceneOverlay() const {
    float barHeight = std::max(42.0f, cutscene.letterbox);
    Color parchment = { 224, 216, 202, 255 };
    Color ashText = { 182, 176, 166, 255 };

    DrawRectangle(0, 0, screenW, (int)barHeight, Fade(BLACK, 0.94f));
    DrawRectangle(0, screenH - (int)barHeight, screenW, (int)barHeight, Fade(BLACK, 0.94f));
    DrawRectangle(0, (int)barHeight, screenW, 2, Fade(neonGold, 0.18f));
    DrawRectangle(0, screenH - (int)barHeight - 2, screenW, 2, Fade(neonGold, 0.18f));

    if (!cutscene.sceneName.empty()) {
        int titleW = MeasureText(cutscene.sceneName.c_str(), 20);
        Rectangle titleRect = { screenW * 0.5f - titleW * 0.5f - 18.0f, 10.0f, (float)titleW + 36.0f, 30.0f };
        DrawPanel(titleRect, panel2, neonGold);
        DrawText(cutscene.sceneName.c_str(), (int)titleRect.x + 18, (int)titleRect.y + 7, 20, neonGold);
    }

    if (cutscene.sceneTitleTimer > 0.0f && cutscene.sceneTitleMax > 0.0f && !cutscene.sceneName.empty()) {
        std::string titleText = cutscene.sceneName;
        std::string overline = "CHRONICLE";
        Color titleColor = cutscene.speakerColor;

        size_t sep = titleText.find("//");
        if (sep != std::string::npos) {
            overline = titleText.substr(0, sep - 1);
            titleText = titleText.substr(sep + 3);
        }

        float alpha = ClampFloat(cutscene.sceneTitleTimer / cutscene.sceneTitleMax, 0.0f, 1.0f);
        Rectangle card = { screenW * 0.5f - 300.0f, screenH * 0.34f - 48.0f, 600.0f, 96.0f };
        DrawPanel(card, Fade(panel2, 0.96f), titleColor);
        DrawSoftGlow({ card.x + card.width * 0.5f, card.y + card.height * 0.5f }, 44.0f, titleColor, 0.12f * alpha);
        DrawText(overline.c_str(), (int)card.x + 24, (int)card.y + 18, 18, Fade(ashText, alpha));
        int bigSize = titleText.size() > 18 ? 28 : 34;
        DrawText(titleText.c_str(), (int)(card.x + card.width * 0.5f) - MeasureText(titleText.c_str(), bigSize) / 2, (int)card.y + 42, bigSize, Fade(parchment, alpha));
    }

    DrawText("F SKIP", screenW - 92, 16, 18, ashText);

    if (!cutscene.text.empty()) {
        Rectangle dialogueRect = { screenW * 0.5f - 470.0f, screenH - 190.0f, 940.0f, 132.0f };
        DrawPanel(dialogueRect, panel2, cutscene.speakerColor);
        DrawText(cutscene.speaker.c_str(), (int)dialogueRect.x + 22, (int)dialogueRect.y + 14, 22, cutscene.speakerColor);
        DrawRectangle((int)dialogueRect.x + 20, (int)dialogueRect.y + 42, (int)dialogueRect.width - 40, 1, Fade(cutscene.speakerColor, 0.28f));
        DrawText(cutscene.text.c_str(), (int)dialogueRect.x + 22, (int)dialogueRect.y + 54, 21, parchment);

        if (cutscene.awaitingAdvance) {
            Color pulse = ((int)(GetTime() * 2.4f) % 2 == 0) ? neonGold : ashText;
            DrawText("SPACE / ENTER", (int)dialogueRect.x + (int)dialogueRect.width - 176, (int)dialogueRect.y + 96, 18, pulse);
        }

        if (cutscene.showPortrait) {
            Rectangle portraitRect = { dialogueRect.x + dialogueRect.width - 168.0f, dialogueRect.y - 128.0f, 146.0f, 112.0f };
            DrawPanel(portraitRect, panel, cutscene.speakerColor);
            DrawSoftGlow({ portraitRect.x + portraitRect.width * 0.5f, portraitRect.y + 52.0f }, 18.0f, cutscene.speakerColor, 0.18f);
            DrawCircleV({ portraitRect.x + portraitRect.width * 0.5f, portraitRect.y + 52.0f }, 18.0f, cutscene.speakerColor);
            DrawCircleLines((int)(portraitRect.x + portraitRect.width * 0.5f), (int)(portraitRect.y + 52.0f), 30.0f, Fade(parchment, 0.35f));
            std::string sigil = cutscene.speaker.empty() ? "?" : std::string(1, cutscene.speaker[0]);
            DrawText(sigil.c_str(), (int)(portraitRect.x + portraitRect.width * 0.5f) - MeasureText(sigil.c_str(), 28) / 2, (int)portraitRect.y + 38, 28, BLACK);
            DrawText("MEMORY", (int)portraitRect.x + 34, (int)portraitRect.y + 82, 16, parchment);
        }
    }

    if (cutscene.fadeAlpha > 0.001f) {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, cutscene.fadeAlpha));
    }
}

void Game::DrawHud() const {
    Color parchment = { 216, 208, 194, 255 };
    Color ashText = { 166, 160, 150, 255 };
    Color dimText = { 126, 122, 118, 255 };
    Color wardColor = { 118, 158, 122, 255 };
    Color lowHealthColor = { 174, 82, 78, 255 };

    Rectangle vigilRect = { 18.0f, 18.0f, 420.0f, 190.0f };
    DrawPanel(vigilRect, panel, neonBlue);
    DrawText("KNIGHT'S VIGIL", 34, 28, 20, neonGold);
    DrawText(TextFormat("ARMAMENT  %s", weaponDB[player.equippedWeaponIdx].name.c_str()), 34, 56, 18, parchment);
    DrawText(TextFormat("DISCIPLINE  %s", WeaponTraitLabel(weaponDB[player.equippedWeaponIdx].trait)), 34, 78, 16, weaponDB[player.equippedWeaponIdx].color);
    DrawText(TextFormat("WAVE %d", player.wave), 300, 28, 20, neonGold);
    DrawText(TextFormat("SLAIN %d", player.kills), 300, 54, 18, parchment);
    DrawText(TextFormat("RELICS %d", player.relicsCollected), 300, 78, 18, neonPink);

    DrawRectangleRounded({ 34.0f, 104.0f, 250.0f, 18.0f }, 0.22f, 6, Fade(BLACK, 0.36f));
    DrawRectangleRounded({ 34.0f, 104.0f, 250.0f * ((float)player.hp / (float)player.maxHp), 18.0f }, 0.22f, 6, player.hp < player.maxHp * 0.3f ? lowHealthColor : wardColor);
    DrawText(TextFormat("VIGOR %d / %d", player.hp, player.maxHp), 44, 105, 16, parchment);

    DrawRectangleRounded({ 34.0f, 134.0f, 250.0f, 12.0f }, 0.24f, 6, Fade(BLACK, 0.32f));
    DrawRectangleRounded({ 34.0f, 134.0f, ClampFloat((float)player.xp / 3000.0f * 250.0f, 0.0f, 250.0f), 12.0f }, 0.24f, 6, neonGold);
    DrawText(TextFormat("RENOWN %d", player.xp), 294, 128, 16, neonGold);
    DrawText(TextFormat("EURO %d", euro), 34, 158, 18, wardColor);
    DrawText(TextFormat("HEIRLOOM %d", legacyRenown), 172, 158, 18, { 142, 118, 82, 255 });

    Rectangle artsRect = { 18.0f, 212.0f, 300.0f, 118.0f };
    DrawPanel(artsRect, panel, neonPink);
    DrawText("BATTLE ARTS", 32, 224, 18, neonPink);
    DrawText(TextFormat("1 DASH    %.1fs", player.dashCd), 32, 250, 18, player.dashCd <= 0.0f ? neonCyan : dimText);
    DrawText(TextFormat("2 NOVA    %.1fs", player.empCd), 32, 274, 18, player.empCd <= 0.0f ? neonCyan : dimText);
    DrawText(TextFormat("3 TOTEM   %.1fs", player.turretCd), 32, 298, 18, player.turretCd <= 0.0f ? neonCyan : dimText);

    Rectangle ledgerRect = { 18.0f, 342.0f, 420.0f, 188.0f };
    DrawPanel(ledgerRect, panel, neonGold);
    DrawText("WAR LEDGER", 34, 354, 20, neonGold);
    if (mainQuestIndex >= 0 && mainQuestIndex < (int)mainQuestDB.size()) {
        const QuestDefinition& mainQuest = mainQuestDB[mainQuestIndex];
        WorldId questWorld = MainQuestWorld(mainQuestIndex);
        DrawText(TextFormat("MAIN VOW // %s", WorldLabel(questWorld)), 34, 378, 16, neonBlue);
        DrawText(mainQuest.title.c_str(), 34, 398, 18, parchment);
        DrawText(mainQuest.description.c_str(), 34, 420, 16, ashText);
        DrawRectangleRounded({ 34.0f, 446.0f, 220.0f, 10.0f }, 0.25f, 6, Fade(BLACK, 0.32f));
        DrawRectangleRounded({ 34.0f, 446.0f, 220.0f * (float)mainQuestProgress / (float)std::max(1, mainQuest.target), 10.0f }, 0.25f, 6, mainQuestReady ? safeGreen : neonGold);
        DrawText(TextFormat("%s %d / %d %s", QuestObjectiveVerb(mainQuest.objective), mainQuestProgress, mainQuest.target, QuestObjectiveLabel(mainQuest.objective)), 34, 462, 16, mainQuestReady ? safeGreen : neonGold);
        DrawText(mainQuestReady ? TextFormat("CLAIM %d EURO AT THE BOARD", mainQuest.euroReward) : TextFormat("REWARD %d EURO", mainQuest.euroReward), 34, 478, 16, mainQuestReady ? safeGreen : parchment);
    }
    else {
        DrawText("ALL V2 CHAPTERS STAND COMPLETED.", 34, 392, 18, parchment);
        DrawText("Roam the broken realms and forge the rest of the legend.", 34, 416, 16, ashText);
    }

    int sideRowY = 486;
    bool hasSideQuest = false;
    for (int i = 0; i < 3; ++i) {
        if (!sideQuestAccepted[i]) {
            continue;
        }
        hasSideQuest = true;
        const QuestDefinition& sideQuest = sideQuestDB[sideQuestOfferIndices[i]];
        DrawText(TextFormat("SIDE VOW // %s  %d/%d", sideQuest.title.c_str(), sideQuestProgress[i], sideQuest.target), 34, sideRowY, 16, sideQuestReady[i] ? safeGreen : neonCyan);
        sideRowY += 18;
    }
    if (!hasSideQuest) {
        DrawText("No side vow is sworn. Visit the board in a safe court.", 34, 504, 16, ashText);
    }

    Rectangle reportRect = { screenW - 352.0f, 18.0f, 334.0f, 190.0f };
    DrawPanel(reportRect, panel, safeGreen);
    DrawText("FIELD REPORT", screenW - 334, 28, 20, safeGreen);
    DrawText(TextFormat("REALM  %s", WorldLabel(currentWorld)), screenW - 334, 54, 18, parchment);
    DrawText(TextFormat("FOES %d", (int)monsters.size()), screenW - 334, 78, 18, parchment);
    DrawText(TextFormat("TOTEMS %d", (int)turrets.size()), screenW - 334, 102, 18, parchment);
    DrawText(TextFormat("COMPANION  %s", (pet.active && pet.petIndex >= 0 && pet.petIndex < (int)petDB.size()) ? petDB[pet.petIndex].name.c_str() : "NONE"), screenW - 334, 126, 16, pet.active ? petDB[pet.petIndex].color : ashText);
    DrawText(TextFormat("BOND RANK %d", pet.active ? GetPetBondLevel(pet.petIndex) : 0), screenW - 334, 144, 16, pet.active ? petDB[pet.petIndex].color : ashText);

    bool inSafeZone = CheckCollisionPointRec(player.pos, safeZone);
    const DungeonArea* currentArea = GetCurrentArea(player.pos);
    DrawText(inSafeZone ? "COURT STATUS  SANCTUARY" : "COURT STATUS  BESIEGED", screenW - 334, 160, 16, inSafeZone ? safeGreen : softRed);
    DrawText(TextFormat("GROUND  %s", currentArea ? currentArea->name.c_str() : "STONE ROAD"), screenW - 334, 178, 14, ashText);

    DrawMiniMap();

    std::string footerText;
    Color footerBorder = neonBlue;
    if (rewardChestActive && !rewardSelectionOpen && Distance(player.pos, rewardChestPos) < 72.0f) {
        footerText = "RELIQUARY UNSEALED // PRESS E TO CLAIM A BLESSING";
        footerBorder = neonGold;
    }
    else if (realmMapOpen) {
        footerText = "GATE MAP OPEN // 1 CROWNHEART // 2 FROSTVEIL // 3 SUNSCAR // 4 MIRETHORN // R CLOSE";
        footerBorder = neonBlue;
    }
    else if (questBoardOpen) {
        footerText = "OATHBOARD OPEN // E CLAIM MAIN VOW // 1-3 ACCEPT OR TURN IN SIDE VOWS // Q CLOSE";
        footerBorder = neonGold;
    }
    else if (inSafeZone) {
        int desiredShopkeeperStage = 0;
        if (mainQuestIndex >= (int)mainQuestDB.size()) desiredShopkeeperStage = 4;
        else if (mainQuestIndex >= 9) desiredShopkeeperStage = 3;
        else if (mainQuestIndex >= 6) desiredShopkeeperStage = 2;
        else if (mainQuestIndex >= 3) desiredShopkeeperStage = 1;
        bool talkReady = !shopkeeperCutsceneSeen || shopkeeperStoryStage < desiredShopkeeperStage;
        footerText = talkReady ? "SAFE COURT // E SPEAK WITH MAERWYN // Q OATHBOARD // R GATE OF REALMS" : "SAFE COURT // E BLACK HALL ARMORY // Q OATHBOARD // R GATE OF REALMS";
        footerBorder = safeGreen;
    }
    else {
        footerText = "WASD MOVE   SPACE SWING   1 DASH   2 NOVA   3 TOTEM   HOLD THE ROADS AND CLEANSE THE COURTS";
        footerBorder = neonBlue;
    }

    Rectangle footerRect = { 18.0f, screenH - 50.0f, screenW - 36.0f, 34.0f };
    DrawPanel(footerRect, panel, footerBorder);
    DrawText(footerText.c_str(), (int)footerRect.x + 14, (int)footerRect.y + 9, 17, parchment);

    if (announcementTimer > 0.0f) {
        int fontSize = 28;
        int width = MeasureText(announcement.c_str(), fontSize);
        Rectangle announceRect = { screenW * 0.5f - width * 0.5f - 22.0f, 16.0f, (float)width + 44.0f, 42.0f };
        DrawPanel(announceRect, panel2, neonGold);
        DrawText(announcement.c_str(), screenW / 2 - width / 2, 24, fontSize, neonGold);
    }
}

void Game::DrawMiniMap() const {
    Color accentTint = WorldAccentTint(currentWorld);
    Color shadowTint = WorldShadowTint(currentWorld);
    Rectangle panelRect = { screenW - 292.0f, 198.0f, 274.0f, 274.0f };
    DrawPanel(panelRect, panel, accentTint);
    DrawText("WAR MAP", (int)panelRect.x + 20, (int)panelRect.y + 16, 18, accentTint);
    DrawText(WorldLabel(currentWorld), (int)panelRect.x + 108, (int)panelRect.y + 16, 14, { 182, 176, 166, 255 });

    Rectangle mapRect = { panelRect.x + 16.0f, panelRect.y + 42.0f, panelRect.width - 32.0f, panelRect.height - 60.0f };
    DrawRectangleRec(mapRect, Fade(shadowTint, 0.72f));
    DrawRectangleLinesEx(mapRect, 1.0f, Fade(accentTint, 0.32f));

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
        DrawRectangleRec(mini, Fade(accentTint, 0.12f));
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
        Color fill = room.isSafeZone ? Fade(safeGreen, 0.22f) : Fade(accentTint, 0.10f);
        Color border = room.isSafeZone ? safeGreen : Fade({ 196, 192, 186, 255 }, 0.30f);
        if (i == waveTargetRoomIndex && !rewardChestActive) {
            border = neonGold;
            fill = Fade(neonGold, 0.08f);
        }
        if (&room == currentArea) {
            border = neonCyan;
            fill = Fade(neonCyan, 0.14f);
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
            DrawRectangleRec(mini, Fade(obstacle.color, 0.14f));
        }
        else {
            for (const auto& collider : obstacle.colliders) {
                Rectangle mini = {
                    mapRect.x + (collider.x - minX) * scale,
                    mapRect.y + (collider.y - minY) * scale,
                    collider.width * scale,
                    collider.height * scale
                };
                DrawRectangleRec(mini, Fade(obstacle.color, 0.14f));
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
        DrawCircleLines((int)chestMini.x, (int)chestMini.y, 7.0f, { 228, 220, 206, 255 });
    }

    Vector2 playerMini = WorldToMap(player.pos);
    DrawCircleV(playerMini, 4.8f, { 228, 220, 206, 255 });
    DrawCircleV(playerMini, 3.1f, neonCyan);
}

void Game::DrawRewardOverlay() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.74f));

    Rectangle panelRect = { screenW * 0.5f - 470.0f, screenH * 0.5f - 210.0f, 940.0f, 420.0f };
    DrawPanel(panelRect, panel2, neonGold);
    DrawText("RELIQUARY UNSEALED", (int)panelRect.x + 28, (int)panelRect.y + 22, 30, neonGold);
    DrawText("Choose one blessing to bear deeper into the siege.", (int)panelRect.x + 30, (int)panelRect.y + 58, 18, { 202, 196, 186, 255 });

    for (int i = 0; i < (int)rewardChoices.size(); ++i) {
        Rectangle card = { panelRect.x + 28.0f + i * 295.0f, panelRect.y + 108.0f, 265.0f, 250.0f };
        const RelicChoice& relic = rewardChoices[i];

        DrawPanel(card, panel, relic.color);
        DrawRectangleGradientV((int)card.x + 10, (int)card.y + 44, (int)card.width - 20, 70, Fade(relic.color, 0.18f), Fade(BLACK, 0.02f));
        DrawText(TextFormat("BLESSING %d", i + 1), (int)card.x + 18, (int)card.y + 14, 16, relic.color);
        DrawCircleV({ card.x + card.width * 0.5f, card.y + 78.0f }, 26.0f, relic.color);
        DrawCircleV({ card.x + card.width * 0.5f, card.y + 78.0f }, 12.0f, { 238, 232, 222, 255 });
        DrawText(relic.name.c_str(), (int)card.x + 18, (int)card.y + 128, 24, { 224, 216, 202, 255 });
        DrawText(relic.description.c_str(), (int)card.x + 18, (int)card.y + 168, 19, { 184, 178, 170, 255 });
        DrawText(TextFormat("PRESS %d TO CLAIM", i + 1), (int)card.x + 18, (int)card.y + 212, 20, relic.color);
    }
}

void Game::DrawShop() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.70f));

    Color parchment = { 220, 212, 198, 255 };
    Color ashText = { 176, 170, 160, 255 };
    Rectangle panelRect = { screenW * 0.5f - 580.0f, screenH * 0.5f - 320.0f, 1160.0f, 640.0f };
    DrawPanel(panelRect, panel2, neonGold);

    int collectedWeapons = 0;
    for (bool owned : shop.ownedWeapons) {
        if (owned) {
            collectedWeapons++;
        }
    }

    int collectedPets = 0;
    for (size_t i = 0; i < persistentOwnedPets.size(); ++i) {
        if (persistentOwnedPets[i]) {
            collectedPets++;
        }
    }

    DrawText("BLACK HALL ARMORY", (int)panelRect.x + 24, (int)panelRect.y + 22, 28, neonGold);
    DrawText("Forge, blessing and companion service for the siege ahead.", (int)panelRect.x + 24, (int)panelRect.y + 52, 17, ashText);
    DrawText(TextFormat("RENOWN %d", player.xp), (int)panelRect.x + 764, (int)panelRect.y + 24, 20, safeGreen);
    DrawText(TextFormat("EURO %d", euro), (int)panelRect.x + 930, (int)panelRect.y + 24, 20, neonGold);
    DrawText(TextFormat("ARMORY %d / %d", collectedWeapons, (int)weaponDB.size()), (int)panelRect.x + 728, (int)panelRect.y + 54, 18, neonBlue);
    DrawText(TextFormat("STALL %d / %d", collectedPets, (int)petDB.size()), (int)panelRect.x + 940, (int)panelRect.y + 54, 18, safeGreen);

    int hpUpgradeCost = GetHpUpgradeCost();
    Rectangle blessingRect = { panelRect.x + 24.0f, panelRect.y + 78.0f, 692.0f, 70.0f };
    DrawPanel(blessingRect, panel, safeGreen);
    DrawText("HEARTH ALTAR", (int)blessingRect.x + 18, (int)blessingRect.y + 12, 20, safeGreen);
    DrawText(TextFormat("VIGOR +18   COST %d RENOWN   RANK %d / 8   PRESS H", hpUpgradeCost, player.hpUpgradeLevel), (int)blessingRect.x + 18, (int)blessingRect.y + 40, 18, parchment);

    Rectangle gridRect = { panelRect.x + 24.0f, panelRect.y + 164.0f, 692.0f, 334.0f };
    DrawPanel(gridRect, panel, neonBlue);
    DrawText("ARSENAL OF THE KEEP", (int)gridRect.x + 18, (int)gridRect.y + 14, 22, neonBlue);

    const int cols = 8;
    const float cardW = 76.0f;
    const float cardH = 50.0f;
    const float gap = 5.0f;
    for (int i = 0; i < (int)weaponDB.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        Rectangle card = {
            gridRect.x + 18.0f + col * (cardW + gap),
            gridRect.y + 46.0f + row * (cardH + gap),
            cardW,
            cardH
        };

        const Weapon& weapon = weaponDB[i];
        bool owned = i < (int)shop.ownedWeapons.size() ? shop.ownedWeapons[i] : false;
        bool selected = (i == shop.browseWeaponIdx);
        bool realmUnlocked = IsWorldUnlocked(WeaponOriginWorld(i));
        bool signatureLocked = WeaponIsRealmSignature(i)
            && !owned
            && !(WorldIndex(WeaponOriginWorld(i)) < (int)realmSignatureClaimed.size() ? realmSignatureClaimed[WorldIndex(WeaponOriginWorld(i))] : false);

        Color border = selected ? weapon.color : (owned ? safeGreen : (!realmUnlocked ? GRAY : (signatureLocked ? neonGold : Fade(weapon.color, 0.75f))));
        Color fill = selected ? Fade(weapon.color, 0.18f) : Fade(BLACK, 0.26f);
        DrawRectangleRounded(card, 0.12f, 6, fill);
        DrawRectangleLinesEx(card, selected ? 3.0f : 2.0f, border);

        if (weaponAtlas.id != 0) {
            Rectangle src = WeaponSourceRect(weapon.spriteIndex);
            Rectangle dst = { card.x + 4.0f, card.y + 8.0f, 28.0f, 28.0f };
            DrawTexturePro(weaponAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        }

        std::string cardName = weapon.name;
        if (cardName.size() > 8) {
            cardName = cardName.substr(0, 7) + ".";
        }
        DrawText(cardName.c_str(), (int)card.x + 32, (int)card.y + 8, 10, parchment);
        DrawText(TextFormat("%d", weapon.damage), (int)card.x + 32, (int)card.y + 21, 10, weapon.color);
        const char* state = owned ? "OWN" : (!realmUnlocked ? "SEALED" : (signatureLocked ? "GIFT" : "SALE"));
        DrawText(state, (int)card.x + 32, (int)card.y + 34, 10, owned ? safeGreen : (signatureLocked ? neonGold : ashText));
    }

    const Weapon& browseWeapon = weaponDB[shop.browseWeaponIdx];
    WorldId browseWorld = WeaponOriginWorld(shop.browseWeaponIdx);
    bool browseRealmUnlocked = IsWorldUnlocked(browseWorld);
    bool signatureWeapon = WeaponIsRealmSignature(shop.browseWeaponIdx);
    int signatureWorldSlot = WorldIndex(browseWorld);
    bool signatureClaimed = signatureWorldSlot >= 0 && signatureWorldSlot < (int)realmSignatureClaimed.size()
        ? realmSignatureClaimed[signatureWorldSlot]
        : false;
    bool signatureLocked = signatureWeapon && !shop.ownedWeapons[shop.browseWeaponIdx] && !signatureClaimed;

    Rectangle detailRect = { panelRect.x + 734.0f, panelRect.y + 78.0f, 402.0f, 260.0f };
    DrawPanel(detailRect, panel, browseWeapon.color);
    DrawText("SELECTED ARMAMENT", (int)detailRect.x + 18, (int)detailRect.y + 14, 22, browseWeapon.color);
    if (weaponAtlas.id != 0) {
        Rectangle src = WeaponSourceRect(browseWeapon.spriteIndex);
        Rectangle dst = { detailRect.x + 18.0f, detailRect.y + 52.0f, 112.0f, 112.0f };
        DrawTexturePro(weaponAtlas, src, dst, { 0.0f, 0.0f }, -22.0f, WHITE);
    }
    DrawText(browseWeapon.name.c_str(), (int)detailRect.x + 144, (int)detailRect.y + 54, 24, parchment);
    DrawText(TextFormat("RARITY %s", browseWeapon.rarity.c_str()), (int)detailRect.x + 144, (int)detailRect.y + 82, 18, browseWeapon.color);
    DrawText(TextFormat("TRAIT %s", WeaponTraitLabel(browseWeapon.trait)), (int)detailRect.x + 144, (int)detailRect.y + 108, 18, neonGold);
    DrawText(WeaponTraitDesc(browseWeapon.trait), (int)detailRect.x + 144, (int)detailRect.y + 134, 16, ashText);
    const char* identityDesc = WeaponIdentityDesc(shop.browseWeaponIdx);
    if (identityDesc[0] != '\0') {
        DrawText(identityDesc, (int)detailRect.x + 144, (int)detailRect.y + 156, 15, browseWeapon.color);
    }
    DrawText(TextFormat("DAMAGE %d", browseWeapon.damage), (int)detailRect.x + 18, (int)detailRect.y + 184, 18, parchment);
    DrawText(TextFormat("RANGE %.0f", browseWeapon.range), (int)detailRect.x + 150, (int)detailRect.y + 184, 18, parchment);
    DrawText(TextFormat("SWING %.2fs", GetWeaponAttackCooldown(browseWeapon)), (int)detailRect.x + 258, (int)detailRect.y + 184, 18, parchment);
    DrawText(TextFormat("SOURCE %s", WeaponSourceLabel(shop.browseWeaponIdx)), (int)detailRect.x + 18, (int)detailRect.y + 212, 18, ashText);
    DrawText(TextFormat("COST %d RENOWN", browseWeapon.cost), (int)detailRect.x + 18, (int)detailRect.y + 236, 18, neonGold);

    std::string stateLabel;
    Color stateColor = WHITE;
    std::string actionLabel;
    if (shop.ownedWeapons[shop.browseWeaponIdx]) {
        stateLabel = (player.equippedWeaponIdx == shop.browseWeaponIdx) ? "READIED" : "CLAIMED";
        stateColor = (player.equippedWeaponIdx == shop.browseWeaponIdx) ? neonCyan : safeGreen;
        actionLabel = "PRESS B TO READY THIS ARMAMENT";
    }
    else if (!browseRealmUnlocked) {
        stateLabel = "SEALED BY REALM";
        stateColor = softRed;
        actionLabel = "ADVANCE THE MAIN VOWS TO UNSEAL THIS FORGE";
    }
    else if (signatureLocked) {
        stateLabel = "REALM GIFT";
        stateColor = neonGold;
        actionLabel = "CLEAR A SEALED COURT IN THIS REALM TO CLAIM IT";
    }
    else {
        stateLabel = "FOR SALE";
        stateColor = neonGold;
        actionLabel = "PRESS B TO BUY THIS ARMAMENT";
    }

    Rectangle stateRect = { panelRect.x + 734.0f, panelRect.y + 352.0f, 402.0f, 66.0f };
    DrawPanel(stateRect, panel, stateColor);
    DrawText(TextFormat("STATUS: %s", stateLabel.c_str()), (int)stateRect.x + 18, (int)stateRect.y + 12, 20, stateColor);
    DrawText(actionLabel.c_str(), (int)stateRect.x + 18, (int)stateRect.y + 38, 16, parchment);

    Rectangle petRect = { panelRect.x + 734.0f, panelRect.y + 432.0f, 402.0f, 138.0f };
    DrawPanel(petRect, panel, safeGreen);
    DrawText("COMPANION STALL", (int)petRect.x + 18, (int)petRect.y + 12, 20, safeGreen);
    if (!petDB.empty()) {
        const PetDefinition& petInfo = petDB[shop.browsePetIdx];
        bool ownedPet = shop.browsePetIdx < (int)persistentOwnedPets.size() ? persistentOwnedPets[shop.browsePetIdx] : false;
        bool activePet = ownedPet && persistentEquippedPetIdx == shop.browsePetIdx;
        int bondRank = GetPetBondLevel(shop.browsePetIdx);
        if (petAtlas.id != 0) {
            Rectangle src = PetSourceRect(petInfo.spriteIndex);
            Rectangle dst = { petRect.x + 18.0f, petRect.y + 28.0f, 54.0f, 54.0f };
            DrawTexturePro(petAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
        }
        DrawText(petInfo.name.c_str(), (int)petRect.x + 84, (int)petRect.y + 28, 20, petInfo.color);
        DrawText(petInfo.description.c_str(), (int)petRect.x + 84, (int)petRect.y + 48, 16, ashText);
        DrawText(TextFormat("%d EURO  %d DMG  %.2fs", petInfo.euroCost, GetPetDamage(shop.browsePetIdx), GetPetAttackCooldown(shop.browsePetIdx)), (int)petRect.x + 84, (int)petRect.y + 68, 16, parchment);
        DrawText(TextFormat("BOND %d  XP %d", bondRank, shop.browsePetIdx < (int)persistentPetBondXp.size() ? persistentPetBondXp[shop.browsePetIdx] : 0), (int)petRect.x + 84, (int)petRect.y + 88, 16, petInfo.color);
        DrawText(activePet ? "READY" : (ownedPet ? "OWNED" : "FOR HIRE"), (int)petRect.x + 314, (int)petRect.y + 28, 18, activePet ? petInfo.color : (ownedPet ? safeGreen : neonGold));
        DrawText("P CYCLE   N CLAIM / READY", (int)petRect.x + 192, (int)petRect.y + 86, 16, parchment);
    }
    DrawText(TextFormat("STALL TRAINING %d  COST %d EURO  PRESS M", persistentPetTrainingLevel, GetPetTrainingEuroCost()), (int)petRect.x + 18, (int)petRect.y + 114, 16, neonGold);

    Rectangle footerRect = { panelRect.x + 24.0f, panelRect.y + 586.0f, 1112.0f, 42.0f };
    DrawPanel(footerRect, panel, neonBlue);
    DrawText("E LEAVE   ARROWS BROWSE ARMORY   B BUY OR READY   H VIGOR   P CYCLE PETS   N CLAIM OR READY PET   M TRAIN STALL", (int)footerRect.x + 16, (int)footerRect.y + 12, 15, parchment);

    if (shop.messageTimer > 0.0f) {
        DrawText(shop.message.c_str(), (int)panelRect.x + 734, (int)panelRect.y + 606, 18, shop.messageColor);
    }
}

void Game::DrawQuestBoard() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.72f));

    Color parchment = { 220, 212, 198, 255 };
    Color ashText = { 176, 170, 160, 255 };
    Rectangle panelRect = { screenW * 0.5f - 470.0f, screenH * 0.5f - 250.0f, 940.0f, 500.0f };
    DrawPanel(panelRect, panel2, neonGold);
    DrawText("OATHBOARD OF THE KEEP", (int)panelRect.x + 28, (int)panelRect.y + 22, 30, neonGold);
    DrawText(TextFormat("EURO HELD: %d", euro), (int)panelRect.x + 700, (int)panelRect.y + 28, 22, safeGreen);

    Rectangle mainCard = { panelRect.x + 26.0f, panelRect.y + 72.0f, panelRect.width - 52.0f, 128.0f };
    DrawPanel(mainCard, panel, neonGold);
    DrawText("MAIN VOW", (int)mainCard.x + 18, (int)mainCard.y + 14, 20, neonGold);
    if (mainQuestIndex >= 0 && mainQuestIndex < (int)mainQuestDB.size()) {
        const QuestDefinition& mainQuest = mainQuestDB[mainQuestIndex];
        WorldId questWorld = MainQuestWorld(mainQuestIndex);
        DrawText(TextFormat("REALM %s", WorldLabel(questWorld)), (int)mainCard.x + 18, (int)mainCard.y + 38, 16, neonBlue);
        DrawText(mainQuest.title.c_str(), (int)mainCard.x + 18, (int)mainCard.y + 58, 24, parchment);
        DrawText(mainQuest.description.c_str(), (int)mainCard.x + 18, (int)mainCard.y + 88, 18, ashText);
        DrawRectangleRounded({ mainCard.x + 18.0f, mainCard.y + 114.0f, 320.0f, 10.0f }, 0.25f, 6, Fade(BLACK, 0.32f));
        DrawRectangleRounded({ mainCard.x + 18.0f, mainCard.y + 114.0f, 320.0f * (float)mainQuestProgress / (float)std::max(1, mainQuest.target), 10.0f }, 0.25f, 6, mainQuestReady ? safeGreen : neonGold);
        DrawText(TextFormat("%s %d / %d %s", QuestObjectiveVerb(mainQuest.objective), mainQuestProgress, mainQuest.target, QuestObjectiveLabel(mainQuest.objective)), (int)mainCard.x + 352, (int)mainCard.y + 108, 18, mainQuestReady ? safeGreen : neonBlue);
        DrawText(mainQuestReady ? TextFormat("PRESS E TO CLAIM %d EURO", mainQuest.euroReward) : TextFormat("REWARD %d EURO", mainQuest.euroReward), (int)mainCard.x + 612, (int)mainCard.y + 108, 18, mainQuestReady ? safeGreen : parchment);
    }
    else {
        DrawText("ALL V2 MAIN VOWS STAND COMPLETED", (int)mainCard.x + 18, (int)mainCard.y + 56, 24, parchment);
        DrawText("Free hunt the realms, finish side vows, and forge the rest of the arsenal.", (int)mainCard.x + 18, (int)mainCard.y + 92, 18, ashText);
    }

    for (int i = 0; i < 3; ++i) {
        Rectangle card = { panelRect.x + 28.0f + i * 295.0f, panelRect.y + 226.0f, 265.0f, 214.0f };
        DrawPanel(card, panel, neonBlue);

        int defIndex = sideQuestOfferIndices[i];
        if (defIndex < 0 || defIndex >= (int)sideQuestDB.size()) {
            DrawText("NO CONTRACT", (int)card.x + 18, (int)card.y + 36, 22, parchment);
            continue;
        }

        const QuestDefinition& quest = sideQuestDB[defIndex];
        DrawText(TextFormat("SIDE VOW %d", i + 1), (int)card.x + 18, (int)card.y + 14, 18, neonBlue);
        DrawText(quest.title.c_str(), (int)card.x + 18, (int)card.y + 42, 22, parchment);
        DrawText(quest.description.c_str(), (int)card.x + 18, (int)card.y + 76, 18, ashText);
        DrawRectangleRounded({ card.x + 18.0f, card.y + 116.0f, 188.0f, 8.0f }, 0.25f, 6, Fade(BLACK, 0.32f));
        DrawRectangleRounded({ card.x + 18.0f, card.y + 116.0f, 188.0f * (float)sideQuestProgress[i] / (float)std::max(1, quest.target), 8.0f }, 0.25f, 6, sideQuestReady[i] ? safeGreen : neonGold);
        DrawText(TextFormat("%s %d / %d %s", QuestObjectiveVerb(quest.objective), sideQuestProgress[i], quest.target, QuestObjectiveLabel(quest.objective)), (int)card.x + 18, (int)card.y + 132, 18, sideQuestReady[i] ? safeGreen : neonGold);
        DrawText(TextFormat("REWARD %d EURO", quest.euroReward), (int)card.x + 18, (int)card.y + 158, 18, safeGreen);

        const char* status = sideQuestAccepted[i] ? (sideQuestReady[i] ? "TURN IN" : "ACTIVE") : "ACCEPT";
        Color statusColor = sideQuestAccepted[i] ? (sideQuestReady[i] ? safeGreen : neonCyan) : neonBlue;
        DrawText(TextFormat("%d  %s", i + 1, status), (int)card.x + 18, (int)card.y + 184, 18, statusColor);
    }

    Rectangle footerRect = { panelRect.x + 24.0f, panelRect.y + 452.0f, panelRect.width - 48.0f, 32.0f };
    DrawPanel(footerRect, panel, neonBlue);
    DrawText("Q CLOSE BOARD   MAIN VOWS UNLOCK REALMS // SIDE VOWS PAY EURO // CLEAR COURTS AND CLAIM BLESSINGS TO ADVANCE", (int)footerRect.x + 14, (int)footerRect.y + 9, 16, parchment);
}

void Game::DrawRealmMap() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.70f));

    Color parchment = { 220, 212, 198, 255 };
    Color ashText = { 176, 170, 160, 255 };
    Rectangle panelRect = { screenW * 0.5f - 490.0f, screenH * 0.5f - 230.0f, 980.0f, 460.0f };
    DrawPanel(panelRect, panel2, neonBlue);
    DrawText("GATE OF REALMS", (int)panelRect.x + 28, (int)panelRect.y + 22, 30, neonBlue);
    DrawText("Choose which wounded land will bear the next hunt.", (int)panelRect.x + 28, (int)panelRect.y + 58, 18, ashText);

    const WorldId worlds[4] = { WorldId::Crownheart, WorldId::Frostveil, WorldId::Sunscar, WorldId::Mirethorn };
    for (int i = 0; i < 4; ++i) {
        WorldId world = worlds[i];
        bool unlocked = IsWorldUnlocked(world);
        bool active = (world == currentWorld);
        Rectangle card = { panelRect.x + 22.0f + i * 233.0f, panelRect.y + 108.0f, 212.0f, 278.0f };
        Color accent = WorldAccentTint(world);
        DrawPanel(card, panel, active ? accent : Fade(accent, 0.75f));
        DrawRectangleGradientV((int)card.x + 12, (int)card.y + 42, (int)card.width - 24, 82, Fade(WorldSkyTopTint(world), 0.85f), Fade(WorldRoadTint(world), 0.95f));
        DrawRectangle((int)card.x + 12, (int)card.y + 122, (int)card.width - 24, 1, Fade(accent, 0.40f));
        DrawText(TextFormat("%d", i + 1), (int)card.x + 16, (int)card.y + 12, 22, accent);
        DrawText(WorldLabel(world), (int)card.x + 14, (int)card.y + 138, 18, unlocked ? parchment : GRAY);
        DrawText(WorldBlurbLine1(world), (int)card.x + 14, (int)card.y + 174, 16, unlocked ? ashText : GRAY);
        DrawText(WorldBlurbLine2(world), (int)card.x + 14, (int)card.y + 196, 16, unlocked ? ashText : GRAY);

        if (active) {
            DrawText("CURRENT REALM", (int)card.x + 14, (int)card.y + 240, 18, safeGreen);
        }
        else if (unlocked) {
            DrawText("PRESS NUMBER TO TRAVEL", (int)card.x + 14, (int)card.y + 240, 16, accent);
        }
        else {
            DrawText(TextFormat("UNLOCK AT %d MAIN VOWS", MainQuestUnlockRequirement(world)), (int)card.x + 14, (int)card.y + 240, 16, softRed);
        }
    }

    Rectangle footerRect = { panelRect.x + 24.0f, panelRect.y + 400.0f, panelRect.width - 48.0f, 32.0f };
    DrawPanel(footerRect, panel, neonBlue);
    DrawText("R CLOSE MAP   TRAVEL KEEPS YOUR RUN BUILD, SHIFTS THE REALM ART, AND SENDS THE KNIGHT INTO THE NEXT HUNT", (int)footerRect.x + 14, (int)footerRect.y + 9, 16, parchment);
}

void Game::DrawGameOver() const {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.82f));

    Color parchment = { 224, 214, 198, 255 };
    Rectangle panelRect = { screenW * 0.5f - 360.0f, 154.0f, 720.0f, 350.0f };
    DrawPanel(panelRect, panel2, softRed);
    DrawText("THE KNIGHT HAS FALLEN", screenW / 2 - MeasureText("THE KNIGHT HAS FALLEN", 50) / 2, 186, 50, softRed);
    DrawText("Yet the kingdom remembers what was carried from the field.", screenW / 2 - MeasureText("Yet the kingdom remembers what was carried from the field.", 20) / 2, 240, 20, { 176, 170, 160, 255 });
    DrawText(TextFormat("WAVE REACHED: %d", player.wave), screenW / 2 - MeasureText(TextFormat("WAVE REACHED: %d", player.wave), 28) / 2, 294, 28, parchment);
    DrawText(TextFormat("TOTAL KILLS: %d", player.kills), screenW / 2 - MeasureText(TextFormat("TOTAL KILLS: %d", player.kills), 28) / 2, 330, 28, parchment);
    DrawText(TextFormat("RENOWN CARRIED FORWARD: %d", legacyRenown), screenW / 2 - MeasureText(TextFormat("RENOWN CARRIED FORWARD: %d", legacyRenown), 28) / 2, 368, 28, neonGold);
    DrawText(TextFormat("EURO KEPT: %d", euro), screenW / 2 - MeasureText(TextFormat("EURO KEPT: %d", euro), 24) / 2, 406, 24, safeGreen);
    DrawText("WEAPONS, EURO, QUESTS, PET BONDS AND VIGOR UPGRADES ARE KEPT", screenW / 2 - MeasureText("WEAPONS, EURO, QUESTS, PET BONDS AND VIGOR UPGRADES ARE KEPT", 20) / 2, 444, 20, parchment);
    DrawText("PRESS ENTER TO RIDE OUT ONCE MORE", screenW / 2 - MeasureText("PRESS ENTER TO RIDE OUT ONCE MORE", 26) / 2, 474, 26, neonCyan);
}

void Game::DrawEnemySprite(const ActiveMonster& monster) const {
    float t = (float)GetTime();
    DrawCartoonShadow({ monster.pos.x, monster.pos.y + monster.radius + 6.0f }, monster.radius * 0.85f, monster.radius * 0.30f, 0.20f);

    int spriteIndex = 0;
    float size = 72.0f;
    if (monster.isBoss) {
        spriteIndex = (monster.typeIndex == 11) ? 5 : 4;
        size = 98.0f;
    }
    else if (monster.typeIndex == 0 || monster.typeIndex == 9) {
        spriteIndex = 0;
        size = 68.0f;
    }
    else if (monster.typeIndex == 1 || monster.typeIndex == 8) {
        spriteIndex = 1;
        size = 70.0f;
    }
    else if (monster.typeIndex == 2 || monster.typeIndex == 5) {
        spriteIndex = 2;
        size = 72.0f;
    }
    else {
        spriteIndex = 3;
        size = 80.0f;
    }

    Color drawTint = (monster.hitFlash > 0.0f) ? WHITE : monster.color;
    if (enemyAtlas.id != 0) {
        Rectangle src = EnemySourceRect(spriteIndex);
        Rectangle dst = { monster.pos.x - size * 0.5f, monster.pos.y - size * 0.70f, size, size };
        DrawTexturePro(enemyAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, drawTint);
    }
    else {
        DrawCircleV(monster.pos, monster.radius, drawTint);
    }

    if (monster.slowTimer > 0.0f) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 4.0f + std::sin(t * 7.0f), Fade({ 200, 234, 255, 255 }, 0.35f));
    }
    if (monster.burnTimer > 0.0f) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 2.0f + std::sin(t * 9.0f), Fade({ 244, 170, 78, 255 }, 0.28f));
    }
    if (monster.poisonTimer > 0.0f) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 1.0f + std::sin(t * 6.0f), Fade({ 150, 206, 106, 255 }, 0.30f));
    }

    if (monster.isElite) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 6.0f + std::sin(t * 5.0f) * 1.5f, Fade(neonPink, 0.30f));
    }

    if (monster.isBoss) {
        DrawCircleLines((int)monster.pos.x, (int)monster.pos.y, monster.radius + 12.0f + std::sin(t * 3.0f) * 2.0f, Fade(neonGold, 0.34f));
        DrawText(monster.name.c_str(), (int)monster.pos.x - MeasureText(monster.name.c_str(), 16) / 2, (int)(monster.pos.y - monster.radius - 42.0f), 16, neonGold);
    }
}

void Game::DrawPlayerWeapon() const {
    if (weaponAtlas.id == 0 || weaponDB.empty()) {
        return;
    }

    const Weapon& weapon = weaponDB[player.equippedWeaponIdx];
    Vector2 aim = player.aimDir;
    if (aim.x == 0.0f && aim.y == 0.0f) {
        aim = { 1.0f, 0.0f };
    }

    float swingDuration = GetWeaponAttackCooldown(weapon);
    float swingPhase = 0.0f;
    if (swingDuration > 0.001f) {
        swingPhase = ClampFloat(player.attackCd / swingDuration, 0.0f, 1.0f);
    }

    float swingArc = 0.0f;
    if (swingPhase > 0.0f) {
        float strength = 1.0f;
        if (weapon.trait == WeaponTrait::Heavy) strength = 1.35f;
        else if (weapon.trait == WeaponTrait::Swift) strength = 0.82f;
        else if (weapon.trait == WeaponTrait::Royal) strength = 1.15f;
        swingArc = std::sin((1.0f - swingPhase) * 3.14159265f) * 42.0f * strength;
    }

    float angle = std::atan2(aim.y, aim.x) * 57.2957795f + 28.0f - swingArc;
    Vector2 handPos = {
        player.pos.x + aim.x * (12.0f + swingPhase * 6.0f),
        player.pos.y - 20.0f + aim.y * (8.0f + swingPhase * 4.0f)
    };
    Rectangle src = WeaponSourceRect(weapon.spriteIndex);
    Rectangle dst = { handPos.x - 14.0f, handPos.y - 10.0f, 72.0f, 72.0f };
    DrawCartoonShadow({ handPos.x + 6.0f, handPos.y + 18.0f }, 16.0f, 4.0f, 0.12f);
    DrawTexturePro(weaponAtlas, src, dst, { 20.0f, 44.0f }, angle, WHITE);
}

void Game::DrawPetSprite() const {
    if (!pet.active || pet.petIndex < 0 || pet.petIndex >= (int)petDB.size()) {
        return;
    }

    const PetDefinition& petInfo = petDB[pet.petIndex];
    DrawCartoonShadow({ pet.pos.x, pet.pos.y + 12.0f }, 12.0f, 4.0f, 0.14f);
    DrawCircleLines((int)pet.pos.x, (int)pet.pos.y, 18.0f + std::sin((float)GetTime() * 4.0f) * 1.2f, Fade(petInfo.color, 0.24f));

    if (petAtlas.id != 0) {
        Rectangle src = PetSourceRect(petInfo.spriteIndex);
        Rectangle dst = { pet.pos.x - 28.0f, pet.pos.y - 30.0f, 56.0f, 56.0f };
        DrawTexturePro(petAtlas, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
    else {
        DrawCircleV(pet.pos, 12.0f, petInfo.color);
    }
}
