@echo off
set /p assetdir="Enter source assets dir: "
cd /d "%~dp0"
tool\ogre_importer.exe %assetdir%/jack.mesh.xml data/models/jack.mdl -t
tool\ogre_importer.exe %assetdir%/level.mesh.xml data/models/ninja_snow_war/level.mdl -t
tool\ogre_importer.exe %assetdir%/mushroom.mesh.xml data/models/mushroom.mdl -t
tool\ogre_importer.exe %assetdir%/ninja.mesh.xml data/models/ninja_snow_war/ninja.mdl -t
tool\ogre_importer.exe %assetdir%/potion.mesh.xml data/models/ninja_snow_war/potion.mdl -t
tool\ogre_importer.exe %assetdir%/snowball.mesh.xml data/models/ninja_snow_war/snowball.mdl -t
tool\ogre_importer.exe %assetdir%/snow_crate.mesh.xml data/models/ninja_snow_war/snow_crate.mdl -t
