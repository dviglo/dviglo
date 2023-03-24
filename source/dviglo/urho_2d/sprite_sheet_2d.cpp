// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics_api/texture_2d.h"
#include "../io/deserializer.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../io/path.h"
#include "../resource/json_file.h"
#include "../resource/plist_file.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"
#include "sprite_2d.h"
#include "sprite_sheet_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

SpriteSheet2D::SpriteSheet2D()
{
}

SpriteSheet2D::~SpriteSheet2D() = default;

void SpriteSheet2D::register_object()
{
    DV_CONTEXT.RegisterFactory<SpriteSheet2D>();
}

bool SpriteSheet2D::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    loadTextureName_.Clear();
    spriteMapping_.Clear();

    String extension = GetExtension(source.GetName());
    if (extension == ".plist")
        return BeginLoadFromPListFile(source);

    if (extension == ".xml")
        return BeginLoadFromXMLFile(source);

    if (extension == ".json")
        return BeginLoadFromJSONFile(source);


    DV_LOGERROR("Unsupported file type");
    return false;
}

bool SpriteSheet2D::EndLoad()
{
    if (loadPListFile_)
        return EndLoadFromPListFile();

    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    if (loadJSONFile_)
        return EndLoadFromJSONFile();

    return false;
}

void SpriteSheet2D::SetTexture(Texture2D* texture)
{
    loadTextureName_.Clear();
    texture_ = texture;
}

void SpriteSheet2D::DefineSprite(const String& name, const IntRect& rectangle, const Vector2& hotSpot, const IntVector2& offset)
{
    if (!texture_)
        return;

    if (GetSprite(name))
        return;

    SharedPtr<Sprite2D> sprite(new Sprite2D());
    sprite->SetName(name);
    sprite->SetTexture(texture_);
    sprite->SetRectangle(rectangle);
    sprite->SetHotSpot(hotSpot);
    sprite->SetOffset(offset);
    sprite->SetSpriteSheet(this);

    spriteMapping_[name] = sprite;
}

Sprite2D* SpriteSheet2D::GetSprite(const String& name) const
{
    HashMap<String, SharedPtr<Sprite2D>>::ConstIterator i = spriteMapping_.Find(name);
    if (i == spriteMapping_.End())
        return nullptr;

    return i->second_;
}

bool SpriteSheet2D::BeginLoadFromPListFile(Deserializer& source)
{
    loadPListFile_ = new PListFile();
    if (!loadPListFile_->Load(source))
    {
        DV_LOGERROR("Could not load sprite sheet");
        loadPListFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    const PListValueMap& root = loadPListFile_->GetRoot();
    const PListValueMap& metadata = root["metadata"]->GetValueMap();
    const String& textureFileName = metadata["realTextureFileName"]->GetString();

    // If we're async loading, request the texture now. Finish during EndLoad().
    loadTextureName_ = get_parent(GetName()) + textureFileName;
    if (GetAsyncLoadState() == ASYNC_LOADING)
        DV_RES_CACHE.background_load_resource<Texture2D>(loadTextureName_, true, this);

    return true;
}

bool SpriteSheet2D::EndLoadFromPListFile()
{
    texture_ = DV_RES_CACHE.GetResource<Texture2D>(loadTextureName_);
    if (!texture_)
    {
        DV_LOGERROR("Could not load texture " + loadTextureName_);
        loadPListFile_.Reset();
        loadTextureName_.Clear();
        return false;
    }

    const PListValueMap& root = loadPListFile_->GetRoot();
    const PListValueMap& frames = root["frames"]->GetValueMap();
    for (PListValueMap::ConstIterator i = frames.Begin(); i != frames.End(); ++i)
    {
        String name = i->first_.Split('.')[0];

        const PListValueMap& frameInfo = i->second_.GetValueMap();
        if (frameInfo["rotated"]->GetBool())
        {
            DV_LOGWARNING("Rotated sprite is not support now");
            continue;
        }

        IntRect rectangle = frameInfo["frame"]->GetIntRect();
        Vector2 hotSpot(0.5f, 0.5f);
        IntVector2 offset(0, 0);

        IntRect sourceColorRect = frameInfo["sourceColorRect"]->GetIntRect();
        if (sourceColorRect.left_ != 0 && sourceColorRect.top_ != 0)
        {
            offset.x = -sourceColorRect.left_;
            offset.y = -sourceColorRect.top_;

            IntVector2 sourceSize = frameInfo["sourceSize"]->GetIntVector2();
            hotSpot.x = (offset.x + sourceSize.x / 2.f) / rectangle.Width();
            hotSpot.y = 1.0f - (offset.y + sourceSize.y / 2.f) / rectangle.Height();
        }

        DefineSprite(name, rectangle, hotSpot, offset);
    }

    loadPListFile_.Reset();
    loadTextureName_.Clear();
    return true;
}

bool SpriteSheet2D::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XmlFile();
    if (!loadXMLFile_->Load(source))
    {
        DV_LOGERROR("Could not load sprite sheet");
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    XmlElement rootElem = loadXMLFile_->GetRoot("TextureAtlas");
    if (!rootElem)
    {
        DV_LOGERROR("Invalid sprite sheet");
        loadXMLFile_.Reset();
        return false;
    }

    // If we're async loading, request the texture now. Finish during EndLoad().
    loadTextureName_ = get_parent(GetName()) + rootElem.GetAttribute("imagePath");
    if (GetAsyncLoadState() == ASYNC_LOADING)
        DV_RES_CACHE.background_load_resource<Texture2D>(loadTextureName_, true, this);

    return true;
}

bool SpriteSheet2D::EndLoadFromXMLFile()
{
    texture_ = DV_RES_CACHE.GetResource<Texture2D>(loadTextureName_);
    if (!texture_)
    {
        DV_LOGERROR("Could not load texture " + loadTextureName_);
        loadXMLFile_.Reset();
        loadTextureName_.Clear();
        return false;
    }

    XmlElement rootElem = loadXMLFile_->GetRoot("TextureAtlas");
    XmlElement subTextureElem = rootElem.GetChild("SubTexture");
    while (subTextureElem)
    {
        String name = subTextureElem.GetAttribute("name");

        int x = subTextureElem.GetI32("x");
        int y = subTextureElem.GetI32("y");
        int width = subTextureElem.GetI32("width");
        int height = subTextureElem.GetI32("height");
        IntRect rectangle(x, y, x + width, y + height);

        Vector2 hotSpot(0.5f, 0.5f);
        IntVector2 offset(0, 0);
        if (subTextureElem.HasAttribute("frameWidth") && subTextureElem.HasAttribute("frameHeight"))
        {
            offset.x = subTextureElem.GetI32("frameX");
            offset.y = subTextureElem.GetI32("frameY");
            int frameWidth = subTextureElem.GetI32("frameWidth");
            int frameHeight = subTextureElem.GetI32("frameHeight");
            hotSpot.x = (offset.x + frameWidth / 2.f) / width;
            hotSpot.y = 1.0f - (offset.y + frameHeight / 2.f) / height;
        }

        DefineSprite(name, rectangle, hotSpot, offset);

        subTextureElem = subTextureElem.GetNext("SubTexture");
    }

    loadXMLFile_.Reset();
    loadTextureName_.Clear();
    return true;
}

bool SpriteSheet2D::BeginLoadFromJSONFile(Deserializer& source)
{
    loadJSONFile_ = new JSONFile();
    if (!loadJSONFile_->Load(source))
    {
        DV_LOGERROR("Could not load sprite sheet");
        loadJSONFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    JSONValue rootElem = loadJSONFile_->GetRoot();
    if (rootElem.IsNull())
    {
        DV_LOGERROR("Invalid sprite sheet");
        loadJSONFile_.Reset();
        return false;
    }

    // If we're async loading, request the texture now. Finish during EndLoad().
    loadTextureName_ = get_parent(GetName()) + rootElem.Get("imagePath").GetString();
    if (GetAsyncLoadState() == ASYNC_LOADING)
        DV_RES_CACHE.background_load_resource<Texture2D>(loadTextureName_, true, this);

    return true;
}

bool SpriteSheet2D::EndLoadFromJSONFile()
{
    texture_ = DV_RES_CACHE.GetResource<Texture2D>(loadTextureName_);
    if (!texture_)
    {
        DV_LOGERROR("Could not load texture " + loadTextureName_);
        loadJSONFile_.Reset();
        loadTextureName_.Clear();
        return false;
    }

    JSONValue rootVal = loadJSONFile_->GetRoot();
    JSONArray subTextureArray = rootVal.Get("subtextures").GetArray();

    for (const JSONValue& subTextureVal : subTextureArray)
    {
        String name = subTextureVal.Get("name").GetString();

        int x = subTextureVal.Get("x").GetI32();
        int y = subTextureVal.Get("y").GetI32();
        int width = subTextureVal.Get("width").GetI32();
        int height = subTextureVal.Get("height").GetI32();
        IntRect rectangle(x, y, x + width, y + height);

        Vector2 hotSpot(0.5f, 0.5f);
        IntVector2 offset(0, 0);
        JSONValue frameWidthVal = subTextureVal.Get("frameWidth");
        JSONValue frameHeightVal = subTextureVal.Get("frameHeight");

        if (!frameWidthVal.IsNull() && !frameHeightVal.IsNull())
        {
            offset.x = subTextureVal.Get("frameX").GetI32();
            offset.y = subTextureVal.Get("frameY").GetI32();
            int frameWidth = frameWidthVal.GetI32();
            int frameHeight = frameHeightVal.GetI32();
            hotSpot.x = (offset.x + frameWidth / 2.f) / width;
            hotSpot.y = 1.0f - (offset.y + frameHeight / 2.f) / height;
        }

        DefineSprite(name, rectangle, hotSpot, offset);
    }

    loadJSONFile_.Reset();
    loadTextureName_.Clear();
    return true;
}

}
