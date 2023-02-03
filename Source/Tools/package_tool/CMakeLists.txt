# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name package_tool)

# Создаём список файлов
file(GLOB_RECURSE source_files *.cpp *.h)

# Создаём приложение
add_executable(${target_name} ${source_files})

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${target_name} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${target_name} PRIVATE Urho3D)

# Копируем динамические библиотеки в папку с приложением
if(NOT URHO3D_STATIC)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Urho3D>/$<TARGET_FILE_NAME:Urho3D> 
            $<TARGET_FILE_DIR:${target_name}>)
endif()

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
