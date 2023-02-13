# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Аналог #pragma once
include_guard(GLOBAL)

# Предупреждаем об in-source build
if(CMAKE_BINARY_DIR MATCHES "^${CMAKE_SOURCE_DIR}")
    message(WARNING "Генерировать проекты в папке с иходниками - плохая идея")
endif()

# Выводим параметры командной строки
get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach(cache_var ${CACHE_VARS})
    get_property(CACHE_VAR_HELPSTRING CACHE ${cache_var} PROPERTY HELPSTRING)

    if(CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
        get_property(cache_var_type CACHE ${cache_var} PROPERTY TYPE)

        if(cache_var_type STREQUAL "UNINITIALIZED")
            set(cache_var_type)
        else()
            set(cache_var_type ":${cache_var_type}")
        endif()

        set(cmake_args "${cmake_args} -D ${cache_var}${cache_var_type}=\"${${cache_var}}\"")
    endif ()
endforeach ()
message(STATUS "Параметры командной строки:${cmake_args}")

# Версия стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем поддержку папок в IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ==================== Опции движка ====================

# Получаем доступ к макросу cmake_dependent_option
include(CMakeDependentOption)

option(URHO3D_LOGGING "Логирование" TRUE)
option(URHO3D_THREADING "Многопоточность" TRUE)
option(URHO3D_URHO2D "2D-графика" TRUE)
option(URHO3D_NETWORK "Сеть" TRUE)
option(URHO3D_FILEWATCHER "Filewatcher" TRUE)
option(DV_BULLET "3D-физика" TRUE)
option(DV_BOX2D "2D-физика" TRUE)
option(URHO3D_TESTING "CTest")
option(URHO3D_SAMPLES "Примеры" TRUE)
option(URHO3D_TOOLS "Инструменты" TRUE)
option(DV_NAVIGATION "Навигация" TRUE)
cmake_dependent_option(URHO3D_OPENGL "OpenGL" TRUE "WIN32" TRUE) # Не на Windows всегда TRUE
cmake_dependent_option(URHO3D_D3D11 "Direct3D 11" TRUE "WIN32" FALSE) # Не на Windows всегда FALSE
cmake_dependent_option(DV_STATIC_RUNTIME "Сатическая линковка MSVC runtime" FALSE "MSVC" FALSE)

if(DV_STATIC_RUNTIME)
    # Статически линкуем MSVC runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # Используем динамическую версию MSVC runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(URHO3D_TESTING)
    enable_testing() # Должно быть в корневом CMakeLists.txt
endif()

# ==================== Ищем DirectX ====================

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Копируем d3dcompiler_xx.dll в папку bin. Этот файл есть в Windows SDK,
# но отсутствует в DirectX redist, поэтому его надо распространять вместе с игрой
if(WIN32)
    set(DIRECTX_REQUIRED_COMPONENTS D3D11)

    find_package(DirectX REQUIRED ${DIRECTX_REQUIRED_COMPONENTS})
    if(DIRECTX_FOUND)
        if(DIRECT3D_DLL AND URHO3D_D3D11)
            file(COPY ${DIRECT3D_DLL} DESTINATION ${CMAKE_BINARY_DIR}/bin)
            file(COPY ${DIRECT3D_DLL} DESTINATION ${CMAKE_BINARY_DIR}/bin/tool)
        endif()
    endif ()
endif ()

# ==================== Утилиты ====================

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

# Создаёт символическую ссылку для папки. Требует админских прав в Windows. Если создать символическую
# ссылку не удалось, то копирует папку.
# Существует функция file(CREATE_LINK ${from} ${to} COPY_ON_ERROR SYMBOLIC), однако в случае
# неудачи она копирует папку без содержимого
function(dv_create_symlink from to)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${from} ${to} RESULT_VARIABLE RESULT)

    if(NOT RESULT EQUAL "0")
        message("Не удалось создать символическую ссылку. Нужно запускать cmake от Админа. Копируем вместо создания ссылки")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory  ${from} ${to})
    endif()
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
        if(MSVC AND NOT DV_STATIC_RUNTIME)
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
