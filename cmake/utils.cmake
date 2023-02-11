# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

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
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${bin_dir} PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${bin_dir} PARENT_SCOPE)

    # Для многоконфигурационных генераторов (Visual Studio)
    foreach(config_name ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${config_name} config_name)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_name} ${bin_dir} PARENT_SCOPE)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_name} ${bin_dir} PARENT_SCOPE)
    endforeach()
endfunction()

# Функция создаёт кастомный таргет, который перед компиляцией экзешника копирует нужные dll или so в папку с экзешником.
# exe_target_name - таргет, рядом с которым надо положить библиотеки.
# exe_target_dir - папка, в которую компилируется экзешник.
# copying_target_name - название создаваемого кастомного таргета, который будет заниматься копированием.
# Если несколько экзешников компилируются в одну и ту же папку, то у них должны совпадать copying_target_name. 
# Кастомные таргеты с одинаковыми именами не будут создаваться, и не будет происходить повторного копирования библиотек.
function(dv_copy_shared_libs_to_bin_dir exe_target_name exe_target_dir copying_target_name)
    # Кастомный таргет с таким именем ещё не создан
    if(NOT TARGET ${copying_target_name})
        # Полные пути к библиотекам, которые будут скопированы в папку с экзешником
        unset(libs)

        # Если движок скомпилирован как динамическая библиотека, то добавляем её в список
        if(URHO3D_LIB_TYPE STREQUAL "SHARED")
            list(APPEND libs "$<TARGET_FILE:Urho3D>")
        endif()

        # Если движок не линкует MSVC runtime статически, то добавляем библиотеки в список
        if(MSVC AND NOT dv_static_runtime)
            # Помимо релизных будут искаться и отладочные версии библиотек
            set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
            # Заполняем список CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
            include(InstallRequiredSystemLibraries)
            # Добавляем в список
            list(APPEND libs ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        endif()

        # Если список пустой, то не создаём кастомный таргет
        if(NOT libs)
            return()
        endif()

        # Сам по себе таргет не активируется. Нужно использовать функцию add_dependencies
        add_custom_target(${copying_target_name}
                      COMMAND ${CMAKE_COMMAND} -E copy
                          ${libs}
                          "${exe_target_dir}"
                      COMMENT "Копируем динамические библиотеки в папку ${exe_target_dir}")

        # В IDE кастомный таргет будет отображаться в папке custom_targets
        set_target_properties(${copying_target_name} PROPERTIES FOLDER "кастомные таргеты")
    endif()

    add_dependencies(${exe_target_name} ${copying_target_name})
endfunction()
