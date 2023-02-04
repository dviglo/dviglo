// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../math/matrix3x4.h"
#include "../ui/ui_element.h"

namespace Urho3D
{

/// %UI element which allows sub-pixel positioning and size, as well as rotation. Only other Sprites should be added as child elements.
class URHO3D_API Sprite : public UIElement
{
    URHO3D_OBJECT(Sprite, UIElement);

public:
    /// Construct.
    explicit Sprite(Context* context);
    /// Destruct.
    ~Sprite() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Return whether is visible and inside a scissor rectangle and should be rendered.
    bool IsWithinScissor(const IntRect& currentScissor) override;
    /// Update and return screen position.
    const IntVector2& GetScreenPosition() const override;
    /// Return UI rendering batches.
    void GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor) override;
    /// React to position change.
    void OnPositionSet(const IntVector2& newPosition) override;
    /// Convert screen coordinates to element coordinates.
    IntVector2 ScreenToElement(const IntVector2& screenPosition) override;
    /// Convert element coordinates to screen coordinates.
    IntVector2 ElementToScreen(const IntVector2& position) override;

    /// Set floating point position.
    void SetPosition(const Vector2& position);
    /// Set floating point position.
    void SetPosition(float x, float y);
    /// Set hotspot for positioning and rotation.
    void SetHotSpot(const IntVector2& hotSpot);
    /// Set hotspot for positioning and rotation.
    void SetHotSpot(int x, int y);
    /// Set scale. Scale also affects child sprites.
    void SetScale(const Vector2& scale);
    /// Set scale. Scale also affects child sprites.
    void SetScale(float x, float y);
    /// Set uniform scale. Scale also affects child sprites.
    void SetScale(float scale);
    /// Set rotation angle.
    void SetRotation(float angle);
    /// Set texture.
    void SetTexture(Texture* texture);
    /// Set part of texture to use as the image.
    void SetImageRect(const IntRect& rect);
    /// Use whole texture as the image.
    void SetFullImageRect();
    /// Set blend mode.
    void SetBlendMode(BlendMode mode);

    /// Return floating point position.
    const Vector2& GetPosition() const { return floatPosition_; }

    /// Return hotspot.
    const IntVector2& GetHotSpot() const { return hotSpot_; }

    /// Return scale.
    const Vector2& GetScale() const { return scale_; }

    /// Return rotation angle.
    float GetRotation() const { return rotation_; }

    /// Return texture.
    Texture* GetTexture() const { return texture_; }

    /// Return image rectangle.
    const IntRect& GetImageRect() const { return imageRect_; }

    /// Return blend mode.
    BlendMode GetBlendMode() const { return blendMode_; }

    /// Set texture attribute.
    void SetTextureAttr(const ResourceRef& value);
    /// Return texture attribute.
    ResourceRef GetTextureAttr() const;
    /// Update and return rendering transform, also used to transform child sprites.
    const Matrix3x4& GetTransform() const;

protected:
    /// Floating point position.
    Vector2 floatPosition_;
    /// Hotspot for positioning and rotation.
    IntVector2 hotSpot_;
    /// Scale.
    Vector2 scale_;
    /// Rotation angle.
    float rotation_;
    /// Texture.
    SharedPtr<Texture> texture_;
    /// Image rectangle.
    IntRect imageRect_;
    /// Blend mode flag.
    BlendMode blendMode_;
    /// Transform matrix.
    mutable Matrix3x4 transform_;
};

}
