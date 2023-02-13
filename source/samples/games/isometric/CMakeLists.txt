# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Движок должен быть скомпилирован с поддержкой 2D-графики и 2D-физики
if(NOT URHO3D_URHO2D OR NOT DV_BOX2D)
    return()
endif()

# Название таргета
set(target_name game_isometric)

# Создаём список файлов
file(GLOB source_files *.cpp *.h)
set(common_source_files ../../Utilities2D/Sample2D.h ../../Utilities2D/Sample2D.cpp
                        ../../Utilities2D/Mover.h ../../Utilities2D/Mover.cpp)

# Создаём приложение
add_executable(${target_name} WIN32 ${source_files} ${common_source_files})

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${target_name} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${target_name} PRIVATE Urho3D)

# Копируем динамические библиотеки в папку с приложением
dv_copy_shared_libs_to_bin_dir(${target_name} "${CMAKE_BINARY_DIR}/bin" copy_shared_libs_to_bin_dir)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../.. FILES ${common_source_files})

# Добавляем приложение в список тестируемых
add_test(NAME ${target_name} COMMAND ${target_name} -timeout 5)
