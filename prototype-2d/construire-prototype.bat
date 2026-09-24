@echo off
rem Compile le prototype 2D avec CMake (raylib est telecharge automatiquement la premiere fois),
rem puis le lance. Double-clique sur ce fichier.

cd /d "%~dp0"
set CLION=C:\Program Files\JetBrains\CLion 2026.2.2\bin
set PATH=%CLION%\mingw\bin;%PATH%

echo Preparation (la premiere fois, telechargement de raylib : ca peut prendre une minute)...
"%CLION%\cmake\win\x64\bin\cmake.exe" -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%CLION%\mingw\bin\mingw32-make.exe"
if errorlevel 1 goto erreur

echo Compilation...
"%CLION%\cmake\win\x64\bin\cmake.exe" --build build -j 8
if errorlevel 1 goto erreur

copy /Y "%CLION%\mingw\bin\libwinpthread-1.dll" build\ >nul

echo.
echo C'est pret ! Lancement du prototype...
start "" /D build build\vesperance_2d.exe
exit /b 0

:erreur
echo.
echo Quelque chose n'a pas marche : regarde les messages ci-dessus.
pause
exit /b 1
