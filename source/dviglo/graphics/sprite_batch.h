// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Работает в двух режимах - рендеринг фигур и рендеринг спрайтов.
// Если после рисования спрайтов рисуется какая-либо фигура (или наоборот) автоматически вызывается flush()

#pragma once

#include "sprite_batch_base.h"

#include "../ui/font.h"

namespace dviglo
{

/// Режимы зеркального отображения спрайтов и текста
enum class FlipModes : u32
{
    None = 0,
    Horizontally = 1,
    Vertically = 2,
    Both = Horizontally | Vertically
};
DV_FLAGS(FlipModes);

class DV_API SpriteBatch : public SpriteBatchBase
{
    DV_OBJECT(SpriteBatch, SpriteBatchBase);

    // ============================ Рисование фигур с помощью функции AddTriangle() ============================

public:

    void DrawTriangle(const Vector2& v0, const Vector2& v1, const Vector2& v2);

    void draw_line(const Vector2& start, const Vector2&end, float width);
    void draw_line(float startX, float startY, float endX, float endY, float width);

    void DrawAABBSolid(const Vector2& min, const Vector2& max);
    void DrawAABoxSolid(const Vector2& centerPos, const Vector2& halfSize);
    void DrawAABoxSolid(float centerX, float centerY, float halfWidth, float halfHeight);

    void DrawCircle(const Vector2& centerPos, float radius);
    void DrawCircle(float centerX, float centerY, float radius);

    // Граница рисуется по внутреннему периметру (не выходит за пределы AABox)
    void DrawAABoxBorder(float centerX, float centerY, float halfWidth, float halfHeight, float borderWidth);

    void DrawArrow(const Vector2& start, const Vector2& end, float width);

    // ========================== Рисование спрайтов и текста с помощью функции AddQuad() ==========================

public:

    /// color - цвет в формате 0xAABBGGRR
    void draw_sprite(Texture2D* texture, const Rect& destination, const Rect* source = nullptr, u32 color = 0xFFFFFFFF,
        float rotation = 0.0f, const Vector2& origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, FlipModes flipModes = FlipModes::None);

    /// color - цвет в формате 0xAABBGGRR
    void draw_sprite(Texture2D* texture, const Vector2& position, const Rect* source = nullptr, u32 color = 0xFFFFFFFF,
        float rotation = 0.0f, const Vector2 &origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, FlipModes flipModes = FlipModes::None);

    /// color - цвет в формате 0xAABBGGRR
    void draw_string(const String& text, Font* font, float fontSize, const Vector2& position, u32 color = 0xFFFFFFFF,
        float rotation = 0.0f, const Vector2& origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, FlipModes flipModes = FlipModes::None);

private:

    // Кэширование шейдеров. Инициализируются в конструкторе
    ShaderVariation* spriteVS_;
    ShaderVariation* spritePS_;
    ShaderVariation* ttfTextVS_;
    ShaderVariation* ttfTextPS_;
    ShaderVariation* spriteTextVS_;
    ShaderVariation* spriteTextPS_;
    ShaderVariation* sdfTextVS_;
    ShaderVariation* sdfTextPS_;
    ShaderVariation* shapeVS_;
    ShaderVariation* shapePS_;

    // Данные для функции DrawSpriteInternal()
    struct
    {
        Texture2D* texture;
        ShaderVariation* vs;
        ShaderVariation* ps;
        Rect destination;
        Rect source_uv; // Текстурные координаты в диапазоне [0, 1]
        FlipModes flip_modes;
        Vector2 scale;
        float rotation;
        Vector2 origin;
        u32 color0;
        u32 color1;
        u32 color2;
        u32 color3;
    } sprite_;

    // Перед вызовом этой функции нужно заполнить структуру sprite_
    void DrawSpriteInternal();

    // ========================================= Остальное =========================================

public:

    SpriteBatch();
};

}
