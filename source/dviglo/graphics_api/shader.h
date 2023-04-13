// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/array_ptr.h"
#include "graphics_defs.h"
#include "../resource/resource.h"

namespace dviglo
{

class ShaderVariation;

/// %Shader resource consisting of several shader variations.
class DV_API Shader : public Resource
{
    DV_OBJECT(Shader);

public:
    /// Construct.
    explicit Shader();
    /// Destruct.
    ~Shader() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    bool end_load() override;

    /// Return a variation with defines. Separate multiple defines with spaces.
    ShaderVariation* GetVariation(ShaderType type, const String& defines);
    /// Return a variation with defines. Separate multiple defines with spaces.
    ShaderVariation* GetVariation(ShaderType type, const char* defines);

    /// Return either vertex or pixel shader source code.
    const String& GetSourceCode() const { return source_code_; }

    /// Return the latest timestamp of the shader code and its includes.
    unsigned GetTimeStamp() const { return timeStamp_; }

private:
    /// Process source code and include files. Return true if successful.
    bool ProcessSource(String& code, Deserializer& source);
    /// Sort the defines and strip extra spaces to prevent creation of unnecessary duplicate shader variations.
    String NormalizeDefines(const String& defines);
    /// Recalculate the memory used by the shader.
    void RefreshMemoryUse();

    String source_code_;

    /// Vertex shader variations.
    HashMap<StringHash, SharedPtr<ShaderVariation>> vsVariations_;
    /// Pixel shader variations.
    HashMap<StringHash, SharedPtr<ShaderVariation>> psVariations_;
    /// Source code timestamp.
    unsigned timeStamp_;
    /// Number of unique variations so far.
    unsigned numVariations_;
};

}
