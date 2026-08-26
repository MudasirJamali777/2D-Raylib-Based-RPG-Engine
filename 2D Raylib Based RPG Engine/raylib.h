#pragma once
#include <cstdarg>
#include <cstddef>

typedef struct Color { unsigned char r, g, b, a; } Color;
typedef struct Vector2 { float x, y; } Vector2;
typedef struct Rectangle { float x, y, width, height; } Rectangle;
typedef struct Camera2D { Vector2 offset; Vector2 target; float rotation; float zoom; } Camera2D;
typedef struct Texture { unsigned int id; int width; int height; int mipmaps; int format; } Texture2D;

static constexpr float DEG2RAD = 0.017453292519943295769f;
static constexpr int FLAG_MSAA_4X_HINT = 1;
static constexpr int FLAG_VSYNC_HINT = 2;
static constexpr int FLAG_FULLSCREEN_MODE = 4;
static constexpr int KEY_ENTER = 257;
static constexpr int KEY_SPACE = 32;
static constexpr int KEY_E = 69;
static constexpr int KEY_H = 72;
static constexpr int KEY_B = 66;
static constexpr int KEY_C = 67;
static constexpr int KEY_Q = 81;
static constexpr int KEY_R = 82;
static constexpr int KEY_W = 87;
static constexpr int KEY_A = 65;
static constexpr int KEY_S = 83;
static constexpr int KEY_D = 68;
static constexpr int KEY_ONE = 49;
static constexpr int KEY_TWO = 50;
static constexpr int KEY_THREE = 51;
static constexpr int KEY_FOUR = 52;
static constexpr int KEY_N = 78;
static constexpr int KEY_P = 80;
static constexpr int KEY_UP = 265;
static constexpr int KEY_DOWN = 264;
static constexpr int KEY_LEFT = 263;
static constexpr int KEY_RIGHT = 262;

static const Color WHITE{ 255,255,255,255 };
static const Color BLACK{ 0,0,0,255 };
static const Color GRAY{ 130,130,130,255 };
static const Color LIGHTGRAY{ 200,200,200,255 };
static const Color ORANGE{ 255,161,0,255 };
static const Color SKYBLUE{ 102,191,255,255 };
static const Color BLUE{ 0,121,241,255 };
static const Color RED{ 230,41,55,255 };
static const Color LIME{ 0,158,47,255 };
static const Color PURPLE{ 200,122,255,255 };
static const Color VIOLET{ 135,60,190,255 };
static const Color GOLD{ 255,203,0,255 };
static const Color YELLOW{ 253,249,0,255 };
static const Color GREEN{ 0,228,48,255 };
static const Color DARKBLUE{ 0,82,172,255 };
static const Color PINK{ 255,109,194,255 };
static const Color BROWN{ 127,106,79,255 };
static const Color MAROON{ 190,33,55,255 };
static const Color RAYWHITE{ 245,245,245,255 };

inline void SetConfigFlags(unsigned int) {}
inline void InitWindow(int, int, const char*) {}
inline int GetScreenWidth() { return 1280; }
inline int GetScreenHeight() { return 720; }
inline void SetTargetFPS(int) {}
inline bool IsWindowReady() { return true; }
inline void CloseWindow() {}
inline bool WindowShouldClose() { return true; }
inline float GetFrameTime() { return 0.016f; }
inline double GetTime() { return 0.0; }
inline bool IsKeyPressed(int) { return false; }
inline bool IsKeyDown(int) { return false; }
inline bool CheckCollisionPointRec(Vector2, Rectangle) { return false; }
inline Texture2D LoadTexture(const char*) { return Texture2D{ 1,64,64,1,0 }; }
inline void UnloadTexture(Texture2D) {}
inline Color Fade(Color c, float) { return c; }
inline void BeginDrawing() {}
inline void ClearBackground(Color) {}
inline void DrawRectangleGradientV(int, int, int, int, Color, Color) {}
inline void DrawCircleV(Vector2, float, Color) {}
inline void DrawRectangleRec(Rectangle, Color) {}
inline void DrawRectangleLinesEx(Rectangle, float, Color) {}
inline void DrawText(const char*, int, int, int, Color) {}
inline int MeasureText(const char*, int) { return 100; }
inline const char* TextFormat(const char* text, ...) { return text; }
inline void BeginMode2D(Camera2D) {}
inline void EndMode2D() {}
inline void DrawLine(int, int, int, int, Color) {}
inline void DrawLineEx(Vector2, Vector2, float, Color) {}
inline void DrawCircleLines(int, int, float, Color) {}
inline void DrawEllipse(int, int, float, float, Color) {}
inline void DrawRectangle(int, int, int, int, Color) {}
inline void DrawRectangleRounded(Rectangle, float, int, Color) {}
inline void DrawRectanglePro(Rectangle, Vector2, float, Color) {}
inline void DrawTexturePro(Texture2D, Rectangle, Rectangle, Vector2, float, Color) {}
inline void EndDrawing() {}
inline void DrawTriangle(Vector2, Vector2, Vector2, Color) {}
inline void DrawPoly(Vector2, int, float, float, Color) {}
