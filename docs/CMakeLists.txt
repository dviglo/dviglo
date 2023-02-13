# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if(DVIGLO_IS_SUBPROJECT)
    return()
endif ()

# Помещаем документацию в отдельный проект
project("dv docs")

if (DV_DOCS_QUIET)
    set (DV_DOCS 1)
endif ()

# Macro for enabling IDE-specific integration with Urho3D documentation (See comments in Doxyfile for more detail instructions)
macro (enable_help IDE)
    if (${IDE} OR CMAKE_EXTRA_GENERATOR MATCHES ${IDE})
        set (${IDE}_HELP YES)
    else ()
        set (${IDE}_HELP NO)
    endif ()
endmacro ()

# There could be bug in CMake find_package() command, it currently does not honor NO_CMAKE_FIND_ROOT_PATH option for a non-rooted search as per CMake's documentation
# As a workaround, we unset CMAKE_FIND_ROOT_PATH (even when we are cross-compiling) but in this scope ONLY in order to always do a non-rooted search for Doxygen package
unset (CMAKE_FIND_ROOT_PATH)
# Find Doxygen and DOT packages
if (DV_DOCS)
    set (REQUIRED REQUIRED)
endif ()
# https://github.com/urho3d/Urho3D/issues/2757
find_package (Doxygen 1.9.2 ${REQUIRED})

if (DOXYGEN_FOUND)
    # It is not an error when DOT is not found, instead just switching off the Doxygen's HAVE_DOT option
    find_package_handle_standard_args (Dot REQUIRED_VARS DOXYGEN_DOT_EXECUTABLE)

    # Generate platform specific Doxyfile automatically
    if (NOT DV_OPENGL EQUAL DOXYFILE_DV_OPENGL OR ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)
        set (DOXYFILE_DV_OPENGL ${DV_OPENGL} CACHE INTERNAL "DV_OPENGL build option when Doxyfile was last generated")
        if (DV_OPENGL)
            set (EXCLUDE_GRAPHICS_API Direct3D9)
            set (EXCLUDE_GRAPHICS_API2 Direct3D11)
        else ()
            set (EXCLUDE_GRAPHICS_API OpenGL)
            if (DV_D3D11)
                set (EXCLUDE_GRAPHICS_API2 Direct3D9)
            else ()
                set (EXCLUDE_GRAPHICS_API2 Direct3D11)
            endif ()
        endif ()
        foreach (IDE XCODE MSVC Eclipse CodeBlocks)
            enable_help (${IDE})
        endforeach ()
        configure_file (${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/generated/doxyfile)
    endif ()

    # In quiet mode, redirect standard output stream of Doxygen to a null device
    if (DV_DOCS_QUIET)
        set (REDIRECT_STDOUT 1>${NULL_DEVICE})
    endif ()
    # If DV_DOCS build option is set then add the custom 'doc' target into the default 'all' target, i.e. a normal build would not only build the software but also the documentation
    if (DV_DOCS)
        set (ALL ALL)
    endif ()

    # Add custom 'doc' target for generating Urho3D documentation
    file (GLOB DOX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.dox)
    add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE} generated/doxyfile ${REDIRECT_STDOUT}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/doxyfile ${DOX_FILES}
        COMMENT "Generating documentation with Doxygen")
    add_custom_target (doc ${ALL}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/index.html)
endif ()

# Make sure html output directory exists and not empty
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/html/Readme.txt "If the DV_DOCS build option is set then the Urho3D documentation will be built automatically as part of the Urho3D library build. If build option is not set, however, then use 'doc' built-in target to generate the documentation manually (must do this before installing the SDK).\n")

# Define post build steps
if (CMAKE_EXTRA_GENERATOR MATCHES Eclipse AND NOT CMAKE_HOST_WIN32)
    add_custom_command (TARGET doc
        COMMAND ${CMAKE_COMMAND} -E remove ~/.eclipse/`ls -1t ~/.eclipse |grep org.eclipse.platform_ |head -1`/plugins/html || true
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/html ~/.eclipse/`ls -1t ~/.eclipse |grep org.eclipse.platform_ |head -1`/plugins/html || true
        COMMENT "Creating symbolic link to HTML documentation in the Eclipse IDE")
endif ()
