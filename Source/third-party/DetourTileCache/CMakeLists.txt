# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name detour_tile_cache)

# Создаём список файлов
file(GLOB_RECURSE source_files DetourTileCache/*.cpp DetourTileCache/*.h)

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке
target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/DetourTileCache)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Эта библиотека использует другую библиотеку
target_link_libraries(${target_name} PUBLIC detour)

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/DetourTileCache FILES ${source_files})
