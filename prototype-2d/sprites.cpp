// sprites.cpp : le pixel art du jeu
//
// Chaque dessin est une grille de 16 lignes de 16 caracteres. Chaque caractere est une couleur :
//   .  transparent        k  contour sombre      w  blanc           s  peau
//   b  bleu               B  bleu fonce          y  or              n  bois / cuir
//   N  bois fonce         g  metal               G  metal fonce     r  rouge
//   f  fourrure           F  fourrure foncee     e  yeux rouges     p  violet
//   v  vert               V  vert fonce          l  vert clair      o  orange (flammes)
//   Y  jaune              W  neige
//   c  la couleur des vetements (elle change selon le Haschen)
// Pour modifier un dessin, il suffit de changer les lettres !
#include <string>
#include <vector>
#include "sprites.h"

Sprites lesSprites;

// La couleur de chaque lettre
Color couleurDuPixel(char lettre, Color vetement) {
    switch (lettre) {
        case 'k': return {22, 18, 30, 255};
        case 'w': return {235, 235, 240, 255};
        case 's': return {240, 200, 160, 255};
        case 'b': return {70, 130, 220, 255};
        case 'B': return {40, 70, 140, 255};
        case 'y': return {240, 195, 60, 255};
        case 'n': return {140, 95, 55, 255};
        case 'N': return {85, 55, 30, 255};
        case 'g': return {185, 190, 200, 255};
        case 'G': return {105, 105, 118, 255};
        case 'r': return {200, 50, 50, 255};
        case 'f': return {190, 160, 125, 255};
        case 'F': return {125, 95, 70, 255};
        case 'e': return {230, 40, 40, 255};
        case 'p': return {170, 90, 230, 255};
        case 'v': return {70, 150, 60, 255};
        case 'V': return {38, 95, 42, 255};
        case 'l': return {125, 195, 90, 255};
        case 'o': return {240, 130, 40, 255};
        case 'Y': return {250, 225, 90, 255};
        case 'W': return {225, 232, 242, 255};
        case 'c': return vetement;
        default:  return BLANK;     // '.' et tout le reste : transparent
    }
}

// Transforme une grille de texte en texture
Texture2D creerTexture(const std::vector<std::string>& dessin, Color vetement) {
    Image image = GenImageColor(TAILLE_SPRITE, TAILLE_SPRITE, BLANK);
    int lignes = dessin.size();
    for (int y = 0; y < lignes && y < TAILLE_SPRITE; y++) {
        int colonnes = dessin[y].size();
        for (int x = 0; x < colonnes && x < TAILLE_SPRITE; x++) {
            Color couleur = couleurDuPixel(dessin[y][x], vetement);
            if (couleur.a > 0) {
                ImageDrawPixel(&image, x, y, couleur);
            }
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

// Une dalle de sol : une couleur de fond avec quelques cailloux et fissures, placees "au hasard"
// (un hasard fixe, calcule a partir de la position du pixel, pour que la dalle soit toujours la meme)
// Eclaircit (nombre positif) ou assombrit (nombre negatif) une couleur
Color nuance(Color couleur, int ecart) {
    auto borner = [](int valeur) {
        return static_cast<unsigned char>(valeur < 0 ? 0 : (valeur > 255 ? 255 : valeur));
    };
    return {borner(couleur.r + ecart), borner(couleur.g + ecart), borner(couleur.b + ecart), 255};
}

Texture2D creerSol(Color fond, int variante) {
    Image image = GenImageColor(TAILLE_SPRITE, TAILLE_SPRITE, fond);
    for (int y = 0; y < TAILLE_SPRITE; y++) {
        for (int x = 0; x < TAILLE_SPRITE; x++) {
            int bruit = (x * 7 + y * 13 + variante * 29 + x * y * 3) % 23;
            if (bruit == 0) {
                ImageDrawPixel(&image, x, y, nuance(fond, -14));
            } else if (bruit == 7) {
                ImageDrawPixel(&image, x, y, nuance(fond, 12));
            }
        }
    }
    // Un liseré sombre en bas et a droite : on devine les dalles
    for (int i = 0; i < TAILLE_SPRITE; i++) {
        ImageDrawPixel(&image, i, TAILLE_SPRITE - 1, nuance(fond, -8));
        ImageDrawPixel(&image, TAILLE_SPRITE - 1, i, nuance(fond, -8));
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

// ===================== Les dessins =====================

// AYLIS : une silhouette encapuchonnee, cape bleue et fermoir d'or
const std::vector<std::string> DESSIN_AYLIS = {
    "................",
    ".....kkkkkk.....",
    "....kbbbbbbk....",
    "...kbbbbbbbbk...",
    "...kbbssssbbk...",
    "...kbskssksbk...",
    "...kbbssssbbk...",
    "....kbbbbbbk....",
    "...kbbbyybbbk...",
    "..kbbbbyybbbbk..",
    "..ksbbbbbbbbsk..",
    "...kbbbbbbbbk...",
    "...kbbBBBBbbk...",
    "....kBBkkBBk....",
    "....knnk.knnk...",
    "....kkkk.kkkk...",
};

// Les armes, dessinees par-dessus le personnage (dans sa main droite)
const std::vector<std::string> DESSIN_EPEE = {
    "................",
    "................",
    "..............k.",
    ".............kgk",
    "............kgk.",
    "...........kgk..",
    "..........kgk...",
    ".........kgk....",
    "......k.kgk.....",
    ".......kyk......",
    "......kykyk.....",
    ".....knk........",
    "....kk..........",
};

const std::vector<std::string> DESSIN_ARC = {
    "................",
    "............kk..",
    "............knk.",
    "............kwnk",
    "............kw.nk",
    "............kw.nk",
    "............kw.nk",
    "............kw.nk",
    "............kw.nk",
    "............kw.nk",
    "............kwnk",
    "............knk.",
    "............kk..",
};

const std::vector<std::string> DESSIN_BATON = {
    "............kkk.",
    "...........kpppk",
    "...........kpwpk",
    "...........kpppk",
    "............knk.",
    "............knk.",
    "............knk.",
    "............knk.",
    "............knk.",
    "............knk.",
    "............knk.",
    "............knk.",
    "............knk.",
    "............kNk.",
    "............kkk.",
};

// La couronne d'Ashka, posee entre ses oreilles
const std::vector<std::string> DESSIN_COURONNE = {
    "................",
    "....k.k.k.......",
    "....kykykyk.....",
    "....kyyryyk.....",
    "....kkkkkkk.....",
};

// Un Haschen : un guerrier-lievre aux longues oreilles. 'c' = la couleur de sa tenue.
const std::vector<std::string> DESSIN_HASCHEN = {
    "..kk.....kk.....",
    ".kffk...kffk....",
    ".kfFk...kfFk....",
    ".kfFk...kfFk....",
    ".kffkkkkkffk....",
    "..kffffffffk....",
    ".kfffffffffk....",
    ".kffekfffekfk...",
    ".kfffffwwfffk...",
    "..kfffkkfffk....",
    "..kcccccccck....",
    ".kfccccyccccfk..",
    ".kfccccyccccfk..",
    "..kccccccccck...",
    "...kFFk..kFFk...",
    "...kkkk..kkkk...",
};

// ----- La foret -----
const std::vector<std::string> DESSIN_ARBRE = {
    ".....kkkkk......",
    "...kkVvvvVkk....",
    "..kVvvlvvvvVk...",
    ".kVvvlvvvvvvVk..",
    ".kvvvvvvvlvvvk..",
    "kVvlvvvvvvvvvVk.",
    "kvvvvvvvlvvvvvk.",
    "kVvvvvvvvvvvvVk.",
    ".kVvvvlvvvvvVk..",
    "..kkVvvvvvVkk...",
    "....kkkNkkk.....",
    "......kNk.......",
    "......kNnk......",
    ".....kNNnk......",
    "....kkkkkkk.....",
};

const std::vector<std::string> DESSIN_SAPIN = {
    ".......k........",
    "......kVk.......",
    ".....kVvVk......",
    "....kVvlvVk.....",
    ".....kvvvk......",
    "....kVvvvVk.....",
    "...kVvvlvvVk....",
    "....kvvvvvk.....",
    "...kVvvvvvVk....",
    "..kVvvvlvvvVk...",
    "..kkkkVvVkkkk...",
    "......kNk.......",
    "......kNk.......",
    ".....kkkkk......",
};

const std::vector<std::string> DESSIN_HERBE = {
    "................", "................", "................", "................",
    "................", "................", "................", "................",
    "................", "................",
    "....l.....l.....",
    "...lvl...vl..l..",
    "...vVv..lvVlvl..",
    "....V....V..V...",
};

const std::vector<std::string> DESSIN_FLEURS = {
    "................", "................", "................", "................",
    "................", "................", "................", "................",
    ".........Y......",
    "..w.....YyY.....",
    ".wyw.....Y......",
    "..w......v...r..",
    "..v.....lv..ryr.",
    "..vl.....v...v..",
};

// ----- Le camp de guerre -----
const std::vector<std::string> DESSIN_TONNEAU = {
    "................",
    "................",
    "....kkkkkkk.....",
    "...knnnnnnnk....",
    "...kNnnnnnNk....",
    "...kGGGGGGGk....",
    "..knnnnnnnnnk...",
    "..kNnnnnnnnNk...",
    "..knnnnnnnnnk...",
    "..kGGGGGGGGGk...",
    "..knnnnnnnnnk...",
    "...kNnnnnnNk....",
    "...kGGGGGGGk....",
    "....kkkkkkk.....",
};

const std::vector<std::string> DESSIN_TENTE = {
    ".......k........",
    "......knk.......",
    ".....kfFfk......",
    "....kfFfFfk.....",
    "...kfFfkfFfk....",
    "..kfFfkNkfFfk...",
    ".kfFfkNNNkfFfk..",
    "kfFfkNNNNNkfFfk.",
    "kkkkkNNNNNkkkkk.",
    "....kkkkkkk.....",
};

const std::vector<std::string> DESSIN_OS = {
    "................", "................", "................", "................",
    "................", "................", "................", "................",
    "................", "................",
    "..kk.......kk...",
    ".kwwk.....kwk...",
    "..kwwwwwwwwk....",
    ".kwwk.....kwk...",
    "..kk.......kk...",
};

const std::vector<std::string> DESSIN_TRACES = {
    "................", "................", "................", "................",
    "................", "................",
    "...N.N..........",
    "..NNN...........",
    "...N.....N.N....",
    ".........NNN....",
    "..........N.....",
    "....N.N.........",
    "...NNN..........",
    "....N...........",
};

// Le feu de camp : deux images, affichees l'une apres l'autre, font danser les flammes
const std::vector<std::string> DESSIN_FEU_1 = {
    "................", "................", "................", "................",
    "......o.........",
    ".....oYo...o....",
    "....oYYo..oo....",
    "....oYYYooYo....",
    "...ooYwYYYYo....",
    "...oYYwwYYoo....",
    "....oYYYYYo.....",
    "..knNnkknNnk....",
    "..kkNNnnNNkk....",
    "....kkkkkk......",
};

const std::vector<std::string> DESSIN_FEU_2 = {
    "................", "................", "................", "................",
    ".........o......",
    "..o.....oYo.....",
    "..oo...oYYo.....",
    "...oYooYYYo.....",
    "...oYYYYwYoo....",
    "...ooYYwwYYo....",
    "....oYYYYYo.....",
    "..knNnkknNnk....",
    "..kkNNnnNNkk....",
    "....kkkkkk......",
};

// ----- Le col d'Ashka -----
const std::vector<std::string> DESSIN_CAILLOUX = {
    "................", "................", "................", "................",
    "................", "................", "................", "................",
    "................", "................",
    "...kk.......kk..",
    "..kGgk.....kgk..",
    "..kGGk..kk..kk..",
    "...kk..kGgk.....",
    "........kk......",
};

const std::vector<std::string> DESSIN_NEIGE = {
    "................", "................", "................", "................",
    "................", "................", "................",
    "....WW..........",
    "..WWWWWW........",
    ".WWWWWWWWW..W...",
    "..WWWWWWW..WWW..",
    "....WWW....WW...",
};

const std::vector<std::string> DESSIN_ROCHER = {
    "................",
    "................",
    ".....kkkkk......",
    "....kGgggGkk....",
    "...kGgggwggGk...",
    "..kGgggggwgggk..",
    "..kgggGggggggk..",
    ".kGggggggGgggGk.",
    ".kgggggggggGggk.",
    ".kGgggGgggggggk.",
    ".kGggggggggGgGk.",
    "..kGGgggGggGGk..",
    "...kkGGGGGGkk...",
    ".....kkkkkk.....",
};

// ===================== Charger et decharger =====================

void chargerSprites() {
    Color aucune = BLANK;
    lesSprites.aylis = creerTexture(DESSIN_AYLIS, aucune);
    lesSprites.epee = creerTexture(DESSIN_EPEE, aucune);
    lesSprites.arc = creerTexture(DESSIN_ARC, aucune);
    lesSprites.baton = creerTexture(DESSIN_BATON, aucune);
    lesSprites.couronne = creerTexture(DESSIN_COURONNE, aucune);

    // Le meme lievre, avec une tenue differente pour chaque type de Haschen
    lesSprites.eclaireur = creerTexture(DESSIN_HASCHEN, {230, 140, 40, 255});   // orange
    lesSprites.guerrier = creerTexture(DESSIN_HASCHEN, {190, 45, 45, 255});     // rouge
    lesSprites.traqueur = creerTexture(DESSIN_HASCHEN, {60, 150, 70, 255});     // vert
    lesSprites.chaman = creerTexture(DESSIN_HASCHEN, {140, 70, 190, 255});      // violet
    lesSprites.louvetier = creerTexture(DESSIN_HASCHEN, {110, 75, 45, 255});    // brun
    lesSprites.ashka = creerTexture(DESSIN_HASCHEN, {120, 20, 40, 255});        // pourpre

    // La foret : herbe, arbres, touffes et fleurs
    lesSprites.sol[0][0] = creerSol({58, 92, 52, 255}, 0);
    lesSprites.sol[0][1] = creerSol({52, 84, 47, 255}, 1);
    lesSprites.obstacle[0][0] = creerTexture(DESSIN_ARBRE, aucune);
    lesSprites.obstacle[0][1] = creerTexture(DESSIN_SAPIN, aucune);
    lesSprites.decor[0][0] = creerTexture(DESSIN_HERBE, aucune);
    lesSprites.decor[0][1] = creerTexture(DESSIN_FLEURS, aucune);

    // Le camp : terre battue, tonneaux et tentes, os et traces de pas
    lesSprites.sol[1][0] = creerSol({110, 84, 58, 255}, 2);
    lesSprites.sol[1][1] = creerSol({100, 76, 52, 255}, 3);
    lesSprites.obstacle[1][0] = creerTexture(DESSIN_TONNEAU, aucune);
    lesSprites.obstacle[1][1] = creerTexture(DESSIN_TENTE, aucune);
    lesSprites.decor[1][0] = creerTexture(DESSIN_OS, aucune);
    lesSprites.decor[1][1] = creerTexture(DESSIN_TRACES, aucune);
    lesSprites.feu[0] = creerTexture(DESSIN_FEU_1, aucune);
    lesSprites.feu[1] = creerTexture(DESSIN_FEU_2, aucune);

    // Le col : dalles de pierre froide, rochers, cailloux et neige
    lesSprites.sol[2][0] = creerSol({78, 82, 96, 255}, 4);
    lesSprites.sol[2][1] = creerSol({70, 74, 88, 255}, 5);
    lesSprites.obstacle[2][0] = creerTexture(DESSIN_ROCHER, aucune);
    lesSprites.obstacle[2][1] = creerTexture(DESSIN_ROCHER, aucune);
    lesSprites.decor[2][0] = creerTexture(DESSIN_CAILLOUX, aucune);
    lesSprites.decor[2][1] = creerTexture(DESSIN_NEIGE, aucune);
}

void dechargerSprites() {
    const Texture2D textures[] = {
        lesSprites.aylis, lesSprites.epee, lesSprites.arc, lesSprites.baton, lesSprites.couronne,
        lesSprites.eclaireur, lesSprites.guerrier, lesSprites.traqueur, lesSprites.chaman,
        lesSprites.louvetier, lesSprites.ashka, lesSprites.feu[0], lesSprites.feu[1],
    };
    for (const Texture2D& texture : textures) {
        UnloadTexture(texture);
    }
    for (int lieu = 0; lieu < 3; lieu++) {
        for (int variante = 0; variante < 2; variante++) {
            UnloadTexture(lesSprites.sol[lieu][variante]);
            UnloadTexture(lesSprites.obstacle[lieu][variante]);
            UnloadTexture(lesSprites.decor[lieu][variante]);
        }
    }
}

const Sprites& sprites() {
    return lesSprites;
}

// Est-ce que le nom contient ce mot ? (find renvoie std::string::npos quand il ne trouve pas)
bool contient(const std::string& nom, const std::string& mot) {
    return nom.find(mot) != std::string::npos;
}

const Texture2D& spriteHaschen(const std::string& nom) {
    if (contient(nom, "Ashka")) return lesSprites.ashka;
    if (contient(nom, "eclaireur")) return lesSprites.eclaireur;
    if (contient(nom, "traqueur")) return lesSprites.traqueur;
    if (contient(nom, "chaman")) return lesSprites.chaman;
    if (contient(nom, "louvetier")) return lesSprites.louvetier;
    return lesSprites.guerrier;
}

const Texture2D* spriteArmeHaschen(const std::string& nom) {
    if (contient(nom, "traqueur") || contient(nom, "Ashka")) return &lesSprites.arc;
    if (contient(nom, "chaman")) return &lesSprites.baton;
    if (contient(nom, "louvetier")) return nullptr;     // il se bat avec ses crocs... et son loup
    return &lesSprites.epee;
}
