@echo off
set /p assetdir="Enter source assets dir: "
cd /d "%~dp0"
tool\ogre_importer.exe %assetdir%/Jack.mesh.xml data/models/Jack.mdl -t
tool\ogre_importer.exe %assetdir%/Level.mesh.xml data/models/ninja_snow_war/Level.mdl -t
tool\ogre_importer.exe %assetdir%/Mushroom.mesh.xml data/models/Mushroom.mdl -t
tool\ogre_importer.exe %assetdir%/Ninja.mesh.xml data/models/ninja_snow_war/Ninja.mdl -t
tool\ogre_importer.exe %assetdir%/Potion.mesh.xml data/models/ninja_snow_war/Potion.mdl -t
tool\ogre_importer.exe %assetdir%/SnowBall.mesh.xml data/models/ninja_snow_war/SnowBall.mdl -t
tool\ogre_importer.exe %assetdir%/SnowCrate.mesh.xml data/models/ninja_snow_war/SnowCrate.mdl -t
