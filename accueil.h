// accueil.h : l'ecran titre et les regles du jeu
#pragma once

// Affiche l'ecran titre et renvoie le choix :
// 1 = nouvelle partie, 2 = continuer la partie sauvegardee, 3 = regles, 4 = quitter
int ecranTitre(bool sauvegardeDisponible);

// Affiche les regles du jeu, page par page
void afficherRegles();
