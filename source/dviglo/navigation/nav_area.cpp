// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/debug_renderer.h"
#include "../io/log.h"
#include "nav_area.h"
#include "../scene/node.h"

namespace dviglo
{

static const unsigned MAX_NAV_AREA_ID = 255;
static const Vector3 DEFAULT_BOUNDING_BOX_MIN(-10.0f, -10.0f, -10.0f);
static const Vector3 DEFAULT_BOUNDING_BOX_MAX(10.0f, 10.0f, 10.0f);
static const unsigned DEFAULT_AREA_ID = 0;

extern const char* NAVIGATION_CATEGORY;

NavArea::NavArea() :
    areaID_(DEFAULT_AREA_ID),
    boundingBox_(DEFAULT_BOUNDING_BOX_MIN, DEFAULT_BOUNDING_BOX_MAX)
{
}

NavArea::~NavArea() = default;

void NavArea::register_object()
{
    DV_CONTEXT->RegisterFactory<NavArea>(NAVIGATION_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(Component);
    DV_ATTRIBUTE("Bounding Box Min", boundingBox_.min_, DEFAULT_BOUNDING_BOX_MIN, AM_DEFAULT);
    DV_ATTRIBUTE("Bounding Box Max", boundingBox_.max_, DEFAULT_BOUNDING_BOX_MAX, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Area ID", GetAreaID, SetAreaID, DEFAULT_AREA_ID, AM_DEFAULT);
}

void NavArea::SetAreaID(unsigned newID)
{
    if (newID > MAX_NAV_AREA_ID)
        DV_LOGERRORF("NavArea Area ID %u exceeds maximum value of %u", newID, MAX_NAV_AREA_ID);
    areaID_ = (unsigned char)newID;
    MarkNetworkUpdate();
}

BoundingBox NavArea::GetWorldBoundingBox() const
{
    Matrix3x4 mat;
    mat.SetTranslation(node_->GetWorldPosition());
    return boundingBox_.Transformed(mat);
}

void NavArea::draw_debug_geometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        Matrix3x4 mat;
        mat.SetTranslation(node_->GetWorldPosition());
        debug->AddBoundingBox(boundingBox_, mat, Color::GREEN, depthTest);
        debug->AddBoundingBox(boundingBox_, mat, Color(0.0f, 1.0f, 0.0f, 0.15f), true, true);
    }
}

}
