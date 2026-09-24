// animations.h : tout ce qui bouge a l'ecran (deplacements, bonds, projectiles, particules, secousses)
//
// Les regles (regles.cpp) calculent tout IMMEDIATEMENT : les degats sont deja retires.
// Les animations ne font que MONTRER ce qui s'est passe, avec un petit "delai" :
// par exemple, le chiffre des degats n'apparait que quand la fleche arrive sur sa cible.
#pragma once

#include "jeu2d.h"

// La position d'un pion a l'ecran, en pixels (le centre de sa case, plus son bond et son recul)
Vector2 positionAffichee(const Pion& pion);

// Une attaque : un bond vers la cible (corps a corps) ou un projectile qui vole jusqu'a elle.
// Renvoie le delai avant l'impact, en secondes.
float animerAttaque(Jeu& jeu, Pion& attaquant, const Pion& cible, bool aDistance, SorteProjectile sorte);

// L'impact d'un coup sur un pion (-1 = AYLIS, sinon le numero du Haschen) : etincelles, recul, secousse...
void animerImpact(Jeu& jeu, int numeroPion, float delai, bool critique, Color couleur);

void animerExplosion(Jeu& jeu, const Pion& cible, float delai);
void animerEclair(Jeu& jeu, const Pion& cible);
void animerSoin(Jeu& jeu, const Pion& pion);
void animerBouclier(Jeu& jeu, const Pion& pion);
void animerEtat(Jeu& jeu, const Pion& pion, Color couleur);      // poison, brulure... qui font effet
void animerChute(Jeu& jeu, Pion& pion, float delai);             // un Haschen vaincu se dissout

// A appeler a chaque image
void mettreAJourAnimations(Jeu& jeu, float secondes);

// Est-ce qu'une animation importante est encore en cours ? (on attend la fin avant d'afficher "Victoire")
bool animationsEnCours(const Jeu& jeu);

// Le dessin des effets
void dessinerParticules(const Jeu& jeu);
void dessinerProjectiles(const Jeu& jeu);
void dessinerEclairs(const Jeu& jeu);

// La camera qui tremble (a utiliser avec BeginMode2D)
Camera2D cameraAvecSecousse(const Jeu& jeu);
