@echo off
set /p assetdir="Enter source assets dir: "
cd /d "%~dp0"
tool\ogre_importer.exe %assetdir%/Jack.mesh.xml data/models/Jack.mdl -t
tool\ogre_importer.exe %assetdir%/Level.mesh.xml data/models/NinjaSnowWar/Level.mdl -t
tool\ogre_importer.exe %assetdir%/Mushroom.mesh.xml data/models/Mushroom.mdl -t
tool\ogre_importer.exe %assetdir%/Ninja.mesh.xml data/models/NinjaSnowWar/Ninja.mdl -t
tool\ogre_importer.exe %assetdir%/Potion.mesh.xml data/models/NinjaSnowWar/Potion.mdl -t
tool\ogre_importer.exe %assetdir%/SnowBall.mesh.xml data/models/NinjaSnowWar/SnowBall.mdl -t
tool\ogre_importer.exe %assetdir%/SnowCrate.mesh.xml data/models/NinjaSnowWar/SnowCrate.mdl -t
