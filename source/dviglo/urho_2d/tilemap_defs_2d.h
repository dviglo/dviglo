// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../containers/ref_counted.h"
#include "sprite_2d.h"

namespace dviglo
{

class XmlElement;

/// Orientation.
enum Orientation2D
{
    /// Orthogonal.
    O_ORTHOGONAL = 0,
    /// Isometric.
    O_ISOMETRIC,
    /// Staggered.
    O_STAGGERED,
    /// Hexagonal.
    O_HEXAGONAL
};

/// Tile map information.
struct DV_API TileMapInfo2D
{
    /// Orientation.
    Orientation2D orientation_;
    /// Width.
    int width_;
    /// Height.
    int height_;
    /// Tile width.
    float tileWidth_;
    /// Tile height.
    float tileHeight_;

    /// Return map width.
    float GetMapWidth() const;
    /// return map height.
    float GetMapHeight() const;
    /// Convert tmx position to Urho position.
    Vector2 ConvertPosition(const Vector2& position) const;
    /// Convert tile index to position.
    Vector2 tile_index_to_position(int x, int y) const;
    /// Convert position to tile index, if out of map return false.
    bool position_to_tile_index(int& x, int& y, const Vector2& position) const;
};

/// Tile map layer type.
enum TileMapLayerType2D
{
    /// Tile layer.
    LT_TILE_LAYER = 0,
    /// Object group.
    LT_OBJECT_GROUP,
    /// Image layer.
    LT_IMAGE_LAYER,
    /// Invalid.
    LT_INVALID = 0xffff
};

/// Tile map object type.
enum TileMapObjectType2D
{
    /// Rectangle.
    OT_RECTANGLE = 0,
    /// Ellipse.
    OT_ELLIPSE,
    /// Polygon.
    OT_POLYGON,
    /// Polyline.
    OT_POLYLINE,
    /// Tile.
    OT_TILE,
    /// Invalid.
    OT_INVALID = 0xffff
};

/// Property set.
class DV_API PropertySet2D : public RefCounted
{
public:
    PropertySet2D();
    ~PropertySet2D() override;

    /// Load from XML element.
    void Load(const XmlElement& element);
    /// Return has property.
    bool HasProperty(const String& name) const;
    /// Return property value.
    const String& GetProperty(const String& name) const;

protected:
    /// Property name to property value mapping.
    HashMap<String, String> nameToValueMapping_;
};

/// Tile flipping flags.
static const unsigned FLIP_HORIZONTAL = 0x80000000u;
static const unsigned FLIP_VERTICAL   = 0x40000000u;
static const unsigned FLIP_DIAGONAL   = 0x20000000u;
static const unsigned FLIP_RESERVED   = 0x10000000u;
static const unsigned FLIP_ALL = FLIP_HORIZONTAL | FLIP_VERTICAL | FLIP_DIAGONAL | FLIP_RESERVED;

/// Tile define.
class DV_API Tile2D : public RefCounted
{
public:
    /// Construct.
    Tile2D();

    /// Return gid.
    unsigned GetGid() const { return gid_ & ~FLIP_ALL; }
    /// Return flip X.
    bool GetFlipX() const { return gid_ & FLIP_HORIZONTAL; }
    /// Return flip Y.
    bool GetFlipY() const { return gid_ & FLIP_VERTICAL; }
    /// Return swap X and Y.
    bool GetSwapXY() const { return gid_ & FLIP_DIAGONAL; }

    /// Return sprite.
    Sprite2D* GetSprite() const;
    /// Return has property.
    bool HasProperty(const String& name) const;
    /// Return property.
    const String& GetProperty(const String& name) const;

private:
    friend class TmxTileLayer2D;

    /// Gid.
    unsigned gid_;
    /// Sprite.
    SharedPtr<Sprite2D> sprite_;
    /// Property set.
    SharedPtr<PropertySet2D> propertySet_;
};

/// Tile map object.
class DV_API TileMapObject2D : public RefCounted
{
public:
    TileMapObject2D();

    /// Return type.
    TileMapObjectType2D GetObjectType() const { return objectType_; }

    /// Return name.
    const String& GetName() const { return name_; }

    /// Return type.
    const String& GetType() const { return type_; }

    /// Return position.
    const Vector2& GetPosition() const { return position_; }

    /// Return size (for rectangle and ellipse).
    const Vector2& GetSize() const { return size_; }

    /// Return number of points (use for script).
    unsigned GetNumPoints() const;
    /// Return point at index (use for script).
    const Vector2& GetPoint(unsigned index) const;

    /// Return tile Gid.
    unsigned GetTileGid() const { return gid_ & ~FLIP_ALL; }
    /// Return tile flip X.
    bool GetTileFlipX() const { return gid_ & FLIP_HORIZONTAL; }
    /// Return tile flip Y.
    bool GetTileFlipY() const { return gid_ & FLIP_VERTICAL; }
    /// Return tile swap X and Y.
    bool GetTileSwapXY() const { return gid_ & FLIP_DIAGONAL; }

    /// Return tile sprite.
    Sprite2D* GetTileSprite() const;
    /// Return has property.
    bool HasProperty(const String& name) const;
    /// Return property value.
    const String& GetProperty(const String& name) const;

private:
    friend class TmxObjectGroup2D;

    /// Object type.
    TileMapObjectType2D objectType_{};
    /// Name.
    String name_;
    /// Type.
    String type_;
    /// Position.
    Vector2 position_;
    /// Size (for rectangle and ellipse).
    Vector2 size_;
    /// Points(for polygon and polyline).
    Vector<Vector2> points_;
    /// Gid (for tile).
    unsigned gid_{};
    /// Sprite (for tile).
    SharedPtr<Sprite2D> sprite_;
    /// Property set.
    SharedPtr<PropertySet2D> propertySet_;
};

}
