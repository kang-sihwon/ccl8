@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0graphify_ccl8.ps1" %*
exit /b %ERRORLEVEL%
