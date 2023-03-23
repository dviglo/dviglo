// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"
#include "tilemap_defs_2d.h"

namespace dviglo
{

class TileMapLayer2D;
class TmxFile2D;

/// Tile map component.
class DV_API TileMap2D : public Component
{
    DV_OBJECT(TileMap2D, Component);

public:
    /// Construct.
    explicit TileMap2D();
    /// Destruct.
    ~TileMap2D() override;
    /// Register object factory.
    static void RegisterObject();

    /// Visualize the component as debug geometry.
    void draw_debug_geometry(DebugRenderer* debug, bool depthTest) override;

    /// Set tmx file.
    void SetTmxFile(TmxFile2D* tmxFile);
    /// Add debug geometry to the debug renderer.
    void draw_debug_geometry();

    /// Return tmx file.
    TmxFile2D* GetTmxFile() const;

    /// Return information.
    const TileMapInfo2D& GetInfo() const { return info_; }

    /// Return number of layers.
    unsigned GetNumLayers() const { return layers_.Size(); }

    /// Return tile map layer at index.
    TileMapLayer2D* GetLayer(unsigned index) const;
    /// Convert tile index to position.
    Vector2 tile_index_to_position(int x, int y) const;
    /// Convert position to tile index, if out of map return false.
    bool PositionToTileIndex(int& x, int& y, const Vector2& position) const;

    /// Set tile map file attribute.
    void SetTmxFileAttr(const ResourceRef& value);
    /// Return tile map file attribute.
    ResourceRef GetTmxFileAttr() const;
    ///
    Vector<SharedPtr<TileMapObject2D>> GetTileCollisionShapes(unsigned gid) const;
private:
    /// Tmx file.
    SharedPtr<TmxFile2D> tmxFile_;
    /// Tile map information.
    TileMapInfo2D info_{};
    /// Root node for tile map layer.
    SharedPtr<Node> rootNode_;
    /// Tile map layers.
    Vector<WeakPtr<TileMapLayer2D>> layers_;
};

}
