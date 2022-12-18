# Copyright (c) 2008-2022 the Urho3D project
# Copyright (c) 2022-2022 the Dviglo project
# License: MIT

# VS generator is multi-config, we need to use the CMake generator expression to get the correct target linker filename during post build step

configure_file (dviglo.pc.msvc dviglo.pc @ONLY)
