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
