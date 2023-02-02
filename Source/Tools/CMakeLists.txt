# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if (dviglo_subproject)
    return()
endif ()

# Помещаем инструменты в отдельный проект
project(dviglo-tools)

# В IDE таргеты будут отображаться в папке tools
set(CMAKE_FOLDER tools)

# Find Urho3D library
find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS})

# Build host-tools using host compiler toolchain
if (CMAKE_CROSSCOMPILING)
    check_native_compiler_exist ()
    # When cross-compiling, build the host tool as external project
    include (ExternalProject)
    if (IOS OR TVOS)
        # When cross-compiling for iOS/tvOS the host environment has been altered by xcodebuild for the said platform, the following fix is required to reset the host environment before spawning another process to configure/generate project file for external project
        # Also workaround a known CMake/Xcode generator bug which prevents it from installing native tool binaries correctly
        set (ALTERNATE_COMMAND CMAKE_COMMAND /usr/bin/env -i PATH=$ENV{PATH} CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND} BUILD_COMMAND bash -c "sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' CMakeScripts/install_postBuildPhase.make*")
    else ()
        set (ALTERNATE_COMMAND CMAKE_COMMAND ${CMAKE_COMMAND} -E env CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND})
    endif ()
    if (URHO3D_PACKAGING)
        ExternalProject_Add (PackageTool
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/Source/Tools/PackageTool
            CMAKE_ARGS -D URHO3D_DEPLOYMENT_TARGET=generic -D DEST_RUNTIME_DIR=${CMAKE_BINARY_DIR}/bin/tool -D BAKED_CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -D BAKED_CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -D CMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            ${ALTERNATE_COMMAND})
        add_make_clean_files (${CMAKE_BINARY_DIR}/bin/tool/PackageTool)
        if (CMAKE_HOST_WIN32 AND NOT HAS_MKLINK)
            add_dependencies (PackageTool Urho3D)   # Ensure Urho3D headers are fresh when building PackageTool externally on Windows host system without MKLINK
        endif ()
        install (PROGRAMS ${CMAKE_BINARY_DIR}/bin/tool/PackageTool DESTINATION ${DEST_RUNTIME_DIR}/tool)
    endif ()
endif ()

# Папка для экзешников и динамических библиотек
dv_set_bin_dir(${CMAKE_BINARY_DIR}/bin/tool)

if (URHO3D_TOOLS)
    # Urho3D tools
    add_subdirectory(ogre_importer)
    add_subdirectory(PackageTool)
    add_subdirectory(ramp_generator)
    add_subdirectory(sprite_packer)
    add_subdirectory(tests)
elseif (NOT CMAKE_CROSSCOMPILING AND URHO3D_PACKAGING)
    # PackageTool target is required but we are not cross-compiling, so build it as per normal
    add_subdirectory(PackageTool)
endif ()
