# Copyright (c) 2022-2024 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME libcpuid)

# Создаём список файлов
file(GLOB_RECURSE source_files libcpuid/*.c libcpuid/*.h)

if(MSVC)
    list(APPEND source_files libcpuid/masm-x64.asm)
    enable_language(ASM_MASM)
endif()

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Добавляем дефайн. Версию можно посмотреть в libcpuid.h
target_compile_definitions(${TARGET_NAME} PRIVATE VERSION="0.6.2+")

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libcpuid)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/libcpuid FILES ${source_files})
