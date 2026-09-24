// outils.h : les petites fonctions utiles partout (menus, degats, affichage, soins)
#pragma once

#include <string>
#include "types.h"

// Lit un choix entre min et max. Redemande tant que ce n'est pas valide.
int lireChoix(int min, int max);

// Calcule les degats d'une attaque (puissance et chanceCritique en pourcentage)
int calculerDegats(int attaque, int puissance, int defense, int chanceCritique);

// Affiche une barre comme [##########----------]
// Sans couleur, c'est une barre de vie (verte, jaune puis rouge). Exemple : afficherBarre(mana, manaMax, BLEU);
void afficherBarre(int valeur, int maximum, const std::string& couleur = "");

// Affichages des armes et des objets
void afficherArme(const Arme& arme);
std::string nomRarete(int rarete);
Objet objetDepuisArme(const Arme& arme);
void afficherObjet(const Objet& objet);

// Les soins
void soigner(Combattant& c, int quantite);
void boirePotion(Combattant& c);

// Attend que le joueur appuie sur Entree (pour lui laisser le temps de lire)
void attendreEntree();
