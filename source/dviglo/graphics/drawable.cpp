// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "camera.h"
#include "debug_renderer.h"
#include "geometry.h"
#include "material.h"
#include "octree.h"
#include "renderer.h"
#include "zone.h"
#include "../graphics_api/vertex_buffer.h"
#include "../io/file.h"
#include "../io/log.h"
#include "../scene/scene.h"
#include "graphics.h"

#include "../common/debug_new.h"

namespace dviglo
{

const char* GEOMETRY_CATEGORY = "Geometry";

SourceBatch::SourceBatch() = default;

SourceBatch::SourceBatch(const SourceBatch& batch) = default;

SourceBatch::~SourceBatch() = default;

SourceBatch& SourceBatch::operator =(const SourceBatch& rhs)= default;


Drawable::Drawable(DrawableTypes drawableType) :
    boundingBox_(0.0f, 0.0f),
    drawableType_(drawableType),
    worldBoundingBoxDirty_(true),
    castShadows_(false),
    occluder_(false),
    occludee_(true),
    updateQueued_(false),
    zoneDirty_(false),
    octant_(nullptr),
    zone_(nullptr),
    viewMask_(DEFAULT_VIEWMASK),
    lightMask_(DEFAULT_LIGHTMASK),
    shadowMask_(DEFAULT_SHADOWMASK),
    zoneMask_(DEFAULT_ZONEMASK),
    viewFrameNumber_(0),
    distance_(0.0f),
    lodDistance_(0.0f),
    drawDistance_(0.0f),
    shadowDistance_(0.0f),
    sortValue_(0.0f),
    minZ_(0.0f),
    maxZ_(0.0f),
    lodBias_(1.0f),
    basePassFlags_(0),
    max_lights_(0),
    first_light_(nullptr)
{
    if (drawableType == DrawableTypes::Undefined)
        DV_LOGERROR("Drawable with undefined drawableType");
    else if (CountSetBits((u32)drawableType) != 1)
        DV_LOGERROR("Drawable with incorrect drawableType");
}

Drawable::~Drawable()
{
    RemoveFromOctree();
}

void Drawable::RegisterObject()
{
    DV_ATTRIBUTE("Max Lights", max_lights_, 0, AM_DEFAULT);
    DV_ATTRIBUTE("View Mask", viewMask_, DEFAULT_VIEWMASK, AM_DEFAULT);
    DV_ATTRIBUTE("Light Mask", lightMask_, DEFAULT_LIGHTMASK, AM_DEFAULT);
    DV_ATTRIBUTE("Shadow Mask", shadowMask_, DEFAULT_SHADOWMASK, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Zone Mask", GetZoneMask, SetZoneMask, DEFAULT_ZONEMASK, AM_DEFAULT);
}

void Drawable::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled && !octant_)
        AddToOctree();
    else if (!enabled && octant_)
        RemoveFromOctree();
}

void Drawable::ProcessRayQuery(const RayOctreeQuery& query, Vector<RayQueryResult>& results)
{
    float distance = query.ray_.HitDistance(GetWorldBoundingBox());
    if (distance < query.maxDistance_)
    {
        RayQueryResult result;
        result.position_ = query.ray_.origin_ + distance * query.ray_.direction_;
        result.normal_ = -query.ray_.direction_;
        result.distance_ = distance;
        result.drawable_ = this;
        result.node_ = GetNode();
        result.subObject_ = NINDEX;
        results.Push(result);
    }
}

void Drawable::update_batches(const FrameInfo& frame)
{
    const BoundingBox& worldBoundingBox = GetWorldBoundingBox();
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    distance_ = frame.camera_->GetDistance(worldBoundingBox.Center());

    for (unsigned i = 0; i < batches_.Size(); ++i)
    {
        batches_[i].distance_ = distance_;
        batches_[i].worldTransform_ = &worldTransform;
    }

    float scale = worldBoundingBox.Size().DotProduct(DOT_SCALE);
    float newLodDistance = frame.camera_->GetLodDistance(distance_, scale, lodBias_);

    if (newLodDistance != lodDistance_)
        lodDistance_ = newLodDistance;
}

Geometry* Drawable::GetLodGeometry(i32 batchIndex, i32 level)
{
    assert(batchIndex >= 0);
    assert(level >= 0 || level == NINDEX);

    // By default return the visible batch geometry
    if (batchIndex < batches_.Size())
        return batches_[batchIndex].geometry_;
    else
        return nullptr;
}

bool Drawable::DrawOcclusion(OcclusionBuffer* buffer)
{
    return true;
}

void Drawable::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
        debug->AddBoundingBox(GetWorldBoundingBox(), Color::GREEN, depthTest);
}

void Drawable::SetDrawDistance(float distance)
{
    drawDistance_ = distance;
    MarkNetworkUpdate();
}

void Drawable::SetShadowDistance(float distance)
{
    shadowDistance_ = distance;
    MarkNetworkUpdate();
}

void Drawable::SetLodBias(float bias)
{
    lodBias_ = Max(bias, M_EPSILON);
    MarkNetworkUpdate();
}

void Drawable::SetViewMask(mask32 mask)
{
    viewMask_ = mask;
    MarkNetworkUpdate();
}

void Drawable::SetLightMask(mask32 mask)
{
    lightMask_ = mask;
    MarkNetworkUpdate();
}

void Drawable::SetShadowMask(mask32 mask)
{
    shadowMask_ = mask;
    MarkNetworkUpdate();
}

void Drawable::SetZoneMask(mask32 mask)
{
    zoneMask_ = mask;
    // Mark dirty to reset cached zone
    OnMarkedDirty(node_);
    MarkNetworkUpdate();
}

void Drawable::SetMaxLights(i32 num)
{
    assert(num >= 0);
    max_lights_ = num;
    MarkNetworkUpdate();
}

void Drawable::SetCastShadows(bool enable)
{
    castShadows_ = enable;
    MarkNetworkUpdate();
}

void Drawable::SetOccluder(bool enable)
{
    occluder_ = enable;
    MarkNetworkUpdate();
}

void Drawable::SetOccludee(bool enable)
{
    if (enable != occludee_)
    {
        occludee_ = enable;
        // Reinsert to octree to make sure octant occlusion does not erroneously hide this drawable
        if (octant_ && !updateQueued_)
            octant_->GetRoot()->QueueUpdate(this);
        MarkNetworkUpdate();
    }
}

void Drawable::MarkForUpdate()
{
    if (!updateQueued_ && octant_)
        octant_->GetRoot()->QueueUpdate(this);
}

const BoundingBox& Drawable::GetWorldBoundingBox()
{
    if (worldBoundingBoxDirty_)
    {
        OnWorldBoundingBoxUpdate();
        worldBoundingBoxDirty_ = false;
    }

    return worldBoundingBox_;
}

bool Drawable::IsInView() const
{
    // Note: in headless mode there is no renderer subsystem and no view frustum tests are performed, so return
    // always false in that case
    if (GParams::is_headless())
        return false;

    return viewFrameNumber_ == DV_RENDERER.GetFrameInfo().frameNumber_ && !viewCameras_.Empty();
}

bool Drawable::IsInView(Camera* camera) const
{
    if (GParams::is_headless())
        return false;

    return viewFrameNumber_ == DV_RENDERER.GetFrameInfo().frameNumber_ && (!camera || viewCameras_.Contains(camera));
}

bool Drawable::IsInView(const FrameInfo& frame, bool anyCamera) const
{
    return viewFrameNumber_ == frame.frameNumber_ && (anyCamera || viewCameras_.Contains(frame.camera_));
}

void Drawable::SetZone(Zone* zone, bool temporary)
{
    zone_ = zone;

    // If the zone assignment was temporary (inconclusive) set the dirty flag so that it will be re-evaluated on the next frame
    zoneDirty_ = temporary;
}

void Drawable::SetSortValue(float value)
{
    sortValue_ = value;
}

void Drawable::MarkInView(const FrameInfo& frame)
{
    if (frame.frameNumber_ != viewFrameNumber_)
    {
        viewFrameNumber_ = frame.frameNumber_;
        viewCameras_.Resize(1);
        viewCameras_[0] = frame.camera_;
    }
    else
        viewCameras_.Push(frame.camera_);

    basePassFlags_ = 0;
    first_light_ = nullptr;
    lights_.Clear();
    vertexLights_.Clear();
}

void Drawable::MarkInView(i32 frameNumber)
{
    assert(frameNumber > 0);

    if (frameNumber != viewFrameNumber_)
    {
        viewFrameNumber_ = frameNumber;
        viewCameras_.Clear();
    }
}

void Drawable::LimitLights()
{
    // Maximum lights value 0 means unlimited
    if (!max_lights_ || lights_.Size() <= max_lights_)
        return;

    // If more lights than allowed, move to vertex lights and cut the list
    const BoundingBox& box = GetWorldBoundingBox();
    for (unsigned i = 0; i < lights_.Size(); ++i)
        lights_[i]->SetIntensitySortValue(box);

    std::sort(lights_.Begin(), lights_.End(), CompareDrawables);
    vertexLights_.Insert(vertexLights_.End(), lights_.Begin() + max_lights_, lights_.End());
    lights_.Resize(max_lights_);
}

void Drawable::LimitVertexLights(bool removeConvertedLights)
{
    if (removeConvertedLights)
    {
        for (i32 i = vertexLights_.Size() - 1; i >= 0; --i)
        {
            if (!vertexLights_[i]->GetPerVertex())
                vertexLights_.Erase(i);
        }
    }

    if (vertexLights_.Size() <= MAX_VERTEX_LIGHTS)
        return;

    const BoundingBox& box = GetWorldBoundingBox();
    for (unsigned i = 0; i < vertexLights_.Size(); ++i)
        vertexLights_[i]->SetIntensitySortValue(box);

    std::sort(vertexLights_.Begin(), vertexLights_.End(), CompareDrawables);
    vertexLights_.Resize(MAX_VERTEX_LIGHTS);
}

void Drawable::OnNodeSet(Node* node)
{
    if (node)
        node->AddListener(this);
}

void Drawable::OnSceneSet(Scene* scene)
{
    if (scene)
        AddToOctree();
    else
        RemoveFromOctree();
}

void Drawable::OnMarkedDirty(Node* node)
{
    worldBoundingBoxDirty_ = true;
    if (!updateQueued_ && octant_)
        octant_->GetRoot()->QueueUpdate(this);

    // Mark zone assignment dirty when transform changes
    if (node == node_)
        zoneDirty_ = true;
}

void Drawable::AddToOctree()
{
    // Do not add to octree when disabled
    if (!IsEnabledEffective())
        return;

    Scene* scene = GetScene();
    if (scene)
    {
        auto* octree = scene->GetComponent<Octree>();
        if (octree)
            octree->InsertDrawable(this);
        else
            DV_LOGERROR("No Octree component in scene, drawable will not render");
    }
    else
    {
        // We have a mechanism for adding detached nodes to an octree manually, so do not log this error
        //DV_LOGERROR("Node is detached from scene, drawable will not render");
    }
}

void Drawable::RemoveFromOctree()
{
    if (octant_)
    {
        Octree* octree = octant_->GetRoot();
        if (updateQueued_)
            octree->CancelUpdate(this);

        // Perform subclass specific deinitialization if necessary
        OnRemoveFromOctree();

        octant_->RemoveDrawable(this);
    }
}

bool WriteDrawablesToOBJ(const Vector<Drawable*>& drawables, File* outputFile, bool asZUp, bool asRightHanded, bool writeLightmapUV)
{
    // Must track indices independently to deal with potential mismatching of drawables vertex attributes (ie. one with UV, another without, then another with)
    unsigned currentPositionIndex = 1;
    unsigned currentUVIndex = 1;
    unsigned currentNormalIndex = 1;
    bool anythingWritten = false;

    // Write the common "I came from X" comment
    outputFile->WriteLine("# OBJ file exported from Urho3D");

    for (unsigned i = 0; i < drawables.Size(); ++i)
    {
        Drawable* drawable = drawables[i];

        // Only write enabled drawables
        if (!drawable->IsEnabledEffective())
            continue;

        Node* node = drawable->GetNode();
        Matrix3x4 transMat = drawable->GetNode()->GetWorldTransform();
        Matrix3x4 n = transMat.Inverse();
        Matrix3 normalMat = Matrix3(n.m00_, n.m01_, n.m02_, n.m10_, n.m11_, n.m12_, n.m20_, n.m21_, n.m22_);
        normalMat = normalMat.Transpose();

        const Vector<SourceBatch>& batches = drawable->GetBatches();
        for (unsigned geoIndex = 0; geoIndex < batches.Size(); ++geoIndex)
        {
            Geometry* geo = drawable->GetLodGeometry(geoIndex, 0);
            if (geo == nullptr)
                continue;
            if (geo->GetPrimitiveType() != TRIANGLE_LIST)
            {
                DV_LOGERRORF("%s (%u) %s (%u) Geometry %u contains an unsupported geometry type %u", node->GetName().Length() > 0 ? node->GetName().c_str() : "Node", node->GetID(), drawable->GetTypeName().c_str(), drawable->GetID(), geoIndex, (unsigned)geo->GetPrimitiveType());
                continue;
            }

            // If we've reached here than we're going to actually write something to the OBJ file
            anythingWritten = true;

            const byte* vertexData;
            const byte* indexData;
            i32 elementSize, indexSize;
            const Vector<VertexElement>* elements;
            geo->GetRawData(vertexData, elementSize, indexData, indexSize, elements);
            if (!vertexData || !elements)
                continue;

            bool hasPosition = VertexBuffer::HasElement(*elements, TYPE_VECTOR3, SEM_POSITION);
            if (!hasPosition)
            {
                DV_LOGERRORF("%s (%u) %s (%u) Geometry %u contains does not have Vector3 type positions in vertex data", node->GetName().Length() > 0 ? node->GetName().c_str() : "Node", node->GetID(), drawable->GetTypeName().c_str(), drawable->GetID(), geoIndex);
                continue;
            }

            bool hasNormals = VertexBuffer::HasElement(*elements, TYPE_VECTOR3, SEM_NORMAL);
            bool hasUV = VertexBuffer::HasElement(*elements, TYPE_VECTOR2, SEM_TEXCOORD, 0);
            bool hasLMUV = VertexBuffer::HasElement(*elements, TYPE_VECTOR2, SEM_TEXCOORD, 1);

            if (elementSize > 0 && indexSize > 0)
            {
                unsigned vertexStart = geo->GetVertexStart();
                unsigned vertexCount = geo->GetVertexCount();
                unsigned indexStart = geo->GetIndexStart();
                unsigned indexCount = geo->GetIndexCount();

                // Name NodeID DrawableType DrawableID GeometryIndex ("Geo" is included for clarity as StaticModel_32_2 could easily be misinterpreted or even quickly misread as 322)
                // Generated object name example: Node_5_StaticModel_32_Geo_0 ... or ... Bob_5_StaticModel_32_Geo_0
                outputFile->WriteLine(String("o ").AppendWithFormat("%s_%u_%s_%u_Geo_%u", node->GetName().Length() > 0 ? node->GetName().c_str() : "Node", node->GetID(), drawable->GetTypeName().c_str(), drawable->GetID(), geoIndex));

                // Write vertex position
                unsigned positionOffset = VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_POSITION);
                for (unsigned j = 0; j < vertexCount; ++j)
                {
                    Vector3 vertexPosition = *((const Vector3*)(&vertexData[(vertexStart + j) * elementSize + positionOffset]));
                    vertexPosition = transMat * vertexPosition;

                    // Convert coordinates as requested
                    if (asRightHanded)
                        vertexPosition.x_ *= -1;
                    if (asZUp)
                    {
                        float yVal = vertexPosition.y_;
                        vertexPosition.y_ = vertexPosition.z_;
                        vertexPosition.z_ = yVal;
                    }
                    outputFile->WriteLine("v " + String(vertexPosition));
                }

                if (hasNormals)
                {
                    unsigned normalOffset = VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_NORMAL);
                    for (unsigned j = 0; j < vertexCount; ++j)
                    {
                        Vector3 vertexNormal = *((const Vector3*)(&vertexData[(vertexStart + j) * elementSize + normalOffset]));
                        vertexNormal = normalMat * vertexNormal;
                        vertexNormal.Normalize();

                        if (asRightHanded)
                            vertexNormal.x_ *= -1;
                        if (asZUp)
                        {
                            float yVal = vertexNormal.y_;
                            vertexNormal.y_ = vertexNormal.z_;
                            vertexNormal.z_ = yVal;
                        }

                        outputFile->WriteLine("vn " + String(vertexNormal));
                    }
                }

                // Write TEXCOORD1 or TEXCOORD2 if it was chosen
                if (hasUV || (hasLMUV && writeLightmapUV))
                {
                    // if writing Lightmap UV is chosen, only use it if TEXCOORD2 exists, otherwise use TEXCOORD1
                    unsigned texCoordOffset = (writeLightmapUV && hasLMUV) ? VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR2, SEM_TEXCOORD, 1) :
                        VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR2, SEM_TEXCOORD, 0);
                    for (unsigned j = 0; j < vertexCount; ++j)
                    {
                        Vector2 uvCoords = *((const Vector2*)(&vertexData[(vertexStart + j) * elementSize + texCoordOffset]));
                        outputFile->WriteLine("vt " + String(uvCoords));
                    }
                }

                // If we don't have UV but have normals then must write a double-slash to indicate the absence of UV coords, otherwise use a single slash
                const String slashCharacter = hasNormals ? "//" : "/";

                // Amount by which to offset indices in the OBJ vs their values in the Urho3D geometry, basically the lowest index value
                // Compensates for the above vertex writing which doesn't write ALL vertices, just the used ones
                unsigned indexOffset = M_MAX_INT;
                for (unsigned indexIdx = indexStart; indexIdx < indexStart + indexCount; indexIdx++)
                {
                    if (indexSize == 2)
                        indexOffset = Min(indexOffset, (unsigned)*((unsigned short*)(indexData + indexIdx * indexSize)));
                    else
                        indexOffset = Min(indexOffset, *((unsigned*)(indexData + indexIdx * indexSize)));
                }

                for (unsigned indexIdx = indexStart; indexIdx < indexStart + indexCount; indexIdx += 3)
                {
                    // Deal with 16 or 32 bit indices
                    unsigned longIndices[3];
                    if (indexSize == 2)
                    {
                        //16 bit indices
                        unsigned short indices[3];
                        memcpy(indices, indexData + (indexIdx * indexSize), (size_t)indexSize * 3);
                        longIndices[0] = indices[0] - indexOffset;
                        longIndices[1] = indices[1] - indexOffset;
                        longIndices[2] = indices[2] - indexOffset;
                    }
                    else
                    {
                        //32 bit indices
                        unsigned indices[3];
                        memcpy(indices, indexData + (indexIdx * indexSize), (size_t)indexSize * 3);
                        longIndices[0] = indices[0] - indexOffset;
                        longIndices[1] = indices[1] - indexOffset;
                        longIndices[2] = indices[2] - indexOffset;
                    }

                    String output = "f ";
                    if (hasNormals)
                    {
                        output.AppendWithFormat("%u/%u/%u %u/%u/%u %u/%u/%u",
                            currentPositionIndex + longIndices[0],
                            currentUVIndex + longIndices[0],
                            currentNormalIndex + longIndices[0],
                            currentPositionIndex + longIndices[1],
                            currentUVIndex + longIndices[1],
                            currentNormalIndex + longIndices[1],
                            currentPositionIndex + longIndices[2],
                            currentUVIndex + longIndices[2],
                            currentNormalIndex + longIndices[2]);
                    }
                    else if (hasNormals || hasUV)
                    {
                        unsigned secondTraitIndex = hasNormals ? currentNormalIndex : currentUVIndex;
                        output.AppendWithFormat("%u%s%u %u%s%u %u%s%u",
                            currentPositionIndex + longIndices[0],
                            slashCharacter.c_str(),
                            secondTraitIndex + longIndices[0],
                            currentPositionIndex + longIndices[1],
                            slashCharacter.c_str(),
                            secondTraitIndex + longIndices[1],
                            currentPositionIndex + longIndices[2],
                            slashCharacter.c_str(),
                            secondTraitIndex + longIndices[2]);
                    }
                    else
                    {
                        output.AppendWithFormat("%u %u %u",
                            currentPositionIndex + longIndices[0],
                            currentPositionIndex + longIndices[1],
                            currentPositionIndex + longIndices[2]);
                    }
                    outputFile->WriteLine(output);
                }

                // Increment our positions based on what vertex attributes we have
                currentPositionIndex += vertexCount;
                currentNormalIndex += hasNormals ? vertexCount : 0;
                // is it possible to have TEXCOORD2 but not have TEXCOORD1, assume anything
                currentUVIndex += (hasUV || hasLMUV) ? vertexCount : 0;
            }
        }
    }
    return anythingWritten;
}

}
