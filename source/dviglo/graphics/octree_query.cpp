// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../graphics/octree_query.h"

#include "../debug_new.h"

namespace dviglo
{

Intersection PointOctreeQuery::TestOctant(const BoundingBox& box, bool inside)
{
    if (inside)
        return INSIDE;
    else
        return box.IsInside(point_);
}

void PointOctreeQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    while (start != end)
    {
        Drawable* drawable = *start++;

        if (!!(drawable->GetDrawableType() & drawableTypes_) && (drawable->GetViewMask() & viewMask_))
        {
            if (inside || drawable->GetWorldBoundingBox().IsInside(point_))
                result_.Push(drawable);
        }
    }
}

Intersection SphereOctreeQuery::TestOctant(const BoundingBox& box, bool inside)
{
    if (inside)
        return INSIDE;
    else
        return sphere_.IsInside(box);
}

void SphereOctreeQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    while (start != end)
    {
        Drawable* drawable = *start++;

        if (!!(drawable->GetDrawableType() & drawableTypes_) && (drawable->GetViewMask() & viewMask_))
        {
            if (inside || sphere_.IsInsideFast(drawable->GetWorldBoundingBox()))
                result_.Push(drawable);
        }
    }
}

Intersection BoxOctreeQuery::TestOctant(const BoundingBox& box, bool inside)
{
    if (inside)
        return INSIDE;
    else
        return box_.IsInside(box);
}

void BoxOctreeQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    while (start != end)
    {
        Drawable* drawable = *start++;

        if (!!(drawable->GetDrawableType() & drawableTypes_) && (drawable->GetViewMask() & viewMask_))
        {
            if (inside || box_.IsInsideFast(drawable->GetWorldBoundingBox()))
                result_.Push(drawable);
        }
    }
}

Intersection FrustumOctreeQuery::TestOctant(const BoundingBox& box, bool inside)
{
    if (inside)
        return INSIDE;
    else
        return frustum_.IsInside(box);
}

void FrustumOctreeQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    while (start != end)
    {
        Drawable* drawable = *start++;

        if (!!(drawable->GetDrawableType() & drawableTypes_) && (drawable->GetViewMask() & viewMask_))
        {
            if (inside || frustum_.IsInsideFast(drawable->GetWorldBoundingBox()))
                result_.Push(drawable);
        }
    }
}


Intersection AllContentOctreeQuery::TestOctant(const BoundingBox& box, bool inside)
{
    return INSIDE;
}

void AllContentOctreeQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    while (start != end)
    {
        Drawable* drawable = *start++;

        if (!!(drawable->GetDrawableType() & drawableTypes_) && (drawable->GetViewMask() & viewMask_))
            result_.Push(drawable);
    }
}

}
