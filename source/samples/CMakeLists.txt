# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if(dviglo_subproject)
    return()
endif ()

if(NOT URHO3D_SAMPLES)
    return ()
endif ()

# Помещаем примеры в отдельный проект
project(dviglo-samples)

# В IDE таргеты будут отображаться в папке samples
set(CMAKE_FOLDER samples)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Копируем ресурсы в bin, если нужно
if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/Autoload)
    create_symlink(${CMAKE_SOURCE_DIR}/bin/Autoload ${CMAKE_BINARY_DIR}/bin/Autoload FALLBACK_TO_COPY)
endif()

if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/CoreData)
    create_symlink(${CMAKE_SOURCE_DIR}/bin/CoreData ${CMAKE_BINARY_DIR}/bin/CoreData FALLBACK_TO_COPY)
endif()

if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/Data)
    create_symlink(${CMAKE_SOURCE_DIR}/bin/Data ${CMAKE_BINARY_DIR}/bin/Data FALLBACK_TO_COPY)
endif()

# Добавляем все папки
dv_add_all_subdirs()
