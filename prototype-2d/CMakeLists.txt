cmake_minimum_required(VERSION 3.20)
project(vesperance_2d)

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
add_executable(vesperance_2d
    main.cpp
    regles.cpp
    route.cpp
    haltes.cpp
    dessin.cpp
    sprites.cpp
    animations.cpp
    ../outils.cpp
    ../couleurs.cpp
    ../jauge.cpp
)
target_include_directories(vesperance_2d PRIVATE ..)
target_link_libraries(vesperance_2d raylib)

# Sous Windows : le jeu fonctionne sans les DLL du compilateur (sauf libwinpthread, copiee a cote)
if (MINGW)
    target_link_options(vesperance_2d PRIVATE -static-libgcc -static-libstdc++)
endif()
