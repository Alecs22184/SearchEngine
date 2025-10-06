@echo off
setlocal EnableDelayedExpansion

echo ===============================================
echo Search Engine Build Fix for OpenSSL Issue
echo ===============================================

echo Checking vcpkg installation...
if not exist "C:\vcpkg\installed\x64-windows\include\openssl" (
    echo ERROR: OpenSSL not found in vcpkg!
    echo Please install OpenSSL first:
    echo C:\vcpkg\vcpkg install openssl:x64-windows
    pause
    exit /b 1
)

echo.
echo OpenSSL found in vcpkg directory.
echo.

echo Cleaning previous build...
rmdir /s /q build 2>nul
mkdir build

cd build

echo.
echo Configuring project with explicit OpenSSL paths...
echo.

set VCPKG_ROOT=C:\vcpkg
set OPENSSL_ROOT_DIR=%VCPKG_ROOT%\installed\x64-windows
set OPENSSL_INCLUDE_DIR=%OPENSSL_ROOT_DIR%\include
set OPENSSL_CRYPTO_LIBRARY=%OPENSSL_ROOT_DIR%\lib\libcrypto.lib
set OPENSSL_SSL_LIBRARY=%OPENSSL_ROOT_DIR%\lib\libssl.lib

echo VCPKG_ROOT: %VCPKG_ROOT%
echo OPENSSL_ROOT_DIR: %OPENSSL_ROOT_DIR%
echo OPENSSL_INCLUDE_DIR: %OPENSSL_INCLUDE_DIR%
echo OPENSSL_CRYPTO_LIBRARY: %OPENSSL_CRYPTO_LIBRARY%
echo OPENSSL_SSL_LIBRARY: %OPENSSL_SSL_LIBRARY%

echo.
echo Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DOPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR% ^
  -DOPENSSL_INCLUDE_DIR=%OPENSSL_INCLUDE_DIR% ^
  -DOPENSSL_CRYPTO_LIBRARY=%OPENSSL_CRYPTO_LIBRARY% ^
  -DOPENSSL_SSL_LIBRARY=%OPENSSL_SSL_LIBRARY%

if %errorlevel% neq 0 (
    echo.
    echo CMake configuration failed!
    echo.
    echo Troubleshooting:
    echo 1. Check if OpenSSL is installed: dir C:\vcpkg\installed\x64-windows\include\openssl
    echo 2. Check if lib files exist: dir C:\vcpkg\installed\x64-windows\lib\*ssl* *crypto*
    echo 3. Try reinstalling OpenSSL: C:\vcpkg\vcpkg remove openssl:x64-windows && C:\vcpkg\vcpkg install openssl:x64-windows
    pause
    exit /b %errorlevel%
)

echo.
echo CMake configuration successful!
echo.

echo Building project...
cmake --build . --config Release --parallel

if %errorlevel% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo.
echo ===============================================
echo Build completed successfully!
echo Executables are in: build\Release\
echo ===============================================
echo.

cd ..
pause