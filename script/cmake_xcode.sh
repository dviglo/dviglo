#!/usr/bin/env bash

# Copyright (c) 2008-2022 the Urho3D project
# Copyright (c) 2022-2022 the Dviglo project
# License: MIT

$(dirname $0)/cmake_generic.sh "$@" -G Xcode -T buildsystem=1

# vi: set ts=4 sw=4 expandtab:
