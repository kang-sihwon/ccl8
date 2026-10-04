@echo off
setlocal
node "%~dp0graft_ue58_federated_mcp.cjs" %*
exit /b %errorlevel%
