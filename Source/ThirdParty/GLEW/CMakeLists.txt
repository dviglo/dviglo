# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(target_name glew)

# Создаём список файлов
file(GLOB_RECURSE source_files GLEW/*.c GLEW/*.h)

# Создаём статическую библиотеку
add_library(${target_name} STATIC ${source_files})

# Дефайны для библиотеки и для таргетов, которые используют текущую библиотеку
target_compile_definitions(${target_name} PUBLIC GLEW_STATIC GLEW_NO_GLU)

# Делаем заголовочные файлы доступными библиотеке
target_include_directories(${target_name} PRIVATE GLEW)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Подключаем OpenGL к таргетам, которые используют текущую библиотеку
find_package(OpenGL)
target_link_libraries(${target_name} INTERFACE ${OPENGL_gl_LIBRARY})
target_include_directories(${target_name} INTERFACE ${OPENGL_INCLUDE_DIR})

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/GLEW FILES ${source_files})
