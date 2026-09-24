// marchands.h : la halte et ses trois marchands
#pragma once

#include <vector>
#include "types.h"

// Ou en est l'histoire : les marchands n'ont pas les memes repliques
const int MOMENT_DEBUT = 0;           // le debut de la route
const int MOMENT_AVANT_ASHKA = 1;
const int MOMENT_APRES_ASHKA = 2;
const int MOMENT_AVANT_VORGATH = 3;
const int MOMENT_AVANT_SKARN = 4;

// La halte. "visite" = combien de haltes AYLIS a deja faites (0 a la premiere) : les prix montent.
// "moment" = un des MOMENT_... ci-dessus.
void halte(Combattant& aylis, const std::vector<Arme>& armes, int visite, int moment);
