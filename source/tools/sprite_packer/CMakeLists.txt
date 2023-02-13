# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name sprite_packer)

# Создаём список файлов
file(GLOB_RECURSE source_files *.cpp *.h)

# Создаём приложение
add_executable(${target_name} ${source_files})

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${target_name} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${target_name} PRIVATE dviglo)

# Копируем динамические библиотеки в папку с приложением
dv_copy_shared_libs_to_bin_dir(${target_name} "${CMAKE_BINARY_DIR}/bin/tool" copy_shared_libs_to_tool_dir)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
