# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Define target name
set (TARGET_NAME Urho3D)

if (MSVC)
    # Set warning level for Urho3D project
    add_compile_options (/W3)
endif ()

# Prevent ICE from old MinGW versions, workaround from the GCC bugzilla https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61189
if (MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.1 AND NOT URHO3D_64BIT)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpreferred-stack-boundary=2")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mpreferred-stack-boundary=2")
endif ()

check_include_files (stdint.h HAVE_STDINT_H)
if (HAVE_STDINT_H)
  add_definitions (-DHAVE_STDINT_H)
endif ()

# Generate JSON compilation database format specification
if (URHO3D_CLANG_TOOLS OR URHO3D_BINDINGS)
    set (CMAKE_EXPORT_COMPILE_COMMANDS 1)
    # Clang assumes all their tools to be installed in a relative path to their libs but this is not the case for our own Clang-tools
    # Clang has a hard-coded way to search for their headers (see http://clang.llvm.org/docs/LibTooling.html#builtin-includes)
    # In order for our Clang-tools to work correctly we need to make this Clang's assumption to be true
    execute_process (COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string (REGEX REPLACE "([.0123456789]+).*" \\1 LLVM_VERSION "${LLVM_VERSION}")      # Stringify against empty output variable
    execute_process (COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process (COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/tool/lib/clang/${LLVM_VERSION})
    create_symlink (${LLVM_LIBDIR}/clang/${LLVM_VERSION}/include ${CMAKE_BINARY_DIR}/bin/tool/lib/clang/${LLVM_VERSION}/include FALLBACK_TO_COPY)
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
    set (CMAKE_DEBUG_POSTFIX _d)
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
foreach (DIR IK Navigation Network Physics Physics2D Urho2D WebP)
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

# Define generated source files
set (URHO3D_DEPS ${STATIC_LIBRARY_TARGETS})
if (TARGET LuaJIT_universal)
    list (REMOVE_ITEM URHO3D_DEPS LuaJIT)
    set (LuaJIT_universal_symbolic LuaJIT_universal_symbolic)
    add_custom_command (OUTPUT ${LuaJIT_universal_symbolic}
        COMMAND xcodebuild ARCHS="$(ARCHS)" -target LuaJIT_universal -configuration $(CONFIGURATION)
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    list (APPEND SYMBOLIC_SOURCES ${LuaJIT_universal_symbolic})
endif ()
set_source_files_properties (${SYMBOLIC_SOURCES} PROPERTIES SYMBOLIC TRUE)

# Use the AutoBinder tool to generate source files for binding script subsystems
if (URHO3D_BINDINGS)
    # Build the Clang-tools as external project (even when we are not cross-compiling because it needs C++11 standard on) for auto-binding generation
    include (ExternalProject)
    if (IOS OR TVOS)
        # When cross-compiling for iOS/tvOS the host environment has been altered by xcodebuild for the said platform, the following fix is required to reset the host environment before spawning another process to configure/generate project file for external project
        # Also workaround a known CMake/Xcode generator bug which prevents it from installing native tool binaries correctly
        set (ALTERNATE_COMMAND CMAKE_COMMAND /usr/bin/env -i PATH=$ENV{PATH} CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND} BUILD_COMMAND bash -c "sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' CMakeScripts/install_postBuildPhase.make*")
    else ()
        set (ALTERNATE_COMMAND CMAKE_COMMAND ${CMAKE_COMMAND} -E env CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND})
    endif ()
    ExternalProject_Add (AutoBinder
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/Source/Clang-Tools
        CMAKE_ARGS -D URHO3D_CLANG_TOOLS=AutoBinder -D URHO3D_DEPLOYMENT_TARGET=generic -D DEST_RUNTIME_DIR=${CMAKE_BINARY_DIR}/bin/tool/clang -D DEST_INCLUDE_DIR=${DEST_INCLUDE_DIR} -D BAKED_CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -D CMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        ${ALTERNATE_COMMAND})
    add_make_clean_files (${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder)
    file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/generated)
    foreach (SCRIPT AngelScript LuaScript JavaScript)
        string (TOUPPER URHO3D_${SCRIPT} OPT)
        if (${OPT})
            list (APPEND SCRIPT_SUBSYSTEMS -s ${SCRIPT})
            list (APPEND GEN_CPP_FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/${SCRIPT}API.cpp)
        endif ()
    endforeach ()
    string (REGEX REPLACE "[^;]+API\\.cpp" "" ANNOTATED_SOURCES "${CPP_FILES}")    # Stringify to preserve the semicolons
    add_custom_command (OUTPUT ${GEN_CPP_FILES}
        COMMAND ${CMAKE_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_BINARY_DIR} -t ${CMAKE_BINARY_DIR}/bin/tool/clang/Templates -o ${CMAKE_CURRENT_BINARY_DIR}/generated ${SCRIPT_SUBSYSTEMS} ${ANNOTATED_SOURCES}
        COMMAND ${CMAKE_COMMAND} -E touch ${GEN_CPP_FILES}   # TODO: fake the output - WIP
        DEPENDS AutoBinder ${H_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating source files for binding script subsystems")
endif ()
if (URHO3D_LUA)
    # The host tool must be built natively
    if (CMAKE_CROSSCOMPILING OR URHO3D_LUAJIT)
        check_native_compiler_exist ()
        # When cross-compiling or using LuaJIT, build the tolua++ host tool as external project using normal Lua (there is not much point using LuaJIT for the tool building even when technically it can)
        include (ExternalProject)
        if (ANDROID AND DEFINED ENV{TRAVIS})
            set (TRAVIS_ANDROID_FIX -DURHO3D_DEPLOYMENT_TARGET=generic)
        endif ()
        if (IOS OR TVOS)
            set (ALTERNATE_COMMAND CMAKE_COMMAND /usr/bin/env -i PATH=$ENV{PATH} CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND} BUILD_COMMAND bash -c "sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' CMakeScripts/install_postBuildPhase.make*")
        else ()
            set (ALTERNATE_COMMAND CMAKE_COMMAND ${CMAKE_COMMAND} -E env CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND})
        endif ()
        ExternalProject_Add (tolua++
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/Source/ThirdParty/toluapp/src/bin
            CMAKE_ARGS -D URHO3D_DEPLOYMENT_TARGET=generic -D DEST_RUNTIME_DIR=${CMAKE_BINARY_DIR}/bin/tool -D BAKED_CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -D URHO3D_UPDATE_SOURCE_TREE=${URHO3D_UPDATE_SOURCE_TREE} -D CMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            ${TRAVIS_ANDROID_FIX}
            ${ALTERNATE_COMMAND})
        add_make_clean_files (${CMAKE_BINARY_DIR}/bin/tool/tolua++)
    else ()
        # Otherwise, build it internally as per normal
        add_subdirectory (../ThirdParty/toluapp/src/bin ../ThirdParty/toluapp/src/bin)
    endif ()
    # Use the host tool to generate source files for tolua++ API binding
    file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/LuaScript/generated)
    file (GLOB API_PKG_FILES LuaScript/pkgs/*.pkg)
    foreach (DIR IK Navigation Network Database Physics Physics2D Urho2D)
        string (TOUPPER URHO3D_${DIR} OPT)
        if (NOT ${OPT})
            list (REMOVE_ITEM API_PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/LuaScript/pkgs/${DIR}LuaAPI.pkg)
        endif ()
    endforeach ()
    foreach (API_PKG_FILE ${API_PKG_FILES})
        get_filename_component (NAME ${API_PKG_FILE} NAME)
        string (REGEX REPLACE LuaAPI\\.pkg$ "" DIR ${NAME})
        set (GEN_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/LuaScript/generated/${DIR}LuaAPI.cpp)
        list (APPEND LUA_GEN_CPP_FILES ${GEN_CPP_FILE})
        file (GLOB PKG_FILES LuaScript/pkgs/${DIR}/*.pkg)
        add_custom_command (OUTPUT ${GEN_CPP_FILE}
            COMMAND ${CMAKE_BINARY_DIR}/bin/tool/tolua++ -E ${CMAKE_PROJECT_NAME} -L ToCppHook.lua -o ${GEN_CPP_FILE} ${NAME}
            DEPENDS tolua++ ${API_PKG_FILE} ${PKG_FILES} LuaScript/pkgs/ToCppHook.lua
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/LuaScript/pkgs
            COMMENT "Generating tolua++ API binding on the fly for ${DIR}")
    endforeach ()
    source_group ("Source Files\\LuaScript\\Generated" FILES ${LUA_GEN_CPP_FILES})
    if (URHO3D_LINT)
        execute_process (COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/.clang-tidy ${CMAKE_CURRENT_BINARY_DIR}/LuaScript/generated)
    endif ()
endif ()

# Aggregate all source files
list (APPEND SOURCE_FILES ${GEN_CPP_FILES} ${LUA_GEN_CPP_FILES} dviglo.h ${SYMBOLIC_SOURCES})

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
    list (APPEND INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIR}/Detour)
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
if (URHO3D_CLANG_TOOLS)
    set (ANNOTATE_NONSCRIPTABLE "__attribute__((annotate(\"nonscriptable\")))")
endif ()
set (APPENDIX "${APPENDIX}\n#define NONSCRIPTABLE ${ANNOTATE_NONSCRIPTABLE}\n\n")
foreach (DEFINE URHO3D_STATIC_DEFINE URHO3D_OPENGL URHO3D_D3D11 URHO3D_SSE URHO3D_DATABASE_ODBC URHO3D_DATABASE_SQLITE URHO3D_LUAJIT URHO3D_TESTING CLANG_PRE_STANDARD)
    if (${DEFINE})
        set (APPENDIX "${APPENDIX}#define ${DEFINE}\n")
    endif ()
endforeach()
if (URHO3D_FORCE_AS_MAX_PORTABILITY)
    set (APPENDIX "${APPENDIX}#define AS_MAX_PORTABILITY\n")
endif ()
generate_export_header (${TARGET_NAME} EXPORT_MACRO_NAME URHO3D_API EXPORT_FILE_NAME dviglo.h.new CUSTOM_CONTENT_FROM_VARIABLE APPENDIX)
set (COPYRIGHT "// Copyright (c) 2008-2023 the Urho3D project\n// License: MIT\n")
file (READ ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new EXPORT_HEADER)
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new ${COPYRIGHT}${EXPORT_HEADER})
execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h)
file (REMOVE ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h.new)
source_group ("Source Files\\Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/dviglo.h)

# Precompile the header after the export header file has been generated
enable_pch (precompiled.h)

if (NOT ANDROID AND NOT WEB)
    file (READ .soversion SOVERSION)
    string (STRIP ${SOVERSION} SOVERSION)
    string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED ${SOVERSION})
    if (MATCHED)
        set_target_properties (${TARGET_NAME} PROPERTIES VERSION ${MATCHED} SOVERSION ${CMAKE_MATCH_1})
    else ()
        message (FATAL_ERROR "The .soversion file is corrupted. It should contain a version number with this format major(0xFFFF).minor(0xFF).patch-level(0xFF). e.g.: 0.1.2")
    endif ()
else ()
    set_target_properties (${TARGET_NAME} PROPERTIES NO_SONAME TRUE)
endif ()
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
    recast detour detour_crowd detour_tile_cache box2d lz4 pugixml webp freetype
    stanhull civetweb slikenet ik)

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
set (ENGINE_INCLUDE_DIRS "${DASH}I\"\${includedir}\" ${DASH}I\"\${includedir}/Urho3D/ThirdParty\"")
if (URHO3D_PHYSICS)
    # Bullet library depends on its own include dir to be added in the header search path
    # This is more practical than patching its header files in many places to make them work with relative path
    set (ENGINE_INCLUDE_DIRS "${ENGINE_INCLUDE_DIRS} ${DASH}I\"\${includedir}/Urho3D/ThirdParty/Bullet\"")
endif ()
if (URHO3D_LUA)
    # ditto for Lua/LuaJIT
    set (ENGINE_INCLUDE_DIRS "${ENGINE_INCLUDE_DIRS} ${DASH}I\"\${includedir}/Urho3D/ThirdParty/Lua${JIT}\"")
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
if (URHO3D_SCP_TO_TARGET)
    # Ensure SCP is the last command
    add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND scp $<TARGET_FILE:${TARGET_NAME}> ${URHO3D_SCP_TO_TARGET} || exit 0
        COMMENT "Scp-ing Urho3D library to target system")
endif ()

if (URHO3D_FORCE_AS_MAX_PORTABILITY)
    add_definitions (-DAS_MAX_PORTABILITY)
endif ()
