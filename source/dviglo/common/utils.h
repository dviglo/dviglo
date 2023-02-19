// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "primitive_types.h"

#include <type_traits>

namespace dviglo
{

/// Преобразует enum class в u8, если базовый тип - u8
template<typename EnumClass>
constexpr std::enable_if_t<std::is_same_v<std::underlying_type_t<EnumClass>, u8>, u8> to_u8(EnumClass enum_class)
{
    return static_cast<u8>(enum_class);
}

/// Преобразует enum class в u32, если базовый тип - u32
template<typename EnumClass>
constexpr std::enable_if_t<std::is_same_v<std::underlying_type_t<EnumClass>, u32>, u32> to_u32(EnumClass enum_class)
{
    return static_cast<u32>(enum_class);
}

} // namespace dviglo
