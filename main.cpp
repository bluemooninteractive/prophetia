#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

// Une arme : de melee (corps a corps) ou a distance
struct Arme {
    std::string nom;
    bool aDistance;         // true = arc, arbalete... / false = epee, hache...
    int bonusAttaque;       // s'ajoute a l'attaque d'AYLIS (peut etre negatif)
    int chanceCritique;     // sur 100 : 10 = 1 chance sur 10
    int nombreDeCoups;      // 2 = l'arme frappe deux fois par attaque
    int prix;
};

// Un combattant regroupe toutes ses stats dans un seul "paquet"
struct Combattant {
    std::string nom;
    int pv;
    int pvMax;
    int attaque;
    int defense;
    int potions;
    bool estBoss;
    int xpDonne;            // l'XP que l'ennemi donne quand il est vaincu
    int orDonne;            // les pieces d'or que l'ennemi laisse tomber
    bool enrage = false;    // un boss s'enrage une fois quand il passe sous la moitie de ses pv
    bool etourdi = false;   // un ennemi paralyse passe son prochain tour
    int niveau = 1;
    int xp = 0;
    int points = 0;         // points de competence a depenser
    int pieces = 0;         // l'or ("or" est un mot reserve en C++, on ne peut pas l'utiliser)
    int mana = 10;
    int manaMax = 10;
    int sortsConnus = 1;    // 1 = Boule de feu, 2 = + Soin, 3 = + Eclair
    Arme arme = {"Epee courte", false, 0, 10, 1, 0};
};

const int rageMax = 100;
const int nombreDeSorts = 3;
const int distanceDepart = 2;       // 2 = loin, 1 = proche, 0 = au contact

// Lit un choix entre min et max. Redemande tant que ce n'est pas valide.
int lireChoix(int min, int max) {
    while (true) {
        std::cout << "Ton choix : ";
        int choix;
        std::cin >> choix;

        if (!std::cin) {
            // Plus rien a lire (console fermee) : on quitte le jeu
            if (std::cin.eof()) {
                std::exit(0);
            }
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        } else if (choix >= min && choix <= max) {
            return choix;
        }
        std::cout << "Tape un nombre entre " << min << " et " << max << ".\n";
    }
}

// Calcule les degats d'une attaque : attaque x puissance - defense,
// plus un peu de hasard et parfois un coup critique.
// La puissance est en pourcentage : 100 = normal, 180 = presque le double.
// chanceCritique est sur 100 : 10 = 1 chance sur 10 de faire x2.
int calculerDegats(int attaque, int puissance, int defense, int chanceCritique) {
    int degats = attaque * puissance / 100 - defense;

    // Hasard : -1, 0 ou +1
    degats = degats + (std::rand() % 3) - 1;

    // Coup critique : degats x2
    if (std::rand() % 100 < chanceCritique) {
        degats = degats * 2;
        std::cout << "COUP CRITIQUE ! ";
    }

    // On fait toujours au moins 1 point de degats
    if (degats < 1) {
        degats = 1;
    }

    return degats;
}

// Affiche une barre comme [##########----------]
void afficherBarre(int valeur, int maximum) {
    const int largeur = 20;
    if (valeur < 0) {
        valeur = 0;
    }
    int remplis = valeur * largeur / maximum;

    std::cout << "[";
    for (int i = 0; i < largeur; i++) {
        if (i < remplis) {
            std::cout << "#";
        } else {
            std::cout << "-";
        }
    }
    std::cout << "]";
}

// Affiche la description d'une arme sur une ligne
void afficherArme(const Arme& arme) {
    std::cout << arme.nom;
    if (arme.aDistance) {
        std::cout << " [DISTANCE]";
    } else {
        std::cout << " [MELEE]";
    }
    std::cout << "  attaque ";
    if (arme.bonusAttaque >= 0) {
        std::cout << "+";
    }
    std::cout << arme.bonusAttaque << ", critique " << arme.chanceCritique << "%";
    if (arme.nombreDeCoups > 1) {
        std::cout << ", frappe " << arme.nombreDeCoups << " fois";
    }
}

// Soigne un combattant sans depasser ses pv max
void soigner(Combattant& c, int quantite) {
    c.pv = c.pv + quantite;
    if (c.pv > c.pvMax) {
        c.pv = c.pvMax;
    }
}

// Boire une potion : +15 pv
void boirePotion(Combattant& c) {
    c.potions = c.potions - 1;
    soigner(c, 15);
    std::cout << c.nom << " boit une potion ! Retour a " << c.pv << " pv.\n";
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

        std::cout << "\n*** NIVEAU " << aylis.niveau << " ! ***\n";
        std::cout << "+2 points de competence. AYLIS retrouve tous ses pv !\n";
    }

    std::cout << "XP         ";
    afficherBarre(aylis.xp, xpPourNiveauSuivant(aylis.niveau));
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
                } else {
                    std::cout << "Nouveau sort appris : ECLAIR !\n";
                }
            }
            std::cout << "Mana max : " << aylis.manaMax << ".\n";
        }
    }
}

// ===================== Le marche =====================

// L'armurerie : acheter une nouvelle arme. L'ancienne est laissee au marchand.
void armurerie(Combattant& aylis, const std::vector<Arme>& armes) {
    int nombreArmes = armes.size();

    while (true) {
        std::cout << "\n=== ARMURERIE ===   Tu as " << aylis.pieces << " pieces d'or\n";
        std::cout << "Arme actuelle : ";
        afficherArme(aylis.arme);
        std::cout << "\n\n";

        for (int i = 0; i < nombreArmes; i++) {
            std::cout << (i + 1) << ". ";
            afficherArme(armes[i]);
            std::cout << "  ...  " << armes[i].prix << " or\n";
        }
        std::cout << (nombreArmes + 1) << ". Retour\n";

        int choix = lireChoix(1, nombreArmes + 1);
        if (choix == nombreArmes + 1) {
            return;
        }

        const Arme& arme = armes[choix - 1];
        if (arme.nom == aylis.arme.nom) {
            std::cout << "AYLIS a deja cette arme en main !\n";
        } else if (aylis.pieces < arme.prix) {
            std::cout << "Pas assez d'or ! Il te manque " << (arme.prix - aylis.pieces) << " pieces.\n";
        } else {
            aylis.pieces = aylis.pieces - arme.prix;
            aylis.arme = arme;
            std::cout << "AYLIS s'equipe : " << arme.nom << " !\n";
        }
    }
}

void marche(Combattant& aylis, const std::vector<Arme>& armes) {
    const int prixPotion = 15;
    const int prixArmure = 35;
    const int prixElixir = 30;

    while (true) {
        std::cout << "\n=== LE MARCHE ===   Tu as " << aylis.pieces << " pieces d'or\n";
        std::cout << "1. Potion (+15 pv en combat) ......... " << prixPotion << " or  (tu en as " << aylis.potions << ")\n";
        std::cout << "2. Armure renforcee (+1 defense) ..... " << prixArmure << " or\n";
        std::cout << "3. Elixir de mana (+5 mana max) ...... " << prixElixir << " or\n";
        std::cout << "4. Armurerie (acheter une arme)\n";
        std::cout << "5. Reprendre la route\n";

        int choix = lireChoix(1, 5);
        if (choix == 5) {
            return;
        }
        if (choix == 4) {
            armurerie(aylis, armes);
            continue;
        }

        // Le prix de l'objet choisi
        int prix = 0;
        if (choix == 1) {
            prix = prixPotion;
        } else if (choix == 2) {
            prix = prixArmure;
        } else {
            prix = prixElixir;
        }

        if (aylis.pieces < prix) {
            std::cout << "Pas assez d'or ! Il te manque " << (prix - aylis.pieces) << " pieces.\n";
            continue;
        }
        aylis.pieces = aylis.pieces - prix;

        if (choix == 1) {
            aylis.potions = aylis.potions + 1;
            std::cout << "Achete ! " << aylis.potions << " potions.\n";
        } else if (choix == 2) {
            aylis.defense = aylis.defense + 1;
            std::cout << "Achete ! Defense : " << aylis.defense << ".\n";
        } else {
            aylis.manaMax = aylis.manaMax + 5;
            std::cout << "Achete ! Mana max : " << aylis.manaMax << ".\n";
        }
    }
}

// ===================== Le combat =====================

// AYLIS frappe avec son arme. Renvoie le total des degats.
// Une arme a distance est moins efficace au contact (x0.6).
int frapper(Combattant& aylis, Combattant& ennemi, int puissance, int distance) {
    const Arme& arme = aylis.arme;

    if (arme.aDistance && distance == 0) {
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
bool lancerSort(Combattant& aylis, Combattant& ennemi) {
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

    if (choix == 1) {
        int degats = calculerDegats(aylis.attaque, 150, 0, 10);
        ennemi.pv = ennemi.pv - degats;
        std::cout << "BOULE DE FEU ! " << ennemi.nom << " perd " << degats << " pv.\n";
    } else if (choix == 2) {
        soigner(aylis, 20);
        std::cout << "SOIN ! AYLIS remonte a " << aylis.pv << " pv.\n";
    } else {
        int degats = calculerDegats(aylis.attaque, 100, ennemi.defense, 10);
        ennemi.pv = ennemi.pv - degats;
        ennemi.etourdi = true;
        std::cout << "ECLAIR ! " << ennemi.nom << " perd " << degats << " pv. Paralysie !\n";
    }
    return true;
}

// Le tour de l'ennemi. Le & veut dire qu'on modifie les vrais combattants, pas des copies.
void tourEnnemi(Combattant& ennemi, Combattant& aylis, bool aylisEnGarde, int& rage, int& distance) {
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

    // Les Haschen se battent au corps a corps : s'ils sont loin, ils s'approchent
    if (distance > 0) {
        distance = distance - 1;
        if (distance == 0) {
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

    if (aylisEnGarde) {
        degats = degats / 2;
        std::cout << "(AYLIS bloque la moitie du coup) ";
    }
    aylis.pv = aylis.pv - degats;
    std::cout << ennemi.nom << " frappe ! AYLIS perd " << degats << " pv.\n";

    // Chaque coup recu remplit la rage
    rage = rage + degats * 4;
    if (rage > rageMax) {
        rage = rageMax;
    }
}

// Un combat complet contre un ennemi. Renvoie true si AYLIS gagne.
bool combattre(Combattant& aylis, Combattant& ennemi, int& rage) {
    int tour = 1;
    int distance = distanceDepart;  // chaque combat commence de loin
    aylis.mana = aylis.manaMax;     // le mana se recharge au debut de chaque combat

    while (aylis.pv > 0 && ennemi.pv > 0) {
        std::cout << "\n--- Tour " << tour << " ---\n";

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

        std::cout << ennemi.nom << "\n             ";
        afficherBarre(ennemi.pv, ennemi.pvMax);
        std::cout << " " << ennemi.pv << "/" << ennemi.pvMax << " pv\n";

        std::cout << "Distance : ";
        if (distance == 2) {
            std::cout << "LOIN";
        } else if (distance == 1) {
            std::cout << "PROCHE";
        } else {
            std::cout << "AU CONTACT";
        }
        std::cout << "   |   Arme : ";
        afficherArme(aylis.arme);
        std::cout << "\n\n";

        std::cout << "1. Attaque normale   (degats normaux, ne rate jamais)\n";
        std::cout << "2. Attaque lourde    (degats x1.8, mais 40% de chances de rater)\n";
        std::cout << "3. Attaque en garde  (petits degats, mais l'ennemi tape 2x moins fort)\n";
        std::cout << "4. Boire une potion  (+15 pv, reste " << aylis.potions << ")\n";
        std::cout << "5. Lancer un sort\n";
        if (rage >= rageMax) {
            std::cout << "6. ATTAQUE SPECIALE  (degats x2.2, ne rate jamais)\n";
        }
        if (distance > 0) {
            std::cout << "7. Avancer vers l'ennemi\n";
        }

        int choix = lireChoix(1, 7);

        // Les choix 1, 2, 3 et 6 sont des attaques avec l'arme :
        // une arme de melee ne peut pas frapper de loin
        bool attaqueAvecArme = choix == 1 || choix == 2 || choix == 3 || choix == 6;
        if (attaqueAvecArme && !aylis.arme.aDistance && distance > 0) {
            std::cout << "Trop loin pour frapper avec " << aylis.arme.nom
                      << " ! Avance d'abord (choix 7), ou utilise un sort.\n";
            continue;
        }

        // Est-ce qu'AYLIS se protege pendant ce tour ?
        bool enGarde = false;

        // ===== Tour du joueur =====
        if (choix == 1) {
            int degats = frapper(aylis, ennemi, 100, distance);
            std::cout << "Attaque normale ! " << ennemi.nom << " perd " << degats << " pv.\n";
        } else if (choix == 2) {
            // 60 chances sur 100 de toucher
            if (std::rand() % 100 < 60) {
                int degats = frapper(aylis, ennemi, 180, distance);
                std::cout << "Attaque lourde ! BAM ! " << ennemi.nom << " perd " << degats << " pv.\n";
            } else {
                std::cout << "Attaque lourde... ratee ! " << ennemi.nom << " esquive.\n";
            }
        } else if (choix == 3) {
            int degats = frapper(aylis, ennemi, 60, distance);
            enGarde = true;
            std::cout << "AYLIS attaque en restant en garde. " << ennemi.nom << " perd " << degats << " pv.\n";
        } else if (choix == 4) {
            if (aylis.potions == 0) {
                std::cout << "Plus de potions ! Choisis autre chose.\n";
                continue;
            }
            boirePotion(aylis);
        } else if (choix == 5) {
            if (!lancerSort(aylis, ennemi)) {
                continue;   // pas de sort lance : on revient au menu sans perdre le tour
            }
        } else if (choix == 6) {
            if (rage < rageMax) {
                std::cout << "La rage n'est pas encore pleine !\n";
                continue;
            }
            int degats = frapper(aylis, ennemi, 220, distance);
            rage = 0;
            std::cout << "*** ATTAQUE SPECIALE ! *** " << ennemi.nom << " perd " << degats << " pv !\n";
        } else {
            if (distance == 0) {
                std::cout << "AYLIS est deja au contact !\n";
                continue;
            }
            distance = distance - 1;
            std::cout << "AYLIS avance vers " << ennemi.nom << ".\n";
        }

        if (ennemi.pv <= 0) {
            break;
        }

        // ===== Tour de l'ennemi =====
        tourEnnemi(ennemi, aylis, enGarde, rage, distance);

        tour = tour + 1;
    }

    return aylis.pv > 0;
}

int main() {
    // Pour que le hasard change a chaque partie
    std::srand(std::time(nullptr));

    //                   nom            pv  pvMax att  def potions boss   xp  or
    Combattant aylis = {"AYLIS",        40, 40,   12,  4,  3,      false, 0,  0};
    int rage = 0;

    // Les armes vendues a l'armurerie
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

    // La liste des ennemis, dans l'ordre (les deux derniers nombres = XP et or donnes)
    std::vector<Combattant> ennemis = {
        {"Haschen eclaireur",              18, 18,  8, 1, 0, false,  20, 20},
        {"Haschen guerrier",               22, 22,  9, 2, 0, false,  25, 25},
        {"Ashka, Matriarche des Haschen",  40, 40, 14, 4, 1, true,   50, 50},
        {"Haschen berserker",              30, 30, 15, 5, 0, false,  30, 30},
        {"Vorgath le Destructeur",         60, 60, 17, 6, 2, true,  100,  0},
    };

    std::cout << "=== AYLIS contre les Haschen : la route vers Vorgath le Destructeur ===\n";

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

    int nombreEnnemis = ennemis.size();

    for (int i = 0; i < nombreEnnemis; i++) {
        Combattant& ennemi = ennemis[i];

        std::cout << "\n==========================================\n";
        if (ennemi.estBoss) {
            std::cout << "  !!! BOSS : " << ennemi.nom << " !!!\n";
        } else {
            std::cout << "  Combat " << (i + 1) << "/" << nombreEnnemis << " : " << ennemi.nom << " apparait !\n";
        }
        std::cout << "==========================================\n";

        bool victoire = combattre(aylis, ennemi, rage);

        if (!victoire) {
            std::cout << "\n=== GAME OVER ===\n";
            std::cout << "AYLIS tombe au combat face a " << ennemi.nom << ".\n";
            return 0;
        }

        std::cout << "\nVictoire contre " << ennemi.nom << " !\n";

        // Le dernier ennemi : la partie est gagnee, pas besoin de marche
        if (i == nombreEnnemis - 1) {
            break;
        }

        // Les recompenses
        aylis.pieces = aylis.pieces + ennemi.orDonne;
        std::cout << "AYLIS ramasse " << ennemi.orDonne << " pieces d'or (" << aylis.pieces << " en tout).\n";
        gagnerXp(aylis, ennemi.xpDonne);
        depenserPoints(aylis);

        soigner(aylis, 10);
        std::cout << "AYLIS souffle un peu : +10 pv (" << aylis.pv << "/" << aylis.pvMax << ").\n";

        marche(aylis, armes);
    }

    std::cout << "\n=== VICTOIRE TOTALE ! ===\n";
    std::cout << "Les Haschen sont en deroute : AYLIS a abattu Vorgath le Destructeur !\n";

    return 0;
}
