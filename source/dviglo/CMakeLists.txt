# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Define target name
set (TARGET_NAME Urho3D)

# Отладочная версия библиотеки будут иметь суффикс _d.
# Не работает для экзешников
set(CMAKE_DEBUG_POSTFIX _d)

if (MSVC)
    # Set warning level for Urho3D project
    add_compile_options (/W3)
endif ()

check_include_files (stdint.h HAVE_STDINT_H)
if (HAVE_STDINT_H)
  add_definitions (-DHAVE_STDINT_H)
endif ()

# Save keep the preprocessor macros (for using the Urho3D library) for later use in generating dviglo.pc file
get_directory_property (URHO3D_COMPILE_DEFINITIONS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)

# Define preprocessor macros (for building the Urho3D library)
add_definitions (-DURHO3D_IS_BUILDING)
if (ANDROID AND DEFINED ENV{ASSET_DIR_INDICATOR})
    add_definitions (-DASSET_DIR_INDICATOR=\"$ENV{ASSET_DIR_INDICATOR}\")
endif ()
if ($ENV{COVERITY_SCAN_BRANCH})
    # This macro is used when scanning Urho3D library via Coverity-Scan
    # TODO: The Coverity-Scan modelling is not yet working properly (anyone interested in static analyzer is welcome to give it another try)
    add_definitions (-DCOVERITY_SCAN_MODEL)
endif ()
if (HAVE_SINCOSF)
    add_definitions (-DHAVE_SINCOSF)
elseif (HAVE___SINCOSF)
    add_definitions (-DHAVE___SINCOSF)
endif ()
# Enable/disable SIMD instruction set for STB image
if (NEON)
    if (XCODE)
        if (IOS)
            set (SDK iphoneos)
        elseif (TVOS)
            set (SDK appletvos)
        endif ()
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CFLAGS[sdk=${SDK}*] "-DSTBI_NEON $(OTHER_CFLAGS)")
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS[sdk=${SDK}*] "-DSTBI_NEON $(OTHER_CPLUSPLUSFLAGS)")
    else ()
        add_definitions (-DSTBI_NEON)
    endif ()
elseif (MSVC AND NOT URHO3D_SSE)
    add_definitions (-DSTBI_NO_SIMD)
elseif (MINGW AND NOT URHO3D_64BIT AND URHO3D_SSE)
    add_definitions (-DSTBI_MINGW_ENABLE_SSE2)
endif ()
if (URHO3D_LUA)
    if (NOT URHO3D_SAFE_LUA)
        add_definitions (-DTOLUA_RELEASE)
    endif ()
    if (URHO3D_LUA_RAW_SCRIPT_LOADER)
        add_definitions (-DURHO3D_LUA_RAW_SCRIPT_LOADER)
    endif ()
endif ()
if (ODBC_VERSION AND NOT ODBC_VERSION VERSION_LESS 3)
    add_definitions (-DODBC_3_OR_LATER)
endif ()
if (WIN32)
    if (NOT MINGW OR HAVE_RTL_OSVERSIONINFOW)
        add_definitions (-DHAVE_RTL_OSVERSIONINFOW)
    endif ()
endif ()
if (URHO3D_HASH_DEBUG)
    add_definitions (-DURHO3D_HASH_DEBUG)
endif ()

if (URHO3D_SSL)
    add_definitions (-DURHO3D_SSL)
    if (URHO3D_SSL_DYNAMIC)
        add_definitions (-DURHO3D_SSL_DYNAMIC)
    endif ()
endif ()

# Define source files
foreach (DIR IK Navigation Network Physics Physics2D Urho2D)
    string (TOUPPER URHO3D_${DIR} OPT)
    if (NOT ${OPT})
        list (APPEND EXCLUDED_SOURCE_DIRS ${DIR})
    endif ()
endforeach ()
if (NOT URHO3D_ANGELSCRIPT)
    list (APPEND EXCLUDED_SOURCE_DIRS AngelScript)
endif ()
if (NOT URHO3D_LUA)
    list (APPEND EXCLUDED_SOURCE_DIRS LuaScript)
endif ()
if (URHO3D_DATABASE)
    if (URHO3D_DATABASE_ODBC)
        list (APPEND EXCLUDED_SOURCE_DIRS database/sqite)
    else ()
        list (APPEND EXCLUDED_SOURCE_DIRS database/odbc)
    endif ()
else ()
    list (APPEND EXCLUDED_SOURCE_DIRS database)
endif ()

if (NOT URHO3D_OPENGL)
    list (APPEND EXCLUDED_SOURCE_DIRS graphics_api/opengl)
endif ()

if (NOT URHO3D_D3D11)
    list (APPEND EXCLUDED_SOURCE_DIRS graphics_api/direct3d11)
endif ()

if (APPLE AND NOT ARM)
    set (GLOB_OBJC_PATTERN *.m)     # Should only pick up MacFileWatcher.m for macOS platform at the moment
endif ()
string (REPLACE ";" "/[^;]+;" EXCLUDE_PATTERNS "${EXCLUDED_SOURCE_DIRS};")
define_source_files (EXCLUDE_PATTERNS ${EXCLUDE_PATTERNS} GLOB_CPP_PATTERNS *.cpp ${GLOB_OBJC_PATTERN} RECURSE GROUP)

# Aggregate all source files
list (APPEND SOURCE_FILES ${GEN_CPP_FILES} ${LUA_GEN_CPP_FILES} dviglo.h)

# Define dependency libs
# Add include directories to find the precompiled header, export header, and installed headers from third-party libs
set (INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${THIRD_PARTY_INCLUDE_DIR})
if (URHO3D_PHYSICS)
    # Bullet library depends on its own include dir to be added in the header search path
    # This is more practical than patching its header files in many places to make them work with relative path
    list (APPEND INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIR}/Bullet)
endif ()
if (URHO3D_NAVIGATION)
    # DetourTileCache and DetourCrowd libraries depend on Detour's include dir to be added in the header search path
    list (APPEND INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIR}/detour)
endif ()
if (URHO3D_LUA)
    # Lua/LuaJIT library depends on its own include dir to be added in the header search path
    # This is more practical than patching its header files in many places to make them work with relative path
    list (APPEND INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIR}/Lua${JIT})
endif ()
# Workaround for GCC 5.4 and above when building a SHARED lib type for Linux platform to fix the undefined symbol "__cpu_model" issue (see #1519)
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.3.1 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND URHO3D_LIB_TYPE STREQUAL SHARED)  # 5.3.1 was the last known good version
    list (APPEND LIBS gcc)
endif ()

# Setup library output path
set_output_directories (${CMAKE_BINARY_DIR}/lib ARCHIVE LIBRARY)

# Setup target
if (IOS OR TVOS)
    set (EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)
endif ()
setup_library (${URHO3D_LIB_TYPE} ${EXCLUDE_FROM_ALL})

# Generate platform specific export header file (can only be called after the target has been added)
if (MSVC)
    set (APPENDIX "\n#pragma warning(disable: 4251)\n#pragma warning(disable: 4275)\n\n#if _MSC_VER < 1900\n#define strtoll _strtoi64\n#define strtoull _strtoui64\n#endif\n")
endif ()
set (APPENDIX "${APPENDIX}\n#define NONSCRIPTABLE ${ANNOTATE_NONSCRIPTABLE}\n\n")
foreach (DEFINE URHO3D_STATIC_DEFINE URHO3D_OPENGL URHO3D_D3D11 URHO3D_SSE URHO3D_DATABASE_ODBC URHO3D_DATABASE_SQLITE URHO3D_LUAJIT URHO3D_TESTING CLANG_PRE_STANDARD)
    if (${DEFINE})
        set (APPENDIX "${APPENDIX}#define ${DEFINE}\n")
    endif ()
endforeach()
generate_export_header (${TARGET_NAME} EXPORT_MACRO_NAME URHO3D_API EXPORT_FILE_NAME dviglo.h.new CUSTOM_CONTENT_FROM_VARIABLE APPENDIX)
set (COPYRIGHT "// Copyright (c) 2008-2023 the Urho3D project\n// License: MIT\n")
file (READ ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new EXPORT_HEADER)
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new ${COPYRIGHT}${EXPORT_HEADER})
execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h)
file (REMOVE ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new)
source_group ("Source Files\\Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h)

# TODO: to use 'xcframework' as Apple discourages the use of 'lipo' to create universal binary combining libs/frameworks from different SDK
# As of Xcode 12 the 'iphonesimulator' SDK produces binary for both 'x86_64' and 'arm64' archs
# This has caused problem with 'lipo' to combine the already FAT lib from 'iphonesimulator' SDK with 'iphoneos' SDK containing 'arm64' arch
# Temporary workaround by not generating the universal binary when the issue is detected
if (IOS)
    # Add a custom target to build Mach-O universal binary consisting of both iPhoneOS and iPhoneSimulator SDKs
    add_custom_target (${TARGET_NAME}_universal ALL
        COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]\; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'\; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both iphoneos and iphonesimulator SDKs\; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |egrep -cq x86_64\; then sdk=iphoneos\; else sdk=iphonesimulator\; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.$$sdk} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.$$sdk 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'\; then echo 'Skip on Xcode 12 due to arch conflict' && mv $<TARGET_FILE:${TARGET_NAME}>{.ori,} && rm $<TARGET_FILE:${TARGET_NAME}>.$$sdk\; else lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.$$sdk} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.$$sdk}\; fi\; fi\; fi
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating Mach-O universal binary library consisting of both iPhoneOS and iPhoneSimulator SDks")
    install (FILES $<TARGET_FILE:${TARGET_NAME}> DESTINATION ${DEST_LIBRARY_DIR})
elseif (TVOS)
    # Add a custom target to build Mach-O universal binary consisting of both AppleTVOS and AppleTVSimulator SDKs
    add_custom_target (${TARGET_NAME}_universal ALL
        COMMAND if [ '$(CONFIGURATION)' != 'Debug' ]\; then if lipo -info $<TARGET_FILE:${TARGET_NAME}> 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'\; then echo $<TARGET_FILE:${TARGET_NAME}> is already a Mach-O universal binary consisting of both AppleTVOS and AppleTVSimulator SDKs\; else mv $<TARGET_FILE:${TARGET_NAME}>{,.ori} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.ori 2>/dev/null |grep -cq x86_64\; then sdk=appletvos\; else sdk=appletvsimulator\; fi && xcodebuild -target ${TARGET_NAME} -configuration $(CONFIGURATION) -sdk $$sdk 2>&1 && mv $<TARGET_FILE:${TARGET_NAME}>{,.$$sdk} && if lipo -info $<TARGET_FILE:${TARGET_NAME}>.$$sdk 2>/dev/null |egrep -cq 'x86_64.*arm64|arm64.*x86_64'\; then echo 'Skip on Xcode 12 due to arch conflict' && mv $<TARGET_FILE:${TARGET_NAME}>{.ori,} && rm $<TARGET_FILE:${TARGET_NAME}>.$$sdk\; else lipo -create -output $<TARGET_FILE:${TARGET_NAME}>{,.ori,.$$sdk} && rm $<TARGET_FILE:${TARGET_NAME}>{.ori,.$$sdk}\; fi\; fi\; fi
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating Mach-O universal binary library consisting of both AppleTVOS and AppleTVSimulator SDKs")
    install (FILES $<TARGET_FILE:${TARGET_NAME}> DESTINATION ${DEST_LIBRARY_DIR})
    # TODO: Temporary workaround as CMake does not recognize AppleTV platform yet
    add_dependencies (${TARGET_NAME} APPLETV_POST_CMAKE_FIX)
elseif (EMSCRIPTEN)
    install (TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
else ()
    install (TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${DEST_RUNTIME_DIR} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
endif ()
if (WIN32 AND URHO3D_LIB_TYPE STREQUAL SHARED)
    set (URHO3D_DLL_INSTALLED TRUE)
endif ()

# TODO: This seems out of place
if (MINGW)
    set (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES_INIT} -lwinmm")
    set (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES_INIT} -lwinmm")
    if (URHO3D_LIB_TYPE STREQUAL SHARED)
        set_target_properties (${TARGET_NAME} PROPERTIES PREFIX "")
    endif ()
endif ()

target_link_libraries(${TARGET_NAME} PRIVATE ${STATIC_LIBRARY_TARGETS})

# TODO: Либы подключаются без учёта дефайнов
target_link_libraries(${TARGET_NAME} PUBLIC SDL3-static bullet glew stb rapidjson
    recast detour detour_crowd detour_tile_cache box2d lz4 pugixml freetype
    stanhull civetweb slikenet ik etcpack)

if (URHO3D_TRACY_PROFILING)
    target_link_libraries(${TARGET_NAME} PUBLIC tracy)
endif ()

# Не линкуем LibCpuId при сборке для Linux
if (NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(${TARGET_NAME} PUBLIC libcpuid)
endif ()

# Делаем h-файлы доступными приложениям, которые подключили движок как поддиректорию
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)

# Создаём список h-файлов
file(GLOB_RECURSE dviglo_headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)
# Обрамляем названия файлов
list(TRANSFORM dviglo_headers REPLACE "(.+)" "#include <dviglo/\\1>")
# Сортируем список h-файлов в реверсном порядке
list(SORT dviglo_headers ORDER DESCENDING)

# Опциональные папки, которые могут быть отключены дефайнами
# TODO: Сделать их отдельными модулями?
set (OPTIONAL_SUBS database ik navigation network physics physics_2d urho_2d)

foreach (SUB ${OPTIONAL_SUBS})
    if (dviglo_headers MATCHES "(#include <dviglo/${SUB}/[^;]+>)")
        # Так как список h-файлов был отсортирован в реверсном порядке, то первый
        # найденный элемент на самом деле последний. Вставляем перед ним #endif
        list (FIND dviglo_headers ${CMAKE_MATCH_1} FOUND_INDEX)
        list (INSERT dviglo_headers ${FOUND_INDEX} "#endif")
    endif ()
endforeach ()
# Меняем порядок элементов на правильный
list(REVERSE dviglo_headers)

foreach (SUB ${OPTIONAL_SUBS})
    if (dviglo_headers MATCHES "(#include <dviglo/${SUB}/[^;]+>)")
        string (TOUPPER ${SUB} SUB)

        # Заменяем дефайны для случаев, когда дефайны не совпадают с именами папок
        if ("${SUB}" STREQUAL "PHYSICS_2D")
            set (SUB "PHYSICS2D")
        elseif ("${SUB}" STREQUAL "URHO_2D")
            set (SUB "URHO2D")
        endif ()

        # Добавляем дефайны для h-файлов в опциональных папках
        list (FIND dviglo_headers ${CMAKE_MATCH_1} FOUND_INDEX)
        list (INSERT dviglo_headers ${FOUND_INDEX} "#if URHO3D_${SUB}")
    endif ()
endforeach ()

# Исключаем некоторые h-файлы
string (REGEX REPLACE "#include <[^;]+(debug_new|direct3d|graphics_impl|ik_converters|odbc|opengl|precompiled|sqlite|dviglo|librevision|wrap\.h|aswrappedcall\.h)[^;]*>;" "" dviglo_headers "${dviglo_headers};")

# Заменяем ; на перевод строки
string (REPLACE ";" \n dviglo_headers "${dviglo_headers}")

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/dviglo_all.h.in ${CMAKE_CURRENT_BINARY_DIR}/dviglo_all.h)

# Generate platform specific pkg-config file for the benefit of Urho3D library users via SDK without CMake
if (ABSOLUTE_PATH_LIBS)
    string (REPLACE ";" "\" \"" URHO3D_ABS_PATH_LIBS "\"${ABSOLUTE_PATH_LIBS}\"")   # Note: need to always "stringify" a variable in list context for replace to work correctly, besides the list could be empty
    string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" URHO3D_ABS_PATH_LIBS "${URHO3D_ABS_PATH_LIBS}")
endif ()
if (MSVC)
    set (DASH /)
    set (LIB_DIR "/LIBPATH:\"\${libdir}\"")
    string (REPLACE ";" ".lib\" \"" URHO3D_LIBS "\"@LIB_NAME@\" \"${LIBS}.lib\"")
    set (PC_SUFFIX .msvc)
else ()
    set (LIB_NAME ${TARGET_NAME})
    if (MINGW)
        if (CMAKE_BUILD_TYPE STREQUAL Debug)
            set (LIB_NAME ${LIB_NAME}_d)
        endif ()
        if (URHO3D_LIB_TYPE STREQUAL SHARED)
            set (LIB_NAME ${LIB_NAME}.dll)
        endif ()
    endif ()
    set (DASH -)
    set (LIB_DIR "-L\"\${libdir}\"")
    string (REPLACE ";" " -l" URHO3D_LIBS "-l${LIB_NAME};${LIBS}")
endif ()
string (REPLACE ";" " ${DASH}D" URHO3D_COMPILE_DEFINITIONS "${DASH}D${URHO3D_COMPILE_DEFINITIONS}")
get_directory_property (GLOBAL_INCLUDE_DIRS DIRECTORY ${CMAKE_SOURCE_DIR} INCLUDE_DIRECTORIES)
if (GLOBAL_INCLUDE_DIRS)
    string (REPLACE ";" "\" ${DASH}I\"" GLOBAL_INCLUDE_DIRS "${DASH}I\"${GLOBAL_INCLUDE_DIRS}\"")
    string (REPLACE "${SYSROOT}" "" GLOBAL_INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS})
endif ()
string (REGEX REPLACE " -include \"[^\"]+\"" "" CLEANED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_CXX_FLAGS "${CLEANED_CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
if (ANDROID)
    # After migration to Gradle build system we do not support Android cross-compiling build via pkgconfig anymore, however, this file is still being provided as reference only
    set (RENAME RENAME Urho3D-${ANDROID_ABI}.pc)
endif ()
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/dviglo.pc DESTINATION ${DEST_PKGCONFIG_DIR} ${RENAME})

# Define post build steps
if (URHO3D_LIB_TYPE STREQUAL SHARED AND ARM)
    # Strip the output shared library for embedded ARM platforms (we don't do this for Android anymore, assuming Android plugin will take care of it)
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>
        COMMENT "Stripping Urho3D shared library")
endif ()
