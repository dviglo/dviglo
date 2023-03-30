@echo off
setlocal
set "dirname=%~dp0"
if %0 == "%~dpnx0" where /q "%cd%:%~nx0" && set "dirname=%cd%\"
if exist "%dirname%graph3d_pbr_materials.exe" (set "debug=") else (set "debug=_d")
"%dirname%graph3d_pbr_materials%debug%.exe" -renderpath core_data/render_paths/pbr_deferred.xml %*
