// carte.cpp : les etapes de la route, les choix de chemin et les recompenses
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "carte.h"
#include "outils.h"
#include "progression.h"
#include "marchands.h"
#include "combat.h"

// Les sortes d'etapes sur la carte
const int ETAPE_CHOIX = 0;      // AYLIS choisit son chemin
const int ETAPE_HALTE = 1;      // une halte obligatoire, pour se preparer avant un boss
const int ETAPE_ASHKA = 2;
const int ETAPE_VORGATH = 3;

// Les sortes de chemins proposes a une etape de choix
const int CHEMIN_COMBAT = 0;
const int CHEMIN_ELITE = 1;
const int CHEMIN_REPOS = 2;
const int CHEMIN_HALTE = 3;

// Un chemin propose au joueur
struct Chemin {
    int type;
    std::string lieu;
    Combattant ennemi;      // seulement pour les chemins de combat
};

// Apres Ashka, les Haschen sont plus forts : +20% de pv et d'attaque
const int renfortApresAshka = 120;

// Choisit un element au hasard dans une liste
std::string lieuAuHasard(const std::vector<std::string>& lieux) {
    return lieux[std::rand() % lieux.size()];
}

Combattant ennemiAuHasard(const std::vector<Combattant>& ennemis) {
    return ennemis[std::rand() % ennemis.size()];
}

// Rend un ennemi plus fort (pourcentage : 120 = +20%)
void renforcer(Combattant& ennemi, int pourcentage) {
    ennemi.pvMax = ennemi.pvMax * pourcentage / 100;
    ennemi.pv = ennemi.pvMax;
    ennemi.attaque = ennemi.attaque * pourcentage / 100;
}

// Prepare les 3 chemins d'une etape de choix : un combat, une elite, et un repos ou une halte
std::vector<Chemin> proposerChemins(const Bestiaire& bestiaire, bool apresAshka) {
    const std::vector<std::string> lieuxNormaux = {
        "La foret des Brumes", "Le marais puant", "Les ruines du vieux fort",
        "La riviere gelee", "Le champ de pierres",
    };
    const std::vector<std::string> lieuxElites = {
        "Le col des Hurlements", "La grotte aux ossements", "Le camp de guerre haschen",
    };

    Combattant normal = ennemiAuHasard(bestiaire.normaux);
    Combattant elite = ennemiAuHasard(bestiaire.elites);
    if (apresAshka) {
        renforcer(normal, renfortApresAshka);
        renforcer(elite, renfortApresAshka);
    }

    std::vector<Chemin> chemins;
    chemins.push_back({CHEMIN_COMBAT, lieuAuHasard(lieuxNormaux), normal});
    chemins.push_back({CHEMIN_ELITE, lieuAuHasard(lieuxElites), elite});

    // Le troisieme chemin : une chance sur deux pour un repos ou une halte
    if (std::rand() % 2 == 0) {
        chemins.push_back({CHEMIN_REPOS, "Un feu de camp abrite", {}});
    } else {
        chemins.push_back({CHEMIN_HALTE, "Le carrefour des marchands", {}});
    }
    return chemins;
}

// Affiche un chemin sur une ligne
void afficherChemin(const Chemin& chemin) {
    std::cout << chemin.lieu << " : ";
    if (chemin.type == CHEMIN_COMBAT) {
        std::cout << "combat contre " << chemin.ennemi.nom;
    } else if (chemin.type == CHEMIN_ELITE) {
        std::cout << "ELITE, " << chemin.ennemi.nom << " (dangereux, butin RARE garanti)";
    } else if (chemin.type == CHEMIN_REPOS) {
        std::cout << "repos (AYLIS recupere la moitie de ses pv max)";
    } else {
        std::cout << "halte avec les marchands (pas de combat)";
    }
}

// Un combat, puis les recompenses. Renvoie false si AYLIS tombe.
bool combatEtRecompenses(Combattant& aylis, Combattant ennemi, int& rage, bool dernierCombat) {
    std::cout << "\n==========================================\n";
    if (ennemi.estBoss) {
        std::cout << "  !!! BOSS : " << ennemi.nom << " !!!\n";
    } else {
        std::cout << "  " << ennemi.nom << " surgit !\n";
    }
    std::cout << "==========================================\n";

    if (!combattre(aylis, ennemi, rage)) {
        std::cout << "\n=== GAME OVER ===\n";
        std::cout << "AYLIS tombe au combat face a " << ennemi.nom << ".\n";
        return false;
    }

    std::cout << "\nVictoire contre " << ennemi.nom << " !\n";
    if (dernierCombat) {
        return true;    // la partie est gagnee, pas besoin de recompenses
    }

    ramasserButin(aylis, ennemi);
    gagnerXp(aylis, ennemi.xpDonne);
    depenserPoints(aylis);

    soigner(aylis, 10);
    std::cout << "AYLIS souffle un peu : +10 pv (" << aylis.pv << "/" << aylis.pvMax << ").\n";
    return true;
}

// Ou en est l'histoire, pour les repliques des marchands
int momentDeLHistoire(bool ashkaVaincue, int prochaineEtape) {
    if (prochaineEtape == ETAPE_VORGATH) {
        return MOMENT_AVANT_VORGATH;
    }
    if (ashkaVaincue) {
        return MOMENT_APRES_ASHKA;
    }
    if (prochaineEtape == ETAPE_ASHKA) {
        return MOMENT_AVANT_ASHKA;
    }
    return MOMENT_DEBUT;
}

bool parcourirCarte(Combattant& aylis, const std::vector<Arme>& armes, const Bestiaire& bestiaire) {
    // Le plan de la route : deux choix, une halte, Ashka, deux choix, une halte, Vorgath
    const std::vector<int> plan = {
        ETAPE_CHOIX, ETAPE_CHOIX, ETAPE_HALTE, ETAPE_ASHKA,
        ETAPE_CHOIX, ETAPE_CHOIX, ETAPE_HALTE, ETAPE_VORGATH,
    };

    int rage = 0;
    int haltesVisitees = 0;     // les prix montent a chaque halte
    bool ashkaVaincue = false;
    int nombreEtapes = plan.size();

    for (int etape = 0; etape < nombreEtapes; etape++) {
        int typeEtape = plan[etape];

        // L'etape d'apres, pour savoir ce que les marchands doivent raconter
        int prochaineEtape = ETAPE_CHOIX;
        if (etape + 1 < nombreEtapes) {
            prochaineEtape = plan[etape + 1];
        }

        std::cout << "\n\n######## ETAPE " << (etape + 1) << "/" << nombreEtapes << " ########\n";

        if (typeEtape == ETAPE_ASHKA) {
            if (!combatEtRecompenses(aylis, bestiaire.ashka, rage, false)) {
                return false;
            }
            ashkaVaincue = true;
            continue;
        }

        if (typeEtape == ETAPE_VORGATH) {
            return combatEtRecompenses(aylis, bestiaire.vorgath, rage, true);
        }

        if (typeEtape == ETAPE_HALTE) {
            if (prochaineEtape == ETAPE_ASHKA) {
                std::cout << "Le col d'Ashka se dresse devant AYLIS. Une derniere halte avant l'affrontement.\n";
            } else {
                std::cout << "La forteresse de Vorgath est en vue. Une derniere halte avant l'assaut.\n";
            }
            halte(aylis, armes, haltesVisitees, momentDeLHistoire(ashkaVaincue, prochaineEtape));
            haltesVisitees = haltesVisitees + 1;
            continue;
        }

        // Une etape de choix : on propose trois chemins
        std::vector<Chemin> chemins = proposerChemins(bestiaire, ashkaVaincue);
        int nombreChemins = chemins.size();

        std::cout << "Plusieurs chemins s'offrent a AYLIS :\n";
        for (int i = 0; i < nombreChemins; i++) {
            std::cout << (i + 1) << ". ";
            afficherChemin(chemins[i]);
            std::cout << "\n";
        }
        std::cout << "(AYLIS : " << aylis.pv << "/" << aylis.pvMax << " pv, "
                  << aylis.potions << " potions, " << aylis.pieces << " or)\n";

        const Chemin& chemin = chemins[lireChoix(1, nombreChemins) - 1];

        if (chemin.type == CHEMIN_COMBAT || chemin.type == CHEMIN_ELITE) {
            if (!combatEtRecompenses(aylis, chemin.ennemi, rage, false)) {
                return false;
            }
        } else if (chemin.type == CHEMIN_REPOS) {
            soigner(aylis, aylis.pvMax / 2);
            std::cout << "\nAYLIS se repose pres du feu. Retour a " << aylis.pv << "/" << aylis.pvMax << " pv.\n";
        } else {
            halte(aylis, armes, haltesVisitees, momentDeLHistoire(ashkaVaincue, prochaineEtape));
            haltesVisitees = haltesVisitees + 1;
        }
    }

    return true;
}
