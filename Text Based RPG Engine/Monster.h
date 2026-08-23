#pragma once
#include <iostream>
#include <string>
#include "Character.h"

class Monster : public Character {
public:
    bool isBoss;

    Monster(const std::string& n, int h, int d, bool boss = false)
        : Character(n, h, d), isBoss(boss) {}

    void attack(Character& target) override {
        int finalDmg = isBoss ? static_cast<int>(damage * 1.5) : damage;
        std::cout << "\033[1;31m[" << name << "]\033[0m Executing strike...\n";
        target.takeDamage(finalDmg);
    }
};