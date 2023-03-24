// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/debug_renderer.h"
#include "../resource/resource_cache.h"
#include "../scene/node.h"
#include "static_sprite_2d.h"
#include "tilemap_2d.h"
#include "tilemap_layer_2d.h"
#include "tmx_file_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

TileMapLayer2D::TileMapLayer2D()
{
}

TileMapLayer2D::~TileMapLayer2D() = default;

void TileMapLayer2D::register_object()
{
    DV_CONTEXT.RegisterFactory<TileMapLayer2D>();
}

// Transform vector from node-local space to global space
static Vector2 TransformNode2D(const Matrix3x4& transform, Vector2 local)
{
    Vector3 transformed = transform * Vector4(local.x, local.y, 0.f, 1.f);
    return Vector2(transformed.x, transformed.y);
}

void TileMapLayer2D::draw_debug_geometry(DebugRenderer* debug, bool depthTest)
{
    if (!debug)
        return;

    if (objectGroup_)
    {
        const Matrix3x4 transform = GetTileMap()->GetNode()->GetTransform();
        for (unsigned i = 0; i < objectGroup_->GetNumObjects(); ++i)
        {
            TileMapObject2D* object = objectGroup_->GetObject(i);
            const Color& color = Color::YELLOW;
            const Vector2& size = object->GetSize();
            const TileMapInfo2D& info = tileMap_->GetInfo();


            switch (object->GetObjectType())
            {
            case OT_RECTANGLE:
                {
                    Vector<Vector2> points;

                    switch (info.orientation_)
                    {
                    case O_ORTHOGONAL:
                    case O_HEXAGONAL:
                    case O_STAGGERED:
                        {
                            points.Push(Vector2::ZERO);
                            points.Push(Vector2(size.x, 0.0f));
                            points.Push(Vector2(size.x, -size.y));
                            points.Push(Vector2(0.0f, -size.y));
                            break;
                        }
                    case O_ISOMETRIC:
                        {
                            float ratio = (info.tileWidth_ / info.tileHeight_) * 0.5f;
                            points.Push(Vector2::ZERO);
                            points.Push(Vector2(size.y * ratio, size.y * 0.5f));
                            points.Push(Vector2((size.x + size.y) * ratio, (-size.x + size.y) * 0.5f));
                            points.Push(Vector2(size.x * ratio, -size.x * 0.5f));
                            break;
                        }
                    }

                    for (unsigned j = 0; j < points.Size(); ++j)
                        debug->AddLine(Vector3(TransformNode2D(transform, points[j] + object->GetPosition())),
                            Vector3(TransformNode2D(transform, points[(j + 1) % points.Size()] + object->GetPosition())), color,
                            depthTest);
                }
                break;

            case OT_ELLIPSE:
                {
                    const Vector2 halfSize = object->GetSize() * 0.5f;
                    float ratio = (info.tileWidth_ / info.tileHeight_) * 0.5f; // For isometric only

                    Vector2 pivot = object->GetPosition();
                    if (info.orientation_ == O_ISOMETRIC)
                    {
                        pivot += Vector2((halfSize.x + halfSize.y) * ratio, (-halfSize.x + halfSize.y) * 0.5f);
                    }
                    else
                    {
                        pivot += halfSize;
                    }

                    for (unsigned i = 0; i < 360; i += 30)
                    {
                        unsigned j = i + 30;
                        float x1 = halfSize.x * Cos((float)i);
                        float y1 = halfSize.y * Sin((float)i);
                        float x2 = halfSize.x * Cos((float)j);
                        float y2 = halfSize.y * Sin((float)j);
                        Vector2 point1 = Vector2(x1, - y1);
                        Vector2 point2 = Vector2(x2, - y2);

                        if (info.orientation_ == O_ISOMETRIC)
                        {
                            point1 = Vector2((point1.x + point1.y) * ratio, (point1.y - point1.x) * 0.5f);
                            point2 = Vector2((point2.x + point2.y) * ratio, (point2.y - point2.x) * 0.5f);
                        }

                        debug->AddLine(Vector3(TransformNode2D(transform, pivot + point1)),
                            Vector3(TransformNode2D(transform, pivot + point2)), color, depthTest);
                    }
                }
                break;

            case OT_POLYGON:
            case OT_POLYLINE:
                {
                    for (unsigned j = 0; j < object->GetNumPoints() - 1; ++j)
                        debug->AddLine(Vector3(TransformNode2D(transform, object->GetPoint(j))),
                            Vector3(TransformNode2D(transform, object->GetPoint(j + 1))), color, depthTest);

                    if (object->GetObjectType() == OT_POLYGON)
                        debug->AddLine(Vector3(TransformNode2D(transform, object->GetPoint(0))),
                            Vector3(TransformNode2D(transform, object->GetPoint(object->GetNumPoints() - 1))), color, depthTest);
                    // Also draw a circle at origin to indicate direction
                    else
                        debug->AddCircle(Vector3(TransformNode2D(transform, object->GetPoint(0))), Vector3::FORWARD, 0.05f, color,
                            64, depthTest);
                }
                break;

            default: break;
            }
        }
    }
}

void TileMapLayer2D::Initialize(TileMap2D* tileMap, const TmxLayer2D* tmxLayer)
{
    if (tileMap == tileMap_ && tmxLayer == tmxLayer_)
        return;

    if (tmxLayer_)
    {
        for (unsigned i = 0; i < nodes_.Size(); ++i)
        {
            if (nodes_[i])
                nodes_[i]->Remove();
        }

        nodes_.Clear();
    }

    tileLayer_ = nullptr;
    objectGroup_ = nullptr;
    imageLayer_ = nullptr;

    tileMap_ = tileMap;
    tmxLayer_ = tmxLayer;

    if (!tmxLayer_)
        return;

    switch (tmxLayer_->GetType())
    {
    case LT_TILE_LAYER:
        SetTileLayer((const TmxTileLayer2D*)tmxLayer_);
        break;

    case LT_OBJECT_GROUP:
        SetObjectGroup((const TmxObjectGroup2D*)tmxLayer_);
        break;

    case LT_IMAGE_LAYER:
        SetImageLayer((const TmxImageLayer2D*)tmxLayer_);
        break;

    default:
        break;
    }

    SetVisible(tmxLayer_->IsVisible());
}

void TileMapLayer2D::SetDrawOrder(int drawOrder)
{
    if (drawOrder == drawOrder_)
        return;

    drawOrder_ = drawOrder;

    for (unsigned i = 0; i < nodes_.Size(); ++i)
    {
        if (!nodes_[i])
            continue;

        auto* staticSprite = nodes_[i]->GetComponent<StaticSprite2D>();
        if (staticSprite)
            staticSprite->SetLayer(drawOrder_);
    }
}

void TileMapLayer2D::SetVisible(bool visible)
{
    if (visible == visible_)
        return;

    visible_ = visible;

    for (unsigned i = 0; i < nodes_.Size(); ++i)
    {
        if (nodes_[i])
            nodes_[i]->SetEnabled(visible_);
    }
}

TileMap2D* TileMapLayer2D::GetTileMap() const
{
    return tileMap_;
}

bool TileMapLayer2D::HasProperty(const String& name) const
{
    if (!tmxLayer_)
        return false;

    return tmxLayer_->HasProperty(name);
}

const String& TileMapLayer2D::GetProperty(const String& name) const
{
    if (!tmxLayer_)
        return String::EMPTY;
    return tmxLayer_->GetProperty(name);
}

TileMapLayerType2D TileMapLayer2D::GetLayerType() const
{
    return tmxLayer_ ? tmxLayer_->GetType() : LT_INVALID;
}

int TileMapLayer2D::GetWidth() const
{
    return tmxLayer_ ? tmxLayer_->GetWidth() : 0;
}

int TileMapLayer2D::GetHeight() const
{
    return tmxLayer_ ? tmxLayer_->GetHeight() : 0;
}

Tile2D* TileMapLayer2D::GetTile(int x, int y) const
{
    if (!tileLayer_)
        return nullptr;

    return tileLayer_->GetTile(x, y);
}

Node* TileMapLayer2D::GetTileNode(int x, int y) const
{
    if (!tileLayer_)
        return nullptr;

    if (x < 0 || x >= tileLayer_->GetWidth() || y < 0 || y >= tileLayer_->GetHeight())
        return nullptr;

    return nodes_[y * tileLayer_->GetWidth() + x];
}

unsigned TileMapLayer2D::GetNumObjects() const
{
    if (!objectGroup_)
        return 0;

    return objectGroup_->GetNumObjects();
}

TileMapObject2D* TileMapLayer2D::GetObject(unsigned index) const
{
    if (!objectGroup_)
        return nullptr;

    return objectGroup_->GetObject(index);
}

Node* TileMapLayer2D::GetObjectNode(unsigned index) const
{
    if (!objectGroup_)
        return nullptr;

    if (index >= nodes_.Size())
        return nullptr;

    return nodes_[index];
}

Node* TileMapLayer2D::GetImageNode() const
{
    if (!imageLayer_ || nodes_.Empty())
        return nullptr;

    return nodes_[0];
}

void TileMapLayer2D::SetTileLayer(const TmxTileLayer2D* tileLayer)
{
    tileLayer_ = tileLayer;

    int width = tileLayer->GetWidth();
    int height = tileLayer->GetHeight();
    nodes_.Resize((unsigned)(width * height));

    const TileMapInfo2D& info = tileMap_->GetInfo();
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Tile2D* tile = tileLayer->GetTile(x, y);
            if (!tile)
                continue;

            SharedPtr<Node> tileNode(GetNode()->CreateTemporaryChild("Tile"));
            tileNode->SetPosition(Vector3(info.tile_index_to_position(x, y)));

            auto* staticSprite = tileNode->create_component<StaticSprite2D>();
            staticSprite->SetSprite(tile->GetSprite());
            staticSprite->SetFlip(tile->GetFlipX(), tile->GetFlipY(), tile->GetSwapXY());
            staticSprite->SetLayer(drawOrder_);
            staticSprite->SetOrderInLayer(y * width + x);

            nodes_[y * width + x] = tileNode;
        }
    }
}

void TileMapLayer2D::SetObjectGroup(const TmxObjectGroup2D* objectGroup)
{
    objectGroup_ = objectGroup;

    TmxFile2D* tmxFile = objectGroup->GetTmxFile();
    nodes_.Resize(objectGroup->GetNumObjects());

    for (unsigned i = 0; i < objectGroup->GetNumObjects(); ++i)
    {
        const TileMapObject2D* object = objectGroup->GetObject(i);

        // Create dummy node for all object
        SharedPtr<Node> objectNode(GetNode()->CreateTemporaryChild("Object"));
        objectNode->SetPosition(Vector3(object->GetPosition()));

        // If object is tile, create static sprite component
        if (object->GetObjectType() == OT_TILE && object->GetTileGid() && object->GetTileSprite())
        {
            auto* staticSprite = objectNode->create_component<StaticSprite2D>();
            staticSprite->SetSprite(object->GetTileSprite());
            staticSprite->SetFlip(object->GetTileFlipX(), object->GetTileFlipY(), object->GetTileSwapXY());
            staticSprite->SetLayer(drawOrder_);
            staticSprite->SetOrderInLayer((int)((10.0f - object->GetPosition().y) * 100));

            if (tmxFile->GetInfo().orientation_ == O_ISOMETRIC)
            {
                staticSprite->SetUseHotSpot(true);
                staticSprite->SetHotSpot(Vector2(0.5f, 0.0f));
            }
        }

        nodes_[i] = objectNode;
    }
}

void TileMapLayer2D::SetImageLayer(const TmxImageLayer2D* imageLayer)
{
    imageLayer_ = imageLayer;

    if (!imageLayer->GetSprite())
        return;

    SharedPtr<Node> imageNode(GetNode()->CreateTemporaryChild("Tile"));
    imageNode->SetPosition(Vector3(imageLayer->GetPosition()));

    auto* staticSprite = imageNode->create_component<StaticSprite2D>();
    staticSprite->SetSprite(imageLayer->GetSprite());
    staticSprite->SetOrderInLayer(0);

    nodes_.Push(imageNode);
}

}
