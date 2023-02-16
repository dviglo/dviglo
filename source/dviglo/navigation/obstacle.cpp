// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/debug_renderer.h"
#include "../io/log.h"
#include "dynamic_navigation_mesh.h"
#include "obstacle.h"
#include "navigation_events.h"
#include "../scene/scene.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* NAVIGATION_CATEGORY;

Obstacle::Obstacle(Context* context) :
    Component(context),
    height_(5.0f),
    radius_(5.0f),
    obstacleId_(0)
{
}

Obstacle::~Obstacle()
{
    if (obstacleId_ > 0 && ownerMesh_)
        ownerMesh_->RemoveObstacle(this);
}

void Obstacle::RegisterObject(Context* context)
{
    context->RegisterFactory<Obstacle>(NAVIGATION_CATEGORY);
    DV_COPY_BASE_ATTRIBUTES(Component);
    DV_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, 5.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Height", GetHeight, SetHeight, 5.0f, AM_DEFAULT);
}

void Obstacle::OnSetEnabled()
{
    if (ownerMesh_)
    {
        if (IsEnabledEffective())
            ownerMesh_->AddObstacle(this);
        else
            ownerMesh_->RemoveObstacle(this);
    }
}

void Obstacle::SetHeight(float newHeight)
{
    height_ = newHeight;
    if (ownerMesh_)
        ownerMesh_->ObstacleChanged(this);
    MarkNetworkUpdate();
}

void Obstacle::SetRadius(float newRadius)
{
    radius_ = newRadius;
    if (ownerMesh_)
        ownerMesh_->ObstacleChanged(this);
    MarkNetworkUpdate();
}

void Obstacle::OnNodeSet(Node* node)
{
    if (node)
        node->AddListener(this);
}

void Obstacle::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        if (scene == node_)
        {
            DV_LOGWARNING(GetTypeName() + " should not be created to the root scene node");
            return;
        }
        if (!ownerMesh_)
            ownerMesh_ = node_->GetParentComponent<DynamicNavigationMesh>(true);
        if (ownerMesh_)
        {
            ownerMesh_->AddObstacle(this);
            SubscribeToEvent(ownerMesh_, E_NAVIGATION_TILE_ADDED, DV_HANDLER(Obstacle, HandleNavigationTileAdded));
        }
    }
    else
    {
        if (obstacleId_ > 0 && ownerMesh_)
            ownerMesh_->RemoveObstacle(this);

        UnsubscribeFromEvent(E_NAVIGATION_TILE_ADDED);
        ownerMesh_.Reset();
    }
}

void Obstacle::OnMarkedDirty(Node* node)
{
    if (IsEnabledEffective() && ownerMesh_)
    {
        Scene* scene = GetScene();
        /// \hack If scene already unassigned, or if it's being destroyed, do nothing
        if (!scene || scene->Refs() == 0)
            return;

        // If within threaded update, update later
        if (scene->IsThreadedUpdate())
        {
            scene->DelayedMarkedDirty(this);
            return;
        }

        ownerMesh_->ObstacleChanged(this);
    }
}

void Obstacle::HandleNavigationTileAdded(StringHash eventType, VariantMap& eventData)
{
    // Re-add obstacle if it is intersected with newly added tile
    const IntVector2 tile = eventData[NavigationTileAdded::P_TILE].GetIntVector2();
    if (IsEnabledEffective() && ownerMesh_ && ownerMesh_->IsObstacleInTile(this, tile))
        ownerMesh_->ObstacleChanged(this);
}

void Obstacle::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
        debug->AddCylinder(node_->GetWorldPosition(), radius_, height_, Color(0.0f, 1.0f, 1.0f), depthTest);
}

void Obstacle::DrawDebugGeometry(bool depthTest)
{
    Scene* scene = GetScene();
    if (scene)
        DrawDebugGeometry(scene->GetComponent<DebugRenderer>(), depthTest);
}

}
