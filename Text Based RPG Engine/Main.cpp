#include <iostream>
#include <fstream> 
#include <vector>
#include <memory>
#include <ctime>   
#include <cstdlib> 
#include <string>
#include <limits>
#include "Character.h"
#include "Monster.h"
#include "Weapon.h"

using namespace std;

// Helper function for safe integer input
int getValidInput(const string& prompt) {
    int input;
    while (true) {
        cout << prompt;
        if (cin >> input) {
            return input;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\033[1;31m[ERROR]\033[0m Invalid input. Please enter a number.\n";
    }
}

// Helper function to generate visual progress bars
string renderBar(int current, int max, int length = 20) {
    if (max <= 0) return "[Error]";

    int fill = (current * length) / max;
    if (fill < 0) fill = 0;
    if (fill > length) fill = length;

    string bar = "[";
    for (int i = 0; i < length; ++i) {
        if (i < fill) bar += "|";
        else bar += " ";
    }
    bar += "]";
    return bar;
}

class Player : public Character {
public:
    vector<Item> inventory;
    Weapon equippedWeapon = { "Iron Pipe", 5, "Common" };

    Player(string n) : Character(n, 120, 25) {
        inventory.push_back({ "Field Kit", 20 });
    }

    int getXP() const { return xp; }

    bool spendXP(int cost) {
        if (xp >= cost) {
            xp -= cost;
            return true;
        }
        cout << "\033[1;31m[MERCHANT]\033[0m Insufficient XP.\n";
        return false;
    }

    void upgradeDamage(int amount) { damage += amount; }
    void upgradeHealth(int amount) { maxHealth += amount; health = maxHealth; }

    void attack(Character& target) override {
        int critChance = rand() % 100 + 1;
        int finalDamage = damage + equippedWeapon.damageBonus;

        if (critChance <= 20) {
            finalDamage *= 2;
            cout << "\033[1;33m[CRITICAL HIT!]\033[0m ";
        }

        cout << "\033[1;32m[YOU]\033[0m Strike with " << equippedWeapon.name
            << " dealing " << finalDamage << " damage!\n";
        target.takeDamage(finalDamage);
    }

    void specialAbility(Character& target) {
        if (mana >= 20) {
            mana -= 20;
            int specialDmg = (damage + equippedWeapon.damageBonus) * 2;
            cout << "\033[1;35m[TACTICAL STRIKE]\033[0m " << name << " executes a heavy blow for " << specialDmg << " damage!\n";
            target.takeDamage(specialDmg);

            // Inflict 10 damage per turn for 3 turns
            target.applyEffect({ "Fracture", 10, 3 });
        }
        else {
            cout << "\033[1;33m[RESOURCES LOW]\033[0m Insufficient energy.\n";
        }
    }

    void equipWeapon(Weapon newWep) {
        cout << "\033[1;33m[EQUIP]\033[0m Swapped to: " << newWep.name << " (+" << newWep.damageBonus << " DMG)\n";
        equippedWeapon = newWep;
    }

    void addLoot(Item loot) {
        cout << "\033[1;33m[LOOT FOUND]\033[0m You acquired: " << loot.itemName << ".\n";
        inventory.push_back(loot);
    }

    // Pass wave as a parameter to save it
    void saveGame(int wave) {
        ofstream outFile("savegame.txt");
        if (outFile.is_open()) {
            outFile << level << " " << health << " " << maxHealth << " " << damage << " " << xp << " "
                << mana << " " << maxMana << " " << armor << " " << equippedWeapon.damageBonus << " " << wave << " " << equippedWeapon.name;
            outFile.close();
            cout << "\033[1;32m[SYSTEM]\033[0m Progress Saved.\n";
        }
    }

    // Pass wave by reference to load it
    void loadGame(int& currentWave) {
        ifstream inFile("savegame.txt");
        if (inFile.is_open()) {
            inFile >> level >> health >> maxHealth >> damage >> xp >> mana >> maxMana >> armor >> equippedWeapon.damageBonus >> currentWave;
            getline(inFile >> ws, equippedWeapon.name);
            inFile.close();

            // Fix XP exploit
            xpToNextLevel = level * 100;
            cout << "\033[1;36m[SYSTEM]\033[0m Data Loaded.\n";
        }
    }

private:
    int xp = 0;
    int xpToNextLevel = 100;
    int mana = 50;
    int maxMana = 50;

public:
    void gainXP(int amount) {
        xp += amount;
        if (xp >= xpToNextLevel) levelUp();
    }

    void levelUp() {
        level++;
        xpToNextLevel = level * 100;
        maxHealth += 20;
        health = maxHealth;
        damage += 5;
        mana = maxMana;
        cout << "\033[1;33m[LEVEL UP!]\033[0m Reached Level " << level << ".\n";
    }

    void displayHUD() {
        cout << "\n======================================================\n";
        cout << " COMMANDER: " << name << " | RANK: " << level << " | WEP: " << equippedWeapon.name << "\n";
        cout << " HP: " << renderBar(health, maxHealth) << " " << health << "/" << maxHealth << "\n";
        cout << " MP: " << renderBar(mana, maxMana, 10) << " " << mana << "/" << maxMana << "\n";
        cout << " XP: " << renderBar(xp, xpToNextLevel, 15) << " " << xp << "/" << xpToNextLevel << "\n";
        cout << " ARMOR: " << armor << "%\n";
        cout << "======================================================\n";
    }
};

void openShop(Player& hero) {
    cout << "\n\033[1;36m--- [SAFE ZONE: THE OUTPOST] ---\033[0m\n";
    cout << "Merchant: 'Select your upgrades.'\n";

    bool shopping = true;
    while (shopping) {
        cout << "\nYour XP: " << hero.getXP() << " | Armor: " << hero.armor << "%\n";
        cout << "1. Reinforce Blade (+5 DMG) - 150 XP\n";
        cout << "2. Upgrade Plating (+10% Def) - 200 XP\n";
        cout << "3. Exit Outpost\n";

        int choice = getValidInput("Choice: ");

        if (choice == 1 && hero.spendXP(150)) {
            hero.upgradeDamage(5);
            cout << "Damage upgraded.\n";
        }
        else if (choice == 2 && hero.spendXP(200)) {
            if (hero.armor < 50) {
                hero.armor += 10;
                cout << "Armor reinforced.\n";
            }
            else cout << "Maximum Armor capacity reached.\n";
        }
        else if (choice == 3) {
            shopping = false;
        }
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    auto hero = make_unique<Player>("MK_Void");

    int wave = 1;
    hero->loadGame(wave); // Load wave data

    while (hero->isAlive()) {
        bool isBossWave = (wave % 5 == 0);
        unique_ptr<Monster> enemy;

        if (isBossWave) {
            cout << "\n\033[1;35m[!!! HEAVY TARGET WARNING !!!]\033[0m\n";
            enemy = make_unique<Monster>("Juggernaut_v" + to_string(wave), 200 + (wave * 20), 15 + wave, true);
        }
        else {
            enemy = make_unique<Monster>("Automaton_v" + to_string(wave), 40 + (wave * 10), 8 + wave);
        }

        while (hero->isAlive() && enemy->isAlive()) {
            // Process any active debuffs before the turn starts
            hero->processEffects();
            enemy->processEffects();

            // Check if status effects killed anyone before proceeding
            if (!hero->isAlive() || !enemy->isAlive()) break;

            hero->displayHUD();
            cout << "\n1. Attack | 2. Inventory | 3. Special\n";
            int choice = getValidInput("Action: ");

            if (choice == 1) {
                hero->attack(*enemy);
            }
            else if (choice == 2) {
                if (hero->inventory.empty()) {
                    cout << "Inventory is empty.\n";
                }
                else {
                    cout << "\n[INVENTORY]\n";
                    for (size_t i = 0; i < hero->inventory.size(); ++i) {
                        cout << i + 1 << ". " << hero->inventory[i].itemName << " (Heals " << hero->inventory[i].healAmount << ")\n";
                    }
                    cout << "0. Cancel\n";
                    int itemChoice = getValidInput("Select item: ");

                    if (itemChoice > 0 && itemChoice <= hero->inventory.size()) {
                        hero->heal(hero->inventory[itemChoice - 1].healAmount);
                        hero->inventory.erase(hero->inventory.begin() + (itemChoice - 1));
                    }
                }
            }
            else if (choice == 3) {
                hero->specialAbility(*enemy);
            }

            if (enemy->isAlive() && choice != 2) enemy->attack(*hero);
        }

        if (hero->isAlive()) {
            cout << "\033[1;32m[AREA CLEARED]\033[0m\n";
            if (isBossWave) {
                Weapon bossLoot = { "Titan-Slayer", 30 + (wave * 5), "LEGENDARY" };
                hero->equipWeapon(bossLoot);
            }
            else {
                hero->gainXP(50 + (wave * 10));
                if (rand() % 2 == 0) hero->addLoot({ "Trauma Kit", 40 });
                else hero->addLoot({ "Bandage", 15 });
            }

            wave++;
            hero->saveGame(wave); // Save new wave data

            if (wave % 3 == 0 && hero->isAlive()) {
                openShop(*hero);
                hero->saveGame(wave);
            }
        }
        else {
            cout << "\033[1;31m[SYSTEM FAILURE]\033[0m\n";
            break;
        }
    }
    return 0;
}