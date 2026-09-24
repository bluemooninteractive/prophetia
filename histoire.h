// histoire.h : l'introduction et les differentes fins de l'aventure
#pragma once

#include "types.h"

// Raconte le debut de l'histoire
void afficherIntro();

// Raconte la fin : elle depend de l'honneur d'AYLIS et de son compagnon
void afficherFin(const EtatPartie& etat);
