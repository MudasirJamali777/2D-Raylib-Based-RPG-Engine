#pragma once
#include "raylib.h"
#include <vector>
#include <string>

struct DungeonArea {
    Rectangle rect{};
    std::string name;
    bool isSafeZone = false;
    bool isSpawnZone = false;
};

struct DungeonObstacle {
    Rectangle rect{};
    Color color{ 42, 50, 72, 255 };
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

inline bool IsPositionWalkable(Vector2 pos, float radius, const DungeonMap& dungeon) {
    bool insideWalkArea = false;

    for (const auto& room : dungeon.rooms) {
        if (CheckCollisionPointRec(pos, ExpandRect(room.rect, radius))) {
            insideWalkArea = true;
            break;
        }
    }

    if (!insideWalkArea) {
        for (const auto& corridor : dungeon.corridors) {
            if (CheckCollisionPointRec(pos, ExpandRect(corridor, radius))) {
                insideWalkArea = true;
                break;
            }
        }
    }

    if (!insideWalkArea) {
        return false;
    }

    for (const auto& obstacle : dungeon.obstacles) {
        if (CheckCollisionPointRec(pos, ExpandRect(obstacle.rect, radius + 2.0f))) {
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
