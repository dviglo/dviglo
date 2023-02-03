# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Движок должен быть скомпилирован с поддержкой сети и 3D-физики
if(NOT URHO3D_NETWORK OR NOT URHO3D_PHYSICS)
    return()
endif()

# Название таргета
set(target_name game_ninja_snow_war)

# Создаём список файлов
file(GLOB_RECURSE source_files *.cpp *.h)

# Создаём приложение
add_executable(${target_name} WIN32 ${source_files})

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${target_name} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${target_name} PRIVATE Urho3D)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})

# Добавляем приложение в список тестируемых
add_test(NAME ${target_name} COMMAND ${target_name} -timeout 5)
