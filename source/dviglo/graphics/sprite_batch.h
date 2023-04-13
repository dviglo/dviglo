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
    none         = 0,
    horizontally = 1 << 0,
    vertically   = 1 << 1,
    both = horizontally | vertically
};
DV_FLAGS(FlipModes);

class DV_API SpriteBatch : public SpriteBatchBase
{
    DV_OBJECT(SpriteBatch);

public:

    SpriteBatch();

    // ============================ Рисование фигур с помощью функции add_triangle() ============================

public:

    void draw_triangle(const Vector2& v0, const Vector2& v1, const Vector2& v2);

    void draw_line(const Vector2& start, const Vector2&end, float width);
    void draw_line(float start_x, float start_y, float end_x, float end_y, float width);

    void draw_aabb_solid(const Vector2& min, const Vector2& max);

    void draw_aabox_solid(const Vector2& center_pos, const Vector2& half_size);
    void draw_aabox_solid(float center_x, float center_y, float half_width, float half_height);

    // Граница рисуется по внутреннему периметру (не выходит за пределы AABox)
    void draw_aabox_border(float center_x, float center_y, float half_width, float half_height, float border_width);

    void draw_circle(const Vector2& center_pos, float radius);
    void draw_circle(float center_x, float center_y, float radius);

    void draw_arrow(const Vector2& start, const Vector2& end, float width);

    // ========================== Рисование спрайтов и текста с помощью функции add_quad() ==========================

private:

    // Кэшированные шейдеры. Инициализируются в конструкторе
    ShaderVariation* sprite_vs_;
    ShaderVariation* sprite_ps_;
    ShaderVariation* ttf_text_vs_;
    ShaderVariation* ttf_text_ps_;
    ShaderVariation* sprite_text_vs_;
    ShaderVariation* sprite_text_ps_;
    ShaderVariation* sdf_text_vs_;
    ShaderVariation* sdf_text_ps_;
    ShaderVariation* shape_vs_;
    ShaderVariation* shape_ps_;

    // Данные для функции draw_sprite_internal()
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
    void draw_sprite_internal();

public:

    /// color - цвет в формате 0xAABBGGRR
    void draw_sprite(Texture2D* texture, const Rect& destination, const Rect* source = nullptr, u32 color = 0xFFFFFFFF,
        float rotation = 0.0f, const Vector2& origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, FlipModes flip_modes = FlipModes::none);

    /// color - цвет в формате 0xAABBGGRR
    void draw_sprite(Texture2D* texture, const Vector2& position, const Rect* source = nullptr, u32 color = 0xFFFFFFFF,
        float rotation = 0.0f, const Vector2 &origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, FlipModes flip_modes = FlipModes::none);

    /// color - цвет в формате 0xAABBGGRR
    void draw_string(const String& text, Font* font, float font_size, const Vector2& position, u32 color = 0xFFFFFFFF,
        float rotation = 0.0f, const Vector2& origin = Vector2::ZERO, const Vector2& scale = Vector2::ONE, FlipModes flip_modes = FlipModes::none);
};

} // namespace dviglo
