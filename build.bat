@echo off
setlocal
cd /d "%~dp0"

set "GAME=%~1"
if "%GAME%"=="" set "GAME=C:\Program Files (x86)\Vivendi Universal Games\The Simpsons Hit & Run"
if "%DEVKITPRO_WIN%"=="" set "DEVKITPRO_WIN=C:\devkitPro"

python tools\3ds\setup_sd.py "%GAME%" || exit /b 1

set MSYSTEM=MSYS
set CHERE_INVOKING=1
"%DEVKITPRO_WIN%\msys2\usr\bin\bash.exe" -lc "cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DCMAKE_BUILD_TYPE=Release -DSRR2_P3D_PDDI=PICA -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && cmake --build build" || exit /b 1

copy /y build\SimpsonsNTSC.3dsx sdcard\3ds\simpsons\ >nul || exit /b 1
echo.
echo Done: copy sdcard\3ds to the root of the SD card.
