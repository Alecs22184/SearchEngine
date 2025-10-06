@echo off
cd /d "%~dp0"
chcp 65001 >nul
title Search Engine Launcher

echo ====================================
echo Search Engine - Project Runner
echo ====================================

:menu
echo.
echo Select option:
echo 1 - Run Spider (index websites)
echo 2 - Run Search Server  
echo 3 - Run Both (in separate windows)
echo 4 - Check Database Connection
echo 5 - View Configuration
echo 6 - Update Config Location
echo 7 - Exit
echo.
set /p choice="Enter your choice (1-7): "

if "%choice%"=="" goto menu
if "%choice%"=="1" goto spider
if "%choice%"=="2" goto server
if "%choice%"=="3" goto both
if "%choice%"=="4" goto check_db
if "%choice%"=="5" goto view_config
if "%choice%"=="6" goto update_config
if "%choice%"=="7" goto exit

echo Invalid choice! Please try again.
goto menu

:spider
echo.
echo Starting Spider...
if not exist "build\Release\spider.exe" (
    echo ERROR: spider.exe not found in build\Release\
    echo Please build the project first using build_fix.bat
    pause
    goto menu
)
if not exist "config.ini" (
    echo ERROR: config.ini not found!
    echo Please create config.ini file first
    pause
    goto menu
)
copy "config.ini" "build\Release\" >nul
cd build\Release
echo Running spider from: %CD%
spider.exe
cd ..\..
echo.
pause
goto menu

:server
echo.
echo Starting Search Server...
if not exist "build\Release\search_server.exe" (
    echo ERROR: search_server.exe not found in build\Release\
    echo Please build the project first using build_fix.bat
    pause
    goto menu
)
if not exist "config.ini" (
    echo ERROR: config.ini not found!
    echo Please create config.ini file first
    pause
    goto menu
)
copy "config.ini" "build\Release\" >nul
cd build\Release
echo Running server from: %CD%
search_server.exe
cd ..\..
echo.
pause
goto menu

:both
echo.
echo Starting both applications in separate windows...
if not exist "build\Release\spider.exe" (
    echo ERROR: spider.exe not found!
    pause
    goto menu
)
if not exist "build\Release\search_server.exe" (
    echo ERROR: search_server.exe not found!
    pause
    goto menu
)
if not exist "config.ini" (
    echo ERROR: config.ini not found!
    pause
    goto menu
)
copy "config.ini" "build\Release\" >nul
start "Search Engine Spider" cmd /k "cd /d %~dp0build\Release && spider.exe"
timeout /t 2 >nul
start "Search Engine Server" cmd /k "cd /d %~dp0build\Release && search_server.exe"
echo Both applications started in separate windows.
echo.
pause
goto menu

:check_db
echo.
echo Checking Database Configuration...
if exist "config.ini" (
    echo Database settings from config.ini:
    echo ================================
    type config.ini | findstr /i "database"
) else (
    echo config.ini not found in current directory!
)
echo.
pause
goto menu

:view_config
echo.
echo Current Configuration:
echo =====================
if exist "config.ini" (
    type config.ini
) else (
    echo config.ini not found!
)
echo.
pause
goto menu

:update_config
echo.
echo Updating config.ini location...
if exist "config.ini" (
    copy "config.ini" "build\Release\" >nul
    echo config.ini copied to build\Release\
) else (
    echo config.ini not found in current directory!
)
echo.
pause
goto menu

:exit
echo.
echo Thank you for using Search Engine!
echo.
timeout /t 2 >nul
exit