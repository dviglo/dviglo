# Copyright (c) 2022-2024 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME dviglo)

# Отладочная версия библиотеки будут иметь суффикс _d.
# Не работает для экзешников
set(CMAKE_DEBUG_POSTFIX _d)

# Начинаем формировать список исходников
file(GLOB source_files
    audio/*.cpp                audio/*.h
    common/*.cpp               common/*.h
    containers/*.cpp           containers/*.h
    core/*.cpp                 core/*.h
    engine/*.cpp               engine/*.h
    graphics/*.cpp             graphics/*.h
    graphics_api/*.cpp         graphics_api/*.h
    graphics_api/opengl/*.cpp  graphics_api/opengl/*.h
    input/*.cpp                input/*.h
    io/*.cpp                   io/*.h
    math/*.cpp                 math/*.h
    resource/*.cpp             resource/*.h
    scene/*.cpp                scene/*.h
    ui/*.cpp                   ui/*.h
)

# Заодно начинаем формировать содержимое файла dviglo_all.h
file(GLOB dviglo_all_content RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    audio/*.h
    common/*.h
    containers/*.h
    core/*.h
    engine/*.h
    graphics/*.h
    graphics_api/*.h
    graphics_api/opengl/*.h
    input/*.h
    io/*.h
    math/*.h
    resource/*.h
    scene/*.h
    ui/*.h
)

# Удаляем лишние элементы из списка
list(REMOVE_ITEM dviglo_all_content "common/debug_new.h" "common/config.h")

# Обрамляем каждый элемент в списке
list(TRANSFORM dviglo_all_content REPLACE "(.+)" "#include <dviglo/\\1>")

if(DV_NAVIGATION)
    # Пополняем список исходников
    file(GLOB tmp_var navigation/*.cpp navigation/*.h)
    list(APPEND source_files ${tmp_var})

    # Пополняем содержимое файла dviglo_all.h
    file(GLOB tmp_var RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} navigation/*.h)
    list(TRANSFORM tmp_var REPLACE "(.+)" "#include <dviglo/\\1>")
    list(APPEND dviglo_all_content ";#if DV_NAVIGATION")
    list(APPEND dviglo_all_content ${tmp_var})
    list(APPEND dviglo_all_content "#endif // DV_NAVIGATION")
endif()

if(DV_NETWORK)
    # Пополняем список исходников
    file(GLOB tmp_var network/*.cpp network/*.h)
    list(APPEND source_files ${tmp_var})

    # Пополняем содержимое файла dviglo_all.h
    file(GLOB tmp_var RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} network/*.h)
    list(TRANSFORM tmp_var REPLACE "(.+)" "#include <dviglo/\\1>")
    list(APPEND dviglo_all_content ";#if DV_NETWORK")
    list(APPEND dviglo_all_content ${tmp_var})
    list(APPEND dviglo_all_content "#endif // DV_NETWORK")
endif()

if(DV_BULLET)
    # Пополняем список исходников
    file(GLOB tmp_var physics/*.cpp physics/*.h)
    list(APPEND source_files ${tmp_var})

    # Пополняем содержимое файла dviglo_all.h
    file(GLOB tmp_var RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} physics/*.h)
    list(TRANSFORM tmp_var REPLACE "(.+)" "#include <dviglo/\\1>")
    list(APPEND dviglo_all_content ";#if DV_BULLET")
    list(APPEND dviglo_all_content ${tmp_var})
    list(APPEND dviglo_all_content "#endif // DV_BULLET")
endif()

if(DV_BOX2D)
    # Пополняем список исходников
    file(GLOB tmp_var physics_2d/*.cpp physics_2d/*.h)
    list(APPEND source_files ${tmp_var})

    # Пополняем содержимое файла dviglo_all.h
    file(GLOB tmp_var RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} physics_2d/*.h)
    list(TRANSFORM tmp_var REPLACE "(.+)" "#include <dviglo/\\1>")
    list(APPEND dviglo_all_content ";#if DV_BOX2D")
    list(APPEND dviglo_all_content ${tmp_var})
    list(APPEND dviglo_all_content "#endif // DV_BOX2D")
endif()

if(DV_URHO2D)
    # Пополняем список исходников
    file(GLOB tmp_var urho_2d/*.cpp urho_2d/*.h)
    list(APPEND source_files ${tmp_var})

    # Пополняем содержимое файла dviglo_all.h
    file(GLOB tmp_var RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} urho_2d/*.h)
    list(TRANSFORM tmp_var REPLACE "(.+)" "#include <dviglo/\\1>")
    list(APPEND dviglo_all_content ";#if DV_URHO2D")
    list(APPEND dviglo_all_content ${tmp_var})
    list(APPEND dviglo_all_content "#endif // DV_URHO2D")
endif()

# Список генерируемых файлов
set(generated_src_files "${CMAKE_CURRENT_BINARY_DIR}/dviglo_all.h")

if(DV_SHARED)
    # Создаём динамическую библиотеку
    add_library(${TARGET_NAME} SHARED ${source_files} ${generated_src_files})
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_SHARED=1)
else()
    # Создаём статическую библиотеку
    add_library(${TARGET_NAME} STATIC ${source_files} ${generated_src_files})
endif()

# При компиляции библиотеки и при использовании библиотеки h-файлы отличаются
target_compile_definitions(${TARGET_NAME} PRIVATE DV_IS_BUILDING=1)

# Подключаем библиотеки из папки third-party
target_link_libraries(${TARGET_NAME} PUBLIC SDL3-static stb rapidjson lz4 pugixml freetype etcpack glew)

# Не линкуем libcpuid при сборке для Linux
if(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(${TARGET_NAME} PUBLIC libcpuid)
endif()

# TODO: Будет удалено
target_compile_definitions(${TARGET_NAME} PUBLIC DV_OPENGL=1)

if(DV_MINIDUMPS)
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_MINIDUMPS=1)
    target_link_libraries(${TARGET_NAME} PUBLIC dbghelp)
endif()

if(DV_NAVIGATION)
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_NAVIGATION=1)
    target_link_libraries(${TARGET_NAME} PUBLIC detour detour_crowd detour_tile_cache recast)
endif()

if(DV_NETWORK)
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_NETWORK=1)
    target_link_libraries(${TARGET_NAME} PUBLIC civetweb slikenet)
endif()

if(DV_BULLET)
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_BULLET=1)
    target_link_libraries(${TARGET_NAME} PUBLIC bullet stanhull)
endif()

if(DV_BOX2D)
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_BOX2D=1)
    target_link_libraries(${TARGET_NAME} PUBLIC box2d)
endif()

if(DV_TRACY)
    target_compile_definitions(${TARGET_NAME} PUBLIC DV_TRACY=1)
    target_link_libraries(${TARGET_NAME} PUBLIC tracy)
endif()

# Опции, которые не требуют ничего, кроме создания дефайнов
foreach(opt
            DV_LOGGING
            DV_TESTING # enable_testing() вызывается в common.cmake
            DV_FILEWATCHER
            DV_THREADING
            DV_URHO2D
            DV_WIN32_CONSOLE)
    if(${opt})
        target_compile_definitions(${TARGET_NAME} PUBLIC ${opt}=1)
    endif()
endforeach()

if(DV_ALL_WARNINGS)
    # Выводим больше предупреждений
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)

# VS определяет дефайн _DEBUG в отладочной версии. Определяем этот дефайн также в GCC и MinGW.
# _DEBUG не определён в конфигурации RelWithDebInfo
if(NOT MSVC)
    target_compile_definitions(${TARGET_NAME} PUBLIC $<$<CONFIG:Debug>:_DEBUG>)
endif()

# Генерируем файл dviglo_all.h
list(JOIN dviglo_all_content "\n" dviglo_all_content)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dviglo_all.h.in ${CMAKE_CURRENT_BINARY_DIR}/dviglo_all.h)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
source_group("_generated" FILES ${generated_src_files})
