// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../io/serializer.h"
#include "../io/deserializer.h"

namespace Urho3D
{

/// A common root class for objects that implement both Serializer and Deserializer.
class URHO3D_API AbstractFile : public Deserializer, public Serializer
{
public:
    /// Construct.
    AbstractFile() : Deserializer() { }
    /// Construct.
    explicit AbstractFile(i64 size) : Deserializer(size) { }
    /// Destruct.
    ~AbstractFile() override = default;
    /// Change the file name. Used by the resource system.
    /// @property
    virtual void SetName(const String& name) { name_ = name; }
    /// Return the file name.
    const String& GetName() const override { return name_; }

protected:
    /// File name.
    String name_;
};

}
