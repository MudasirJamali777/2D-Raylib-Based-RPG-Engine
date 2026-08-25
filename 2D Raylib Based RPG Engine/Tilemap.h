#pragma once
#include "raylib.h"
#include <vector>

enum class TileType {
    Grass = 0,
    Path = 1,
    GrassAlt = 2,
    Flowers = 3
};

struct TileMap {
    int width = 0;
    int height = 0;
    int tileSize = 64;
    int originX = 0;
    int originY = 0;
    std::vector<int> tiles;
};

struct PropInstance {
    Vector2 pos{};
    int spriteIndex = 0;
    float scale = 1.0f;
};

inline int TileAt(const TileMap& map, int x, int y) {
    if (x < 0 || y < 0 || x >= map.width || y >= map.height) {
        return (int)TileType::Grass;
    }
    return map.tiles[(size_t)y * (size_t)map.width + (size_t)x];
}

inline Rectangle TileWorldRect(const TileMap& map, int x, int y) {
    return {
        (float)(map.originX + x * map.tileSize),
        (float)(map.originY + y * map.tileSize),
        (float)map.tileSize,
        (float)map.tileSize
    };
}

inline Vector2 TileWorldCenter(const TileMap& map, int x, int y) {
    Rectangle rect = TileWorldRect(map, x, y);
    return { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
}
