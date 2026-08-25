#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>

struct DungeonArea {
    Rectangle rect{};
    std::string name;
    bool isSafeZone = false;
    bool isSpawnZone = false;
};

struct DungeonObstacle {
    Rectangle rect{};
    Color color{ 42, 50, 72, 255 };
    int spriteIndex = -1;
    float spriteScale = 1.0f;
};

struct DungeonMap {
    std::vector<DungeonArea> rooms;
    std::vector<Rectangle> corridors;
    std::vector<DungeonObstacle> obstacles;
};

inline Rectangle ExpandRect(Rectangle rect, float amount) {
    return {
        rect.x - amount,
        rect.y - amount,
        rect.width + amount * 2.0f,
        rect.height + amount * 2.0f
    };
}

inline bool CircleRectCollision(Vector2 center, float radius, Rectangle rect) {
    float nearestX = center.x;
    float nearestY = center.y;

    if (nearestX < rect.x) nearestX = rect.x;
    if (nearestX > rect.x + rect.width) nearestX = rect.x + rect.width;
    if (nearestY < rect.y) nearestY = rect.y;
    if (nearestY > rect.y + rect.height) nearestY = rect.y + rect.height;

    float dx = center.x - nearestX;
    float dy = center.y - nearestY;
    return (dx * dx + dy * dy) <= radius * radius;
}

inline bool IsCircleInsideRect(Vector2 center, float radius, Rectangle rect) {
    return center.x - radius >= rect.x
        && center.x + radius <= rect.x + rect.width
        && center.y - radius >= rect.y
        && center.y + radius <= rect.y + rect.height;
}

inline bool IsPointInWalkArea(const DungeonMap& dungeon, Vector2 pos) {
    for (const auto& room : dungeon.rooms) {
        if (CheckCollisionPointRec(pos, room.rect)) {
            return true;
        }
    }

    for (const auto& corridor : dungeon.corridors) {
        if (CheckCollisionPointRec(pos, corridor)) {
            return true;
        }
    }

    return false;
}

inline bool IsPositionWalkable(Vector2 pos, float radius, const DungeonMap& dungeon) {
    const float diag = radius * 0.70710678f;
    const Vector2 probes[] = {
        pos,
        { pos.x + radius, pos.y },
        { pos.x - radius, pos.y },
        { pos.x, pos.y + radius },
        { pos.x, pos.y - radius },
        { pos.x + diag, pos.y + diag },
        { pos.x + diag, pos.y - diag },
        { pos.x - diag, pos.y + diag },
        { pos.x - diag, pos.y - diag }
    };

    for (const auto& probe : probes) {
        if (!IsPointInWalkArea(dungeon, probe)) {
            return false;
        }
    }

    for (const auto& obstacle : dungeon.obstacles) {
        if (CircleRectCollision(pos, radius + 1.5f, obstacle.rect)) {
            return false;
        }
    }

    return true;
}

inline const DungeonArea* FindDungeonArea(const DungeonMap& dungeon, Vector2 pos) {
    for (const auto& room : dungeon.rooms) {
        if (CheckCollisionPointRec(pos, room.rect)) {
            return &room;
        }
    }
    return nullptr;
}

inline Rectangle ShrinkRect(Rectangle rect, float amount) {
    return {
        rect.x + amount,
        rect.y + amount,
        rect.width - amount * 2.0f,
        rect.height - amount * 2.0f
    };
}
