// sprites.h : les dessins du jeu (pixel art), fabriques au demarrage a partir de petites grilles de texte
#pragma once

#include <string>
#include "raylib.h"

const int NOMBRE_LIEUX = 9;         // foret, village, gue, bois, camp, forteresse, col, cendres, citadelle

// Les images du jeu. Une "Texture2D" est une image chargee dans la carte graphique, prete a etre dessinee.
struct Sprites {
    Texture2D aylis;
    Texture2D epee;
    Texture2D arc;
    Texture2D baton;
    Texture2D couronne;
    Texture2D eclaireur;
    Texture2D guerrier;
    Texture2D traqueur;
    Texture2D chaman;
    Texture2D louvetier;
    Texture2D ashka;
    Texture2D skarn;
    Texture2D matriarche;
    Texture2D vorgath;
    Texture2D casque;               // la coiffe de Skarn
    Texture2D ramure;               // la coiffe de la Matriarche
    Texture2D marchands[3];         // Maren, Durgan et Silas
    // L'environnement de chaque lieu : [numero du lieu][variante] (voir lieux.cpp pour la liste des lieux)
    Texture2D lumineux[NOMBRE_LIEUX];       // la source de lumiere du lieu (champignon, lanterne, torche...)
    Texture2D sol[NOMBRE_LIEUX][2];
    Texture2D obstacle[NOMBRE_LIEUX][2];    // les "arbres" du lieu (arbres, saules, arbres morts, rochers...)
    Texture2D decor[NOMBRE_LIEUX][2];       // les petits details par terre (herbe, roseaux, feuilles, gravats...)
    Texture2D tonneau;
    Texture2D tente;
    Texture2D rocher;
    Texture2D feu[2];               // le feu de camp : 2 images qui alternent

    // Les icones des 9 actions, dans l'ordre de la barre d'actions
    Texture2D icones[9];
};

const int TAILLE_SPRITE = 16;       // chaque dessin fait 16 x 16 pixels, affiche 4 fois plus grand

// A appeler une fois, APRES InitWindow (il faut une fenetre pour creer des textures)
void chargerSprites();

// A appeler avant CloseWindow, pour liberer la memoire de la carte graphique
void dechargerSprites();

// Les sprites charges
const Sprites& sprites();

// Le sprite d'un Haschen, d'apres son nom ("Haschen chaman" -> le chaman)
const Texture2D& spriteHaschen(const std::string& nom);

// L'arme dessinee dans la main d'un pion (ou nullptr s'il n'en a pas)
const Texture2D* spriteArmeHaschen(const std::string& nom);

// Ce que porte un boss sur la tete : couronne, casque, ramure (ou nullptr)
const Texture2D* spriteCoiffe(const std::string& nom);
