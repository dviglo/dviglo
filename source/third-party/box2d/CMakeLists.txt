# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME box2d)

# Создаём список файлов
file(GLOB_RECURSE source_files src/*.cpp src/*.h include/*.h)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке и таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
