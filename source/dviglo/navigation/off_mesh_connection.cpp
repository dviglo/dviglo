// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/debug_renderer.h"
#include "off_mesh_connection.h"
#include "../scene/scene.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* NAVIGATION_CATEGORY;

static const float DEFAULT_RADIUS = 1.0f;
static const unsigned DEFAULT_MASK_FLAG = 1;
static const unsigned DEFAULT_AREA = 1;

OffMeshConnection::OffMeshConnection() :
    endPointID_(0),
    radius_(DEFAULT_RADIUS),
    bidirectional_(true),
    endPointDirty_(false),
    mask_(DEFAULT_MASK_FLAG),
    areaId_(DEFAULT_AREA)
{
}

OffMeshConnection::~OffMeshConnection() = default;

void OffMeshConnection::RegisterObject()
{
    DV_CONTEXT.RegisterFactory<OffMeshConnection>(NAVIGATION_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ATTRIBUTE_EX("Endpoint NodeID", endPointID_, MarkEndPointDirty, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE("Radius", radius_, DEFAULT_RADIUS, AM_DEFAULT);
    DV_ATTRIBUTE("Bidirectional", bidirectional_, true, AM_DEFAULT);
    DV_ATTRIBUTE("Flags Mask", mask_, DEFAULT_MASK_FLAG, AM_DEFAULT);
    DV_ATTRIBUTE("Area Type", areaId_, DEFAULT_AREA, AM_DEFAULT);
}

void OffMeshConnection::ApplyAttributes()
{
    if (endPointDirty_)
    {
        Scene* scene = GetScene();
        endPoint_ = scene ? scene->GetNode(endPointID_) : nullptr;
        endPointDirty_ = false;
    }
}

void OffMeshConnection::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!node_ || !endPoint_)
        return;

    Vector3 start = node_->GetWorldPosition();
    Vector3 end = endPoint_->GetWorldPosition();

    debug->AddSphere(Sphere(start, radius_), Color::WHITE, depthTest);
    debug->AddSphere(Sphere(end, radius_), Color::WHITE, depthTest);
    debug->AddLine(start, end, Color::WHITE, depthTest);
}

void OffMeshConnection::SetRadius(float radius)
{
    radius_ = radius;
    MarkNetworkUpdate();
}

void OffMeshConnection::SetBidirectional(bool enabled)
{
    bidirectional_ = enabled;
    MarkNetworkUpdate();
}

void OffMeshConnection::SetMask(unsigned newMask)
{
    mask_ = newMask;
    MarkNetworkUpdate();
}

void OffMeshConnection::SetAreaID(unsigned newAreaID)
{
    areaId_ = newAreaID;
    MarkNetworkUpdate();
}

void OffMeshConnection::SetEndPoint(Node* node)
{
    endPoint_ = node;
    endPointID_ = node ? node->GetID() : 0;
    MarkNetworkUpdate();
}

Node* OffMeshConnection::GetEndPoint() const
{
    return endPoint_;
}

}
