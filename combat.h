// combat.h : le deroulement d'un combat
#pragma once

#include <vector>
#include "types.h"

// Un combat complet contre un groupe d'ennemis (un seul ou plusieurs).
// Si avecCompagnon est vrai, le compagnon combat aux cotes d'AYLIS.
// Renvoie true si AYLIS gagne.
bool combattre(Combattant& aylis, std::vector<Combattant>& ennemis, int& rage,
               Combattant& compagnon, bool avecCompagnon);
