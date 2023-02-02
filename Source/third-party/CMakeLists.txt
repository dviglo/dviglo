# Copyright (c) 2022-2023 the Dviglo project
# Copyright (c) 2008-2023 the Urho3D project
# License: MIT

# Помещаем сторонние библиотеки в отдельный проект
project(dviglo-third-party)

# В IDE таргеты будут отображаться в папке third-party libs
set(CMAKE_FOLDER "third-party libs")

foreach (TARGET etcpack FreeType lz4 PugiXml rapidjson sdl StanHull stb)
    add_subdirectory (${TARGET})
endforeach ()

if (URHO3D_LUA)
    add_subdirectory (Lua${JIT})
    add_subdirectory (toluapp/src/lib)
endif ()

if (URHO3D_NETWORK)
    add_subdirectory (civetweb)
    add_subdirectory (slikenet)
endif ()

if (URHO3D_DATABASE_ODBC)
    add_subdirectory (nanodbc)
endif ()
if (URHO3D_DATABASE_SQLITE)
    add_subdirectory (SQLite)
endif ()

if (URHO3D_IK)
    add_subdirectory (ik)
endif ()

if (URHO3D_NAVIGATION)
    add_subdirectory (Detour)
    add_subdirectory (DetourCrowd)
    add_subdirectory (DetourTileCache)
    add_subdirectory (Recast)
endif ()

if (URHO3D_URHO2D)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/spine)
        add_subdirectory (spine)
        # Add definition for Spine
        add_definitions (-DURHO3D_SPINE)
    endif ()
endif ()

if (URHO3D_PHYSICS)
    add_subdirectory (Bullet)
endif ()

if (URHO3D_PHYSICS2D)
    add_subdirectory (box2d)
endif ()

if (URHO3D_TRACY_PROFILING)
    add_subdirectory (Tracy)
endif ()

if (NOT ANDROID AND NOT ARM AND NOT WEB)
    if (URHO3D_OPENGL)
        add_subdirectory(glew)
    endif()

    if (NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
        add_subdirectory (LibCpuId)
    endif ()
endif ()
