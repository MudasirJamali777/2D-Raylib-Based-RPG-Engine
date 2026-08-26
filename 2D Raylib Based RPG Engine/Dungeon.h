#pragma once
#include <raylib.h>
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
    std::vector<Rectangle> colliders;
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
    if (!IsPointInWalkArea(dungeon, pos)) {
        return false;
    }

    constexpr int sampleCount = 16;
    for (int i = 0; i < sampleCount; ++i) {
        float angle = (6.28318530718f * (float)i) / (float)sampleCount;
        Vector2 probe = {
            pos.x + std::cos(angle) * radius,
            pos.y + std::sin(angle) * radius
        };

        if (!IsPointInWalkArea(dungeon, probe)) {
            return false;
        }
    }

    for (const auto& obstacle : dungeon.obstacles) {
        if (obstacle.colliders.empty()) {
            if (CircleRectCollision(pos, radius + 0.5f, obstacle.rect)) {
                return false;
            }
        }
        else {
            for (const auto& collider : obstacle.colliders) {
                if (CircleRectCollision(pos, radius + 0.5f, collider)) {
                    return false;
                }
            }
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
