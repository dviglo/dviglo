# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name dviglo)

# Отладочная версия библиотеки будут иметь суффикс _d.
# Не работает для экзешников
set(CMAKE_DEBUG_POSTFIX _d)

# Начинаем формировать список исходников
file(GLOB source_files
                       *.h
    audio/*.cpp        audio/*.h
    base/*.cpp         base/*.h
    container/*.cpp    container/*.h
    core/*.cpp         core/*.h
    engine/*.cpp       engine/*.h
    graphics/*.cpp     graphics/*.h
    graphics_api/*.cpp graphics_api/*.h
    input/*.cpp        input/*.h
    io/*.cpp           io/*.h
    math/*.cpp         math/*.h
    resource/*.cpp     resource/*.h
    scene/*.cpp        scene/*.h
    ui/*.cpp           ui/*.h
)

# Заодно начинаем формировать содержимое файла dviglo_all.h
file(GLOB dviglo_all_content RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.h
    audio/*.h
    base/*.h
    container/*.h
    core/*.h
    engine/*.h
    graphics/*.h
    graphics_api/*.h
    input/*.h
    io/*.h
    math/*.h
    resource/*.h
    scene/*.h
    ui/*.h
)

# Удаляем лишние элементы из списка
list(REMOVE_ITEM dviglo_all_content "debug_new.h" "dviglo_config.h")

# Обрамляем каждый элемент в списке
list(TRANSFORM dviglo_all_content REPLACE "(.+)" "#include <dviglo/\\1>")

if(URHO3D_D3D11)
    # Пополняем список исходников
    file(GLOB tmp_var graphics_api/direct3d11/*.cpp graphics_api/direct3d11/*.h)
    list(APPEND source_files ${tmp_var})
endif()

if(URHO3D_OPENGL)
    # Пополняем список исходников
    file(GLOB tmp_var graphics_api/opengl/*.cpp graphics_api/opengl/*.h)
    list(APPEND source_files ${tmp_var})
endif()

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

if(URHO3D_URHO2D)
    # Пополняем список исходников
    file(GLOB tmp_var urho_2d/*.cpp urho_2d/*.h)
    list(APPEND source_files ${tmp_var})

    # Пополняем содержимое файла dviglo_all.h
    file(GLOB tmp_var RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} urho_2d/*.h)
    list(TRANSFORM tmp_var REPLACE "(.+)" "#include <dviglo/\\1>")
    list(APPEND dviglo_all_content ";#if URHO3D_URHO2D")
    list(APPEND dviglo_all_content ${tmp_var})
    list(APPEND dviglo_all_content "#endif // URHO3D_URHO2D")
endif()

# Список генерируемых файлов
set(generated_src_files "${CMAKE_CURRENT_BINARY_DIR}/dviglo_all.h")

if(URHO3D_LIB_TYPE STREQUAL "SHARED")
    # Создаём динамическую библиотеку
    add_library(${target_name} SHARED ${source_files} ${generated_src_files})
    target_compile_definitions(${target_name} PUBLIC DVIGLO_SHARED=1)
else()
    # Создаём статическую библиотеку
    add_library(${target_name} STATIC ${source_files} ${generated_src_files})
endif()

# При компиляции библиотеки и при использовании библиотеки h-файлы отличаются
target_compile_definitions(${target_name} PRIVATE DVIGLO_IS_BUILDING=1)

# Подключаем библиотеки из папки third-party
target_link_libraries(${target_name} PUBLIC SDL3-static stb rapidjson lz4 pugixml freetype etcpack)

# Не линкуем libcpuid при сборке для Linux
if(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(${target_name} PUBLIC libcpuid)
endif()

if(URHO3D_D3D11)
    target_compile_definitions(${target_name} PUBLIC URHO3D_D3D11=1)
    target_link_libraries(${target_name} PUBLIC ${DIRECT3D_LIBRARIES})
endif()

if(URHO3D_OPENGL)
    target_compile_definitions(${target_name} PUBLIC URHO3D_OPENGL=1)
    target_link_libraries(${target_name} PUBLIC glew)

    if(NOT WIN32) # Целевая ОС - Linux
        target_link_libraries(${target_name} PUBLIC GL)
    endif()
endif()

if(URHO3D_MINIDUMPS)
    target_compile_definitions(${target_name} PUBLIC URHO3D_MINIDUMPS=1)
    target_link_libraries(${target_name} PUBLIC dbghelp)
endif()

if(DV_NAVIGATION)
    target_compile_definitions(${target_name} PUBLIC DV_NAVIGATION=1)
    target_link_libraries(${target_name} PUBLIC detour detour_crowd detour_tile_cache recast)
endif()

if(DV_NETWORK)
    target_compile_definitions(${target_name} PUBLIC DV_NETWORK=1)
    target_link_libraries(${target_name} PUBLIC civetweb slikenet)
endif()

if(DV_BULLET)
    target_compile_definitions(${target_name} PUBLIC DV_BULLET=1)
    target_link_libraries(${target_name} PUBLIC bullet stanhull)
endif()

if(DV_BOX2D)
    target_compile_definitions(${target_name} PUBLIC DV_BOX2D=1)
    target_link_libraries(${target_name} PUBLIC box2d)
endif()

if(URHO3D_TRACY_PROFILING)
    target_compile_definitions(${target_name} PUBLIC URHO3D_TRACY_PROFILING=1)
    target_link_libraries(${target_name} PUBLIC tracy)
endif()

# Опции, которые не требуют ничего, кроме создания дефайнов
foreach(opt
            URHO3D_LOGGING
            URHO3D_TESTING # enable_testing() вызывается в common.cmake
            URHO3D_FILEWATCHER
            URHO3D_PROFILING
            URHO3D_THREADING
            URHO3D_URHO2D)
    if(${opt})
        target_compile_definitions(${target_name} PUBLIC ${opt}=1)
    endif()
endforeach()

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)

# Генерируем файл dviglo_all.h
list(JOIN dviglo_all_content "\n" dviglo_all_content)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dviglo_all.h.in ${CMAKE_CURRENT_BINARY_DIR}/dviglo_all.h)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
source_group("_generated" FILES ${generated_src_files})
