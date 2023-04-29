// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "batch.h"
#include "camera.h"
#include "skybox.h"
#include "../scene/node.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* GEOMETRY_CATEGORY;

Skybox::Skybox() :
    lastFrame_(0)
{
}

Skybox::~Skybox() = default;

void Skybox::register_object()
{
    DV_CONTEXT->RegisterFactory<Skybox>(GEOMETRY_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(StaticModel);
}

void Skybox::ProcessRayQuery(const RayOctreeQuery& query, Vector<RayQueryResult>& results)
{
    // Do not record a raycast result for a skybox, as it would block all other results
}

void Skybox::update_batches(const FrameInfo& frame)
{
    distance_ = 0.0f;

    if (frame.frameNumber_ != lastFrame_)
    {
        customWorldTransforms_.Clear();
        lastFrame_ = frame.frameNumber_;
    }

    // Add camera position to fix the skybox in space. Use effective world transform to take reflection into account
    Matrix3x4 customWorldTransform = node_->GetWorldTransform();
    customWorldTransform.SetTranslation(node_->GetWorldPosition() + frame.camera_->GetEffectiveWorldTransform().Translation());
    HashMap<Camera*, Matrix3x4>::Iterator it = customWorldTransforms_.Insert(MakePair(frame.camera_, customWorldTransform));

    for (SourceBatch& batch : batches_)
    {
        batch.worldTransform_ = &it->second_;
        batch.distance_ = 0.0f;
    }
}

void Skybox::OnWorldBoundingBoxUpdate()
{
    // The skybox is supposed to be visible everywhere, so set a humongous bounding box
    worldBoundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
}

}
