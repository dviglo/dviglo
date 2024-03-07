# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(target_name editor)

# Создаём список файлов
file(GLOB_RECURSE source_files *.cpp *.h)

# Создаём приложение
add_executable(${target_name} ${source_files})

if(NOT DV_WIN32_CONSOLE)
    # Используем точку входа WinMain()
    set_property(TARGET ${target_name} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${target_name} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку-движок к приложению
target_link_libraries(${target_name} PRIVATE dviglo)

file(MAKE_DIRECTORY ${dviglo_BINARY_DIR}/bin)

# Список папок для копирования в bin
set(dir_names autoload core_data data)

# Копируем папки в bin, если нужно
foreach(dir_name ${dir_names})
    if(NOT EXISTS ${dviglo_BINARY_DIR}/bin/${dir_name})
        dv_create_dir_link(${dviglo_SOURCE_DIR}/bin/${dir_name} ${dviglo_BINARY_DIR}/bin/${dir_name})
    endif()
endforeach()

# Копируем динамические библиотеки в папку с приложением
dv_copy_shared_libs_to_bin_dir(${target_name} "${dviglo_BINARY_DIR}/bin" copy_shared_libs_to_bin_dir)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})

# Добавляем приложение в список тестируемых
add_test(NAME ${target_name} COMMAND ${target_name} -timeout 5)
