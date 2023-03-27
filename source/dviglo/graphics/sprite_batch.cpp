// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "sprite_batch.h"

#include "../ui/font_face.h"

namespace dviglo
{

SpriteBatch::SpriteBatch()
{
    Graphics& graphics = DV_GRAPHICS;

    spriteVS_ = graphics.GetShader(VS, "Basic", "DIFFMAP VERTEXCOLOR");
    spritePS_ = graphics.GetShader(PS, "Basic", "DIFFMAP VERTEXCOLOR");
    ttfTextVS_ = graphics.GetShader(VS, "Text");
    ttfTextPS_ = graphics.GetShader(PS, "Text", "ALPHAMAP");
    spriteTextVS_ = graphics.GetShader(VS, "Text");
    spriteTextPS_ = graphics.GetShader(PS, "Text");
    sdfTextVS_ = graphics.GetShader(VS, "Text");
    sdfTextPS_ = graphics.GetShader(PS, "Text", "SIGNED_DISTANCE_FIELD");
    shapeVS_ = graphics.GetShader(VS, "Basic", "VERTEXCOLOR");
    shapePS_ = graphics.GetShader(PS, "Basic", "VERTEXCOLOR");
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
        float invWidth = 1.0f / texture->GetWidth();
        float invHeight = 1.0f / texture->GetHeight();
        return Rect
        (
            source->min_.x * invWidth,
            source->min_.y * invHeight,
            source->max_.x * invWidth,
            source->max_.y * invHeight
        );
    }
}

void SpriteBatch::draw_sprite(Texture2D* texture, const Rect& destination, const Rect* source, u32 color,
    float rotation, const Vector2& origin, const Vector2& scale, FlipModes flipModes)
{
    if (!texture)
        return;

    sprite_.texture = texture;
    sprite_.vs = spriteVS_;
    sprite_.ps = spritePS_;
    sprite_.destination = destination;
    sprite_.source_uv = SrcToUV(source, texture);
    sprite_.flip_modes = flipModes;
    sprite_.scale = scale;
    sprite_.rotation = rotation;
    sprite_.origin = origin;
    sprite_.color0 = color;
    sprite_.color1 = color;
    sprite_.color2 = color;
    sprite_.color3 = color;

    DrawSpriteInternal();
}

void SpriteBatch::draw_sprite(Texture2D* texture, const Vector2& position, const Rect* source, u32 color,
    float rotation, const Vector2 &origin, const Vector2& scale, FlipModes flipModes)
{
    if (!texture)
        return;

    sprite_.texture = texture;
    sprite_.vs = spriteVS_;
    sprite_.ps = spritePS_;
    sprite_.destination = PosToDest(position, texture, source);
    sprite_.source_uv = SrcToUV(source, texture);
    sprite_.flip_modes = flipModes;
    sprite_.scale = scale;
    sprite_.rotation = rotation;
    sprite_.origin = origin;
    sprite_.color0 = color;
    sprite_.color1 = color;
    sprite_.color2 = color;
    sprite_.color3 = color;

    DrawSpriteInternal();
}

void SpriteBatch::DrawSpriteInternal()
{
    quad_.vs = sprite_.vs;
    quad_.ps = sprite_.ps;
    quad_.texture = sprite_.texture;

    // Если спрайт не отмасштабирован и не повёрнут, то прорисовка очень проста
    if (sprite_.rotation == 0.0f && sprite_.scale == Vector2::ONE)
    {
        // Сдвигаем спрайт на -origin
        Rect resultDest(sprite_.destination.min_ - sprite_.origin, sprite_.destination.max_ - sprite_.origin);

        // Лицевая грань задаётся по часовой стрелке. Учитываем, что ось Y направлена вниз.
        // Но нет большой разницы, так как спрайты двусторонние
        quad_.v0.position = Vector3(resultDest.min_.x, resultDest.min_.y, 0); // Верхний левый угол спрайта
        quad_.v1.position = Vector3(resultDest.max_.x, resultDest.min_.y, 0); // Верхний правый угол
        quad_.v2.position = Vector3(resultDest.max_.x, resultDest.max_.y, 0); // Нижний правый угол
        quad_.v3.position = Vector3(resultDest.min_.x, resultDest.max_.y, 0); // Нижний левый угол
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

        // transform * Vector3(local.min_.x, local.min_.y, 1.0f);
        quad_.v0.position = Vector3(minXm11 + minYm12 + m13,
                                    minXm21 + minYm22 + m23,
                                    0.0f);

        // transform * Vector3(local.max_.x, local.min_.y, 1.0f).
        quad_.v1.position = Vector3(maxXm11 + minYm12 + m13,
                                    maxXm21 + minYm22 + m23,
                                    0.0f);

        // transform * Vector3(local.max_.x, local.max_.y, 1.0f).
        quad_.v2.position = Vector3(maxXm11 + maxYm12 + m13,
                                    maxXm21 + maxYm22 + m23,
                                    0.0f);

        // transform * Vector3(local.min_.x, local.max_.y, 1.0f).
        quad_.v3.position = Vector3(minXm11 + maxYm12 + m13,
                                    minXm21 + maxYm22 + m23,
                                    0.0f);
    }

    if (!!(sprite_.flip_modes & FlipModes::Horizontally))
        std::swap(sprite_.source_uv.min_.x, sprite_.source_uv.max_.x);

    if (!!(sprite_.flip_modes & FlipModes::Vertically))
        std::swap(sprite_.source_uv.min_.y, sprite_.source_uv.max_.y);

    quad_.v0.color = sprite_.color0;
    quad_.v0.uv = sprite_.source_uv.min_;

    quad_.v1.color = sprite_.color1;
    quad_.v1.uv = Vector2(sprite_.source_uv.max_.x, sprite_.source_uv.min_.y);

    quad_.v2.color = sprite_.color2;
    quad_.v2.uv = sprite_.source_uv.max_;

    quad_.v3.color = sprite_.color3;
    quad_.v3.uv = Vector2(sprite_.source_uv.min_.x, sprite_.source_uv.max_.y);

    AddQuad();
}

void SpriteBatch::draw_string(const String& text, Font* font, float fontSize, const Vector2& position, u32 color,
    float rotation, const Vector2& origin, const Vector2& scale, FlipModes flipModes)
{
    if (text.Length() == 0)
        return;

    Vector<c32> unicodeText;
    for (i32 i = 0; i < text.Length();)
        unicodeText.Push(text.NextUTF8Char(i));

    if (font->GetFontType() == FONT_FREETYPE)
    {
        sprite_.vs = ttfTextVS_;
        sprite_.ps = ttfTextPS_;
    }
    else // FONT_BITMAP
    {
        if (font->IsSDFFont())
        {
            sprite_.vs = sdfTextVS_;
            sprite_.ps = sdfTextPS_;
        }
        else
        {
            sprite_.vs = spriteTextVS_;
            sprite_.ps = spriteTextPS_;
        }
    }

    sprite_.flip_modes = flipModes;
    sprite_.scale = scale;
    sprite_.rotation = rotation;
    sprite_.color0 = color;
    sprite_.color1 = color;
    sprite_.color2 = color;
    sprite_.color3 = color;

    FontFace* face = font->GetFace(fontSize);
    const Vector<SharedPtr<Texture2D>>& textures = face->GetTextures();
    // По идее все текстуры одинакового размера
    float pixelWidth = 1.0f / textures[0]->GetWidth();
    float pixelHeight = 1.0f / textures[0]->GetHeight();

    Vector2 charPos = position;
    Vector2 charOrig = origin;

    i32 i = 0;
    i32 step = 1;

    if (!!(flipModes & FlipModes::Horizontally))
    {
        i = unicodeText.Size() - 1;
        step = -1;
    }

    for (; i >= 0 && i < unicodeText.Size(); i += step)
    {
        const FontGlyph* glyph = face->GetGlyph(unicodeText[i]);
        float gx = (float)glyph->x;
        float gy = (float)glyph->y;
        float gw = (float)glyph->width;
        float gh = (float)glyph->height;
        float gox = (float)glyph->offset_x;
        float goy = (float)glyph->offset_y;

        sprite_.texture = textures[glyph->page];
        sprite_.destination = Rect(charPos.x, charPos.y, charPos.x + gw, charPos.y + gh);
        sprite_.source_uv = Rect(gx * pixelWidth, gy * pixelHeight, (gx + gw) * pixelWidth, (gy + gh) * pixelHeight);

        // Модифицируем origin, а не позицию, чтобы было правильное вращение
        sprite_.origin = !!(flipModes & FlipModes::Vertically) ? charOrig - Vector2(gox, face->GetRowHeight() - goy - gh) : charOrig - Vector2(gox, goy);

        DrawSpriteInternal();

        charOrig.x -= (float)glyph->advance_x;
    }
}

// В отличие от Sign() никогда не возвращает ноль
template <typename T>
T MySign(T value) { return value >= 0.0f ? 1.0f : -1.0f; }

void SpriteBatch::DrawTriangle(const Vector2& v0, const Vector2& v1, const Vector2& v2)
{
    triangle_.v0.position = Vector3(v0);
    triangle_.v1.position = Vector3(v1);
    triangle_.v2.position = Vector3(v2);
    AddTriangle();
}

void SpriteBatch::draw_line(const Vector2& start, const Vector2&end, float width)
{
    float len = (end - start).Length();
    if (Equals(len, 0.0f))
        return;

    // Линия - это прямоугольный полигон. Когда линия не повернута (угол поворота равен нулю), она горизонтальна.
    //   v0 ┌───────────────┐ v1
    //start ├───────────────┤ end
    //   v3 └───────────────┘ v2
    // Пользователь задает координаты точек start и end, а нам нужно определить координаты вершин v0, v1, v2, v3.
    // Легче всего вычислить СМЕЩЕНИЯ вершин v0 и v3 от точки start и смещения вершин v1 и v2 от точки end,
    // а потом прибавить эти смещения к координатам точек start и end.

    // Когда линия горизонтальна, v0 имеет смещение (0, -halfWidth) относительно точки start,
    // а вершина v3 имеет смещение (0, halfWidth) относительно той же точки start.
    // Аналогично v1 = (0, -halfWidth) и v2 = (0, halfWidth) относительно точки end.
    float halfWidth = Abs(width * 0.5f);

    // Так как мы оперируем смещениями, то при повороте линии вершины v0 и v3 вращаются вокруг start, а v1 и v2 - вокруг end.
    // Итак, вращаем точку v0 с локальными координатами (0, halfWidth).
    // {newX = oldX * cos(deltaAngle) - oldY * sin(deltaAngle) = 0 * cos(deltaAngle) - halfWidth * sin(deltaAngle)
    // {newY = oldX * sin(deltaAngle) + oldY * cos(deltaAngle) = 0 * sin(deltaAngle) + halfWidth * cos(deltaAngle)
    // Так как повернутая линия может оказаться в любом квадранте, при вычислениии синуса и косинуса нам важен знак.
    len = len * MySign(end.x - start.x) * MySign(end.y - start.y);
    float cos = (end.x - start.x) / len; // Прилежащий катет к гипотенузе.
    float sin = (end.y - start.y) / len; // Противолежащий катет к гипотенузе.
    Vector2 offset = Vector2(-halfWidth * sin, halfWidth * cos);

    // Так как противоположные стороны параллельны, то можно не делать повторных вычислений:
    // смещение v0 всегда равно смещению v1, смещение v3 = смещению v2.
    // К тому же смещения вершин v0, v1 отличаются от смещений вершин v3, v2 только знаком (противоположны).
    Vector2 v0 = Vector2(start.x + offset.x, start.y + offset.y);
    Vector2 v1 = Vector2(end.x + offset.x, end.y + offset.y);
    Vector2 v2 = Vector2(end.x - offset.x, end.y - offset.y);
    Vector2 v3 = Vector2(start.x - offset.x, start.y - offset.y);

    DrawTriangle(v0, v1, v2);
    DrawTriangle(v2, v3, v0);
}

void SpriteBatch::draw_line(float startX, float startY, float endX, float endY, float width)
{
    draw_line(Vector2(startX, startY), Vector2(endX, endY), width);
}

void SpriteBatch::DrawAABoxSolid(const Vector2& centerPos, const Vector2& halfSize)
{
    DrawAABoxSolid(centerPos.x, centerPos.y, halfSize.x, halfSize.y);
}

void SpriteBatch::DrawAABBSolid(const Vector2& min, const Vector2& max)
{
    Vector2 rightTop = Vector2(max.x, min.y); // Правый верхний угол
    Vector2 leftBot = Vector2(min.x, max.y); // Левый нижний

    DrawTriangle(min, rightTop, max);
    DrawTriangle(leftBot, min, max);
}

void SpriteBatch::DrawAABoxSolid(float centerX, float centerY, float halfWidth, float halfHeight)
{
    if (halfWidth < M_EPSILON || halfHeight < M_EPSILON)
        return;

    Vector2 v0 = Vector2(centerX - halfWidth, centerY - halfHeight); // Левый верхний угол
    Vector2 v1 = Vector2(centerX + halfWidth, centerY - halfHeight); // Правый верхний
    Vector2 v2 = Vector2(centerX + halfWidth, centerY + halfHeight); // Правый нижний
    Vector2 v3 = Vector2(centerX - halfWidth, centerY + halfHeight); // Левый нижний

    DrawTriangle(v0, v1, v2);
    DrawTriangle(v2, v3, v0);
}

void SpriteBatch::DrawAABoxBorder(float centerX, float centerY, float halfWidth, float halfHeight, float borderWidth)
{
    if (borderWidth < M_EPSILON || halfWidth < M_EPSILON || halfHeight < M_EPSILON)
        return;

    float halfBorderWidth = borderWidth * 0.5f;

    // Тут нужно обработать случай, когда толщина границы больше размера AABB

    // Верхняя граница
    float y = centerY - halfHeight + halfBorderWidth;
    draw_line(centerX - halfWidth, y, centerX + halfWidth, y, borderWidth);

    // Нижняя граница
    y = centerY + halfHeight - halfBorderWidth;
    draw_line(centerX - halfWidth, y, centerX + halfWidth, y, borderWidth);

    // При отрисовке боковых границ не перекрываем верхнюю и нижнюю, чтобы нормально отрисовывалось в случае полупрозрачного цвета

    // Левая граница
    float x = centerX - halfWidth + halfBorderWidth;
    draw_line(x, centerY - halfHeight + borderWidth, x, centerY + halfHeight - borderWidth, borderWidth);

    // Правая граница
    x = centerX + halfWidth - halfBorderWidth;
    draw_line(x, centerY - halfHeight + borderWidth, x, centerY + halfHeight - borderWidth, borderWidth);
}

void SpriteBatch::DrawCircle(const Vector2& centerPos, float radius)
{
    const int numPoints = 40;
    Vector2 points[numPoints];

    for (int i = 0; i < numPoints; ++i)
    {
        float angle = 360.0f * (float)i / (float)numPoints;
        float cos, sin;
        SinCos(angle, sin, cos);
        points[i] = Vector2(cos, sin) * radius + centerPos;
    }

    for (int i = 1; i < numPoints; ++i)
        DrawTriangle(points[i], points[i - 1], centerPos);

    // Рисуем последний сегмент
    DrawTriangle(points[0], points[numPoints - 1], centerPos);
}

void SpriteBatch::DrawCircle(float centerX, float centerY, float radius)
{
    DrawCircle(Vector2(centerX, centerY), radius);
}

// Поворачивает вектор по часовой стрелке на 90 градусов
static Vector2 RotatePlus90(const Vector2& v)
{
    Vector2 result(-v.y, v.x);
    return result;
}

// Поворачивает вектор по часовой стрелке на -90 градусов
static Vector2 RotateMinus90(const Vector2& v)
{
    Vector2 result(v.y, -v.x);
    return result;
}

void SpriteBatch::DrawArrow(const Vector2& start, const Vector2& end, float width)
{
    // TODO: настроить Doxygen на поддержку картинок и тут ссылку на картинку
    Vector2 vec = end - start;

    float len = vec.Length();
    if (len < M_EPSILON)
        return;

    Vector2 dir = vec.normalized();

    // TODO: Обработать случай, когда вектор короткий
    float headLen = width * 2; // Длина наконечника
    float shaftLen = len - headLen; // Длина древка
    Vector2 headStart = dir * shaftLen + start; // Начало наконечника
    Vector2 head = dir * headLen; // Вектор от точки headStart до точки end
    Vector2 headTop = RotateMinus90(head) + headStart;
    Vector2 headBottom = RotatePlus90(head) + headStart;
    draw_line(start, headStart, width);
    DrawTriangle(headStart, headTop, end);
    DrawTriangle(headStart, headBottom, end);
}

}
