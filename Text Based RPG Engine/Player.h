#pragma once
#include "raylib.h"

struct Player {
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 aimDir = { 1.0f, 0.0f };
    float speed = 4.6f;

    int hp = 160;
    int maxHp = 160;
    int xp = 0;
    int kills = 0;
    int wave = 1;
    int hpUpgradeLevel = 0;
    int equippedWeaponIdx = 0;

    float dashCd = 0.0f;
    float empCd = 0.0f;
    float turretCd = 0.0f;
    float attackCd = 0.0f;
    float hitFlash = 0.0f;
};
