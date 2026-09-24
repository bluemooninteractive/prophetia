// memoire.cpp : ce qu'AYLIS retient d'une course a l'autre
//
// Dans VESPERANCE, tomber n'est pas une fin : c'etait une VISION, un futur possible montre par la prophetie.
// AYLIS se reveille... et s'en souvient. Ces souvenirs sont enregistres dans un petit fichier texte,
// comme la sauvegarde du jeu console (une valeur par ligne, toujours dans le meme ordre).
#include <fstream>
#include <string>
#include <cstdlib>
#include "jeu2d.h"

const std::string fichierMemoire = "vesperance_memoire.txt";
const std::string enteteMemoire = "VESPERANCE-MEMOIRE-1";

// ===================== Le fichier =====================

void chargerMemoire(Memoire& memoire) {
    memoire = Memoire();    // on repart de zero si le fichier n'existe pas
    std::ifstream fichier(fichierMemoire);
    std::string ligne;
    if (!fichier || !std::getline(fichier, ligne) || ligne != enteteMemoire) {
        return;
    }
    int* valeurs[] = {&memoire.visions, &memoire.victoires, &memoire.meilleureSalle,
                      &memoire.ashkaAffrontee, &memoire.fragments, &memoire.fragmentsTotal};
    for (int* valeur : valeurs) {
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
    fichier << enteteMemoire << "\n";
    fichier << memoire.visions << "\n" << memoire.victoires << "\n" << memoire.meilleureSalle << "\n";
    fichier << memoire.ashkaAffrontee << "\n" << memoire.fragments << "\n" << memoire.fragmentsTotal << "\n";
}

// ===================== La fin d'une course =====================

void terminerCourse(Jeu& jeu, bool victoire) {
    Memoire& m = jeu.memoire;

    // Les fragments de prophetie : 1 par salle franchie, 2 de plus par elite, 5 pour Ashka
    int sallesFranchies = victoire ? NOMBRE_SALLES : jeu.salle - 1;
    jeu.fragmentsGagnes = sallesFranchies + 2 * jeu.elitesVaincues + (victoire ? 5 : 0);
    m.fragments = m.fragments + jeu.fragmentsGagnes;
    m.fragmentsTotal = m.fragmentsTotal + jeu.fragmentsGagnes;

    if (jeu.salle > m.meilleureSalle) {
        m.meilleureSalle = jeu.salle;
    }
    if (jeu.typeSalle == TypeSalle::Boss) {
        m.ashkaAffrontee = m.ashkaAffrontee + 1;
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
    if (jeu.typeSalle == TypeSalle::Boss || qui == "Ashka") {
        if (m.ashkaAffrontee <= 1) {
            texte = texte + "Le visage d'Ashka reste grave dans sa memoire : la couronne, l'arc, ce regard sans pitie.";
        } else {
            texte = texte + "Ashka, encore. Ses gestes deviennent familiers... la prochaine fois, AYLIS saura les lire.";
        }
    } else if (qui == "poison") {
        texte = texte + "Le gout amer du poison des chamans reste sur ses levres.";
    } else if (qui.find("elite") != std::string::npos) {
        texte = texte + "Le rugissement du " + qui + " resonne encore. Certains combats demandent plus de preparation.";
    } else if (jeu.lieu == 0) {
        texte = texte + "La brume de la foret s'efface lentement de ses souvenirs.";
    } else if (jeu.lieu == 1) {
        texte = texte + "L'odeur de fumee du camp haschen colle encore a sa peau.";
    } else {
        texte = texte + "Le vent glace du col souffle encore dans sa memoire, si pres d'Ashka.";
    }
    return texte;
}

std::string phraseDeDepart(const Memoire& memoire) {
    if (memoire.visions == 0 && memoire.victoires == 0) {
        return "Choisis la voie d'AYLIS";
    }
    if (memoire.victoires > 0) {
        return "Ashka est tombee dans au moins un futur. La prophetie en montre d'autres...";
    }
    return "AYLIS se reveille. La route attend, encore.";
}
