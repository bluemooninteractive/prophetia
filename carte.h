// carte.h : la route vers Vorgath, avec des choix de chemin a chaque etape
#pragma once

#include <vector>
#include "types.h"

// Tous les ennemis de l'aventure, ranges par categorie
struct Bestiaire {
    std::vector<Combattant> normaux;    // tires au hasard sur les chemins normaux
    std::vector<Combattant> elites;     // tires au hasard sur les chemins d'elite
    Combattant ashka;                   // premier boss
    Combattant skarn;                   // deuxieme boss
    Combattant vorgath;                 // boss final
};

// Parcourt la carte a partir de etat.etape (0 pour une nouvelle partie), en sauvegardant a chaque etape.
// Renvoie true si AYLIS abat Vorgath.
bool parcourirCarte(EtatPartie& etat, const std::vector<Arme>& armes, const Bestiaire& bestiaire);
