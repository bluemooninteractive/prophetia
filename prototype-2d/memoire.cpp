// memoire.cpp : ce qu'AYLIS retient d'une course a l'autre
//
// Dans VESPERANCE, tomber n'est pas une fin : c'etait une VISION, un futur possible montre par la prophetie.
// AYLIS se reveille... et s'en souvient. Ces souvenirs sont enregistres dans un petit fichier texte,
// comme la sauvegarde du jeu console (une valeur par ligne, toujours dans le meme ordre).
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include "jeu2d.h"

const std::string fichierMemoire = "vesperance_memoire.txt";
const std::string enteteMemoire = "VESPERANCE-MEMOIRE-3";
const std::string enteteV2 = "VESPERANCE-MEMOIRE-2";       // avant les 4 actes
const std::string enteteAncienne = "VESPERANCE-MEMOIRE-1";    // la version d'avant le Seuil : on sait encore la lire

// ===================== Le fichier =====================

// Toutes les valeurs de la memoire, dans l'ordre du fichier.
// Un pointeur (int*) permet a la meme liste de servir pour lire ET pour ecrire.
std::vector<int*> valeursDeLaMemoire(Memoire& m) {
    std::vector<int*> valeurs = {&m.visions, &m.victoires, &m.meilleureSalle, &m.ashkaAffrontee,
                                 &m.fragments, &m.fragmentsTotal,
                                 // a partir d'ici : la version 2 (le Seuil)
                                 &m.passagesAuSeuil, &m.voyageurAide, &m.voyageurDepouille,
                                 &m.deserteurEpargne, &m.deserteurDepouille};
    for (int i = 0; i < NOMBRE_AMELIORATIONS; i++) {
        valeurs.push_back(&m.ameliorations[i]);
    }
    // a partir d'ici : la version 3 (les 4 actes et leurs boss)
    for (int i = 0; i < 4; i++) {
        valeurs.push_back(&m.bossAffrontes[i]);
        valeurs.push_back(&m.bossVaincus[i]);
    }
    return valeurs;
}

void chargerMemoire(Memoire& memoire) {
    memoire = Memoire();    // on repart de zero si le fichier n'existe pas
    std::ifstream fichier(fichierMemoire);
    std::string ligne;
    if (!fichier || !std::getline(fichier, ligne) || (ligne != enteteMemoire && ligne != enteteV2 && ligne != enteteAncienne)) {
        return;
    }
    // Une vieille memoire s'arrete apres les 6 premieres valeurs : le reste reste a zero
    for (int* valeur : valeursDeLaMemoire(memoire)) {
        if (std::getline(fichier, ligne)) {
            *valeur = std::atoi(ligne.c_str());
        }
    }
}

void enregistrerMemoire(const Memoire& memoire) {
    std::ofstream fichier(fichierMemoire);
    if (!fichier) {
        return;
    }
    Memoire copie = memoire;    // valeursDeLaMemoire a besoin d'une memoire modifiable
    fichier << enteteMemoire << "\n";
    for (int* valeur : valeursDeLaMemoire(copie)) {
        fichier << *valeur << "\n";
    }
}

// ===================== La fin d'une course =====================

void terminerCourse(Jeu& jeu, bool victoire) {
    Memoire& m = jeu.memoire;

    // Les fragments de prophetie : 1 par salle franchie, 2 de plus par elite, 5 par boss vaincu
    int sallesFranchies = victoire ? nombreDeSalles() : jeu.salle - 1;
    int bossVaincus = victoire ? NOMBRE_ACTES : acteDeLaSalle(jeu.salle);
    jeu.fragmentsGagnes = sallesFranchies + 2 * jeu.elitesVaincues + 5 * bossVaincus;
    m.fragments = m.fragments + jeu.fragmentsGagnes;
    m.fragmentsTotal = m.fragmentsTotal + jeu.fragmentsGagnes;

    if (jeu.salle > m.meilleureSalle) {
        m.meilleureSalle = jeu.salle;
    }
    if (victoire) {
        m.victoires = m.victoires + 1;
    } else {
        m.visions = m.visions + 1;
    }
    enregistrerMemoire(m);
}

// ===================== Les mots du reveil =====================
// (AYLIS n'a pas de genre connu : les phrases sont tournees pour ne jamais l'accorder)

std::string texteDuReveil(const Jeu& jeu) {
    const Memoire& m = jeu.memoire;
    std::string texte;

    // 1. Le reveil lui-meme : il change avec le nombre de visions deja vecues
    if (m.visions == 1) {
        texte = "AYLIS ouvre les yeux, le souffle court. La lame, la douleur, la chute... tout semblait si reel. "
                "Pourtant le feu crepite encore, et la route n'a pas commence. La prophetie vient de montrer un futur possible.";
    } else if (m.visions < 4) {
        texte = "Encore une vision. Le coeur d'AYLIS bat trop vite, mais la peur laisse deja place a autre chose : "
                "des souvenirs. Des chemins. Des erreurs a ne plus refaire.";
    } else {
        texte = "La prophetie montre, encore et encore, les chemins qui menent a la chute... "
                "et, peut-etre, le seul qui n'y mene pas. AYLIS se releve sans trembler.";
    }
    texte = texte + " ";

    // 2. Le souvenir le plus fort : qui, ou quoi, a fait tomber AYLIS
    const std::string& qui = jeu.derniereBlessure;
    int bossDeLActe = acte(acteDeLaSalle(jeu.salle)).boss;
    if (jeu.typeSalle == TypeSalle::Boss && bossDeLActe == BOSS_SKARN) {
        texte = texte + (m.bossAffrontes[0] <= 1
            ? "Le sol tremble encore sous la masse de Skarn. Quand il la leve, il faut s'ecarter : le souvenir est net."
            : "Skarn, encore. Trois coups, puis la masse. AYLIS commence a compter avec lui.");
    } else if (jeu.typeSalle == TypeSalle::Boss && bossDeLActe == BOSS_MATRIARCHE) {
        texte = texte + "Le hurlement de la Matriarche resonne encore. Les loups repondent toujours a son appel... "
                        "il faudra frapper avant qu'elle n'appelle.";
    } else if (jeu.typeSalle == TypeSalle::Boss && bossDeLActe == BOSS_VORGATH) {
        texte = texte + (m.bossAffrontes[3] <= 1
            ? "Vorgath. Une montagne d'armure noire, et la lave qui monte sous les pieds. La prophetie n'avait jamais montre cela."
            : "Vorgath, encore. Le sol rougit avant d'exploser : AYLIS le sait, maintenant.");
    } else if (jeu.typeSalle == TypeSalle::Boss || qui == "Ashka") {
        if (m.ashkaAffrontee <= 1) {
            texte = texte + "Le visage d'Ashka reste grave dans sa memoire : la couronne, l'arc, ce regard sans pitie.";
        } else {
            texte = texte + "Ashka, encore. Ses gestes deviennent familiers... la prochaine fois, AYLIS saura les lire.";
        }
    } else if (qui == "poison") {
        texte = texte + "Le gout amer du poison des chamans reste sur ses levres.";
    } else if (qui.find("elite") != std::string::npos) {
        texte = texte + "Le rugissement du " + qui + " resonne encore. Certains combats demandent plus de preparation.";
    } else {
        // Sinon : le souvenir du lieu de la chute
        const char* souvenirs[NOMBRE_LIEUX] = {
            "La brume de la foret s'efface lentement de ses souvenirs.",
            "Les maisons brulees de Brennac fument encore dans sa memoire. Qui vivait la, avant les Haschen ?",
            "Le bruit de la riviere, au gue des Saules, ne veut pas se taire.",
            "Des yeux rouges, entre les arbres morts du bois des Pendus. Ils regardaient. Ils attendaient.",
            "L'odeur de fumee du camp haschen colle encore a sa peau.",
            "Les remparts de Karn se dressent encore dans sa memoire, et les bannieres pourpres d'Ashka.",
            "Le vent glace du col souffle encore dans sa memoire, si pres d'Ashka.",
            "La cendre des terres de Vorgath brule encore les yeux d'AYLIS.",
            "Les murs noirs de la citadelle de Vorgath se referment encore dans ses reves.",
        };
        texte = texte + souvenirs[jeu.lieu];
    }
    return texte;
}

std::string phraseDeDepart(const Memoire& memoire) {
    if (memoire.visions == 0 && memoire.victoires == 0) {
        return "Choisis la voie d'AYLIS";
    }
    if (memoire.victoires > 0) {
        return "Vorgath est tombe dans au moins un futur. La prophetie en montre d'autres...";
    }
    return "AYLIS se reveille. La route attend, encore.";
}
