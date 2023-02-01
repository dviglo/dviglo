# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name bullet)

# Создаём список файлов
file(GLOB_RECURSE source_files Bullet/*.cpp Bullet/*.h)

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке и таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Bullet)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Примечание: Bullet не испольузет относительные пути к заголовкам, к тому же движок
# хочет видеть заголовки в папке Bullet. Поэтому заголовки должны быть видны по разным путям

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Bullet FILES ${source_files})
