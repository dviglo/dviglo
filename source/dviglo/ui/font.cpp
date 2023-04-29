// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "font.h"

#include "../core/context.h"
#include "../core/profiler.h"
#include "../graphics/graphics.h"
#include "../io/deserializer.h"
#include "../io/file_system.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_element.h"
#include "../resource/xml_file.h"
#include "font_face_bitmap.h"
#include "font_face_freetype.h"

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

namespace
{
    // Convert float to 26.6 fixed-point (as used internally by FreeType)
    inline int FloatToFixed(float value)
    {
        return (int)(value * 64);
    }
}

static const float MIN_POINT_SIZE = 1;
static const float MAX_POINT_SIZE = 96;

Font::Font() :
    fontDataSize_(0),
    absoluteOffset_(IntVector2::ZERO),
    scaledOffset_(Vector2::ZERO),
    fontType_(FONT_NONE),
    sdfFont_(false)
{
}

Font::~Font()
{
    // To ensure FreeType deallocates properly, first clear all faces, then release the raw font data
    ReleaseFaces();
    fontData_.reset();
}

void Font::register_object()
{
    DV_CONTEXT->RegisterFactory<Font>();
}

bool Font::begin_load(Deserializer& source)
{
    // In headless mode, do not actually load, just return success
    if (GParams::is_headless())
        return true;

    fontType_ = FONT_NONE;
    faces_.Clear();

    fontDataSize_ = source.GetSize();
    if (fontDataSize_)
    {
        fontData_.reset(new byte[fontDataSize_]);
        if (source.Read(&fontData_[0], fontDataSize_) != fontDataSize_)
            return false;
    }
    else
    {
        fontData_.reset();
        return false;
    }

    String ext = GetExtension(GetName());
    if (ext == ".ttf" || ext == ".otf" || ext == ".woff")
    {
        fontType_ = FONT_FREETYPE;
        LoadParameters();
    }
    else if (ext == ".xml" || ext == ".fnt" || ext == ".sdf")
        fontType_ = FONT_BITMAP;

    sdfFont_ = ext == ".sdf";

    SetMemoryUse(fontDataSize_);
    return true;
}

bool Font::save_xml(Serializer& dest, int pointSize, bool usedGlyphs, const String& indentation)
{
    FontFace* fontFace = GetFace(pointSize);
    if (!fontFace)
        return false;

    DV_PROFILE(FontSaveXML);

    unique_ptr<FontFaceBitmap> packedFontFace(new FontFaceBitmap(this));
    if (!packedFontFace->Load(fontFace, usedGlyphs))
        return false;

    return packedFontFace->Save(dest, pointSize, indentation);
}

void Font::SetAbsoluteGlyphOffset(const IntVector2& offset)
{
    absoluteOffset_ = offset;
}

void Font::SetScaledGlyphOffset(const Vector2& offset)
{
    scaledOffset_ = offset;
}

FontFace* Font::GetFace(float pointSize)
{
    // In headless mode, always return null
    if (GParams::is_headless())
        return nullptr;

    // For bitmap font type, always return the same font face provided by the font's bitmap file regardless of the actual requested point size
    if (fontType_ == FONT_BITMAP)
        pointSize = 0;
    else
        pointSize = Clamp(pointSize, MIN_POINT_SIZE, MAX_POINT_SIZE);

    // For outline fonts, we return the nearest size in 1/64th increments, as that's what FreeType supports.
    int key = FloatToFixed(pointSize);
    HashMap<int, SharedPtr<FontFace>>::Iterator i = faces_.Find(key);
    if (i != faces_.End())
    {
        if (!i->second_->IsDataLost())
            return i->second_;
        else
        {
            // Erase and reload face if texture data lost (OpenGL mode only)
            faces_.Erase(i);
        }
    }

    DV_PROFILE(GetFontFace);

    switch (fontType_)
    {
    case FONT_FREETYPE:
        return GetFaceFreeType(pointSize);

    case FONT_BITMAP:
        return GetFaceBitmap(pointSize);

    default:
        return nullptr;
    }
}

IntVector2 Font::GetTotalGlyphOffset(float pointSize) const
{
    Vector2 multipliedOffset = pointSize * scaledOffset_;
    return absoluteOffset_ + IntVector2(RoundToInt(multipliedOffset.x), RoundToInt(multipliedOffset.y));
}

void Font::ReleaseFaces()
{
    faces_.Clear();
}

void Font::LoadParameters()
{
    String xmlName = replace_extension(GetName(), ".xml");
    SharedPtr<XmlFile> xml = DV_RES_CACHE->GetTempResource<XmlFile>(xmlName, false);
    if (!xml)
        return;

    XmlElement rootElem = xml->GetRoot();

    XmlElement absoluteElem = rootElem.GetChild("absoluteoffset");
    if (!absoluteElem)
        absoluteElem = rootElem.GetChild("absolute");

    if (absoluteElem)
    {
        absoluteOffset_.x = absoluteElem.GetI32("x");
        absoluteOffset_.y = absoluteElem.GetI32("y");
    }

    XmlElement scaledElem = rootElem.GetChild("scaledoffset");
    if (!scaledElem)
        scaledElem = rootElem.GetChild("scaled");

    if (scaledElem)
    {
        scaledOffset_.x = scaledElem.GetFloat("x");
        scaledOffset_.y = scaledElem.GetFloat("y");
    }
}

FontFace* Font::GetFaceFreeType(float pointSize)
{
    SharedPtr<FontFace> newFace(new FontFaceFreeType(this));
    if (!newFace->Load(fontData_.get(), fontDataSize_, pointSize))
        return nullptr;

    int key = FloatToFixed(pointSize);
    faces_[key] = newFace;
    return newFace;
}

FontFace* Font::GetFaceBitmap(float pointSize)
{
    SharedPtr<FontFace> newFace(new FontFaceBitmap(this));
    if (!newFace->Load(fontData_.get(), fontDataSize_, pointSize))
        return nullptr;

    int key = FloatToFixed(pointSize);
    faces_[key] = newFace;
    return newFace;
}

} // namespace dviglo
