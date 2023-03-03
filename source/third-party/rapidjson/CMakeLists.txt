# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME rapidjson)

# Создаём список файлов
file(GLOB_RECURSE source_files include/rapidjson/*.h)

# Создаём интерфейсную библиотеку. Интерфейсная библиотека не компилируется,
# так как состоит только из заголовочных файлов
add_library(${TARGET_NAME} INTERFACE ${source_files})

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE include)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/rapidjson FILES ${source_files})
