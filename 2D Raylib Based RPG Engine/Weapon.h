#pragma once
#include <raylib.h>
#include <string>

enum class WeaponTrait {
    Balanced,
    Swift,
    Heavy,
    Frost,
    Chain,
    Sunfire,
    Venom,
    Guardian,
    Executioner,
    Royal
};

struct Weapon {
    std::string name;
    int damage;
    float range;
    std::string rarity;
    int cost;
    Color color;
    int spriteIndex = 0;
    WeaponTrait trait = WeaponTrait::Balanced;
    float cooldownScale = 1.0f;
};
