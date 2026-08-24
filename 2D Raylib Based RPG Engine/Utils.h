#pragma once
#include "raylib.h"
#include <cmath>
#include <cstdlib>

inline float ClampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

inline float LerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float RandomRange(float minValue, float maxValue) {
    return minValue + ((float)std::rand() / (float)RAND_MAX) * (maxValue - minValue);
}

inline Vector2 VecAdd(Vector2 a, Vector2 b) {
    return { a.x + b.x, a.y + b.y };
}

inline Vector2 VecSub(Vector2 a, Vector2 b) {
    return { a.x - b.x, a.y - b.y };
}

inline Vector2 VecScale(Vector2 v, float s) {
    return { v.x * s, v.y * s };
}

inline float VecLength(Vector2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

inline Vector2 VecNormalizeSafe(Vector2 v) {
    float len = VecLength(v);
    if (len <= 0.0001f) return { 0.0f, 0.0f };
    return { v.x / len, v.y / len };
}

inline float Distance(Vector2 a, Vector2 b) {
    return VecLength(VecSub(a, b));
}

inline Vector2 SafeZoneCenter(Rectangle rec) {
    return { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
}

inline void DrawGlowCircle(Vector2 pos, float radius, Color color) {
    for (int i = 4; i >= 1; --i) {
        DrawCircleV(pos, radius + i * 6.0f, Fade(color, 0.045f * (float)i));
    }
    DrawCircleV(pos, radius, color);
}

inline void DrawPanel(Rectangle rect, Color fill, Color border) {
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 2.0f, border);
}
