@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0apply_graft_orbis_patch.ps1" %*
exit /b %errorlevel%
