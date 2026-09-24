// combat.cpp : les attaques, les sorts, le tour des ennemis et la boucle de combat
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "combat.h"
#include "outils.h"

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

// Annonce les ennemis qui viennent de tomber
void annoncerChutes(std::vector<Combattant>& ennemis) {
    for (Combattant& ennemi : ennemis) {
        if (ennemi.pv <= 0 && !ennemi.vaincu) {
            ennemi.vaincu = true;
            std::cout << ">> " << ennemi.nom << " tombe !\n";
        }
    }
}

// ===================== Les actions d'AYLIS =====================

// AYLIS frappe avec son arme. Renvoie le total des degats.
// Une arme a distance est moins efficace au contact (x0.6).
int frapper(Combattant& aylis, Combattant& ennemi, int puissance) {
    const Arme& arme = aylis.arme;

    if (arme.aDistance && ennemi.distance == 0) {
        puissance = puissance * 60 / 100;
        std::cout << "(tir a bout portant, moins efficace) ";
    }

    int total = 0;
    for (int coup = 0; coup < arme.nombreDeCoups; coup++) {
        total = total + calculerDegats(aylis.attaque + arme.bonusAttaque, puissance,
                                       ennemi.defense, arme.chanceCritique);
    }
    ennemi.pv = ennemi.pv - total;
    return total;
}

// Le menu des sorts. Renvoie true si un sort a ete lance (le tour est utilise).
// Les sorts marchent a n'importe quelle distance.
bool lancerSort(Combattant& aylis, std::vector<Combattant>& ennemis) {
    const int coutBouleDeFeu = 4;
    const int coutSoin = 5;
    const int coutEclair = 7;

    std::cout << "\nMana : " << aylis.mana << "/" << aylis.manaMax << "\n";
    std::cout << "1. Boule de feu (" << coutBouleDeFeu << " mana) : degats x1.5 qui ignorent la defense\n";
    if (aylis.sortsConnus >= 2) {
        std::cout << "2. Soin (" << coutSoin << " mana) : +20 pv\n";
    } else {
        std::cout << "2. ??? (pas encore appris)\n";
    }
    if (aylis.sortsConnus >= 3) {
        std::cout << "3. Eclair (" << coutEclair << " mana) : degats normaux + l'ennemi passe son tour\n";
    } else {
        std::cout << "3. ??? (pas encore appris)\n";
    }
    std::cout << "4. Retour\n";

    int choix = lireChoix(1, 4);
    if (choix == 4) {
        return false;
    }
    if (choix > aylis.sortsConnus) {
        std::cout << "AYLIS ne connait pas encore ce sort !\n";
        return false;
    }

    int prixMana = 0;
    if (choix == 1) {
        prixMana = coutBouleDeFeu;
    } else if (choix == 2) {
        prixMana = coutSoin;
    } else {
        prixMana = coutEclair;
    }
    if (aylis.mana < prixMana) {
        std::cout << "Pas assez de mana !\n";
        return false;
    }
    aylis.mana = aylis.mana - prixMana;

    if (choix == 2) {
        soigner(aylis, 20);
        std::cout << "SOIN ! AYLIS remonte a " << aylis.pv << " pv.\n";
        return true;
    }

    // Les sorts d'attaque : on choisit la cible, a n'importe quelle distance
    Combattant& cible = ennemis[choisirCible(ennemis, false)];
    if (choix == 1) {
        int degats = calculerDegats(aylis.attaque, 150, 0, 10);
        cible.pv = cible.pv - degats;
        std::cout << "BOULE DE FEU ! " << cible.nom << " perd " << degats << " pv.\n";
    } else {
        int degats = calculerDegats(aylis.attaque, 100, cible.defense, 10);
        cible.pv = cible.pv - degats;
        cible.etourdi = true;
        std::cout << "ECLAIR ! " << cible.nom << " perd " << degats << " pv. Paralysie !\n";
    }
    return true;
}

// ===================== Le tour des ennemis =====================

// AYLIS encaisse un coup : la garde divise par 2, et la rage se remplit
void toucherAylis(const Combattant& ennemi, Combattant& aylis, int degats, bool aylisEnGarde, int& rage) {
    if (aylisEnGarde) {
        degats = degats / 2;
        std::cout << "(AYLIS bloque la moitie du coup) ";
    }
    aylis.pv = aylis.pv - degats;
    std::cout << ennemi.nom << " touche ! AYLIS perd " << degats << " pv.\n";

    // Chaque coup recu remplit la rage
    rage = rage + degats * 4;
    if (rage > rageMax) {
        rage = rageMax;
    }
}

// Le tour d'un ennemi. Le & veut dire qu'on modifie les vrais combattants, pas des copies.
void tourEnnemi(Combattant& ennemi, Combattant& aylis, bool aylisEnGarde, int& rage) {
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
        std::cout << "!!! " << ennemi.nom << " S'ENRAGE ! Son attaque augmente ! !!!\n";
    }

    // Un boss en danger se soigne
    if (ennemi.estBoss && ennemi.pv <= 12 && ennemi.potions > 0) {
        boirePotion(ennemi);
        return;
    }

    // L'ennemi est encore loin : ce qu'il fait depend de son style
    if (ennemi.distance > 0) {
        // Un lanceur tire une fois sur deux au lieu d'avancer
        if (ennemi.style == STYLE_LANCEUR && std::rand() % 2 == 0) {
            std::cout << ennemi.nom << " lance un javelot ! ";
            int degats = calculerDegats(ennemi.attaque, 80, aylis.defense, 10);
            toucherAylis(ennemi, aylis, degats, aylisEnGarde, rage);
            return;
        }

        // Un chargeur fonce directement au contact et frappe dans l'elan
        if (ennemi.style == STYLE_CHARGEUR) {
            ennemi.distance = 0;
            std::cout << ennemi.nom << " CHARGE et arrive au contact ! ";
            int degats = calculerDegats(ennemi.attaque, 70, aylis.defense, 10);
            toucherAylis(ennemi, aylis, degats, aylisEnGarde, rage);
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
            degats = calculerDegats(ennemi.attaque, 180, aylis.defense, 10);
        } else {
            std::cout << ennemi.nom << " tente une attaque lourde... et rate !\n";
            return;
        }
    } else {
        degats = calculerDegats(ennemi.attaque, 100, aylis.defense, 10);
    }

    toucherAylis(ennemi, aylis, degats, aylisEnGarde, rage);
}

// ===================== La boucle de combat =====================

// Affiche l'etat du combat : AYLIS, puis chaque ennemi encore debout
void afficherEtat(const Combattant& aylis, const std::vector<Combattant>& ennemis, int rage) {
    std::cout << "AYLIS niv." << aylis.niveau << "  ";
    afficherBarre(aylis.pv, aylis.pvMax);
    std::cout << " " << aylis.pv << "/" << aylis.pvMax << " pv\n";

    std::cout << "Mana         ";
    afficherBarre(aylis.mana, aylis.manaMax);
    std::cout << " " << aylis.mana << "/" << aylis.manaMax << "\n";

    std::cout << "Rage         ";
    afficherBarre(rage, rageMax);
    if (rage >= rageMax) {
        std::cout << " PLEINE !";
    }
    std::cout << "\n";

    std::cout << "Arme : ";
    afficherArme(aylis.arme);
    std::cout << "\n\n";

    for (const Combattant& ennemi : ennemis) {
        if (ennemi.pv <= 0) {
            continue;
        }
        std::cout << ennemi.nom << "  (" << nomDistance(ennemi.distance) << ")\n             ";
        afficherBarre(ennemi.pv, ennemi.pvMax);
        std::cout << " " << ennemi.pv << "/" << ennemi.pvMax << " pv\n";
    }
    std::cout << "\n";
}

bool combattre(Combattant& aylis, std::vector<Combattant>& ennemis, int& rage) {
    int tour = 1;
    aylis.mana = aylis.manaMax;     // le mana se recharge au debut de chaque combat
    for (Combattant& ennemi : ennemis) {
        ennemi.distance = distanceDepart;   // chaque combat commence de loin
    }

    while (aylis.pv > 0 && resteDesEnnemis(ennemis)) {
        std::cout << "\n--- Tour " << tour << " ---\n";
        afficherEtat(aylis, ennemis, rage);

        // Est-ce qu'au moins un ennemi est encore a distance ?
        bool quelquUnEstLoin = false;
        for (const Combattant& ennemi : ennemis) {
            if (ennemi.pv > 0 && ennemi.distance > 0) {
                quelquUnEstLoin = true;
            }
        }

        std::cout << "1. Attaque normale   (degats normaux, ne rate jamais)\n";
        std::cout << "2. Attaque lourde    (degats x1.8, mais 40% de chances de rater)\n";
        std::cout << "3. Attaque en garde  (petits degats, mais les ennemis tapent 2x moins fort)\n";
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
                std::cout << "Personne a portee de " << aylis.arme.nom
                          << " ! Avance d'abord (choix 7), ou utilise un sort.\n";
                continue;
            }
            Combattant& cible = ennemis[numeroCible];

            if (choix == 1) {
                int degats = frapper(aylis, cible, 100);
                std::cout << "Attaque normale ! " << cible.nom << " perd " << degats << " pv.\n";
            } else if (choix == 2) {
                // 60 chances sur 100 de toucher
                if (std::rand() % 100 < 60) {
                    int degats = frapper(aylis, cible, 180);
                    std::cout << "Attaque lourde ! BAM ! " << cible.nom << " perd " << degats << " pv.\n";
                } else {
                    std::cout << "Attaque lourde... ratee ! " << cible.nom << " esquive.\n";
                }
            } else if (choix == 3) {
                int degats = frapper(aylis, cible, 60);
                enGarde = true;
                std::cout << "AYLIS attaque en restant en garde. " << cible.nom << " perd " << degats << " pv.\n";
            } else {
                int degats = frapper(aylis, cible, 220);
                rage = 0;
                std::cout << "*** ATTAQUE SPECIALE ! *** " << cible.nom << " perd " << degats << " pv !\n";
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

        // ===== Tour des ennemis : chacun a son tour =====
        for (Combattant& ennemi : ennemis) {
            if (ennemi.pv <= 0) {
                continue;
            }
            tourEnnemi(ennemi, aylis, enGarde, rage);
            if (aylis.pv <= 0) {
                break;
            }
        }

        tour = tour + 1;
    }

    return aylis.pv > 0;
}
