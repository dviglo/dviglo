#!/usr/bin/env bash

if [ -f "$(dirname $0)/graph3d_pbr_materials" ]; then
    debug=""
else 
    debug="_d"
fi

$(dirname $0)/graph3d_pbr_materials${debug} -renderpath CoreData/RenderPaths/PBRDeferredHWDepth.xml $@
