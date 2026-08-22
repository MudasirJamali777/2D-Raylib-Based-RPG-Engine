#pragma once
#include <string>
#include <iostream>
#include <vector>

struct StatusEffect {
    std::string name;
    int damagePerTurn;
    int durationTurns;
};

class Character {
public:
    std::string name;
    int health, maxHealth, damage, level, armor;
    std::vector<StatusEffect> activeEffects; // Array to hold ongoing statuses

    Character(std::string n, int h, int d)
        : name(n), health(h), maxHealth(h), damage(d), level(1), armor(0) {}

    virtual ~Character() = default;

    virtual void takeDamage(int amount) {
        int reduced = amount * (100 - armor) / 100;
        health -= reduced;
        if (health < 0) health = 0;
        std::cout << "\033[1;31m[DAMAGE]\033[0m " << name << " sustained " << reduced << " damage (Blocked " << armor << "%).\n";
    }

    void heal(int amount) {
        health += amount;
        if (health > maxHealth) health = maxHealth;
        std::cout << "\033[1;32m[HEAL]\033[0m System integrity restored.\n";
    }

    // Apply a new status effect
    void applyEffect(StatusEffect effect) {
        activeEffects.push_back(effect);
        std::cout << "\033[1;33m[STATUS WARNING]\033[0m " << name << " afflicted with " << effect.name << "!\n";
    }

    // Process effects at the start of a turn
    void processEffects() {
        for (auto it = activeEffects.begin(); it != activeEffects.end(); ) {
            health -= it->damagePerTurn;
            if (health < 0) health = 0;
            std::cout << "\033[1;31m[" << it->name << "]\033[0m " << name << " suffers " << it->damagePerTurn
                << " damage. (" << it->durationTurns - 1 << " turns remaining)\n";

            it->durationTurns--;
            if (it->durationTurns <= 0) {
                it = activeEffects.erase(it); // Remove effect when duration expires
            }
            else {
                ++it;
            }
        }
    }

    bool isAlive() const { return health > 0; }
    virtual void attack(Character& target) = 0;
};