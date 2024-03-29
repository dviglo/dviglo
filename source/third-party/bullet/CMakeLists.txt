# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME bullet)

# Создаём список файлов
file(GLOB_RECURSE source_files bullet/*.cpp bullet/*.h)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Делаем заголовочные файлы доступными библиотеке и таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bullet)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Примечание: Bullet не испольузет относительные пути к заголовкам, к тому же движок
# хочет видеть заголовки в папке bullet. Поэтому заголовки должны быть видны по разным путям

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/bullet FILES ${source_files})
