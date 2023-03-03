# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME glew)

# Создаём список файлов
file(GLOB_RECURSE source_files glew/*.c glew/*.h)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Дефайны для библиотеки и для таргетов, которые используют текущую библиотеку
target_compile_definitions(${TARGET_NAME} PUBLIC GLEW_STATIC GLEW_NO_GLU)

# Делаем заголовочные файлы доступными библиотеке
target_include_directories(${TARGET_NAME} PRIVATE glew)

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Подключаем OpenGL к таргетам, которые используют текущую библиотеку
find_package(OpenGL)
target_link_libraries(${TARGET_NAME} INTERFACE ${OPENGL_LIBRARIES})
target_include_directories(${TARGET_NAME} INTERFACE ${OPENGL_INCLUDE_DIR})

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/glew FILES ${source_files})
