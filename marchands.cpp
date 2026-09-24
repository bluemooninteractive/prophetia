// marchands.cpp : la halte, Maren, Durgan et Silas
#include <iostream>
#include <cstdlib>
#include "marchands.h"
#include "outils.h"
#include "couleurs.h"

// ===================== La halte et ses marchands =====================

// Un marchand dit une phrase
void parler(const Marchand& marchand, const std::string& texte) {
    std::cout << colorer(marchand.nom, CYAN + GRAS) << " : \"" << texte << "\"\n";
}

// La replique qui correspond au moment de l'histoire (voir MOMENT_... dans marchands.h)
std::string repliqueDuMoment(const std::vector<std::string>& repliques, int moment) {
    int nombre = repliques.size();
    if (moment >= nombre) {
        return repliques[nombre - 1];
    }
    return repliques[moment];
}

// Quelle replique d'accueil choisir : la presentation a la toute premiere halte,
// puis une replique qui suit l'histoire
int indexAccueil(int visite, int moment) {
    if (visite == 0) {
        return 0;               // "Je suis Maren..."
    }
    if (moment == MOMENT_DEBUT) {
        return 1;               // "Te revoila !"
    }
    return moment;
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
void boutiqueHerboriste(Combattant& aylis, const Marchand& maren, int visite, int moment, int& potionsEnStock) {
    const int prixPotion = prixDuJour(15, visite);
    const int prixElixir = prixDuJour(30, visite);

    std::cout << "\n";
    parler(maren, repliqueDuMoment(maren.accueil, indexAccueil(visite, moment)));

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
            parler(maren, repliqueDuMoment(maren.nouvelles, moment));
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
void forge(Combattant& aylis, const Marchand& durgan, int visite, int moment,
           std::vector<Arme>& stock, const std::string& armeEnPromo) {
    const int prixArmure = prixDuJour(35, visite);

    std::cout << "\n";
    parler(durgan, repliqueDuMoment(durgan.accueil, indexAccueil(visite, moment)));

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
            parler(durgan, repliqueDuMoment(durgan.nouvelles, moment));
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
void collectionneur(Combattant& aylis, const Marchand& silas, int visite, int moment) {
    std::cout << "\n";
    parler(silas, repliqueDuMoment(silas.accueil, indexAccueil(visite, moment)));

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
            parler(silas, repliqueDuMoment(silas.nouvelles, moment));
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

// La halte. "visite" = combien de haltes AYLIS a deja faites (les prix montent),
// "moment" = ou en est l'histoire (les marchands n'ont pas les memes repliques).
void halte(Combattant& aylis, const std::vector<Arme>& armes, int visite, int moment) {
    const Marchand maren = {
        "Maren", "l'herboriste",
        {
            "Oh ! Quelqu'un d'encore debout sur cette route ? Je suis Maren. Les Haschen ont brule mon jardin, mais pas mes potions !",
            "Te revoila, AYLIS ! Mes potions t'ont servi ? J'en ai prepare de nouvelles.",
            "Tu as abattu Ashka ?! Toute la vallee ne parle que de ca !",
            "Derniere halte avant la forteresse de Vorgath... Prends tout ce qu'il te faut.",
        },
        {
            "Les chemins d'elite sont dangereux... mais les Haschen qu'on y croise portent toujours un butin rare.",
            "Ashka, la Matriarche, garde le col. Elle lance ses javelots bien avant d'arriver au contact. Garde des potions.",
            "Au-dela du col, les Haschen sont plus nombreux et plus feroces. Ne pars jamais sans de quoi te soigner.",
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
            "Les Haschen de l'autre cote du col portent de meilleures armures. Une arme lourde passe mieux qu'une arme rapide.",
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
            boutiqueHerboriste(aylis, maren, visite, moment, potionsEnStock);
        } else if (choix == 2) {
            forge(aylis, durgan, visite, moment, stock, armeEnPromo);
        } else if (choix == 3) {
            collectionneur(aylis, silas, visite, moment);
        } else if (choix == 4) {
            gererInventaire(aylis);
        } else {
            return;
        }
    }
}
