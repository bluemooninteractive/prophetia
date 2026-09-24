// sauvegarde.h : enregistrer la partie dans un fichier, et la reprendre plus tard
#pragma once

#include "types.h"

// Est-ce qu'une partie sauvegardee existe ?
bool sauvegardeExiste();

// Ecrit toute la partie dans le fichier de sauvegarde
void sauvegarder(const EtatPartie& etat);

// Relit la partie depuis le fichier. Renvoie false si le fichier est absent ou abime.
bool charger(EtatPartie& etat);

// Supprime la sauvegarde (a la fin d'une partie, gagnee ou perdue)
void effacerSauvegarde();
