@echo off

pushd %~dp0

.\inject.exe -k .\idzhook.dll .\InitialD0_DX11_Nu.exe
.\inject.exe -d -k .\idzhook.dll .\amdaemon.exe -c configDHCP_Final_Common.json configDHCP_Final_JP.json configDHCP_Final_JP_ST1.json configDHCP_Final_JP_ST2.json configDHCP_Final_EX.json configDHCP_Final_EX_ST1.json configDHCP_Final_EX_ST2.json

echo.
echo Game processes have terminated
pause