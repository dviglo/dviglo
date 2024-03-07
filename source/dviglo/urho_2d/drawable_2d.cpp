// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/camera.h"
#include "../graphics/material.h"
#include "../graphics_api/texture_2d.h"
#include "../scene/scene.h"
#include "drawable_2d.h"
#include "renderer_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

SourceBatch2D::SourceBatch2D() :
    distance_(0.0f),
    drawOrder_(0)
{
}

Drawable2d::Drawable2d() :
    Drawable(DrawableTypes::Geometry2D),
    layer_(0),
    orderInLayer_(0),
    sourceBatchesDirty_(true)
{
}

Drawable2d::~Drawable2d()
{
    if (renderer_)
        renderer_->RemoveDrawable(this);
}

void Drawable2d::register_object()
{
    DV_ACCESSOR_ATTRIBUTE("Layer", GetLayer, SetLayer, 0, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Order in Layer", GetOrderInLayer, SetOrderInLayer, 0, AM_DEFAULT);
    DV_ATTRIBUTE("View Mask", viewMask_, DEFAULT_VIEWMASK, AM_DEFAULT);
}

void Drawable2d::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled && renderer_)
        renderer_->AddDrawable(this);
    else if (!enabled && renderer_)
        renderer_->RemoveDrawable(this);
}

void Drawable2d::SetLayer(int layer)
{
    if (layer == layer_)
        return;

    layer_ = layer;

    OnDrawOrderChanged();
    MarkNetworkUpdate();
}

void Drawable2d::SetOrderInLayer(int orderInLayer)
{
    if (orderInLayer == orderInLayer_)
        return;

    orderInLayer_ = orderInLayer;

    OnDrawOrderChanged();
    MarkNetworkUpdate();
}

const Vector<SourceBatch2D>& Drawable2d::GetSourceBatches()
{
    if (sourceBatchesDirty_)
        UpdateSourceBatches();

    return sourceBatches_;
}

void Drawable2d::OnSceneSet(Scene* scene)
{
    // Do not call Drawable::OnSceneSet(node), as 2D drawable components should not be added to the octree
    // but are instead rendered through Renderer2D
    if (scene)
    {
        renderer_ = scene->GetOrCreateComponent<Renderer2D>();

        if (IsEnabledEffective())
            renderer_->AddDrawable(this);
    }
    else
    {
        if (renderer_)
            renderer_->RemoveDrawable(this);
    }
}

void Drawable2d::OnMarkedDirty(Node* node)
{
    Drawable::OnMarkedDirty(node);

    sourceBatchesDirty_ = true;
}

}
