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
#include "histoire.h"
#include "sauvegarde.h"

// Applique la difficulte a un ennemi (pourcentages : 125 = +25%)
void appliquerDifficulte(Combattant& ennemi, int forceEnnemis, int orEnnemis) {
    ennemi.pvMax = ennemi.pvMax * forceEnnemis / 100;
    ennemi.pv = ennemi.pvMax;
    ennemi.attaque = ennemi.attaque * forceEnnemis / 100;
    ennemi.orDonne = ennemi.orDonne * orEnnemis / 100;
}

// Le choix de la voie au depart : elle donne les stats, l'arme et les sorts d'AYLIS
Combattant choisirVoie(const std::vector<Arme>& armes) {
    std::cout << "\nChoisis la voie d'AYLIS :\n\n";
    std::cout << "1. " << colorer("La voie de l'epee", JAUNE + GRAS) << "   44 pv, attaque 13, defense 4, peu de mana\n";
    std::cout << "   -> solide au corps a corps. Epee courte.\n";
    std::cout << "2. " << colorer("La voie de l'arc", VERT + GRAS) << "    40 pv, attaque 12, defense 4\n";
    std::cout << "   -> tire pendant que les Haschen approchent. Arc court.\n";
    std::cout << "3. " << colorer("La voie des arcanes", VIOLET + GRAS) << " 34 pv, attaque 12, defense 3, beaucoup de mana\n";
    std::cout << "   -> fragile, mais connait deja Boule de feu ET Soin. Baton de mage.\n";

    //                   nom      pv  pvMax att def potions boss   xp or
    Combattant aylis = {"AYLIS",  40, 40,   12,  4,  3,      false, 0, 0};

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        aylis.voie = "de l'epee";
        aylis.pv = 44;
        aylis.pvMax = 44;
        aylis.attaque = 13;
        aylis.defense = 4;
        aylis.mana = 6;
        aylis.manaMax = 6;
        aylis.arme = armes[0];
    } else if (choix == 2) {
        aylis.voie = "de l'arc";
        aylis.arme = armes[4];
    } else {
        aylis.voie = "des arcanes";
        aylis.pv = 34;
        aylis.pvMax = 34;
        aylis.defense = 3;
        aylis.potions = 2;
        aylis.mana = 20;
        aylis.manaMax = 20;
        aylis.sortsConnus = 2;
        //            nom              distance bonus crit coups prix
        aylis.arme = {"Baton de mage", true,    0,    5,   1,    20};
    }
    std::cout << "AYLIS suit la voie " << aylis.voie << ", avec : " << aylis.arme.nom << ".\n";
    return aylis;
}

// Le nombre du defi du jour : la date d'aujourd'hui, par exemple 20260925.
// Tout le monde a le meme nombre le meme jour, donc les memes tirages au hasard.
unsigned int nombreDuJour() {
    std::time_t maintenant = std::time(nullptr);
    std::tm* date = std::localtime(&maintenant);
    return (date->tm_year + 1900) * 10000 + (date->tm_mon + 1) * 100 + date->tm_mday;
}

// Une partie : on prepare les armes et les ennemis, puis on part sur la route.
// continuer = true pour reprendre la partie sauvegardee au lieu d'en commencer une nouvelle.
// defi = true pour le defi du jour : le hasard part de la date, la meme pour tout le monde.
void jouerPartie(bool continuer, bool defi) {
    if (defi) {
        std::srand(nombreDuJour());
        std::cout << "\n" << colorer("=== DEFI DU JOUR : " + std::to_string(nombreDuJour()) + " ===", JAUNE + GRAS) << "\n";
        std::cout << "Aujourd'hui, tout le monde affronte la meme route. Qui ira le plus loin ?\n";
    } else {
        std::srand(std::time(nullptr));     // un hasard different a chaque partie
    }

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
    Objet croc     = {"Croc de Haschen",        TypeObjet::Materiau, Rarete::Commun,  8};
    Objet peau     = {"Peau de Haschen",        TypeObjet::Materiau, Rarete::Commun, 12};
    Objet totem    = {"Totem de chaman",        TypeObjet::Materiau, Rarete::Rare,   30};
    Objet griffe   = {"Griffe de berserker",    TypeObjet::Materiau, Rarete::Rare,   25};
    Objet osGraves = {"Os graves de runes",     TypeObjet::Materiau, Rarete::Rare,   25};
    Objet couronne = {"Couronne d'os d'Ashka",  TypeObjet::Materiau, Rarete::Epique, 70};
    Objet collier  = {"Collier de crocs",       TypeObjet::Materiau, Rarete::Rare,   30};
    Objet ecu      = {"Bouclier de Haschen",    TypeObjet::Materiau, Rarete::Rare,   35};
    Objet masque   = {"Masque de Skarn",        TypeObjet::Materiau, Rarete::Epique, 80};
    Objet potion   = {"Potion",                 TypeObjet::Potion,   Rarete::Commun,  7};

    // Les armes qu'on peut trouver sur les ennemis
    //                                  nom                   distance bonus crit coups prix rarete
    Objet arcDOs         = objetDepuisArme({"Arc d'os",           true,   3, 20, 1,  60, Rarete::Rare});
    Objet lance          = objetDepuisArme({"Lance de Haschen",   false,  3, 15, 1,  60, Rarete::Rare});
    Objet hacheBerserker = objetDepuisArme({"Hache du berserker", false,  6, 15, 1,  80, Rarete::Rare});
    Objet javelots       = objetDepuisArme({"Javelots d'Ashka",   true,   6, 20, 1, 120, Rarete::Epique});
    Objet sceptre        = objetDepuisArme({"Sceptre d'ombre",    true,   5, 15, 2, 140, Rarete::Epique});

    // Les ennemis. Apres l'XP et l'or : la table de loot (chaque objet avec sa chance sur 100), puis le style.
    Bestiaire bestiaire;

    // Les Haschen des chemins normaux
    bestiaire.normaux = {
        {"Haschen eclaireur",   18, 18,  8, 1, 0, false,  20, 20,
            {{croc, 70}, {peau, 30}, {potion, 20}, {arcDOs, 10}}},
        {"Haschen guerrier",    22, 22,  9, 2, 0, false,  25, 25,
            {{croc, 60}, {peau, 50}, {potion, 20}, {lance, 12}}},
        {"Haschen traqueur",    20, 20,  9, 1, 0, false,  25, 22,
            {{croc, 50}, {peau, 40}, {potion, 20}, {arcDOs, 12}}, Style::Lanceur},
        {"Haschen chaman",      20, 20, 10, 1, 0, false,  25, 25,
            {{croc, 40}, {totem, 30}, {potion, 40}}},
        {"Haschen louvetier",   22, 22, 10, 2, 0, false,  25, 25,
            {{croc, 60}, {peau, 50}, {collier, 20}}, Style::Chargeur},
    };
    bestiaire.normaux[3].attaquePoison = true;      // les coups du chaman peuvent empoisonner

    // Les Haschen d'elite : plus durs, avec un objet RARE garanti
    bestiaire.elites = {
        {"Haschen berserker",   30, 30, 15, 5, 0, false,  35, 35,
            {{griffe, 100}, {croc, 50}, {hacheBerserker, 20}}},
        {"Haschen brise-os",    34, 34, 13, 6, 0, false,  35, 35,
            {{osGraves, 100}, {peau, 60}, {lance, 20}}},
        {"Haschen porte-bouclier", 40, 40, 11, 9, 0, false, 40, 40,
            {{ecu, 100}, {osGraves, 30}, {potion, 30}}},
    };

    // Les boss
    bestiaire.ashka = {"Ashka, Matriarche des Haschen", 40, 40, 14, 4, 1, true, 50, 50,
        {{couronne, 100}, {javelots, 100}, {potion, 50}}, Style::Lanceur};
    bestiaire.skarn = {"Skarn, le Tisseur d'ombres", 50, 50, 15, 4, 1, true, 70, 60,
        {{masque, 100}, {sceptre, 100}, {potion, 50}}, Style::Lanceur};
    bestiaire.skarn.attaquePoison = true;           // ses sorts d'ombre empoisonnent
    bestiaire.vorgath = {"Vorgath le Destructeur", 60, 60, 17, 6, 2, true, 100, 0,
        {}, Style::Chargeur};

    // L'etat de la partie : tout ce qui sera sauvegarde
    EtatPartie etat = {};

    if (continuer) {
        if (!charger(etat)) {
            std::cout << "\n" << colorer("La sauvegarde est introuvable ou abimee.", ROUGE) << "\n";
            attendreEntree();
            return;
        }
        std::cout << "\n" << colorer("=== Reprise de la partie ===", JAUNE + GRAS) << "\n";
        std::cout << "AYLIS reprend la route a l'etape " << (etat.etape + 1) << ", niveau " << etat.aylis.niveau
                  << ", " << etat.aylis.pv << "/" << etat.aylis.pvMax << " pv.\n";
    } else {
        afficherIntro();
        etat.aylis = choisirVoie(armes);

        if (defi) {
            // Le defi du jour : difficulte normale imposee, pour que tout le monde joue la meme partie
            etat.difficulte = 2;
            std::cout << "\nDefi du jour : difficulte " << colorer("Normale", JAUNE) << " imposee. Bonne chance !\n";
        } else {
            // Le choix de la difficulte : change la force des ennemis et l'or qu'ils donnent
            std::cout << "\nChoisis la difficulte :\n";
            std::cout << "1. Facile     (ennemis -20% pv et attaque, +20% d'or)\n";
            std::cout << "2. Normal\n";
            std::cout << "3. Difficile  (ennemis +25% pv et attaque, -20% d'or)\n";
            etat.difficulte = lireChoix(1, 3);
        }
    }

    int forceEnnemis = 100;     // en pourcentage
    int orEnnemis = 100;
    int difficulte = etat.difficulte;
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
    appliquerDifficulte(bestiaire.skarn, forceEnnemis, orEnnemis);
    appliquerDifficulte(bestiaire.vorgath, forceEnnemis, orEnnemis);

    // En route !
    bool victoire = parcourirCarte(etat, armes, bestiaire);

    // La partie est finie (gagnee ou perdue) : la sauvegarde ne sert plus
    effacerSauvegarde();

    if (victoire) {
        std::cout << "\n" << colorer("=== VICTOIRE TOTALE ! ===", VERT + GRAS) << "\n";
        afficherFin(etat);
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
        int choix = ecranTitre(sauvegardeExiste());
        if (choix == 1) {
            jouerPartie(false, false);
        } else if (choix == 2) {
            jouerPartie(true, false);
        } else if (choix == 3) {
            jouerPartie(false, true);
        } else if (choix == 4) {
            afficherRegles();
        } else {
            std::cout << "\nA bientot sur la route, AYLIS !\n";
            return 0;
        }
    }
}
