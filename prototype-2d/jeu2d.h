// jeu2d.h : les reglages et les donnees du jeu en vue du dessus
//
// On REUTILISE les structures du jeu console (types.h, dans le dossier du dessus) :
// Combattant, Arme, les enum class, la classe Jauge... Un pion sur l'arene, c'est simplement
// un Combattant (ses stats, son arme, ses etats) + une position + une couleur.
#pragma once

#include <string>
#include <vector>
#include "raylib.h"
#include "types.h"

// ===================== Les reglages =====================

const int COLONNES = 12;
const int LIGNES = 8;
const int TAILLE_CASE = 72;                         // en pixels
const int LARGEUR_FENETRE = COLONNES * TAILLE_CASE;
const int HAUTEUR_ARENE = LIGNES * TAILLE_CASE;
const int HAUTEUR_PANNEAU = 190;                    // la barre d'actions et le journal, en bas
const int HAUTEUR_FENETRE = HAUTEUR_ARENE + HAUTEUR_PANNEAU;

const int DEPLACEMENT_AYLIS = 3;                    // cases par tour
const int PORTEE_MELEE = 1;
const int PORTEE_DISTANCE = 4;                      // arc, arbalete, baton...
const int PORTEE_SORT = 5;
const int PORTEE_LANCEUR = 4;                       // les Haschen qui tirent de loin
const float PAUSE_ENTRE_ENNEMIS = 0.5f;             // en secondes

// ===================== Les donnees =====================

// Un pion sur l'arene : AYLIS ou un Haschen
struct Pion {
    Combattant stats;       // pv, attaque, arme, etats... : la meme struct que dans le jeu console
    int colonne;
    int ligne;
    Color couleur;
    float flash = 0.0f;     // > 0 juste apres avoir ete touche : le pion clignote en rouge
};

// Les grandes etapes de l'ecran
enum class Phase {
    ChoixVoie,      // l'ecran de depart
    Deplacement,    // tour d'AYLIS : deplacement encore possible
    Action,         // tour d'AYLIS : deplacement fait, il reste l'action
    TourEnnemi,     // les Haschen jouent, un par un
    CombatGagne,    // entre deux combats
    Victoire,
    Defaite,
};

// Les actions d'AYLIS (la barre en bas de l'ecran)
enum class Action {
    Attaque,
    AttaqueLourde,
    Garde,
    Potion,
    BouleDeFeu,
    Soin,
    Eclair,
    Bouclier,
    Speciale,
};

// Un texte qui s'envole au-dessus d'un pion (les degats, les soins...), puis s'efface
struct TexteFlottant {
    std::string texte;
    float x;
    float y;
    float tempsRestant;
    Color couleur;
};

// Tout l'etat du jeu
struct Jeu {
    Pion aylis;
    std::vector<Pion> haschen;
    std::vector<bool> rochers;          // une case par position : true = rocher
    Phase phase = Phase::ChoixVoie;
    Action actionChoisie = Action::Attaque;
    Jauge rage = Jauge(rageMax);        // la classe du jeu console
    bool enGarde = false;               // attaque en garde ce tour-ci : les coups recus sont divises par 2
    int combat = 0;                     // le numero du combat en cours (0, 1, 2)
    std::string nomDuLieu;
    int ennemiQuiJoue = 0;
    float minuteur = 0.0f;
    int tour = 1;
    std::vector<std::string> journal;
    std::vector<TexteFlottant> textes;
};

// ===================== regles.cpp : les regles du jeu =====================

// La distance en cases, sans diagonales
int distanceCases(int c1, int l1, int c2, int l2);
int distanceEntre(const Pion& a, const Pion& b);
bool estDansArene(int colonne, int ligne);
bool estRocher(const Jeu& jeu, int colonne, int ligne);
int haschenSurCase(const Jeu& jeu, int colonne, int ligne);     // son numero, ou -1

// Les cases ou AYLIS peut aller ce tour-ci : pour chaque case, le nombre de pas (-1 = inaccessible)
std::vector<int> casesAtteignables(const Jeu& jeu);

// Les actions
std::string nomAction(Action action);
int porteeAction(const Jeu& jeu, Action action);        // 0 = sur soi-meme
int coutMana(Action action);
bool actionDisponible(const Jeu& jeu, Action action);   // assez de mana, de rage, de potions, sort connu...
bool actionSurSoi(Action action);                       // potion, soin, bouclier : pas besoin de cible

// Les grandes etapes
void choisirVoie(Jeu& jeu, int voie);                   // 1 = epee, 2 = arc, 3 = arcanes
void preparerCombat(Jeu& jeu);                          // place AYLIS et les Haschen du combat jeu.combat
void combatSuivant(Jeu& jeu);
int nombreDeCombats();

// Le tour d'AYLIS
void deplacerAylis(Jeu& jeu, int colonne, int ligne);
bool agirSurHaschen(Jeu& jeu, int numero);              // renvoie true si l'action a ete faite
bool agirSurSoi(Jeu& jeu);
void finirTourAylis(Jeu& jeu);

// Le tour des Haschen (appelee a chaque image)
void mettreAJourTourEnnemi(Jeu& jeu, float secondes);

void ecrireJournal(Jeu& jeu, const std::string& message);
void mettreAJourTextes(Jeu& jeu, float secondes);

// ===================== dessin.cpp : l'affichage =====================

void dessinerJeu(const Jeu& jeu, int colonneSouris, int ligneSouris);

// Les boutons de la barre d'actions : leur rectangle a l'ecran
Rectangle rectangleBouton(int numero);
const std::vector<Action>& actionsDeLaBarre();
Rectangle rectangleCarteVoie(int voie);                 // les 3 cartes de l'ecran de depart
