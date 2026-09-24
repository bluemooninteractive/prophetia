// types.h : les constantes et les structures du jeu, partagees par tous les fichiers
#pragma once

#include <string>
#include <vector>

// Les raretes des objets
const int COMMUN = 0;
const int RARE = 1;
const int EPIQUE = 2;

// Les types d'objets
const int OBJET_MATERIAU = 0;   // ne sert qu'a etre revendu
const int OBJET_ARME = 1;
const int OBJET_POTION = 2;     // va directement dans les potions d'AYLIS

// Les styles de combat des ennemis quand ils sont loin
const int STYLE_MELEE = 0;      // avance d'un pas par tour
const int STYLE_LANCEUR = 1;    // lance des javelots une fois sur deux
const int STYLE_CHARGEUR = 2;   // fonce au contact d'un coup et frappe

// Une arme : de melee (corps a corps) ou a distance
struct Arme {
    std::string nom;
    bool aDistance;         // true = arc, arbalete... / false = epee, hache...
    int bonusAttaque;       // s'ajoute a l'attaque d'AYLIS (peut etre negatif)
    int chanceCritique;     // sur 100 : 10 = 1 chance sur 10
    int nombreDeCoups;      // 2 = l'arme frappe deux fois par attaque
    int prix;               // le marchand la rachete a moitie prix
    int rarete = COMMUN;
};

// Un objet de l'inventaire
struct Objet {
    std::string nom;
    int type;
    int rarete;
    int valeur;             // le prix auquel le marchand l'achete
    Arme arme = {"", false, 0, 0, 1, 0};  // utilise seulement si c'est une arme
};

// Une ligne de la table de loot d'un ennemi
struct Butin {
    Objet objet;
    int chance;             // sur 100 : 30 = 30% de chances de tomber
};

// Un combattant regroupe toutes ses stats dans un seul "paquet"
struct Combattant {
    std::string nom;
    int pv;
    int pvMax;
    int attaque;
    int defense;
    int potions;
    bool estBoss;
    int xpDonne;            // l'XP que l'ennemi donne quand il est vaincu
    int orDonne;            // les pieces d'or que l'ennemi laisse tomber (en moyenne)
    std::vector<Butin> butin = {};      // la table de loot de l'ennemi
    int style = STYLE_MELEE;            // comment l'ennemi se comporte quand il est loin
    bool enrage = false;    // un boss s'enrage une fois quand il passe sous la moitie de ses pv
    bool etourdi = false;   // un ennemi paralyse passe son prochain tour
    int niveau = 1;
    int xp = 0;
    int points = 0;         // points de competence a depenser
    int pieces = 0;         // l'or ("or" est un mot reserve en C++, on ne peut pas l'utiliser)
    int mana = 10;
    int manaMax = 10;
    int sortsConnus = 1;    // 1 = Boule de feu, 2 = + Soin, 3 = + Eclair
    Arme arme = {"Epee courte", false, 0, 10, 1, 0};
    std::vector<Objet> inventaire = {};
};

const int rageMax = 100;
const int nombreDeSorts = 3;
const int distanceDepart = 2;       // 2 = loin, 1 = proche, 0 = au contact

// Un marchand : son nom, son metier et ses repliques
struct Marchand {
    std::string nom;
    std::string metier;
    std::vector<std::string> accueil;       // une replique d'accueil par halte
    std::vector<std::string> nouvelles;     // ce qu'il raconte quand on discute, par halte
    std::string merci;                      // quand on lui achete ou vend quelque chose
    std::string pasAssez;                   // quand AYLIS n'a pas assez d'or
    std::string auRevoir;
};
