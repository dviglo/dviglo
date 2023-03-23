// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../graphics_api/graphics_defs.h"
#include "ui_element.h"

namespace dviglo
{

class Texture;
class Texture2D;

/// %Image %UI element with optional border.
class DV_API BorderImage : public UiElement
{
    DV_OBJECT(BorderImage, UiElement);

public:
    /// Construct.
    explicit BorderImage();
    /// Destruct.
    ~BorderImage() override;
    /// Register object factory.
    static void register_object();

    /// Return UI rendering batches.
    void GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor) override;

    /// Set texture.
    void SetTexture(Texture* texture);
    /// Set part of texture to use as the image.
    void SetImageRect(const IntRect& rect);
    /// Use whole texture as the image.
    void SetFullImageRect();
    /// Set border dimensions on the screen.
    void SetBorder(const IntRect& rect);
    /// Set border dimensions on the image. If zero (default) uses the screen dimensions, resulting in pixel-perfect borders.
    void SetImageBorder(const IntRect& rect);
    /// Set offset to image rectangle used on hover.
    void SetHoverOffset(const IntVector2& offset);
    /// Set offset to image rectangle used on hover.
    void SetHoverOffset(int x, int y);
    /// Set offset to image rectangle used when disabled.
    void SetDisabledOffset(const IntVector2& offset);
    /// Set offset to image rectangle used when disabled.
    void SetDisabledOffset(int x, int y);
    /// Set blend mode.
    void SetBlendMode(BlendMode mode);
    /// Set tiled mode.
    void SetTiled(bool enable);
    /// Set material for custom rendering.
    void SetMaterial(Material* material);

    /// Return texture.
    Texture* GetTexture() const { return texture_; }

    /// Return image rectangle.
    const IntRect& GetImageRect() const { return imageRect_; }

    /// Return border screen dimensions.
    const IntRect& GetBorder() const { return border_; }

    /// Return border image dimensions. Zero rect uses border screen dimensions.
    const IntRect& GetImageBorder() const { return imageBorder_; }

    /// Return offset to image rectangle used on hover.
    const IntVector2& GetHoverOffset() const { return hoverOffset_; }

    /// Return offset to image rectangle used when disabled.
    const IntVector2& GetDisabledOffset() const { return disabledOffset_; }

    /// Return blend mode.
    BlendMode blend_mode() const { return blend_mode_; }

    /// Return whether is tiled.
    bool IsTiled() const { return tiled_; }

    /// Get material used for custom rendering.
    Material* GetMaterial() const;

    /// Set texture attribute.
    void SetTextureAttr(const ResourceRef& value);
    /// Return texture attribute.
    ResourceRef GetTextureAttr() const;

    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Get material attribute.
    ResourceRef GetMaterialAttr() const;
protected:
    /// Return UI rendering batches with offset to image rectangle.
    void GetBatches
        (Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor, const IntVector2& offset);

    /// Texture.
    SharedPtr<Texture> texture_;
    /// Image rectangle.
    IntRect imageRect_;
    /// Border dimensions on screen.
    IntRect border_;
    /// Border dimensions on the image.
    IntRect imageBorder_;
    /// Offset to image rectangle on hover.
    IntVector2 hoverOffset_;
    /// Offset to image rectangle when disabled.
    IntVector2 disabledOffset_;
    /// Blend mode flag.
    BlendMode blend_mode_;
    /// Tiled flag.
    bool tiled_;
    /// Material used for custom rendering.
    SharedPtr<Material> material_;
};

}
