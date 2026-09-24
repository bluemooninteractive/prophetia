// progression.h : le loot, l'XP et les points de competence
#pragma once

#include "types.h"

// Ramasse l'or et les objets d'un ennemi vaincu
void ramasserButin(Combattant& aylis, const Combattant& ennemi);

// L'XP qu'il faut pour passer au niveau suivant
int xpPourNiveauSuivant(int niveau);

// Donne de l'XP a AYLIS (chaque niveau gagne donne 2 points de competence)
void gagnerXp(Combattant& aylis, int xpGagne);

// Le menu pour depenser les points de competence
void depenserPoints(Combattant& aylis);
