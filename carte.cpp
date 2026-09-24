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
#include "evenements.h"

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
const int CHEMIN_EVENEMENT = 4;

// Un chemin propose au joueur
struct Chemin {
    int type;
    std::string lieu;
    std::vector<Combattant> ennemis;    // seulement pour les chemins de combat (un ou plusieurs)
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

// Les noms d'un groupe d'ennemis, par exemple : "Haschen eclaireur et Haschen chaman"
std::string nomsDuGroupe(const std::vector<Combattant>& groupe) {
    std::string noms = "";
    int nombre = groupe.size();
    for (int i = 0; i < nombre; i++) {
        if (i > 0 && i == nombre - 1) {
            noms = noms + " et ";
        } else if (i > 0) {
            noms = noms + ", ";
        }
        noms = noms + groupe[i].nom;
    }
    return noms;
}

// Une meute de deux Haschen normaux, un peu moins solides chacun :
// 75% de leurs pv, et 75% de l'XP et de l'or qu'ils donnent
std::vector<Combattant> creerMeute(const Bestiaire& bestiaire) {
    std::vector<Combattant> meute = {ennemiAuHasard(bestiaire.normaux), ennemiAuHasard(bestiaire.normaux)};
    for (Combattant& haschen : meute) {
        haschen.pvMax = haschen.pvMax * 75 / 100;
        haschen.pv = haschen.pvMax;
        haschen.xpDonne = haschen.xpDonne * 75 / 100;
        haschen.orDonne = haschen.orDonne * 75 / 100;
    }

    // Deux fois le meme Haschen ? On les numerote pour pouvoir choisir sa cible
    if (meute[0].nom == meute[1].nom) {
        meute[0].nom = meute[0].nom + " A";
        meute[1].nom = meute[1].nom + " B";
    }
    return meute;
}

// Prepare les 3 chemins d'une etape de choix :
// un combat (seul ou en meute), une elite, et un repos, une halte ou un evenement
std::vector<Chemin> proposerChemins(const Bestiaire& bestiaire, bool apresAshka) {
    const std::vector<std::string> lieuxNormaux = {
        "La foret des Brumes", "Le marais puant", "Les ruines du vieux fort",
        "La riviere gelee", "Le champ de pierres",
    };
    const std::vector<std::string> lieuxElites = {
        "Le col des Hurlements", "La grotte aux ossements", "Le camp de guerre haschen",
    };

    // Le combat normal : une chance sur deux de tomber sur une meute
    std::vector<Combattant> normaux;
    if (std::rand() % 2 == 0) {
        normaux = creerMeute(bestiaire);
    } else {
        normaux = {ennemiAuHasard(bestiaire.normaux)};
    }
    std::vector<Combattant> elites = {ennemiAuHasard(bestiaire.elites)};

    if (apresAshka) {
        for (Combattant& haschen : normaux) {
            renforcer(haschen, renfortApresAshka);
        }
        renforcer(elites[0], renfortApresAshka);
    }

    std::vector<Chemin> chemins;
    chemins.push_back({CHEMIN_COMBAT, lieuAuHasard(lieuxNormaux), normaux});
    chemins.push_back({CHEMIN_ELITE, lieuAuHasard(lieuxElites), elites});

    // Le troisieme chemin : un repos, une halte ou un evenement (une chance sur trois chacun)
    int tirage = std::rand() % 3;
    if (tirage == 0) {
        chemins.push_back({CHEMIN_REPOS, "Un feu de camp abrite", {}});
    } else if (tirage == 1) {
        chemins.push_back({CHEMIN_HALTE, "Le carrefour des marchands", {}});
    } else {
        chemins.push_back({CHEMIN_EVENEMENT, "Un sentier inconnu", {}});
    }
    return chemins;
}

// Affiche un chemin sur une ligne
void afficherChemin(const Chemin& chemin) {
    std::cout << chemin.lieu << " : ";
    if (chemin.type == CHEMIN_COMBAT) {
        if (chemin.ennemis.size() > 1) {
            std::cout << "MEUTE, " << nomsDuGroupe(chemin.ennemis);
        } else {
            std::cout << "combat contre " << nomsDuGroupe(chemin.ennemis);
        }
    } else if (chemin.type == CHEMIN_ELITE) {
        std::cout << "ELITE, " << nomsDuGroupe(chemin.ennemis) << " (dangereux, butin RARE garanti)";
    } else if (chemin.type == CHEMIN_REPOS) {
        std::cout << "repos (AYLIS recupere la moitie de ses pv max)";
    } else if (chemin.type == CHEMIN_HALTE) {
        std::cout << "halte avec les marchands (pas de combat)";
    } else {
        std::cout << "??? (qui sait ce qui attend AYLIS...)";
    }
}

// Un combat contre un groupe, puis les recompenses de chaque ennemi. Renvoie false si AYLIS tombe.
bool combatEtRecompenses(Combattant& aylis, std::vector<Combattant> groupe, int& rage, bool dernierCombat) {
    std::cout << "\n==========================================\n";
    if (groupe[0].estBoss) {
        std::cout << "  !!! BOSS : " << groupe[0].nom << " !!!\n";
    } else if (groupe.size() > 1) {
        std::cout << "  Une meute surgit : " << nomsDuGroupe(groupe) << " !\n";
    } else {
        std::cout << "  " << groupe[0].nom << " surgit !\n";
    }
    std::cout << "==========================================\n";

    if (!combattre(aylis, groupe, rage)) {
        std::cout << "\n=== GAME OVER ===\n";
        std::cout << "AYLIS tombe au combat face a " << nomsDuGroupe(groupe) << ".\n";
        return false;
    }

    std::cout << "\nVictoire contre " << nomsDuGroupe(groupe) << " !\n";
    if (dernierCombat) {
        return true;    // la partie est gagnee, pas besoin de recompenses
    }

    // Chaque ennemi vaincu donne son butin et son XP
    for (const Combattant& ennemi : groupe) {
        ramasserButin(aylis, ennemi);
        gagnerXp(aylis, ennemi.xpDonne);
    }
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
            if (!combatEtRecompenses(aylis, {bestiaire.ashka}, rage, false)) {
                return false;
            }
            ashkaVaincue = true;
            continue;
        }

        if (typeEtape == ETAPE_VORGATH) {
            return combatEtRecompenses(aylis, {bestiaire.vorgath}, rage, true);
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
            if (!combatEtRecompenses(aylis, chemin.ennemis, rage, false)) {
                return false;
            }
        } else if (chemin.type == CHEMIN_REPOS) {
            soigner(aylis, aylis.pvMax / 2);
            std::cout << "\nAYLIS se repose pres du feu. Retour a " << aylis.pv << "/" << aylis.pvMax << " pv.\n";
        } else if (chemin.type == CHEMIN_HALTE) {
            halte(aylis, armes, haltesVisitees, momentDeLHistoire(ashkaVaincue, prochaineEtape));
            haltesVisitees = haltesVisitees + 1;
        } else {
            evenementAleatoire(aylis);
        }
    }

    return true;
}
