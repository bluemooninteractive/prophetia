// marchands.h : la halte entre deux combats et ses trois marchands
#pragma once

#include <vector>
#include "types.h"

// La halte entre deux combats. "visite" = combien de haltes AYLIS a deja faites (0 a la premiere).
void halte(Combattant& aylis, const std::vector<Arme>& armes, int visite);
