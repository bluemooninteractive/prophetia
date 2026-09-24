// lieux.cpp : les lieux de la route, et leurs cartes dessinees en texte
//
// Comme pour les sprites, chaque carte est une petite grille de texte : 8 lignes de 12 caracteres,
// une lettre par case de l'arene. Pour changer une carte, il suffit de changer les lettres !
//
//   .  sol libre            ,  chemin (libre)            *  source de lumiere (libre)
//   g  herbes, roseaux      w  toile d'araignee          x  gravats (libre)
//   =  pont ou gue (libre)  G  porte de forteresse (libre)
//   T  arbre (bloque)       r  rocher                    ~  eau
//   H  maison (2x2 cases)   A  tente (2x2 cases)         #  rempart
//   I  tour                 f  cloture ou palissade      o  puits
//   b  tonneau              F  feu de camp
// Dans les Cendres et la citadelle de Vorgath, l'eau (~) est de la LAVE, et les ponts (=) sont en pierre.
//
// AYLIS commence toujours en colonne 1, ligne 3 : cette case doit etre libre.
#include "jeu2d.h"

// ===================== Les noms =====================

std::string nomLieu(int lieu) {
    switch (lieu) {
        case 0: return "La foret des Brumes";
        case 1: return "Le village de Brennac";
        case 2: return "Le gue des Saules";
        case 3: return "Le bois des Pendus";
        case 4: return "Le camp de guerre haschen";
        case 5: return "La forteresse de Karn";
        case 6: return "Le col d'Ashka";
        case 7: return "Les Cendres";
        default: return "La citadelle de Vorgath";
    }
}

// Un nom tres court, pour la frise de la route
std::string nomCourtLieu(int lieu) {
    const char* noms[NOMBRE_LIEUX] = {"Foret", "Village", "Gue", "Bois", "Camp", "Karn", "Col", "Cendres", "Citadelle"};
    return noms[lieu];
}

// Le marchand qu'on croise dans chaque lieu : Maren au debut, Durgan au milieu, Silas a la fin
int marchandDuLieu(int lieu) {
    if (lieu <= 1) {
        return 0;
    }
    return lieu <= 4 ? 1 : 2;
}

// ===================== Les cartes =====================

using Carte = std::vector<std::string>;

// Deux cartes par lieu, dans l'ordre de la route
const std::vector<Carte> CARTES[NOMBRE_LIEUX] = {
    {   // 0. La foret des Brumes : des arbres serres, et un ruisseau qu'on traverse a gue
        {"TT.T..T..TTT",
         "T..*....T..T",
         "..T.....g...",
         ".....T......",
         "..g.....*...",
         "~~=~~~~~~=~~",
         "T.....T....T",
         "TT.T*..TT.TT"},
        {"TTT.TT.TT.TT",
         "T.*..T....*T",
         "..T....T....",
         "....g.......",
         ".T....TT..T.",
         "...T.......T",
         "T*...g..T...",
         "TT.TTT.TTT.T"},
    },
    {   // 1. Le village de Brennac : pille par les Haschen. Des maisons, un puits, des clotures
        {"HH.,..HH.ff.",
         "HH.,..HH...*",
         "...,*.......",
         ".,,,,,,,,,,,",
         "...o...,..b.",
         ".f.....,....",
         ".f.HH..,.HH*",
         "*..HH..,.HH."},
        {"..*...HH..HH",
         ".HH...HH..HH",
         ".HH.b..,....",
         "....,,,,,..*",
         "ff..,.o.,...",
         "....,...,.HH",
         ".HH.*...,.HH",
         ".HH.....,..."},
    },
    {   // 2. Le gue des Saules : une riviere, des ponts, des saules et des roseaux
        {"T...g~~..T..",
         "..*..~~g....",
         ".g...==....*",
         "....r~~.....",
         "..T..~~..r..",
         ".....~~g....",
         ".*...==...T.",
         "T..g.~~..g.."},
        {"T..*..g..T..",
         ".r......r..*",
         "~~~~==~~~~~~",
         "............",
         "..g..T...g..",
         "~~~~~~~==~~~",
         ".*..g.......",
         "T......r..T."},
    },
    {   // 3. Le bois des Pendus : des arbres morts, des toiles, des pierres runiques qui luisent
        {"TT.T.w.T.TTT",
         "T..w...T...T",
         "..T..*....w.",
         ".....T......",
         "w.T......T..",
         "...*..T....w",
         "T.w....T.*.T",
         "TT.T.T..TT.T"},
        {"T.TTT.w.TTT.",
         ".w..T...*..T",
         "*...w..T....",
         "...T........",
         ".T....T..w.T",
         "...w.......*",
         "T.T..T.w.T..",
         "TT..TT.TT.TT"},
    },
    {   // 4. Le camp de guerre haschen : des tentes, des palissades, le grand feu
        {"ffffff.fffff",
         "..AA...b..AA",
         "..AA.*....AA",
         "......F.....",
         "..b.....*...",
         ".AA......AA.",
         ".AA..*...AA.",
         "ffff.ffff.ff"},
        {".AA..fff..*.",
         ".AA.......b.",
         "....*..AA...",
         "......FAA...",
         "b...........",
         "...AA...*.AA",
         "*..AA.....AA",
         "fff...fff..."},
    },
    {   // 5. La forteresse de Karn : des remparts en ruine, des tours, des portes
        {"......I####I",
         "..x...#..*.#",
         "......#....#",
         "..*...G.....",
         "......#....#",
         "..x...#.*..#",
         "*.....I##G#I",
         "...x........"},
        {"I#.#I...I#.I",
         "#..*#...#..#",
         "..x......x..",
         ".......*....",
         "#.I....I.#..",
         "#......#.x.*",
         "..x.*.....I#",
         "I##.#I..###I"},
    },
    {   // 6. Le col d'Ashka : des rochers, des cristaux, la neige
        {"....r...r...",
         "*...r...r..*",
         "......r.....",
         "......r.....",
         ".r........r.",
         "....*...r...",
         "....r...r*..",
         "..r........."},
        {"r.*...r....r",
         ".....r...*..",
         "..r.........",
         ".......r....",
         "...r...r..r.",
         ".*...r......",
         "......r..r.*",
         "r..r.....r.."},
    },
    {   // 7. Les Cendres : un desert de cendre, fendu par des rivieres de lave
        {"r...~~..r...",
         "..*.~~....*.",
         "....==..r...",
         "....~~.r....",
         "....~~~~==~~",
         "..r.....*...",
         "*...r....r..",
         "...r......r."},
        {"..r......r..",
         ".*...r....*.",
         "~~~==~~~~~~~",
         "............",
         "..r.*...r...",
         "~~~~~~~==~~~",
         ".r.......*..",
         "....r....r.."},
    },
    {   // 8. La citadelle de Vorgath : des remparts d'obsidienne, des bassins de lave, des braseros
        {"I###I..I###I",
         "#..*#..#*..#",
         "............",
         "..~~....~~..",
         "..~~.**.~~..",
         "............",
         "#..*#..#*..#",
         "I###I..I###I"},
        {"..I#G#I.....",
         "..#...#..*..",
         "*.#.*.#.....",
         "............",
         "~~~==~~~~==~",
         "......*.....",
         ".I##G##I..r.",
         ".#.....#...."},
    },
};

// Les lettres qui bloquent le passage
bool estBloquant(char lettre) {
    switch (lettre) {
        case 'T': case 'r': case '~': case 'H': case 'A': case '#':
        case 'I': case 'f': case 'o': case 'b': case 'F':
            return true;
        default:
            return false;
    }
}

char caseDuTerrain(const Jeu& jeu, int colonne, int ligne) {
    if (!estDansArene(colonne, ligne)) {
        return ' ';
    }
    return jeu.terrain[ligne * COLONNES + colonne];
}

// Les cases qu'on peut atteindre depuis la case de depart d'AYLIS (sans compter les pions) :
// les Haschen ne sont places que la, pour qu'aucun ne soit coince derriere une riviere ou un rempart.
std::vector<bool> casesAccessibles(const Jeu& jeu) {
    std::vector<bool> vu(COLONNES * LIGNES, false);
    std::vector<int> aVisiter = {3 * COLONNES + 1};
    vu[3 * COLONNES + 1] = true;
    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < (int)aVisiter.size(); i++) {
        int c = aVisiter[i] % COLONNES;
        int l = aVisiter[i] / COLONNES;
        for (const auto& dir : directions) {
            int nc = c + dir[0];
            int nl = l + dir[1];
            if (estDansArene(nc, nl) && !estRocher(jeu, nc, nl) && !vu[nl * COLONNES + nc]) {
                vu[nl * COLONNES + nc] = true;
                aVisiter.push_back(nl * COLONNES + nc);
            }
        }
    }
    return vu;
}

// Choisit une carte du lieu, et remplit le terrain, les obstacles et les lumieres
void chargerCarte(Jeu& jeu) {
    const std::vector<Carte>& cartes = CARTES[jeu.lieu];
    const Carte& carte = cartes[GetRandomValue(0, (int)cartes.size() - 1)];
    // Parfois, la carte est retournee de haut en bas : encore un peu plus de variete
    bool retournee = GetRandomValue(0, 1) == 1;

    jeu.terrain.assign(COLONNES * LIGNES, '.');
    jeu.rochers.assign(COLONNES * LIGNES, false);
    jeu.lumieres.clear();
    jeu.feuColonne = -1;
    jeu.feuLigne = -1;
    for (int l = 0; l < LIGNES; l++) {
        const std::string& ligne = carte[retournee ? LIGNES - 1 - l : l];
        for (int c = 0; c < COLONNES; c++) {
            char lettre = c < (int)ligne.size() ? ligne[c] : '.';
            jeu.terrain[l * COLONNES + c] = lettre;
            jeu.rochers[l * COLONNES + c] = estBloquant(lettre);
            if (lettre == '*') {
                jeu.lumieres.push_back({c, l});
            } else if (lettre == 'F') {
                jeu.feuColonne = c;
                jeu.feuLigne = l;
            }
        }
    }
    // Une maison ou une tente retournee de haut en bas reste une maison : elles font 2 cases de haut,
    // donc rien a corriger. Mais la case de depart d'AYLIS doit toujours etre libre.
    int depart = 3 * COLONNES + 1;
    if (jeu.rochers[depart]) {
        jeu.rochers[depart] = false;
        jeu.terrain[depart] = '.';
    }
}
