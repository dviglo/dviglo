@echo off
set /p assetdir="Enter source assets dir: "
cd /d "%~dp0"
tool\ogre_importer.exe %assetdir%/Jack.mesh.xml Data/Models/Jack.mdl -t
tool\ogre_importer.exe %assetdir%/Level.mesh.xml Data/Models/NinjaSnowWar/Level.mdl -t
tool\ogre_importer.exe %assetdir%/Mushroom.mesh.xml Data/Models/Mushroom.mdl -t
tool\ogre_importer.exe %assetdir%/Ninja.mesh.xml Data/Models/NinjaSnowWar/Ninja.mdl -t
tool\ogre_importer.exe %assetdir%/Potion.mesh.xml Data/Models/NinjaSnowWar/Potion.mdl -t
tool\ogre_importer.exe %assetdir%/SnowBall.mesh.xml Data/Models/NinjaSnowWar/SnowBall.mdl -t
tool\ogre_importer.exe %assetdir%/SnowCrate.mesh.xml Data/Models/NinjaSnowWar/SnowCrate.mdl -t
