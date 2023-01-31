# Добавляет все поддиректории, в которых есть CMakeLists.txt.
# Использование: dv_add_all_subdirs() или dv_add_all_subdirs(EXCLUDE_FROM_ALL)
function(dv_add_all_subdirs)
    # Список файлов и подпапок
    file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

    foreach(child ${children})
        # Если не директория, то пропускаем
        if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
            continue()
        endif()

        # Если в папке нет CMakeLists.txt, то пропускаем
        if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt)
            continue()
        endif()

        # Функция dv_add_all_subdirs() ожидает 0 аргументов.
        # Все лишние аргументы помещаются в ARGN
        add_subdirectory(${child} ${ARGN})
    endforeach()
endfunction()

# Куда будут помещены экзешники и динамические библиотеки.
# Функцию нужно вызывать перед созданием таргетов
function(dv_set_bin_dir bin_dir)
    # Для одноконфигурационных генераторов (MinGW)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)

    # Для многоконфигурационных генераторов (Visual Studio)
    foreach(config_name ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${config_name} config_name)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_name} ${bin_dir} PARENT_SCOPE)
    endforeach()
endfunction()
