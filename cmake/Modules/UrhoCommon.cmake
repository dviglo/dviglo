# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# Save the initial values of CC and CXX environment variables
if (NOT CMAKE_CROSSCOMPILING)
    set (SAVED_CC $ENV{CC} CACHE INTERNAL "Initial value for CC")
    set (SAVED_CXX $ENV{CXX} CACHE INTERNAL "Initial value for CXX")
endif ()

# Limit the supported build configurations
set (URHO3D_BUILD_CONFIGURATIONS Release RelWithDebInfo Debug)
set (DOC_STRING "Specify CMake build configuration (single-configuration generator only), possible values are Release (default), RelWithDebInfo, and Debug")
if (CMAKE_CONFIGURATION_TYPES)
    # For multi-configurations generator, such as VS and Xcode
    set (CMAKE_CONFIGURATION_TYPES ${URHO3D_BUILD_CONFIGURATIONS} CACHE STRING ${DOC_STRING} FORCE)
    unset (CMAKE_BUILD_TYPE)
else ()
    # For single-configuration generator, such as Unix Makefile generator
    if (CMAKE_BUILD_TYPE STREQUAL "")
        # If not specified then default to Release
        set (CMAKE_BUILD_TYPE Release)
    endif ()
    set (CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING ${DOC_STRING} FORCE)
endif ()

# Define other useful variables not defined by CMake
if (CMAKE_GENERATOR STREQUAL Xcode)
    set (XCODE TRUE)
elseif (CMAKE_GENERATOR STREQUAL Ninja)
    set (NINJA TRUE)
elseif (CMAKE_GENERATOR MATCHES Visual)
    set (VS TRUE)
endif ()

include (CheckHost)
include (CheckCompilerToolchain)

# Define all supported build options
include (CMakeDependentOption)
cmake_dependent_option (IOS "Setup build for iOS platform" FALSE "XCODE" FALSE)
cmake_dependent_option (TVOS "Setup build for tvOS platform" FALSE "XCODE" FALSE)
cmake_dependent_option (URHO3D_64BIT "Enable 64-bit build, the default is set based on the native ABI of the chosen compiler toolchain" "${NATIVE_64BIT}" "NOT MSVC AND NOT ANDROID AND NOT (ARM AND NOT IOS) AND NOT WEB" "${NATIVE_64BIT}")     # Intentionally only enable the option for iOS but not for tvOS as the latter is 64-bit only
#option (URHO3D_ANGELSCRIPT "Enable AngelScript scripting support" TRUE)
unset (URHO3D_ANGELSCRIPT CACHE)
option (URHO3D_IK "Enable inverse kinematics support" TRUE)
#option (URHO3D_LUA "Enable additional Lua scripting support" TRUE)
unset (URHO3D_LUA CACHE)
option (URHO3D_NAVIGATION "Enable navigation support" TRUE)
cmake_dependent_option (URHO3D_NETWORK "Enable networking support" TRUE "NOT WEB" FALSE)
option (URHO3D_PHYSICS "Enable physics support" TRUE)
option (URHO3D_PHYSICS2D "Enable 2D physics support" TRUE)
option (URHO3D_URHO2D "Enable 2D graphics support" TRUE)
option (URHO3D_GLES3 "Enable GLES3" FALSE)
#option (URHO3D_WEBP "Enable WebP support" TRUE)
unset(URHO3D_WEBP CACHE)

if (PROJECT_NAME STREQUAL Urho3D)
    set (URHO3D_LIB_TYPE STATIC CACHE STRING "Specify Urho3D library type, possible values are STATIC (default) and SHARED (not available for Emscripten)")
    # Non-Windows platforms always use OpenGL, the URHO3D_OPENGL variable will always be forced to TRUE, i.e. it is not an option at all
    # Windows platform has URHO3D_OPENGL as an option, MSVC compiler default to FALSE (i.e. prefers Direct3D) while MinGW compiler default to TRUE
    if (MINGW)
        set (DEFAULT_OPENGL TRUE)
    endif ()
    cmake_dependent_option (URHO3D_OPENGL "Use OpenGL (Windows platform only)" "${DEFAULT_OPENGL}" WIN32 TRUE)
    # On Windows platform Direct3D11 can be optionally chosen
    # Using Direct3D11 on non-MSVC compiler may require copying and renaming Microsoft official libraries (.lib to .a), else link failures or non-functioning graphics may result
    cmake_dependent_option (URHO3D_D3D11 "Use Direct3D11 (Windows platform only)" TRUE "WIN32" FALSE)
    if (X86 OR WEB)
        # TODO: Rename URHO3D_SSE to URHO3D_SIMD
        if (MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.1)
            if (NOT DEFINED URHO3D_SSE)     # Only give the warning once during initial configuration
                # Certain MinGW versions fail to compile SSE code. This is the initial guess for known "bad" version range, and can be tightened later
                message (WARNING "Disabling SSE by default due to MinGW version. It is recommended to upgrade to MinGW with GCC >= 4.9.1. "
                    "You can also try to re-enable SSE with CMake option -DURHO3D_SSE=1, but this may result in compile errors.")
            endif ()
            set (URHO3D_DEFAULT_SIMD FALSE)
        else ()
            set (URHO3D_DEFAULT_SIMD ${HAVE_SSE})
        endif ()
        # It is not possible to turn SSE off on 64-bit MSVC and it appears it is also not able to do so safely on 64-bit GCC
        cmake_dependent_option (URHO3D_SSE "Enable SIMD instruction set (32-bit Web and Intel platforms only, including Android on Intel Atom); default to true on Intel and false on Web platform; the effective SSE level could be higher, see also URHO3D_DEPLOYMENT_TARGET and CMAKE_OSX_DEPLOYMENT_TARGET build options" "${URHO3D_DEFAULT_SIMD}" "NOT URHO3D_64BIT" TRUE)
    endif ()
    cmake_dependent_option (URHO3D_HASH_DEBUG "Enable StringHash reversing and hash collision detection at the expense of memory and performance penalty" FALSE "NOT CMAKE_BUILD_TYPE STREQUAL Release" FALSE)
    cmake_dependent_option (URHO3D_3DNOW "Enable 3DNow! instruction set (Linux platform only); should only be used for older CPU with (legacy) 3DNow! support" "${HAVE_3DNOW}" "X86 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND NOT URHO3D_SSE" FALSE)
    cmake_dependent_option (URHO3D_MMX "Enable MMX instruction set (32-bit Linux platform only); the MMX is effectively enabled when 3DNow! or SSE is enabled; should only be used for older CPU with MMX support" "${HAVE_MMX}" "X86 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND NOT URHO3D_64BIT AND NOT URHO3D_SSE AND NOT URHO3D_3DNOW" FALSE)
    #cmake_dependent_option (URHO3D_LUAJIT "Enable Lua scripting support using LuaJIT (check LuaJIT's CMakeLists.txt for more options)" TRUE "NOT WEB AND NOT APPLE" FALSE)
    unset (URHO3D_LUAJIT CACHE)
    #cmake_dependent_option (URHO3D_LUAJIT_AMALG "Enable LuaJIT amalgamated build (LuaJIT only); default to true when LuaJIT is enabled" TRUE URHO3D_LUAJIT FALSE)
    unset (URHO3D_LUAJIT_AMALG CACHE)
    #cmake_dependent_option (URHO3D_SAFE_LUA "Enable Lua C++ wrapper safety checks (Lua/LuaJIT only)" FALSE URHO3D_LUA FALSE)
    unset (URHO3D_SAFE_LUA CACHE)
    if (NOT CMAKE_BUILD_TYPE STREQUAL Release AND NOT CMAKE_CONFIGURATION_TYPES)
        set (DEFAULT_LUA_RAW TRUE)
    endif ()
    #cmake_dependent_option (URHO3D_LUA_RAW_SCRIPT_LOADER "Prefer loading raw script files from the file system before falling back on Urho3D resource cache. Useful for debugging (e.g. breakpoints), but less performant (Lua/LuaJIT only)" "${DEFAULT_LUA_RAW}" URHO3D_LUA FALSE)
    unset (URHO3D_LUA_RAW_SCRIPT_LOADER CACHE)
    option (URHO3D_PLAYER "Build Urho3D script player" TRUE)
    option (URHO3D_SAMPLES "Build sample applications" TRUE)
    option(URHO3D_TOOLS "Build tools" TRUE)
    option (URHO3D_DOCS "Generate documentation as part of normal build")
    option (URHO3D_DOCS_QUIET "Generate documentation as part of normal build, suppress generation process from sending anything to stdout")
    #cmake_dependent_option (URHO3D_DATABASE_ODBC "Enable Database support with ODBC, requires vendor-specific ODBC driver" FALSE "NOT IOS AND NOT TVOS AND NOT ANDROID AND NOT WEB;NOT MSVC OR NOT MSVC_VERSION VERSION_LESS 1900" FALSE)
    unset(URHO3D_DATABASE_ODBC CACHE)
    #option (URHO3D_DATABASE_SQLITE "Enable Database support with SQLite embedded")
    unset(URHO3D_DATABASE_SQLITE CACHE)
    # Enable file watcher support for automatic resource reloads by default.
    option (URHO3D_FILEWATCHER "Enable filewatcher support" TRUE)
    option (URHO3D_TESTING "Enable testing support")
    # By default this option is off (i.e. we use the MSVC dynamic runtime), this can be switched on if using Urho3D as a STATIC library
    cmake_dependent_option (URHO3D_STATIC_RUNTIME "Use static C/C++ runtime libraries and eliminate the need for runtime DLLs installation (VS only)" FALSE "MSVC" FALSE)
    if (CPACK_SYSTEM_NAME STREQUAL Linux)
        cmake_dependent_option (URHO3D_USE_LIB64_RPM "Enable 64-bit RPM CPack generator using /usr/lib64 and disable all other generators (Debian-based host only)" FALSE "URHO3D_64BIT AND NOT HAS_LIB64" FALSE)
        cmake_dependent_option (URHO3D_USE_LIB_DEB "Enable 64-bit DEB CPack generator using /usr/lib and disable all other generators (Redhat-based host only)" FALSE "URHO3D_64BIT AND HAS_LIB64" FALSE)
    endif ()
    # Set to search in 'lib' or 'lib64' based on the chosen ABI
    if (NOT CMAKE_HOST_WIN32)
        set_property (GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ${URHO3D_64BIT})
    endif ()
else ()
    set (URHO3D_LIB_TYPE "" CACHE STRING "Specify Urho3D library type, possible values are STATIC (default) and SHARED (not available for Emscripten)")
    set (URHO3D_HOME "" CACHE PATH "Path to Urho3D build tree or SDK installation location (downstream project only)")
    if (URHO3D_SAMPLES OR URHO3D_TOOLS)
        # Just reference it to suppress "unused variable" CMake warning on downstream projects using this CMake module
    endif ()
    if (CMAKE_PROJECT_NAME MATCHES ^Urho3D-ExternalProject-)
        set (URHO3D_SSE ${HAVE_SSE})
    else ()
        # All Urho3D downstream projects require Urho3D library, so find Urho3D library here now
        find_package (Urho3D REQUIRED)
        include_directories (${URHO3D_INCLUDE_DIRS})
    endif ()
endif ()
cmake_dependent_option (URHO3D_PACKAGING "Enable resources packaging support" FALSE "NOT WEB" TRUE)
# Enable profiling by default. If disabled, autoprofileblocks become no-ops and the Profiler subsystem is not instantiated.
option (URHO3D_PROFILING "Enable default profiling support" TRUE)
# Extended "Tracy Profiler" based profiling. Disabled by default.
option (URHO3D_TRACY_PROFILING "Enable extended profiling support using Tracy Profiler; overrides URHO3D_PROFILING option" FALSE)
# Enable logging by default. If disabled, LOGXXXX macros become no-ops and the Log subsystem is not instantiated.
option (URHO3D_LOGGING "Enable logging support" TRUE)
# Enable threading by default, except for Emscripten because its thread support is yet experimental
if (NOT WEB)
    set (THREADING_DEFAULT TRUE)
endif ()
option (URHO3D_THREADING "Enable thread support, on Web platform default to 0, on other platforms default to 1" ${THREADING_DEFAULT})
if (URHO3D_TESTING)
    if (WEB)
        set (DEFAULT_TIMEOUT 10)
        if (EMSCRIPTEN)
            set (EMSCRIPTEN_EMRUN_BROWSER firefox CACHE STRING "Specify the particular browser to be spawned by emrun during testing (Emscripten only), use 'emrun --list_browsers' command to get the list of possible values")
        endif ()
    else ()
        set (DEFAULT_TIMEOUT 5)
    endif ()
    set (URHO3D_TEST_TIMEOUT ${DEFAULT_TIMEOUT} CACHE STRING "Number of seconds to test run the executables (when testing support is enabled only), default to 10 on Web platform and 5 on other platforms")
else ()
    unset (URHO3D_TEST_TIMEOUT CACHE)
    if (EMSCRIPTEN_EMRUN_BROWSER)   # Suppress unused variable warning at the same time
        unset (EMSCRIPTEN_EMRUN_BROWSER CACHE)
    endif ()
endif ()
# Structured exception handling and minidumps on MSVC only
cmake_dependent_option (URHO3D_MINIDUMPS "Enable minidumps on crash (VS only)" TRUE "MSVC" FALSE)
# By default Windows platform setups main executable as Windows application with WinMain() as entry point
cmake_dependent_option (URHO3D_WIN32_CONSOLE "Use console main() instead of WinMain() as entry point when setting up Windows executable targets (Windows platform only)" FALSE "WIN32" FALSE)
cmake_dependent_option (URHO3D_MACOSX_BUNDLE "Use MACOSX_BUNDLE when setting up macOS executable targets (Xcode/macOS platform only)" FALSE "XCODE AND NOT ARM" FALSE)
if (MINGW AND CMAKE_CROSSCOMPILING)
    set (MINGW_PREFIX "" CACHE STRING "Prefix path to MinGW cross-compiler tools (MinGW cross-compiling build only)")
    set (MINGW_SYSROOT "" CACHE PATH "Path to MinGW system root (MinGW only); should only be used when the system root could not be auto-detected")
    # When cross-compiling then we are most probably in Unix-alike host environment which should not have problem to handle long include dirs
    # This change is required to keep ccache happy because it does not like the CMake generated include response file
    foreach (lang C CXX)
        foreach (cat OBJECTS INCLUDES)
            unset (CMAKE_${lang}_USE_RESPONSE_FILE_FOR_${cat})
        endforeach ()
    endforeach ()
endif ()

if (EMSCRIPTEN)     # CMAKE_CROSSCOMPILING is always true for Emscripten
    set (EMSCRIPTEN_SYSROOT "" CACHE PATH "Path to Emscripten system root (Emscripten only)")
    option (EMSCRIPTEN_AUTO_SHELL "Auto adding a default HTML shell-file when it is not explicitly specified (Emscripten only)" TRUE)
    option (EMSCRIPTEN_ALLOW_MEMORY_GROWTH "Enable memory growing based on application demand, default to true as there should be little or no overhead (Emscripten only)" TRUE)
    math (EXPR EMSCRIPTEN_TOTAL_MEMORY "128 * 1024 * 1024")
    set (EMSCRIPTEN_TOTAL_MEMORY ${EMSCRIPTEN_TOTAL_MEMORY} CACHE STRING "Specify the total size of memory to be used (Emscripten only); default to 128 MB, must be in multiple of 64 KB")
    option (EMSCRIPTEN_SHARE_DATA "Enable sharing data file support (Emscripten only)")
else ()
    set (SHARED SHARED)
endif ()
# Constrain the build option values in cmake-gui, if applicable
set_property (CACHE URHO3D_LIB_TYPE PROPERTY STRINGS STATIC ${SHARED})
if (NOT CMAKE_CONFIGURATION_TYPES)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${URHO3D_BUILD_CONFIGURATIONS})
endif ()

if (URHO3D_TRACY_PROFILING)
    set (URHO3D_PROFILING 0)
    unset (URHO3D_PROFILING CACHE)
endif ()

# Union all the sysroot variables into one so it can be referred to generically later
set (SYSROOT ${CMAKE_SYSROOT} ${MINGW_SYSROOT} ${IOS_SYSROOT} ${TVOS_SYSROOT} CACHE INTERNAL "Path to system root of the cross-compiling target")  # SYSROOT is empty for native build

# Enable testing
if (URHO3D_TESTING)
    enable_testing ()
endif ()

# Default library type is STATIC
if (URHO3D_LIB_TYPE)
    string (TOUPPER ${URHO3D_LIB_TYPE} URHO3D_LIB_TYPE)
endif ()
if (NOT URHO3D_LIB_TYPE STREQUAL SHARED)
    set (URHO3D_LIB_TYPE STATIC)
    if (MSVC)
        # This define will be baked into the export header for MSVC compiler
        set (URHO3D_STATIC_DEFINE 1)
    else ()
        # Only define it on the fly when necessary (both SHARED and STATIC libs can coexist) for other compiler toolchains
        add_definitions (-DURHO3D_STATIC_DEFINE)
    endif ()
endif ()

if (URHO3D_DATABASE_ODBC)
    find_package (ODBC REQUIRED)
endif ()

# Define preprocessor macros (for building the Urho3D library) based on the configured build options
foreach (OPT
        URHO3D_ANGELSCRIPT
        URHO3D_DATABASE
        URHO3D_FILEWATCHER
        URHO3D_IK
        URHO3D_LOGGING
        URHO3D_LUA
        URHO3D_MINIDUMPS
        URHO3D_NAVIGATION
        URHO3D_NETWORK
        URHO3D_PHYSICS
        URHO3D_PHYSICS2D
        URHO3D_PROFILING
        URHO3D_TRACY_PROFILING
        URHO3D_THREADING
        URHO3D_URHO2D
        URHO3D_GLES3
        URHO3D_WEBP
        URHO3D_WIN32_CONSOLE)
    if (${OPT})
        add_definitions (-D${OPT})
    endif ()
endforeach ()

# TODO: The logic below is earmarked to be moved into SDL's CMakeLists.txt when refactoring the library dependency handling, until then ensure the DirectX package is not being searched again in external projects such as when building LuaJIT library
if (WIN32 AND NOT CMAKE_PROJECT_NAME MATCHES ^Urho3D-ExternalProject-)
    set (DIRECTX_REQUIRED_COMPONENTS)
    set (DIRECTX_OPTIONAL_COMPONENTS DInput DSound XInput)
    if (URHO3D_D3D11)
        list (APPEND DIRECTX_REQUIRED_COMPONENTS D3D11)
    endif ()

    find_package (DirectX REQUIRED ${DIRECTX_REQUIRED_COMPONENTS} OPTIONAL_COMPONENTS ${DIRECTX_OPTIONAL_COMPONENTS})
    if (DIRECTX_FOUND)
        include_directories (SYSTEM ${DIRECTX_INCLUDE_DIRS})   # These variables may be empty when WinSDK or MinGW is being used
        link_directories (${DIRECTX_LIBRARY_DIRS})
    endif ()
endif ()

# Platform and compiler specific options
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # VS-specific setup
    add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
    if (URHO3D_STATIC_RUNTIME)
        set (RELEASE_RUNTIME /MT)
        set (DEBUG_RUNTIME /MTd)
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_RUNTIME}")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_RUNTIME} /fp:fast /Zi /GS-")
    set (CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_RUNTIME}")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_RUNTIME} /fp:fast /Zi /GS- /D _SECURE_SCL=0")
    set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    # Visual Studio 2012 onward enables the SSE2 by default, however, we must set the flag to IA32 if user intention is to turn the SIMD off
    if (NOT URHO3D_SSE)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:IA32")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:IA32")
    endif ()
    set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /DEBUG")
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
else ()
    # GCC/Clang-specific setup
    set (CMAKE_CXX_VISIBILITY_PRESET hidden)
    set (CMAKE_VISIBILITY_INLINES_HIDDEN true)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
    if (NOT ANDROID)    # Most of the flags are already setup in Android toolchain file
        if (FALSE) # ARM
        else ()
            if (NOT XCODE AND NOT WEB)
                set (CMAKE_C_FLAGS "-mtune=generic ${CMAKE_C_FLAGS}")
                set (CMAKE_CXX_FLAGS "-mtune=generic ${CMAKE_CXX_FLAGS}")
            endif ()
            if (URHO3D_SSE AND NOT XCODE AND NOT WEB)
                # This may influence the effective SSE level when URHO3D_SSE is on as well
                set (URHO3D_DEPLOYMENT_TARGET native CACHE STRING "Specify the minimum CPU type on which the target binaries are to be deployed (non-ARM platform only), see GCC/Clang's -march option for possible values; Use 'generic' for targeting a wide range of generic processors")
                if (NOT URHO3D_DEPLOYMENT_TARGET STREQUAL generic)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${URHO3D_DEPLOYMENT_TARGET}")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${URHO3D_DEPLOYMENT_TARGET}")
                endif ()
            endif ()
            # We don't add these flags directly here for Xcode because we support Mach-O universal binary build
            # The compiler flags will be added later conditionally when the effective arch is i386 during build time (using XCODE_ATTRIBUTE target property)
            if (NOT XCODE)
                if (URHO3D_64BIT)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3")
                else ()
                    # Not the compiler native ABI, this could only happen on multilib-capable compilers
                    if (NATIVE_64BIT)
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
                    endif ()
                    # The effective SSE level could be higher, see also URHO3D_DEPLOYMENT_TARGET and CMAKE_OSX_DEPLOYMENT_TARGET build options
                    # The -mfpmath=sse is not set in global scope but it may be set in local scope when building LuaJIT sub-library for x86 arch
                    if (URHO3D_SSE)
                        if (HAVE_SSE3)
                            set (SIMD_FLAG -msse3)
                        elseif (HAVE_SSE2)
                            set (SIMD_FLAG -msse2)
                        else ()
                            set (SIMD_FLAG -msse)
                        endif ()
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIMD_FLAG}")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_FLAG}")
                    endif ()
                endif ()
                if (NOT URHO3D_SSE)
                    if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
                        # Clang enables SSE support for i386 ABI by default, so use the '-mno-sse' compiler flag to nullify that and make it consistent with GCC
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-sse")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-sse")
                    endif ()
                    if (URHO3D_MMX)
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmmx")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx")
                    endif ()
                    if (URHO3D_3DNOW)
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m3dnow")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m3dnow")
                    endif ()
                endif ()
            endif ()
        endif ()
        if (WEB)
            if (EMSCRIPTEN)
                # Emscripten-specific setup
                set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option --bind")
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option --bind")

                if (URHO3D_THREADING)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
                endif ()
                # Since version 1.37.25 emcc reduces default runtime exports, but we need "Pointer_stringify" so it needs to be explicitly declared now
                # (See https://github.com/kripken/emscripten/commit/3bc1f9f08b9f420680124af703c787244468cedc for more detail)
                # Since version 1.37.28 emcc reduces default runtime exports, but we need "FS" so it needs to be explicitly requested now
                # (See https://github.com/kripken/emscripten/commit/f2191c1223e8261bf45f4e27d2ba4d2e9d8b3341 for more detail)
                # Since version 1.39.5 emcc disables deprecated find event target behavior by default; we revert the flag for now until the support is removed
                # (See https://github.com/emscripten-core/emscripten/commit/948af470be12559367e7629f31cf7c841fbeb2a9#diff-291d81f9d42b322a89881b6d91f7a122 for more detail)
                set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXTRA_EXPORTED_RUNTIME_METHODS=\"['Pointer_stringify']\" -s FORCE_FILESYSTEM=1 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0")
                if (URHO3D_GLES3)
                    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sFULL_ES3")
                endif()
                set (CMAKE_C_FLAGS_RELEASE "-Oz -DNDEBUG")
                set (CMAKE_CXX_FLAGS_RELEASE "-Oz -DNDEBUG")
                # Remove variables to make the -O3 regalloc easier
                set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1")
                # Preserve LLVM debug information, show line number debug comments, and generate source maps; always disable exception handling codegen
                set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
            endif ()
        elseif (MINGW)
            # MinGW-specific setup
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc -fno-keep-inline-dllexport")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -fno-keep-inline-dllexport")
            if (NOT URHO3D_64BIT)
                set (CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
                set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
                # Prevent auto-vectorize optimization when using -O2, unless stack realign is being enforced globally
                if (URHO3D_SSE)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
                else ()
                    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-tree-slp-vectorize -fno-tree-vectorize")
                    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize -fno-tree-vectorize")
                endif ()
            endif ()
        else ()
            # Not Android and not Emscripten and not MinGW derivative
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")     # This will emit '-DREENTRANT' to compiler and '-lpthread' to linker on Linux and Mac OSX platform
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") # However, it may emit other equivalent compiler define and/or linker flag on other *nix platforms
        endif ()
        set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
    endif ()
    if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
        # Clang-specific
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
        if (NINJA OR "$ENV{USE_CCACHE}")    # Stringify to guard against undefined environment variable
            # When ccache support is on, these flags keep the color diagnostics pipe through ccache output and suppress Clang warning due ccache internal preprocessing step
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
        endif ()
        if (URHO3D_LINT)
            find_program (CLANG_TIDY clang-tidy NO_CMAKE_FIND_ROOT_PATH)
            if (CLANG_TIDY)
                set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
                set (CMAKE_C_CLANG_TIDY ${CLANG_TIDY} -config=)
                set (CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} -config=)
                set (CMAKE_EXPORT_COMPILE_COMMANDS 1)
            else ()
                message (FATAL_ERROR "Ensure clang-tidy host tool is installed and can be found in the PATH environment variable.")
            endif ()
        endif ()
        if ((NOT APPLE AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.1) OR (APPLE AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0.0))
            # Workaround for Clang 7.0.1 and above until the Bullet upstream has fixed the Clang 7 diagnostic checks issue (see https://github.com/bulletphysics/bullet3/issues/2114)
            # ditto for AppleClang 11.0.0 and above
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-argument-outside-range")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-argument-outside-range")
        endif ()
    else ()
        # GCC-specific
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9.1)
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=auto")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
        endif ()
    endif ()
endif ()

# Trim the leading white space in the compiler/linker flags, if any
foreach (TYPE C CXX EXE_LINKER SHARED_LINKER)
    string (REGEX REPLACE "^ +" "" CMAKE_${TYPE}_FLAGS "${CMAKE_${TYPE}_FLAGS}")
endforeach ()

include (CMakeParseArguments)

# Macro for adjusting target output name by dropping _suffix from the target name
macro (adjust_target_name)
    if (TARGET_NAME MATCHES _.*$)
        string (REGEX REPLACE _.*$ "" OUTPUT_NAME ${TARGET_NAME})
        set_target_properties (${TARGET_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})
    endif ()
endmacro ()

# Macro for checking the SOURCE_FILES variable is properly initialized
macro (check_source_files)
    if (NOT SOURCE_FILES)
        if (NOT ${ARGN} STREQUAL "")
            message (FATAL_ERROR ${ARGN})
        else ()
            message (FATAL_ERROR "Could not configure and generate the project file because no source files have been defined yet. "
                "You can define the source files explicitly by setting the SOURCE_FILES variable in your CMakeLists.txt; or "
                "by calling the define_source_files() macro which would by default glob all the C++ source files found in the same scope of "
                "CMakeLists.txt where the macro is being called and the macro would set the SOURCE_FILES variable automatically. "
                "If your source files are not located in the same directory as the CMakeLists.txt or your source files are "
                "more than just C++ language then you probably have to pass in extra arguments when calling the macro in order to make it works. "
                "See the define_source_files() macro definition in the cmake/Modules/UrhoCommon.cmake for more detail.")
        endif ()
    endif ()
endmacro ()

# Macro for setting symbolic link on platform that supports it
macro (create_symlink SOURCE DESTINATION)
    cmake_parse_arguments (ARG "FALLBACK_TO_COPY" "BASE" "" ${ARGN})
    # Make absolute paths so they work more reliably on cmake-gui
    if (IS_ABSOLUTE ${SOURCE})
        set (ABS_SOURCE ${SOURCE})
    else ()
        set (ABS_SOURCE ${CMAKE_SOURCE_DIR}/${SOURCE})
    endif ()
    if (IS_ABSOLUTE ${DESTINATION})
        set (ABS_DESTINATION ${DESTINATION})
    else ()
        if (ARG_BASE)
            set (ABS_DESTINATION ${ARG_BASE}/${DESTINATION})
        else ()
            set (ABS_DESTINATION ${CMAKE_BINARY_DIR}/${DESTINATION})
        endif ()
    endif ()
    if (CMAKE_HOST_WIN32)
        if (IS_DIRECTORY ${ABS_SOURCE})
            set (SLASH_D /D)
        else ()
            unset (SLASH_D)
        endif ()
        if (HAS_MKLINK)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_D} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET)
            endif ()
        elseif (ARG_FALLBACK_TO_COPY)
            if (SLASH_D)
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory ${ABS_SOURCE} ${ABS_DESTINATION})
            else ()
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_SOURCE} ${ABS_DESTINATION})
            endif ()
            # Fallback to copy only one time
            execute_process (${COMMAND})
            if (TARGET ${TARGET_NAME})
                # Fallback to copy every time the target is built
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD ${COMMAND})
            endif ()
        else ()
            message (WARNING "Unable to create symbolic link on this host system, you may need to manually copy file/dir from \"${SOURCE}\" to \"${DESTINATION}\"")
        endif ()
    else ()
        get_filename_component (DIRECTORY ${ABS_DESTINATION} DIRECTORY)
        file (RELATIVE_PATH REL_SOURCE ${DIRECTORY} ${ABS_SOURCE})
        execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink ${REL_SOURCE} ${ABS_DESTINATION})
    endif ()
endmacro ()

# Macro for adding additional make clean files
macro (add_make_clean_files)
    get_directory_property (ADDITIONAL_MAKE_CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties (PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${ADDITIONAL_MAKE_CLEAN_FILES};${ARGN}")
endmacro ()

# *** THIS IS A DEPRECATED MACRO ***
# Macro for defining external library dependencies
# The purpose of this macro is emulate CMake to set the external library dependencies transitively
# It works for both targets setup within Urho3D project and downstream projects that uses Urho3D as external static/shared library
# *** THIS IS A DEPRECATED MACRO ***
macro (define_dependency_libs TARGET)
    # third-party/sdl external dependency
    if (${TARGET} MATCHES SDL|Urho3D)
        if (WIN32)
            list (APPEND LIBS user32 gdi32 winmm imm32 ole32 oleaut32 setupapi version uuid)
        elseif (APPLE)
            list (APPEND LIBS iconv)
        elseif (ANDROID)
            list (APPEND LIBS dl log android)
        else ()
            # Linux
            if (NOT WEB)
                list (APPEND LIBS dl m rt)
            endif ()
        endif ()
    endif ()

    # third-party/civetweb external dependency
    if (${TARGET} MATCHES civetweb|Urho3D)
        if (WIN32)
            list (APPEND LIBS ws2_32)
        endif ()
    endif ()

    # third-party/slikeNet external dependency
    if (${TARGET} MATCHES slikeNet|Urho3D)
        if (WIN32)
            list (APPEND LIBS iphlpapi)
        endif ()
    endif ()

    # third-party/tracy external dependency
    if (${TARGET} MATCHES tracy|Urho3D)
        if (MINGW)
            list (APPEND LIBS ws2_32 dbghelp advapi32 user32)
        endif ()
    endif ()

    # third-party/civetweb external dependency
    if (${TARGET} MATCHES civetweb|Urho3D)
        if (URHO3D_SSL)
            if (NOT URHO3D_SSL_DYNAMIC)
                set(OPENSSL_USE_STATIC_LIBS TRUE)
            endif ()
            find_package(OpenSSL)
            list (APPEND LIBS ${OPENSSL_LIBRARIES})
        endif ()
    endif ()

    # Urho3D/LuaJIT external dependency
    if (URHO3D_LUAJIT AND ${TARGET} MATCHES LuaJIT|Urho3D)
        if (NOT WIN32 AND NOT WEB)
            list (APPEND LIBS dl m)
        endif ()
    endif ()

    # Urho3D external dependency
    if (${TARGET} STREQUAL Urho3D)
        # Core
        if (WIN32)
            list (APPEND LIBS winmm)
            if (URHO3D_MINIDUMPS)
                list (APPEND LIBS dbghelp)
            endif ()
        elseif (APPLE)
        endif ()

        # Graphics
        if (URHO3D_OPENGL)
            if (NOT (ANDROID OR WEB OR IOS OR TVOS))
                set (URHO3D_GLES3 FALSE)
            endif()
            if (APPLE)
                # Do nothing
            elseif (WIN32)
                list (APPEND LIBS opengl32)
            else ()
                list (APPEND LIBS GL)
            endif ()
        endif ()
        if (DIRECT3D_LIBRARIES)
            list (APPEND LIBS ${DIRECT3D_LIBRARIES})
        endif ()

        # Database
        if (URHO3D_DATABASE_ODBC)
            list (APPEND LIBS ${ODBC_LIBRARIES})
        endif ()

        # This variable value can either be 'Urho3D' target or an absolute path to an actual static/shared Urho3D library or empty (if we are building the library itself)
        # The former would cause CMake not only to link against the Urho3D library but also to add a dependency to Urho3D target
        if (URHO3D_LIBRARIES)
            if (WIN32 AND URHO3D_LIBRARIES_DBG AND URHO3D_LIBRARIES_REL AND TARGET ${TARGET_NAME})
                # Special handling when both debug and release libraries are found
                target_link_libraries (${TARGET_NAME} debug ${URHO3D_LIBRARIES_DBG} optimized ${URHO3D_LIBRARIES_REL})
            else ()
                if (TARGET ${TARGET}_universal)
                    add_dependencies (${TARGET_NAME} ${TARGET}_universal)
                endif ()
                list (APPEND ABSOLUTE_PATH_LIBS ${URHO3D_LIBRARIES})
            endif ()
        endif ()
    endif ()
endmacro ()

# Macro for defining source files with optional arguments as follows:
#  GLOB_CPP_PATTERNS <list> - Use the provided globbing patterns for CPP_FILES instead of the default *.cpp
#  GLOB_H_PATTERNS <list> - Use the provided globbing patterns for H_FILES instead of the default *.h
#  EXCLUDE_PATTERNS <list> - Use the provided regex patterns for excluding the unwanted matched source files
#  EXTRA_CPP_FILES <list> - Include the provided list of files into CPP_FILES result
#  EXTRA_H_FILES <list> - Include the provided list of files into H_FILES result
#  RECURSE - Option to glob recursively
#  GROUP - Option to group source files based on its relative path to the corresponding parent directory
macro (define_source_files)
    # Source files are defined by globbing source files in current source directory and also by including the extra source files if provided
    cmake_parse_arguments (ARG "RECURSE;GROUP" "" "EXTRA_CPP_FILES;EXTRA_H_FILES;GLOB_CPP_PATTERNS;GLOB_H_PATTERNS;EXCLUDE_PATTERNS" ${ARGN})
    if (NOT ARG_GLOB_CPP_PATTERNS)
        set (ARG_GLOB_CPP_PATTERNS *.cpp)    # Default glob pattern
    endif ()
    if (NOT ARG_GLOB_H_PATTERNS)
        set (ARG_GLOB_H_PATTERNS *.h)
    endif ()
    if (ARG_RECURSE)
        set (ARG_RECURSE _RECURSE)
    else ()
        unset (ARG_RECURSE)
    endif ()
    file (GLOB${ARG_RECURSE} CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_GLOB_CPP_PATTERNS})
    file (GLOB${ARG_RECURSE} H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_GLOB_H_PATTERNS})
    if (ARG_EXCLUDE_PATTERNS)
        set (CPP_FILES_WITH_SENTINEL ";${CPP_FILES};")  # Stringify the lists
        set (H_FILES_WITH_SENTINEL ";${H_FILES};")
        foreach (PATTERN ${ARG_EXCLUDE_PATTERNS})
            foreach (LOOP RANGE 1)
                string (REGEX REPLACE ";${PATTERN};" ";;" CPP_FILES_WITH_SENTINEL "${CPP_FILES_WITH_SENTINEL}")
                string (REGEX REPLACE ";${PATTERN};" ";;" H_FILES_WITH_SENTINEL "${H_FILES_WITH_SENTINEL}")
            endforeach ()
        endforeach ()
        set (CPP_FILES ${CPP_FILES_WITH_SENTINEL})      # Convert strings back to lists, extra sentinels are harmless
        set (H_FILES ${H_FILES_WITH_SENTINEL})
    endif ()
    list (APPEND CPP_FILES ${ARG_EXTRA_CPP_FILES})
    list (APPEND H_FILES ${ARG_EXTRA_H_FILES})
    set (SOURCE_FILES ${CPP_FILES} ${H_FILES})
    # Optionally group the sources based on their physical subdirectories
    if (ARG_GROUP)
        source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})
    endif ()
endmacro ()

# Macro for defining resource directories with optional arguments as follows:
#  GLOB_PATTERNS <list> - Use the provided globbing patterns for resource directories, default to "${CMAKE_SOURCE_DIR}/bin/*Data"
#  EXCLUDE_PATTERNS <list> - Use the provided regex patterns for excluding the unwanted matched directories
#  EXTRA_DIRS <list> - Include the provided list of directories into globbing result
macro (define_resource_dirs)
    check_source_files ("Could not call define_resource_dirs() macro before define_source_files() macro.")
    cmake_parse_arguments (ARG "" "" "GLOB_PATTERNS;EXCLUDE_PATTERNS;EXTRA_DIRS" ${ARGN})
    # If not explicitly specified then use the Urho3D project structure convention
    if (NOT ARG_GLOB_PATTERNS)
        set (ARG_GLOB_PATTERNS ${CMAKE_SOURCE_DIR}/bin/*Data)
    endif ()
    file (GLOB GLOB_RESULTS ${ARG_GLOB_PATTERNS})
    unset (GLOB_DIRS)
    foreach (DIR ${GLOB_RESULTS})
        if (IS_DIRECTORY ${DIR})
            list (APPEND GLOB_DIRS ${DIR})
        endif ()
    endforeach ()
    if (ARG_EXCLUDE_PATTERNS)
        set (GLOB_DIRS_WITH_SENTINEL ";${GLOB_DIRS};")  # Stringify the lists
        foreach (PATTERN ${ARG_EXCLUDE_PATTERNS})
            foreach (LOOP RANGE 1)
                string (REGEX REPLACE ";${PATTERN};" ";;" GLOB_DIRS_WITH_SENTINEL "${GLOB_DIRS_WITH_SENTINEL}")
            endforeach ()
        endforeach ()
        set (GLOB_DIRS ${GLOB_DIRS_WITH_SENTINEL})      # Convert strings back to lists, extra sentinels are harmless
    endif ()
    list (APPEND RESOURCE_DIRS ${GLOB_DIRS})
    foreach (DIR ${ARG_EXTRA_DIRS})
        if (EXISTS ${DIR})
            list (APPEND RESOURCE_DIRS ${DIR})
        endif ()
    endforeach ()
    source_group ("Resource Dirs" FILES ${RESOURCE_DIRS})
    # Populate all the variables required by resource packaging, if the build option is enabled
    if (URHO3D_PACKAGING AND RESOURCE_DIRS)
        foreach (DIR ${RESOURCE_DIRS})
            get_filename_component (NAME ${DIR} NAME)
            if (ANDROID)
                set (RESOURCE_${DIR}_PATHNAME ${CMAKE_BINARY_DIR}/assets/${NAME}.pak)
            else ()
                set (RESOURCE_${DIR}_PATHNAME ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}.pak)
            endif ()
            list (APPEND RESOURCE_PAKS ${RESOURCE_${DIR}_PATHNAME})
            if (EMSCRIPTEN AND NOT EMSCRIPTEN_SHARE_DATA)
                # Set the custom EMCC_OPTION property to preload the *.pak individually
                set_source_files_properties (${RESOURCE_${DIR}_PATHNAME} PROPERTIES EMCC_OPTION preload-file EMCC_FILE_ALIAS "${NAME}.pak")
            endif ()
        endforeach ()
        set_property (SOURCE ${RESOURCE_PAKS} PROPERTY GENERATED TRUE)
        if (WEB)
            if (EMSCRIPTEN)
                # Set the custom EMCC_OPTION property to preload the generated shared data file
                if (EMSCRIPTEN_SHARE_DATA)
                    set (SHARED_RESOURCE_JS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.js)
                    list (APPEND SOURCE_FILES ${SHARED_RESOURCE_JS} ${SHARED_RESOURCE_JS}.data)
                    # DEST_BUNDLE_DIR may be empty when macro caller does not wish to install anything
                    if (DEST_BUNDLE_DIR)
                        install (FILES ${SHARED_RESOURCE_JS} ${SHARED_RESOURCE_JS}.data DESTINATION ${DEST_BUNDLE_DIR})
                    endif ()
                    # Define a custom command for generating a shared data file
                    if (RESOURCE_PAKS)
                        # When sharing a single data file, all main targets are assumed to use a same set of resource paks
                        foreach (FILE ${RESOURCE_PAKS})
                            get_filename_component (NAME ${FILE} NAME)
                            list (APPEND PAK_NAMES ${NAME})
                        endforeach ()
                        set (EMPACKAGER "${EMSCRIPTEN_ROOT_PATH}/tools/file_packager")
                        add_custom_command (OUTPUT ${SHARED_RESOURCE_JS} ${SHARED_RESOURCE_JS}.data
                            COMMAND ${EMPACKAGER} ${SHARED_RESOURCE_JS}.data --preload ${PAK_NAMES} --js-output=${SHARED_RESOURCE_JS} --use-preload-cache
                            DEPENDS RESOURCE_CHECK ${RESOURCE_PAKS}
                            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                            COMMENT "Generating shared data file")
                    endif ()
                endif ()
            endif ()
        endif ()
    endif ()
    if (XCODE)
        if (NOT RESOURCE_FILES)
            # Default app bundle icon
            set (RESOURCE_FILES ${CMAKE_SOURCE_DIR}/bin/Data/Textures/UrhoIcon.icns)
        endif ()
        # Group them together under 'Resources' in Xcode IDE
        source_group (Resources FILES ${RESOURCE_PAKS} ${RESOURCE_FILES})     # RESOURCE_PAKS could be empty if packaging is not requested
        # But only use either paks or dirs
        if (RESOURCE_PAKS)
            set_source_files_properties (${RESOURCE_PAKS} ${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        else ()
            set_source_files_properties (${RESOURCE_DIRS} ${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif ()
    endif ()
    list (APPEND SOURCE_FILES ${RESOURCE_DIRS} ${RESOURCE_PAKS} ${RESOURCE_FILES})
endmacro()

include (GenerateExportHeader)

# Macro for setting common output directories
macro (set_output_directories OUTPUT_PATH)
    cmake_parse_arguments (ARG LOCAL "" "" ${ARGN})
    if (ARG_LOCAL)
        unset (SCOPE)
        unset (OUTPUT_DIRECTORY_PROPERTIES)
    else ()
        set (SCOPE CMAKE_)
    endif ()
    foreach (TYPE ${ARG_UNPARSED_ARGUMENTS})
        set (${SCOPE}${TYPE}_OUTPUT_DIRECTORY ${OUTPUT_PATH})
        list (APPEND OUTPUT_DIRECTORY_PROPERTIES ${TYPE}_OUTPUT_DIRECTORY ${${TYPE}_OUTPUT_DIRECTORY})
        foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
            string (TOUPPER ${CONFIG} CONFIG)
            set (${SCOPE}${TYPE}_OUTPUT_DIRECTORY_${CONFIG} ${OUTPUT_PATH})
            list (APPEND OUTPUT_DIRECTORY_PROPERTIES ${TYPE}_OUTPUT_DIRECTORY_${CONFIG} ${${TYPE}_OUTPUT_DIRECTORY_${CONFIG}})
        endforeach ()
        if (TYPE STREQUAL RUNTIME AND NOT ${OUTPUT_PATH} STREQUAL .)
            file (RELATIVE_PATH REL_OUTPUT_PATH ${CMAKE_BINARY_DIR} ${OUTPUT_PATH})
            set (DEST_RUNTIME_DIR ${REL_OUTPUT_PATH})
        endif ()
    endforeach ()
    if (ARG_LOCAL)
        list (APPEND TARGET_PROPERTIES ${OUTPUT_DIRECTORY_PROPERTIES})
    endif ()
endmacro ()

# Macro for setting up a library target
# Macro arguments:
#  NODEPS - setup library target without defining Urho3D dependency libraries (applicable for downstream projects)
#  STATIC/SHARED/MODULE/EXCLUDE_FROM_ALL - see CMake help on add_library() command
# CMake variables:
#  SOURCE_FILES - list of source files
#  INCLUDE_DIRS - list of directories for include search path
#  LIBS - list of dependent libraries that are built internally in the project
#  ABSOLUTE_PATH_LIBS - list of dependent libraries that are external to the project
#  LINK_DEPENDS - list of additional files on which a target binary depends for linking (Makefile-based generator only)
#  LINK_FLAGS - list of additional link flags
#  TARGET_PROPERTIES - list of target properties
macro (setup_library)
    cmake_parse_arguments (ARG NODEPS "" "" ${ARGN})
    check_source_files ()
    add_library (${TARGET_NAME} ${ARG_UNPARSED_ARGUMENTS} ${SOURCE_FILES})
    get_target_property (LIB_TYPE ${TARGET_NAME} TYPE)
    if (NOT ARG_NODEPS AND NOT PROJECT_NAME STREQUAL Urho3D)
        define_dependency_libs (Urho3D)
    endif ()
    if (XCODE AND LUAJIT_SHARED_LINKER_FLAGS_APPLE AND LIB_TYPE STREQUAL SHARED_LIBRARY)
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS[arch=x86_64] "${LUAJIT_SHARED_LINKER_FLAGS_APPLE} $(OTHER_LDFLAGS)")    # Xcode universal build linker flags when targeting 64-bit OSX with LuaJIT enabled
    endif ()
    _setup_target ()
endmacro ()

# This cache variable is used to keep track of whether a resource directory has been instructed to be installed by CMake or not
unset (INSTALLED_RESOURCE_DIRS CACHE)

# Macro for setting up dependency lib for compilation and linking of a target (to be used internally)
macro (_setup_target)
    # Include directories
    include_directories (${INCLUDE_DIRS})
    # Link libraries
    define_dependency_libs (${TARGET_NAME})
    target_link_libraries (${TARGET_NAME} PRIVATE ${ABSOLUTE_PATH_LIBS} ${LIBS})
    # Extra compiler flags for Xcode which are dynamically changed based on active arch in order to support Mach-O universal binary targets
    # We don't add the ABI flag for Xcode because it automatically passes '-arch i386' compiler flag when targeting 32 bit which does the same thing as '-m32'
    if (XCODE)
        # Speed up build when in Debug configuration by building active arch only
        list (FIND TARGET_PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH ATTRIBUTE_ALREADY_SET)
        if (ATTRIBUTE_ALREADY_SET EQUAL -1)
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH $<$<CONFIG:Debug>:YES>)
        endif ()
        if (NOT URHO3D_SSE)
            # Nullify the Clang default so that it is consistent with GCC
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CFLAGS[arch=i386] "-mno-sse $(OTHER_CFLAGS)")
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS[arch=i386] "-mno-sse $(OTHER_CPLUSPLUSFLAGS)")
        endif ()
    endif ()
    # Extra linker flags for Emscripten
    if (EMSCRIPTEN)
        # These flags are set only once in the main executable
        if (NOT LIB_TYPE)   # LIB_TYPE is empty for executable target
            list (APPEND LINK_FLAGS "-s TOTAL_MEMORY=${EMSCRIPTEN_TOTAL_MEMORY}")
            if (EMSCRIPTEN_ALLOW_MEMORY_GROWTH)
                list (APPEND LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
            endif ()
            if (EMSCRIPTEN_SHARE_DATA)
                list (APPEND LINK_FLAGS "--pre-js \"${CMAKE_BINARY_DIR}/Source/pak-loader.js\"")
            endif ()
            if (URHO3D_TESTING)
                list (APPEND LINK_FLAGS --emrun)
            else ()
                # If not using EMRUN then we need to include the emrun_prejs.js manually in order to process the request parameters as app's arguments correctly
                list (APPEND LINK_FLAGS "--pre-js \"${EMSCRIPTEN_ROOT_PATH}/src/emrun_prejs.js\"")
            endif ()
            # These flags are here instead of in the CMAKE_EXE_LINKER_FLAGS so that they do not interfere with the auto-detection logic during initial configuration
            list (APPEND LINK_FLAGS "-s NO_EXIT_RUNTIME=1 -s ERROR_ON_UNDEFINED_SYMBOLS=1")
        endif ()
        # Pass additional source files to linker with the supported flags, such as: js-library, pre-js, post-js, embed-file, preload-file, shell-file
        foreach (FILE ${SOURCE_FILES})
            get_property (EMCC_OPTION SOURCE ${FILE} PROPERTY EMCC_OPTION)
            if (EMCC_OPTION)
                unset (EMCC_FILE_ALIAS)
                unset (EMCC_EXCLUDE_FILE)
                unset (USE_PRELOAD_CACHE)
                if (EMCC_OPTION STREQUAL embed-file OR EMCC_OPTION STREQUAL preload-file)
                    get_property (EMCC_FILE_ALIAS SOURCE ${FILE} PROPERTY EMCC_FILE_ALIAS)
                    if (EMCC_FILE_ALIAS)
                        set (EMCC_FILE_ALIAS "@\"${EMCC_FILE_ALIAS}\"")
                    endif ()
                    get_property (EMCC_EXCLUDE_FILE SOURCE ${FILE} PROPERTY EMCC_EXCLUDE_FILE)
                    if (EMCC_EXCLUDE_FILE)
                        set (EMCC_EXCLUDE_FILE " --exclude-file \"${EMCC_EXCLUDE_FILE}\"")
                    else ()
                        list (APPEND LINK_DEPENDS ${FILE})
                    endif ()
                    if (EMCC_OPTION STREQUAL preload-file)
                        set (USE_PRELOAD_CACHE " --use-preload-cache")
                    endif ()
                endif ()
                list (APPEND LINK_FLAGS "--${EMCC_OPTION} \"${FILE}\"${EMCC_FILE_ALIAS}${EMCC_EXCLUDE_FILE}${USE_PRELOAD_CACHE}")
            endif ()
        endforeach ()
    endif ()
    # Set additional linker dependencies (only work for Makefile-based generator according to CMake documentation)
    if (LINK_DEPENDS)
        string (REPLACE ";" "\;" LINK_DEPENDS "${LINK_DEPENDS}")        # Stringify for string replacement
        list (APPEND TARGET_PROPERTIES LINK_DEPENDS "${LINK_DEPENDS}")  # Stringify with semicolons already escaped
        unset (LINK_DEPENDS)
    endif ()
    # Set additional linker flags
    if (LINK_FLAGS)
        string (REPLACE ";" " " LINK_FLAGS "${LINK_FLAGS}")
        list (APPEND TARGET_PROPERTIES LINK_FLAGS ${LINK_FLAGS})
        unset (LINK_FLAGS)
    endif ()
    if (TARGET_PROPERTIES)
        set_target_properties (${TARGET_NAME} PROPERTIES ${TARGET_PROPERTIES})
        unset (TARGET_PROPERTIES)
    endif ()
    # Create symbolic links in the build tree
    if (NOT ANDROID AND NOT URHO3D_PACKAGING)
        # Ensure the asset root directory exist before creating the symlinks
        file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
        foreach (I ${RESOURCE_DIRS})
            get_filename_component (NAME ${I} NAME)
            if (NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${NAME} AND EXISTS ${I})
                create_symlink (${I} ${CMAKE_BINARY_DIR}/bin/${NAME} FALLBACK_TO_COPY)
            endif ()
        endforeach ()
    endif ()
endmacro()

# Macro for setting up a test case
macro (setup_test)
    if (URHO3D_TESTING)
        cmake_parse_arguments (ARG "" NAME OPTIONS ${ARGN})
        if (NOT ARG_NAME)
            set (ARG_NAME ${TARGET_NAME})
        endif ()
        list (APPEND ARG_OPTIONS -timeout ${URHO3D_TEST_TIMEOUT})
        if (WEB)
            if (EMSCRIPTEN)
                math (EXPR EMRUN_TIMEOUT "2 * ${URHO3D_TEST_TIMEOUT}")
                add_test (NAME ${ARG_NAME} COMMAND ${EMRUN} --browser ${EMSCRIPTEN_EMRUN_BROWSER} --timeout ${EMRUN_TIMEOUT} --kill_exit ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.html ${ARG_OPTIONS})
            endif ()
        else ()
            add_test (NAME ${ARG_NAME} COMMAND ${TARGET_NAME} ${ARG_OPTIONS})
        endif ()
    endif ()
endmacro ()

# Warn user if PATH environment variable has not been correctly set for using ccache
if (NOT CMAKE_HOST_WIN32 AND "$ENV{USE_CCACHE}")
    if (APPLE)
        set (WHEREIS brew info ccache)
    else ()
        set (WHEREIS whereis -b ccache)
    endif ()
    execute_process (COMMAND ${WHEREIS} COMMAND grep -o \\S*lib\\S* RESULT_VARIABLE EXIT_CODE OUTPUT_VARIABLE CCACHE_SYMLINK ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (EXIT_CODE EQUAL 0 AND NOT $ENV{PATH} MATCHES "${CCACHE_SYMLINK}")  # Need to stringify because CCACHE_SYMLINK variable could be empty when the command failed
        message (WARNING "The lib directory containing the ccache symlinks (${CCACHE_SYMLINK}) has not been added in the PATH environment variable. "
            "This is required to enable ccache support for native compiler toolchain. CMake has been configured to use the actual compiler toolchain instead of ccache. "
            "In order to rectify this, the build tree must be regenerated after the PATH environment variable has been adjusted accordingly.")
    endif ()
endif ()
