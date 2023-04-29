// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics_api/texture_2d.h"
#include "../resource/resource_cache.h"
#include "border_image.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* blendModeNames[];
extern const char* UI_CATEGORY;

BorderImage::BorderImage() :
    imageRect_(IntRect::ZERO),
    border_(IntRect::ZERO),
    imageBorder_(IntRect::ZERO),
    hoverOffset_(IntVector2::ZERO),
    disabledOffset_(IntVector2::ZERO),
    blend_mode_(BLEND_REPLACE),
    tiled_(false)
{
}

BorderImage::~BorderImage() = default;

void BorderImage::register_object()
{
    DV_CONTEXT->RegisterFactory<BorderImage>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(UiElement);
    DV_ACCESSOR_ATTRIBUTE("Texture", GetTextureAttr, SetTextureAttr, ResourceRef(Texture2D::GetTypeStatic()),
        AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Image Rect", GetImageRect, SetImageRect, IntRect::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Border", GetBorder, SetBorder, IntRect::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Image Border", GetImageBorder, SetImageBorder, IntRect::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Hover Image Offset", GetHoverOffset, SetHoverOffset, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Disabled Image Offset", GetDisabledOffset, SetDisabledOffset, IntVector2::ZERO, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Tiled", IsTiled, SetTiled, false, AM_FILE);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Blend Mode", blend_mode, SetBlendMode, blendModeNames, 0, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef(Material::GetTypeStatic()),
        AM_FILE);
}

void BorderImage::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor)
{
    if (enabled_)
        GetBatches(batches, vertexData, currentScissor, (hovering_ || selected_ || HasFocus()) ? hoverOffset_ : IntVector2::ZERO);
    else
        GetBatches(batches, vertexData, currentScissor, disabledOffset_);
}

void BorderImage::SetTexture(Texture* texture)
{
    texture_ = texture;
    if (imageRect_ == IntRect::ZERO)
        SetFullImageRect();
}

void BorderImage::SetImageRect(const IntRect& rect)
{
    if (rect != IntRect::ZERO)
        imageRect_ = rect;
}

void BorderImage::SetFullImageRect()
{
    if (texture_)
        SetImageRect(IntRect(0, 0, texture_->GetWidth(), texture_->GetHeight()));
}

void BorderImage::SetBorder(const IntRect& rect)
{
    border_.left_ = Max(rect.left_, 0);
    border_.top_ = Max(rect.top_, 0);
    border_.right_ = Max(rect.right_, 0);
    border_.bottom_ = Max(rect.bottom_, 0);
}

void BorderImage::SetImageBorder(const IntRect& rect)
{
    imageBorder_.left_ = Max(rect.left_, 0);
    imageBorder_.top_ = Max(rect.top_, 0);
    imageBorder_.right_ = Max(rect.right_, 0);
    imageBorder_.bottom_ = Max(rect.bottom_, 0);
}

void BorderImage::SetHoverOffset(const IntVector2& offset)
{
    hoverOffset_ = offset;
}

void BorderImage::SetHoverOffset(int x, int y)
{
    hoverOffset_ = IntVector2(x, y);
}

void BorderImage::SetDisabledOffset(const IntVector2& offset)
{
    disabledOffset_ = offset;
}

void BorderImage::SetDisabledOffset(int x, int y)
{
    disabledOffset_ = IntVector2(x, y);
}

void BorderImage::SetBlendMode(BlendMode mode)
{
    blend_mode_ = mode;
}

void BorderImage::SetTiled(bool enable)
{
    tiled_ = enable;
}

void BorderImage::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor,
    const IntVector2& offset)
{
    bool allOpaque = true;
    if (GetDerivedOpacity() < 1.0f || colors_[C_TOPLEFT].a_ < 1.0f || colors_[C_TOPRIGHT].a_ < 1.0f ||
        colors_[C_BOTTOMLEFT].a_ < 1.0f || colors_[C_BOTTOMRIGHT].a_ < 1.0f)
        allOpaque = false;

    UIBatch
        batch(this, blend_mode_ == BLEND_REPLACE && !allOpaque ? BLEND_ALPHA : blend_mode_, currentScissor, texture_, &vertexData);

    if (material_)
        batch.customMaterial_ = material_;

    // Calculate size of the inner rect, and texture dimensions of the inner rect
    const IntRect& uvBorder = (imageBorder_ == IntRect::ZERO) ? border_ : imageBorder_;
    int x = GetIndentWidth();
    IntVector2 size = GetSize();
    size.x -= x;
    IntVector2 innerSize(
        Max(size.x - border_.left_ - border_.right_, 0),
        Max(size.y - border_.top_ - border_.bottom_, 0));
    IntVector2 innerUvSize(
        Max(imageRect_.right_ - imageRect_.left_ - uvBorder.left_ - uvBorder.right_, 0),
        Max(imageRect_.bottom_ - imageRect_.top_ - uvBorder.top_ - uvBorder.bottom_, 0));

    IntVector2 uvTopLeft(imageRect_.left_, imageRect_.top_);
    uvTopLeft += offset;

    // Top
    if (border_.top_)
    {
        if (border_.left_)
            batch.add_quad(x, 0, border_.left_, border_.top_, uvTopLeft.x, uvTopLeft.y, uvBorder.left_, uvBorder.top_);
        if (innerSize.x)
            batch.add_quad(x + border_.left_, 0, innerSize.x, border_.top_, uvTopLeft.x + uvBorder.left_, uvTopLeft.y,
                innerUvSize.x, uvBorder.top_, tiled_);
        if (border_.right_)
            batch.add_quad(x + border_.left_ + innerSize.x, 0, border_.right_, border_.top_,
                uvTopLeft.x + uvBorder.left_ + innerUvSize.x, uvTopLeft.y, uvBorder.right_, uvBorder.top_);
    }
    // Middle
    if (innerSize.y)
    {
        if (border_.left_)
            batch.add_quad(x, border_.top_, border_.left_, innerSize.y, uvTopLeft.x, uvTopLeft.y + uvBorder.top_,
                uvBorder.left_, innerUvSize.y, tiled_);
        if (innerSize.x)
            batch.add_quad(x + border_.left_, border_.top_, innerSize.x, innerSize.y, uvTopLeft.x + uvBorder.left_,
                uvTopLeft.y + uvBorder.top_, innerUvSize.x, innerUvSize.y, tiled_);
        if (border_.right_)
            batch.add_quad(x + border_.left_ + innerSize.x, border_.top_, border_.right_, innerSize.y,
                uvTopLeft.x + uvBorder.left_ + innerUvSize.x, uvTopLeft.y + uvBorder.top_, uvBorder.right_, innerUvSize.y,
                tiled_);
    }
    // Bottom
    if (border_.bottom_)
    {
        if (border_.left_)
            batch.add_quad(x, border_.top_ + innerSize.y, border_.left_, border_.bottom_, uvTopLeft.x,
                uvTopLeft.y + uvBorder.top_ + innerUvSize.y, uvBorder.left_, uvBorder.bottom_);
        if (innerSize.x)
            batch.add_quad(x + border_.left_, border_.top_ + innerSize.y, innerSize.x, border_.bottom_,
                uvTopLeft.x + uvBorder.left_, uvTopLeft.y + uvBorder.top_ + innerUvSize.y, innerUvSize.x, uvBorder.bottom_,
                tiled_);
        if (border_.right_)
            batch.add_quad(x + border_.left_ + innerSize.x, border_.top_ + innerSize.y, border_.right_, border_.bottom_,
                uvTopLeft.x + uvBorder.left_ + innerUvSize.x, uvTopLeft.y + uvBorder.top_ + innerUvSize.y, uvBorder.right_,
                uvBorder.bottom_);
    }

    UIBatch::AddOrMerge(batch, batches);

    // Reset hovering for next frame
    hovering_ = false;
}

void BorderImage::SetTextureAttr(const ResourceRef& value)
{
    SetTexture(DV_RES_CACHE->GetResource<Texture2D>(value.name_));
}

ResourceRef BorderImage::GetTextureAttr() const
{
    return GetResourceRef(texture_, Texture2D::GetTypeStatic());
}

void BorderImage::SetMaterialAttr(const ResourceRef& value)
{
    SetMaterial(DV_RES_CACHE->GetResource<Material>(value.name_));
}

ResourceRef BorderImage::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void BorderImage::SetMaterial(Material* material)
{
    material_ = material;
}

Material* BorderImage::GetMaterial() const
{
    return material_;
}

}
