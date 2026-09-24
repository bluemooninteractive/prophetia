// seuil.cpp : le Seuil, le monde entre les visions
//
// Quand une vision se brise, AYLIS ne se reveille pas tout de suite : la prophetie l'emmene au Seuil,
// une ile qui flotte entre les mondes, sous un ciel d'etoiles. Maren, Durgan et Silas y sont aussi...
// ou plutot leurs echos. Ils se souviennent de ce qu'AYLIS a fait dans les visions, et contre des
// fragments de prophetie, ils offrent des ameliorations qui restent pour toutes les courses suivantes.
#include "jeu2d.h"

// ===================== Les ameliorations =====================

struct Amelioration {
    int marchand;           // 0 = Maren, 1 = Durgan, 2 = Silas
    std::string nom;
    std::string description;
    int rangMax;
    int couts[3];           // le prix de chaque rang, en fragments
};

// Deux ameliorations par personnage : 0 et 1 pour Maren, 2 et 3 pour Durgan, 4 et 5 pour Silas
const Amelioration AMELIORATIONS[NOMBRE_AMELIORATIONS] = {
    {0, "Racines de vie", "+5 pv max au depart", 3, {4, 6, 9}},
    {0, "Fiole d'ambre", "+1 potion au depart", 2, {5, 8, 0}},
    {1, "Lame trempee", "+1 attaque au depart", 3, {5, 7, 10}},
    {1, "Cuir d'ombre", "+1 defense au depart", 2, {6, 9, 0}},
    {2, "Bourse du voyageur", "+20 or au depart", 3, {3, 5, 7}},
    {2, "Rune d'eveil", "Chaque course commence avec une rune", 1, {12, 0, 0}},
};

std::string nomAmelioration(int numero) {
    return AMELIORATIONS[numero].nom;
}

std::string descriptionAmelioration(int numero) {
    return AMELIORATIONS[numero].description;
}

int rangMaxAmelioration(int numero) {
    return AMELIORATIONS[numero].rangMax;
}

// Le prix du prochain rang (0 si le rang maximum est deja atteint)
int coutAmelioration(const Memoire& memoire, int numero) {
    int rang = memoire.ameliorations[numero];
    if (rang >= AMELIORATIONS[numero].rangMax) {
        return 0;
    }
    return AMELIORATIONS[numero].couts[rang];
}

// Au Seuil, les touches 1 et 2 achetent les ameliorations du personnage a qui AYLIS parle
void acheterAmelioration(Jeu& jeu, int choix) {
    if (jeu.interlocuteur < 0 || choix < 0 || choix > 1) {
        return;
    }
    int numero = jeu.interlocuteur * 2 + choix;
    Memoire& m = jeu.memoire;
    int cout = coutAmelioration(m, numero);
    if (cout == 0) {
        jeu.messageSeuil = nomAmelioration(numero) + " : deja au maximum.";
        return;
    }
    if (m.fragments < cout) {
        jeu.messageSeuil = "Pas assez de fragments de prophetie (il en faut " + std::to_string(cout) + ").";
        return;
    }
    m.fragments = m.fragments - cout;
    m.ameliorations[numero] = m.ameliorations[numero] + 1;
    jeu.messageSeuil = nomAmelioration(numero) + " : rang " + std::to_string(m.ameliorations[numero]) + " !";
    enregistrerMemoire(m);      // tout de suite : un achat ne doit jamais se perdre
}

// Au depart d'une course : les ameliorations du Seuil s'appliquent (appelee par choisirVoie)
void appliquerAmeliorations(Jeu& jeu) {
    const int* rang = jeu.memoire.ameliorations;
    Combattant& aylis = jeu.aylis.stats;
    aylis.pvMax = aylis.pvMax + 5 * rang[0];
    aylis.pv = aylis.pvMax;
    aylis.potions = aylis.potions + rang[1];
    aylis.attaque = aylis.attaque + rang[2];
    aylis.defense = aylis.defense + rang[3];
    aylis.pieces = aylis.pieces + 20 * rang[4];
    if (rang[5] > 0) {
        // La rune d'eveil : une rune normale au hasard, des la premiere salle
        std::vector<Rune> possibles;
        for (const Rune& r : catalogueRunes()) {
            if (!r.epique && runeDisponible(jeu, r)) {
                possibles.push_back(r);
            }
        }
        if (!possibles.empty()) {
            Rune rune = possibles[GetRandomValue(0, (int)possibles.size() - 1)];
            appliquerRune(jeu, rune);
            jeu.messageRoute = "La rune d'eveil brille : " + rune.nom + " (" + rune.description + ").";
        }
    }
}

// ===================== Arriver au Seuil, et en repartir =====================

void entrerAuSeuil(Jeu& jeu) {
    jeu.memoire.passagesAuSeuil = jeu.memoire.passagesAuSeuil + 1;
    enregistrerMemoire(jeu.memoire);
    jeu.interlocuteur = -1;
    jeu.messageSeuil = "";
    jeu.phase = Phase::Seuil;
}

void parlerA(Jeu& jeu, int personnage) {
    jeu.interlocuteur = personnage;
    jeu.messageSeuil = "";
}

// ===================== Ce qu'ils disent =====================
// Les echos du Seuil se souviennent des visions : leurs paroles changent avec la memoire d'AYLIS.
// On teste les souvenirs du plus marquant au plus banal : la premiere phrase qui correspond est choisie.
// (Toujours sans genre pour AYLIS : on lui parle a la deuxieme personne, sans accord.)

std::string paroleAuSeuil(const Memoire& m, int personnage) {
    if (personnage == 0) {      // Maren
        if (m.passagesAuSeuil <= 1) {
            return "Toi aussi, la chute te mene ici ? Au Seuil, mes herbes poussent a la lumiere des etoiles. "
                   "Assieds-toi. Rien ne presse, entre deux visions.";
        }
        if (m.bossAffrontes[1] > 0 && m.bossVaincus[1] == 0) {
            return "La Matriarche... elle soigne ses blessures avec mes propres herbes, je les ai reconnues. "
                   "Frappe fort, et vite, avant qu'elle ne boive.";
        }
        if (m.voyageurDepouille > m.voyageurAide) {
            return "On raconte qu'un voyageur blesse a ete depouille, dans la foret des Brumes... "
                   "Les visions gardent tout en memoire, tu sais. Le Seuil aussi.";
        }
        if (m.voyageurAide > 0) {
            return "Un voyageur m'a parle d'une main tendue, sur la route de la foret. "
                   "Il a laisse ces graines pour toi. Elles ont deja germe.";
        }
        return "Chaque vision laisse une graine. Regarde l'arbre : il a encore grandi depuis ta derniere chute.";
    }
    if (personnage == 1) {      // Durgan
        if (m.bossAffrontes[3] > 0 && m.bossVaincus[3] == 0) {
            return "Vorgath... Meme ici, je sens la chaleur de sa forge. Le sol rougit avant d'exploser : "
                   "ne reste jamais sur une case qui brille.";
        }
        if (m.bossAffrontes[0] > 0 && m.bossVaincus[0] == 0) {
            return "Skarn leve sa masse tous les trois tours. Trois ! Compte-les, et recule quand il la leve.";
        }
        if (m.ashkaAffrontee >= 2) {
            return "Ashka recule toujours d'un pas avant de tirer. Frappe a ce moment-la. "
                   "J'ai mis ca dans le metal, en quelque sorte.";
        }
        if (m.ashkaAffrontee == 1) {
            return "Tu as vu Ashka. Ca se lit dans tes yeux. Donne-moi ton arme : la prochaine fois, elle sera prete.";
        }
        if (m.victoires > 0) {
            return "Ashka est tombee, dans un futur au moins. Mais la route continue, plus loin, vers Vorgath. "
                   "Le metal doit etre plus dur.";
        }
        return "Ici, le fer chante tout seul, et l'enclume ne refroidit jamais. Drole d'endroit pour une forge... "
               "je ne me plains pas.";
    }
    // Silas
    if (m.deserteurDepouille > m.deserteurEpargne) {
        return "Tu fais les poches des faibles, maintenant ? Je ne juge pas. Je note. Le Seuil note aussi.";
    }
    if (m.deserteurEpargne > 0) {
        return "Un Haschen epargne, sur la route du camp... La prophetie aime ces choses-la. Moi aussi, pour tout dire.";
    }
    if (m.fragmentsTotal >= 30) {
        return "Tant de fragments... Tu commences a voir la forme de la prophetie, pas vrai ? Moi, je la vois depuis longtemps.";
    }
    return "Le Seuil n'appartient a personne. Ni aux vivants, ni aux visions. Un peu comme moi.";
}

// La phrase d'accueil du Seuil, en haut de l'ecran
std::string murmureDuSeuil(const Memoire& m) {
    if (m.passagesAuSeuil <= 1) {
        return "Entre deux visions, un monde suspendu. Ici, rien ne peut te blesser.";
    }
    if (m.victoires > 0) {
        return "L'arbre du Seuil a fleuri : dans un futur au moins, Ashka est tombee.";
    }
    return "Le Seuil t'attendait. Il t'attend toujours.";
}
