// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../math/area_allocator.h"

namespace dviglo
{

class Font;
class Image;
class Texture2D;

/// %Font glyph description.
struct DV_API FontGlyph
{
    /// X position in texture.
    short x{};
    /// Y position in texture.
    short y{};
    /// Width in texture.
    short tex_width{};
    /// Height in texture.
    short tex_height{};
    /// Width on screen.
    float width{};
    /// Height on screen.
    float height{};
    /// Glyph X offset from origin.
    float offset_x{};
    /// Glyph Y offset from origin.
    float offset_y{};
    /// Horizontal advance.
    float advance_x{};
    /// Texture page. NINDEX if not yet resident on any texture.
    i32 page{NINDEX};
    /// Used flag.
    bool used{};
};

/// %Font face description.
class DV_API FontFace : public RefCounted
{
    friend class Font;

public:
    /// Construct.
    explicit FontFace(Font* font);
    /// Destruct.
    ~FontFace() override;

    /// Load font face.
    virtual bool Load(const byte* fontData, unsigned fontDataSize, float pointSize) = 0;
    /// Return pointer to the glyph structure corresponding to a character. Return null if glyph not found.
    virtual const FontGlyph* GetGlyph(c32 c);

    /// Return if font face uses mutable glyphs.
    virtual bool HasMutableGlyphs() const { return false; }

    /// Return the kerning for a character and the next character.
    float GetKerning(c32 c, c32 d) const;
    /// Return true when one of the texture has a data loss.
    bool IsDataLost() const;

    /// Return point size.
    float GetPointSize() const { return pointSize_; }

    /// Return row height.
    float GetRowHeight() const { return rowHeight_; }

    /// Return textures.
    const Vector<SharedPtr<Texture2D>>& GetTextures() const { return textures_; }

protected:
    friend class FontFaceBitmap;
    /// Create a texture for font rendering.
    SharedPtr<Texture2D> CreateFaceTexture();
    /// Load font face texture from image resource.
    SharedPtr<Texture2D> LoadFaceTexture(const SharedPtr<Image>& image);

    /// Parent font.
    Font* font_{};
    /// Glyph mapping.
    HashMap<c32, FontGlyph> glyphMapping_;
    /// Kerning mapping.
    HashMap<u32, float> kerningMapping_;
    /// Glyph texture pages.
    Vector<SharedPtr<Texture2D>> textures_;
    /// Point size.
    float pointSize_{};
    /// Row height.
    float rowHeight_{};
};

}
