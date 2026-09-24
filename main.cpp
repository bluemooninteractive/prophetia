#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

// Les raretes des objets
const int COMMUN = 0;
const int RARE = 1;
const int EPIQUE = 2;

// Les types d'objets
const int OBJET_MATERIAU = 0;   // ne sert qu'a etre revendu
const int OBJET_ARME = 1;
const int OBJET_POTION = 2;     // va directement dans les potions d'AYLIS

// Les styles de combat des ennemis quand ils sont loin
const int STYLE_MELEE = 0;      // avance d'un pas par tour
const int STYLE_LANCEUR = 1;    // lance des javelots une fois sur deux
const int STYLE_CHARGEUR = 2;   // fonce au contact d'un coup et frappe

// Une arme : de melee (corps a corps) ou a distance
struct Arme {
    std::string nom;
    bool aDistance;         // true = arc, arbalete... / false = epee, hache...
    int bonusAttaque;       // s'ajoute a l'attaque d'AYLIS (peut etre negatif)
    int chanceCritique;     // sur 100 : 10 = 1 chance sur 10
    int nombreDeCoups;      // 2 = l'arme frappe deux fois par attaque
    int prix;               // le marchand la rachete a moitie prix
    int rarete = COMMUN;
};

// Un objet de l'inventaire
struct Objet {
    std::string nom;
    int type;
    int rarete;
    int valeur;             // le prix auquel le marchand l'achete
    Arme arme = {"", false, 0, 0, 1, 0};  // utilise seulement si c'est une arme
};

// Une ligne de la table de loot d'un ennemi
struct Butin {
    Objet objet;
    int chance;             // sur 100 : 30 = 30% de chances de tomber
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
    int orDonne;            // les pieces d'or que l'ennemi laisse tomber (en moyenne)
    std::vector<Butin> butin = {};      // la table de loot de l'ennemi
    int style = STYLE_MELEE;            // comment l'ennemi se comporte quand il est loin
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
    std::vector<Objet> inventaire = {};
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

// Le nom d'une rarete
std::string nomRarete(int rarete) {
    if (rarete == EPIQUE) {
        return "EPIQUE";
    } else if (rarete == RARE) {
        return "RARE";
    }
    return "commun";
}

// Transforme une arme en objet d'inventaire (revendue a moitie prix)
Objet objetDepuisArme(const Arme& arme) {
    return {arme.nom, OBJET_ARME, arme.rarete, arme.prix / 2, arme};
}

// Affiche un objet sur une ligne, par exemple : [RARE] Griffe de berserker (25 or)
void afficherObjet(const Objet& objet) {
    std::cout << "[" << nomRarete(objet.rarete) << "] ";
    if (objet.type == OBJET_ARME) {
        afficherArme(objet.arme);
    } else {
        std::cout << objet.nom;
    }
    std::cout << "  (" << objet.valeur << " or)";
}

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

        if (ligne.objet.type == OBJET_POTION) {
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

// ===================== La halte et ses marchands =====================

// Un marchand : son nom, son metier et ses repliques
struct Marchand {
    std::string nom;
    std::string metier;
    std::vector<std::string> accueil;       // une replique d'accueil par halte
    std::vector<std::string> nouvelles;     // ce qu'il raconte quand on discute, par halte
    std::string merci;                      // quand on lui achete ou vend quelque chose
    std::string pasAssez;                   // quand AYLIS n'a pas assez d'or
    std::string auRevoir;
};

// Un marchand dit une phrase
void parler(const Marchand& marchand, const std::string& texte) {
    std::cout << marchand.nom << " : \"" << texte << "\"\n";
}

// La replique qui correspond a cette halte (la derniere si on depasse la liste)
std::string repliqueDuJour(const std::vector<std::string>& repliques, int visite) {
    int nombre = repliques.size();
    if (visite >= nombre) {
        return repliques[nombre - 1];
    }
    return repliques[visite];
}

// Affiche l'inventaire numerote a partir de 1
void afficherInventaire(const Combattant& aylis) {
    int taille = aylis.inventaire.size();
    for (int i = 0; i < taille; i++) {
        std::cout << (i + 1) << ". ";
        afficherObjet(aylis.inventaire[i]);
        std::cout << "\n";
    }
}

// Les prix montent a chaque halte : +10% par halte deja visitee
int prixDuJour(int prixDeBase, int visite) {
    return prixDeBase * (100 + 10 * visite) / 100;
}

// Le forgeron choisit au hasard quelques armes a vendre aujourd'hui
std::vector<Arme> tirerStock(const std::vector<Arme>& armes, const Arme& armeEnMain, int nombre) {
    // On part de toutes les armes, sauf celle qu'AYLIS a deja en main
    std::vector<Arme> candidates;
    int nombreArmes = armes.size();
    for (int i = 0; i < nombreArmes; i++) {
        if (armes[i].nom != armeEnMain.nom) {
            candidates.push_back(armes[i]);
        }
    }

    // On en tire "nombre" au hasard, sans prendre deux fois la meme
    std::vector<Arme> stock;
    for (int tirage = 0; tirage < nombre && !candidates.empty(); tirage++) {
        int index = std::rand() % candidates.size();
        stock.push_back(candidates[index]);
        candidates.erase(candidates.begin() + index);
    }
    return stock;
}

// ----- Maren, l'herboriste : potions et mana -----
void boutiqueHerboriste(Combattant& aylis, const Marchand& maren, int visite, int& potionsEnStock) {
    const int prixPotion = prixDuJour(15, visite);
    const int prixElixir = prixDuJour(30, visite);

    std::cout << "\n";
    parler(maren, repliqueDuJour(maren.accueil, visite));

    while (true) {
        std::cout << "\n=== " << maren.nom << ", " << maren.metier << " ===   Tu as " << aylis.pieces << " pieces d'or\n";
        std::cout << "1. Potion (+15 pv en combat) ....... " << prixPotion << " or  (tu en as " << aylis.potions
                  << ", il en reste " << potionsEnStock << ")\n";
        std::cout << "2. Elixir de mana (+5 mana max) .... " << prixElixir << " or\n";
        std::cout << "3. Discuter\n";
        std::cout << "4. Partir\n";

        int choix = lireChoix(1, 4);
        if (choix == 4) {
            parler(maren, maren.auRevoir);
            return;
        }
        if (choix == 3) {
            parler(maren, repliqueDuJour(maren.nouvelles, visite));
            continue;
        }
        if (choix == 1 && potionsEnStock == 0) {
            parler(maren, "Desolee, je n'ai plus une seule potion. Reviens a la prochaine halte !");
            continue;
        }

        int prix = prixPotion;
        if (choix == 2) {
            prix = prixElixir;
        }
        if (aylis.pieces < prix) {
            parler(maren, maren.pasAssez);
            std::cout << "(Il te manque " << (prix - aylis.pieces) << " pieces.)\n";
            continue;
        }
        aylis.pieces = aylis.pieces - prix;

        if (choix == 1) {
            aylis.potions = aylis.potions + 1;
            potionsEnStock = potionsEnStock - 1;
            std::cout << "Achete ! " << aylis.potions << " potions.\n";
        } else {
            aylis.manaMax = aylis.manaMax + 5;
            std::cout << "Achete ! Mana max : " << aylis.manaMax << ".\n";
        }
        parler(maren, maren.merci);
    }
}

// ----- Durgan, le forgeron : armes et armure -----
void forge(Combattant& aylis, const Marchand& durgan, int visite,
           std::vector<Arme>& stock, const std::string& armeEnPromo) {
    const int prixArmure = prixDuJour(35, visite);

    std::cout << "\n";
    parler(durgan, repliqueDuJour(durgan.accueil, visite));

    while (true) {
        std::cout << "\n=== " << durgan.nom << ", " << durgan.metier << " ===   Tu as " << aylis.pieces << " pieces d'or\n";
        std::cout << "Arme en main : ";
        afficherArme(aylis.arme);
        std::cout << "\n\n";

        // Les armes du jour, puis l'armure, puis discuter et partir
        int nombreArmes = stock.size();
        for (int i = 0; i < nombreArmes; i++) {
            std::cout << (i + 1) << ". ";
            afficherArme(stock[i]);
            std::cout << "  ...  " << stock[i].prix << " or";
            if (stock[i].nom == armeEnPromo) {
                std::cout << "  ** PROMO -25% **";
            }
            std::cout << "\n";
        }
        int choixArmure = nombreArmes + 1;
        int choixDiscuter = nombreArmes + 2;
        int choixPartir = nombreArmes + 3;
        std::cout << choixArmure << ". Armure renforcee (+1 defense) ..... " << prixArmure << " or\n";
        std::cout << choixDiscuter << ". Discuter\n";
        std::cout << choixPartir << ". Partir\n";

        int choix = lireChoix(1, choixPartir);
        if (choix == choixPartir) {
            parler(durgan, durgan.auRevoir);
            return;
        }
        if (choix == choixDiscuter) {
            parler(durgan, repliqueDuJour(durgan.nouvelles, visite));
            continue;
        }

        if (choix == choixArmure) {
            if (aylis.pieces < prixArmure) {
                parler(durgan, durgan.pasAssez);
                continue;
            }
            aylis.pieces = aylis.pieces - prixArmure;
            aylis.defense = aylis.defense + 1;
            std::cout << "Achete ! Defense : " << aylis.defense << ".\n";
            parler(durgan, durgan.merci);
            continue;
        }

        // Sinon, c'est une arme du stock
        const Arme arme = stock[choix - 1];     // une copie : on va l'enlever du stock
        if (aylis.pieces < arme.prix) {
            parler(durgan, durgan.pasAssez);
            std::cout << "(Il te manque " << (arme.prix - aylis.pieces) << " pieces.)\n";
            continue;
        }
        aylis.pieces = aylis.pieces - arme.prix;
        stock.erase(stock.begin() + (choix - 1));   // le forgeron n'en avait qu'une

        aylis.inventaire.push_back(objetDepuisArme(aylis.arme));
        std::cout << aylis.arme.nom << " va dans le sac.\n";
        aylis.arme = arme;
        std::cout << "AYLIS s'equipe : " << arme.nom << " !\n";
        parler(durgan, durgan.merci);
    }
}

// Silas paie 50% de plus pour les objets RARE et EPIQUE
int prixDeRachat(const Objet& objet) {
    if (objet.rarete == COMMUN) {
        return objet.valeur;
    }
    return objet.valeur * 150 / 100;
}

// ----- Silas, le collectionneur : rachete le butin -----
void collectionneur(Combattant& aylis, const Marchand& silas, int visite) {
    std::cout << "\n";
    parler(silas, repliqueDuJour(silas.accueil, visite));

    while (true) {
        std::cout << "\n=== " << silas.nom << ", " << silas.metier << " ===   Tu as " << aylis.pieces << " pieces d'or\n";
        std::cout << "(Silas paie 50% de plus pour les objets RARE et EPIQUE)\n";

        int taille = aylis.inventaire.size();
        for (int i = 0; i < taille; i++) {
            std::cout << (i + 1) << ". Vendre ";
            afficherObjet(aylis.inventaire[i]);
            std::cout << "  -> Silas en donne " << prixDeRachat(aylis.inventaire[i]) << " or\n";
        }
        int choixTout = taille + 1;
        int choixDiscuter = taille + 2;
        std::cout << choixTout << ". Vendre tous les materiaux d'un coup\n";
        std::cout << choixDiscuter << ". Discuter\n";
        std::cout << "0. Partir\n";

        int choix = lireChoix(0, choixDiscuter);
        if (choix == 0) {
            parler(silas, silas.auRevoir);
            return;
        }
        if (choix == choixDiscuter) {
            parler(silas, repliqueDuJour(silas.nouvelles, visite));
            continue;
        }

        if (choix == choixTout) {
            // On parcourt a l'envers : effacer un objet ne decale pas ceux qu'il reste a voir
            int gain = 0;
            bool epiqueVendu = false;
            for (int i = taille - 1; i >= 0; i--) {
                if (aylis.inventaire[i].type == OBJET_MATERIAU) {
                    gain = gain + prixDeRachat(aylis.inventaire[i]);
                    if (aylis.inventaire[i].rarete == EPIQUE) {
                        epiqueVendu = true;
                    }
                    aylis.inventaire.erase(aylis.inventaire.begin() + i);
                }
            }
            if (gain == 0) {
                parler(silas, "Tu n'as aucun materiau a me vendre...");
                continue;
            }
            aylis.pieces = aylis.pieces + gain;
            std::cout << "Materiaux vendus : +" << gain << " or.\n";
            if (epiqueVendu) {
                parler(silas, "Par tous les dieux... Une piece pareille ! Elle trouvera une place d'honneur dans ma collection.");
            } else {
                parler(silas, silas.merci);
            }
            continue;
        }

        const Objet objet = aylis.inventaire[choix - 1];   // une copie : on va l'effacer
        aylis.inventaire.erase(aylis.inventaire.begin() + (choix - 1));
        int gain = prixDeRachat(objet);
        aylis.pieces = aylis.pieces + gain;
        std::cout << objet.nom << " vendu : +" << gain << " or.\n";

        if (objet.rarete == EPIQUE) {
            parler(silas, "Par tous les dieux... Une piece pareille ! Elle trouvera une place d'honneur dans ma collection.");
        } else {
            parler(silas, silas.merci);
        }
    }
}

// Voir l'inventaire et changer d'arme
void gererInventaire(Combattant& aylis) {
    while (true) {
        std::cout << "\n=== INVENTAIRE ===\n";
        std::cout << "Arme en main : ";
        afficherArme(aylis.arme);
        std::cout << "\n";

        int taille = aylis.inventaire.size();
        if (taille == 0) {
            std::cout << "(sac vide)\n";
            return;
        }
        afficherInventaire(aylis);
        std::cout << "Tape le numero d'une arme pour l'equiper, ou 0 pour revenir.\n";

        int choix = lireChoix(0, taille);
        if (choix == 0) {
            return;
        }

        Objet& objet = aylis.inventaire[choix - 1];
        if (objet.type != OBJET_ARME) {
            std::cout << objet.nom << " n'est pas une arme.\n";
            continue;
        }

        // On echange : l'arme en main va dans le sac, l'arme du sac va en main
        Arme ancienne = aylis.arme;
        aylis.arme = objet.arme;
        objet = objetDepuisArme(ancienne);
        std::cout << "AYLIS s'equipe : " << aylis.arme.nom << " !\n";
    }
}

// La halte entre deux combats. "visite" = combien de haltes AYLIS a deja faites (0 a la premiere).
void halte(Combattant& aylis, const std::vector<Arme>& armes, int visite) {
    const Marchand maren = {
        "Maren", "l'herboriste",
        {
            "Oh ! Quelqu'un d'encore debout sur cette route ? Je suis Maren. Les Haschen ont brule mon jardin, mais pas mes potions !",
            "Te revoila, AYLIS ! Mes potions t'ont servi ? J'en ai prepare de nouvelles.",
            "Tu as abattu Ashka ?! Toute la vallee ne parle que de ca !",
            "Derniere halte avant la forteresse de Vorgath... Prends tout ce qu'il te faut.",
        },
        {
            "Un guerrier Haschen rode plus loin. Plus solide qu'un eclaireur, mais rien d'impossible.",
            "Ashka, la Matriarche, garde le col. Elle lance ses javelots bien avant d'arriver au contact. Garde des potions.",
            "Un berserker barre le pont. Il frappe tres fort : ne pars pas sans de quoi te soigner.",
            "Vorgath ne marche pas vers ses ennemis : il CHARGE. Tu n'auras pas le temps de tirer de loin.",
        },
        "Bon choix ! Ca soigne meme les blessures de Haschen.",
        "Je voudrais bien te faire credit, mais les Haschen ont vide ma caisse...",
        "Prends soin de toi, AYLIS.",
    };

    const Marchand durgan = {
        "Durgan", "le forgeron",
        {
            "Hmpf. Durgan, forgeron. Ta lame est emoussee. Regarde plutot ce que j'ai forge.",
            "Encore toi. Bien. Les clients vivants sont les meilleurs clients.",
            "La Matriarche est tombee ? ... Je retire ce que j'ai dit sur ta lame.",
            "Pour Vorgath, il te faut du solide. Je t'ai garde mes meilleures pieces.",
        },
        {
            "Les dagues et les couteaux frappent deux fois. Parfait contre les Haschen sans armure.",
            "Contre quelqu'un qui tire de loin, une arme a distance te permet de repondre.",
            "Le berserker a une bonne defense. Une arme lourde passe mieux qu'une arme rapide.",
            "Vorgath porte une armure epaisse. Les petits coups vont rebondir dessus. Frappe fort.",
        },
        "Du bon travail. Tu verras.",
        "Pas d'or, pas d'acier. C'est la regle.",
        "Ne casse pas mon travail.",
    };

    const Marchand silas = {
        "Silas", "le collectionneur",
        {
            "Silas, collectionneur. Crocs, peaux, trophees... je rachete tout ce que les Haschen laissent derriere eux.",
            "Ah, AYLIS ! Qu'est-ce que tu m'as ramene cette fois ?",
            "Tu... tu as quelque chose d'Ashka ? Montre-moi. MONTRE-MOI !",
            "Si tu rapportes un souvenir de Vorgath, je te rends riche. Enfin... si tu reviens.",
        },
        {
            "Les Haschen etaient un peuple calme, autrefois. C'est Vorgath qui les a rassembles et rendus fous de rage.",
            "Ashka etait la mere de tout un clan. On dit que sa couronne est taillee dans les os de ses ancetres.",
            "Sans Ashka, beaucoup de Haschen fuient vers les montagnes. Vorgath est seul, maintenant. Et furieux.",
            "Personne n'est jamais ressorti de la forteresse de Vorgath. Tu serais la premiere personne a le faire.",
        },
        "Marche conclu ! Toujours un plaisir.",
        "Tu n'as rien qui m'interesse... pour l'instant.",
        "Reviens avec de belles pieces !",
    };

    // Le stock du jour : 1 a 3 potions chez Maren, 3 armes au hasard chez Durgan
    int potionsEnStock = 1 + std::rand() % 3;
    std::vector<Arme> stock = tirerStock(armes, aylis.arme, 3);

    // Les armes suivent la hausse des prix, et l'une d'elles est en promo
    int nombreArmes = stock.size();
    for (int i = 0; i < nombreArmes; i++) {
        stock[i].prix = prixDuJour(stock[i].prix, visite);
    }
    std::string armeEnPromo = "";
    if (nombreArmes > 0) {
        int index = std::rand() % nombreArmes;
        stock[index].prix = stock[index].prix * 75 / 100;
        armeEnPromo = stock[index].nom;
    }

    std::cout << "\n~~~ Une halte au bord de la route. Trois marchands ont installe leurs etals. ~~~\n";
    if (visite > 0) {
        std::cout << "(La guerre contre les Haschen fait monter les prix : +" << (10 * visite) << "%)\n";
    }

    while (true) {
        std::cout << "\n=== LA HALTE ===   Tu as " << aylis.pieces << " pieces d'or\n";
        std::cout << "1. " << maren.nom << ", " << maren.metier << "        (potions, mana)\n";
        std::cout << "2. " << durgan.nom << ", " << durgan.metier << "        (armes, armure)\n";
        std::cout << "3. " << silas.nom << ", " << silas.metier << "   (rachete ton butin, "
                  << aylis.inventaire.size() << " objets dans le sac)\n";
        std::cout << "4. Inventaire (changer d'arme)\n";
        std::cout << "5. Reprendre la route\n";

        int choix = lireChoix(1, 5);
        if (choix == 1) {
            boutiqueHerboriste(aylis, maren, visite, potionsEnStock);
        } else if (choix == 2) {
            forge(aylis, durgan, visite, stock, armeEnPromo);
        } else if (choix == 3) {
            collectionneur(aylis, silas, visite);
        } else if (choix == 4) {
            gererInventaire(aylis);
        } else {
            return;
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

    // L'ennemi est encore loin : ce qu'il fait depend de son style
    if (distance > 0) {
        // Un lanceur tire une fois sur deux au lieu d'avancer
        if (ennemi.style == STYLE_LANCEUR && std::rand() % 2 == 0) {
            std::cout << ennemi.nom << " lance un javelot ! ";
            int degats = calculerDegats(ennemi.attaque, 80, aylis.defense, 10);
            toucherAylis(ennemi, aylis, degats, aylisEnGarde, rage);
            return;
        }

        // Un chargeur fonce directement au contact et frappe dans l'elan
        if (ennemi.style == STYLE_CHARGEUR) {
            distance = 0;
            std::cout << ennemi.nom << " CHARGE et arrive au contact ! ";
            int degats = calculerDegats(ennemi.attaque, 70, aylis.defense, 10);
            toucherAylis(ennemi, aylis, degats, aylisEnGarde, rage);
            return;
        }

        // Les autres avancent d'un pas
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

    toucherAylis(ennemi, aylis, degats, aylisEnGarde, rage);
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

    // Les objets qu'on peut trouver sur les ennemis
    //                 nom                      type            rarete  valeur
    Objet croc     = {"Croc de Haschen",        OBJET_MATERIAU, COMMUN,  8};
    Objet peau     = {"Peau de Haschen",        OBJET_MATERIAU, COMMUN, 12};
    Objet griffe   = {"Griffe de berserker",    OBJET_MATERIAU, RARE,   25};
    Objet couronne = {"Couronne d'os d'Ashka",  OBJET_MATERIAU, EPIQUE, 70};
    Objet potion   = {"Potion",                 OBJET_POTION,   COMMUN,  7};

    // Les armes qu'on peut trouver sur les ennemis
    //                                  nom                   distance bonus crit coups prix rarete
    Objet arcDOs         = objetDepuisArme({"Arc d'os",           true,   3, 20, 1,  60, RARE});
    Objet lance          = objetDepuisArme({"Lance de Haschen",   false,  3, 15, 1,  60, RARE});
    Objet hacheBerserker = objetDepuisArme({"Hache du berserker", false,  6, 15, 1,  80, RARE});
    Objet javelots       = objetDepuisArme({"Javelots d'Ashka",   true,   6, 20, 1, 120, EPIQUE});

    // La liste des ennemis, dans l'ordre.
    // Apres l'XP et l'or : la table de loot (chaque objet avec sa chance sur 100), puis le style.
    std::vector<Combattant> ennemis = {
        {"Haschen eclaireur",              18, 18,  8, 1, 0, false,  20, 20,
            {{croc, 70}, {peau, 30}, {potion, 20}, {arcDOs, 10}}},
        {"Haschen guerrier",               22, 22,  9, 2, 0, false,  25, 25,
            {{croc, 60}, {peau, 50}, {potion, 20}, {lance, 12}}},
        {"Ashka, Matriarche des Haschen",  40, 40, 14, 4, 1, true,   50, 50,
            {{couronne, 100}, {javelots, 100}, {potion, 50}}, STYLE_LANCEUR},
        {"Haschen berserker",              30, 30, 15, 5, 0, false,  30, 30,
            {{croc, 50}, {griffe, 60}, {hacheBerserker, 15}}},
        {"Vorgath le Destructeur",         60, 60, 17, 6, 2, true,  100,  0,
            {}, STYLE_CHARGEUR},
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

    // Le choix de la difficulte : change la force des ennemis et l'or qu'ils donnent
    std::cout << "\nChoisis la difficulte :\n";
    std::cout << "1. Facile     (ennemis -20% pv et attaque, +20% d'or)\n";
    std::cout << "2. Normal\n";
    std::cout << "3. Difficile  (ennemis +25% pv et attaque, -20% d'or)\n";
    int difficulte = lireChoix(1, 3);

    int forceEnnemis = 100;     // en pourcentage
    int orEnnemis = 100;
    if (difficulte == 1) {
        forceEnnemis = 80;
        orEnnemis = 120;
    } else if (difficulte == 3) {
        forceEnnemis = 125;
        orEnnemis = 80;
    }

    // On applique la difficulte a chaque ennemi. Le & modifie le vrai ennemi de la liste.
    for (Combattant& ennemi : ennemis) {
        ennemi.pvMax = ennemi.pvMax * forceEnnemis / 100;
        ennemi.pv = ennemi.pvMax;
        ennemi.attaque = ennemi.attaque * forceEnnemis / 100;
        ennemi.orDonne = ennemi.orDonne * orEnnemis / 100;
    }

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
        ramasserButin(aylis, ennemi);
        gagnerXp(aylis, ennemi.xpDonne);
        depenserPoints(aylis);

        soigner(aylis, 10);
        std::cout << "AYLIS souffle un peu : +10 pv (" << aylis.pv << "/" << aylis.pvMax << ").\n";

        halte(aylis, armes, i);
    }

    std::cout << "\n=== VICTOIRE TOTALE ! ===\n";
    std::cout << "Les Haschen sont en deroute : AYLIS a abattu Vorgath le Destructeur !\n";

    return 0;
}
