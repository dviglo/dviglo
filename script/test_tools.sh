#!/usr/bin/env bash

# Copyright (c) 2008-2023 the Urho3D project
# Copyright (c) 2022-2023 the Dviglo project
# License: MIT

if [[ $# != 2 ]]
then
    echo "Usage: test_tools.sh repo_dir build_dir"
    exit 1
fi

repo_dir=$1
build_dir=$2

# Remove trailing slash in Unix and Windows
repo_dir=${repo_dir%/}
repo_dir=${repo_dir%\\}
build_dir=${build_dir%/}
build_dir=${build_dir%\\}

# ================== ogre_importer ==================

src_filenames=(
    jack.mesh.xml
    level.mesh.xml
    mushroom.mesh.xml
    ninja.mesh.xml
    potion.mesh.xml
    snowball.mesh.xml
    snow_crate.mesh.xml
)

result_filenames=(
    jack_walk.ani
    ninja_attack1.ani
    ninja_attack2.ani
    ninja_attack3.ani
    ninja_backflip.ani
    ninja_block.ani
    ninja_climb.ani
    ninja_crouch.ani
    ninja_death1.ani
    ninja_death2.ani
    ninja_high_jump.ani
    ninja_idle1.ani
    ninja_idle2.ani
    ninja_idle3.ani
    ninja_jump.ani
    ninja_jump_no_height.ani
    ninja_kick.ani
    ninja_side_kick.ani
    ninja_spin.ani
    ninja_stealth.ani
    ninja_walk.ani
    jack.mdl
    level.mdl
    mushroom.mdl
    ninja.mdl
    potion.mdl
    snowball.mdl
    snow_crate.mdl
)

# This is not used because the result is compiler dependent.
# Perhaps this is due to the calculation error when using float
result_checksums=(
    a07f69d23b84b0039d7e9159a494d7c5a587ae6d # jack_walk.ani
    5abfe9cee41a2d8c4d8af8af5baf879fc6ab36cb # ninja_attack1.ani
    f73548094148f9d18d6a93b66eac647db50672eb # ninja_attack2.ani
    0549824bcbf73ebaff17964ba81ae2ca813ad684 # ninja_attack3.ani
    a2aab4ad61b9de9038135abf9b1e459e17d6bb20 # ninja_backflip.ani
    dec34fb0ec43f6fca8037328fc0dc8bd1e55d30d # ninja_block.ani
    ae20b728ca1f2776ab6bb66ab4855f56710e8d14 # ninja_climb.ani
    8f21ea4e1f999167a2b82a575c15de614f576a0f # ninja_crouch.ani
    b3314a675d1a41f97f818f06b562e20ab072de5a # ninja_death1.ani
    e3dd6aa2fb9b585180e3772b7c598828d935bf15 # ninja_death2.ani
    ff6766e3d55855a240fc35ccf7395a6517911c99 # ninja_high_jump.ani
    d565fea93087a60df836637aad7f9c8184c28c6a # ninja_idle1.ani
    065f7d89e032c18dedeae0b64975984ec80d3557 # ninja_idle2.ani
    00b4d893b3cffbf58db4fb7f0e970990accb1155 # ninja_idle3.ani
    bad0c363ec9cf9283ca08b8ca44a86dc09068603 # ninja_jump.ani
    e55e9a21e635eb8a31b708fa009e9aca06ea9e7d # ninja_jump_no_height.ani
    82fcc8eda8929809fd771805466b8e287af3a3c7 # ninja_kick.ani
    b09e8cc6e0fb58786c21184bcc0aa8dcd4b51b57 # ninja_side_kick.ani
    63683537161f7352684bc50a44b753b5f9691af3 # ninja_spin.ani
    801383ba0ad30d4a0d401a69af5ca88bf5c6b39f # ninja_stealth.ani
    608a91452ff6980ef34ca68c4fd688fd719592c2 # ninja_walk.ani
    c8b032146ccde40058af94d5578e61345f6fb648 # jack.mdl
    cde0671fced710d666f15cd941c9f385359a45e9 # level.mdl
    39afeecb472b8be081fddb59419db595ccae2105 # mushroom.mdl
    ef5b6c831566f45cef0c7652fe16a7eec7f57f4c # ninja.mdl
    351044e86480d7ab0ee5bac2f557f800e6770925 # potion.mdl
    4c3c7b1c47f62c5c0a99e1f33bd20463a64d6b93 # snowball.mdl
    110a90536d58e0a57ef6070b2b372341cb97bdaa # snow_crate.mdl
)

result_sizes=(
    56410  # jack_Walk.ani
    2916   # ninja_attack1.ani
    2788   # ninja_attack2.ani
    2628   # ninja_attack3.ani
    3141   # ninja_backflip.ani
    1186   # ninja_block.ani
    2466   # ninja_climb.ani
    4003   # ninja_crouch.ani
    2307   # ninja_death1.ani
    2723   # ninja_death2.ani
    5541   # ninja_high_jump.ani
    3682   # ninja_idle1.ani
    2434   # ninja_idle2.ani
    2658   # ninja_idle3.ani
    2785   # ninja_jump.ani
    2857   # ninja_jump_no_height.ani
    3745   # ninja_kick.ani
    2885   # ninja_side_kick.ani
    2625   # ninja_spin.ani
    2244   # ninja_stealth.ani
    3201   # ninja_walk.ani
    433684 # jack.mdl
    26100  # level.mdl
    74196  # mushroom.mdl
    66895  # ninja.mdl
    7092   # potion.mdl
    2484   # snowball.mdl
    11724  # snow_crate.mdl
)

# Если существует отладочная версия, то используем её
if [ -f ${build_dir}/bin/tool/ogre_importer_d ]
then
    ogre_importer_path=${build_dir}/bin/tool/ogre_importer_d
else
    ogre_importer_path=${build_dir}/bin/tool/ogre_importer
fi

# Convert models
for (( i = 0; i < ${#src_filenames[*]}; ++i ))
do
    src_filepath=${repo_dir}/source_assets/${src_filenames[$i]}

    # Replace extension from .mesh.xml to .mdl
    output_filename=${src_filenames[$i]%.mesh.xml}.mdl

    ${ogre_importer_path} $src_filepath $output_filename -t

    # Check exit code of the previous command
    if [[ $? != 0 ]]
    then
        exit 1
    fi
done

# Compare checksums
#for (( i = 0; i < ${#result_filenames[*]}; ++i ))
#do
#    checksum=$(sha1sum ${result_filenames[$i]})
#
#    # Check exit code of the previous command
#    if [[ $? != 0 ]]
#    then
#        exit 1
#    fi
#
#    # Extract checksum from string
#    checksum=$(echo $checksum | awk '{print $1}')
#
#    if [[ $checksum != ${result_checksums[$i]} ]]
#    then
#        echo "${result_filenames[$i]}: incorrect checksum"
#        exit 1
#    fi
#done

# Check sizes instead checksums
for (( i = 0; i < ${#result_filenames[*]}; ++i ))
do
    # This works on any platform
    file_size=$(wc -c ${result_filenames[$i]})

    # Check exit code of the previous command
    if [[ $? != 0 ]]
    then
        exit 1
    fi

    # Extract size from string
    file_size=$(echo $file_size | awk '{print $1}')

    if [[ $file_size != ${result_sizes[$i]} ]]
    then
        echo "${result_filenames[$i]}: incorrect size"
        exit 1
    fi
done

