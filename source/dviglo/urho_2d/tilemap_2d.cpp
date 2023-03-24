// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/debug_renderer.h"
#include "../resource/resource_cache.h"
#include "../scene/node.h"
#include "../scene/scene.h"
#include "tilemap_2d.h"
#include "tilemap_layer_2d.h"
#include "tmx_file_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* URHO2D_CATEGORY;

TileMap2D::TileMap2D()
{
}

TileMap2D::~TileMap2D() = default;

void TileMap2D::register_object()
{
    DV_CONTEXT.RegisterFactory<TileMap2D>(URHO2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Tmx File", GetTmxFileAttr, SetTmxFileAttr, ResourceRef(TmxFile2D::GetTypeStatic()),
        AM_DEFAULT);
}

// Transform vector from node-local space to global space
static Vector2 TransformNode2D(const Matrix3x4& transform, Vector2 local)
{
    Vector3 transformed = transform * Vector4(local.x, local.y, 0.f, 1.f);
    return Vector2(transformed.x, transformed.y);
}

void TileMap2D::draw_debug_geometry(DebugRenderer* debug, bool depthTest)
{
    const Color& color = Color::RED;
    float mapW = info_.GetMapWidth();
    float mapH = info_.GetMapHeight();
    const Matrix3x4 transform = GetNode()->GetTransform();

    switch (info_.orientation_)
    {
    case O_ORTHOGONAL:
    case O_STAGGERED:
    case O_HEXAGONAL:
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(0.0f, 0.0f))),
            Vector3(TransformNode2D(transform, Vector2(mapW, 0.0f))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW, 0.0f))),
            Vector3(TransformNode2D(transform, Vector2(mapW, mapH))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW, mapH))),
            Vector3(TransformNode2D(transform, Vector2(0.0f, mapH))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(0.0f, mapH))),
            Vector3(TransformNode2D(transform, Vector2(0.0f, 0.0f))), color);
        break;

    case O_ISOMETRIC:
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(0.0f, mapH * 0.5f))),
            Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, 0.0f))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, 0.0f))),
            Vector3(TransformNode2D(transform, Vector2(mapW, mapH * 0.5f))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW, mapH * 0.5f))),
            Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, mapH))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, mapH))),
            Vector3(TransformNode2D(transform, Vector2(0.0f, mapH * 0.5f))), color);
        break;
    }

    for (const WeakPtr<TileMapLayer2D>& layer : layers_)
        layer->draw_debug_geometry(debug, depthTest);
}

void TileMap2D::draw_debug_geometry()
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    auto* debug = scene->GetComponent<DebugRenderer>();
    if (!debug)
        return;

    draw_debug_geometry(debug, false);
}

void TileMap2D::SetTmxFile(TmxFile2D* tmxFile)
{
    if (tmxFile == tmxFile_)
        return;

    if (rootNode_)
        rootNode_->RemoveAllChildren();

    layers_.Clear();

    tmxFile_ = tmxFile;
    if (!tmxFile_)
        return;

    info_ = tmxFile_->GetInfo();

    if (!rootNode_)
    {
        rootNode_ = GetNode()->CreateTemporaryChild("_root_", LOCAL);
    }

    unsigned numLayers = tmxFile_->GetNumLayers();
    layers_.Resize(numLayers);

    for (unsigned i = 0; i < numLayers; ++i)
    {
        const TmxLayer2D* tmxLayer = tmxFile_->GetLayer(i);

        Node* layerNode(rootNode_->CreateTemporaryChild(tmxLayer->GetName(), LOCAL));

        auto* layer = layerNode->create_component<TileMapLayer2D>();
        layer->Initialize(this, tmxLayer);
        layer->SetDrawOrder(i * 10);

        layers_[i] = layer;
    }
}

TmxFile2D* TileMap2D::GetTmxFile() const
{
    return tmxFile_;
}

TileMapLayer2D* TileMap2D::GetLayer(unsigned index) const
{
    if (index >= layers_.Size())
        return nullptr;

    return layers_[index];
}

Vector2 TileMap2D::tile_index_to_position(int x, int y) const
{
    return info_.tile_index_to_position(x, y);
}

bool TileMap2D::position_to_tile_index(int& x, int& y, const Vector2& position) const
{
    return info_.position_to_tile_index(x, y, position);
}

void TileMap2D::SetTmxFileAttr(const ResourceRef& value)
{
    SetTmxFile(DV_RES_CACHE.GetResource<TmxFile2D>(value.name_));
}

ResourceRef TileMap2D::GetTmxFileAttr() const
{
    return GetResourceRef(tmxFile_, TmxFile2D::GetTypeStatic());
}

Vector<SharedPtr<TileMapObject2D>> TileMap2D::GetTileCollisionShapes(unsigned gid) const
{
    Vector<SharedPtr<TileMapObject2D>> shapes;
    return tmxFile_ ? tmxFile_->GetTileCollisionShapes(gid) : shapes;
}

}
