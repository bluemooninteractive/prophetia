// compagnons.cpp : ceux qui se battent aux cotes d'AYLIS
//
//   Kerrak : le deserteur haschen. AYLIS l'a epargne, il a choisi de le suivre. Rapide, il frappe souvent.
//   Brenna : une mercenaire, engagee chez Durgan pour 40 pieces. Plus lente, mais solide et puissante.
//
// Un compagnon joue tout seul, juste apres AYLIS et avant les Haschen : il marche vers le Haschen le plus
// proche et le frappe. Les Haschen peuvent le prendre pour cible. S'il tombe, il est "K.O." jusqu'a la fin
// du combat, puis revient au combat suivant avec la moitie de ses pv.
#include "jeu2d.h"
#include "animations.h"

void recruterCompagnon(Jeu& jeu, int compagnon) {
    //                   nom       pv  pvMax att def potions boss   xp or
    Combattant stats = {"Kerrak",  30, 30,   9,  2,  0,      false, 0, 0};
    Color couleur = Color{255, 176, 154, 255};
    if (compagnon == COMPAGNON_BRENNA) {
        stats = {"Brenna", 40, 40, 12, 3, 0, false, 0, 0};
        couleur = Color{240, 190, 120, 255};
    }
    // Plus la route est avancee, plus le compagnon est aguerri
    stats.pvMax = stats.pvMax + (jeu.salle - 1);
    stats.pv = stats.pvMax;
    stats.attaque = stats.attaque + (jeu.salle - 1) / 4;
    jeu.compagnon = compagnon;
    jeu.allie = Pion();
    jeu.allie.stats = stats;
    jeu.allie.couleur = couleur;
    jeu.allie.colonne = -10;        // hors de l'arene tant que le combat n'a pas commence
    jeu.allie.ligne = -10;
}

bool allieDebout(const Jeu& jeu) {
    return jeu.compagnon != COMPAGNON_AUCUN && jeu.allie.stats.estDebout() && estDansArene(jeu.allie.colonne, jeu.allie.ligne);
}

// Au debut d'un combat : sur une case libre, a cote d'AYLIS
void placerCompagnon(Jeu& jeu) {
    if (jeu.compagnon == COMPAGNON_AUCUN) {
        return;
    }
    Pion& allie = jeu.allie;
    allie.colonne = -10;
    allie.ligne = -10;
    // Tombe au combat precedent ? Il revient avec la moitie de ses pv
    if (!allie.stats.estDebout()) {
        allie.stats.pv = allie.stats.pvMax / 2;
    }
    allie.stats.poison = 0;
    allie.stats.brulure = 0;
    allie.stats.saignement = 0;
    allie.stats.etourdi = false;
    allie.xAffiche = -1.0f;
    allie.pvAffiches = -1.0f;
    allie.flash = 0.0f;
    allie.elan = 0.0f;
    allie.recul = 0.0f;
    allie.disparition = 0.0f;
    const int voisins[8][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {1, -1}, {0, 2}, {0, -2}};
    for (const auto& v : voisins) {
        int c = jeu.aylis.colonne + v[0];
        int l = jeu.aylis.ligne + v[1];
        if (caseLibre(jeu, c, l)) {
            allie.colonne = c;
            allie.ligne = l;
            return;
        }
    }
}

// Le Haschen debout le plus proche du compagnon (-1 s'il n'y en a plus)
int haschenLePlusProche(const Jeu& jeu, const Pion& pion) {
    int meilleur = -1;
    int meilleureDistance = 999;
    for (int i = 0; i < (int)jeu.haschen.size(); i++) {
        const Pion& h = jeu.haschen[i];
        if (h.stats.estDebout() && distanceEntre(pion, h) < meilleureDistance) {
            meilleureDistance = distanceEntre(pion, h);
            meilleur = i;
        }
    }
    return meilleur;
}

void jouerCompagnon(Jeu& jeu) {
    if (!allieDebout(jeu)) {
        return;
    }
    Pion& allie = jeu.allie;
    int cible = haschenLePlusProche(jeu, allie);
    if (cible == -1) {
        return;
    }
    // Il avance vers sa cible (3 cases au plus), sans jamais la traverser
    for (int pas = 0; pas < 3 && distanceEntre(allie, jeu.haschen[cible]) > 1; pas++) {
        if (!unPasVers(jeu, allie, jeu.haschen[cible].colonne, jeu.haschen[cible].ligne)) {
            break;
        }
    }
    Pion& h = jeu.haschen[cible];
    if (distanceEntre(allie, h) != 1) {
        return;
    }
    bool critique = false;
    int degats = calculerDegats2D(allie.stats.attaque, 100, h.stats.defense, 10, critique);
    float delai = animerAttaque(jeu, allie, h, false, SorteProjectile::Fleche);
    ecrireJournal(jeu, allie.stats.nom + " frappe " + h.stats.nom + " : -" + std::to_string(degats) + " pv");
    blesserHaschen(jeu, h, degats, critique, delai);
}

// Un Haschen touche le compagnon
void toucherAllie(Jeu& jeu, const Pion& attaquant, int degats, bool critique, float delai) {
    Pion& allie = jeu.allie;
    allie.stats.pv = allie.stats.pv - degats;
    allie.flash = 0.25f + delai;
    ajouterTexte(jeu, allie, (critique ? "CRIT -" : "-") + std::to_string(degats), RED, delai);
    animerImpact(jeu, -2, delai, critique, RED);
    ecrireJournal(jeu, attaquant.stats.nom + " touche " + allie.stats.nom + " : -" + std::to_string(degats) + " pv");
    if (!allie.stats.estDebout()) {
        ecrireJournal(jeu, allie.stats.nom + " est K.O. ! Il reviendra au prochain combat.");
        if (jeu.compagnon == COMPAGNON_BRENNA) {
            jeu.journal.back() = "Brenna est K.O. ! Elle reviendra au prochain combat.";
        }
        animerChute(jeu, allie, delai);
    }
}
