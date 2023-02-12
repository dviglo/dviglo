# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if(DVIGLO_IS_SUBPROJECT)
    return()
endif()

if(NOT URHO3D_SAMPLES)
    return()
endif()

# Помещаем примеры в отдельный проект
project(dviglo-samples)

# В IDE таргеты будут отображаться в папке примеры
set(CMAKE_FOLDER примеры)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Копируем ресурсы в bin, если нужно
if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/Autoload)
    dv_create_symlink(${CMAKE_SOURCE_DIR}/bin/Autoload ${CMAKE_BINARY_DIR}/bin/Autoload)
endif()

if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/CoreData)
    dv_create_symlink(${CMAKE_SOURCE_DIR}/bin/CoreData ${CMAKE_BINARY_DIR}/bin/CoreData)
endif()

if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/Data)
    dv_create_symlink(${CMAKE_SOURCE_DIR}/bin/Data ${CMAKE_BINARY_DIR}/bin/Data)
endif()

# Добавляем все папки
dv_add_all_subdirs()
