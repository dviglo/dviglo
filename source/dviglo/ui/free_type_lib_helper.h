// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "../core/object.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H


namespace dviglo
{

/// Обёртка вокруг библиотеки FreeType
class DV_API FreeTypeLibHelper : public Object
{
    DV_OBJECT(FreeTypeLibHelper);

    /// Только Ui может создать и уничтожить объект
    friend class UI;

private:
    /// Инициализируется в конструкторе
    inline static FreeTypeLibHelper* instance_ = nullptr;

public:
    static FreeTypeLibHelper* instance() { return instance_; }

private:
    /// Библиотека FreeType
    FT_Library library_ = nullptr;

    explicit FreeTypeLibHelper();
    ~FreeTypeLibHelper() override;

public:
    FT_Library library() const { return library_; }
};

} // namespace dviglo
