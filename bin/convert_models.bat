@echo off
set /p assetdir="Enter source assets dir: "
cd /d "%~dp0"
tool\ogre_importer.exe %assetdir%/Jack.mesh.xml data/Models/Jack.mdl -t
tool\ogre_importer.exe %assetdir%/Level.mesh.xml data/Models/NinjaSnowWar/Level.mdl -t
tool\ogre_importer.exe %assetdir%/Mushroom.mesh.xml data/Models/Mushroom.mdl -t
tool\ogre_importer.exe %assetdir%/Ninja.mesh.xml data/Models/NinjaSnowWar/Ninja.mdl -t
tool\ogre_importer.exe %assetdir%/Potion.mesh.xml data/Models/NinjaSnowWar/Potion.mdl -t
tool\ogre_importer.exe %assetdir%/SnowBall.mesh.xml data/Models/NinjaSnowWar/SnowBall.mdl -t
tool\ogre_importer.exe %assetdir%/SnowCrate.mesh.xml data/Models/NinjaSnowWar/SnowCrate.mdl -t
