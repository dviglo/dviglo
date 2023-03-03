# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name graph3d_water)

# Создаём список файлов
file(GLOB source_files *.cpp *.h)
set(common_source_files ../../Sample.h ../../Sample.inl)

# Создаём приложение
add_executable(${target_name} ${source_files} ${common_source_files})

if(NOT DV_WIN32_CONSOLE)
    # Используем точку входа WinMain()
    set_property(TARGET ${target_name} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${target_name} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${target_name} PRIVATE dviglo)

# Копируем динамические библиотеки в папку с приложением
dv_copy_shared_libs_to_bin_dir(${target_name} "${CMAKE_BINARY_DIR}/bin" copy_shared_libs_to_bin_dir)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../.. PREFIX common FILES ${common_source_files})

# Добавляем приложение в список тестируемых
add_test(NAME ${target_name} COMMAND ${target_name} -timeout 5)
