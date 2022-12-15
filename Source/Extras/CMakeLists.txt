# Copyright (c) 2008-2022 the Urho3D project
# Copyright (c) 2022-2022 the Dviglo project
# License: MIT

# Set project name
project (Urho3D-Extras)

# Find Urho3D library
find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS})

# Urho3D extras
add_subdirectory (OgreBatchConverter)
