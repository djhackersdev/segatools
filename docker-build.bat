@echo off
setlocal enabledelayedexpansion

:: Static Environment Variables
set BUILD_OUTPUT_PATH=build\docker
set IMAGE_NAME=djhackers/segatools-build:latest
set CONTAINER_NAME=segatools-build

:: Main Execution
docker build . -t %IMAGE_NAME%

if ERRORLEVEL 1 (
    goto failure
)

docker create --name %CONTAINER_NAME% %IMAGE_NAME%

if ERRORLEVEL 1 (
    goto failure
)

rd /s /q "!BUILD_OUTPUT_PATH!"
mkdir "!BUILD_OUTPUT_PATH!"

docker cp %CONTAINER_NAME%:/segatools/build/zip %BUILD_OUTPUT_PATH%

docker rm -f %CONTAINER_NAME% > nul

goto success

:failure
echo segatools Docker build FAILED!
goto finish

:success
echo segatools Docker build completed successfully.
goto finish

:finish
pause
