// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../resource/resource.h"
#include "urho_2d.h"

namespace dviglo
{

class SpriteSheet2D;
class Texture2D;

/// Sprite.
class DV_API Sprite2D : public Resource
{
    DV_OBJECT(Sprite2D);

public:
    /// Construct.
    explicit Sprite2D();
    /// Destruct.
    ~Sprite2D() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool end_load() override;

    /// Set texture.
    void SetTexture(Texture2D* texture);
    /// Set rectangle.
    void SetRectangle(const IntRect& rectangle);
    /// Set hot spot.
    void SetHotSpot(const Vector2& hotSpot);
    /// Set offset.
    void SetOffset(const IntVector2& offset);
    /// Set texture edge offset in pixels. This affects the left/right and top/bottom edges equally to prevent edge sampling artifacts. Default 0.
    void SetTextureEdgeOffset(float offset);
    /// Set sprite sheet.
    void SetSpriteSheet(SpriteSheet2D* spriteSheet);

    /// Return texture.
    Texture2D* GetTexture() const { return texture_; }

    /// Return rectangle.
    const IntRect& GetRectangle() const { return rectangle_; }

    /// Return hot spot.
    const Vector2& GetHotSpot() const { return hotSpot_; }

    /// Return offset.
    const IntVector2& GetOffset() const { return offset_; }

    /// Return texture edge offset.
    float GetTextureEdgeOffset() const { return edge_offset_; }

    /// Return sprite sheet.
    SpriteSheet2D* GetSpriteSheet() const { return spriteSheet_; }


    /// Return draw rectangle.
    bool GetDrawRectangle(Rect& rect, bool flipX = false, bool flipY = false) const;
    /// Return draw rectangle with custom hot spot.
    bool GetDrawRectangle(Rect& rect, const Vector2& hotSpot, bool flipX = false, bool flipY = false) const;
    /// Return texture rectangle.
    bool GetTextureRectangle(Rect& rect, bool flipX = false, bool flipY = false) const;

    /// Save sprite to ResourceRef.
    static ResourceRef SaveToResourceRef(Sprite2D* sprite);
    /// Load sprite from ResourceRef.
    static Sprite2D* LoadFromResourceRef(Object* object, const ResourceRef& value);

private:
    /// Texture.
    SharedPtr<Texture2D> texture_;
    /// Rectangle.
    IntRect rectangle_;
    /// Hot spot.
    Vector2 hotSpot_;
    /// Offset (for trimmed sprite).
    IntVector2 offset_;
    /// Sprite sheet.
    WeakPtr<SpriteSheet2D> spriteSheet_;
    /// Texture used while loading.
    SharedPtr<Texture2D> loadTexture_;
    /// Offset to fix texture edge bleeding.
    float edge_offset_;
};

}
