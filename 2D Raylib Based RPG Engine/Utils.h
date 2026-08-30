#pragma once
#include <raylib.h>
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

inline float VecLengthSqr(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

inline Vector2 VecNormalizeSafe(Vector2 v) {
    float len = VecLength(v);
    if (len <= 0.0001f) return { 0.0f, 0.0f };
    return { v.x / len, v.y / len };
}

inline float Distance(Vector2 a, Vector2 b) {
    return VecLength(VecSub(a, b));
}

inline float DistanceSqr(Vector2 a, Vector2 b) {
    return VecLengthSqr(VecSub(a, b));
}

inline bool WithinRange(Vector2 a, Vector2 b, float range) {
    return DistanceSqr(a, b) <= range * range;
}

inline Vector2 SafeZoneCenter(Rectangle rec) {
    return { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
}

inline void DrawGlowCircle(Vector2 pos, float radius, Color color) {
    for (int i = 4; i >= 1; --i) {
        DrawCircleV(pos, radius + i * 5.0f, Fade(color, 0.03f * (float)i));
    }
    DrawCircleV(pos, radius, color);
}

inline void DrawPanel(Rectangle rect, Color fill, Color border) {
    DrawRectangleRounded({ rect.x + 8.0f, rect.y + 10.0f, rect.width, rect.height }, 0.06f, 8, Fade(BLACK, 0.34f));
    DrawRectangleRounded(rect, 0.06f, 8, fill);
    DrawRectangleRounded({ rect.x + 3.0f, rect.y + 3.0f, rect.width - 6.0f, rect.height - 6.0f }, 0.06f, 8, Fade(WHITE, 0.015f));
    DrawRectangleRounded({ rect.x + 5.0f, rect.y + 5.0f, rect.width - 10.0f, rect.height * 0.22f }, 0.06f, 8, Fade(WHITE, 0.035f));
    DrawRectangle((int)rect.x + 10, (int)rect.y + 34, (int)rect.width - 20, 1, Fade(border, 0.28f));
    DrawRectangleLinesEx({ rect.x + 3.0f, rect.y + 3.0f, rect.width - 6.0f, rect.height - 6.0f }, 1.0f, Fade(BLACK, 0.32f));
    DrawRectangleLinesEx(rect, 2.0f, border);
}

inline void DrawCartoonShadow(Vector2 pos, float radiusX, float radiusY, float alpha) {
    DrawEllipse((int)pos.x, (int)pos.y, radiusX, radiusY, Fade(BLACK, alpha));
}

inline void DrawTreeProp(Vector2 pos, float scale, Color leafColor) {
    DrawCartoonShadow({ pos.x, pos.y + 18.0f * scale }, 16.0f * scale, 8.0f * scale, 0.18f);
    DrawRectangleRounded({ pos.x - 6.0f * scale, pos.y - 6.0f * scale, 12.0f * scale, 24.0f * scale }, 0.25f, 6, { 121, 84, 52, 255 });
    DrawCircleV({ pos.x, pos.y - 18.0f * scale }, 18.0f * scale, { 39, 115, 48, 255 });
    DrawCircleV({ pos.x - 12.0f * scale, pos.y - 12.0f * scale }, 13.0f * scale, leafColor);
    DrawCircleV({ pos.x + 12.0f * scale, pos.y - 13.0f * scale }, 13.0f * scale, leafColor);
    DrawCircleV({ pos.x, pos.y - 4.0f * scale }, 16.0f * scale, leafColor);
}

inline void DrawBushProp(Vector2 pos, float scale) {
    DrawCartoonShadow({ pos.x, pos.y + 8.0f * scale }, 14.0f * scale, 6.0f * scale, 0.14f);
    DrawCircleV({ pos.x - 10.0f * scale, pos.y }, 10.0f * scale, { 69, 153, 67, 255 });
    DrawCircleV({ pos.x, pos.y - 4.0f * scale }, 12.0f * scale, { 78, 172, 74, 255 });
    DrawCircleV({ pos.x + 10.0f * scale, pos.y }, 10.0f * scale, { 69, 153, 67, 255 });
}

inline void DrawRockProp(Vector2 pos, float scale) {
    DrawCartoonShadow({ pos.x, pos.y + 10.0f * scale }, 12.0f * scale, 5.0f * scale, 0.16f);
    DrawTriangle({ pos.x - 14.0f * scale, pos.y + 8.0f * scale }, { pos.x - 2.0f * scale, pos.y - 14.0f * scale }, { pos.x + 15.0f * scale, pos.y + 10.0f * scale }, { 163, 154, 133, 255 });
    DrawTriangle({ pos.x - 9.0f * scale, pos.y + 6.0f * scale }, { pos.x + 3.0f * scale, pos.y - 8.0f * scale }, { pos.x + 10.0f * scale, pos.y + 6.0f * scale }, { 202, 194, 171, 255 });
}

inline void DrawHutProp(Vector2 pos, float scale) {
    DrawCartoonShadow({ pos.x, pos.y + 22.0f * scale }, 24.0f * scale, 8.0f * scale, 0.18f);
    DrawRectangleRounded({ pos.x - 22.0f * scale, pos.y - 6.0f * scale, 44.0f * scale, 28.0f * scale }, 0.18f, 8, { 161, 130, 75, 255 });
    DrawTriangle({ pos.x - 28.0f * scale, pos.y - 2.0f * scale }, { pos.x, pos.y - 26.0f * scale }, { pos.x + 28.0f * scale, pos.y - 2.0f * scale }, { 171, 141, 67, 255 });
    DrawRectangleRounded({ pos.x - 7.0f * scale, pos.y + 4.0f * scale, 14.0f * scale, 18.0f * scale }, 0.25f, 6, { 100, 71, 44, 255 });
}

inline void DrawTowerProp(Vector2 pos, float scale, Color flagColor) {
    DrawCartoonShadow({ pos.x, pos.y + 24.0f * scale }, 18.0f * scale, 7.0f * scale, 0.18f);
    DrawRectangleRounded({ pos.x - 15.0f * scale, pos.y - 6.0f * scale, 30.0f * scale, 40.0f * scale }, 0.25f, 8, { 170, 170, 176, 255 });
    DrawTriangle({ pos.x - 18.0f * scale, pos.y - 4.0f * scale }, { pos.x, pos.y - 24.0f * scale }, { pos.x + 18.0f * scale, pos.y - 4.0f * scale }, { 119, 86, 67, 255 });
    DrawRectangleRounded({ pos.x - 5.0f * scale, pos.y + 12.0f * scale, 10.0f * scale, 16.0f * scale }, 0.25f, 4, { 107, 77, 53, 255 });
    DrawLineEx({ pos.x + 10.0f * scale, pos.y - 24.0f * scale }, { pos.x + 10.0f * scale, pos.y - 42.0f * scale }, 2.0f, { 88, 72, 60, 255 });
    DrawTriangle({ pos.x + 10.0f * scale, pos.y - 42.0f * scale }, { pos.x + 24.0f * scale, pos.y - 36.0f * scale }, { pos.x + 10.0f * scale, pos.y - 28.0f * scale }, flagColor);
}

inline void DrawCastleProp(Vector2 pos, float scale) {
    DrawCartoonShadow({ pos.x, pos.y + 34.0f * scale }, 44.0f * scale, 12.0f * scale, 0.18f);
    DrawRectangleRounded({ pos.x - 36.0f * scale, pos.y - 4.0f * scale, 72.0f * scale, 48.0f * scale }, 0.10f, 8, { 176, 176, 183, 255 });
    DrawRectangleRounded({ pos.x - 58.0f * scale, pos.y - 10.0f * scale, 22.0f * scale, 54.0f * scale }, 0.18f, 8, { 165, 165, 173, 255 });
    DrawRectangleRounded({ pos.x + 36.0f * scale, pos.y - 10.0f * scale, 22.0f * scale, 54.0f * scale }, 0.18f, 8, { 165, 165, 173, 255 });
    DrawRectangleRounded({ pos.x - 10.0f * scale, pos.y - 20.0f * scale, 20.0f * scale, 60.0f * scale }, 0.18f, 8, { 160, 160, 168, 255 });
    DrawTriangle({ pos.x - 58.0f * scale, pos.y - 10.0f * scale }, { pos.x - 47.0f * scale, pos.y - 34.0f * scale }, { pos.x - 36.0f * scale, pos.y - 10.0f * scale }, { 88, 128, 204, 255 });
    DrawTriangle({ pos.x + 36.0f * scale, pos.y - 10.0f * scale }, { pos.x + 47.0f * scale, pos.y - 34.0f * scale }, { pos.x + 58.0f * scale, pos.y - 10.0f * scale }, { 88, 128, 204, 255 });
    DrawTriangle({ pos.x - 10.0f * scale, pos.y - 20.0f * scale }, { pos.x, pos.y - 44.0f * scale }, { pos.x + 10.0f * scale, pos.y - 20.0f * scale }, { 88, 128, 204, 255 });
    DrawRectangleRounded({ pos.x - 10.0f * scale, pos.y + 14.0f * scale, 20.0f * scale, 30.0f * scale }, 0.35f, 6, { 120, 85, 56, 255 });
    DrawRectangleRounded({ pos.x - 12.0f * scale, pos.y - 2.0f * scale, 24.0f * scale, 14.0f * scale }, 0.16f, 6, { 167, 58, 66, 255 });
}
