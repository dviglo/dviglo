// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "sprite_batch.h"

#include "../ui/font_face.h"

using namespace std;

namespace dviglo
{

SpriteBatch::SpriteBatch()
{
    Graphics* graphics = DV_GRAPHICS;

    sprite_vs_ = graphics->GetShader(VS, "basic", "DIFFMAP VERTEXCOLOR");
    sprite_ps_ = graphics->GetShader(PS, "basic", "DIFFMAP VERTEXCOLOR");
    ttf_text_vs_ = graphics->GetShader(VS, "text");
    ttf_text_ps_ = graphics->GetShader(PS, "text", "ALPHAMAP");
    sprite_text_vs_ = graphics->GetShader(VS, "text");
    sprite_text_ps_ = graphics->GetShader(PS, "text");
    sdf_text_vs_ = graphics->GetShader(VS, "text");
    sdf_text_ps_ = graphics->GetShader(PS, "text", "SIGNED_DISTANCE_FIELD");
    shape_vs_ = graphics->GetShader(VS, "basic", "VERTEXCOLOR");
    shape_ps_ = graphics->GetShader(PS, "basic", "VERTEXCOLOR");
}

static Rect PosToDest(const Vector2& position, Texture2D* texture, const Rect* src)
{
    if (src == nullptr)
    {
        // Проверки не производятся, текстура должна быть корректной
        return Rect
        (
            position.x,
            position.y,
            position.x + texture->GetWidth(),
            position.y + texture->GetHeight()
        );
    }
    else
    {
        return Rect
        (
            position.x,
            position.y,
            position.x + (src->Right() - src->Left()), // Сперва вычисляем размер, так как там вероятно более близкие
            position.y + (src->Bottom() - src->Top()) // значения и меньше ошибка вычислений
        );
    }
}

// Преобразует пиксельные координаты в диапазон [0, 1]
static Rect SrcToUV(const Rect* source, Texture2D* texture)
{
    if (source == nullptr)
    {
        return Rect(Vector2::ZERO, Vector2::ONE);
    }
    else
    {
        // Проверки не производятся, текстура должна быть корректной
        float inv_width = 1.f / texture->GetWidth();
        float inv_height = 1.f / texture->GetHeight();
        return Rect
        (
            source->min_.x * inv_width,
            source->min_.y * inv_height,
            source->max_.x * inv_width,
            source->max_.y * inv_height
        );
    }
}

void SpriteBatch::draw_sprite(Texture2D* texture, const Rect& destination, const Rect* source, u32 color,
    float rotation, const Vector2& origin, const Vector2& scale, FlipModes flip_modes)
{
    if (!texture)
        return;

    sprite_.texture = texture;
    sprite_.vs = sprite_vs_;
    sprite_.ps = sprite_ps_;
    sprite_.destination = destination;
    sprite_.source_uv = SrcToUV(source, texture);
    sprite_.flip_modes = flip_modes;
    sprite_.scale = scale;
    sprite_.rotation = rotation;
    sprite_.origin = origin;
    sprite_.color0 = color;
    sprite_.color1 = color;
    sprite_.color2 = color;
    sprite_.color3 = color;

    draw_sprite_internal();
}

void SpriteBatch::draw_sprite(Texture2D* texture, const Vector2& position, const Rect* source, u32 color,
    float rotation, const Vector2 &origin, const Vector2& scale, FlipModes flip_modes)
{
    if (!texture)
        return;

    sprite_.texture = texture;
    sprite_.vs = sprite_vs_;
    sprite_.ps = sprite_ps_;
    sprite_.destination = PosToDest(position, texture, source);
    sprite_.source_uv = SrcToUV(source, texture);
    sprite_.flip_modes = flip_modes;
    sprite_.scale = scale;
    sprite_.rotation = rotation;
    sprite_.origin = origin;
    sprite_.color0 = color;
    sprite_.color1 = color;
    sprite_.color2 = color;
    sprite_.color3 = color;

    draw_sprite_internal();
}

void SpriteBatch::draw_sprite_internal()
{
    quad_.vs = sprite_.vs;
    quad_.ps = sprite_.ps;
    quad_.texture = sprite_.texture;

    // Если спрайт не отмасштабирован и не повёрнут, то прорисовка очень проста
    if (sprite_.rotation == 0.f && sprite_.scale == Vector2::ONE)
    {
        // Сдвигаем спрайт на -origin
        Rect resultDest(sprite_.destination.min_ - sprite_.origin, sprite_.destination.max_ - sprite_.origin);

        // Лицевая грань задаётся по часовой стрелке. Учитываем, что ось Y направлена вниз.
        // Но нет большой разницы, так как спрайты двусторонние
        quad_.v0.position = Vector3(resultDest.min_.x, resultDest.min_.y, 0.f); // Верхний левый угол спрайта
        quad_.v1.position = Vector3(resultDest.max_.x, resultDest.min_.y, 0.f); // Верхний правый угол
        quad_.v2.position = Vector3(resultDest.max_.x, resultDest.max_.y, 0.f); // Нижний правый угол
        quad_.v3.position = Vector3(resultDest.min_.x, resultDest.max_.y, 0.f); // Нижний левый угол
    }
    else
    {
        // Масштабировать и вращать необходимо относительно центра локальных координат:
        // 1) При стандартном origin == Vector2::ZERO, который соответствует верхнему левому углу спрайта,
        //    локальные координаты будут Rect(ноль, размерыСпрайта),
        //    то есть Rect(Vector2::ZERO, destination.max_ - destination.min_)
        // 2) При ненулевом origin нужно сдвинуть на -origin
        Rect local(-sprite_.origin, sprite_.destination.max_ - sprite_.destination.min_ - sprite_.origin);

        float sin, cos;
        SinCos(sprite_.rotation, sin, cos);

        // Нам нужна матрица, которая масштабирует и поворачивает вершину в локальных координатах, а затем
        // смещает ее в требуемые мировые координаты.
        // Но в матрице 3x3 последняя строка "0 0 1", умножать на которую бессмысленно.
        // Поэтому вычисляем без матрицы для оптимизации
        float m11 = cos * sprite_.scale.x; float m12 = -sin * sprite_.scale.y; float m13 = sprite_.destination.min_.x;
        float m21 = sin * sprite_.scale.x; float m22 =  cos * sprite_.scale.y; float m23 = sprite_.destination.min_.y;
        //          0                                   0                                  1

        float minXm11 = local.min_.x * m11;
        float minXm21 = local.min_.x * m21;
        float maxXm11 = local.max_.x * m11;
        float maxXm21 = local.max_.x * m21;
        float minYm12 = local.min_.y * m12;
        float minYm22 = local.min_.y * m22;
        float maxYm12 = local.max_.y * m12;
        float maxYm22 = local.max_.y * m22;

        // transform * Vector3(local.min_.x, local.min_.y, 1.f)
        quad_.v0.position = Vector3(minXm11 + minYm12 + m13,
                                    minXm21 + minYm22 + m23,
                                    0.f);

        // transform * Vector3(local.max_.x, local.min_.y, 1.f)
        quad_.v1.position = Vector3(maxXm11 + minYm12 + m13,
                                    maxXm21 + minYm22 + m23,
                                    0.f);

        // transform * Vector3(local.max_.x, local.max_.y, 1.f)
        quad_.v2.position = Vector3(maxXm11 + maxYm12 + m13,
                                    maxXm21 + maxYm22 + m23,
                                    0.f);

        // transform * Vector3(local.min_.x, local.max_.y, 1.f)
        quad_.v3.position = Vector3(minXm11 + maxYm12 + m13,
                                    minXm21 + maxYm22 + m23,
                                    0.f);
    }

    if (!!(sprite_.flip_modes & FlipModes::horizontally))
        swap(sprite_.source_uv.min_.x, sprite_.source_uv.max_.x);

    if (!!(sprite_.flip_modes & FlipModes::vertically))
        swap(sprite_.source_uv.min_.y, sprite_.source_uv.max_.y);

    quad_.v0.color = sprite_.color0;
    quad_.v0.uv = sprite_.source_uv.min_;

    quad_.v1.color = sprite_.color1;
    quad_.v1.uv = Vector2(sprite_.source_uv.max_.x, sprite_.source_uv.min_.y);

    quad_.v2.color = sprite_.color2;
    quad_.v2.uv = sprite_.source_uv.max_;

    quad_.v3.color = sprite_.color3;
    quad_.v3.uv = Vector2(sprite_.source_uv.min_.x, sprite_.source_uv.max_.y);

    add_quad();
}

void SpriteBatch::draw_string(const String& text, Font* font, float font_size, const Vector2& position, u32 color,
    float rotation, const Vector2& origin, const Vector2& scale, FlipModes flip_modes)
{
    if (text.Length() == 0)
        return;

    Vector<c32> unicode_text;
    for (i32 i = 0; i < text.Length();)
        unicode_text.Push(text.NextUTF8Char(i));

    if (font->GetFontType() == FONT_FREETYPE)
    {
        sprite_.vs = ttf_text_vs_;
        sprite_.ps = ttf_text_ps_;
    }
    else // FONT_BITMAP
    {
        if (font->IsSDFFont())
        {
            sprite_.vs = sdf_text_vs_;
            sprite_.ps = sdf_text_ps_;
        }
        else
        {
            sprite_.vs = sprite_text_vs_;
            sprite_.ps = sprite_text_ps_;
        }
    }

    sprite_.flip_modes = flip_modes;
    sprite_.scale = scale;
    sprite_.rotation = rotation;
    sprite_.color0 = color;
    sprite_.color1 = color;
    sprite_.color2 = color;
    sprite_.color3 = color;

    FontFace* face = font->GetFace(font_size);
    const Vector<SharedPtr<Texture2D>>& textures = face->GetTextures();
    // По идее все текстуры одинакового размера
    float pixel_width = 1.f / textures[0]->GetWidth();
    float pixel_height = 1.f / textures[0]->GetHeight();

    Vector2 char_pos = position;
    Vector2 char_orig = origin;

    i32 i = 0;
    i32 step = 1;

    if (!!(flip_modes & FlipModes::horizontally))
    {
        i = unicode_text.Size() - 1;
        step = -1;
    }

    for (; i >= 0 && i < unicode_text.Size(); i += step)
    {
        const FontGlyph* glyph = face->GetGlyph(unicode_text[i]);
        float gx = (float)glyph->x;
        float gy = (float)glyph->y;
        float gw = (float)glyph->width;
        float gh = (float)glyph->height;
        float gox = (float)glyph->offset_x;
        float goy = (float)glyph->offset_y;

        sprite_.texture = textures[glyph->page];
        sprite_.destination = Rect(char_pos.x, char_pos.y, char_pos.x + gw, char_pos.y + gh);
        sprite_.source_uv = Rect(gx * pixel_width, gy * pixel_height, (gx + gw) * pixel_width, (gy + gh) * pixel_height);

        // Модифицируем origin, а не позицию, чтобы было правильное вращение
        sprite_.origin = !!(flip_modes & FlipModes::vertically) ? char_orig - Vector2(gox, face->GetRowHeight() - goy - gh) : char_orig - Vector2(gox, goy);

        draw_sprite_internal();

        char_orig.x -= (float)glyph->advance_x;
    }
}

// В отличие от Sign() никогда не возвращает ноль
float my_sign(float value) { return value >= 0.f ? 1.f : -1.f; }

void SpriteBatch::draw_triangle(const Vector2& v0, const Vector2& v1, const Vector2& v2)
{
    triangle_.v0.position = Vector3(v0);
    triangle_.v1.position = Vector3(v1);
    triangle_.v2.position = Vector3(v2);
    add_triangle();
}

void SpriteBatch::draw_line(const Vector2& start, const Vector2&end, float width)
{
    float len = (end - start).Length();
    if (Equals(len, 0.f))
        return;

    // Линия - это прямоугольный полигон. Когда линия не повернута (угол поворота равен нулю), она горизонтальна.
    //   v0 ┌───────────────┐ v1
    //start ├───────────────┤ end
    //   v3 └───────────────┘ v2
    // Пользователь задает координаты точек start и end, а нам нужно определить координаты вершин v0, v1, v2, v3.
    // Легче всего вычислить СМЕЩЕНИЯ вершин v0 и v3 от точки start и смещения вершин v1 и v2 от точки end,
    // а потом прибавить эти смещения к координатам точек start и end

    // Когда линия горизонтальна, v0 имеет смещение (0, -half_width) относительно точки start,
    // а вершина v3 имеет смещение (0, half_width) относительно той же точки start.
    // Аналогично v1 = (0, -half_width) и v2 = (0, half_width) относительно точки end
    float half_width = Abs(width * 0.5f);

    // Так как мы оперируем смещениями, то при повороте линии вершины v0 и v3 вращаются вокруг start, а v1 и v2 - вокруг end.
    // Итак, вращаем точку v0 с локальными координатами (0, half_width).
    // {newX = oldX * cos(deltaAngle) - oldY * sin(deltaAngle) = 0 * cos(deltaAngle) - half_width * sin(deltaAngle)
    // {newY = oldX * sin(deltaAngle) + oldY * cos(deltaAngle) = 0 * sin(deltaAngle) + half_width * cos(deltaAngle)
    // Так как повернутая линия может оказаться в любом квадранте, при вычислениии синуса и косинуса нам важен знак
    len = len * my_sign(end.x - start.x) * my_sign(end.y - start.y);
    float cos = (end.x - start.x) / len; // Прилежащий катет к гипотенузе
    float sin = (end.y - start.y) / len; // Противолежащий катет к гипотенузе
    Vector2 offset = Vector2(-half_width * sin, half_width * cos);

    // Так как противоположные стороны параллельны, то можно не делать повторных вычислений:
    // смещение v0 всегда равно смещению v1, смещение v3 = смещению v2.
    // К тому же смещения вершин v0, v1 отличаются от смещений вершин v3, v2 только знаком (противоположны)
    Vector2 v0 = Vector2(start.x + offset.x, start.y + offset.y);
    Vector2 v1 = Vector2(end.x + offset.x, end.y + offset.y);
    Vector2 v2 = Vector2(end.x - offset.x, end.y - offset.y);
    Vector2 v3 = Vector2(start.x - offset.x, start.y - offset.y);

    draw_triangle(v0, v1, v2);
    draw_triangle(v2, v3, v0);
}

void SpriteBatch::draw_line(float start_x, float start_y, float end_x, float end_y, float width)
{
    draw_line(Vector2(start_x, start_y), Vector2(end_x, end_y), width);
}

void SpriteBatch::draw_aabox_solid(const Vector2& center_pos, const Vector2& half_size)
{
    draw_aabox_solid(center_pos.x, center_pos.y, half_size.x, half_size.y);
}

void SpriteBatch::draw_aabb_solid(const Vector2& min, const Vector2& max)
{
    Vector2 right_top = Vector2(max.x, min.y); // Правый верхний угол
    Vector2 left_bot = Vector2(min.x, max.y); // Левый нижний

    draw_triangle(min, right_top, max);
    draw_triangle(left_bot, min, max);
}

void SpriteBatch::draw_aabox_solid(float center_x, float center_y, float half_width, float half_height)
{
    if (half_width < M_EPSILON || half_height < M_EPSILON)
        return;

    Vector2 v0 = Vector2(center_x - half_width, center_y - half_height); // Левый верхний угол
    Vector2 v1 = Vector2(center_x + half_width, center_y - half_height); // Правый верхний
    Vector2 v2 = Vector2(center_x + half_width, center_y + half_height); // Правый нижний
    Vector2 v3 = Vector2(center_x - half_width, center_y + half_height); // Левый нижний

    draw_triangle(v0, v1, v2);
    draw_triangle(v2, v3, v0);
}

void SpriteBatch::draw_aabox_border(float center_x, float center_y, float half_width, float half_height, float border_width)
{
    if (border_width < M_EPSILON || half_width < M_EPSILON || half_height < M_EPSILON)
        return;

    float half_border_width = border_width * 0.5f;

    // Тут нужно обработать случай, когда толщина границы больше размера AABB

    // Верхняя граница
    float y = center_y - half_height + half_border_width;
    draw_line(center_x - half_width, y, center_x + half_width, y, border_width);

    // Нижняя граница
    y = center_y + half_height - half_border_width;
    draw_line(center_x - half_width, y, center_x + half_width, y, border_width);

    // При отрисовке боковых границ не перекрываем верхнюю и нижнюю, чтобы нормально отрисовывалось в случае полупрозрачного цвета

    // Левая граница
    float x = center_x - half_width + half_border_width;
    draw_line(x, center_y - half_height + border_width, x, center_y + half_height - border_width, border_width);

    // Правая граница
    x = center_x + half_width - half_border_width;
    draw_line(x, center_y - half_height + border_width, x, center_y + half_height - border_width, border_width);
}

void SpriteBatch::draw_circle(const Vector2& center_pos, float radius)
{
    const int num_points = 40;
    Vector2 points[num_points];

    for (int i = 0; i < num_points; ++i)
    {
        float angle = 360.f * (float)i / (float)num_points;
        float cos, sin;
        SinCos(angle, sin, cos);
        points[i] = Vector2(cos, sin) * radius + center_pos;
    }

    for (int i = 1; i < num_points; ++i)
        draw_triangle(points[i], points[i - 1], center_pos);

    // Рисуем последний сегмент
    draw_triangle(points[0], points[num_points - 1], center_pos);
}

void SpriteBatch::draw_circle(float center_x, float center_y, float radius)
{
    draw_circle(Vector2(center_x, center_y), radius);
}

// Поворачивает вектор по часовой стрелке на 90 градусов
static Vector2 rotate_plus_90(const Vector2& v)
{
    Vector2 result(-v.y, v.x);
    return result;
}

// Поворачивает вектор по часовой стрелке на -90 градусов
static Vector2 rotate_minus_90(const Vector2& v)
{
    Vector2 result(v.y, -v.x);
    return result;
}

void SpriteBatch::draw_arrow(const Vector2& start, const Vector2& end, float width)
{
    // TODO: настроить Doxygen на поддержку картинок и тут ссылку на картинку
    Vector2 vec = end - start;

    float len = vec.Length();
    if (len < M_EPSILON)
        return;

    Vector2 dir = vec.normalized();

    // TODO: Обработать случай, когда вектор короткий
    float head_len = width * 2; // Длина наконечника
    float shaft_len = len - head_len; // Длина древка
    Vector2 head_start = dir * shaft_len + start; // Начало наконечника
    Vector2 head = dir * head_len; // Вектор от точки head_start до точки end
    Vector2 head_top = rotate_minus_90(head) + head_start;
    Vector2 head_bottom = rotate_plus_90(head) + head_start;
    draw_line(start, head_start, width);
    draw_triangle(head_start, head_top, end);
    draw_triangle(head_start, head_bottom, end);
}

} // namespace dviglo
