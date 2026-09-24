// evenements.cpp : les rencontres surprises des chemins "???", avec des choix qui ont des consequences
#include <iostream>
#include <cstdlib>
#include "evenements.h"
#include "outils.h"
#include "couleurs.h"
#include "progression.h"

// Un evenement ne peut pas tuer AYLIS : il lui reste toujours au moins 1 pv
void blesser(Combattant& aylis, int degats) {
    aylis.pv = aylis.pv - degats;
    if (aylis.pv < 1) {
        aylis.pv = 1;
    }
    std::cout << "AYLIS perd " << colorer(degats, ROUGE) << " pv (" << aylis.pv << "/" << aylis.pvMax << ").\n";
}

void gagnerOr(Combattant& aylis, int pieces) {
    aylis.pieces = aylis.pieces + pieces;
    std::cout << "+" << pieces << " pieces d'or (" << aylis.pieces << " en tout).\n";
}

// Un bon ou un mauvais choix : l'honneur monte ou descend (il decide de la fin de l'histoire)
void changerHonneur(Combattant& aylis, int changement) {
    aylis.honneur = aylis.honneur + changement;
    if (changement > 0) {
        std::cout << colorer("(Honneur +1)", VERT) << "\n";
    } else {
        std::cout << colorer("(Honneur -1)", ROUGE) << "\n";
    }
}

// Un nouveau compagnon rejoint AYLIS
void recruter(EtatPartie& etat, const Combattant& compagnon) {
    etat.compagnon = compagnon;
    etat.avecCompagnon = true;
    std::cout << colorer(compagnon.nom + " rejoint AYLIS et combattra a ses cotes !", CYAN + GRAS) << "\n";
}

// ----- Le voyageur blesse -----
void voyageurBlesse(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
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
            Objet amulette = {"Amulette du voyageur", TypeObjet::Materiau, Rarete::Rare, 40};
            aylis.inventaire.push_back(amulette);
            std::cout << "Butin : ";
            afficherObjet(amulette);
            std::cout << "\n";
        }
        changerHonneur(aylis, 1);
    } else if (choix == 2) {
        std::cout << "AYLIS trouve une bourse... et le voyageur murmure une malediction.\n";
        gagnerOr(aylis, 20);
        aylis.pvMax = aylis.pvMax - 3;
        if (aylis.pv > aylis.pvMax) {
            aylis.pv = aylis.pvMax;
        }
        std::cout << "La malediction pese sur AYLIS : -3 pv max (" << aylis.pvMax << ").\n";
        changerHonneur(aylis, -1);
    } else {
        std::cout << "AYLIS s'eloigne sans se retourner.\n";
    }
}

// ----- Le coffre piege -----
void coffrePiege(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
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
void autelMysterieux(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
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
            aylis.soigner(10);
            std::cout << "Rien ne se passe... ou presque. AYLIS se sent un peu mieux (" << aylis.pv << " pv).\n";
        }
    } else {
        std::cout << "Les chuchotements s'eteignent derriere AYLIS.\n";
    }
}

// ----- Le deserteur haschen -----
void deserteurHaschen(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
    std::cout << "Un jeune Haschen, blesse et sans arme, leve les mains en tremblant.\n";
    std::cout << "\"Je m'appelle Kerrak. Je fuis Vorgath... Je ne veux plus me battre pour lui.\"\n\n";
    std::cout << "1. Le soigner (donner une potion si tu en as, tu en as " << aylis.potions << ")\n";
    std::cout << "2. Le depouiller\n";
    std::cout << "3. Le laisser partir\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (aylis.potions > 0) {
            aylis.potions = aylis.potions - 1;
            std::cout << "Kerrak boit la potion. \"Vorgath fonce toujours droit sur ses ennemis. Et nos chefs\n";
            std::cout << "cachent leurs tresors sous les pierres plates.\" AYLIS apprend beaucoup sur les Haschen.\n";
            gagnerXp(aylis, 25);
            depenserPoints(aylis);
        } else {
            std::cout << "AYLIS n'a plus de potion, mais bande la blessure de Kerrak avec un bout de sa cape.\n";
        }
        changerHonneur(aylis, 1);

        if (!etat.avecCompagnon) {
            std::cout << "\"Je te dois la vie. Laisse-moi t'aider a abattre Vorgath.\"\n";
            //                  nom       pv  pvMax att def potions boss  xp or
            Combattant kerrak = {"Kerrak", 26, 26,   9,  2,  0,      false, 0, 0};
            recruter(etat, kerrak);
        }
    } else if (choix == 2) {
        std::cout << "Kerrak lache sa bourse et s'enfuit en boitant.\n";
        gagnerOr(aylis, 30);
        changerHonneur(aylis, -1);
    } else {
        std::cout << "Kerrak disparait dans les buissons. AYLIS se sent en paix.\n";
        aylis.soigner(5);
        changerHonneur(aylis, 1);
    }
}

// ----- La mercenaire -----
void mercenaire(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
    const int prix = 40;
    std::cout << "Une guerriere aux cheveux tresses aiguise sa lame pres d'un feu.\n";
    std::cout << "\"Brenna, mercenaire. Tu vas chez Vorgath ? Pour " << prix << " pieces, je viens avec toi.\"\n\n";

    if (etat.avecCompagnon) {
        std::cout << "Elle jette un oeil a " << etat.compagnon.nom << ". \"Ah, tu es deja accompagne. Bonne route alors.\"\n";
        return;
    }

    std::cout << "1. L'engager (" << prix << " or, tu en as " << aylis.pieces << ")\n";
    std::cout << "2. Refuser poliment\n";

    int choix = lireChoix(1, 2);
    if (choix == 1) {
        if (aylis.pieces < prix) {
            std::cout << "\"Reviens quand tu auras de quoi payer.\"\n";
            return;
        }
        aylis.pieces = aylis.pieces - prix;
        //                  nom       pv  pvMax att def potions boss  xp or
        Combattant brenna = {"Brenna", 30, 30,  11,  3,  0,      false, 0, 0};
        recruter(etat, brenna);
    } else {
        std::cout << "\"Comme tu veux. Ne meurs pas trop vite.\"\n";
    }
}

// ----- Le puits aux souhaits -----
void puitsAuxSouhaits(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
    const int offrande = 20;
    std::cout << "Un vieux puits couvert de mousse. On dit qu'il exauce les voeux de ceux qui y jettent de l'or.\n\n";
    std::cout << "1. Jeter " << offrande << " pieces et faire un voeu (tu en as " << aylis.pieces << ")\n";
    std::cout << "2. Boire son eau\n";
    std::cout << "3. Passer son chemin\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (aylis.pieces < offrande) {
            std::cout << "Les poches d'AYLIS sont trop legeres pour un voeu.\n";
            return;
        }
        aylis.pieces = aylis.pieces - offrande;
        int voeu = std::rand() % 3;
        if (voeu == 0) {
            aylis.potions = aylis.potions + 1;
            std::cout << "Une fiole remonte a la surface ! (" << aylis.potions << " potions)\n";
        } else if (voeu == 1) {
            aylis.pvMax = aylis.pvMax + 5;
            aylis.soigner(5);
            std::cout << "Une chaleur envahit AYLIS : pv max +5 (" << aylis.pvMax << ").\n";
        } else {
            std::cout << "Plouf. Rien ne se passe. Le puits garde l'or... et le voeu.\n";
        }
    } else if (choix == 2) {
        if (std::rand() % 100 < 30) {
            std::cout << "Beurk ! L'eau est croupie.\n";
            blesser(aylis, 5);
        } else {
            aylis.soigner(15);
            std::cout << "L'eau est fraiche et pure. AYLIS se sent mieux (" << aylis.pv << " pv).\n";
        }
    } else {
        std::cout << "AYLIS n'a pas de temps pour les superstitions.\n";
    }
}

// ----- Les villageois captifs -----
void villageoisCaptifs(EtatPartie& etat) {
    Combattant& aylis = etat.aylis;
    const int pourboire = 30;
    std::cout << "Deux gardes haschen jouent aux des devant une cage. A l'interieur, des villageois terrorises.\n";
    std::cout << "Les gardes n'ont pas encore remarque AYLIS.\n\n";
    std::cout << "1. Crocheter la cage en silence\n";
    std::cout << "2. Soudoyer les gardes (" << pourboire << " or, tu en as " << aylis.pieces << ")\n";
    std::cout << "3. Ne pas s'en meler\n";

    int choix = lireChoix(1, 3);
    if (choix == 1) {
        if (std::rand() % 100 < 40) {
            std::cout << "CRAC ! La serrure grince. Un garde lance sa hache avant de s'enfuir.\n";
            blesser(aylis, 10);
        }
        std::cout << "La cage s'ouvre. Les villageois glissent une potion et quelques pieces dans les mains d'AYLIS.\n";
        aylis.potions = aylis.potions + 1;
        gagnerOr(aylis, 15);
        changerHonneur(aylis, 1);
    } else if (choix == 2) {
        if (aylis.pieces < pourboire) {
            std::cout << "Les gardes ricanent devant la maigre bourse d'AYLIS.\n";
            return;
        }
        aylis.pieces = aylis.pieces - pourboire;
        std::cout << "Les gardes empochent l'or et regardent ailleurs. Les villageois s'enfuient en remerciant AYLIS.\n";
        changerHonneur(aylis, 1);
    } else {
        std::cout << "AYLIS passe son chemin. Les appels des villageois resonnent longtemps dans son dos.\n";
        changerHonneur(aylis, -1);
    }
}

void evenementAleatoire(EtatPartie& etat) {
    std::cout << "\n" << colorer("??????????????????????????????????????????", VIOLET) << "\n";
    int tirage = std::rand() % 7;
    if (tirage == 0) {
        voyageurBlesse(etat);
    } else if (tirage == 1) {
        coffrePiege(etat);
    } else if (tirage == 2) {
        autelMysterieux(etat);
    } else if (tirage == 3) {
        deserteurHaschen(etat);
    } else if (tirage == 4) {
        mercenaire(etat);
    } else if (tirage == 5) {
        puitsAuxSouhaits(etat);
    } else {
        villageoisCaptifs(etat);
    }
    std::cout << colorer("??????????????????????????????????????????", VIOLET) << "\n";
}
