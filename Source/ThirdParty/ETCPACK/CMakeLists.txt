# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name etcpack)

# Из всех исходников нужен только один файл. И даже h-файлы не нужны
set(source_files source/etcdec.cxx)

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files})

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source FILES ${source_files})
