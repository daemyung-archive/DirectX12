@echo off

FOR /F "tokens=*" %%g IN ('where.exe vcpkg') do (SET VCPKG_PATH=%%g)
SET CMAKE_TOOLCHAIN_FILE=%VCPKG_PATH:vcpkg.exe=scripts\buildsystems\vcpkg.cmake%

SET BUILD_DIR=build
MD %BUILD_DIR%
cmake . -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE%