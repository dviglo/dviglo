# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME tracy)

# Создаём список заголовочных файлов
file(GLOB_RECURSE source_files public/*.h public/*.hpp)

# Нужен единственный cpp-файл, который инклудит остальные cpp-файлы
list(APPEND source_files public/TracyClient.cpp)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Добавляем дефайны
target_compile_definitions(${TARGET_NAME} PRIVATE TRACY_ENABLE=1)

if(DV_SHARED)
    target_compile_definitions(${TARGET_NAME} PRIVATE TRACY_EXPORTS=1)
endif ()

if(MINGW)
    # Подключаем библиотеки
    target_link_libraries(${TARGET_NAME} PRIVATE dbghelp ws2_32)
endif ()

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/public)

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/public FILES ${source_files})
