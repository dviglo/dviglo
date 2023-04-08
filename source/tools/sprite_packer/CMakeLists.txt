# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME sprite_packer)

# Создаём список файлов
file(GLOB_RECURSE source_files *.cpp *.h)

# Создаём приложение
add_executable(${TARGET_NAME} ${source_files})

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${TARGET_NAME} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${TARGET_NAME} PRIVATE dviglo)

# Копируем динамические библиотеки в папку с приложением
dv_copy_shared_libs_to_bin_dir(${TARGET_NAME} "${dviglo_BINARY_DIR}/bin/tool" copy_shared_libs_to_tool_dir)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
