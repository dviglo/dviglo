# Название таргета
set(target_name rapidjson)

# Создаём список файлов
file(GLOB_RECURSE source_files include/rapidjson/*.h)

# Создаём интерфейсную библиотеку. Интерфейсная библиотека не компилируется,
# так как состоит только из заголовочных файлов
add_library(${target_name} INTERFACE ${source_files})

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${target_name} INTERFACE include)

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/rapidjson FILES ${source_files})
