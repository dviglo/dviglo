# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME detour_tile_cache)

# Создаём список файлов
file(GLOB_RECURSE source_files detour_tile_cache/*.cpp detour_tile_cache/*.h)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/detour_tile_cache)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Эта библиотека использует другую библиотеку
target_link_libraries(${TARGET_NAME} PUBLIC detour)

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/detour_tile_cache FILES ${source_files})
