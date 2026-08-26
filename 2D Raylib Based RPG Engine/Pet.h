#pragma once
#include "raylib.h"
#include <string>

struct PetDefinition {
    std::string name;
    std::string description;
    int euroCost = 0;
    int damage = 0;
    float attackCooldown = 1.0f;
    float orbitRadius = 28.0f;
    Color color = WHITE;
    int spriteIndex = 0;
};

struct ActivePet {
    bool active = false;
    int petIndex = -1;
    Vector2 pos = { 0.0f, 0.0f };
    float orbitAngle = 0.0f;
    float fireTimer = 0.0f;
    float bob = 0.0f;
};
