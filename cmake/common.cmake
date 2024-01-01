# Copyright (c) 2022-2024 the Dviglo project
# License: MIT

# Аналог #pragma once
include_guard(GLOBAL)

# Если используется одноконфигурационный генератор
# и конфигурация не указана
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    # то конфигурацией по умолчанию будет Release
    set(CMAKE_BUILD_TYPE Release)

    # Нельзя оставлять переменную CMAKE_BUILD_TYPE пустой,
    # так как при этом не будут заданы флаги GCC и MinGW:
    # * Пустая строка: CXX_FLAGS = -std=c++20
    # * Release: CXX_FLAGS = -O3 -DNDEBUG -std=c++20
    # * Debug: CXX_FLAGS = -g -std=c++20
    # * RelWithDebInfo: CXX_FLAGS = -O2 -g -DNDEBUG -std=c++20
    # * MinSizeRel: CXX_FLAGS = -Os -DNDEBUG -std=c++20
    # Флаги можно посмотреть в файле build/source/dviglo/CMakeFiles/dviglo.dir/flags.make
endif()

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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем поддержку папок в IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ==================== Опции движка ====================

# Получаем доступ к макросу cmake_dependent_option
include(CMakeDependentOption)

option(DV_SHARED "Динамическая версия (а не статическая)" FALSE)
option(DV_LOGGING "Логирование" TRUE)
option(DV_THREADING "Многопоточность" TRUE)
option(DV_URHO2D "2D-графика" TRUE)
option(DV_NETWORK "Сеть" TRUE)
option(DV_FILEWATCHER "Filewatcher" TRUE)
option(DV_BULLET "3D-физика" TRUE)
option(DV_BOX2D "2D-физика" TRUE)
option(DV_TESTING "CTest")
option(DV_SAMPLES "Примеры" TRUE)
option(DV_TOOLS "Инструменты" TRUE)
option(DV_NAVIGATION "Навигация" TRUE)
option(DV_TRACY "Профилирование" FALSE)
cmake_dependent_option(DV_STATIC_RUNTIME "Статическая линковка MSVC runtime" FALSE "MSVC" FALSE)
cmake_dependent_option(DV_WIN32_CONSOLE "Использовать main(), а не WinMain()" FALSE "WIN32" FALSE) # Не на Windows всегда FALSE
option(DV_ALL_WARNINGS "Все предупреждения компилятора" FALSE) # Влияет только на таргет dviglo

if(DV_STATIC_RUNTIME)
    # Статически линкуем MSVC runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # Используем динамическую версию MSVC runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(DV_TESTING)
    enable_testing() # Должно быть в корневом CMakeLists.txt
endif()

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

# Создаёт ссылку для папки. Если ссылку создать не удалось, то копирует папку
function(dv_create_dir_link from to)
    if(NOT CMAKE_HOST_WIN32)
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${from} ${to}
                        OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE RESULT)
    else()
        # Не используем create_symlink в Windows, так как создание symbolic links
        # [требует админских прав](https://ss64.com/nt/mklink.html),
        # а поддержка junctions из CMake
        # [была удалена](https://gitlab.kitware.com/cmake/cmake/-/merge_requests/7530)
        string(REPLACE / \\ from ${from})
        string(REPLACE / \\ to ${to})
        execute_process(COMMAND cmd /C mklink /J ${to} ${from}
                        OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE RESULT)
    endif()

    if(NOT RESULT EQUAL 0)
        message("Не удалось создать ссылку для папки, поэтому копируем папку")
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
        if(DV_SHARED)
            list(APPEND libs "$<TARGET_FILE:dviglo>")
        endif()

        # Если движок не линкует MSVC runtime статически, то добавляем библиотеки в список
        if(MSVC AND NOT DV_STATIC_RUNTIME)
            # Помимо релизных будут искаться и отладочные версии библиотек
            set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
            # Не создаём таргет INSTALL
            set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
            # Заполняем список CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
            include(InstallRequiredSystemLibraries)
            # Добавляем в список
            list(APPEND libs ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        endif()

        # Ищем dll-ки msys64
        if(MINGW)
            # dll-ки находятся рядом с экзешником, поэтому определяем путь к экзешнику
            execute_process(COMMAND where x86_64-w64-mingw32-gcc.exe
                            OUTPUT_VARIABLE mingw_fullpath)

            # Команда where может вернуть несколько строк, если MinGW установлен в разные места.
            # Преобразуем вывод команды в список и получаем нулевой элемент
            string(REPLACE "\n" ";" mingw_fullpath ${mingw_fullpath})
            list(GET mingw_fullpath 0 mingw_fullpath)

            cmake_path(GET mingw_fullpath PARENT_PATH mingw_dir)

            # Выводим содержимое папки
            #file(GLOB mingw_dir_content ${mingw_dir}/*.dll)
            #message("!!! mingw_dir_content: ${mingw_dir_content}")

            list(APPEND libs "${mingw_dir}/libgcc_s_seh-1.dll" "${mingw_dir}/libstdc++-6.dll"
                             "${mingw_dir}/libwinpthread-1.dll")
        endif()

        # Если список пустой, то не создаём кастомный таргет
        if(NOT libs)
            return()
        endif()

        # Сам по себе таргет не активируется. Нужно использовать функцию add_dependencies
        add_custom_target(${copying_target_name}
                      COMMAND ${CMAKE_COMMAND} -E make_directory "${exe_target_dir}"
                      COMMAND ${CMAKE_COMMAND} -E copy
                          ${libs}
                          "${exe_target_dir}"
                      COMMENT "Копируем динамические библиотеки в папку ${exe_target_dir}")

        # В IDE кастомный таргет будет отображаться в папке custom_targets
        set_target_properties(${copying_target_name} PROPERTIES FOLDER "кастомные таргеты")
    endif()

    add_dependencies(${exe_target_name} ${copying_target_name})
endfunction()
