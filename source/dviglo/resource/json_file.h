// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "resource.h"
#include "json_value.h"

namespace dviglo
{

/// JSON document resource.
class DV_API JSONFile : public Resource
{
    DV_OBJECT(JSONFile, Resource);

public:
    /// Construct.
    explicit JSONFile();
    /// Destruct.
    ~JSONFile() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;
    /// Save resource with default indentation (one tab). Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save resource with user-defined indentation, only the first character (if any) of the string is used and the length of the string defines the character count. Return true if successful.
    bool Save(Serializer& dest, const String& indendation) const;

    /// Deserialize from a string. Return true if successful.
    bool FromString(const String& source);
    /// Save to a string.
    String ToString(const String& indendation = "\t") const;

    /// Return root value.
    JSONValue& GetRoot() { return root_; }
    /// Return root value.
    const JSONValue& GetRoot() const { return root_; }

private:
    /// JSON root value.
    JSONValue root_;
};

}
