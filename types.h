// types.h : les constantes et les structures du jeu, partagees par tous les fichiers
#pragma once

#include <string>
#include <vector>
#include "jauge.h"

// Un "enum class" est un type qui ne peut prendre qu'une liste de valeurs precises.
// C'est plus sur qu'un simple int : on ne peut pas melanger une rarete et un style par erreur,
// et on ecrit Rarete::Rare au lieu d'un nombre qu'il faudrait retenir.

// Les raretes des objets
enum class Rarete {
    Commun,
    Rare,
    Epique,
};

// Les types d'objets
enum class TypeObjet {
    Materiau,   // ne sert qu'a etre revendu
    Arme,
    Potion,     // va directement dans les potions d'AYLIS
};

// Les styles de combat des ennemis quand ils sont loin
enum class Style {
    Melee,      // avance d'un pas par tour
    Lanceur,    // lance des javelots une fois sur deux
    Chargeur,   // fonce au contact d'un coup et frappe
};

// Une arme : de melee (corps a corps) ou a distance
struct Arme {
    std::string nom;
    bool aDistance;         // true = arc, arbalete... / false = epee, hache...
    int bonusAttaque;       // s'ajoute a l'attaque d'AYLIS (peut etre negatif)
    int chanceCritique;     // sur 100 : 10 = 1 chance sur 10
    int nombreDeCoups;      // 2 = l'arme frappe deux fois par attaque
    int prix;               // le marchand la rachete a moitie prix
    Rarete rarete = Rarete::Commun;
};

// Un objet de l'inventaire
struct Objet {
    std::string nom;
    TypeObjet type;
    Rarete rarete;
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
    Style style = Style::Melee;         // comment l'ennemi se comporte quand il est loin
    bool enrage = false;    // un boss s'enrage une fois quand il passe sous la moitie de ses pv
    bool etourdi = false;   // un ennemi paralyse passe son prochain tour
    int distance = 0;       // la distance entre cet ennemi et AYLIS (2 = loin, 0 = au contact)
    bool vaincu = false;    // true une fois que sa chute a ete annoncee
    int niveau = 1;
    int xp = 0;
    int points = 0;         // points de competence a depenser
    int pieces = 0;         // l'or ("or" est un mot reserve en C++, on ne peut pas l'utiliser)
    int mana = 10;
    int manaMax = 10;
    int sortsConnus = 1;    // 1 = Boule de feu, 2 = + Soin, 3 = + Eclair, 4 = + Bouclier
    Arme arme = {"Epee courte", false, 0, 10, 1, 0};
    std::vector<Objet> inventaire = {};

    // Les etats : le nombre de tours qu'il leur reste (0 = pas d'etat)
    int poison = 0;         // -3 pv par tour
    int brulure = 0;        // -4 pv par tour
    int saignement = 0;     // -3 pv par tour
    int bouclier = 0;       // absorbe ce nombre de points de degats
    bool attaquePoison = false;     // pour un ennemi : ses coups peuvent empoisonner

    int honneur = 0;        // les bons (+1) et mauvais (-1) choix d'AYLIS : ils decident de la fin
    std::string voie = "";  // la voie choisie au depart : "de l'epee", "de l'arc" ou "des arcanes"

    // Les methodes : des fonctions qui appartiennent au combattant.
    // On les appelle avec un point : aylis.soigner(10), ennemi.boirePotion(), aylis.estDebout()...
    // Elles sont ecrites dans outils.cpp.
    void soigner(int quantite);
    void boirePotion();
    bool estDebout() const;     // "const" : cette methode ne modifie pas le combattant
};

const int rageMax = 100;
const int nombreDeSorts = 4;
const int distanceDepart = 2;       // 2 = loin, 1 = proche, 0 = au contact

// Tout ce qui decrit une partie en cours : c'est ce qu'on ecrit dans le fichier de sauvegarde
struct EtatPartie {
    Combattant aylis;
    Combattant compagnon;           // le compagnon d'AYLIS, s'il y en a un
    bool avecCompagnon = false;
    Jauge rage = Jauge(rageMax);    // une classe : voir jauge.h
    int etape = 0;                  // l'etape de la route ou on en est
    int haltesVisitees = 0;         // les prix montent a chaque halte
    bool ashkaVaincue = false;
    bool skarnVaincu = false;
    int difficulte = 2;             // 1 = facile, 2 = normal, 3 = difficile
};

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
