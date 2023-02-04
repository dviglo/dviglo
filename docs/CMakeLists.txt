# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if (dviglo_subproject)
    return()
endif ()

# Помещаем документацию в отдельный проект
project(dviglo-docs)

if (URHO3D_DOCS_QUIET)
    set (URHO3D_DOCS 1)
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
if (URHO3D_DOCS)
    set (REQUIRED REQUIRED)
endif ()
# https://github.com/urho3d/Urho3D/issues/2757
find_package (Doxygen 1.9.2 ${REQUIRED})

if (DOXYGEN_FOUND)
    # It is not an error when DOT is not found, instead just switching off the Doxygen's HAVE_DOT option
    find_package_handle_standard_args (Dot REQUIRED_VARS DOXYGEN_DOT_EXECUTABLE)

    # Generate platform specific Doxyfile automatically
    if (NOT URHO3D_OPENGL EQUAL DOXYFILE_URHO3D_OPENGL OR ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        set (DOXYFILE_URHO3D_OPENGL ${URHO3D_OPENGL} CACHE INTERNAL "URHO3D_OPENGL build option when Doxyfile was last generated")
        if (URHO3D_OPENGL)
            set (EXCLUDE_GRAPHICS_API Direct3D9)
            set (EXCLUDE_GRAPHICS_API2 Direct3D11)
        else ()
            set (EXCLUDE_GRAPHICS_API OpenGL)
            if (URHO3D_D3D11)
                set (EXCLUDE_GRAPHICS_API2 Direct3D9)
            else ()
                set (EXCLUDE_GRAPHICS_API2 Direct3D11)
            endif ()
        endif ()
        foreach (IDE XCODE MSVC Eclipse CodeBlocks)
            enable_help (${IDE})
        endforeach ()
        configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/generated/Doxyfile)
    endif ()

    # In quiet mode, redirect standard output stream of Doxygen to a null device
    if (URHO3D_DOCS_QUIET)
        set (REDIRECT_STDOUT 1>${NULL_DEVICE})
    endif ()
    # If URHO3D_DOCS build option is set then add the custom 'doc' target into the default 'all' target, i.e. a normal build would not only build the software but also the documentation
    if (URHO3D_DOCS)
        set (ALL ALL)
    endif ()

    # Add custom 'doc' target for generating Urho3D documentation
    file (GLOB DOX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.dox)
    add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE} generated/Doxyfile ${REDIRECT_STDOUT}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/Doxyfile ${DOX_FILES}
        COMMENT "Generating documentation with Doxygen")
    add_custom_target (doc ${ALL}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/index.html)
endif ()

# Make sure html output directory exists and not empty
file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/html/Readme.txt "If the URHO3D_DOCS build option is set then the Urho3D documentation will be built automatically as part of the Urho3D library build. If build option is not set, however, then use 'doc' built-in target to generate the documentation manually (must do this before installing the SDK).\n")

# Currently it is not possible to make built-in 'install' target to depend on 'doc' in CMake, therefore 'make doc' command need to be invoked manually before 'make install' in order to install the SDK with complete documentation
# Unless, URHO3D_DOCS build option is set in which case the custom 'doc' target is part of the default 'all' target which in turn the 'install' target depends on, so a single 'make install' alone is suffice to install everything
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/ DESTINATION ${DEST_SHARE_DIR}/docs)

# Define post build steps
if (CMAKE_EXTRA_GENERATOR MATCHES Eclipse AND NOT CMAKE_HOST_WIN32)
    add_custom_command (TARGET doc
        COMMAND ${CMAKE_COMMAND} -E remove ~/.eclipse/`ls -1t ~/.eclipse |grep org.eclipse.platform_ |head -1`/plugins/html || true
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/html ~/.eclipse/`ls -1t ~/.eclipse |grep org.eclipse.platform_ |head -1`/plugins/html || true
        COMMENT "Creating symbolic link to HTML documentation in the Eclipse IDE")
endif ()