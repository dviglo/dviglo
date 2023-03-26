// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/profiler.h"
#include "drawable_events.h"
#include "geometry.h"
#include "material.h"
#include "octree.h"
#include "terrain.h"
#include "terrain_patch.h"
#include "../graphics_api/index_buffer.h"
#include "../graphics_api/vertex_buffer.h"
#include "../io/log.h"
#include "../resource/image.h"
#include "../resource/resource_cache.h"
#include "../resource/resource_events.h"
#include "../scene/node.h"
#include "../scene/scene.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* GEOMETRY_CATEGORY;

static const Vector3 DEFAULT_SPACING(1.0f, 0.25f, 1.0f);
static const unsigned MIN_LOD_LEVELS = 1;
static const unsigned MAX_LOD_LEVELS = 4;
static const int DEFAULT_PATCH_SIZE = 32;
static const int MIN_PATCH_SIZE = 4;
static const int MAX_PATCH_SIZE = 128;
static const unsigned STITCH_NORTH = 1;
static const unsigned STITCH_SOUTH = 2;
static const unsigned STITCH_WEST = 4;
static const unsigned STITCH_EAST = 8;

inline void GrowUpdateRegion(IntRect& updateRegion, int x, int y)
{
    if (updateRegion.left_ < 0)
    {
        updateRegion.left_ = updateRegion.right_ = x;
        updateRegion.top_ = updateRegion.bottom_ = y;
    }
    else
    {
        if (x < updateRegion.left_)
            updateRegion.left_ = x;
        if (x > updateRegion.right_)
            updateRegion.right_ = x;
        if (y < updateRegion.top_)
            updateRegion.top_ = y;
        if (y > updateRegion.bottom_)
            updateRegion.bottom_ = y;
    }
}

Terrain::Terrain() :
    indexBuffer_(new IndexBuffer()),
    spacing_(DEFAULT_SPACING),
    lastSpacing_(Vector3::ZERO),
    patchWorldOrigin_(Vector2::ZERO),
    patchWorldSize_(Vector2::ZERO),
    numVertices_(IntVector2::ZERO),
    lastNumVertices_(IntVector2::ZERO),
    numPatches_(IntVector2::ZERO),
    patchSize_(DEFAULT_PATCH_SIZE),
    lastPatchSize_(0),
    numLodLevels_(1),
    maxLodLevels_(MAX_LOD_LEVELS),
    occlusionLodLevel_(NINDEX),
    smoothing_(false),
    visible_(true),
    castShadows_(false),
    occluder_(false),
    occludee_(true),
    viewMask_(DEFAULT_VIEWMASK),
    lightMask_(DEFAULT_LIGHTMASK),
    shadowMask_(DEFAULT_SHADOWMASK),
    zoneMask_(DEFAULT_ZONEMASK),
    drawDistance_(0.0f),
    shadowDistance_(0.0f),
    lodBias_(1.0f),
    max_lights_(0),
    northID_(0),
    southID_(0),
    westID_(0),
    eastID_(0),
    recreateTerrain_(false),
    neighborsDirty_(false)
{
    indexBuffer_->SetShadowed(true);
}

Terrain::~Terrain() = default;

void Terrain::register_object()
{
    DV_CONTEXT.RegisterFactory<Terrain>(GEOMETRY_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Height Map", GetHeightMapAttr, SetHeightMapAttr, ResourceRef(Image::GetTypeStatic()),
        AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef(Material::GetTypeStatic()),
        AM_DEFAULT);
    DV_ATTRIBUTE_EX("North Neighbor NodeID", northID_, MarkNeighborsDirty, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE_EX("South Neighbor NodeID", southID_, MarkNeighborsDirty, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE_EX("West Neighbor NodeID", westID_, MarkNeighborsDirty, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE_EX("East Neighbor NodeID", eastID_, MarkNeighborsDirty, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE_EX("Vertex Spacing", spacing_, MarkTerrainDirty, DEFAULT_SPACING, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Patch Size", GetPatchSize, SetPatchSizeAttr, DEFAULT_PATCH_SIZE, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max LOD Levels", GetMaxLodLevels, SetMaxLodLevelsAttr, MAX_LOD_LEVELS, AM_DEFAULT);
    DV_ATTRIBUTE_EX("Smooth Height Map", smoothing_, MarkTerrainDirty, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Is Occluder", IsOccluder, SetOccluder, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Can Be Occluded", IsOccludee, SetOccludee, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Cast Shadows", GetCastShadows, SetCastShadows, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Draw Distance", GetDrawDistance, SetDrawDistance, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Shadow Distance", GetShadowDistance, SetShadowDistance, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("LOD Bias", GetLodBias, SetLodBias, 1.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max Lights", max_lights, SetMaxLights, 0, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("View Mask", GetViewMask, SetViewMask, DEFAULT_VIEWMASK, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Light Mask", GetLightMask, SetLightMask, DEFAULT_LIGHTMASK, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Shadow Mask", GetShadowMask, SetShadowMask, DEFAULT_SHADOWMASK, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Zone Mask", GetZoneMask, SetZoneMask, DEFAULT_ZONEMASK, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Occlusion LOD level", GetOcclusionLodLevel, SetOcclusionLodLevelAttr, NINDEX, AM_DEFAULT);
}

void Terrain::apply_attributes()
{
    if (recreateTerrain_)
        CreateGeometry();

    if (neighborsDirty_)
    {
        Scene* scene = GetScene();
        Node* north = scene ? scene->GetNode(northID_) : nullptr;
        Node* south = scene ? scene->GetNode(southID_) : nullptr;
        Node* west = scene ? scene->GetNode(westID_) : nullptr;
        Node* east = scene ? scene->GetNode(eastID_) : nullptr;
        Terrain* northTerrain = north ? north->GetComponent<Terrain>() : nullptr;
        Terrain* southTerrain = south ? south->GetComponent<Terrain>() : nullptr;
        Terrain* westTerrain = west ? west->GetComponent<Terrain>() : nullptr;
        Terrain* eastTerrain = east ? east->GetComponent<Terrain>() : nullptr;
        SetNeighbors(northTerrain, southTerrain, westTerrain, eastTerrain);
        neighborsDirty_ = false;
    }
}

void Terrain::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetEnabled(enabled);
    }
}

void Terrain::SetPatchSize(int size)
{
    if (size < MIN_PATCH_SIZE || size > MAX_PATCH_SIZE || !IsPowerOfTwo((unsigned)size))
        return;

    if (size != patchSize_)
    {
        patchSize_ = size;

        CreateGeometry();
        MarkNetworkUpdate();
    }
}

void Terrain::SetSpacing(const Vector3& spacing)
{
    if (spacing != spacing_)
    {
        spacing_ = spacing;

        CreateGeometry();
        MarkNetworkUpdate();
    }
}

void Terrain::SetMaxLodLevels(unsigned levels)
{
    levels = Clamp(levels, MIN_LOD_LEVELS, MAX_LOD_LEVELS);
    if (levels != maxLodLevels_)
    {
        maxLodLevels_ = levels;
        lastPatchSize_ = 0; // Force full recreate

        CreateGeometry();
        MarkNetworkUpdate();
    }
}

void Terrain::SetOcclusionLodLevel(i32 level)
{
    assert(level >= 0 || level == NINDEX);

    if (level != occlusionLodLevel_)
    {
        occlusionLodLevel_ = level;
        lastPatchSize_ = 0; // Force full recreate

        CreateGeometry();
        MarkNetworkUpdate();
    }
}

void Terrain::SetSmoothing(bool enable)
{
    if (enable != smoothing_)
    {
        smoothing_ = enable;

        CreateGeometry();
        MarkNetworkUpdate();
    }
}

bool Terrain::SetHeightMap(Image* image)
{
    bool success = SetHeightMapInternal(image, true);

    MarkNetworkUpdate();
    return success;
}

void Terrain::SetMaterial(Material* material)
{
    material_ = material;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetMaterial(material);
    }

    MarkNetworkUpdate();
}

void Terrain::SetNorthNeighbor(Terrain* north)
{
    if (north == north_)
        return;

    if (north_ && north_->GetNode())
        UnsubscribeFromEvent(north_->GetNode(), E_TERRAINCREATED);

    north_ = north;
    if (north_ && north_->GetNode())
    {
        northID_ = north_->GetNode()->GetID();
        subscribe_to_event(north_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }

    UpdateEdgePatchNeighbors();
    MarkNetworkUpdate();
}

void Terrain::SetSouthNeighbor(Terrain* south)
{
    if (south == south_)
        return;

    if (south_ && south_->GetNode())
        UnsubscribeFromEvent(south_->GetNode(), E_TERRAINCREATED);

    south_ = south;
    if (south_ && south_->GetNode())
    {
        southID_ = south_->GetNode()->GetID();
        subscribe_to_event(south_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }

    UpdateEdgePatchNeighbors();
    MarkNetworkUpdate();
}

void Terrain::SetWestNeighbor(Terrain* west)
{
    if (west == west_)
        return;

    if (west_ && west_->GetNode())
        UnsubscribeFromEvent(west_->GetNode(), E_TERRAINCREATED);

    west_ = west;
    if (west_ && west_->GetNode())
    {
        westID_ = west_->GetNode()->GetID();
        subscribe_to_event(west_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }

    UpdateEdgePatchNeighbors();
    MarkNetworkUpdate();
}

void Terrain::SetEastNeighbor(Terrain* east)
{
    if (east == east_)
        return;

    if (east_ && east_->GetNode())
        UnsubscribeFromEvent(east_->GetNode(), E_TERRAINCREATED);

    east_ = east;
    if (east_ && east_->GetNode())
    {
        eastID_ = east_->GetNode()->GetID();
        subscribe_to_event(east_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }

    UpdateEdgePatchNeighbors();
    MarkNetworkUpdate();
}

void Terrain::SetNeighbors(Terrain* north, Terrain* south, Terrain* west, Terrain* east)
{
    if (north_ && north_->GetNode())
        UnsubscribeFromEvent(north_->GetNode(), E_TERRAINCREATED);
    if (south_ && south_->GetNode())
        UnsubscribeFromEvent(south_->GetNode(), E_TERRAINCREATED);
    if (west_ && west_->GetNode())
        UnsubscribeFromEvent(west_->GetNode(), E_TERRAINCREATED);
    if (east_ && east_->GetNode())
        UnsubscribeFromEvent(east_->GetNode(), E_TERRAINCREATED);

    north_ = north;
    if (north_ && north_->GetNode())
    {
        northID_ = north_->GetNode()->GetID();
        subscribe_to_event(north_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }
    south_ = south;
    if (south_ && south_->GetNode())
    {
        southID_ = south_->GetNode()->GetID();
        subscribe_to_event(south_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }
    west_ = west;
    if (west_ && west_->GetNode())
    {
        westID_ = west_->GetNode()->GetID();
        subscribe_to_event(west_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }
    east_ = east;
    if (east_ && east_->GetNode())
    {
        eastID_ = east_->GetNode()->GetID();
        subscribe_to_event(east_->GetNode(), E_TERRAINCREATED, DV_HANDLER(Terrain, HandleNeighborTerrainCreated));
    }

    UpdateEdgePatchNeighbors();
    MarkNetworkUpdate();
}

void Terrain::SetDrawDistance(float distance)
{
    drawDistance_ = distance;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetDrawDistance(distance);
    }

    MarkNetworkUpdate();
}

void Terrain::SetShadowDistance(float distance)
{
    shadowDistance_ = distance;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetShadowDistance(distance);
    }

    MarkNetworkUpdate();
}

void Terrain::SetLodBias(float bias)
{
    lodBias_ = bias;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetLodBias(bias);
    }

    MarkNetworkUpdate();
}

void Terrain::SetViewMask(unsigned mask)
{
    viewMask_ = mask;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetViewMask(mask);
    }

    MarkNetworkUpdate();
}

void Terrain::SetLightMask(unsigned mask)
{
    lightMask_ = mask;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetLightMask(mask);
    }

    MarkNetworkUpdate();
}

void Terrain::SetShadowMask(unsigned mask)
{
    shadowMask_ = mask;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetShadowMask(mask);
    }

    MarkNetworkUpdate();
}

void Terrain::SetZoneMask(unsigned mask)
{
    zoneMask_ = mask;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetZoneMask(mask);
    }

    MarkNetworkUpdate();
}

void Terrain::SetMaxLights(unsigned num)
{
    max_lights_ = num;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetMaxLights(num);
    }

    MarkNetworkUpdate();
}

void Terrain::SetCastShadows(bool enable)
{
    castShadows_ = enable;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetCastShadows(enable);
    }

    MarkNetworkUpdate();
}

void Terrain::SetOccluder(bool enable)
{
    occluder_ = enable;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetOccluder(enable);
    }

    MarkNetworkUpdate();
}

void Terrain::SetOccludee(bool enable)
{
    occludee_ = enable;

    for (const WeakPtr<TerrainPatch>& patch : patches_)
    {
        if (patch)
            patch->SetOccludee(enable);
    }

    MarkNetworkUpdate();
}

void Terrain::ApplyHeightMap()
{
    if (heightMap_)
        CreateGeometry();
}

Image* Terrain::GetHeightMap() const
{
    return heightMap_;
}

Material* Terrain::GetMaterial() const
{
    return material_;
}

TerrainPatch* Terrain::GetPatch(i32 index) const
{
    assert(index >= 0);
    return index < patches_.Size() ? patches_[index] : nullptr;
}

TerrainPatch* Terrain::GetPatch(int x, int z) const
{
    if (x < 0 || x >= numPatches_.x || z < 0 || z >= numPatches_.y)
        return nullptr;
    else
        return GetPatch(z * numPatches_.x + x);
}

TerrainPatch* Terrain::GetNeighborPatch(int x, int z) const
{
    if (z >= numPatches_.y && north_)
        return north_->GetPatch(x, z - numPatches_.y);
    else if (z < 0 && south_)
        return south_->GetPatch(x, z + south_->GetNumPatches().y);
    else if (x < 0 && west_)
        return west_->GetPatch(x + west_->GetNumPatches().x, z);
    else if (x >= numPatches_.x && east_)
        return east_->GetPatch(x - numPatches_.x, z);
    else
        return GetPatch(x, z);
}

float Terrain::GetHeight(const Vector3& worldPosition) const
{
    if (node_)
    {
        Vector3 position = node_->GetWorldTransform().Inverse() * worldPosition;
        float xPos = (position.x - patchWorldOrigin_.x) / spacing_.x;
        float zPos = (position.z - patchWorldOrigin_.y) / spacing_.z;
        float xFrac = Fract(xPos);
        float zFrac = Fract(zPos);
        float h1, h2, h3;

        if (xFrac + zFrac >= 1.0f)
        {
            h1 = GetRawHeight((unsigned)xPos + 1, (unsigned)zPos + 1);
            h2 = GetRawHeight((unsigned)xPos, (unsigned)zPos + 1);
            h3 = GetRawHeight((unsigned)xPos + 1, (unsigned)zPos);
            xFrac = 1.0f - xFrac;
            zFrac = 1.0f - zFrac;
        }
        else
        {
            h1 = GetRawHeight((unsigned)xPos, (unsigned)zPos);
            h2 = GetRawHeight((unsigned)xPos + 1, (unsigned)zPos);
            h3 = GetRawHeight((unsigned)xPos, (unsigned)zPos + 1);
        }

        float h = h1 * (1.0f - xFrac - zFrac) + h2 * xFrac + h3 * zFrac;
        /// \todo This assumes that the terrain scene node is upright
        return node_->GetWorldScale().y * h + node_->GetWorldPosition().y;
    }
    else
        return 0.0f;
}

Vector3 Terrain::GetNormal(const Vector3& worldPosition) const
{
    if (node_)
    {
        Vector3 position = node_->GetWorldTransform().Inverse() * worldPosition;
        float xPos = (position.x - patchWorldOrigin_.x) / spacing_.x;
        float zPos = (position.z - patchWorldOrigin_.y) / spacing_.z;
        float xFrac = Fract(xPos);
        float zFrac = Fract(zPos);
        Vector3 n1, n2, n3;

        if (xFrac + zFrac >= 1.0f)
        {
            n1 = GetRawNormal((unsigned)xPos + 1, (unsigned)zPos + 1);
            n2 = GetRawNormal((unsigned)xPos, (unsigned)zPos + 1);
            n3 = GetRawNormal((unsigned)xPos + 1, (unsigned)zPos);
            xFrac = 1.0f - xFrac;
            zFrac = 1.0f - zFrac;
        }
        else
        {
            n1 = GetRawNormal((unsigned)xPos, (unsigned)zPos);
            n2 = GetRawNormal((unsigned)xPos + 1, (unsigned)zPos);
            n3 = GetRawNormal((unsigned)xPos, (unsigned)zPos + 1);
        }

        Vector3 n = (n1 * (1.0f - xFrac - zFrac) + n2 * xFrac + n3 * zFrac).normalized();
        return node_->GetWorldRotation() * n;
    }
    else
        return Vector3::UP;
}

IntVector2 Terrain::WorldToHeightMap(const Vector3& worldPosition) const
{
    if (!node_)
        return IntVector2::ZERO;

    Vector3 position = node_->GetWorldTransform().Inverse() * worldPosition;
    auto xPos = RoundToInt((position.x - patchWorldOrigin_.x) / spacing_.x);
    auto zPos = RoundToInt((position.z - patchWorldOrigin_.y) / spacing_.z);
    xPos = Clamp(xPos, 0, numVertices_.x - 1);
    zPos = Clamp(zPos, 0, numVertices_.y - 1);

    return IntVector2(xPos, numVertices_.y - 1 - zPos);
}

Vector3 Terrain::HeightMapToWorld(const IntVector2& pixelPosition) const
{
    if (!node_)
        return Vector3::ZERO;

    IntVector2 pos(pixelPosition.x, numVertices_.y - 1 - pixelPosition.y);
    auto xPos = pos.x * spacing_.x + patchWorldOrigin_.x;
    auto zPos = pos.y * spacing_.z + patchWorldOrigin_.y;
    Vector3 lPos(xPos, 0.0f, zPos);
    Vector3 wPos = node_->GetWorldTransform() * lPos;
    wPos.y = GetHeight(wPos);

    return wPos;
}

void Terrain::CreatePatchGeometry(TerrainPatch* patch)
{
    ZoneScoped;

    auto row = (unsigned)(patchSize_ + 1);
    VertexBuffer* vertexBuffer = patch->GetVertexBuffer();
    Geometry* geometry = patch->GetGeometry();
    Geometry* maxLodGeometry = patch->GetMaxLodGeometry();
    Geometry* occlusionGeometry = patch->GetOcclusionGeometry();

    if (vertexBuffer->GetVertexCount() != row * row)
    {
        vertexBuffer->SetSize(row * row, VertexElements::Position | VertexElements::Normal
                                         | VertexElements::TexCoord1 | VertexElements::Tangent);
    }

    SharedArrayPtr<byte> cpuVertexData(new byte[row * row * sizeof(Vector3)]);
    SharedArrayPtr<byte> occlusionCpuVertexData(new byte[row * row * sizeof(Vector3)]);

    auto* vertexData = (float*)vertexBuffer->Lock(0, vertexBuffer->GetVertexCount());
    auto* positionData = (float*)cpuVertexData.Get();
    auto* occlusionData = (float*)occlusionCpuVertexData.Get();
    BoundingBox box;

    i32 occlusionLevel = occlusionLodLevel_;
    if (occlusionLevel > numLodLevels_ - 1 || occlusionLevel == NINDEX)
        occlusionLevel = numLodLevels_ - 1;

    if (vertexData)
    {
        const IntVector2& coords = patch->GetCoordinates();
        unsigned lodExpand = (1u << (occlusionLevel)) - 1;
        unsigned halfLodExpand = (1u << (occlusionLevel)) / 2;

        for (i32 z = 0; z <= patchSize_; ++z)
        {
            for (i32 x = 0; x <= patchSize_; ++x)
            {
                int xPos = coords.x * patchSize_ + x;
                int zPos = coords.y * patchSize_ + z;

                // Position
                Vector3 position((float)x * spacing_.x, GetRawHeight(xPos, zPos), (float)z * spacing_.z);
                *vertexData++ = position.x;
                *vertexData++ = position.y;
                *vertexData++ = position.z;
                *positionData++ = position.x;
                *positionData++ = position.y;
                *positionData++ = position.z;

                box.Merge(position);

                // For vertices that are part of the occlusion LOD, calculate the minimum height in the neighborhood
                // to prevent false positive occlusion due to inaccuracy between occlusion LOD & visible LOD
                float minHeight = position.y;
                if (halfLodExpand > 0 && (x & lodExpand) == 0 && (z & lodExpand) == 0)
                {
                    int minX = Max(xPos - halfLodExpand, 0);
                    int maxX = Min(xPos + halfLodExpand, numVertices_.x - 1);
                    int minZ = Max(zPos - halfLodExpand, 0);
                    int maxZ = Min(zPos + halfLodExpand, numVertices_.y - 1);
                    for (int nZ = minZ; nZ <= maxZ; ++nZ)
                    {
                        for (int nX = minX; nX <= maxX; ++nX)
                            minHeight = Min(minHeight, GetRawHeight(nX, nZ));
                    }
                }
                *occlusionData++ = position.x;
                *occlusionData++ = minHeight;
                *occlusionData++ = position.z;

                // Normal
                Vector3 normal = GetRawNormal(xPos, zPos);
                *vertexData++ = normal.x;
                *vertexData++ = normal.y;
                *vertexData++ = normal.z;

                // Texture coordinate
                Vector2 texCoord((float)xPos / (float)(numVertices_.x - 1), 1.0f - (float)zPos / (float)(numVertices_.y - 1));
                *vertexData++ = texCoord.x;
                *vertexData++ = texCoord.y;

                // Tangent
                Vector3 xyz = (Vector3::RIGHT - normal * normal.DotProduct(Vector3::RIGHT)).normalized();
                *vertexData++ = xyz.x;
                *vertexData++ = xyz.y;
                *vertexData++ = xyz.z;
                *vertexData++ = 1.0f;
            }
        }

        vertexBuffer->Unlock();
        vertexBuffer->ClearDataLost();
    }

    patch->SetBoundingBox(box);

    if (drawRanges_.Size())
    {
        unsigned occlusionDrawRange = occlusionLevel << 4u;

        geometry->SetIndexBuffer(indexBuffer_);
        geometry->SetDrawRange(TRIANGLE_LIST, drawRanges_[0].first_, drawRanges_[0].second_, false);
        geometry->SetRawVertexData(cpuVertexData, VertexElements::Position);
        maxLodGeometry->SetIndexBuffer(indexBuffer_);
        maxLodGeometry->SetDrawRange(TRIANGLE_LIST, drawRanges_[0].first_, drawRanges_[0].second_, false);
        maxLodGeometry->SetRawVertexData(cpuVertexData, VertexElements::Position);
        occlusionGeometry->SetIndexBuffer(indexBuffer_);
        occlusionGeometry->SetDrawRange(TRIANGLE_LIST, drawRanges_[occlusionDrawRange].first_, drawRanges_[occlusionDrawRange].second_, false);
        occlusionGeometry->SetRawVertexData(occlusionCpuVertexData, VertexElements::Position);
    }

    patch->ResetLod();
}

void Terrain::UpdatePatchLod(TerrainPatch* patch)
{
    Geometry* geometry = patch->GetGeometry();

    // All LOD levels except the coarsest have 16 versions for stitching
    i32 lodLevel = patch->GetLodLevel();
    i32 drawRangeIndex = lodLevel << 4;
    if (lodLevel < numLodLevels_ - 1)
    {
        TerrainPatch* north = patch->GetNorthPatch();
        TerrainPatch* south = patch->GetSouthPatch();
        TerrainPatch* west = patch->GetWestPatch();
        TerrainPatch* east = patch->GetEastPatch();

        if (north && north->GetLodLevel() > lodLevel)
            drawRangeIndex |= STITCH_NORTH;
        if (south && south->GetLodLevel() > lodLevel)
            drawRangeIndex |= STITCH_SOUTH;
        if (west && west->GetLodLevel() > lodLevel)
            drawRangeIndex |= STITCH_WEST;
        if (east && east->GetLodLevel() > lodLevel)
            drawRangeIndex |= STITCH_EAST;
    }

    if (drawRangeIndex < drawRanges_.Size())
        geometry->SetDrawRange(TRIANGLE_LIST, drawRanges_[drawRangeIndex].first_, drawRanges_[drawRangeIndex].second_, false);
}

void Terrain::SetMaterialAttr(const ResourceRef& value)
{
    SetMaterial(DV_RES_CACHE.GetResource<Material>(value.name_));
}

void Terrain::SetHeightMapAttr(const ResourceRef& value)
{
    auto* image = DV_RES_CACHE.GetResource<Image>(value.name_);
    SetHeightMapInternal(image, false);
}

void Terrain::SetPatchSizeAttr(int value)
{
    if (value < MIN_PATCH_SIZE || value > MAX_PATCH_SIZE || !IsPowerOfTwo((unsigned)value))
        return;

    if (value != patchSize_)
    {
        patchSize_ = value;
        recreateTerrain_ = true;
    }
}

void Terrain::SetMaxLodLevelsAttr(unsigned value)
{
    value = Clamp(value, MIN_LOD_LEVELS, MAX_LOD_LEVELS);

    if (value != maxLodLevels_)
    {
        maxLodLevels_ = value;
        lastPatchSize_ = 0; // Force full recreate
        recreateTerrain_ = true;
    }
}

void Terrain::SetOcclusionLodLevelAttr(i32 value)
{
    assert(value >= 0 || value == NINDEX);

    if (value != occlusionLodLevel_)
    {
        occlusionLodLevel_ = value;
        lastPatchSize_ = 0; // Force full recreate
        recreateTerrain_ = true;
    }
}

ResourceRef Terrain::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

ResourceRef Terrain::GetHeightMapAttr() const
{
    return GetResourceRef(heightMap_, Image::GetTypeStatic());
}

void Terrain::CreateGeometry()
{
    recreateTerrain_ = false;

    if (!node_)
        return;

    DV_PROFILE(CreateTerrainGeometry);

    unsigned prevNumPatches = patches_.Size();

    // Determine number of LOD levels
    auto lodSize = (unsigned)patchSize_;
    numLodLevels_ = 1;
    while (lodSize > MIN_PATCH_SIZE && numLodLevels_ < maxLodLevels_)
    {
        lodSize >>= 1;
        ++numLodLevels_;
    }

    // Determine total terrain size
    patchWorldSize_ = Vector2(spacing_.x * (float)patchSize_, spacing_.z * (float)patchSize_);
    bool updateAll = false;

    if (heightMap_)
    {
        numPatches_ = IntVector2((heightMap_->GetWidth() - 1) / patchSize_, (heightMap_->GetHeight() - 1) / patchSize_);
        numVertices_ = IntVector2(numPatches_.x * patchSize_ + 1, numPatches_.y * patchSize_ + 1);
        patchWorldOrigin_ =
            Vector2(-0.5f * (float)numPatches_.x * patchWorldSize_.x, -0.5f * (float)numPatches_.y * patchWorldSize_.y);
        if (numVertices_ != lastNumVertices_ || lastSpacing_ != spacing_ || patchSize_ != lastPatchSize_)
            updateAll = true;
        auto newDataSize = (unsigned)(numVertices_.x * numVertices_.y);

        // Create new height data if terrain size changed
        if (!heightData_ || updateAll)
            heightData_ = new float[newDataSize];

        // Ensure that the source (unsmoothed) data exists if smoothing is active
        if (smoothing_ && (!sourceHeightData_ || updateAll))
        {
            sourceHeightData_ = new float[newDataSize];
            updateAll = true;
        }
        else if (!smoothing_)
            sourceHeightData_.Reset();
    }
    else
    {
        numPatches_ = IntVector2::ZERO;
        numVertices_ = IntVector2::ZERO;
        patchWorldOrigin_ = Vector2::ZERO;
        heightData_.Reset();
        sourceHeightData_.Reset();
    }

    lastNumVertices_ = numVertices_;
    lastPatchSize_ = patchSize_;
    lastSpacing_ = spacing_;

    // Remove old patch nodes which are not needed
    if (updateAll)
    {
        DV_PROFILE(RemoveOldPatches);

        Vector<Node*> oldPatchNodes;
        node_->GetChildrenWithComponent<TerrainPatch>(oldPatchNodes);
        for (Vector<Node*>::Iterator i = oldPatchNodes.Begin(); i != oldPatchNodes.End(); ++i)
        {
            bool nodeOk = false;
            Vector<String> coords = (*i)->GetName().Substring(6).Split('_');
            if (coords.Size() == 2)
            {
                int x = ToI32(coords[0]);
                int z = ToI32(coords[1]);
                if (x < numPatches_.x && z < numPatches_.y)
                    nodeOk = true;
            }

            if (!nodeOk)
                node_->RemoveChild(*i);
        }
    }

    // Keep track of which patches actually need an update
    Vector<bool> dirtyPatches(numPatches_.x * numPatches_.y, updateAll);

    patches_.Clear();

    if (heightMap_)
    {
        // Copy heightmap data
        const unsigned char* src = heightMap_->GetData();
        float* dest = smoothing_ ? sourceHeightData_ : heightData_;
        unsigned imgComps = heightMap_->GetComponents();
        unsigned imgRow = heightMap_->GetWidth() * imgComps;
        IntRect updateRegion(-1, -1, -1, -1);

        if (imgComps == 1)
        {
            DV_PROFILE(CopyHeightData);

            for (int z = 0; z < numVertices_.y; ++z)
            {
                for (int x = 0; x < numVertices_.x; ++x)
                {
                    float newHeight = (float)src[imgRow * (numVertices_.y - 1 - z) + x] * spacing_.y;

                    if (updateAll)
                        *dest = newHeight;
                    else
                    {
                        if (*dest != newHeight)
                        {
                            *dest = newHeight;
                            GrowUpdateRegion(updateRegion, x, z);
                        }
                    }

                    ++dest;
                }
            }
        }
        else
        {
            DV_PROFILE(CopyHeightData);

            // If more than 1 component, use the green channel for more accuracy
            for (int z = 0; z < numVertices_.y; ++z)
            {
                for (int x = 0; x < numVertices_.x; ++x)
                {
                    float newHeight = ((float)src[imgRow * (numVertices_.y - 1 - z) + imgComps * x] +
                                       (float)src[imgRow * (numVertices_.y - 1 - z) + imgComps * x + 1] / 256.0f) * spacing_.y;

                    if (updateAll)
                        *dest = newHeight;
                    else
                    {
                        if (*dest != newHeight)
                        {
                            *dest = newHeight;
                            GrowUpdateRegion(updateRegion, x, z);
                        }
                    }

                    ++dest;
                }
            }
        }

        // If updating a region of the heightmap, check which patches change
        if (!updateAll)
        {
            int lodExpand = 1u << (numLodLevels_ - 1);
            // Expand the right & bottom 1 pixel more, as patches share vertices at the edge
            updateRegion.left_ -= lodExpand;
            updateRegion.right_ += lodExpand + 1;
            updateRegion.top_ -= lodExpand;
            updateRegion.bottom_ += lodExpand + 1;

            int sX = Max(updateRegion.left_ / patchSize_, 0);
            int eX = Min(updateRegion.right_ / patchSize_, numPatches_.x - 1);
            int sY = Max(updateRegion.top_ / patchSize_, 0);
            int eY = Min(updateRegion.bottom_ / patchSize_, numPatches_.y - 1);
            for (int y = sY; y <= eY; ++y)
            {
                for (int x = sX; x <= eX; ++x)
                    dirtyPatches[y * numPatches_.x + x] = true;
            }
        }

        patches_.Reserve((unsigned)(numPatches_.x * numPatches_.y));

        bool enabled = IsEnabledEffective();

        {
            DV_PROFILE(CreatePatches);

            // Create patches and set node transforms
            for (int z = 0; z < numPatches_.y; ++z)
            {
                for (int x = 0; x < numPatches_.x; ++x)
                {
                    String nodeName = "Patch_" + String(x) + "_" + String(z);
                    Node* patchNode = node_->GetChild(nodeName);

                    if (!patchNode)
                    {
                        // Create the patch scene node as local and temporary so that it is not unnecessarily serialized to either
                        // file or replicated over the network
                        patchNode = node_->CreateTemporaryChild(nodeName, LOCAL);
                    }

                    patchNode->SetPosition(Vector3(patchWorldOrigin_.x + (float)x * patchWorldSize_.x, 0.0f,
                        patchWorldOrigin_.y + (float)z * patchWorldSize_.y));

                    auto* patch = patchNode->GetComponent<TerrainPatch>();
                    if (!patch)
                    {
                        patch = patchNode->create_component<TerrainPatch>();
                        patch->SetOwner(this);
                        patch->SetCoordinates(IntVector2(x, z));

                        // Copy initial drawable parameters
                        patch->SetEnabled(enabled);
                        patch->SetMaterial(material_);
                        patch->SetDrawDistance(drawDistance_);
                        patch->SetShadowDistance(shadowDistance_);
                        patch->SetLodBias(lodBias_);
                        patch->SetViewMask(viewMask_);
                        patch->SetLightMask(lightMask_);
                        patch->SetShadowMask(shadowMask_);
                        patch->SetZoneMask(zoneMask_);
                        patch->SetMaxLights(max_lights_);
                        patch->SetCastShadows(castShadows_);
                        patch->SetOccluder(occluder_);
                        patch->SetOccludee(occludee_);
                    }

                    patches_.Push(WeakPtr<TerrainPatch>(patch));
                }
            }
        }

        // Create the shared index data
        if (updateAll)
            CreateIndexData();

        // Create vertex data for patches. First update smoothing to ensure normals are calculated correctly across patch borders
        if (smoothing_)
        {
            DV_PROFILE(UpdateSmoothing);

            for (i32 i = 0; i < patches_.Size(); ++i)
            {
                if (dirtyPatches[i])
                {
                    TerrainPatch* patch = patches_[i];
                    const IntVector2& coords = patch->GetCoordinates();
                    int startX = coords.x * patchSize_;
                    int endX = startX + patchSize_;
                    int startZ = coords.y * patchSize_;
                    int endZ = startZ + patchSize_;

                    for (int z = startZ; z <= endZ; ++z)
                    {
                        for (int x = startX; x <= endX; ++x)
                        {
                            float smoothedHeight = (
                                GetSourceHeight(x - 1, z - 1) + GetSourceHeight(x, z - 1) * 2.0f + GetSourceHeight(x + 1, z - 1) +
                                GetSourceHeight(x - 1, z) * 2.0f + GetSourceHeight(x, z) * 4.0f + GetSourceHeight(x + 1, z) * 2.0f +
                                GetSourceHeight(x - 1, z + 1) + GetSourceHeight(x, z + 1) * 2.0f + GetSourceHeight(x + 1, z + 1)
                            ) / 16.0f;

                            heightData_[z * numVertices_.x + x] = smoothedHeight;
                        }
                    }
                }
            }
        }

        for (i32 i = 0; i < patches_.Size(); ++i)
        {
            TerrainPatch* patch = patches_[i];

            if (dirtyPatches[i])
            {
                CreatePatchGeometry(patch);
                CalculateLodErrors(patch);
            }

            SetPatchNeighbors(patch);
        }
    }

    // Send event only if new geometry was generated, or the old was cleared
    if (patches_.Size() || prevNumPatches)
    {
        using namespace TerrainCreated;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_NODE] = node_;
        node_->SendEvent(E_TERRAINCREATED, eventData);
    }
}

void Terrain::CreateIndexData()
{
    DV_PROFILE(CreateIndexData);

    Vector<unsigned short> indices;
    drawRanges_.Clear();
    auto row = (unsigned)(patchSize_ + 1);

    /* Build index data for each LOD level. Each LOD level except the lowest can stitch to the next lower LOD from the edges:
       north, south, west, east, or any combination of them, requiring 16 different versions of each LOD level's index data

       Normal edge:     Stitched edge:
       +----+----+      +---------+
       |\   |\   |      |\       /|
       | \  | \  |      | \     / |
       |  \ |  \ |      |  \   /  |
       |   \|   \|      |   \ /   |
       +----+----+      +----+----+
    */
    for (unsigned i = 0; i < numLodLevels_; ++i)
    {
        unsigned combinations = (i < numLodLevels_ - 1) ? 16 : 1;
        int skip = 1u << i;

        for (unsigned j = 0; j < combinations; ++j)
        {
            unsigned indexStart = indices.Size();

            int zStart = 0;
            int xStart = 0;
            int zEnd = patchSize_;
            int xEnd = patchSize_;

            if (j & STITCH_NORTH)
                zEnd -= skip;
            if (j & STITCH_SOUTH)
                zStart += skip;
            if (j & STITCH_WEST)
                xStart += skip;
            if (j & STITCH_EAST)
                xEnd -= skip;

            // Build the main grid
            for (int z = zStart; z < zEnd; z += skip)
            {
                for (int x = xStart; x < xEnd; x += skip)
                {
                    indices.Push((unsigned short)((z + skip) * row + x));
                    indices.Push((unsigned short)(z * row + x + skip));
                    indices.Push((unsigned short)(z * row + x));
                    indices.Push((unsigned short)((z + skip) * row + x));
                    indices.Push((unsigned short)((z + skip) * row + x + skip));
                    indices.Push((unsigned short)(z * row + x + skip));
                }
            }

            // Build the north edge
            if (j & STITCH_NORTH)
            {
                int z = patchSize_ - skip;
                for (int x = 0; x < patchSize_; x += skip * 2)
                {
                    if (x > 0 || (j & STITCH_WEST) == 0)
                    {
                        indices.Push((unsigned short)((z + skip) * row + x));
                        indices.Push((unsigned short)(z * row + x + skip));
                        indices.Push((unsigned short)(z * row + x));
                    }
                    indices.Push((unsigned short)((z + skip) * row + x));
                    indices.Push((unsigned short)((z + skip) * row + x + 2 * skip));
                    indices.Push((unsigned short)(z * row + x + skip));
                    if (x < patchSize_ - skip * 2 || (j & STITCH_EAST) == 0)
                    {
                        indices.Push((unsigned short)((z + skip) * row + x + 2 * skip));
                        indices.Push((unsigned short)(z * row + x + 2 * skip));
                        indices.Push((unsigned short)(z * row + x + skip));
                    }
                }
            }

            // Build the south edge
            if (j & STITCH_SOUTH)
            {
                int z = 0;
                for (int x = 0; x < patchSize_; x += skip * 2)
                {
                    if (x > 0 || (j & STITCH_WEST) == 0)
                    {
                        indices.Push((unsigned short)((z + skip) * row + x));
                        indices.Push((unsigned short)((z + skip) * row + x + skip));
                        indices.Push((unsigned short)(z * row + x));
                    }
                    indices.Push((unsigned short)(z * row + x));
                    indices.Push((unsigned short)((z + skip) * row + x + skip));
                    indices.Push((unsigned short)(z * row + x + 2 * skip));
                    if (x < patchSize_ - skip * 2 || (j & STITCH_EAST) == 0)
                    {
                        indices.Push((unsigned short)((z + skip) * row + x + skip));
                        indices.Push((unsigned short)((z + skip) * row + x + 2 * skip));
                        indices.Push((unsigned short)(z * row + x + 2 * skip));
                    }
                }
            }

            // Build the west edge
            if (j & STITCH_WEST)
            {
                int x = 0;
                for (int z = 0; z < patchSize_; z += skip * 2)
                {
                    if (z > 0 || (j & STITCH_SOUTH) == 0)
                    {
                        indices.Push((unsigned short)(z * row + x));
                        indices.Push((unsigned short)((z + skip) * row + x + skip));
                        indices.Push((unsigned short)(z * row + x + skip));
                    }
                    indices.Push((unsigned short)((z + 2 * skip) * row + x));
                    indices.Push((unsigned short)((z + skip) * row + x + skip));
                    indices.Push((unsigned short)(z * row + x));
                    if (z < patchSize_ - skip * 2 || (j & STITCH_NORTH) == 0)
                    {
                        indices.Push((unsigned short)((z + 2 * skip) * row + x));
                        indices.Push((unsigned short)((z + 2 * skip) * row + x + skip));
                        indices.Push((unsigned short)((z + skip) * row + x + skip));
                    }
                }
            }

            // Build the east edge
            if (j & STITCH_EAST)
            {
                int x = patchSize_ - skip;
                for (int z = 0; z < patchSize_; z += skip * 2)
                {
                    if (z > 0 || (j & STITCH_SOUTH) == 0)
                    {
                        indices.Push((unsigned short)(z * row + x));
                        indices.Push((unsigned short)((z + skip) * row + x));
                        indices.Push((unsigned short)(z * row + x + skip));
                    }
                    indices.Push((unsigned short)((z + skip) * row + x));
                    indices.Push((unsigned short)((z + 2 * skip) * row + x + skip));
                    indices.Push((unsigned short)(z * row + x + skip));
                    if (z < patchSize_ - skip * 2 || (j & STITCH_NORTH) == 0)
                    {
                        indices.Push((unsigned short)((z + skip) * row + x));
                        indices.Push((unsigned short)((z + 2 * skip) * row + x));
                        indices.Push((unsigned short)((z + 2 * skip) * row + x + skip));
                    }
                }
            }

            drawRanges_.Push(MakePair(indexStart, indices.Size() - indexStart));
        }
    }

    indexBuffer_->SetSize(indices.Size(), false);
    indexBuffer_->SetData(&indices[0]);
}

float Terrain::GetRawHeight(int x, int z) const
{
    if (!heightData_)
        return 0.0f;

    x = Clamp(x, 0, numVertices_.x - 1);
    z = Clamp(z, 0, numVertices_.y - 1);
    return heightData_[z * numVertices_.x + x];
}

float Terrain::GetSourceHeight(int x, int z) const
{
    if (!sourceHeightData_)
        return 0.0f;

    x = Clamp(x, 0, numVertices_.x - 1);
    z = Clamp(z, 0, numVertices_.y - 1);
    return sourceHeightData_[z * numVertices_.x + x];
}

float Terrain::GetLodHeight(int x, int z, unsigned lodLevel) const
{
    unsigned offset = 1u << lodLevel;
    auto xFrac = (float)(x % offset) / offset;
    auto zFrac = (float)(z % offset) / offset;
    float h1, h2, h3;

    if (xFrac + zFrac >= 1.0f)
    {
        h1 = GetRawHeight(x + offset, z + offset);
        h2 = GetRawHeight(x, z + offset);
        h3 = GetRawHeight(x + offset, z);
        xFrac = 1.0f - xFrac;
        zFrac = 1.0f - zFrac;
    }
    else
    {
        h1 = GetRawHeight(x, z);
        h2 = GetRawHeight(x + offset, z);
        h3 = GetRawHeight(x, z + offset);
    }

    return h1 * (1.0f - xFrac - zFrac) + h2 * xFrac + h3 * zFrac;
}

Vector3 Terrain::GetRawNormal(int x, int z) const
{
    float baseHeight = GetRawHeight(x, z);
    float nSlope = GetRawHeight(x, z - 1) - baseHeight;
    float neSlope = GetRawHeight(x + 1, z - 1) - baseHeight;
    float eSlope = GetRawHeight(x + 1, z) - baseHeight;
    float seSlope = GetRawHeight(x + 1, z + 1) - baseHeight;
    float sSlope = GetRawHeight(x, z + 1) - baseHeight;
    float swSlope = GetRawHeight(x - 1, z + 1) - baseHeight;
    float wSlope = GetRawHeight(x - 1, z) - baseHeight;
    float nwSlope = GetRawHeight(x - 1, z - 1) - baseHeight;
    float up = 0.5f * (spacing_.x + spacing_.z);

    return (Vector3(0.0f, up, nSlope) +
            Vector3(-neSlope, up, neSlope) +
            Vector3(-eSlope, up, 0.0f) +
            Vector3(-seSlope, up, -seSlope) +
            Vector3(0.0f, up, -sSlope) +
            Vector3(swSlope, up, -swSlope) +
            Vector3(wSlope, up, 0.0f) +
            Vector3(nwSlope, up, nwSlope)).normalized();
}

void Terrain::CalculateLodErrors(TerrainPatch* patch)
{
    ZoneScoped;

    const IntVector2& coords = patch->GetCoordinates();
    Vector<float>& lodErrors = patch->GetLodErrors();
    lodErrors.Clear();
    lodErrors.Reserve(numLodLevels_);

    int xStart = coords.x * patchSize_;
    int zStart = coords.y * patchSize_;
    int xEnd = xStart + patchSize_;
    int zEnd = zStart + patchSize_;

    for (unsigned i = 0; i < numLodLevels_; ++i)
    {
        float maxError = 0.0f;
        int divisor = 1u << i;

        if (i > 0)
        {
            for (int z = zStart; z <= zEnd; ++z)
            {
                for (int x = xStart; x <= xEnd; ++x)
                {
                    if (x % divisor || z % divisor)
                    {
                        float error = Abs(GetLodHeight(x, z, i) - GetRawHeight(x, z));
                        maxError = Max(error, maxError);
                    }
                }
            }

            // Set error to be at least same as (half vertex spacing x LOD) to prevent horizontal stretches getting too inaccurate
            maxError = Max(maxError, 0.25f * (spacing_.x + spacing_.z) * (float)(1u << i));
        }

        lodErrors.Push(maxError);
    }
}

void Terrain::SetPatchNeighbors(TerrainPatch* patch)
{
    if (!patch)
        return;

    const IntVector2& coords = patch->GetCoordinates();
    patch->SetNeighbors(GetNeighborPatch(coords.x, coords.y + 1), GetNeighborPatch(coords.x, coords.y - 1),
        GetNeighborPatch(coords.x - 1, coords.y), GetNeighborPatch(coords.x + 1, coords.y));
}

bool Terrain::SetHeightMapInternal(Image* image, bool recreateNow)
{
    if (image && image->IsCompressed())
    {
        DV_LOGERROR("Can not use a compressed image as a terrain heightmap");
        return false;
    }

    // Unsubscribe from the reload event of previous image (if any), then subscribe to the new
    if (heightMap_)
        UnsubscribeFromEvent(heightMap_, E_RELOADFINISHED);
    if (image)
        subscribe_to_event(image, E_RELOADFINISHED, DV_HANDLER(Terrain, HandleHeightMapReloadFinished));

    heightMap_ = image;

    if (recreateNow)
        CreateGeometry();
    else
        recreateTerrain_ = true;

    return true;
}

void Terrain::HandleHeightMapReloadFinished(StringHash /*eventType*/, VariantMap& eventData)
{
    CreateGeometry();
}

void Terrain::HandleNeighborTerrainCreated(StringHash /*eventType*/, VariantMap& eventData)
{
    UpdateEdgePatchNeighbors();
}

void Terrain::UpdateEdgePatchNeighbors()
{
    for (int x = 1; x < numPatches_.x - 1; ++x)
    {
        SetPatchNeighbors(GetPatch(x, 0));
        SetPatchNeighbors(GetPatch(x, numPatches_.y - 1));
    }
    for (int z = 1; z < numPatches_.y - 1; ++z)
    {
        SetPatchNeighbors(GetPatch(0, z));
        SetPatchNeighbors(GetPatch(numPatches_.x - 1, z));
    }

    SetPatchNeighbors(GetPatch(0, 0));
    SetPatchNeighbors(GetPatch(numPatches_.x - 1, 0));
    SetPatchNeighbors(GetPatch(0, numPatches_.y - 1));
    SetPatchNeighbors(GetPatch(numPatches_.x - 1, numPatches_.y - 1));
}

}
