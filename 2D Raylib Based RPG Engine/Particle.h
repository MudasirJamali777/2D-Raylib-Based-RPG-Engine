#pragma once
#include "raylib.h"
#include <string>

struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
    float maxLife;
    float size;
};

struct Orb {
    Vector2 pos;
    Vector2 vel;
    int value;
    Color color;
};

struct FloatingText {
    Vector2 pos;
    Vector2 vel;
    std::string text;
    Color color;
    float life;
};

struct Turret {
    Vector2 pos;
    float life;
    float fireTimer;
};

struct Shockwave {
    Vector2 pos;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
    Color color;
};

struct Beam {
    Vector2 start;
    Vector2 end;
    Color color;
    float thickness;
    float life;
};

struct Star {
    Vector2 pos;
    float speed;
    float size;
};

struct HealthPickup {
    Vector2 pos;
    Vector2 vel;
    int healAmount;
    float life;
    float spin;
};
