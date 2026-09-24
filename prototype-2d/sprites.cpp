// sprites.cpp : le pixel art du jeu
//
// Chaque dessin est une grille de 16 lignes de 16 caracteres. Chaque caractere est une couleur :
//   .  transparent        k  contour sombre      w  blanc           s  peau
//   b  bleu               B  bleu fonce          y  or              n  bois / cuir
//   N  bois fonce         g  metal               G  metal fonce     r  rouge
//   f  fourrure           F  fourrure foncee     e  yeux rouges     p  violet
//   v  vert               V  vert fonce          l  vert clair      o  orange (flammes)
//   Y  jaune              W  neige
//   u  cyan lumineux       U  cyan sombre         L  violet clair    P  violet sombre
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
        case 'u': return {110, 235, 240, 255};     // cyan lumineux (les champignons de la foret)
        case 'U': return {40, 120, 140, 255};      // cyan sombre
        case 'L': return {215, 170, 255, 255};     // violet clair (les cristaux du col)
        case 'P': return {95, 50, 150, 255};       // violet sombre
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

// Les sources de lumiere, une par lieu : elles brillent dans le noir (voir la lumiere dans dessin.cpp)
// La foret : des champignons qui luisent
const std::vector<std::string> DESSIN_CHAMPIGNON = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "..........kkk...",
    ".........kuuuk..",
    "..kkkk..kuuwuuk.",
    ".kuuuuk.kUUUUUk.",
    "kuuwuuuk..kwk...",
    "kUUUUUUk..kwk...",
    "..kwwk....kwk...",
    "..kwwk...kVVVk..",
    ".kVVVVk.kVVVVVk.",
    "................",
    "................",
};

// Le camp : un brasero de fer, plein de braises
const std::vector<std::string> DESSIN_BRASERO = {
    "................",
    "......o..Y......",
    ".....oYo.oo.....",
    "....oYYYoYYo....",
    "....oYwYYYYo....",
    "...kooYYYYook...",
    "...kGGGGGGGGk...",
    "....kGgggggk....",
    ".....kGGGGk.....",
    "......kGGk......",
    "......kGGk......",
    "......kGGk......",
    ".....kGGGGk.....",
    "....kGGkkGGk....",
    "....kkk..kkk....",
    "................",
};

// Le col : des cristaux violets, charges de la magie de la prophetie
const std::vector<std::string> DESSIN_CRISTAL = {
    "................",
    ".......k........",
    "......kLk.......",
    "......kLpk......",
    ".....kLLpk......",
    "..k..kLwpk..k...",
    ".kLk.kLLpk.kLk..",
    ".kLpkkLLppkkLpk.",
    ".kLpkkLwppkkLpk.",
    ".kLppkLLppkLppk.",
    "kGkLpkLLpPkLpkGk",
    "kGGkPkLppPkPkGGk",
    "kGGGkkPPPPkkGGGk",
    ".kkkkkkkkkkkkkk.",
    "................",
    "................",
};

// ----- Les boss -----

// Skarn : un casque de fer a cornes, par-dessus la tete d'un Haschen (ses oreilles passent au travers)
const std::vector<std::string> DESSIN_CASQUE = {
    "................",
    "................",
    "k...........k...",
    "gk.........kg...",
    "kgk.......kgk...",
    ".kGGGGGGGGGGk...",
    ".kGgGGGGGGgGk...",
};

// La Matriarche : une ramure de bois de cerf, et une rune verte sur le front
const std::vector<std::string> DESSIN_RAMURE = {
    "N.N.........N.N.",
    ".NN.N.....N.NN..",
    "..NNN.....NNN...",
    "...kN.....Nk....",
    "....kNNNNNk.....",
    "................",
    "......l.........",
};

// Vorgath le Destructeur : une armure noire, des cornes, et la lave qui coule dans les fissures
const std::vector<std::string> DESSIN_VORGATH = {
    "kk..........kk..",
    "kGk........kGk..",
    ".kGk......kGk...",
    ".kGGkkkkkkGGk...",
    "..kGGGGGGGGk....",
    ".kGGkeGGekGGk...",
    ".kGGGGooGGGGk...",
    "..kGGGGGGGGk....",
    ".kokGggggGkok...",
    "kGGkGGooGGkGGk..",
    "kGokGGGGGGkoGk..",
    "kGGkGoGGoGkGGk..",
    ".kk.kGGGGGGk.kk.",
    "....kGGkkGGk....",
    "....kGGk.kGGk...",
    "....kkkk.kkkk...",
};

// ----- L'acte IV : les Cendres et la citadelle de Vorgath -----

// Une roche volcanique, fendue de lave
const std::vector<std::string> DESSIN_ROCHE_VOLCANIQUE = {
    "................",
    "................",
    "................",
    "......kkkk......",
    "....kkGGGGkk....",
    "...kGGGoGGGGk...",
    "..kGGGGGoGGGGk..",
    "..kGGoGGGoGGGk..",
    ".kGGGGoGGGGGGGk.",
    ".kGGGGGoGGoGGGk.",
    ".kGGGGGGGGoGGGk.",
    "..kGGGGGGGGGGk..",
    "...kkkkkkkkkk...",
    "................",
    "................",
    "................",
};

// De la cendre grise, et quelques braises
const std::vector<std::string> DESSIN_CENDRES = {
    "................",
    "................",
    "....G...........",
    "...GgG......o...",
    "....G...........",
    "..........G.....",
    ".........GgG....",
    "..o.......G.....",
    "................",
    "......G.........",
    ".....GgG....o...",
    "......G.........",
    "................",
    "..G.............",
    ".GgG............",
    "................",
};

// Une fissure de lave dans le sol : elle eclaire les Cendres
const std::vector<std::string> DESSIN_FISSURE = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "..k.............",
    "..kok...........",
    "...koYk.....k...",
    "....kYoYk..kok..",
    ".....koYYokoYk..",
    "......kkoYYok...",
    "........kkkk....",
    "................",
    "................",
    "................",
    "................",
};

// ----- Les nouveaux lieux de la route -----

// Le gue des Saules : un saule pleureur, dont les branches retombent
const std::vector<std::string> DESSIN_SAULE = {
    ".....kkkkkk.....",
    "...kkvlvvlvkk...",
    "..kvlvvvvvvlvk..",
    ".kvvlvvlvvlvvvk.",
    ".kvlkvvvvvvkvlk.",
    "kvlk.kvnnvk.kvlk",
    "kvk..kvnnvk..kvk",
    "kvk...knnk...kvk",
    "klk...knnk...klk",
    "kvk...knnk...kvk",
    ".k....knnk....k.",
    "......knnk......",
    ".....kNnnNk.....",
    "....kNNkkNNk....",
    "................",
    "................",
};

// Le bois des Pendus : un arbre mort, tordu, aux branches comme des griffes
const std::vector<std::string> DESSIN_ARBRE_MORT = {
    "..k.......k.....",
    "..kk....kk..k...",
    "...kFk.kFk.kk...",
    "k...kFkFk.kFk...",
    "kk...kFFkkFk..k.",
    ".kFk..kFFFk..kk.",
    "..kFkkkFFk..kFk.",
    "....kFFFFkkFFk..",
    "......kFFFFk....",
    "......kFFFk.....",
    "......kFFFk.....",
    "......kFFFFk....",
    ".....kFFkFFk....",
    "....kFFk.kFFk...",
    "...kkk....kkk...",
    "................",
};

// Des roseaux au bord de l'eau
const std::vector<std::string> DESSIN_ROSEAUX = {
    "................",
    "................",
    "....n.....n.....",
    "....n..n..n.....",
    "...kn..n..nk....",
    "...vk..n..kv..n.",
    "...v...v...v..n.",
    "..v...v....v..v.",
    "..v...v.....v.v.",
    "..v...v.....v.v.",
    ".v...v.....v.v..",
    ".v...v.....v.v..",
    "................",
    "................",
    "................",
    "................",
};

// Des feuilles mortes, tombees des arbres morts
const std::vector<std::string> DESSIN_FEUILLES = {
    "................",
    "................",
    "................",
    "..o.............",
    ".oFo......N.....",
    "..o......NnN....",
    ".........nN.....",
    "................",
    "......F.........",
    ".....FoF....o...",
    "......F....oFo..",
    ".............o..",
    "...N............",
    "..NnN...........",
    "................",
    "................",
};

// Des gravats : les pierres tombees des remparts
const std::vector<std::string> DESSIN_GRAVATS = {
    "................",
    "................",
    "................",
    "...kkk..........",
    "..kgGGk.........",
    "..kGGGk....kk...",
    "...kkk....kgGk..",
    "..........kGGk..",
    "...........kk...",
    ".....kk.........",
    "....kgGk....k...",
    "....kGGk...kGk..",
    ".....kk.....k...",
    "................",
    "................",
    "................",
};

// Le village : une lanterne sur un poteau
const std::vector<std::string> DESSIN_LANTERNE = {
    "................",
    ".......kk.......",
    "......kNNk......",
    ".....kkkkkk.....",
    ".....kYwYYk.....",
    ".....kYYYYk.....",
    ".....kYYYok.....",
    ".....kkkkkk.....",
    ".......kk.......",
    ".......Nk.......",
    ".......Nk.......",
    ".......Nk.......",
    ".......Nk.......",
    ".......Nk.......",
    ".....kNNNNk.....",
    "................",
};

// Le gue : un nenuphar qui luit a la surface
const std::vector<std::string> DESSIN_NENUPHAR = {
    "................",
    "................",
    "................",
    "................",
    "................",
    ".......k........",
    "......kuk.......",
    ".....kuwuk......",
    "..kk.kuuuk.kk...",
    ".kVVkkUuUkkVVk..",
    "kVlVVVkUkVVVlVk.",
    "kVVVVVVVVVVVVVk.",
    ".kVVVVVVVVVVVk..",
    "..kkkkkkkkkkk...",
    "................",
    "................",
};

// La forteresse : une torche plantee dans un support de fer
const std::vector<std::string> DESSIN_TORCHE = {
    "................",
    ".......o........",
    "......oYo.......",
    ".....oYwYo......",
    ".....oYYYo......",
    "......oYo.......",
    "......kNk.......",
    "......kNk.......",
    ".....kGNGk......",
    "......kNk.......",
    "......kNk.......",
    "......kNk.......",
    "......kNk.......",
    ".....kGGGk......",
    "....kGGGGGk.....",
    "................",
};

// Le bois des Pendus : une pierre levee, gravee de runes qui luisent d'un vert malsain
const std::vector<std::string> DESSIN_PIERRE_RUNIQUE = {
    "................",
    "......kkkk......",
    ".....kGGGGk.....",
    "....kGGlGGGk....",
    "....kGllGGGk....",
    "....kGGlGGGk....",
    "....kGGGGlGk....",
    "....kGGGllGk....",
    "....kGGGGlGk....",
    "....kGlGGGGk....",
    "....kGllGGGk....",
    "....kGGlGGGk....",
    "...kGGGGGGGGk...",
    "..kVkGGGGGGkVk..",
    "..kkkkkkkkkkkk..",
    "................",
};

// Les marchands de la route
// Maren, l'herboriste : un capuchon vert et une fiole de potion
const std::vector<std::string> DESSIN_MAREN = {
    "................",
    ".....kkkkkk.....",
    "....kVVVVVVk....",
    "...kVvvvvvvVk...",
    "...kVvssssvVk...",
    "...kvskssksvk...",
    "...kvvssssvvk...",
    "....kvvvvvvk....",
    "...kvvvvvvvvk.k.",
    "..kvvvvnvvvvkkrk",
    "..ksvvvnvvvvskrk",
    "...kvvvvvvvvk.k.",
    "...kvvvvvvvvk...",
    "...kVVVVVVVVk...",
    "....knnk.knnk...",
    "....kkkk.kkkk...",
};

// Durgan, le forgeron : une grande barbe, un tablier de cuir et un marteau
const std::vector<std::string> DESSIN_DURGAN = {
    "..........kkk...",
    "..........kGGk..",
    "....kkkkkkkGGk..",
    "...kNNNNNNkkkk..",
    "...kNssssNk.kk..",
    "...kskssksk.kn..",
    "...kssssssk.kn..",
    "...koooooooknk..",
    "..kgoooooooosk..",
    "..ksnoooooonkk..",
    "..ksnnooooonnk..",
    "...knnnnnnnnk...",
    "...knnnnnnnnk...",
    "...kNNNNNNNNk...",
    "....kNNk.kNNk...",
    "....kkkk.kkkk...",
};

// Silas, le collectionneur : une cape violette, un visage dans l'ombre et une lanterne
const std::vector<std::string> DESSIN_SILAS = {
    "................",
    ".....kkkkkk.....",
    "....kppppppk....",
    "...kppkkkkppk...",
    "...kpkkkkkkpk...",
    "...kpkYkkYkpk...",
    "...kpkkkkkkpk...",
    "....kppppppk....",
    "...kppppppppk...",
    "..kppppyypppkk..",
    "..kspppyyppkYk..",
    "...kppppppppkoYk",
    "...kppppppppkYk.",
    "...kppppppppk...",
    "....kNNk.kNNk...",
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

// ----- Les icones des actions -----
const std::vector<std::vector<std::string>> DESSINS_ICONES = {
    {   // 1. Attaque : une epee
        "................", "............kk..", "...........kgk..", "..........kgk...",
        ".........kgk....", "........kgk.....", ".......kgk......", "..k...kgk.......",
        "..kk.kgk........", "...kykk.........", "...kyk..........", "..kykyk.........",
        ".knk..kk........", "kk..............",
    },
    {   // 2. Attaque lourde : un marteau
        "................", "..kkkkkkk.......", ".kGggggggk......", ".kGggggggk......",
        ".kGGgggGGk......", "..kkkkkkkk......", ".....knk........", ".....knk........",
        ".....knk........", ".....knk........", ".....knk........", ".....knk........",
        ".....kNk........", ".....kkk........",
    },
    {   // 3. Garde : un bouclier
        "................", "..kkkkkkkkkk....", "..kgggggggGk....", "..kgbbbbbbGk....",
        "..kgbbyybbGk....", "..kgbyyyybGk....", "..kgbbyybbGk....", "..kgbbbbbbGk....",
        "...kgbbbbGk.....", "...kgbbbbGk.....", "....kgbbGk......", ".....kggk.......",
        "......kk........",
    },
    {   // 4. Potion : une fiole rouge
        "................", "......kkk.......", "......knk.......", "......kwk.......",
        ".....kwwwk......", "....kwrrrwk.....", "...kwrrrrrwk....", "...kwrwrrrwk....",
        "...kwrrrrrwk....", "...kwrrrrrwk....", "....kwrrrwk.....", ".....kkkkk......",
    },
    {   // 5. Boule de feu : une flamme
        "................", ".......o........", "......oo........", "......oYo.......",
        ".....oYYo..o....", "....oYYYo.oo....", "....oYwYYoYo....", "...oYYwwYYYo....",
        "...oYwwwwYYo....", "...oYYwwYYYo....", "....oYYYYYo.....", ".....ooooo......",
    },
    {   // 6. Soin : un coeur vert
        "................", "................", "...kkk...kkk....", "..kvlvk.kvvvk...",
        ".kvlwlvkvvvvvk..", ".kvlvvvvvvvvvk..", ".kvvvvvvvvvvVk..", "..kvvvvvvvvVk...",
        "...kvvvvvvVk....", "....kvvvvVk.....", ".....kvvVk......", "......kVk.......",
        ".......k........",
    },
    {   // 7. Eclair : un eclair jaune
        "................", "........kkkk....", ".......kYYYk....", "......kYYYk.....",
        ".....kYYYk......", "....kYYYYkkk....", "...kYYYYYYYk....", "....kkkYYYk.....",
        "......kYYk......", ".....kYYk.......", "....kYYk........", "....kYk.........",
        "....kk..........",
    },
    {   // 8. Bouclier magique : une bulle bleue
        "................", ".....kkkkk......", "...kkbbbbbkk....", "..kbbwwbbbbbk...",
        "..kbwwbbbbbbk...", ".kbbwbbbbbbbbk..", ".kbbbbbbbbbbbk..", ".kbbbbbbbbbbbk..",
        ".kbbbbbbbbbbBk..", "..kbbbbbbbbBk...", "..kBbbbbbbBBk...", "...kkBBBBBkk....",
        ".....kkkkk......",
    },
    {   // 9. Attaque speciale : une etoile de rage
        "................", ".......k........", "......kok.......", "......kok.......",
        ".kkkkkoYokkkkk..", "..kooYYwYYook...", "...kooYYYook....", "....koYYYok.....",
        "....koYkYok.....", "...koYk.kYok....", "...kok...kok....", "..kok.....kok...",
        "..kk.......kk...",
    },
};

// ===================== Charger et decharger =====================

void chargerSprites() {
    Color aucune = BLANK;
    lesSprites.aylis = creerTexture(DESSIN_AYLIS, aucune);
    lesSprites.epee = creerTexture(DESSIN_EPEE, aucune);
    lesSprites.arc = creerTexture(DESSIN_ARC, aucune);
    lesSprites.baton = creerTexture(DESSIN_BATON, aucune);
    lesSprites.couronne = creerTexture(DESSIN_COURONNE, aucune);
    lesSprites.marchands[0] = creerTexture(DESSIN_MAREN, aucune);
    lesSprites.marchands[1] = creerTexture(DESSIN_DURGAN, aucune);
    lesSprites.marchands[2] = creerTexture(DESSIN_SILAS, aucune);

    // Le meme lievre, avec une tenue differente pour chaque type de Haschen
    lesSprites.eclaireur = creerTexture(DESSIN_HASCHEN, {230, 140, 40, 255});   // orange
    lesSprites.guerrier = creerTexture(DESSIN_HASCHEN, {190, 45, 45, 255});     // rouge
    lesSprites.traqueur = creerTexture(DESSIN_HASCHEN, {60, 150, 70, 255});     // vert
    lesSprites.chaman = creerTexture(DESSIN_HASCHEN, {140, 70, 190, 255});      // violet
    lesSprites.louvetier = creerTexture(DESSIN_HASCHEN, {110, 75, 45, 255});    // brun
    lesSprites.ashka = creerTexture(DESSIN_HASCHEN, {120, 20, 40, 255});        // pourpre

    // Chaque lieu de la route : son sol (2 teintes), ses "arbres", ses petits details et sa source de lumiere.
    // Une petite fonction pour remplir un lieu en une ligne
    auto lieu = [&](int numero, Color sol1, Color sol2, const std::vector<std::string>& arbre1,
                    const std::vector<std::string>& arbre2, const std::vector<std::string>& detail1,
                    const std::vector<std::string>& detail2, const std::vector<std::string>& lumiere) {
        lesSprites.sol[numero][0] = creerSol(sol1, numero * 2);
        lesSprites.sol[numero][1] = creerSol(sol2, numero * 2 + 1);
        lesSprites.obstacle[numero][0] = creerTexture(arbre1, aucune);
        lesSprites.obstacle[numero][1] = creerTexture(arbre2, aucune);
        lesSprites.decor[numero][0] = creerTexture(detail1, aucune);
        lesSprites.decor[numero][1] = creerTexture(detail2, aucune);
        lesSprites.lumineux[numero] = creerTexture(lumiere, aucune);
    };
    lieu(0, {58, 92, 52, 255}, {52, 84, 47, 255}, DESSIN_ARBRE, DESSIN_SAPIN, DESSIN_HERBE, DESSIN_FLEURS,
         DESSIN_CHAMPIGNON);                                                     // la foret des Brumes
    lieu(1, {96, 92, 62, 255}, {90, 86, 58, 255}, DESSIN_ARBRE, DESSIN_ARBRE, DESSIN_HERBE, DESSIN_TRACES,
         DESSIN_LANTERNE);                                                       // le village de Brennac
    lieu(2, {60, 100, 72, 255}, {54, 92, 66, 255}, DESSIN_SAULE, DESSIN_SAULE, DESSIN_ROSEAUX, DESSIN_CAILLOUX,
         DESSIN_NENUPHAR);                                                       // le gue des Saules
    lieu(3, {44, 48, 44, 255}, {40, 44, 40, 255}, DESSIN_ARBRE_MORT, DESSIN_ARBRE_MORT, DESSIN_FEUILLES, DESSIN_OS,
         DESSIN_PIERRE_RUNIQUE);                                                 // le bois des Pendus
    lieu(4, {110, 84, 58, 255}, {100, 76, 52, 255}, DESSIN_TONNEAU, DESSIN_TENTE, DESSIN_OS, DESSIN_TRACES,
         DESSIN_BRASERO);                                                        // le camp de guerre
    lieu(5, {84, 80, 94, 255}, {78, 74, 88, 255}, DESSIN_ROCHER, DESSIN_ROCHER, DESSIN_GRAVATS, DESSIN_CAILLOUX,
         DESSIN_TORCHE);                                                         // la forteresse de Karn
    lieu(6, {78, 82, 96, 255}, {70, 74, 88, 255}, DESSIN_ROCHER, DESSIN_ROCHER, DESSIN_CAILLOUX, DESSIN_NEIGE,
         DESSIN_CRISTAL);                                                        // le col d'Ashka
    lieu(7, {62, 46, 46, 255}, {56, 41, 41, 255}, DESSIN_ROCHE_VOLCANIQUE, DESSIN_ROCHE_VOLCANIQUE, DESSIN_CENDRES,
         DESSIN_CAILLOUX, DESSIN_FISSURE);                                       // les Cendres
    lieu(8, {46, 38, 56, 255}, {42, 34, 52, 255}, DESSIN_ROCHE_VOLCANIQUE, DESSIN_ROCHE_VOLCANIQUE, DESSIN_GRAVATS,
         DESSIN_OS, DESSIN_BRASERO);                                             // la citadelle de Vorgath

    // Les boss : le meme lievre pour Skarn (armure de fer) et la Matriarche (robe verte), et Vorgath a part
    lesSprites.skarn = creerTexture(DESSIN_HASCHEN, {70, 70, 84, 255});
    lesSprites.matriarche = creerTexture(DESSIN_HASCHEN, {48, 104, 66, 255});
    lesSprites.vorgath = creerTexture(DESSIN_VORGATH, aucune);
    lesSprites.casque = creerTexture(DESSIN_CASQUE, aucune);
    lesSprites.ramure = creerTexture(DESSIN_RAMURE, aucune);

    lesSprites.tonneau = creerTexture(DESSIN_TONNEAU, aucune);
    lesSprites.tente = creerTexture(DESSIN_TENTE, aucune);
    lesSprites.rocher = creerTexture(DESSIN_ROCHER, aucune);
    lesSprites.feu[0] = creerTexture(DESSIN_FEU_1, aucune);
    lesSprites.feu[1] = creerTexture(DESSIN_FEU_2, aucune);

    for (int i = 0; i < 9; i++) {
        lesSprites.icones[i] = creerTexture(DESSINS_ICONES[i], aucune);
    }
}

void dechargerSprites() {
    const Texture2D textures[] = {
        lesSprites.aylis, lesSprites.epee, lesSprites.arc, lesSprites.baton, lesSprites.couronne,
        lesSprites.eclaireur, lesSprites.guerrier, lesSprites.traqueur, lesSprites.chaman,
        lesSprites.louvetier, lesSprites.ashka, lesSprites.feu[0], lesSprites.feu[1],
        lesSprites.marchands[0], lesSprites.marchands[1], lesSprites.marchands[2],
        lesSprites.tonneau, lesSprites.tente, lesSprites.rocher,
        lesSprites.skarn, lesSprites.matriarche, lesSprites.vorgath, lesSprites.casque, lesSprites.ramure,
    };
    for (const Texture2D& texture : textures) {
        UnloadTexture(texture);
    }
    for (const Texture2D& icone : lesSprites.icones) {
        UnloadTexture(icone);
    }
    for (int lieu = 0; lieu < NOMBRE_LIEUX; lieu++) {
        UnloadTexture(lesSprites.lumineux[lieu]);
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
    if (contient(nom, "Skarn")) return lesSprites.skarn;
    if (contient(nom, "Matriarche")) return lesSprites.matriarche;
    if (contient(nom, "Vorgath")) return lesSprites.vorgath;
    if (contient(nom, "eclaireur")) return lesSprites.eclaireur;
    if (contient(nom, "traqueur")) return lesSprites.traqueur;
    if (contient(nom, "chaman")) return lesSprites.chaman;
    if (contient(nom, "louvetier")) return lesSprites.louvetier;
    return lesSprites.guerrier;
}

// Ce que porte un boss sur la tete (ou nullptr)
const Texture2D* spriteCoiffe(const std::string& nom) {
    if (contient(nom, "Ashka")) return &lesSprites.couronne;
    if (contient(nom, "Skarn")) return &lesSprites.casque;
    if (contient(nom, "Matriarche")) return &lesSprites.ramure;
    return nullptr;
}

const Texture2D* spriteArmeHaschen(const std::string& nom) {
    if (contient(nom, "Vorgath")) return nullptr;       // Vorgath se bat avec ses griffes de fer
    if (contient(nom, "Skarn") || contient(nom, "brute")) return &lesSprites.epee;
    if (contient(nom, "Matriarche")) return &lesSprites.baton;
    if (contient(nom, "traqueur") || contient(nom, "Ashka")) return &lesSprites.arc;
    if (contient(nom, "chaman")) return &lesSprites.baton;
    if (contient(nom, "louvetier")) return nullptr;     // il se bat avec ses crocs... et son loup
    return &lesSprites.epee;
}
