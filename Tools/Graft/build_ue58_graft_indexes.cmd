@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_ue58_graft_indexes.ps1" %*
exit /b %errorlevel%
