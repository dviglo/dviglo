# Copyright (c) 2022-2024 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME etcpack)

# Из всех исходников нужен только один файл. И даже h-файлы не нужны
set(source_files source/etcdec.cxx)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source FILES ${source_files})
