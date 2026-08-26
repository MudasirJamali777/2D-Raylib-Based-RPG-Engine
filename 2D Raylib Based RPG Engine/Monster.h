#pragma once
#include <raylib.h>
#include <string>

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
    bool isElite = false;
    int eliteKind = -1;
    int typeIndex;
    std::string name;
    float slowTimer = 0.0f;
    float burnTimer = 0.0f;
    float burnTickTimer = 0.0f;
    float poisonTimer = 0.0f;
    float poisonTickTimer = 0.0f;
};
