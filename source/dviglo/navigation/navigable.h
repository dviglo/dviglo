// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

namespace dviglo
{

/// Component which tags geometry for inclusion in the navigation mesh. Optionally auto-includes geometry from child nodes.
class DV_API Navigable : public Component
{
    DV_OBJECT(Navigable, Component);

public:
    /// Construct.
    explicit Navigable();
    /// Destruct.
    ~Navigable() override;
    /// Register object factory.
    static void RegisterObject();

    /// Set whether geometry is automatically collected from child nodes. Default true.
    void SetRecursive(bool enable);

    /// Return whether geometry is automatically collected from child nodes.
    bool IsRecursive() const { return recursive_; }

private:
    /// Recursive flag.
    bool recursive_;
};

}
