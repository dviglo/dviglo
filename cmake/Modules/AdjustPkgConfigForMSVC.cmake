# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

# VS generator is multi-config, we need to use the CMake generator expression to get the correct target linker filename during post build step

configure_file (dviglo.pc.msvc dviglo.pc @ONLY)
