# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Set CMake minimum version
cmake_minimum_required (VERSION 3.15)

# Set project name
project (Urho3D)

# Предупреждаем об in-source build
if(CMAKE_BINARY_DIR MATCHES "^${CMAKE_SOURCE_DIR}")
    message(WARNING "Генерировать проекты в папке с иходниками - плохая идея")
endif()

# Определяем, подключён ли движок как поддиректория
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(dviglo_subproject OFF)
else ()
  set(dviglo_subproject ON)
endif ()

# Show command line options
get_cmake_property (CACHE_VARS CACHE_VARIABLES)
foreach (CACHE_VAR ${CACHE_VARS})
  get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
  if (CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
    get_property(CACHE_VAR_TYPE CACHE ${CACHE_VAR} PROPERTY TYPE)
    if (CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
      set (CACHE_VAR_TYPE)
    else ()
      set (CACHE_VAR_TYPE ":${CACHE_VAR_TYPE}")
    endif()
    set (CMAKE_ARGS "${CMAKE_ARGS} -D ${CACHE_VAR}${CACHE_VAR_TYPE}=\"${${CACHE_VAR}}\"")
  endif ()
endforeach ()
message ("!! CMAKE_ARGS: ${CMAKE_ARGS}")

# Включаем поддержку папок в IDE
set_property (GLOBAL PROPERTY USE_FOLDERS ON)

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

# Include UrhoCommon.cmake module after setting project name
include (UrhoCommon)

include(cmake/utils.cmake)

# Папка для экзешников и динамических библиотек
dv_set_bin_dir(${CMAKE_BINARY_DIR}/bin)

# Копируем d3dcompiler_xx.dll в папку bin. Этот файл есть в Windows SDK,
# но отсутствует в DirectX redist, поэтому его надо распространять вместе с игрой
if (WIN32)
    set(DIRECTX_REQUIRED_COMPONENTS D3D11)

    find_package (DirectX REQUIRED ${DIRECTX_REQUIRED_COMPONENTS})
    if (DIRECTX_FOUND)
        if(DIRECT3D_DLL AND URHO3D_D3D11)
            file(COPY ${DIRECT3D_DLL} DESTINATION ${CMAKE_BINARY_DIR}/bin)
            file(COPY ${DIRECT3D_DLL} DESTINATION ${CMAKE_BINARY_DIR}/bin/tool)
        endif()
    endif ()
endif ()

# Setup SDK install destinations
set (PATH_SUFFIX Urho3D)
if (WIN32)
    set (SCRIPT_EXT .bat)
    if (CMAKE_HOST_WIN32)
        set (PATH_SUFFIX .)
        if (URHO3D_64BIT AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
            string (REPLACE " (x86)" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
        endif ()
    endif ()
else ()
    set (SCRIPT_EXT .sh)
endif ()
if (URHO3D_64BIT)
    # Install to 'lib64' when one of these conditions is true
    if ((MINGW AND CMAKE_CROSSCOMPILING) OR URHO3D_USE_LIB64_RPM OR (HAS_LIB64 AND NOT URHO3D_USE_LIB_DEB))
        set (LIB_SUFFIX 64)
    endif ()
endif ()
set (DEST_INCLUDE_DIR include/dviglo)    # The include directory path contains the 'Urho3D' suffix regardless of PATH_SUFFIX variable
set (DEST_SHARE_DIR share/${PATH_SUFFIX})
set (DEST_BUNDLE_DIR ${DEST_SHARE_DIR}/Applications)
# Note to package maintainer: ${PATH_SUFFIX} for library could be removed if the extra path is not desired, but if so then the RPATH setting in Source's CMakeLists.txt needs to be adjusted accordingly
set (DEST_LIBRARY_DIR lib${LIB_SUFFIX}/${PATH_SUFFIX})
set (DEST_PKGCONFIG_DIR lib${LIB_SUFFIX}/pkgconfig)
# Install application launcher scripts
file (GLOB APP_SCRIPTS ${CMAKE_SOURCE_DIR}/bin/*${SCRIPT_EXT})
#install (PROGRAMS ${APP_SCRIPTS} DESTINATION ${DEST_RUNTIME_DIR})   # DEST_RUNTIME_DIR variable is set by the set_output_directories() macro call in the UrhoCommon module
# Install CMake modules and toolchains provided by and for Urho3D
#install (DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/ DESTINATION ${DEST_SHARE_DIR}/cmake)    # Note: the trailing slash is significant
# Install CMake build scripts and rakefile
file (GLOB CMAKE_SCRIPTS ${CMAKE_SOURCE_DIR}/script/*${SCRIPT_EXT})
#install (PROGRAMS ${CMAKE_SCRIPTS} DESTINATION ${DEST_SHARE_DIR}/scripts)
#install (FILES rakefile DESTINATION ${DEST_SHARE_DIR})

# Setup macOS, iOS, and tvOS bundle variables
if (URHO3D_MACOSX_BUNDLE OR (APPLE AND ARM))
    if (NOT MACOSX_BUNDLE_ICON_FILE)
        set (MACOSX_BUNDLE_ICON_FILE UrhoIcon)
    endif ()
    if (NOT MACOSX_BUNDLE_BUNDLE_VERSION)
        set (MACOSX_BUNDLE_BUNDLE_VERSION ${URHO3D_VERSION})
    endif ()
    if (NOT MACOSX_BUNDLE_LONG_VERSION_STRING)
        set (MACOSX_BUNDLE_LONG_VERSION_STRING ${URHO3D_VERSION})
    endif ()
    if (NOT MACOSX_BUNDLE_SHORT_VERSION_STRING)
        set (MACOSX_BUNDLE_SHORT_VERSION_STRING ${URHO3D_VERSION})
    endif ()
    if (NOT MACOSX_BUNDLE_COPYRIGHT)
        set (MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2008-2023 the Urho3D project.")
    endif ()
endif ()

# Setup SDK-like include dir in the build tree for building the Urho3D library
set (THIRD_PARTY_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)
file (MAKE_DIRECTORY ${THIRD_PARTY_INCLUDE_DIR})

# Urho3D source
add_subdirectory (Source)

# Urho3D documentation
add_subdirectory (Docs)
