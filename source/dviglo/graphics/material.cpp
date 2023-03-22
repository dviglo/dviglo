// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "material.h"

#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/profiler.h"
#include "../core/thread.h"
#include "../graphics_api/texture_2d.h"
#include "../graphics_api/texture_2d_array.h"
#include "../graphics_api/texture_3d.h"
#include "../graphics_api/texture_cube.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../io/vector_buffer.h"
#include "../resource/json_file.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"
#include "../scene/scene.h"
#include "../scene/scene_events.h"
#include "../scene/value_animation.h"
#include "graphics.h"
#include "renderer.h"
#include "technique.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* wrapModeNames[];

static const char* textureUnitNames[] =
{
    "diffuse",
    "normal",
    "specular",
    "emissive",
    "environment",
#ifdef DESKTOP_GRAPHICS_OR_GLES3
    "volume",
    "custom1",
    "custom2",
    "lightramp",
    "lightshape",
    "shadowmap",
    "faceselect",
    "indirection",
    "depth",
    "light",
    "zone",
    nullptr
#else
    "lightramp",
    "lightshape",
    "shadowmap",
    nullptr
#endif
};

const char* cullModeNames[] =
{
    "none",
    "ccw",
    "cw",
    nullptr
};

static const char* fillModeNames[] =
{
    "solid",
    "wireframe",
    "point",
    nullptr
};

TextureUnit ParseTextureUnitName(String name)
{
    name = name.ToLower().Trimmed();

    TextureUnit unit = (TextureUnit)GetStringListIndex(name.c_str(), textureUnitNames, MAX_TEXTURE_UNITS);
    if (unit == MAX_TEXTURE_UNITS)
    {
        // Check also for shorthand names
        if (name == "diff")
            unit = TU_DIFFUSE;
        else if (name == "albedo")
            unit = TU_DIFFUSE;
        else if (name == "norm")
            unit = TU_NORMAL;
        else if (name == "spec")
            unit = TU_SPECULAR;
        else if (name == "env")
            unit = TU_ENVIRONMENT;
        // Finally check for specifying the texture unit directly as a number
        else if (name.Length() < 3)
            unit = (TextureUnit)Clamp(ToI32(name), 0, MAX_TEXTURE_UNITS - 1);
    }

    if (unit == MAX_TEXTURE_UNITS)
        DV_LOGERROR("Unknown texture unit name " + name);

    return unit;
}

StringHash ParseTextureTypeName(const String& name)
{
    String lowerCaseName = name.ToLower().Trimmed();

    if (lowerCaseName == "texture")
        return Texture2D::GetTypeStatic();
    else if (lowerCaseName == "cubemap")
        return TextureCube::GetTypeStatic();
    else if (lowerCaseName == "texture3d")
        return Texture3D::GetTypeStatic();
    else if (lowerCaseName == "texturearray")
        return Texture2DArray::GetTypeStatic();

    return nullptr;
}

StringHash ParseTextureTypeXml(const String& filename)
{
    StringHash type = nullptr;

    SharedPtr<File> texXmlFile = DV_RES_CACHE.GetFile(filename, false);
    if (texXmlFile.NotNull())
    {
        SharedPtr<XmlFile> texXml(new XmlFile());
        if (texXml->Load(*texXmlFile))
            type = ParseTextureTypeName(texXml->GetRoot().GetName());
    }
    return type;
}

static TechniqueEntry noEntry;

bool CompareTechniqueEntries(const TechniqueEntry& lhs, const TechniqueEntry& rhs)
{
    if (lhs.lodDistance_ != rhs.lodDistance_)
        return lhs.lodDistance_ > rhs.lodDistance_;
    else
        return lhs.qualityLevel_ > rhs.qualityLevel_;
}

TechniqueEntry::TechniqueEntry() noexcept :
    qualityLevel_(QUALITY_LOW),
    lodDistance_(0.0f)
{
}

TechniqueEntry::TechniqueEntry(Technique* tech, MaterialQuality qualityLevel, float lodDistance) noexcept :
    technique_(tech),
    original_(tech),
    qualityLevel_(qualityLevel),
    lodDistance_(lodDistance)
{
}

ShaderParameterAnimationInfo::ShaderParameterAnimationInfo(Material* material, const String& name, ValueAnimation* attributeAnimation,
    WrapMode wrapMode, float speed) :
    ValueAnimationInfo(material, attributeAnimation, wrapMode, speed),
    name_(name)
{
}

ShaderParameterAnimationInfo::ShaderParameterAnimationInfo(const ShaderParameterAnimationInfo& other) = default;

ShaderParameterAnimationInfo::~ShaderParameterAnimationInfo() = default;

void ShaderParameterAnimationInfo::ApplyValue(const Variant& newValue)
{
    static_cast<Material*>(target_.Get())->SetShaderParameter(name_, newValue);
}

Material::Material()
{
    ResetToDefaults();
}

Material::~Material() = default;

void Material::RegisterObject()
{
    DV_CONTEXT.RegisterFactory<Material>();
}

bool Material::BeginLoad(Deserializer& source)
{
    // In headless mode, do not actually load the material, just return success
    if (GParams::is_headless())
        return true;

    String extension = GetExtension(source.GetName());

    bool success = false;
    if (extension == ".xml")
    {
        success = begin_load_xml(source);
        if (!success)
            success = begin_load_json(source);

        if (success)
            return true;
    }
    else // Load JSON file
    {
        success = begin_load_json(source);
        if (!success)
            success = begin_load_xml(source);

        if (success)
            return true;
    }

    // All loading failed
    ResetToDefaults();
    loadJSONFile_.Reset();
    return false;
}

bool Material::EndLoad()
{
    // In headless mode, do not actually load the material, just return success
    if (GParams::is_headless())
        return true;

    bool success = false;
    if (loadXMLFile_)
    {
        // If async loading, get the techniques / textures which should be ready now
        XmlElement rootElem = loadXMLFile_->GetRoot();
        success = Load(rootElem);
    }

    if (loadJSONFile_)
    {
        JSONValue rootVal = loadJSONFile_->GetRoot();
        success = Load(rootVal);
    }

    loadXMLFile_.Reset();
    loadJSONFile_.Reset();
    return success;
}

bool Material::begin_load_xml(Deserializer& source)
{
    ResetToDefaults();
    loadXMLFile_ = new XmlFile();
    if (loadXMLFile_->Load(source))
    {
        // If async loading, scan the XML content beforehand for technique & texture resources
        // and request them to also be loaded. Can not do anything else at this point
        if (GetAsyncLoadState() == ASYNC_LOADING)
        {
            ResourceCache& cache = DV_RES_CACHE;
            XmlElement rootElem = loadXMLFile_->GetRoot();
            XmlElement techniqueElem = rootElem.GetChild("technique");
            while (techniqueElem)
            {
                cache.BackgroundLoadResource<Technique>(techniqueElem.GetAttribute("name"), true, this);
                techniqueElem = techniqueElem.GetNext("technique");
            }

            XmlElement textureElem = rootElem.GetChild("texture");
            while (textureElem)
            {
                String name = textureElem.GetAttribute("name");
                // Detect cube maps and arrays by file extension: they are defined by an XML file
                if (GetExtension(name) == ".xml")
                {
#ifdef DESKTOP_GRAPHICS_OR_GLES3
                    StringHash type = ParseTextureTypeXml(name);
                    if (!type && textureElem.HasAttribute("unit"))
                    {
                        TextureUnit unit = ParseTextureUnitName(textureElem.GetAttribute("unit"));
                        if (unit == TU_VOLUMEMAP)
                            type = Texture3D::GetTypeStatic();
                    }

                    if (type == Texture3D::GetTypeStatic())
                        cache.BackgroundLoadResource<Texture3D>(name, true, this);
                    else if (type == Texture2DArray::GetTypeStatic())
                        cache.BackgroundLoadResource<Texture2DArray>(name, true, this);
                    else
#endif
                        cache.BackgroundLoadResource<TextureCube>(name, true, this);
                }
                else
                    cache.BackgroundLoadResource<Texture2D>(name, true, this);
                textureElem = textureElem.GetNext("texture");
            }
        }

        return true;
    }

    return false;
}

bool Material::begin_load_json(Deserializer& source)
{
    // Attempt to load a JSON file
    ResetToDefaults();
    loadXMLFile_.Reset();

    // Attempt to load from JSON file instead
    loadJSONFile_ = new JSONFile();
    if (loadJSONFile_->Load(source))
    {
        // If async loading, scan the XML content beforehand for technique & texture resources
        // and request them to also be loaded. Can not do anything else at this point
        if (GetAsyncLoadState() == ASYNC_LOADING)
        {
            ResourceCache& cache = DV_RES_CACHE;
            const JSONValue& rootVal = loadJSONFile_->GetRoot();

            JSONArray techniqueArray = rootVal.Get("techniques").GetArray();

            for (const JSONValue& techVal : techniqueArray)
                cache.BackgroundLoadResource<Technique>(techVal.Get("name").GetString(), true, this);

            JSONObject textureObject = rootVal.Get("textures").GetObject();
            for (JSONObject::ConstIterator it = textureObject.Begin(); it != textureObject.End(); it++)
            {
                String unitString = it->first_;
                String name = it->second_.GetString();
                // Detect cube maps and arrays by file extension: they are defined by an XML file
                if (GetExtension(name) == ".xml")
                {
#ifdef DESKTOP_GRAPHICS_OR_GLES3
                    StringHash type = ParseTextureTypeXml(name);
                    if (!type && !unitString.Empty())
                    {
                        TextureUnit unit = ParseTextureUnitName(unitString);
                        if (unit == TU_VOLUMEMAP)
                            type = Texture3D::GetTypeStatic();
                    }

                    if (type == Texture3D::GetTypeStatic())
                        cache.BackgroundLoadResource<Texture3D>(name, true, this);
                    else if (type == Texture2DArray::GetTypeStatic())
                        cache.BackgroundLoadResource<Texture2DArray>(name, true, this);
                    else
#endif
                        cache.BackgroundLoadResource<TextureCube>(name, true, this);
                }
                else
                    cache.BackgroundLoadResource<Texture2D>(name, true, this);
            }
        }

        // JSON material was successfully loaded
        return true;
    }

    return false;
}

bool Material::Save(Serializer& dest) const
{
    SharedPtr<XmlFile> xml(new XmlFile());
    XmlElement materialElem = xml->CreateRoot("material");

    Save(materialElem);
    return xml->Save(dest);
}

bool Material::Load(const XmlElement& source)
{
    ResetToDefaults();

    if (source.IsNull())
    {
        DV_LOGERROR("Can not load material from null XML element");
        return false;
    }

    ResourceCache& cache = DV_RES_CACHE;

    XmlElement shaderElem = source.GetChild("shader");
    if (shaderElem)
    {
        vertexShaderDefines_ = shaderElem.GetAttribute("vsdefines");
        pixelShaderDefines_ = shaderElem.GetAttribute("psdefines");
    }

    XmlElement techniqueElem = source.GetChild("technique");
    techniques_.Clear();

    while (techniqueElem)
    {
        auto* tech = cache.GetResource<Technique>(techniqueElem.GetAttribute("name"));
        if (tech)
        {
            TechniqueEntry newTechnique;
            newTechnique.technique_ = newTechnique.original_ = tech;
            if (techniqueElem.HasAttribute("quality"))
                newTechnique.qualityLevel_ = (MaterialQuality)techniqueElem.GetI32("quality");
            if (techniqueElem.HasAttribute("loddistance"))
                newTechnique.lodDistance_ = techniqueElem.GetFloat("loddistance");
            techniques_.Push(newTechnique);
        }

        techniqueElem = techniqueElem.GetNext("technique");
    }

    SortTechniques();
    ApplyShaderDefines();

    XmlElement textureElem = source.GetChild("texture");
    while (textureElem)
    {
        TextureUnit unit = TU_DIFFUSE;
        if (textureElem.HasAttribute("unit"))
            unit = ParseTextureUnitName(textureElem.GetAttribute("unit"));
        if (unit < MAX_TEXTURE_UNITS)
        {
            String name = textureElem.GetAttribute("name");
            // Detect cube maps and arrays by file extension: they are defined by an XML file
            if (GetExtension(name) == ".xml")
            {
#ifdef DESKTOP_GRAPHICS_OR_GLES3
                StringHash type = ParseTextureTypeXml(name);
                if (!type && unit == TU_VOLUMEMAP)
                    type = Texture3D::GetTypeStatic();

                if (type == Texture3D::GetTypeStatic())
                    SetTexture(unit, cache.GetResource<Texture3D>(name));
                else if (type == Texture2DArray::GetTypeStatic())
                    SetTexture(unit, cache.GetResource<Texture2DArray>(name));
                else
#endif
                    SetTexture(unit, cache.GetResource<TextureCube>(name));
            }
            else
                SetTexture(unit, cache.GetResource<Texture2D>(name));
        }
        textureElem = textureElem.GetNext("texture");
    }

    batchedParameterUpdate_ = true;
    XmlElement parameterElem = source.GetChild("parameter");
    while (parameterElem)
    {
        String name = parameterElem.GetAttribute("name");
        if (!parameterElem.HasAttribute("type"))
            SetShaderParameter(name, ParseShaderParameterValue(parameterElem.GetAttribute("value")));
        else
            SetShaderParameter(name, Variant(parameterElem.GetAttribute("type"), parameterElem.GetAttribute("value")));
        parameterElem = parameterElem.GetNext("parameter");
    }
    batchedParameterUpdate_ = false;

    XmlElement parameterAnimationElem = source.GetChild("parameteranimation");
    while (parameterAnimationElem)
    {
        String name = parameterAnimationElem.GetAttribute("name");
        SharedPtr<ValueAnimation> animation(new ValueAnimation());
        if (!animation->load_xml(parameterAnimationElem))
        {
            DV_LOGERROR("Could not load parameter animation");
            return false;
        }

        String wrapModeString = parameterAnimationElem.GetAttribute("wrapmode");
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = parameterAnimationElem.GetFloat("speed");
        SetShaderParameterAnimation(name, animation, wrapMode, speed);

        parameterAnimationElem = parameterAnimationElem.GetNext("parameteranimation");
    }

    XmlElement cullElem = source.GetChild("cull");
    if (cullElem)
        SetCullMode((CullMode)GetStringListIndex(cullElem.GetAttribute("value").c_str(), cullModeNames, CULL_CCW));

    XmlElement shadowCullElem = source.GetChild("shadowcull");
    if (shadowCullElem)
        SetShadowCullMode((CullMode)GetStringListIndex(shadowCullElem.GetAttribute("value").c_str(), cullModeNames, CULL_CCW));

    XmlElement fillElem = source.GetChild("fill");
    if (fillElem)
        SetFillMode((FillMode)GetStringListIndex(fillElem.GetAttribute("value").c_str(), fillModeNames, FILL_SOLID));

    XmlElement depthBiasElem = source.GetChild("depthbias");
    if (depthBiasElem)
        SetDepthBias(BiasParameters(depthBiasElem.GetFloat("constant"), depthBiasElem.GetFloat("slopescaled")));

    XmlElement alphaToCoverageElem = source.GetChild("alphatocoverage");
    if (alphaToCoverageElem)
        SetAlphaToCoverage(alphaToCoverageElem.GetBool("enable"));

    XmlElement lineAntiAliasElem = source.GetChild("lineantialias");
    if (lineAntiAliasElem)
        SetLineAntiAlias(lineAntiAliasElem.GetBool("enable"));

    XmlElement renderOrderElem = source.GetChild("renderorder");
    if (renderOrderElem)
        SetRenderOrder((i8)renderOrderElem.GetI32("value"));

    XmlElement occlusionElem = source.GetChild("occlusion");
    if (occlusionElem)
        SetOcclusion(occlusionElem.GetBool("enable"));

    RefreshShaderParameterHash();
    RefreshMemoryUse();
    return true;
}

bool Material::Load(const JSONValue& source)
{
    ResetToDefaults();

    if (source.IsNull())
    {
        DV_LOGERROR("Can not load material from null JSON element");
        return false;
    }

    ResourceCache& cache = DV_RES_CACHE;

    const JSONValue& shaderVal = source.Get("shader");
    if (!shaderVal.IsNull())
    {
        vertexShaderDefines_ = shaderVal.Get("vsdefines").GetString();
        pixelShaderDefines_ = shaderVal.Get("psdefines").GetString();
    }

    // Load techniques
    JSONArray techniquesArray = source.Get("techniques").GetArray();
    techniques_.Clear();
    techniques_.Reserve(techniquesArray.Size());

    for (const JSONValue& techVal : techniquesArray)
    {
        Technique* tech = cache.GetResource<Technique>(techVal.Get("name").GetString());
        if (tech)
        {
            TechniqueEntry newTechnique;
            newTechnique.technique_ = newTechnique.original_ = tech;
            JSONValue qualityVal = techVal.Get("quality");
            if (!qualityVal.IsNull())
                newTechnique.qualityLevel_ = (MaterialQuality)qualityVal.GetI32();
            JSONValue lodDistanceVal = techVal.Get("loddistance");
            if (!lodDistanceVal.IsNull())
                newTechnique.lodDistance_ = lodDistanceVal.GetFloat();
            techniques_.Push(newTechnique);
        }
    }

    SortTechniques();
    ApplyShaderDefines();

    // Load textures
    JSONObject textureObject = source.Get("textures").GetObject();
    for (JSONObject::ConstIterator it = textureObject.Begin(); it != textureObject.End(); it++)
    {
        String textureUnit = it->first_;
        String textureName = it->second_.GetString();

        TextureUnit unit = TU_DIFFUSE;
        unit = ParseTextureUnitName(textureUnit);

        if (unit < MAX_TEXTURE_UNITS)
        {
            // Detect cube maps and arrays by file extension: they are defined by an XML file
            if (GetExtension(textureName) == ".xml")
            {
#ifdef DESKTOP_GRAPHICS_OR_GLES3
                StringHash type = ParseTextureTypeXml(textureName);
                if (!type && unit == TU_VOLUMEMAP)
                    type = Texture3D::GetTypeStatic();

                if (type == Texture3D::GetTypeStatic())
                    SetTexture(unit, cache.GetResource<Texture3D>(textureName));
                else if (type == Texture2DArray::GetTypeStatic())
                    SetTexture(unit, cache.GetResource<Texture2DArray>(textureName));
                else
#endif
                    SetTexture(unit, cache.GetResource<TextureCube>(textureName));
            }
            else
                SetTexture(unit, cache.GetResource<Texture2D>(textureName));
        }
    }

    // Get shader parameters
    batchedParameterUpdate_ = true;
    JSONObject parameterObject = source.Get("shaderParameters").GetObject();

    for (JSONObject::ConstIterator it = parameterObject.Begin(); it != parameterObject.End(); it++)
    {
        String name = it->first_;
        if (it->second_.IsString())
            SetShaderParameter(name, ParseShaderParameterValue(it->second_.GetString()));
        else if (it->second_.IsObject())
        {
            JSONObject valueObj = it->second_.GetObject();
            SetShaderParameter(name, Variant(valueObj["type"].GetString(), valueObj["value"].GetString()));
        }
    }
    batchedParameterUpdate_ = false;

    // Load shader parameter animations
    JSONObject paramAnimationsObject = source.Get("shaderParameterAnimations").GetObject();
    for (JSONObject::ConstIterator it = paramAnimationsObject.Begin(); it != paramAnimationsObject.End(); it++)
    {
        String name = it->first_;
        JSONValue paramAnimVal = it->second_;

        SharedPtr<ValueAnimation> animation(new ValueAnimation());
        if (!animation->load_json(paramAnimVal))
        {
            DV_LOGERROR("Could not load parameter animation");
            return false;
        }

        String wrapModeString = paramAnimVal.Get("wrapmode").GetString();
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = paramAnimVal.Get("speed").GetFloat();
        SetShaderParameterAnimation(name, animation, wrapMode, speed);
    }

    JSONValue cullVal = source.Get("cull");
    if (!cullVal.IsNull())
        SetCullMode((CullMode)GetStringListIndex(cullVal.GetString().c_str(), cullModeNames, CULL_CCW));

    JSONValue shadowCullVal = source.Get("shadowcull");
    if (!shadowCullVal.IsNull())
        SetShadowCullMode((CullMode)GetStringListIndex(shadowCullVal.GetString().c_str(), cullModeNames, CULL_CCW));

    JSONValue fillVal = source.Get("fill");
    if (!fillVal.IsNull())
        SetFillMode((FillMode)GetStringListIndex(fillVal.GetString().c_str(), fillModeNames, FILL_SOLID));

    JSONValue depthBiasVal = source.Get("depthbias");
    if (!depthBiasVal.IsNull())
        SetDepthBias(BiasParameters(depthBiasVal.Get("constant").GetFloat(), depthBiasVal.Get("slopescaled").GetFloat()));

    JSONValue alphaToCoverageVal = source.Get("alphatocoverage");
    if (!alphaToCoverageVal.IsNull())
        SetAlphaToCoverage(alphaToCoverageVal.GetBool());

    JSONValue lineAntiAliasVal = source.Get("lineantialias");
    if (!lineAntiAliasVal.IsNull())
        SetLineAntiAlias(lineAntiAliasVal.GetBool());

    JSONValue renderOrderVal = source.Get("renderorder");
    if (!renderOrderVal.IsNull())
        SetRenderOrder((i8)renderOrderVal.GetI32());

    JSONValue occlusionVal = source.Get("occlusion");
    if (!occlusionVal.IsNull())
        SetOcclusion(occlusionVal.GetBool());

    RefreshShaderParameterHash();
    RefreshMemoryUse();
    return true;
}

bool Material::Save(XmlElement& dest) const
{
    if (dest.IsNull())
    {
        DV_LOGERROR("Can not save material to null XML element");
        return false;
    }

    // Write techniques
    for (const TechniqueEntry& entry : techniques_)
    {
        if (!entry.technique_)
            continue;

        XmlElement techniqueElem = dest.create_child("technique");
        techniqueElem.SetString("name", entry.technique_->GetName());
        techniqueElem.SetI32("quality", entry.qualityLevel_);
        techniqueElem.SetFloat("loddistance", entry.lodDistance_);
    }

    // Write texture units
    for (unsigned j = 0; j < MAX_TEXTURE_UNITS; ++j)
    {
        Texture* texture = GetTexture((TextureUnit)j);
        if (texture)
        {
            XmlElement textureElem = dest.create_child("texture");
            textureElem.SetString("unit", textureUnitNames[j]);
            textureElem.SetString("name", texture->GetName());
        }
    }

    // Write shader compile defines
    if (!vertexShaderDefines_.Empty() || !pixelShaderDefines_.Empty())
    {
        XmlElement shaderElem = dest.create_child("shader");
        if (!vertexShaderDefines_.Empty())
            shaderElem.SetString("vsdefines", vertexShaderDefines_);
        if (!pixelShaderDefines_.Empty())
            shaderElem.SetString("psdefines", pixelShaderDefines_);
    }

    // Write shader parameters
    for (HashMap<StringHash, MaterialShaderParameter>::ConstIterator j = shaderParameters_.Begin();
         j != shaderParameters_.End(); ++j)
    {
        XmlElement parameterElem = dest.create_child("parameter");
        parameterElem.SetString("name", j->second_.name_);
        if (j->second_.value_.GetType() != VAR_BUFFER && j->second_.value_.GetType() != VAR_INT && j->second_.value_.GetType() != VAR_BOOL)
            parameterElem.SetVectorVariant("value", j->second_.value_);
        else
        {
            parameterElem.SetAttribute("type", j->second_.value_.GetTypeName());
            parameterElem.SetAttribute("value", j->second_.value_.ToString());
        }
    }

    // Write shader parameter animations
    for (HashMap<StringHash, SharedPtr<ShaderParameterAnimationInfo>>::ConstIterator j = shaderParameterAnimationInfos_.Begin();
         j != shaderParameterAnimationInfos_.End(); ++j)
    {
        ShaderParameterAnimationInfo* info = j->second_;
        XmlElement parameterAnimationElem = dest.create_child("parameteranimation");
        parameterAnimationElem.SetString("name", info->GetName());
        if (!info->GetAnimation()->save_xml(parameterAnimationElem))
            return false;

        parameterAnimationElem.SetAttribute("wrapmode", wrapModeNames[info->wrap_mode()]);
        parameterAnimationElem.SetFloat("speed", info->GetSpeed());
    }

    // Write culling modes
    XmlElement cullElem = dest.create_child("cull");
    cullElem.SetString("value", cullModeNames[cullMode_]);

    XmlElement shadowCullElem = dest.create_child("shadowcull");
    shadowCullElem.SetString("value", cullModeNames[shadowCullMode_]);

    // Write fill mode
    XmlElement fillElem = dest.create_child("fill");
    fillElem.SetString("value", fillModeNames[fill_mode_]);

    // Write depth bias
    XmlElement depthBiasElem = dest.create_child("depthbias");
    depthBiasElem.SetFloat("constant", depthBias_.constantBias_);
    depthBiasElem.SetFloat("slopescaled", depthBias_.slopeScaledBias_);

    // Write alpha-to-coverage
    XmlElement alphaToCoverageElem = dest.create_child("alphatocoverage");
    alphaToCoverageElem.SetBool("enable", alphaToCoverage_);

    // Write line anti-alias
    XmlElement lineAntiAliasElem = dest.create_child("lineantialias");
    lineAntiAliasElem.SetBool("enable", lineAntiAlias_);

    // Write render order
    XmlElement renderOrderElem = dest.create_child("renderorder");
    renderOrderElem.SetI32("value", renderOrder_);

    // Write occlusion
    XmlElement occlusionElem = dest.create_child("occlusion");
    occlusionElem.SetBool("enable", occlusion_);

    return true;
}

bool Material::Save(JSONValue& dest) const
{
    // Write techniques
    JSONArray techniquesArray;
    techniquesArray.Reserve(techniques_.Size());
    for (const TechniqueEntry& entry : techniques_)
    {
        if (!entry.technique_)
            continue;

        JSONValue techniqueVal;
        techniqueVal.Set("name", entry.technique_->GetName());
        techniqueVal.Set("quality", (int) entry.qualityLevel_);
        techniqueVal.Set("loddistance", entry.lodDistance_);
        techniquesArray.Push(techniqueVal);
    }
    dest.Set("techniques", techniquesArray);

    // Write texture units
    JSONValue texturesValue;
    for (unsigned j = 0; j < MAX_TEXTURE_UNITS; ++j)
    {
        Texture* texture = GetTexture((TextureUnit)j);
        if (texture)
            texturesValue.Set(textureUnitNames[j], texture->GetName());
    }
    dest.Set("textures", texturesValue);

    // Write shader compile defines
    if (!vertexShaderDefines_.Empty() || !pixelShaderDefines_.Empty())
    {
        JSONValue shaderVal;
        if (!vertexShaderDefines_.Empty())
            shaderVal.Set("vsdefines", vertexShaderDefines_);
        if (!pixelShaderDefines_.Empty())
            shaderVal.Set("psdefines", pixelShaderDefines_);
        dest.Set("shader", shaderVal);
    }

    // Write shader parameters
    JSONValue shaderParamsVal;
    for (HashMap<StringHash, MaterialShaderParameter>::ConstIterator j = shaderParameters_.Begin();
         j != shaderParameters_.End(); ++j)
    {
        if (j->second_.value_.GetType() != VAR_BUFFER && j->second_.value_.GetType() != VAR_INT && j->second_.value_.GetType() != VAR_BOOL)
            shaderParamsVal.Set(j->second_.name_, j->second_.value_.ToString());
        else
        {
            JSONObject valueObj;
            valueObj["type"] = j->second_.value_.GetTypeName();
            valueObj["value"] = j->second_.value_.ToString();
            shaderParamsVal.Set(j->second_.name_, valueObj);
        }
    }
    dest.Set("shaderParameters", shaderParamsVal);

    // Write shader parameter animations
    JSONValue shaderParamAnimationsVal;
    for (HashMap<StringHash, SharedPtr<ShaderParameterAnimationInfo>>::ConstIterator j = shaderParameterAnimationInfos_.Begin();
         j != shaderParameterAnimationInfos_.End(); ++j)
    {
        ShaderParameterAnimationInfo* info = j->second_;
        JSONValue paramAnimationVal;
        if (!info->GetAnimation()->save_json(paramAnimationVal))
            return false;

        paramAnimationVal.Set("wrapmode", wrapModeNames[info->wrap_mode()]);
        paramAnimationVal.Set("speed", info->GetSpeed());
        shaderParamAnimationsVal.Set(info->GetName(), paramAnimationVal);
    }
    dest.Set("shaderParameterAnimations", shaderParamAnimationsVal);

    // Write culling modes
    dest.Set("cull", cullModeNames[cullMode_]);
    dest.Set("shadowcull", cullModeNames[shadowCullMode_]);

    // Write fill mode
    dest.Set("fill", fillModeNames[fill_mode_]);

    // Write depth bias
    JSONValue depthBiasValue;
    depthBiasValue.Set("constant", depthBias_.constantBias_);
    depthBiasValue.Set("slopescaled", depthBias_.slopeScaledBias_);
    dest.Set("depthbias", depthBiasValue);

    // Write alpha-to-coverage
    dest.Set("alphatocoverage", alphaToCoverage_);

    // Write line anti-alias
    dest.Set("lineantialias", lineAntiAlias_);

    // Write render order
    dest.Set("renderorder", renderOrder_);

    // Write occlusion
    dest.Set("occlusion", occlusion_);

    return true;
}

void Material::SetNumTechniques(i32 num)
{
    assert(num >= 0);

    if (!num)
        return;

    techniques_.Resize(num);
    RefreshMemoryUse();
}

void Material::SetTechnique(i32 index, Technique* tech, MaterialQuality qualityLevel, float lodDistance)
{
    assert(index >= 0);

    if (index >= techniques_.Size())
        return;

    techniques_[index] = TechniqueEntry(tech, qualityLevel, lodDistance);
    ApplyShaderDefines(index);
}

void Material::SetVertexShaderDefines(const String& defines)
{
    if (defines != vertexShaderDefines_)
    {
        vertexShaderDefines_ = defines;
        ApplyShaderDefines();
    }
}

void Material::SetPixelShaderDefines(const String& defines)
{
    if (defines != pixelShaderDefines_)
    {
        pixelShaderDefines_ = defines;
        ApplyShaderDefines();
    }
}

void Material::SetShaderParameter(const String& name, const Variant& value)
{
    MaterialShaderParameter newParam;
    newParam.name_ = name;
    newParam.value_ = value;

    StringHash nameHash(name);
    shaderParameters_[nameHash] = newParam;

    if (nameHash == PSP_MATSPECCOLOR)
    {
        VariantType type = value.GetType();
        if (type == VAR_VECTOR3)
        {
            const Vector3& vec = value.GetVector3();
            specular_ = vec.x_ > 0.0f || vec.y_ > 0.0f || vec.z_ > 0.0f;
        }
        else if (type == VAR_VECTOR4)
        {
            const Vector4& vec = value.GetVector4();
            specular_ = vec.x_ > 0.0f || vec.y_ > 0.0f || vec.z_ > 0.0f;
        }
    }

    if (!batchedParameterUpdate_)
    {
        RefreshShaderParameterHash();
        RefreshMemoryUse();
    }
}

void Material::SetShaderParameterAnimation(const String& name, ValueAnimation* animation, WrapMode wrapMode, float speed)
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);

    if (animation)
    {
        if (info && info->GetAnimation() == animation)
        {
            info->SetWrapMode(wrapMode);
            info->SetSpeed(speed);
            return;
        }

        if (shaderParameters_.Find(name) == shaderParameters_.End())
        {
            DV_LOGERROR(GetName() + " has no shader parameter: " + name);
            return;
        }

        StringHash nameHash(name);
        shaderParameterAnimationInfos_[nameHash] = new ShaderParameterAnimationInfo(this, name, animation, wrapMode, speed);
        UpdateEventSubscription();
    }
    else
    {
        if (info)
        {
            StringHash nameHash(name);
            shaderParameterAnimationInfos_.Erase(nameHash);
            UpdateEventSubscription();
        }
    }
}

void Material::SetShaderParameterAnimationWrapMode(const String& name, WrapMode wrapMode)
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    if (info)
        info->SetWrapMode(wrapMode);
}

void Material::SetShaderParameterAnimationSpeed(const String& name, float speed)
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    if (info)
        info->SetSpeed(speed);
}

void Material::SetTexture(TextureUnit unit, Texture* texture)
{
    if (unit < MAX_TEXTURE_UNITS)
    {
        if (texture)
            textures_[unit] = texture;
        else
            textures_.Erase(unit);
    }
}

void Material::SetUVTransform(const Vector2& offset, float rotation, const Vector2& repeat)
{
    Matrix3x4 transform(Matrix3x4::IDENTITY);
    transform.m00_ = repeat.x_;
    transform.m11_ = repeat.y_;

    Matrix3x4 rotationMatrix(Matrix3x4::IDENTITY);
    rotationMatrix.m00_ = Cos(rotation);
    rotationMatrix.m01_ = Sin(rotation);
    rotationMatrix.m10_ = -rotationMatrix.m01_;
    rotationMatrix.m11_ = rotationMatrix.m00_;
    rotationMatrix.m03_ = 0.5f - 0.5f * (rotationMatrix.m00_ + rotationMatrix.m01_);
    rotationMatrix.m13_ = 0.5f - 0.5f * (rotationMatrix.m10_ + rotationMatrix.m11_);

    transform = transform * rotationMatrix;

    Matrix3x4 offsetMatrix = Matrix3x4::IDENTITY;
    offsetMatrix.m03_ = offset.x_;
    offsetMatrix.m13_ = offset.y_;

    transform = offsetMatrix * transform;

    SetShaderParameter("UOffset", Vector4(transform.m00_, transform.m01_, transform.m02_, transform.m03_));
    SetShaderParameter("VOffset", Vector4(transform.m10_, transform.m11_, transform.m12_, transform.m13_));
}

void Material::SetUVTransform(const Vector2& offset, float rotation, float repeat)
{
    SetUVTransform(offset, rotation, Vector2(repeat, repeat));
}

void Material::SetCullMode(CullMode mode)
{
    cullMode_ = mode;
}

void Material::SetShadowCullMode(CullMode mode)
{
    shadowCullMode_ = mode;
}

void Material::SetFillMode(FillMode mode)
{
    fill_mode_ = mode;
}

void Material::SetDepthBias(const BiasParameters& parameters)
{
    depthBias_ = parameters;
    depthBias_.Validate();
}

void Material::SetAlphaToCoverage(bool enable)
{
    alphaToCoverage_ = enable;
}

void Material::SetLineAntiAlias(bool enable)
{
    lineAntiAlias_ = enable;
}

void Material::SetRenderOrder(i8 order)
{
    renderOrder_ = order;
}

void Material::SetOcclusion(bool enable)
{
    occlusion_ = enable;
}

void Material::SetScene(Scene* scene)
{
    UnsubscribeFromEvent(E_UPDATE);
    UnsubscribeFromEvent(E_ATTRIBUTEANIMATIONUPDATE);
    subscribed_ = false;
    scene_ = scene;
    UpdateEventSubscription();
}

void Material::RemoveShaderParameter(const String& name)
{
    StringHash nameHash(name);
    shaderParameters_.Erase(nameHash);

    if (nameHash == PSP_MATSPECCOLOR)
        specular_ = false;

    RefreshShaderParameterHash();
    RefreshMemoryUse();
}

void Material::ReleaseShaders()
{
    for (i32 i = 0; i < techniques_.Size(); ++i)
    {
        Technique* tech = techniques_[i].technique_;
        if (tech)
            tech->ReleaseShaders();
    }
}

SharedPtr<Material> Material::Clone(const String& cloneName) const
{
    SharedPtr<Material> ret(new Material());

    ret->SetName(cloneName);
    ret->techniques_ = techniques_;
    ret->vertexShaderDefines_ = vertexShaderDefines_;
    ret->pixelShaderDefines_ = pixelShaderDefines_;
    ret->shaderParameters_ = shaderParameters_;
    ret->shaderParameterHash_ = shaderParameterHash_;
    ret->textures_ = textures_;
    ret->depthBias_ = depthBias_;
    ret->alphaToCoverage_ = alphaToCoverage_;
    ret->lineAntiAlias_ = lineAntiAlias_;
    ret->occlusion_ = occlusion_;
    ret->specular_ = specular_;
    ret->cullMode_ = cullMode_;
    ret->shadowCullMode_ = shadowCullMode_;
    ret->fill_mode_ = fill_mode_;
    ret->renderOrder_ = renderOrder_;
    ret->RefreshMemoryUse();

    return ret;
}

void Material::SortTechniques()
{
    std::sort(techniques_.Begin(), techniques_.End(), CompareTechniqueEntries);
}

void Material::MarkForAuxView(i32 frameNumber)
{
    assert(frameNumber > 0);
    auxViewFrameNumber_ = frameNumber;
}

const TechniqueEntry& Material::GetTechniqueEntry(i32 index) const
{
    assert(index >= 0);
    return index < techniques_.Size() ? techniques_[index] : noEntry;
}

Technique* Material::GetTechnique(i32 index) const
{
    assert(index >= 0);
    return index < techniques_.Size() ? techniques_[index].technique_ : nullptr;
}

Pass* Material::GetPass(i32 index, const String& passName) const
{
    assert(index >= 0);
    Technique* tech = index < techniques_.Size() ? techniques_[index].technique_ : nullptr;
    return tech ? tech->GetPass(passName) : nullptr;
}

Texture* Material::GetTexture(TextureUnit unit) const
{
    HashMap<TextureUnit, SharedPtr<Texture>>::ConstIterator i = textures_.Find(unit);
    return i != textures_.End() ? i->second_.Get() : nullptr;
}

const Variant& Material::GetShaderParameter(const String& name) const
{
    HashMap<StringHash, MaterialShaderParameter>::ConstIterator i = shaderParameters_.Find(name);
    return i != shaderParameters_.End() ? i->second_.value_ : Variant::EMPTY;
}

ValueAnimation* Material::GetShaderParameterAnimation(const String& name) const
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    return info == nullptr ? nullptr : info->GetAnimation();
}

WrapMode Material::GetShaderParameterAnimationWrapMode(const String& name) const
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    return info == nullptr ? WM_LOOP : info->wrap_mode();
}

float Material::GetShaderParameterAnimationSpeed(const String& name) const
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    return info == nullptr ? 0 : info->GetSpeed();
}

Scene* Material::GetScene() const
{
    return scene_;
}

String Material::GetTextureUnitName(TextureUnit unit)
{
    return textureUnitNames[unit];
}

Variant Material::ParseShaderParameterValue(const String& value)
{
    String valueTrimmed = value.Trimmed();
    if (valueTrimmed.Length() && IsAlpha((unsigned)valueTrimmed[0]))
        return Variant(ToBool(valueTrimmed));
    else
        return ToVectorVariant(valueTrimmed);
}

void Material::ResetToDefaults()
{
    // Needs to be a no-op when async loading, as this does a GetResource() which is not allowed from worker threads
    if (!Thread::IsMainThread())
        return;

    vertexShaderDefines_.Clear();
    pixelShaderDefines_.Clear();

    SetNumTechniques(1);
    SetTechnique(0, (!GParams::is_headless()) ? DV_RENDERER.GetDefaultTechnique() :
        DV_RES_CACHE.GetResource<Technique>("Techniques/NoTexture.xml"));

    textures_.Clear();

    batchedParameterUpdate_ = true;
    shaderParameters_.Clear();
    shaderParameterAnimationInfos_.Clear();
    SetShaderParameter("UOffset", Vector4(1.0f, 0.0f, 0.0f, 0.0f));
    SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, 0.0f));
    SetShaderParameter("MatDiffColor", Vector4::ONE);
    SetShaderParameter("MatEmissiveColor", Vector3::ZERO);
    SetShaderParameter("MatEnvMapColor", Vector3::ONE);
    SetShaderParameter("MatSpecColor", Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    SetShaderParameter("Roughness", 0.5f);
    SetShaderParameter("Metallic", 0.0f);
    batchedParameterUpdate_ = false;

    cullMode_ = CULL_CCW;
    shadowCullMode_ = CULL_CCW;
    fill_mode_ = FILL_SOLID;
    depthBias_ = BiasParameters(0.0f, 0.0f);
    renderOrder_ = DEFAULT_RENDER_ORDER;
    occlusion_ = true;

    UpdateEventSubscription();
    RefreshShaderParameterHash();
    RefreshMemoryUse();
}

void Material::RefreshShaderParameterHash()
{
    VectorBuffer temp;
    for (HashMap<StringHash, MaterialShaderParameter>::ConstIterator i = shaderParameters_.Begin();
         i != shaderParameters_.End(); ++i)
    {
        temp.WriteStringHash(i->first_);
        temp.WriteVariant(i->second_.value_);
    }

    shaderParameterHash_ = 0;
    const byte* data = temp.GetData();
    unsigned dataSize = temp.GetSize();
    for (unsigned i = 0; i < dataSize; ++i)
        shaderParameterHash_ = SDBMHash(shaderParameterHash_, data[i]);
}

void Material::RefreshMemoryUse()
{
    unsigned memoryUse = sizeof(Material);

    memoryUse += techniques_.Size() * sizeof(TechniqueEntry);
    memoryUse += MAX_TEXTURE_UNITS * sizeof(SharedPtr<Texture>);
    memoryUse += shaderParameters_.Size() * sizeof(MaterialShaderParameter);

    SetMemoryUse(memoryUse);
}

ShaderParameterAnimationInfo* Material::GetShaderParameterAnimationInfo(const String& name) const
{
    StringHash nameHash(name);
    HashMap<StringHash, SharedPtr<ShaderParameterAnimationInfo>>::ConstIterator i = shaderParameterAnimationInfos_.Find(nameHash);
    if (i == shaderParameterAnimationInfos_.End())
        return nullptr;
    return i->second_;
}

void Material::UpdateEventSubscription()
{
    if (shaderParameterAnimationInfos_.Size() && !subscribed_)
    {
        if (scene_)
            SubscribeToEvent(scene_, E_ATTRIBUTEANIMATIONUPDATE, DV_HANDLER(Material, HandleAttributeAnimationUpdate));
        else
            SubscribeToEvent(E_UPDATE, DV_HANDLER(Material, HandleAttributeAnimationUpdate));
        subscribed_ = true;
    }
    else if (subscribed_ && shaderParameterAnimationInfos_.Empty())
    {
        UnsubscribeFromEvent(E_UPDATE);
        UnsubscribeFromEvent(E_ATTRIBUTEANIMATIONUPDATE);
        subscribed_ = false;
    }
}

void Material::HandleAttributeAnimationUpdate(StringHash eventType, VariantMap& eventData)
{
    // Timestep parameter is same no matter what event is being listened to
    float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

    // Keep weak pointer to self to check for destruction caused by event handling
    WeakPtr<Object> self(this);

    Vector<String> finishedNames;
    for (HashMap<StringHash, SharedPtr<ShaderParameterAnimationInfo>>::ConstIterator i = shaderParameterAnimationInfos_.Begin();
         i != shaderParameterAnimationInfos_.End(); ++i)
    {
        bool finished = i->second_->Update(timeStep);
        // If self deleted as a result of an event sent during animation playback, nothing more to do
        if (self.Expired())
            return;

        if (finished)
            finishedNames.Push(i->second_->GetName());
    }

    // Remove finished animations
    for (const String& finishedName : finishedNames)
        SetShaderParameterAnimation(finishedName, nullptr);
}

void Material::ApplyShaderDefines(i32 index/* = NINDEX*/)
{
    assert(index >= 0 || index == NINDEX);

    if (index == NINDEX)
    {
        for (i32 i = 0; i < techniques_.Size(); ++i)
            ApplyShaderDefines(i);
        return;
    }

    if (index >= techniques_.Size() || !techniques_[index].original_)
        return;

    if (vertexShaderDefines_.Empty() && pixelShaderDefines_.Empty())
        techniques_[index].technique_ = techniques_[index].original_;
    else
        techniques_[index].technique_ = techniques_[index].original_->CloneWithDefines(vertexShaderDefines_, pixelShaderDefines_);
}

}
