// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../io/log.h"
#include "../graphics_api/texture_2d.h"
#include "../resource/image.h"
#include "../ui/font.h"
#include "../ui/font_face.h"

#include "../debug_new.h"

namespace dviglo
{

FontFace::FontFace(Font* font) :
    font_(font)
{
}

FontFace::~FontFace()
{
    if (font_)
    {
        // When a face is unloaded, deduct the used texture data size from the parent font
        unsigned totalTextureSize = 0;
        for (unsigned i = 0; i < textures_.Size(); ++i)
            totalTextureSize += textures_[i]->GetWidth() * textures_[i]->GetHeight();
        font_->SetMemoryUse(font_->GetMemoryUse() - totalTextureSize);
    }
}

const FontGlyph* FontFace::GetGlyph(c32 c)
{
    HashMap<c32, FontGlyph>::Iterator i = glyphMapping_.Find(c);
    if (i != glyphMapping_.End())
    {
        FontGlyph& glyph = i->second_;
        glyph.used_ = true;
        return &glyph;
    }
    else
        return nullptr;
}

float FontFace::GetKerning(c32 c, c32 d) const
{
    if (kerningMapping_.Empty())
        return 0;

    if (c == '\n' || d == '\n')
        return 0;

    if (c > 0xffff || d > 0xffff)
        return 0;

    u32 value = (c << 16u) + d;

    HashMap<u32, float>::ConstIterator i = kerningMapping_.Find(value);
    if (i != kerningMapping_.End())
        return i->second_;

    return 0;
}

bool FontFace::IsDataLost() const
{
    for (unsigned i = 0; i < textures_.Size(); ++i)
    {
        if (textures_[i]->IsDataLost())
            return true;
    }
    return false;
}


SharedPtr<Texture2D> FontFace::CreateFaceTexture()
{
    SharedPtr<Texture2D> texture(new Texture2D(font_->GetContext()));
    texture->SetMipsToSkip(QUALITY_LOW, 0); // No quality reduction
    texture->SetNumLevels(1); // No mipmaps
    texture->SetAddressMode(COORD_U, ADDRESS_BORDER);
    texture->SetAddressMode(COORD_V, ADDRESS_BORDER);
    texture->SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    return texture;
}

SharedPtr<Texture2D> FontFace::LoadFaceTexture(const SharedPtr<Image>& image)
{
    SharedPtr<Texture2D> texture = CreateFaceTexture();
    if (!texture->SetData(image, true))
    {
        DV_LOGERROR("Could not load texture from image resource");
        return SharedPtr<Texture2D>();
    }
    return texture;
}

}
