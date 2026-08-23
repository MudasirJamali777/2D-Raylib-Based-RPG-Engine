#pragma once
#include "raylib.h"
#include <string>

struct Weapon {
    std::string name;
    int damage;
    float range;
    std::string rarity;
    int cost;
    Color color;
};
