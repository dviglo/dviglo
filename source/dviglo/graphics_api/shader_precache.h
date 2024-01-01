// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../containers/hash_set.h"
#include "../core/object.h"
#include "../resource/xml_file.h"

namespace dviglo
{

class Graphics;
class ShaderVariation;

/// Utility class for collecting used shader combinations during runtime for precaching.
class DV_API ShaderPrecache : public Object
{
    DV_OBJECT(ShaderPrecache);

public:
    /// Construct and begin collecting shader combinations. Load existing combinations from XML if the file exists.
    ShaderPrecache(const String& fileName);
    /// Destruct. Write the collected shaders to XML.
    ~ShaderPrecache() override;

    /// Collect a shader combination. Called by Graphics when shaders have been set.
    void store_shaders(ShaderVariation* vs, ShaderVariation* ps);

    /// Load shaders from an XML file.
    static void load_shaders(Graphics* graphics, Deserializer& source);

private:
    /// XML file name.
    String fileName_;
    /// XML file.
    XmlFile xmlFile_;
    /// Already encountered shader combinations, pointer version for fast queries.
    HashSet<Pair<ShaderVariation*, ShaderVariation*>> usedPtrCombinations_;
    /// Already encountered shader combinations.
    HashSet<String> usedCombinations_;
};

}
