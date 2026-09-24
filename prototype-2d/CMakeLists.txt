cmake_minimum_required(VERSION 3.20)
project(prophetia_2d)

set(CMAKE_CXX_STANDARD 20)

# raylib : la bibliotheque qui ouvre la fenetre, dessine et lit la souris et le clavier.
# CMake la telecharge tout seul (depuis le depot officiel) la premiere fois, puis la garde.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_MakeAvailable(raylib)

# Le prototype REUTILISE du code du jeu console (le dossier du dessus) :
# les structures de types.h, la classe Jauge, et les methodes de Combattant (dans outils.cpp).
add_executable(prophetia_2d
    main.cpp
    regles.cpp
    dessin.cpp
    sprites.cpp
    ../outils.cpp
    ../couleurs.cpp
    ../jauge.cpp
)
target_include_directories(prophetia_2d PRIVATE ..)
target_link_libraries(prophetia_2d raylib)

# Sous Windows : le jeu fonctionne sans les DLL du compilateur (sauf libwinpthread, copiee a cote)
if (MINGW)
    target_link_options(prophetia_2d PRIVATE -static-libgcc -static-libstdc++)
endif()
