@echo off
set /p ASSETDIR="Enter source assets dir: "
cd /d "%~dp0"
tool/ogre_importer %ASSETDIR%/Jack.mesh.xml Data/Models/Jack.mdl -t
tool/ogre_importer %ASSETDIR%/Level.mesh.xml Data/Models/NinjaSnowWar/Level.mdl -t
tool/ogre_importer %ASSETDIR%/Mushroom.mesh.xml Data/Models/Mushroom.mdl -t
tool/ogre_importer %ASSETDIR%/Ninja.mesh.xml Data/Models/NinjaSnowWar/Ninja.mdl -t
tool/ogre_importer %ASSETDIR%/Potion.mesh.xml Data/Models/NinjaSnowWar/Potion.mdl -t
tool/ogre_importer %ASSETDIR%/SnowBall.mesh.xml Data/Models/NinjaSnowWar/SnowBall.mdl -t
tool/ogre_importer %ASSETDIR%/SnowCrate.mesh.xml Data/Models/NinjaSnowWar/SnowCrate.mdl -t
