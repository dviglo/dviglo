# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Должна быть включена опция
if(NOT URHO3D_NAVIGATION)
    return()
endif()

# Название таргета
set(target_name detour_crowd)

# Создаём список файлов
file(GLOB_RECURSE source_files detour_crowd/*.cpp detour_crowd/*.h)

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке и таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/detour_crowd)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Эта библиотека использует другую библиотеку
target_link_libraries(${target_name} PUBLIC detour)

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/detour_crowd FILES ${source_files})
