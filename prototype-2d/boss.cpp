// boss.cpp : les quatre gardiens de la route, un a la fin de chaque acte
//
//   Acte I   Skarn le Brise-Cranes : une brute. Tous les 3 tours, il leve sa masse : les cases autour de lui
//            deviennent rouges... et au tour suivant, elles volent en eclats. Il faut s'ecarter !
//   Acte II  La Matriarche : une chamane. Son malefice empoisonne de loin, elle appelle des louvetiers
//            et se soigne quand elle faiblit.
//   Acte III Ashka : la cheffe de guerre. Une pluie de fleches annoncee, et ses archers quand elle est blessee.
//   Acte IV  Vorgath le Destructeur : le sol entre en eruption la ou AYLIS se tient... et a moitie de ses pv,
//            il se dechaine (deuxieme phase).
//
// Le principe des attaques ANNONCEES : le boss marque des cases (jeu.zonesDanger). Elles restent rouges
// pendant le tour d'AYLIS, qui peut s'en ecarter. Au tour suivant du boss, elles explosent.
#include "jeu2d.h"
#include "animations.h"

const int HASCHEN_MAX = 10;     // jamais plus de Haschen dans l'arene (les invocations s'arretent la)

// ===================== Les boss et leur escorte =====================

Combattant statsDuBoss(int boss) {
    //                      nom                        pv   pvMax att def potions boss  xp  or
    Combattant b = {"Skarn le Brise-Cranes",           80,  80,   13, 3,  1,      true, 60, 80};
    b.style = Style::Chargeur;
    if (boss == BOSS_MATRIARCHE) {
        b = {"La Matriarche",                          95,  95,   14, 3,  2,      true, 70, 100};
        b.style = Style::Lanceur;
        b.attaquePoison = true;
    } else if (boss == BOSS_ASHKA) {
        b = {"Ashka",                                  120, 120,  17, 5,  1,      true, 80, 120};
        b.style = Style::Lanceur;
    } else if (boss == BOSS_VORGATH) {
        b = {"Vorgath le Destructeur",                 180, 180,  19, 6,  0,      true, 100, 0};
        b.style = Style::Chargeur;
    }
    return b;
}

Color couleurDuBoss(int boss) {
    switch (boss) {
        case BOSS_SKARN: return Color{120, 120, 140, 255};
        case BOSS_MATRIARCHE: return Color{80, 160, 90, 255};
        case BOSS_ASHKA: return GOLD;
        default: return Color{220, 60, 40, 255};
    }
}

void placerBossEtEscorte(Jeu& jeu, const std::vector<bool>& accessibles, int boss,
                         const std::vector<std::pair<Combattant, Color>>& groupe) {
    placerHaschen(jeu, accessibles, statsDuBoss(boss), couleurDuBoss(boss));
    if (!jeu.haschen.empty()) {
        jeu.haschen.back().boss = boss;
    }
    // L'escorte : deux Haschen du lieu (un seul pour Vorgath, qui prefere se battre seul...
    // et aucun si Ashka a ete epargnee : ses guerriers ont quitte Vorgath)
    int escorte = boss == BOSS_VORGATH ? (jeu.choixAshka == 1 ? 0 : 1) : 2;
    for (int i = 0; i < escorte; i++) {
        const auto& h = groupe[i % groupe.size()];
        placerHaschen(jeu, accessibles, h.first, h.second);
    }
    jeu.memoire.bossAffrontes[boss - 1] = jeu.memoire.bossAffrontes[boss - 1] + 1;
    if (boss == BOSS_ASHKA) {
        jeu.memoire.ashkaAffrontee = jeu.memoire.ashkaAffrontee + 1;
    }
}

// ===================== Les outils des boss =====================

int haschenDebout(const Jeu& jeu) {
    int nombre = 0;
    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout()) {
            nombre = nombre + 1;
        }
    }
    return nombre;
}

// Fait apparaitre un Haschen en renfort, sur une case libre pres du boss
void invoquer(Jeu& jeu, const Pion& boss, Combattant renfort, Color couleur) {
    if ((int)jeu.haschen.size() >= HASCHEN_MAX) {
        return;
    }
    for (int rayon = 1; rayon <= 4; rayon++) {
        for (int essai = 0; essai < 20; essai++) {
            int c = boss.colonne + GetRandomValue(-rayon, rayon);
            int l = boss.ligne + GetRandomValue(-rayon, rayon);
            if (caseLibre(jeu, c, l)) {
                jeu.haschen.push_back({renfort, c, l, couleur});
                animerEtat(jeu, jeu.haschen.back(), couleur);
                return;
            }
        }
    }
}

// Un Haschen de renfort, a la force de la salle en cours
Combattant renfort(const Jeu& jeu, const std::string& sorte) {
    int bonus = jeu.salle - 1;
    Combattant h = {"Haschen louvetier", 22, 22, 10, 2, 0, false, 25, 25};
    h.style = Style::Chargeur;
    if (sorte == "traqueur") {
        h = {"Haschen traqueur", 20, 20, 9, 1, 0, false, 25, 22};
        h.style = Style::Lanceur;
    } else if (sorte == "brute") {
        h = {"Haschen brute", 34, 34, 12, 4, 0, false, 30, 35};
        h.style = Style::Chargeur;
    }
    h.pvMax = h.pvMax + bonus * 2;
    h.pv = h.pvMax;
    h.attaque = h.attaque + bonus / 3;
    return h;
}

// Marque des cases qui exploseront au prochain tour du boss
void annoncerDanger(Jeu& jeu, const std::vector<std::pair<int, int>>& cases, int degats) {
    jeu.zonesDanger.clear();
    for (const auto& c : cases) {
        if (estDansArene(c.first, c.second) && !estRocher(jeu, c.first, c.second)) {
            jeu.zonesDanger.push_back(c);
        }
    }
    jeu.degatsDanger = degats;
}

// Les cases marquees explosent : AYLIS prend les degats si sa case est encore marquee
void resoudreDanger(Jeu& jeu, Pion& boss) {
    if (jeu.zonesDanger.empty()) {
        return;
    }
    bool touche = false;
    bool allieTouche = false;
    for (const auto& c : jeu.zonesDanger) {
        Pion caseVide = jeu.aylis;      // une copie, juste pour avoir un centre de case a l'endroit voulu
        caseVide.colonne = c.first;
        caseVide.ligne = c.second;
        animerExplosion(jeu, caseVide, 0.0f);
        if (c.first == jeu.aylis.colonne && c.second == jeu.aylis.ligne) {
            touche = true;
        }
        if (allieDebout(jeu) && c.first == jeu.allie.colonne && c.second == jeu.allie.ligne) {
            allieTouche = true;     // le compagnon ne sait pas lire les presages : il reste parfois dans la zone
        }
    }
    if (allieTouche) {
        toucherAllie(jeu, boss, jeu.degatsDanger, false, 0.1f);
    }
    if (touche) {
        toucherAylis(jeu, boss, jeu.degatsDanger, false, 0.1f);
    } else {
        ecrireJournal(jeu, "Esquive ! AYLIS n'etait plus dans la zone.");
    }
    jeu.zonesDanger.clear();
}

// Les cases autour d'un point, jusqu'a une distance donnee
std::vector<std::pair<int, int>> casesAutour(int colonne, int ligne, int distance) {
    std::vector<std::pair<int, int>> cases;
    for (int c = colonne - distance; c <= colonne + distance; c++) {
        for (int l = ligne - distance; l <= ligne + distance; l++) {
            if (distanceCases(c, l, colonne, ligne) <= distance) {
                cases.push_back({c, l});
            }
        }
    }
    return cases;
}

// ===================== Le tour d'un boss =====================
// Renvoie true si le boss a utilise son tour pour une action speciale (il ne se deplace pas en plus)

bool jouerTourDeBoss(Jeu& jeu, Pion& boss) {
    Combattant& stats = boss.stats;
    boss.compteur = boss.compteur + 1;

    // D'abord, ce qui avait ete annonce au tour precedent explose
    bool avaitAnnonce = !jeu.zonesDanger.empty();
    resoudreDanger(jeu, boss);
    if (!jeu.aylis.stats.estDebout()) {
        return true;
    }

    if (boss.boss == BOSS_SKARN) {
        // Tous les 3 tours : il leve sa masse, les cases autour de lui vont voler en eclats
        if (boss.compteur % 3 == 0) {
            annoncerDanger(jeu, casesAutour(boss.colonne, boss.ligne, 2), stats.attaque * 3 / 2);
            ecrireJournal(jeu, "Skarn leve sa masse... ECARTE-TOI des cases rouges !");
            return true;
        }
        return avaitAnnonce;    // juste apres le fracas, il reprend son souffle
    }

    if (boss.boss == BOSS_MATRIARCHE) {
        // Elle se soigne quand elle faiblit
        if (stats.pv < stats.pvMax / 2 && stats.potions > 0 && boss.compteur % 2 == 0) {
            stats.potions = stats.potions - 1;
            stats.soigner(20);
            animerSoin(jeu, boss);
            ecrireJournal(jeu, "La Matriarche boit une decoction : +20 pv.");
            return true;
        }
        // Tous les 3 tours, elle hurle : deux louvetiers accourent
        if (boss.compteur % 3 == 0 && haschenDebout(jeu) < 5) {
            invoquer(jeu, boss, renfort(jeu, "louvetier"), BROWN);
            invoquer(jeu, boss, renfort(jeu, "louvetier"), BROWN);
            ecrireJournal(jeu, "La Matriarche hurle... les loups repondent !");
            return true;
        }
        // Sinon, son malefice : de loin, il empoisonne toujours
        int distance = distanceEntre(boss, jeu.aylis);
        if (distance > 1 && distance <= PORTEE_SORT) {
            bool critique = false;
            int degats = stats.attaque * 90 / 100 - jeu.aylis.stats.defense + GetRandomValue(-1, 1);
            float delai = animerAttaque(jeu, boss, jeu.aylis, true, SorteProjectile::Orbe);
            toucherAylis(jeu, boss, degats < 1 ? 1 : degats, critique, delai);
            jeu.aylis.stats.poison = 3;
            ecrireJournal(jeu, "Malefice ! Le poison ronge AYLIS.");
            return true;
        }
        return false;
    }

    if (boss.boss == BOSS_ASHKA) {
        // Blessee, elle appelle ses archers (une seule fois)
        if (!boss.phase2 && stats.pv <= stats.pvMax / 2) {
            boss.phase2 = true;
            invoquer(jeu, boss, renfort(jeu, "traqueur"), GREEN);
            invoquer(jeu, boss, renfort(jeu, "traqueur"), GREEN);
            ecrireJournal(jeu, "Ashka siffle : ses archers sortent de l'ombre !");
            return true;
        }
        // Tous les 4 tours : une pluie de fleches sur AYLIS et autour
        if (boss.compteur % 4 == 0) {
            annoncerDanger(jeu, casesAutour(jeu.aylis.colonne, jeu.aylis.ligne, 1), stats.attaque * 6 / 5);
            ecrireJournal(jeu, "Ashka bande son arc vers le ciel... une pluie de fleches va tomber !");
            return true;
        }
        return false;
    }

    // Vorgath le Destructeur
    if (!boss.phase2 && stats.pv <= stats.pvMax / 2) {
        boss.phase2 = true;
        stats.attaque = stats.attaque + 4;
        stats.soigner(25);
        invoquer(jeu, boss, renfort(jeu, "brute"), MAROON);
        jeu.secousse = jeu.secousse + 2.0f;
        ecrireJournal(jeu, "VORGATH SE DECHAINE ! La terre se fend sous ses pas.");
        return true;
    }
    // Tous les 2 tours (chaque tour dans sa deuxieme phase) : le sol entre en eruption autour d'AYLIS
    if (boss.phase2 || boss.compteur % 2 == 0) {
        std::vector<std::pair<int, int>> cases = {{jeu.aylis.colonne, jeu.aylis.ligne}};
        int nombre = boss.phase2 ? 6 : 4;
        for (int i = 0; i < nombre; i++) {
            cases.push_back({jeu.aylis.colonne + GetRandomValue(-3, 3), jeu.aylis.ligne + GetRandomValue(-2, 2)});
        }
        annoncerDanger(jeu, cases, stats.attaque * (boss.phase2 ? 3 : 5) / (boss.phase2 ? 2 : 4));
        ecrireJournal(jeu, "Le sol rougeoie sous AYLIS... une eruption se prepare !");
        // Pendant la deuxieme phase, il annonce ET il attaque dans le meme tour
        return !boss.phase2;
    }
    return false;
}

// ===================== Quand un boss tombe =====================
// Son escorte s'enfuit : le combat est gagne

void bossTombe(Jeu& jeu, Pion& boss) {
    jeu.memoire.bossVaincus[boss.boss - 1] = jeu.memoire.bossVaincus[boss.boss - 1] + 1;
    jeu.zonesDanger.clear();
    for (Pion& h : jeu.haschen) {
        if (&h != &boss && h.stats.estDebout()) {
            h.stats.pv = 0;
            animerChute(jeu, h, 0.4f);
        }
    }
    jeu.banniere = boss.stats.nom + " TOMBE !";
    jeu.tempsBanniere = 2.5f;
    ecrireJournal(jeu, boss.stats.nom + " tombe ! Les Haschen s'enfuient.");
}
