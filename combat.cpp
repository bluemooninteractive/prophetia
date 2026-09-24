// combat.cpp : les attaques, les sorts, les etats, le compagnon, le tour des ennemis et la boucle de combat
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "combat.h"
#include "outils.h"
#include "couleurs.h"

// ===================== Les cibles =====================

// Est-ce qu'il reste au moins un ennemi debout ?
bool resteDesEnnemis(const std::vector<Combattant>& ennemis) {
    for (const Combattant& ennemi : ennemis) {
        if (ennemi.pv > 0) {
            return true;
        }
    }
    return false;
}

// Le mot qui decrit une distance
std::string nomDistance(int distance) {
    if (distance == 2) {
        return "LOIN";
    } else if (distance == 1) {
        return "PROCHE";
    }
    return "AU CONTACT";
}

// Choisit la cible d'une attaque. Renvoie son numero dans la liste, ou -1 si aucune cible possible.
// seulementAuContact = true pour une arme de melee : on ne peut frapper que les ennemis au contact.
int choisirCible(const std::vector<Combattant>& ennemis, bool seulementAuContact) {
    // On garde les numeros des ennemis qu'on a le droit de viser
    std::vector<int> possibles;
    int nombre = ennemis.size();
    for (int i = 0; i < nombre; i++) {
        bool debout = ennemis[i].pv > 0;
        bool aPortee = !seulementAuContact || ennemis[i].distance == 0;
        if (debout && aPortee) {
            possibles.push_back(i);
        }
    }

    int nombrePossibles = possibles.size();
    if (nombrePossibles == 0) {
        return -1;
    }
    if (nombrePossibles == 1) {
        return possibles[0];    // une seule cible : pas besoin de demander
    }

    std::cout << "Quelle cible ?\n";
    for (int i = 0; i < nombrePossibles; i++) {
        const Combattant& ennemi = ennemis[possibles[i]];
        std::cout << (i + 1) << ". " << ennemi.nom << " (" << ennemi.pv << " pv, "
                  << nomDistance(ennemi.distance) << ")\n";
    }
    return possibles[lireChoix(1, nombrePossibles) - 1];
}

// Est-ce qu'il existe au moins une cible possible ? (meme regle que choisirCible, sans poser de question)
bool quelquUnAPortee(const std::vector<Combattant>& ennemis, bool seulementAuContact) {
    for (const Combattant& ennemi : ennemis) {
        bool debout = ennemi.pv > 0;
        bool aPortee = !seulementAuContact || ennemi.distance == 0;
        if (debout && aPortee) {
            return true;
        }
    }
    return false;
}

// Un ennemi debout au hasard (pour le compagnon). Renvoie -1 s'il n'y en a plus.
int ennemiDeboutAuHasard(const std::vector<Combattant>& ennemis) {
    std::vector<int> debout;
    int nombre = ennemis.size();
    for (int i = 0; i < nombre; i++) {
        if (ennemis[i].pv > 0) {
            debout.push_back(i);
        }
    }
    if (debout.empty()) {
        return -1;
    }
    return debout[std::rand() % debout.size()];
}

// Annonce les ennemis qui viennent de tomber
void annoncerChutes(std::vector<Combattant>& ennemis) {
    for (Combattant& ennemi : ennemis) {
        if (ennemi.pv <= 0 && !ennemi.vaincu) {
            ennemi.vaincu = true;
            std::cout << colorer(">> " + ennemi.nom + " tombe !", VERT) << "\n";
        }
    }
}

// ===================== Les etats =====================

// Les etats d'un combattant, par exemple " [POISON 2] [BOUCLIER 15]"
std::string texteEtats(const Combattant& c) {
    std::string texte = "";
    if (c.poison > 0) {
        texte = texte + " " + colorer("[POISON " + std::to_string(c.poison) + "]", VERT);
    }
    if (c.brulure > 0) {
        texte = texte + " " + colorer("[BRULURE " + std::to_string(c.brulure) + "]", ROUGE);
    }
    if (c.saignement > 0) {
        texte = texte + " " + colorer("[SAIGNE " + std::to_string(c.saignement) + "]", ROUGE);
    }
    if (c.bouclier > 0) {
        texte = texte + " " + colorer("[BOUCLIER " + std::to_string(c.bouclier) + "]", CYAN);
    }
    return texte;
}

// Un etat fait effet : il retire des pv, puis dure un tour de moins
void unEtatFaitEffet(Combattant& c, int& toursRestants, int degats, const std::string& message) {
    if (toursRestants <= 0) {
        return;
    }
    c.pv = c.pv - degats;
    toursRestants = toursRestants - 1;
    std::cout << c.nom << " " << message << " : -" << colorer(degats, ROUGE) << " pv.\n";
}

// Au debut de son tour, un combattant subit ses etats
void subirEtats(Combattant& c) {
    unEtatFaitEffet(c, c.poison, 3, "souffre du poison");
    unEtatFaitEffet(c, c.brulure, 4, "brule");
    unEtatFaitEffet(c, c.saignement, 3, "saigne");
}

// A la fin d'un combat, les etats disparaissent
void effacerEtats(Combattant& c) {
    c.poison = 0;
    c.brulure = 0;
    c.saignement = 0;
    c.bouclier = 0;
}

// ===================== Les actions d'AYLIS =====================

// AYLIS frappe avec son arme. Renvoie le total des degats.
// Une arme a distance est moins efficace au contact (x0.6).
// Une arme qui frappe plusieurs fois peut faire saigner (1 chance sur 4 par coup).
int frapper(Combattant& aylis, Combattant& ennemi, int puissance) {
    const Arme& arme = aylis.arme;

    if (arme.aDistance && ennemi.distance == 0) {
        puissance = puissance * 60 / 100;
        std::cout << "(tir a bout portant, moins efficace) ";
    }

    int total = 0;
    bool faitSaigner = false;
    for (int coup = 0; coup < arme.nombreDeCoups; coup++) {
        total = total + calculerDegats(aylis.attaque + arme.bonusAttaque, puissance,
                                       ennemi.defense, arme.chanceCritique);
        if (arme.nombreDeCoups > 1 && std::rand() % 4 == 0) {
            faitSaigner = true;
        }
    }
    ennemi.pv = ennemi.pv - total;

    if (faitSaigner && ennemi.pv > 0) {
        ennemi.saignement = 3;
        std::cout << colorer("(" + ennemi.nom + " saigne !) ", ROUGE);
    }
    return total;
}

// Le menu des sorts. Renvoie true si un sort a ete lance (le tour est utilise).
// Les sorts marchent a n'importe quelle distance.
bool lancerSort(Combattant& aylis, std::vector<Combattant>& ennemis) {
    const int coutBouleDeFeu = 4;
    const int coutSoin = 5;
    const int coutEclair = 7;
    const int coutBouclier = 5;

    std::cout << "\nMana : " << aylis.mana << "/" << aylis.manaMax << "\n";
    std::cout << "1. Boule de feu (" << coutBouleDeFeu << " mana) : degats x1.5 qui ignorent la defense, et BRULE 2 tours\n";
    if (aylis.sortsConnus >= 2) {
        std::cout << "2. Soin (" << coutSoin << " mana) : +20 pv, et guerit poison, brulure et saignement\n";
    } else {
        std::cout << "2. ??? (pas encore appris)\n";
    }
    if (aylis.sortsConnus >= 3) {
        std::cout << "3. Eclair (" << coutEclair << " mana) : degats normaux + l'ennemi passe son tour\n";
    } else {
        std::cout << "3. ??? (pas encore appris)\n";
    }
    if (aylis.sortsConnus >= 4) {
        std::cout << "4. Bouclier (" << coutBouclier << " mana) : absorbe les 15 prochains points de degats\n";
    } else {
        std::cout << "4. ??? (pas encore appris)\n";
    }
    std::cout << "5. Retour\n";

    int choix = lireChoix(1, 5);
    if (choix == 5) {
        return false;
    }
    if (choix > aylis.sortsConnus) {
        std::cout << "AYLIS ne connait pas encore ce sort !\n";
        return false;
    }

    int prixMana = coutBouleDeFeu;
    if (choix == 2) {
        prixMana = coutSoin;
    } else if (choix == 3) {
        prixMana = coutEclair;
    } else if (choix == 4) {
        prixMana = coutBouclier;
    }
    if (aylis.mana < prixMana) {
        std::cout << "Pas assez de mana !\n";
        return false;
    }
    aylis.mana = aylis.mana - prixMana;

    if (choix == 2) {
        soigner(aylis, 20);
        aylis.poison = 0;
        aylis.brulure = 0;
        aylis.saignement = 0;
        std::cout << "SOIN ! AYLIS remonte a " << colorer(aylis.pv, VERT) << " pv, et ses blessures se referment.\n";
        return true;
    }
    if (choix == 4) {
        aylis.bouclier = 15;
        std::cout << colorer("BOUCLIER ! Une barriere de lumiere entoure AYLIS.", CYAN) << "\n";
        return true;
    }

    // Les sorts d'attaque : on choisit la cible, a n'importe quelle distance
    Combattant& cible = ennemis[choisirCible(ennemis, false)];
    if (choix == 1) {
        int degats = calculerDegats(aylis.attaque, 150, 0, 10);
        cible.pv = cible.pv - degats;
        cible.brulure = 2;
        std::cout << "BOULE DE FEU ! " << cible.nom << " perd " << colorer(degats, JAUNE) << " pv et prend feu !\n";
    } else {
        int degats = calculerDegats(aylis.attaque, 100, cible.defense, 10);
        cible.pv = cible.pv - degats;
        cible.etourdi = true;
        std::cout << "ECLAIR ! " << cible.nom << " perd " << colorer(degats, JAUNE) << " pv. Paralysie !\n";
    }
    return true;
}

// ===================== Le tour des ennemis =====================

// Un coup d'ennemi touche sa victime (AYLIS ou son compagnon).
// La garde d'AYLIS divise par 2, le bouclier absorbe, et les coups recus par AYLIS remplissent sa rage.
void toucher(const Combattant& ennemi, Combattant& victime, bool estAylis, int degats, bool aylisEnGarde, int& rage) {
    if (estAylis && aylisEnGarde) {
        degats = degats / 2;
        std::cout << "(AYLIS bloque la moitie du coup) ";
    }

    if (victime.bouclier > 0) {
        int absorbe = degats;
        if (absorbe > victime.bouclier) {
            absorbe = victime.bouclier;
        }
        victime.bouclier = victime.bouclier - absorbe;
        degats = degats - absorbe;
        std::cout << colorer("(le bouclier absorbe " + std::to_string(absorbe) + ") ", CYAN);
    }

    victime.pv = victime.pv - degats;
    std::cout << ennemi.nom << " touche " << victime.nom << " : -" << colorer(degats, ROUGE) << " pv.\n";

    // Les coups de certains ennemis empoisonnent (une chance sur deux)
    if (ennemi.attaquePoison && degats > 0 && std::rand() % 2 == 0) {
        victime.poison = 3;
        std::cout << colorer(victime.nom + " est empoisonne !", VERT) << "\n";
    }

    if (estAylis) {
        rage = rage + degats * 4;
        if (rage > rageMax) {
            rage = rageMax;
        }
    }
}

// Le tour d'un ennemi. Il attaque AYLIS, ou parfois son compagnon.
void tourEnnemi(Combattant& ennemi, Combattant& aylis, bool aylisEnGarde, int& rage,
                Combattant& compagnon, bool avecCompagnon) {
    // Les etats font effet en premier
    subirEtats(ennemi);
    if (ennemi.pv <= 0) {
        return;
    }

    // Un ennemi paralyse passe son tour
    if (ennemi.etourdi) {
        ennemi.etourdi = false;
        std::cout << "Paralysie : " << ennemi.nom << " ne peut pas attaquer ce tour !\n";
        return;
    }

    // Un boss sous la moitie de ses pv s'enrage : +3 en attaque, une seule fois
    if (ennemi.estBoss && !ennemi.enrage && ennemi.pv <= ennemi.pvMax / 2) {
        ennemi.enrage = true;
        ennemi.attaque = ennemi.attaque + 3;
        std::cout << colorer("!!! " + ennemi.nom + " S'ENRAGE ! Son attaque augmente ! !!!", ROUGE + GRAS) << "\n";
    }

    // Un boss en danger se soigne
    if (ennemi.estBoss && ennemi.pv <= 12 && ennemi.potions > 0) {
        boirePotion(ennemi);
        return;
    }

    // Qui est vise ? Le compagnon une fois sur trois, s'il est encore debout.
    // (condition ? A : B) veut dire "A si la condition est vraie, sinon B".
    bool viseCompagnon = avecCompagnon && compagnon.pv > 0 && std::rand() % 3 == 0;
    Combattant& victime = viseCompagnon ? compagnon : aylis;

    // L'ennemi est encore loin : ce qu'il fait depend de son style
    if (ennemi.distance > 0) {
        // Un lanceur tire une fois sur deux au lieu d'avancer
        if (ennemi.style == STYLE_LANCEUR && std::rand() % 2 == 0) {
            std::cout << ennemi.nom << " lance un javelot ! ";
            int degats = calculerDegats(ennemi.attaque, 80, victime.defense, 10);
            toucher(ennemi, victime, !viseCompagnon, degats, aylisEnGarde, rage);
            return;
        }

        // Un chargeur fonce directement au contact et frappe dans l'elan
        if (ennemi.style == STYLE_CHARGEUR) {
            ennemi.distance = 0;
            std::cout << ennemi.nom << " CHARGE et arrive au contact ! ";
            int degats = calculerDegats(ennemi.attaque, 70, victime.defense, 10);
            toucher(ennemi, victime, !viseCompagnon, degats, aylisEnGarde, rage);
            return;
        }

        // Les autres avancent d'un pas
        ennemi.distance = ennemi.distance - 1;
        if (ennemi.distance == 0) {
            std::cout << ennemi.nom << " arrive au contact !\n";
        } else {
            std::cout << ennemi.nom << " s'approche...\n";
        }
        return;
    }

    // Un ennemi normal attaque toujours normalement.
    // Un boss tente une attaque lourde si AYLIS est faible, sinon 1 fois sur 4.
    bool attaqueLourde = false;
    if (ennemi.estBoss) {
        attaqueLourde = aylis.pv <= 15 || std::rand() % 4 == 0;
    }

    int degats = 0;
    if (attaqueLourde) {
        if (std::rand() % 100 < 60) {
            std::cout << ennemi.nom << " prepare une attaque lourde... ";
            degats = calculerDegats(ennemi.attaque, 180, victime.defense, 10);
        } else {
            std::cout << ennemi.nom << " tente une attaque lourde... et rate !\n";
            return;
        }
    } else {
        degats = calculerDegats(ennemi.attaque, 100, victime.defense, 10);
    }

    toucher(ennemi, victime, !viseCompagnon, degats, aylisEnGarde, rage);
}

// ===================== Le compagnon =====================

// Le compagnon subit ses etats, puis attaque un ennemi au hasard (il se bat a la fronde : a toute distance)
void tourCompagnon(Combattant& compagnon, std::vector<Combattant>& ennemis) {
    subirEtats(compagnon);
    if (compagnon.pv <= 0) {
        return;
    }

    int numero = ennemiDeboutAuHasard(ennemis);
    if (numero == -1) {
        return;
    }
    Combattant& cible = ennemis[numero];
    int degats = calculerDegats(compagnon.attaque, 100, cible.defense, 10);
    cible.pv = cible.pv - degats;
    std::cout << colorer(compagnon.nom, CYAN + GRAS) << " attaque " << cible.nom << " : -"
              << colorer(degats, JAUNE) << " pv.\n";
}

// ===================== La boucle de combat =====================

// Affiche l'etat du combat : AYLIS, son compagnon, puis chaque ennemi encore debout
void afficherEtat(const Combattant& aylis, const std::vector<Combattant>& ennemis, int rage,
                  const Combattant& compagnon, bool avecCompagnon) {
    std::cout << "AYLIS niv." << aylis.niveau << "  ";
    afficherBarre(aylis.pv, aylis.pvMax);
    std::cout << " " << aylis.pv << "/" << aylis.pvMax << " pv" << texteEtats(aylis) << "\n";

    std::cout << "Mana         ";
    afficherBarre(aylis.mana, aylis.manaMax, BLEU);
    std::cout << " " << aylis.mana << "/" << aylis.manaMax << "\n";

    std::cout << "Rage         ";
    afficherBarre(rage, rageMax, ROUGE);
    if (rage >= rageMax) {
        std::cout << " PLEINE !";
    }
    std::cout << "\n";

    if (avecCompagnon) {
        std::cout << colorer(compagnon.nom, CYAN + GRAS) << " (compagnon)\n             ";
        if (compagnon.pv > 0) {
            afficherBarre(compagnon.pv, compagnon.pvMax);
            std::cout << " " << compagnon.pv << "/" << compagnon.pvMax << " pv" << texteEtats(compagnon) << "\n";
        } else {
            std::cout << colorer("K.O.", GRIS) << "\n";
        }
    }

    std::cout << "Arme : ";
    afficherArme(aylis.arme);
    std::cout << "\n\n";

    for (const Combattant& ennemi : ennemis) {
        if (ennemi.pv <= 0) {
            continue;
        }
        std::cout << ennemi.nom << "  (" << nomDistance(ennemi.distance) << ")" << texteEtats(ennemi) << "\n             ";
        afficherBarre(ennemi.pv, ennemi.pvMax);
        std::cout << " " << ennemi.pv << "/" << ennemi.pvMax << " pv\n";
    }
    std::cout << "\n";
}

bool combattre(Combattant& aylis, std::vector<Combattant>& ennemis, int& rage,
               Combattant& compagnon, bool avecCompagnon) {
    int tour = 1;
    aylis.mana = aylis.manaMax;     // le mana se recharge au debut de chaque combat
    for (Combattant& ennemi : ennemis) {
        ennemi.distance = distanceDepart;   // chaque combat commence de loin
    }
    bool compagnonKO = false;       // pour n'annoncer sa chute qu'une fois

    int dernierTourAffiche = 0;

    while (aylis.pv > 0 && resteDesEnnemis(ennemis)) {
        // Au debut de chaque nouveau tour : les etats d'AYLIS, puis l'etat du combat.
        // Si une action est refusee, on redemande juste le choix.
        if (tour != dernierTourAffiche) {
            std::cout << "\n--- Tour " << tour << " ---\n";
            subirEtats(aylis);
            if (aylis.pv <= 0) {
                break;
            }
            afficherEtat(aylis, ennemis, rage, compagnon, avecCompagnon);
            dernierTourAffiche = tour;
        }

        // Est-ce qu'au moins un ennemi est encore a distance ?
        bool quelquUnEstLoin = false;
        for (const Combattant& ennemi : ennemis) {
            if (ennemi.pv > 0 && ennemi.distance > 0) {
                quelquUnEstLoin = true;
            }
        }

        // Est-ce que l'arme peut toucher quelqu'un ? (une arme de melee a besoin d'un ennemi au contact)
        bool armeAPortee = quelquUnAPortee(ennemis, !aylis.arme.aDistance);
        std::string tropLoin = "";
        if (!armeAPortee) {
            tropLoin = "  -> TROP LOIN, avance d'abord (7)";
            std::cout << "(Les ennemis sont trop loin pour " << aylis.arme.nom
                      << " : avance vers eux avec 7, ou lance un sort avec 5.)\n";
        }

        std::cout << "1. Attaque normale   (degats normaux, ne rate jamais)" << tropLoin << "\n";
        std::cout << "2. Attaque lourde    (degats x1.8, mais 40% de chances de rater)" << tropLoin << "\n";
        std::cout << "3. Attaque en garde  (petits degats, mais les ennemis tapent 2x moins fort)" << tropLoin << "\n";
        std::cout << "4. Boire une potion  (+15 pv, reste " << aylis.potions << ")\n";
        std::cout << "5. Lancer un sort\n";
        if (rage >= rageMax) {
            std::cout << "6. ATTAQUE SPECIALE  (degats x2.2, ne rate jamais)\n";
        }
        if (quelquUnEstLoin) {
            std::cout << "7. Avancer vers les ennemis\n";
        }

        int choix = lireChoix(1, 7);

        if (choix == 6 && rage < rageMax) {
            std::cout << "La rage n'est pas encore pleine !\n";
            continue;
        }

        // Est-ce qu'AYLIS se protege pendant ce tour ?
        bool enGarde = false;

        // ===== Tour du joueur =====
        bool attaqueAvecArme = choix == 1 || choix == 2 || choix == 3 || choix == 6;
        if (attaqueAvecArme) {
            // Une arme de melee ne peut frapper que les ennemis au contact
            int numeroCible = choisirCible(ennemis, !aylis.arme.aDistance);
            if (numeroCible == -1) {
                std::cout << "\n" << colorer("!! Impossible", JAUNE + GRAS) << " : personne a portee de "
                          << aylis.arme.nom << ". Avance d'abord (7), ou lance un sort (5).\n\n";
                continue;
            }
            Combattant& cible = ennemis[numeroCible];

            if (choix == 1) {
                int degats = frapper(aylis, cible, 100);
                std::cout << "Attaque normale ! " << cible.nom << " perd " << colorer(degats, JAUNE) << " pv.\n";
            } else if (choix == 2) {
                // 60 chances sur 100 de toucher
                if (std::rand() % 100 < 60) {
                    int degats = frapper(aylis, cible, 180);
                    std::cout << "Attaque lourde ! BAM ! " << cible.nom << " perd " << colorer(degats, JAUNE) << " pv.\n";
                } else {
                    std::cout << "Attaque lourde... ratee ! " << cible.nom << " esquive.\n";
                }
            } else if (choix == 3) {
                int degats = frapper(aylis, cible, 60);
                enGarde = true;
                std::cout << "AYLIS attaque en restant en garde. " << cible.nom << " perd "
                          << colorer(degats, JAUNE) << " pv.\n";
            } else {
                int degats = frapper(aylis, cible, 220);
                rage = 0;
                std::cout << "*** ATTAQUE SPECIALE ! *** " << cible.nom << " perd " << colorer(degats, JAUNE) << " pv !\n";
            }
        } else if (choix == 4) {
            if (aylis.potions == 0) {
                std::cout << "Plus de potions ! Choisis autre chose.\n";
                continue;
            }
            boirePotion(aylis);
        } else if (choix == 5) {
            if (!lancerSort(aylis, ennemis)) {
                continue;   // pas de sort lance : on revient au menu sans perdre le tour
            }
        } else {
            if (!quelquUnEstLoin) {
                std::cout << "AYLIS est deja au contact de tout le monde !\n";
                continue;
            }
            // AYLIS avance : tous les ennemis encore loin se rapprochent d'un pas
            for (Combattant& ennemi : ennemis) {
                if (ennemi.pv > 0 && ennemi.distance > 0) {
                    ennemi.distance = ennemi.distance - 1;
                }
            }
            std::cout << "AYLIS avance vers les ennemis.\n";
        }

        annoncerChutes(ennemis);

        // ===== Tour du compagnon =====
        if (avecCompagnon && compagnon.pv > 0 && resteDesEnnemis(ennemis)) {
            tourCompagnon(compagnon, ennemis);
            annoncerChutes(ennemis);
        }

        // ===== Tour des ennemis : chacun a son tour =====
        for (Combattant& ennemi : ennemis) {
            if (ennemi.pv <= 0) {
                continue;
            }
            tourEnnemi(ennemi, aylis, enGarde, rage, compagnon, avecCompagnon);
            annoncerChutes(ennemis);    // un ennemi peut tomber a cause de ses etats
            if (aylis.pv <= 0) {
                break;
            }
        }

        if (avecCompagnon && compagnon.pv <= 0 && !compagnonKO) {
            compagnonKO = true;
            std::cout << colorer(compagnon.nom + " est K.O. et ne peut plus combattre !", GRIS) << "\n";
        }

        tour = tour + 1;
    }

    // Fin du combat : les etats disparaissent
    effacerEtats(aylis);
    effacerEtats(compagnon);
    return aylis.pv > 0;
}
