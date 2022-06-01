@echo off
#set CURRENT_DIR=%~dp0
#set BUILD_DIR=build
#set PROJECT_NAME=vdc_process

#echo %CURRENT_DIR%%PROJECT_NAME%
#if not exist %CURRENT_DIR%%BUILD_DIR% (
#    mkdir %CURRENT_DIR%%BUILD_DIR%
#)
#cd %CURRENT_DIR%%BUILD_DIR%

rem Generate VS2017 project.
#cmake ../ -G "Visual Studio 15 2017 Win64"
python bootstrap.py win64
pause
