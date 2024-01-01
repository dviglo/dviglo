// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/graphics.h"
#include "shader.h"
#include "shader_variation.h"
#include "../io/deserializer.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

Shader::Shader() :
    timeStamp_(0),
    numVariations_(0)
{
    RefreshMemoryUse();
}

Shader::~Shader()
{
    // Не обращаемся к ResourceCache при завершении приложения
    if (DV_RES_CACHE)
        DV_RES_CACHE->reset_dependencies(this);
}

void Shader::register_object()
{
    DV_CONTEXT->RegisterFactory<Shader>();
}

bool Shader::begin_load(Deserializer& source)
{
    if (GParams::is_headless())
        return false;

    // Load the shader source code and resolve any includes
    timeStamp_ = 0;
    String shaderCode;
    if (!ProcessSource(shaderCode, source))
        return false;

    source_code_ = shaderCode;

    RefreshMemoryUse();
    return true;
}

bool Shader::end_load()
{
    // If variations had already been created, release them and require recompile
    for (HashMap<StringHash, SharedPtr<ShaderVariation>>::Iterator i = vsVariations_.Begin(); i != vsVariations_.End(); ++i)
        i->second_->Release();
    for (HashMap<StringHash, SharedPtr<ShaderVariation>>::Iterator i = psVariations_.Begin(); i != psVariations_.End(); ++i)
        i->second_->Release();

    return true;
}

ShaderVariation* Shader::GetVariation(ShaderType type, const String& defines)
{
    return GetVariation(type, defines.c_str());
}

ShaderVariation* Shader::GetVariation(ShaderType type, const char* defines)
{
    StringHash definesHash(defines);
    HashMap<StringHash, SharedPtr<ShaderVariation>>& variations(type == VS ? vsVariations_ : psVariations_);
    HashMap<StringHash, SharedPtr<ShaderVariation>>::Iterator i = variations.Find(definesHash);
    if (i == variations.End())
    {
        // If shader not found, normalize the defines (to prevent duplicates) and check again. In that case make an alias
        // so that further queries are faster
        String normalizedDefines = NormalizeDefines(defines);
        StringHash normalizedHash(normalizedDefines);

        i = variations.Find(normalizedHash);
        if (i != variations.End())
            variations.Insert(MakePair(definesHash, i->second_));
        else
        {
            // No shader variation found. Create new
            i = variations.Insert(MakePair(normalizedHash, SharedPtr<ShaderVariation>(new ShaderVariation(this, type))));
            if (definesHash != normalizedHash)
                variations.Insert(MakePair(definesHash, i->second_));

            i->second_->SetName(GetFileName(GetName()));
            i->second_->SetDefines(normalizedDefines);
            ++numVariations_;
            RefreshMemoryUse();
        }
    }

    return i->second_;
}

bool Shader::ProcessSource(String& code, Deserializer& source)
{
    ResourceCache* cache = DV_RES_CACHE;

    // If the source if a non-packaged file, store the timestamp
    auto* file = dynamic_cast<File*>(&source);
    if (file && !file->IsPackaged())
    {
        String fullName = cache->GetResourceFileName(file->GetName());
        unsigned fileTimeStamp = DV_FILE_SYSTEM->GetLastModifiedTime(fullName);
        if (fileTimeStamp > timeStamp_)
            timeStamp_ = fileTimeStamp;
    }

    // Store resource dependencies for includes so that we know to reload if any of them changes
    if (source.GetName() != GetName())
        cache->store_resource_dependency(this, source.GetName());

    while (!source.IsEof())
    {
        String line = source.ReadLine();

        if (line.StartsWith("#dv_include"))
        {
            String includeFileName = GetPath(source.GetName()) + line.Substring(12).Replaced("\"", "").Trimmed();

            shared_ptr<File> includeFile = cache->GetFile(includeFileName);
            if (!includeFile)
                return false;

            // Add the include file into the current code recursively
            if (!ProcessSource(code, *includeFile))
                return false;
        }
        else
        {
            code += line;
            code += "\n";
        }
    }

    // Finally insert an empty line to mark the space between files
    code += "\n";

    return true;
}

String Shader::NormalizeDefines(const String& defines)
{
    Vector<String> definesVec = defines.ToUpper().Split(' ');
    sort(definesVec.Begin(), definesVec.End());
    return String::Joined(definesVec, " ");
}

void Shader::RefreshMemoryUse()
{
    SetMemoryUse(
        (unsigned)(sizeof(Shader) + source_code_.Length() + numVariations_ * sizeof(ShaderVariation)));
}

}
