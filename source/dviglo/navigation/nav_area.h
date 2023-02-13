// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../math/bounding_box.h"
#include "../scene/component.h"

namespace dviglo
{

class URHO3D_API NavArea : public Component
{
    URHO3D_OBJECT(NavArea, Component);

public:
    /// Construct.
    explicit NavArea(Context* context);
    /// Destruct.
    ~NavArea() override;
    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Render debug geometry for the bounds.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Get the area id for this volume.
    unsigned GetAreaID() const { return (unsigned)areaID_; }

    /// Set the area id for this volume.
    void SetAreaID(unsigned newID);

    /// Get the bounding box of this navigation area, in local space.
    BoundingBox GetBoundingBox() const { return boundingBox_; }

    /// Set the bounding box of this area, in local space.
    void SetBoundingBox(const BoundingBox& bnds) { boundingBox_ = bnds; }

    /// Get the bounds of this navigation area in world space.
    BoundingBox GetWorldBoundingBox() const;

private:
    /// Bounds of area to mark.
    BoundingBox boundingBox_;
    /// Area id to assign to the marked area.
    unsigned char areaID_;
};

}
