// evenements.cpp : les rencontres surprises des chemins "???", avec des choix qui ont des consequences
#include <iostream>
#include <cstdlib>
#include "evenements.h"
#include "outils.h"
#include "progression.h"

// Un evenement ne peut pas tuer AYLIS : il lui reste toujours au moins 1 pv
void blesser(Combattant& aylis, int degats) {
    aylis.pv = aylis.pv - degats;
    if (aylis.pv < 1) {
        aylis.pv = 1;
    }
    std::cout << "AYLIS perd " << degats << " pv (" << aylis.pv << "/" << aylis.pvMax << ").\n";
}

void gagnerOr(Combattant& aylis, int pieces) {
    aylis.pieces = aylis.pieces + pieces;
    std::cout << "+" << pieces << " pieces d'or (" << aylis.pieces << " en tout).\n";
}

// ----- Le voyageur blesse -----
void voyageurBlesse(Combattant& aylis) {
    std::cout << "Au bord du chemin, un voyageur est adosse a un arbre, une fleche haschen dans l'epaule.\n";
    std::cout << "\"Par pitie... je n'ai plus rien pour me soigner...\"\n\n";
    std::cout << "1. Lui donner une potion (tu en as " << aylis.potions << ")\n";
    std::cout << "2. Fouiller ses affaires pendant qu'il est trop faible pour resister\n";
    std::cout << "3. Passer son chemin\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (aylis.potions == 0) {
            std::cout << "AYLIS n'a plus de potion a donner... et doit reprendre la route.\n";
            return;
        }
        aylis.potions = aylis.potions - 1;
        std::cout << "Le voyageur boit la potion et reprend des couleurs. \"Merci... prends ceci.\"\n";
        if (std::rand() % 2 == 0) {
            gagnerOr(aylis, 35);
        } else {
            Objet amulette = {"Amulette du voyageur", OBJET_MATERIAU, RARE, 40};
            aylis.inventaire.push_back(amulette);
            std::cout << "Butin : ";
            afficherObjet(amulette);
            std::cout << "\n";
        }
    } else if (choix == 2) {
        std::cout << "AYLIS trouve une bourse... et le voyageur murmure une malediction.\n";
        gagnerOr(aylis, 20);
        aylis.pvMax = aylis.pvMax - 3;
        if (aylis.pv > aylis.pvMax) {
            aylis.pv = aylis.pvMax;
        }
        std::cout << "La malediction pese sur AYLIS : -3 pv max (" << aylis.pvMax << ").\n";
    } else {
        std::cout << "AYLIS s'eloigne sans se retourner.\n";
    }
}

// ----- Le coffre piege -----
void coffrePiege(Combattant& aylis) {
    std::cout << "Un coffre cercle de fer trone au milieu des ruines. Des fils fins brillent autour de la serrure...\n\n";
    std::cout << "1. L'ouvrir prudemment (petit butin, petit risque)\n";
    std::cout << "2. Forcer la serrure (gros butin, gros risque)\n";
    std::cout << "3. Laisser le coffre tranquille\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (std::rand() % 100 < 30) {
            std::cout << "CLIC ! Une aiguille empoisonnee jaillit !\n";
            blesser(aylis, 6);
        }
        std::cout << "Le coffre s'ouvre.\n";
        gagnerOr(aylis, 20 + std::rand() % 21);
    } else if (choix == 2) {
        if (std::rand() % 2 == 0) {
            std::cout << "BOUM ! Le piege explose au visage d'AYLIS !\n";
            blesser(aylis, 12);
        }
        std::cout << "Le couvercle cede dans un craquement.\n";
        gagnerOr(aylis, 40 + std::rand() % 31);
        aylis.potions = aylis.potions + 1;
        std::cout << "Et une potion ! (" << aylis.potions << " potions)\n";
    } else {
        std::cout << "Mieux vaut ne pas tenter le diable.\n";
    }
}

// ----- L'autel mysterieux -----
void autelMysterieux(Combattant& aylis) {
    std::cout << "Un autel de pierre noire, couvert de runes qui pulsent doucement. Une voix chuchote :\n";
    std::cout << "\"Donne... et tu recevras...\"\n\n";
    std::cout << "1. Offrir ton sang (-10 pv) pour devenir plus fort\n";
    std::cout << "2. Prier en silence\n";
    std::cout << "3. S'eloigner de cet endroit\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (aylis.pv <= 10) {
            std::cout << "L'autel reste muet : AYLIS est trop faible pour offrir son sang.\n";
            return;
        }
        blesser(aylis, 10);
        aylis.attaque = aylis.attaque + 2;
        std::cout << "Les runes s'embrasent ! Attaque +2 pour toujours (" << aylis.attaque << ").\n";
    } else if (choix == 2) {
        if (std::rand() % 2 == 0) {
            aylis.manaMax = aylis.manaMax + 5;
            std::cout << "Une douce lumiere enveloppe AYLIS. Mana max +5 (" << aylis.manaMax << ").\n";
        } else {
            soigner(aylis, 10);
            std::cout << "Rien ne se passe... ou presque. AYLIS se sent un peu mieux (" << aylis.pv << " pv).\n";
        }
    } else {
        std::cout << "Les chuchotements s'eteignent derriere AYLIS.\n";
    }
}

// ----- Le deserteur haschen -----
void deserteurHaschen(Combattant& aylis) {
    std::cout << "Un jeune Haschen, blesse et sans arme, leve les mains en tremblant.\n";
    std::cout << "\"Je fuis Vorgath... Je ne veux plus me battre. Laisse-moi partir.\"\n\n";
    std::cout << "1. L'aider (donner une potion, tu en as " << aylis.potions << ")\n";
    std::cout << "2. Le depouiller\n";
    std::cout << "3. Le laisser partir\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (aylis.potions == 0) {
            std::cout << "AYLIS n'a plus de potion. Le Haschen hoche la tete et s'enfuit.\n";
            return;
        }
        aylis.potions = aylis.potions - 1;
        std::cout << "\"Merci... Ecoute : Vorgath fonce toujours droit sur ses ennemis. Et nos chefs cachent\n";
        std::cout << "leurs tresors sous les pierres plates.\" AYLIS apprend beaucoup sur les Haschen.\n";
        gagnerXp(aylis, 25);
        depenserPoints(aylis);
    } else if (choix == 2) {
        std::cout << "Le Haschen lache sa bourse et s'enfuit en boitant.\n";
        gagnerOr(aylis, 30);
    } else {
        std::cout << "Le Haschen disparait dans les buissons. AYLIS se sent en paix.\n";
        soigner(aylis, 5);
    }
}

void evenementAleatoire(Combattant& aylis) {
    std::cout << "\n??????????????????????????????????????????\n";
    int tirage = std::rand() % 4;
    if (tirage == 0) {
        voyageurBlesse(aylis);
    } else if (tirage == 1) {
        coffrePiege(aylis);
    } else if (tirage == 2) {
        autelMysterieux(aylis);
    } else {
        deserteurHaschen(aylis);
    }
    std::cout << "??????????????????????????????????????????\n";
}
