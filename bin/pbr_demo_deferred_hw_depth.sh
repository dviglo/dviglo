#!/usr/bin/env bash

if [ -f "$(dirname $0)/graph3d_pbr_materials_d" ]; then
    debug="_d"
else 
    debug=""
fi

$(dirname $0)/graph3d_pbr_materials${debug} -renderpath CoreData/RenderPaths/PBRDeferredHWDepth.xml $@
