// progression.cpp : le loot, l'XP et les points de competence
#include <iostream>
#include <cstdlib>
#include "progression.h"
#include "outils.h"
#include "couleurs.h"

// ===================== Le loot =====================

// Ramasse l'or et les objets d'un ennemi vaincu
void ramasserButin(Combattant& aylis, const Combattant& ennemi) {
    // L'or varie : entre 80% et 120% de la somme moyenne
    int pieces = ennemi.orDonne * (80 + std::rand() % 41) / 100;
    aylis.pieces = aylis.pieces + pieces;
    std::cout << "AYLIS ramasse " << pieces << " pieces d'or (" << aylis.pieces << " en tout).\n";

    // Chaque ligne de la table de loot est tiree au sort
    int nombreButins = ennemi.butin.size();
    for (int i = 0; i < nombreButins; i++) {
        const Butin& ligne = ennemi.butin[i];
        if (std::rand() % 100 >= ligne.chance) {
            continue;   // pas de chance pour cet objet
        }

        if (ligne.objet.type == TypeObjet::Potion) {
            aylis.potions = aylis.potions + 1;
            std::cout << "Butin : une potion ! (" << aylis.potions << " potions)\n";
        } else {
            aylis.inventaire.push_back(ligne.objet);
            std::cout << "Butin : ";
            afficherObjet(ligne.objet);
            std::cout << "\n";
        }
    }
}

// ===================== XP et points de competence =====================

// L'XP qu'il faut pour passer au niveau suivant : 30 au niveau 1, 60 au niveau 2...
int xpPourNiveauSuivant(int niveau) {
    return niveau * 30;
}

// Donne de l'XP a AYLIS. Chaque niveau gagne donne 2 points de competence.
void gagnerXp(Combattant& aylis, int xpGagne) {
    aylis.xp = aylis.xp + xpGagne;
    std::cout << "AYLIS gagne " << xpGagne << " XP !\n";

    // "while" et pas "if" : on peut monter plusieurs niveaux d'un coup
    while (aylis.xp >= xpPourNiveauSuivant(aylis.niveau)) {
        aylis.xp = aylis.xp - xpPourNiveauSuivant(aylis.niveau);
        aylis.niveau = aylis.niveau + 1;
        aylis.points = aylis.points + 2;
        aylis.pv = aylis.pvMax;     // monter de niveau soigne completement

        std::cout << "\n" << colorer("*** NIVEAU " + std::to_string(aylis.niveau) + " ! ***", JAUNE + GRAS) << "\n";
        std::cout << "+2 points de competence. AYLIS retrouve tous ses pv !\n";
    }

    std::cout << "XP         ";
    afficherBarre(aylis.xp, xpPourNiveauSuivant(aylis.niveau), CYAN);
    std::cout << " " << aylis.xp << "/" << xpPourNiveauSuivant(aylis.niveau)
              << " vers le niveau " << (aylis.niveau + 1) << "\n";
}

// Le menu pour depenser les points de competence
void depenserPoints(Combattant& aylis) {
    while (aylis.points > 0) {
        std::cout << "\n=== POINTS DE COMPETENCE : " << aylis.points << " a depenser ===\n";
        std::cout << "pv max " << aylis.pvMax << "  |  attaque " << aylis.attaque
                  << "  |  mana max " << aylis.manaMax
                  << "  |  sorts connus " << aylis.sortsConnus << "/" << nombreDeSorts << "\n";
        std::cout << "1. + de vie      (+8 pv max)\n";
        std::cout << "2. + de degats   (+3 attaque)\n";
        if (aylis.sortsConnus < nombreDeSorts) {
            std::cout << "3. + de sorts    (apprendre un nouveau sort, +5 mana max)\n";
        } else {
            std::cout << "3. + de mana     (tous les sorts sont appris : +5 mana max)\n";
        }

        int choix = lireChoix(1, 3);
        aylis.points = aylis.points - 1;

        if (choix == 1) {
            aylis.pvMax = aylis.pvMax + 8;
            aylis.pv = aylis.pv + 8;
            std::cout << "AYLIS a maintenant " << aylis.pvMax << " pv max.\n";
        } else if (choix == 2) {
            aylis.attaque = aylis.attaque + 3;
            std::cout << "AYLIS a maintenant " << aylis.attaque << " en attaque.\n";
        } else {
            aylis.manaMax = aylis.manaMax + 5;
            if (aylis.sortsConnus < nombreDeSorts) {
                aylis.sortsConnus = aylis.sortsConnus + 1;
                if (aylis.sortsConnus == 2) {
                    std::cout << "Nouveau sort appris : SOIN !\n";
                } else if (aylis.sortsConnus == 3) {
                    std::cout << "Nouveau sort appris : ECLAIR !\n";
                } else {
                    std::cout << "Nouveau sort appris : BOUCLIER !\n";
                }
            }
            std::cout << "Mana max : " << aylis.manaMax << ".\n";
        }
    }
}
