# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME stb)

# Создаём список файлов
file(GLOB_RECURSE source_files *.c *.h)

# Создаём интерфейсную библиотеку. Интерфейсная библиотека не компилируется,
# так как состоит только из включаемых файлов
add_library(${TARGET_NAME} INTERFACE ${source_files})

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files})
