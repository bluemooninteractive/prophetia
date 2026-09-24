// combat.h : le deroulement d'un combat
#pragma once

#include "types.h"

// Un combat complet contre un ennemi. Renvoie true si AYLIS gagne.
bool combattre(Combattant& aylis, Combattant& ennemi, int& rage);
