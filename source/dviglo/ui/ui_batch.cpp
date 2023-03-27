// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../graphics/graphics.h"
#include "../graphics_api/texture.h"
#include "ui_element.h"

#include "../common/debug_new.h"

namespace dviglo
{

UIBatch::UIBatch()
{
    SetDefaultColor();
}

UIBatch::UIBatch(UiElement* element, BlendMode blendMode, const IntRect& scissor, Texture* texture, Vector<float>* vertexData) :     // NOLINT(modernize-pass-by-value)
    element_(element),
    blend_mode_(blendMode),
    scissor_(scissor),
    texture_(texture),
    invTextureSize_(texture ? Vector2(1.0f / (float)texture->GetWidth(), 1.0f / (float)texture->GetHeight()) : Vector2::ONE),
    vertexData_(vertexData),
    vertexStart_(vertexData->Size()),
    vertexEnd_(vertexData->Size())
{
    SetDefaultColor();
}

void UIBatch::SetColor(const Color& color, bool overrideAlpha)
{
    if (!element_)
        overrideAlpha = true;

    useGradient_ = false;
    color_ =
        overrideAlpha ? color.ToU32() : Color(color.r_, color.g_, color.b_, color.a_ * element_->GetDerivedOpacity()).ToU32();
}

void UIBatch::SetDefaultColor()
{
    if (element_)
    {
        color_ = element_->GetDerivedColor().ToU32();
        useGradient_ = element_->HasColorGradient();
    }
    else
    {
        color_ = 0xffffffff;
        useGradient_ = false;
    }
}

void UIBatch::add_quad(float x, float y, float width, float height, int texOffsetX, int texOffsetY, int texWidth, int texHeight)
{
    unsigned topLeftColor, topRightColor, bottomLeftColor, bottomRightColor;

    if (!useGradient_)
    {
        // If alpha is 0, nothing will be rendered, so do not add the quad
        if (!(color_ & 0xff000000))
            return;

        topLeftColor = color_;
        topRightColor = color_;
        bottomLeftColor = color_;
        bottomRightColor = color_;
    }
    else
    {
        topLeftColor = GetInterpolatedColor(x, y);
        topRightColor = GetInterpolatedColor(x + width, y);
        bottomLeftColor = GetInterpolatedColor(x, y + height);
        bottomRightColor = GetInterpolatedColor(x + width, y + height);
    }

    const IntVector2& screenPos = element_->GetScreenPosition();

    float left = x + screenPos.x;
    float right = left + width;
    float top = y + screenPos.y;
    float bottom = top + height;

    float leftUV = texOffsetX * invTextureSize_.x;
    float topUV = texOffsetY * invTextureSize_.y;
    float rightUV = (texOffsetX + (texWidth ? texWidth : width)) * invTextureSize_.x;
    float bottomUV = (texOffsetY + (texHeight ? texHeight : height)) * invTextureSize_.y;

    unsigned begin = vertexData_->Size();
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    float* dest = &(vertexData_->At(begin));
    vertexEnd_ = vertexData_->Size();

    dest[0] = left;
    dest[1] = top;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = topLeftColor;
    dest[4] = leftUV;
    dest[5] = topUV;

    dest[6] = right;
    dest[7] = top;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = topRightColor;
    dest[10] = rightUV;
    dest[11] = topUV;

    dest[12] = left;
    dest[13] = bottom;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = bottomLeftColor;
    dest[16] = leftUV;
    dest[17] = bottomUV;

    dest[18] = right;
    dest[19] = top;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = topRightColor;
    dest[22] = rightUV;
    dest[23] = topUV;

    dest[24] = right;
    dest[25] = bottom;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = bottomRightColor;
    dest[28] = rightUV;
    dest[29] = bottomUV;

    dest[30] = left;
    dest[31] = bottom;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = bottomLeftColor;
    dest[34] = leftUV;
    dest[35] = bottomUV;
}

void UIBatch::add_quad(const Matrix3x4& transform, int x, int y, int width, int height, int texOffsetX, int texOffsetY,
    int texWidth, int texHeight)
{
    unsigned topLeftColor, topRightColor, bottomLeftColor, bottomRightColor;

    if (!useGradient_)
    {
        // If alpha is 0, nothing will be rendered, so do not add the quad
        if (!(color_ & 0xff000000))
            return;

        topLeftColor = color_;
        topRightColor = color_;
        bottomLeftColor = color_;
        bottomRightColor = color_;
    }
    else
    {
        topLeftColor = GetInterpolatedColor(x, y);
        topRightColor = GetInterpolatedColor(x + width, y);
        bottomLeftColor = GetInterpolatedColor(x, y + height);
        bottomRightColor = GetInterpolatedColor(x + width, y + height);
    }

    Vector3 v1 = (transform * Vector3((float)x, (float)y, 0.0f));
    Vector3 v2 = (transform * Vector3((float)x + (float)width, (float)y, 0.0f));
    Vector3 v3 = (transform * Vector3((float)x, (float)y + (float)height, 0.0f));
    Vector3 v4 = (transform * Vector3((float)x + (float)width, (float)y + (float)height, 0.0f));

    float leftUV = ((float)texOffsetX) * invTextureSize_.x;
    float topUV = ((float)texOffsetY) * invTextureSize_.y;
    float rightUV = ((float)(texOffsetX + (texWidth ? texWidth : width))) * invTextureSize_.x;
    float bottomUV = ((float)(texOffsetY + (texHeight ? texHeight : height))) * invTextureSize_.y;

    unsigned begin = vertexData_->Size();
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    float* dest = &(vertexData_->At(begin));
    vertexEnd_ = vertexData_->Size();

    dest[0] = v1.x;
    dest[1] = v1.y;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = topLeftColor;
    dest[4] = leftUV;
    dest[5] = topUV;

    dest[6] = v2.x;
    dest[7] = v2.y;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = topRightColor;
    dest[10] = rightUV;
    dest[11] = topUV;

    dest[12] = v3.x;
    dest[13] = v3.y;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = bottomLeftColor;
    dest[16] = leftUV;
    dest[17] = bottomUV;

    dest[18] = v2.x;
    dest[19] = v2.y;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = topRightColor;
    dest[22] = rightUV;
    dest[23] = topUV;

    dest[24] = v4.x;
    dest[25] = v4.y;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = bottomRightColor;
    dest[28] = rightUV;
    dest[29] = bottomUV;

    dest[30] = v3.x;
    dest[31] = v3.y;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = bottomLeftColor;
    dest[34] = leftUV;
    dest[35] = bottomUV;
}

void UIBatch::add_quad(int x, int y, int width, int height, int texOffsetX, int texOffsetY, int texWidth, int texHeight, bool tiled)
{
    if (!(element_->HasColorGradient() || element_->GetDerivedColor().ToU32() & 0xff000000))
        return; // No gradient and alpha is 0, so do not add the quad

    if (!tiled)
    {
        add_quad(x, y, width, height, texOffsetX, texOffsetY, texWidth, texHeight);
        return;
    }

    int tileX = 0;
    int tileY = 0;
    int tileW = 0;
    int tileH = 0;

    while (tileY < height)
    {
        tileX = 0;
        tileH = Min(height - tileY, texHeight);

        while (tileX < width)
        {
            tileW = Min(width - tileX, texWidth);

            add_quad(x + tileX, y + tileY, tileW, tileH, texOffsetX, texOffsetY, tileW, tileH);

            tileX += tileW;
        }

        tileY += tileH;
    }
}

void UIBatch::add_quad(const Matrix3x4& transform, const IntVector2& a, const IntVector2& b, const IntVector2& c, const IntVector2& d,
    const IntVector2& texA, const IntVector2& texB, const IntVector2& texC, const IntVector2& texD)
{
    Vector3 v1 = (transform * Vector3((float)a.x, (float)a.y, 0.0f));
    Vector3 v2 = (transform * Vector3((float)b.x, (float)b.y, 0.0f));
    Vector3 v3 = (transform * Vector3((float)c.x, (float)c.y, 0.0f));
    Vector3 v4 = (transform * Vector3((float)d.x, (float)d.y, 0.0f));

    Vector2 uv1((float)texA.x * invTextureSize_.x, (float)texA.y * invTextureSize_.y);
    Vector2 uv2((float)texB.x * invTextureSize_.x, (float)texB.y * invTextureSize_.y);
    Vector2 uv3((float)texC.x * invTextureSize_.x, (float)texC.y * invTextureSize_.y);
    Vector2 uv4((float)texD.x * invTextureSize_.x, (float)texD.y * invTextureSize_.y);

    unsigned begin = vertexData_->Size();
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    float* dest = &(vertexData_->At(begin));
    vertexEnd_ = vertexData_->Size();

    dest[0] = v1.x;
    dest[1] = v1.y;
    dest[2] = 0.0f;
    ((unsigned&)dest[3]) = color_;
    dest[4] = uv1.x;
    dest[5] = uv1.y;

    dest[6] = v2.x;
    dest[7] = v2.y;
    dest[8] = 0.0f;
    ((unsigned&)dest[9]) = color_;
    dest[10] = uv2.x;
    dest[11] = uv2.y;

    dest[12] = v3.x;
    dest[13] = v3.y;
    dest[14] = 0.0f;
    ((unsigned&)dest[15]) = color_;
    dest[16] = uv3.x;
    dest[17] = uv3.y;

    dest[18] = v1.x;
    dest[19] = v1.y;
    dest[20] = 0.0f;
    ((unsigned&)dest[21]) = color_;
    dest[22] = uv1.x;
    dest[23] = uv1.y;

    dest[24] = v3.x;
    dest[25] = v3.y;
    dest[26] = 0.0f;
    ((unsigned&)dest[27]) = color_;
    dest[28] = uv3.x;
    dest[29] = uv3.y;

    dest[30] = v4.x;
    dest[31] = v4.y;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = color_;
    dest[34] = uv4.x;
    dest[35] = uv4.y;
}

void UIBatch::add_quad(const Matrix3x4& transform, const IntVector2& a, const IntVector2& b, const IntVector2& c, const IntVector2& d,
    const IntVector2& texA, const IntVector2& texB, const IntVector2& texC, const IntVector2& texD, const Color& colA,
    const Color& colB, const Color& colC, const Color& colD)
{
    Vector3 v1 = (transform * Vector3((float)a.x, (float)a.y, 0.0f));
    Vector3 v2 = (transform * Vector3((float)b.x, (float)b.y, 0.0f));
    Vector3 v3 = (transform * Vector3((float)c.x, (float)c.y, 0.0f));
    Vector3 v4 = (transform * Vector3((float)d.x, (float)d.y, 0.0f));

    Vector2 uv1((float)texA.x * invTextureSize_.x, (float)texA.y * invTextureSize_.y);
    Vector2 uv2((float)texB.x * invTextureSize_.x, (float)texB.y * invTextureSize_.y);
    Vector2 uv3((float)texC.x * invTextureSize_.x, (float)texC.y * invTextureSize_.y);
    Vector2 uv4((float)texD.x * invTextureSize_.x, (float)texD.y * invTextureSize_.y);

    color32 c1 = colA.ToU32();
    color32 c2 = colB.ToU32();
    color32 c3 = colC.ToU32();
    color32 c4 = colD.ToU32();

    unsigned begin = vertexData_->Size();
    vertexData_->Resize(begin + 6 * UI_VERTEX_SIZE);
    float* dest = &(vertexData_->At(begin));
    vertexEnd_ = vertexData_->Size();

    dest[0] = v1.x;
    dest[1] = v1.y;
    dest[2] = 0.0f;
    ((color32&)dest[3]) = c1;
    dest[4] = uv1.x;
    dest[5] = uv1.y;

    dest[6] = v2.x;
    dest[7] = v2.y;
    dest[8] = 0.0f;
    ((color32&)dest[9]) = c2;
    dest[10] = uv2.x;
    dest[11] = uv2.y;

    dest[12] = v3.x;
    dest[13] = v3.y;
    dest[14] = 0.0f;
    ((color32&)dest[15]) = c3;
    dest[16] = uv3.x;
    dest[17] = uv3.y;

    dest[18] = v1.x;
    dest[19] = v1.y;
    dest[20] = 0.0f;
    ((color32&)dest[21]) = c1;
    dest[22] = uv1.x;
    dest[23] = uv1.y;

    dest[24] = v3.x;
    dest[25] = v3.y;
    dest[26] = 0.0f;
    ((color32&)dest[27]) = c3;
    dest[28] = uv3.x;
    dest[29] = uv3.y;

    dest[30] = v4.x;
    dest[31] = v4.y;
    dest[32] = 0.0f;
    ((unsigned&)dest[33]) = c4;
    dest[34] = uv4.x;
    dest[35] = uv4.y;
}

bool UIBatch::Merge(const UIBatch& batch)
{
    if (batch.blend_mode_ != blend_mode_ ||
        batch.scissor_ != scissor_ ||
        batch.texture_ != texture_ ||
        batch.vertexData_ != vertexData_ ||
        batch.vertexStart_ != vertexEnd_)
        return false;

    vertexEnd_ = batch.vertexEnd_;
    return true;
}

unsigned UIBatch::GetInterpolatedColor(float x, float y)
{
    const IntVector2& size = element_->GetSize();

    if (size.x && size.y)
    {
        float cLerpX = Clamp(x / (float)size.x, 0.0f, 1.0f);
        float cLerpY = Clamp(y / (float)size.y, 0.0f, 1.0f);

        Color topColor = element_->GetColor(C_TOPLEFT).Lerp(element_->GetColor(C_TOPRIGHT), cLerpX);
        Color bottomColor = element_->GetColor(C_BOTTOMLEFT).Lerp(element_->GetColor(C_BOTTOMRIGHT), cLerpX);
        Color color = topColor.Lerp(bottomColor, cLerpY);
        color.a_ *= element_->GetDerivedOpacity();
        return color.ToU32();
    }
    else
    {
        Color color = element_->GetColor(C_TOPLEFT);
        color.a_ *= element_->GetDerivedOpacity();
        return color.ToU32();
    }
}

void UIBatch::AddOrMerge(const UIBatch& batch, Vector<UIBatch>& batches)
{
    if (batch.vertexEnd_ == batch.vertexStart_)
        return;

    if (!batches.Empty() && batches.Back().Merge(batch))
        return;

    batches.Push(batch);
}

}
