# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if (dviglo_subproject)
    return()
endif ()

if (NOT URHO3D_SAMPLES)
    return ()
endif ()

# Set project name
project (Urho3D-Samples)

# В IDE примеры будут помещаться в папку samples
set (CMAKE_FOLDER samples)

# Find Urho3D library
find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS})

# Include common to all samples
set (COMMON_SAMPLE_H_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Sample.h" "${CMAKE_CURRENT_SOURCE_DIR}/Sample.inl")

# Define dependency libs
set (INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

# Добавляем все папки
dv_add_all_subdirs()
