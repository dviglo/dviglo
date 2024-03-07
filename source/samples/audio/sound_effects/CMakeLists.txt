# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME audio_effects)

# Создаём список файлов
file(GLOB source_files *.cpp *.h)
set(common_source_files ../../sample.h ../../sample.inl)

# Создаём приложение
add_executable(${TARGET_NAME} ${source_files} ${common_source_files})

if(NOT DV_WIN32_CONSOLE)
    # Используем точку входа WinMain()
    set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

# Отладочная версия приложения будет иметь суффикс _d
set_property(TARGET ${TARGET_NAME} PROPERTY DEBUG_POSTFIX _d)

# Подключаем библиотеку
target_link_libraries(${TARGET_NAME} PRIVATE dviglo)

# Копируем динамические библиотеки в папку с приложением
dv_copy_shared_libs_to_bin_dir(${TARGET_NAME} "${dviglo_BINARY_DIR}/bin" copy_shared_libs_to_bin_dir)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../.. PREFIX common FILES ${common_source_files})

# Добавляем приложение в список тестируемых
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} -timeout 5)
