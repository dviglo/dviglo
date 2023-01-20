# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name libcpuid)

# Define preprocessor macros
add_definitions (-DVERSION="0.4.0+")
if (HAVE_STDINT_H)
    add_definitions (-DHAVE_STDINT_H)
endif ()

# Создаём список файлов
file(GLOB_RECURSE source_files LibCpuId/*.c LibCpuId/*.h)

if (MSVC)
    list(APPEND source_files LibCpuId/masm-x64.asm)
    enable_language(ASM_MASM)
endif ()

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files} ${ASM_FILES})

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/LibCpuId FILES ${source_files})
