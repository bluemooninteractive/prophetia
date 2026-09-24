// main.cpp : la preparation de la partie (heros, armes, ennemis, difficulte), puis la route
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "types.h"
#include "outils.h"
#include "couleurs.h"
#include "carte.h"
#include "accueil.h"

// Applique la difficulte a un ennemi (pourcentages : 125 = +25%)
void appliquerDifficulte(Combattant& ennemi, int forceEnnemis, int orEnnemis) {
    ennemi.pvMax = ennemi.pvMax * forceEnnemis / 100;
    ennemi.pv = ennemi.pvMax;
    ennemi.attaque = ennemi.attaque * forceEnnemis / 100;
    ennemi.orDonne = ennemi.orDonne * orEnnemis / 100;
}

// Une partie complete : on prepare le heros, les armes et les ennemis, puis on part sur la route
void jouerPartie() {
    //                   nom            pv  pvMax att  def potions boss   xp  or
    Combattant aylis = {"AYLIS",        40, 40,   12,  4,  3,      false, 0,  0};

    // Les armes vendues par Durgan
    //                       nom                 distance  bonus crit coups prix
    std::vector<Arme> armes = {
        {"Epee courte",         false,  0, 10, 1,  20},
        {"Hache de guerre",     false,  4, 20, 1,  60},
        {"Dagues jumelles",     false, -3, 10, 2,  50},
        {"Marteau de siege",    false,  7,  5, 1,  80},
        {"Arc court",           true,   0, 15, 1,  45},
        {"Arbalete",            true,   5, 10, 1,  75},
        {"Couteaux de lancer",  true,  -3, 15, 2,  55},
    };

    // Les objets qu'on peut trouver sur les ennemis
    //                 nom                      type            rarete  valeur
    Objet croc     = {"Croc de Haschen",        OBJET_MATERIAU, COMMUN,  8};
    Objet peau     = {"Peau de Haschen",        OBJET_MATERIAU, COMMUN, 12};
    Objet totem    = {"Totem de chaman",        OBJET_MATERIAU, RARE,   30};
    Objet griffe   = {"Griffe de berserker",    OBJET_MATERIAU, RARE,   25};
    Objet osGraves = {"Os graves de runes",     OBJET_MATERIAU, RARE,   25};
    Objet couronne = {"Couronne d'os d'Ashka",  OBJET_MATERIAU, EPIQUE, 70};
    Objet potion   = {"Potion",                 OBJET_POTION,   COMMUN,  7};

    // Les armes qu'on peut trouver sur les ennemis
    //                                  nom                   distance bonus crit coups prix rarete
    Objet arcDOs         = objetDepuisArme({"Arc d'os",           true,   3, 20, 1,  60, RARE});
    Objet lance          = objetDepuisArme({"Lance de Haschen",   false,  3, 15, 1,  60, RARE});
    Objet hacheBerserker = objetDepuisArme({"Hache du berserker", false,  6, 15, 1,  80, RARE});
    Objet javelots       = objetDepuisArme({"Javelots d'Ashka",   true,   6, 20, 1, 120, EPIQUE});

    // Les ennemis. Apres l'XP et l'or : la table de loot (chaque objet avec sa chance sur 100), puis le style.
    Bestiaire bestiaire;

    // Les Haschen des chemins normaux
    bestiaire.normaux = {
        {"Haschen eclaireur",   18, 18,  8, 1, 0, false,  20, 20,
            {{croc, 70}, {peau, 30}, {potion, 20}, {arcDOs, 10}}},
        {"Haschen guerrier",    22, 22,  9, 2, 0, false,  25, 25,
            {{croc, 60}, {peau, 50}, {potion, 20}, {lance, 12}}},
        {"Haschen traqueur",    20, 20,  9, 1, 0, false,  25, 22,
            {{croc, 50}, {peau, 40}, {potion, 20}, {arcDOs, 12}}, STYLE_LANCEUR},
        {"Haschen chaman",      20, 20, 10, 1, 0, false,  25, 25,
            {{croc, 40}, {totem, 30}, {potion, 40}}},
    };

    // Les Haschen d'elite : plus durs, avec un objet RARE garanti
    bestiaire.elites = {
        {"Haschen berserker",   30, 30, 15, 5, 0, false,  35, 35,
            {{griffe, 100}, {croc, 50}, {hacheBerserker, 20}}},
        {"Haschen brise-os",    34, 34, 13, 6, 0, false,  35, 35,
            {{osGraves, 100}, {peau, 60}, {lance, 20}}},
    };

    // Les boss
    bestiaire.ashka = {"Ashka, Matriarche des Haschen", 40, 40, 14, 4, 1, true, 50, 50,
        {{couronne, 100}, {javelots, 100}, {potion, 50}}, STYLE_LANCEUR};
    bestiaire.vorgath = {"Vorgath le Destructeur", 60, 60, 17, 6, 2, true, 100, 0,
        {}, STYLE_CHARGEUR};

    std::cout << "\n" << colorer("=== Une nouvelle partie commence ===", JAUNE + GRAS) << "\n";
    std::cout << "Les Haschen deferlent sur la vallee, menes par Vorgath le Destructeur.\n";
    std::cout << "AYLIS prend la route vers sa forteresse. Personne d'autre n'ose y aller.\n";

    // Le choix de l'arme de depart : melee ou distance
    std::cout << "\nChoisis l'arme de depart d'AYLIS :\n";
    std::cout << "1. ";
    afficherArme(armes[0]);
    std::cout << "\n   -> doit aller au contact, mais frappe fort une fois la-bas\n";
    std::cout << "2. ";
    afficherArme(armes[4]);
    std::cout << "\n   -> tire pendant que les Haschen approchent, mais moins efficace au contact\n";
    if (lireChoix(1, 2) == 1) {
        aylis.arme = armes[0];
    } else {
        aylis.arme = armes[4];
    }
    std::cout << "AYLIS part avec : " << aylis.arme.nom << ".\n";

    // Le choix de la difficulte : change la force des ennemis et l'or qu'ils donnent
    std::cout << "\nChoisis la difficulte :\n";
    std::cout << "1. Facile     (ennemis -20% pv et attaque, +20% d'or)\n";
    std::cout << "2. Normal\n";
    std::cout << "3. Difficile  (ennemis +25% pv et attaque, -20% d'or)\n";
    int difficulte = lireChoix(1, 3);

    int forceEnnemis = 100;     // en pourcentage
    int orEnnemis = 100;
    if (difficulte == 1) {
        forceEnnemis = 80;
        orEnnemis = 120;
    } else if (difficulte == 3) {
        forceEnnemis = 125;
        orEnnemis = 80;
    }

    // On applique la difficulte a tous les ennemis. Le & modifie le vrai ennemi de la liste.
    for (Combattant& ennemi : bestiaire.normaux) {
        appliquerDifficulte(ennemi, forceEnnemis, orEnnemis);
    }
    for (Combattant& ennemi : bestiaire.elites) {
        appliquerDifficulte(ennemi, forceEnnemis, orEnnemis);
    }
    appliquerDifficulte(bestiaire.ashka, forceEnnemis, orEnnemis);
    appliquerDifficulte(bestiaire.vorgath, forceEnnemis, orEnnemis);

    // En route !
    if (parcourirCarte(aylis, armes, bestiaire)) {
        std::cout << "\n" << colorer("=== VICTOIRE TOTALE ! ===", VERT + GRAS) << "\n";
        std::cout << "Les Haschen sont en deroute : AYLIS a abattu Vorgath le Destructeur !\n";
    }
    // Sinon, le GAME OVER est deja affiche

    attendreEntree();   // on laisse le temps de lire la fin avant de revenir au menu
}

int main() {
    // Pour que le hasard change a chaque partie
    std::srand(std::time(nullptr));
    activerCouleurs();

    // L'ecran d'accueil, jusqu'a ce que le joueur choisisse de quitter
    while (true) {
        int choix = ecranTitre();
        if (choix == 1) {
            jouerPartie();
        } else if (choix == 2) {
            afficherRegles();
        } else {
            std::cout << "\nA bientot sur la route, AYLIS !\n";
            return 0;
        }
    }
}
