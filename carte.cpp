// carte.cpp : les etapes de la route, les choix de chemin et les recompenses
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "carte.h"
#include "outils.h"
#include "couleurs.h"
#include "progression.h"
#include "marchands.h"
#include "combat.h"
#include "evenements.h"
#include "sauvegarde.h"

// Les sortes d'etapes sur la carte
const int ETAPE_CHOIX = 0;      // AYLIS choisit son chemin
const int ETAPE_HALTE = 1;      // une halte obligatoire, pour se preparer avant un boss
const int ETAPE_ASHKA = 2;
const int ETAPE_VORGATH = 3;
const int ETAPE_SKARN = 4;

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

// Plus on avance, plus les Haschen sont forts : +20% apres Ashka, +40% apres Skarn
int renfortDesHaschen(const EtatPartie& etat) {
    if (etat.skarnVaincu) {
        return 140;
    }
    if (etat.ashkaVaincue) {
        return 120;
    }
    return 100;
}

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
std::vector<Chemin> proposerChemins(const Bestiaire& bestiaire, int renfort) {
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

    if (renfort != 100) {
        for (Combattant& haschen : normaux) {
            renforcer(haschen, renfort);
        }
        renforcer(elites[0], renfort);
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
bool combatEtRecompenses(EtatPartie& etat, std::vector<Combattant> groupe, bool dernierCombat) {
    Combattant& aylis = etat.aylis;

    std::cout << "\n==========================================\n";
    if (groupe[0].estBoss) {
        std::cout << colorer("  !!! BOSS : " + groupe[0].nom + " !!!", ROUGE + GRAS) << "\n";
    } else if (groupe.size() > 1) {
        std::cout << "  Une meute surgit : " << nomsDuGroupe(groupe) << " !\n";
    } else {
        std::cout << "  " << groupe[0].nom << " surgit !\n";
    }
    std::cout << "==========================================\n";

    if (!combattre(aylis, groupe, etat.rage, etat.compagnon, etat.avecCompagnon)) {
        std::cout << "\n" << colorer("=== GAME OVER ===", ROUGE + GRAS) << "\n";
        std::cout << "AYLIS tombe au combat face a " << nomsDuGroupe(groupe) << ".\n";
        return false;
    }

    std::cout << "\n" << colorer("Victoire contre " + nomsDuGroupe(groupe) + " !", VERT + GRAS) << "\n";
    if (dernierCombat) {
        return true;    // la partie est gagnee, pas besoin de recompenses
    }

    // Chaque ennemi vaincu donne son butin et son XP
    for (const Combattant& ennemi : groupe) {
        ramasserButin(aylis, ennemi);
        gagnerXp(aylis, ennemi.xpDonne);
    }
    depenserPoints(aylis);

    aylis.soigner(10);
    std::cout << "AYLIS souffle un peu : +10 pv (" << aylis.pv << "/" << aylis.pvMax << ").\n";

    // Le compagnon se remet aussi (meme s'il etait K.O.)
    if (etat.avecCompagnon) {
        if (!etat.compagnon.estDebout()) {
            etat.compagnon.pv = etat.compagnon.pvMax / 2;
            std::cout << etat.compagnon.nom << " se releve peniblement (" << etat.compagnon.pv << " pv).\n";
        } else {
            etat.compagnon.soigner(10);
        }
    }
    return true;
}

// Ashka est vaincue : AYLIS decide de son sort. C'est un choix qui compte pour la fin.
void destinDAshka(EtatPartie& etat) {
    std::cout << "\nAshka s'effondre a genoux, sa lance brisee. \"Acheve-moi, humain... ou laisse-moi\n";
    std::cout << "ramener mon clan dans les collines. Nous n'avons jamais voulu de cette guerre.\"\n\n";
    std::cout << "1. L'epargner\n";
    std::cout << "2. L'achever et prendre ses bracelets d'or\n";

    if (lireChoix(1, 2) == 1) {
        std::cout << "Ashka se releve lentement, et s'eloigne avec ses guerriers vers les collines.\n";
        etat.aylis.honneur = etat.aylis.honneur + 1;
        std::cout << colorer("(Honneur +1)", VERT) << "\n";
    } else {
        std::cout << "La lame d'AYLIS s'abat. Les bracelets d'Ashka valent une petite fortune.\n";
        etat.aylis.pieces = etat.aylis.pieces + 30;
        std::cout << "+30 pieces d'or (" << etat.aylis.pieces << " en tout).\n";
        etat.aylis.honneur = etat.aylis.honneur - 1;
        std::cout << colorer("(Honneur -1)", ROUGE) << "\n";
    }
}

// Ou en est l'histoire, pour les repliques des marchands
int momentDeLHistoire(bool ashkaVaincue, int prochaineEtape) {
    if (prochaineEtape == ETAPE_VORGATH) {
        return MOMENT_AVANT_VORGATH;
    }
    if (prochaineEtape == ETAPE_SKARN) {
        return MOMENT_AVANT_SKARN;
    }
    if (ashkaVaincue) {
        return MOMENT_APRES_ASHKA;
    }
    if (prochaineEtape == ETAPE_ASHKA) {
        return MOMENT_AVANT_ASHKA;
    }
    return MOMENT_DEBUT;
}

bool parcourirCarte(EtatPartie& etat, const std::vector<Arme>& armes, const Bestiaire& bestiaire) {
    // Le plan de la route : deux choix, une halte, Ashka, deux choix, une halte, Vorgath
    const std::vector<int> plan = {
        ETAPE_CHOIX, ETAPE_CHOIX, ETAPE_HALTE, ETAPE_ASHKA,
        ETAPE_CHOIX, ETAPE_CHOIX, ETAPE_HALTE, ETAPE_SKARN,
        ETAPE_CHOIX, ETAPE_HALTE, ETAPE_VORGATH,
    };
    int nombreEtapes = plan.size();
    Combattant& aylis = etat.aylis;

    // On part de l'etape ou en est la partie (0 pour une nouvelle partie, plus loin si on reprend une sauvegarde)
    for (int etape = etat.etape; etape < nombreEtapes; etape++) {
        int typeEtape = plan[etape];

        // Sauvegarde automatique au debut de chaque etape
        etat.etape = etape;
        sauvegarder(etat);

        // L'etape d'apres, pour savoir ce que les marchands doivent raconter
        int prochaineEtape = ETAPE_CHOIX;
        if (etape + 1 < nombreEtapes) {
            prochaineEtape = plan[etape + 1];
        }
        int moment = momentDeLHistoire(etat.ashkaVaincue, prochaineEtape);

        std::cout << "\n\n######## ETAPE " << (etape + 1) << "/" << nombreEtapes << " ########  "
                  << colorer("(partie sauvegardee)", GRIS) << "\n";

        if (typeEtape == ETAPE_ASHKA) {
            if (!combatEtRecompenses(etat, {bestiaire.ashka}, false)) {
                return false;
            }
            destinDAshka(etat);
            etat.ashkaVaincue = true;
            continue;
        }

        if (typeEtape == ETAPE_SKARN) {
            std::cout << "Un brouillard violet envahit le marais. Au centre, une silhouette voutee murmure des\n";
            std::cout << "incantations : " << colorer("Skarn, le Tisseur d'ombres", VIOLET + GRAS)
                      << ", le sorcier qui souffle la rage de Vorgath aux Haschen.\n";
            if (!combatEtRecompenses(etat, {bestiaire.skarn}, false)) {
                return false;
            }
            std::cout << "Le brouillard se dissipe. Sans Skarn, la rage des Haschen commence a vaciller...\n";
            etat.skarnVaincu = true;
            continue;
        }

        if (typeEtape == ETAPE_VORGATH) {
            return combatEtRecompenses(etat, {bestiaire.vorgath}, true);
        }

        if (typeEtape == ETAPE_HALTE) {
            if (prochaineEtape == ETAPE_ASHKA) {
                std::cout << "Le col d'Ashka se dresse devant AYLIS. Une derniere halte avant l'affrontement.\n";
            } else if (prochaineEtape == ETAPE_SKARN) {
                std::cout << "Le marais de Skarn commence ici. Les marchands ont installe un dernier campement.\n";
            } else {
                std::cout << "La forteresse de Vorgath est en vue. Une derniere halte avant l'assaut.\n";
            }
            halte(aylis, armes, etat.haltesVisitees, moment);
            etat.haltesVisitees = etat.haltesVisitees + 1;
            continue;
        }

        // Une etape de choix : on propose trois chemins
        std::vector<Chemin> chemins = proposerChemins(bestiaire, renfortDesHaschen(etat));
        int nombreChemins = chemins.size();

        std::cout << "Plusieurs chemins s'offrent a AYLIS :\n";
        for (int i = 0; i < nombreChemins; i++) {
            std::cout << (i + 1) << ". ";
            afficherChemin(chemins[i]);
            std::cout << "\n";
        }
        std::cout << "(AYLIS : " << aylis.pv << "/" << aylis.pvMax << " pv, "
                  << aylis.potions << " potions, " << aylis.pieces << " or";
        if (etat.avecCompagnon) {
            std::cout << ", avec " << etat.compagnon.nom;
        }
        std::cout << ")\n";

        const Chemin& chemin = chemins[lireChoix(1, nombreChemins) - 1];

        if (chemin.type == CHEMIN_COMBAT || chemin.type == CHEMIN_ELITE) {
            if (!combatEtRecompenses(etat, chemin.ennemis, false)) {
                return false;
            }
        } else if (chemin.type == CHEMIN_REPOS) {
            aylis.soigner(aylis.pvMax / 2);
            std::cout << "\nAYLIS se repose pres du feu. Retour a " << colorer(aylis.pv, VERT) << "/" << aylis.pvMax << " pv.\n";
            if (etat.avecCompagnon) {
                etat.compagnon.pv = etat.compagnon.pvMax;
                std::cout << etat.compagnon.nom << " reprend aussi des forces.\n";
            }
        } else if (chemin.type == CHEMIN_HALTE) {
            halte(aylis, armes, etat.haltesVisitees, moment);
            etat.haltesVisitees = etat.haltesVisitees + 1;
        } else {
            evenementAleatoire(etat);
        }
    }

    return true;
}
