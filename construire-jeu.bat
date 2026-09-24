@echo off
rem Fabrique le dossier AYLIS-jeu : le jeu pret a jouer, sans CLion.
rem Double-clique sur ce fichier apres avoir modifie le code.

cd /d "%~dp0"
set MINGW=C:\Program Files\JetBrains\CLion 2026.2.2\bin\mingw\bin

if not exist AYLIS-jeu mkdir AYLIS-jeu

echo Compilation du jeu...
rem Les fichiers a compiler (les memes que dans CMakeLists.txt)
set FICHIERS=main.cpp outils.cpp progression.cpp marchands.cpp combat.cpp carte.cpp evenements.cpp couleurs.cpp accueil.cpp histoire.cpp sauvegarde.cpp jauge.cpp

"%MINGW%\g++.exe" -std=c++20 -O2 -static-libgcc -static-libstdc++ %FICHIERS% -o AYLIS-jeu\AYLIS.exe
if errorlevel 1 (
    echo.
    echo La compilation a echoue : regarde les erreurs ci-dessus.
    pause
    exit /b 1
)

copy /Y "%MINGW%\libwinpthread-1.dll" AYLIS-jeu\ >nul

echo.
echo C'est pret ! Double-clique sur AYLIS-jeu\AYLIS.exe pour jouer.
pause
