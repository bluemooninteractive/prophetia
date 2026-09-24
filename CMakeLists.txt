cmake_minimum_required(VERSION 3.20)
project(turn_based)

set(CMAKE_CXX_STANDARD 20)

add_executable(turn_based
    main.cpp
    outils.cpp
    progression.cpp
    marchands.cpp
    combat.cpp
    carte.cpp
    evenements.cpp
    couleurs.cpp
    accueil.cpp
    histoire.cpp
    sauvegarde.cpp
)
