// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "../io/log.h"
#include "../core/context.h"
#include "../scene/node.h"
#include "sprite_2d.h"
#include "stretchable_sprite_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

namespace
{

void checkBorder(int border, float drawSize)
{
    /* not clamping yet, as drawSize may still change and come to accommodate large borders */
    if (border < 0 || border * PIXEL_SIZE > drawSize)
        DV_LOGWARNINGF("Border out of bounds (%d), may be clamped", border);
}

Rect calcEffectiveBorder(const IntRect& border, const Vector2& drawSize)
{
    Vector2 min{Clamp(border.left_ * PIXEL_SIZE, 0.0f, drawSize.x),
                Clamp(border.bottom_ * PIXEL_SIZE, 0.0f, drawSize.y)};
    return Rect{min, {Clamp(border.right_ * PIXEL_SIZE, 0.0f, drawSize.x - min.x),
                      Clamp(border.top_ * PIXEL_SIZE, 0.0f, drawSize.y - min.y)} /* max*/ };
}

void prepareXYCoords(float coords[4], float low, float high, float lowBorder, float highBorder, float scale)
{
    coords[0] = low * scale;
    coords[3] = high * scale;

    auto scaleSign = Sign(scale);
    auto borderSize = lowBorder + highBorder;
    if (borderSize > scaleSign * (coords[3] - coords[0]))
    {
        auto size = high - low;
        coords[1] = coords[2] = scale * (low + (lowBorder * size / borderSize));
    }
    else
    {
        auto absScale = Abs(scale);
        coords[1] = (low * absScale + lowBorder) * scaleSign;
        coords[2] = (high * absScale - highBorder) * scaleSign;
    }
}

void prepareUVCoords(float coords[4], float low, float high, float lowBorder, float highBorder, float drawSize)
{
    coords[0] = low;
    coords[1] = low + lowBorder / drawSize;
    coords[2] = high - highBorder / drawSize;
    coords[3] = high;
}

void prepareVertices(Vertex2D vtx[4][4], const float xs[4], const float ys[4], const float us[4], const float vs[4], color32 color,
    const Vector3& position, const Quaternion& rotation)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        for (unsigned j = 0; j < 4; ++j)
        {
            vtx[i][j].position_ = position + rotation * Vector3{xs[i], ys[j], 0.0f};
            vtx[i][j].color_ = color;
            vtx[i][j].uv_ = Vector2{us[i], vs[j]};
        }
    }
}

void pushVertices(Vector<Vertex2D>& target, const Vertex2D source[4][4])
{
    for (unsigned i = 0; i < 3; ++i) // iterate over 3 columns
    {
        if (!Equals(source[i][0].position_.x,
            source[i + 1][0].position_.x)) // if width != 0
        {
            for (unsigned j = 0; j < 3; ++j) // iterate over 3 lines
            {
                if (!Equals(source[0][j].position_.y,
                    source[0][j + 1].position_.y)) // if height != 0
                {
                    target.Push(source[i][j]);          // V0 in V1---V2
                    target.Push(source[i][j + 1]);      // V1 in |   / |
                    target.Push(source[i + 1][j + 1]);  // V2 in | /   |
                    target.Push(source[i + 1][j]);      // V3 in V0---V3
                }
            }
        }
    }
}

} // namespace

extern const char* URHO2D_CATEGORY;

StretchableSprite2D::StretchableSprite2D()
{
}

void StretchableSprite2D::register_object()
{
    DV_CONTEXT->RegisterFactory<StretchableSprite2D>(URHO2D_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(StaticSprite2D);
    DV_ACCESSOR_ATTRIBUTE("Border", GetBorder, SetBorder, IntRect::ZERO, AM_DEFAULT);
}

void StretchableSprite2D::SetBorder(const IntRect& border)
{
    border_ = border;

    auto drawSize = drawRect_.Size();

    checkBorder(border_.left_, drawSize.x);
    checkBorder(border_.right_, drawSize.x);
    checkBorder(border_.left_ + border_.right_, drawSize.x);

    checkBorder(border_.bottom_, drawSize.y);
    checkBorder(border_.top_, drawSize.y);
    checkBorder(border_.bottom_ + border_.top_, drawSize.y);
}

void StretchableSprite2D::UpdateSourceBatches()
{
    /* The general idea is to subdivide the image in the following 9 patches

       *---*---*---*
     2 |   |   |   |
       *---*---*---*
     1 |   |   |   |
       *---*---*---*
     0 |   |   |   |
       *---*---*---*
         0   1   2

     X scaling works as follow: as long as the scale determines that the total
     width is larger than the sum of the widths of columns 0 and 2, only
     column 1 is scaled. Otherwise, column 1 is removed and columns 0 and 2 are
     scaled, maintaining their relative size. The same principle applies for
     Y scaling (but with lines rather than columns). */

    if (!sourceBatchesDirty_ || !sprite_ || (!useTextureRect_ && !sprite_->GetTextureRectangle(textureRect_, flipX_, flipY_)))
        return;

    Vector<Vertex2D>& vertices = sourceBatches_[0].vertices_;
    vertices.Clear();

    auto effectiveBorder = calcEffectiveBorder(border_, drawRect_.Size());

    float xs[4], ys[4], us[4], vs[4]; // prepare all coordinates
    const auto signedScale = node_->GetSignedWorldScale();

    prepareXYCoords(xs, drawRect_.min_.x, drawRect_.max_.x, effectiveBorder.min_.x, effectiveBorder.max_.x, signedScale.x);
    prepareXYCoords(ys, drawRect_.min_.y, drawRect_.max_.y, effectiveBorder.min_.y, effectiveBorder.max_.y, signedScale.y);

    prepareUVCoords(us, textureRect_.min_.x, textureRect_.max_.x, effectiveBorder.min_.x, effectiveBorder.max_.x,
        drawRect_.max_.x - drawRect_.min_.x);
    prepareUVCoords(vs, textureRect_.min_.y, textureRect_.max_.y, -effectiveBorder.min_.y,
        -effectiveBorder.max_.y /* texture y direction inverted*/, drawRect_.max_.y - drawRect_.min_.y);

    Vertex2D vtx[4][4]; // prepare all vertices
    prepareVertices(vtx, xs, ys, us, vs, color_.ToU32(), node_->GetWorldPosition(), node_->GetWorldRotation());

    pushVertices(vertices, vtx); // push the vertices that make up each patch

    sourceBatchesDirty_ = false;
}

}
