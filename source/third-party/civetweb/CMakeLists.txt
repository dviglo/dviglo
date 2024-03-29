# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) the Dviglo project
# License: MIT

# Название таргета
set(TARGET_NAME civetweb)

# Создаём список файлов
file(GLOB_RECURSE source_files civetweb/*.c civetweb/*.h)

# Создаём статическую библиотеку
add_library(${TARGET_NAME} STATIC ${source_files})

# Define preprocessor macros
# Always disable Common Gateway Interface
add_definitions(-DNO_CGI -DNO_FILES -DNO_CACHING)

# Allow debug output to console
# add_definitions(-DDEBUG)

if (WIN32)
    include (CheckStructHasMember)
    check_struct_has_member (struct\ timespec tv_sec time.h HAVE_STRUCT_TIMESPEC_TV_SEC)
    if (HAVE_STRUCT_TIMESPEC_TV_SEC)
        include (CheckSymbolExists)
        check_symbol_exists (_TIMESPEC_DEFINED time.h HAVE__TIMESPEC_DEFINED)
        if (NOT HAVE__TIMESPEC_DEFINED)
            add_definitions (-D_TIMESPEC_DEFINED)
        endif ()
    endif ()
elseif (APPLE AND (NOT IOS OR IPHONEOS_DEPLOYMENT_TARGET STREQUAL "" OR NOT IPHONEOS_DEPLOYMENT_TARGET VERSION_LESS 10.0))    # We cheat a little bit here with a hard-coded check because iOS 10.3 SDK when targeting 9.3 has strong symbol for iPhoneOS archs but weak symbol for iPhoneSimulator archs
    include (CheckLibraryExists)
    check_library_exists (c clock_gettime "" FOUND_CLOCK_GETTIME_IN_C)
    if (FOUND_CLOCK_GETTIME_IN_C)
        add_definitions (-DHAVE_CLOCK_GETTIME)
    endif ()
endif ()

if (NOT DV_SSL)
    add_definitions(-DNO_SSL)
elseif (NOT DV_SSL_DYNAMIC)
    add_definitions(-DNO_SSL_DL)
endif ()

# Link OpenSSL libraries
if (DV_SSL)
    if (DV_SSL_DYNAMIC)
        # Unix systems can define the dynamic library names to load
        if (UNIX AND NOT DARWIN)
            add_definitions(-DCRYPTO_LIB="libcrypto.so")
            add_definitions(-DSSL_LIB="libssl.so")
        endif ()
        find_library(LIBDL_LIBRARIES dl)
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(LibDl DEFAULT_MSG LIBDL_LIBRARIES LIBDL_INCLUDE_DIRS)
        mark_as_advanced(LIBDL_INCLUDE_DIRS LIBDL_LIBRARIES)
        find_path(LIBDL_INCLUDE_DIRS
            NAMES dlfcn.h
            PATHS ${LIBDL_ROOT}/include/
        )
        if (LIBDL_FOUND)
            if (NOT TARGET LIBDL::LIBDL)
                add_library(LIBDL::LIBDL UNKNOWN IMPORTED)
                set_target_properties(LIBDL::LIBDL PROPERTIES
                    IMPORTED_LOCATION "${LIBDL_LIBRARIES}"
                    INTERFACE_INCLUDE_DIRECTORIES "${LIBDL_INCLUDE_DIRS}"
                )
            endif ()
        endif ()
        if (LIBDL_FOUND)
            target_link_libraries(${TARGET_NAME} -ldl)
        endif ()
    else ()
        if (APPLE)
            set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
        endif ()
        find_package(OpenSSL REQUIRED)
        include_directories(${OPENSSL_INCLUDE_DIR})
        message(STATUS "OpenSSL include directory: ${OPENSSL_INCLUDE_DIR}")
        add_definitions(-DOPENSSL_API_1_1)
    endif ()
endif ()

# Делаем заголовочные файлы доступными таргетам, которые используют текущую библиотеку
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Нужно для компиляции динамической версии движка в GCC и Clang (добавляет -fPIC)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

if (MINGW)
    # Требуется для компиляции движка в MinGW
    target_link_libraries(${TARGET_NAME} PRIVATE ws2_32)
endif ()

# Заставляем VS отображать дерево каталогов
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/civetweb FILES ${source_files})
