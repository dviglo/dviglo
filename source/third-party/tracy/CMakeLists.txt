# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME tracy)

# Создаём список h-файлов
file(GLOB_RECURSE source_files tracy/*.h)

# Нужен единственный cpp-файл
list(APPEND source_files tracy/TracyClient.cpp)

# Definition
add_definitions (-DTRACY_ENABLE=1)

if (MINGW)
    add_definitions (-D_WIN32_WINNT=0x601)
endif ()

if (DV_SHARED)
    add_definitions (-DTRACY_EXPORTS)
endif ()

# On Android, we define TRACY_TIMER_FALLBACK to make timer work on both physical and virtual devices
if (ANDROID)
    add_definitions (-DTRACY_TIMER_FALLBACK=1)
endif ()

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tracy FILES ${source_files})
