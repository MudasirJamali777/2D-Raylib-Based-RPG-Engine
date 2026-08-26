#pragma once
#include <raylib.h>
#include <string>
#include <vector>

struct ShopState {
    bool isOpen = false;
    int browseWeaponIdx = 0;
    int browsePetIdx = 0;
    float messageTimer = 0.0f;
    std::string message;
    Color messageColor = WHITE;
    std::vector<bool> ownedWeapons;
};
