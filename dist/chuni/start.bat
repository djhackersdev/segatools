@echo off

start /min inject -d -k chunihook.dll aimeReaderHost.exe -p 12
inject -d -k chunihook.dll chuniApp.exe
taskkill /f /im aimeReaderHost.exe > nul 2>&1

echo.
echo Game processes have terminated
pause