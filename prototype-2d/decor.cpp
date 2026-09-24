// decor.cpp : le terrain des lieux (les cartes de lieux.cpp, dessinees a l'ecran)
//
// Chaque lettre de la carte devient un morceau de decor. Les petites choses sont des sprites
// (arbres, tonneaux, roseaux...) ; les grandes (maisons, remparts, tours, riviere, ponts) sont dessinees
// avec des rectangles, comme du pixel art geant.
//
// On dessine en deux passes :
//   1. le SOL : l'herbe, les chemins, l'eau, les ponts... (tout ce qui est a plat)
//   2. les CONSTRUCTIONS, ligne par ligne de haut en bas : ce qui est plus bas sur l'ecran passe devant,
//      et les choses hautes (arbres, tours, toits) peuvent deborder sur la case du dessus.
#include <cmath>
#include "jeu2d.h"
#include "sprites.h"

const float T = TAILLE_CASE;    // pour ecrire les tailles plus court

// Un "hasard" fixe pour chaque case : la meme maison a toujours le meme toit
int hasardCase(int colonne, int ligne, int salle) {
    return (colonne * 73 + ligne * 151 + salle * 37 + colonne * ligne * 11) % 97;
}

bool estEau(char lettre) {
    return lettre == '~' || lettre == '=';
}

// Une maison est "brulee" une fois sur trois : le village a ete pille par les Haschen
bool maisonBrulee(int colonne, int ligne, int salle) {
    return hasardCase(colonne, ligne, salle) % 3 == 0;
}

// Le coin en haut a gauche d'une maison ou d'une tente (2x2) : la lettre, mais ni a gauche ni au-dessus
bool estCoinHautGauche(const Jeu& jeu, int c, int l, char lettre) {
    return caseDuTerrain(jeu, c, l) == lettre && caseDuTerrain(jeu, c - 1, l) != lettre
           && caseDuTerrain(jeu, c, l - 1) != lettre;
}

void dessinerSpriteDecor(const Texture2D& texture, Rectangle ecran, Color teinte) {
    DrawTexturePro(texture, {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, ecran, {0, 0}, 0, teinte);
}

// ===================== 1. Le sol =====================

void dessinerChemin(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    Color terre = jeu.lieu == 1 ? Color{132, 106, 72, 255} : Color{112, 96, 74, 255};
    Color bord = {90, 72, 50, 255};
    DrawRectangleRec({x + 12, y + 12, T - 24, T - 24}, terre);
    // Le chemin rejoint les cases voisines qui sont aussi du chemin (ou un pont, une porte)
    // Pour chaque direction : la case voisine, et le morceau de chemin qui va du centre jusqu'au bord
    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const Rectangle morceaux[4] = {
        {x + T / 2, y + 12, T / 2, T - 24},     // vers la droite
        {x, y + 12, T / 2, T - 24},             // vers la gauche
        {x + 12, y + T / 2, T - 24, T / 2},     // vers le bas
        {x + 12, y, T - 24, T / 2},             // vers le haut
    };
    for (int i = 0; i < 4; i++) {
        char voisin = caseDuTerrain(jeu, c + directions[i][0], l + directions[i][1]);
        if (voisin == ',' || voisin == '=' || voisin == 'G') {
            DrawRectangleRec(morceaux[i], terre);
        }
    }
    // Quelques cailloux dans la terre
    int h = hasardCase(c, l, jeu.salle);
    DrawRectangleRec({x + 20 + h % 20, y + 24 + h % 13, 5, 4}, bord);
    DrawRectangleRec({x + 40 + h % 9, y + 44 - h % 11, 4, 3}, bord);
}

Color couleurEau(int lieu) {
    if (lieu == 3) {
        return {32, 56, 52, 255};       // l'eau croupie du bois des Pendus
    }
    return lieu == 0 ? Color{40, 92, 112, 255} : Color{44, 96, 132, 255};
}

void dessinerEau(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    float temps = GetTime();
    Color eau = couleurEau(jeu.lieu);
    DrawRectangleRec({x, y, T, T}, eau);
    // Des vaguelettes qui glissent dans le sens du courant
    for (int i = 0; i < 3; i++) {
        float decalage = std::fmod(temps * 14 + i * 26 + c * 17 + l * 5, T);
        DrawRectangleRec({x + decalage - 8, y + 14 + i * 20, 14, 2}, Fade(WHITE, 0.25f));
    }
    // Un reflet qui scintille
    int h = hasardCase(c, l, jeu.salle);
    float eclat = 0.5f + 0.5f * std::sin(temps * 3 + h);
    DrawRectangleRec({x + 10 + h % 50, y + 8 + h % 45, 3, 3}, Fade(WHITE, 0.6f * eclat));

    // Les berges : une bande de terre et de sable la ou l'eau touche le sol
    Color berge = {126, 112, 78, 255};
    Color bordSombre = {30, 44, 50, 255};
    if (!estEau(caseDuTerrain(jeu, c, l - 1)) && l > 0) {
        DrawRectangleRec({x, y, T, 6}, berge);
        DrawRectangleRec({x, y + 6, T, 2}, bordSombre);
    }
    if (!estEau(caseDuTerrain(jeu, c, l + 1)) && l < LIGNES - 1) {
        DrawRectangleRec({x, y + T - 6, T, 6}, berge);
        DrawRectangleRec({x, y + T - 8, T, 2}, bordSombre);
    }
    if (!estEau(caseDuTerrain(jeu, c - 1, l)) && c > 0) {
        DrawRectangleRec({x, y, 6, T}, berge);
        DrawRectangleRec({x + 6, y, 2, T}, bordSombre);
    }
    if (!estEau(caseDuTerrain(jeu, c + 1, l)) && c < COLONNES - 1) {
        DrawRectangleRec({x + T - 6, y, 6, T}, berge);
        DrawRectangleRec({x + T - 8, y, 2, T}, bordSombre);
    }
}

void dessinerPont(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    dessinerEau(jeu, c, l);
    Color planche = {140, 98, 60, 255};
    Color planche2 = {122, 84, 50, 255};
    Color rampe = {70, 46, 28, 255};
    // La riviere coule de haut en bas s'il y a de l'eau au-dessus ou en dessous : on la traverse de gauche a droite
    bool rivereVerticale = caseDuTerrain(jeu, c, l - 1) == '~' || caseDuTerrain(jeu, c, l + 1) == '~';
    if (rivereVerticale) {
        for (int i = 0; i < 7; i++) {
            DrawRectangleRec({x + i * 10.3f, y + 8, 9, T - 16}, i % 2 ? planche : planche2);
        }
        DrawRectangleRec({x, y + 4, T, 6}, rampe);
        DrawRectangleRec({x, y + T - 10, T, 6}, rampe);
    } else {
        for (int i = 0; i < 7; i++) {
            DrawRectangleRec({x + 8, y + i * 10.3f, T - 16, 9}, i % 2 ? planche : planche2);
        }
        DrawRectangleRec({x + 4, y, 6, T}, rampe);
        DrawRectangleRec({x + T - 10, y, 6, T}, rampe);
    }
}

void dessinerToile(float x, float y) {
    Vector2 centre = {x + T / 2, y + T / 2};
    Color fil = Fade(Color{230, 230, 240, 255}, 0.35f);
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4 + 0.2f;
        DrawLineEx(centre, {centre.x + std::cos(angle) * 34, centre.y + std::sin(angle) * 34}, 1, fil);
    }
    for (int anneau = 1; anneau <= 3; anneau++) {
        DrawPolyLines(centre, 8, anneau * 10.0f, 22.5f, fil);
    }
}

// ===================== 2. Les constructions =====================

void dessinerMaison(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    bool brulee = maisonBrulee(c, l, jeu.salle);
    Color enduit = brulee ? Color{120, 104, 88, 255} : Color{186, 166, 130, 255};
    Color poutre = {68, 44, 28, 255};
    Color chaume = brulee ? Color{62, 50, 46, 255} : Color{160, 116, 60, 255};
    Color chaumeSombre = brulee ? Color{40, 32, 30, 255} : Color{124, 86, 42, 255};

    DrawEllipse(x + T, y + 2 * T - 4, T - 4, 12, Fade(BLACK, 0.35f));
    // Les murs a colombages
    DrawRectangleRec({x + 10, y + 58, 2 * T - 20, 2 * T - 64}, enduit);
    DrawRectangleRec({x + 10, y + 58, 2 * T - 20, 6}, poutre);
    DrawRectangleRec({x + 10, y + 100, 2 * T - 20, 5}, poutre);
    for (float px : {x + 10.0f, x + T - 3, x + 2 * T - 16}) {
        DrawRectangleRec({px, y + 58, 6, 2 * T - 64}, poutre);
    }
    DrawLineEx({x + 16, y + 64}, {x + T - 3, y + 100}, 4, poutre);
    DrawLineEx({x + 2 * T - 16, y + 64}, {x + T + 3, y + 100}, 4, poutre);
    // La porte et les fenetres (allumees, ou noires si la maison a brule)
    DrawRectangleRec({x + T - 14, y + 108, 28, 2 * T - 114}, Color{92, 60, 34, 255});
    DrawRectangleLinesEx({x + T - 14, y + 108, 28, 2 * T - 114}, 2, poutre);
    Color vitre = brulee ? Color{30, 24, 24, 255} : Color{255, 206, 120, 255};
    DrawRectangleRec({x + 26, y + 72, 20, 18}, vitre);
    DrawRectangleRec({x + 2 * T - 46, y + 72, 20, 18}, vitre);
    DrawRectangleLinesEx({x + 26, y + 72, 20, 18}, 2, poutre);
    DrawRectangleLinesEx({x + 2 * T - 46, y + 72, 20, 18}, 2, poutre);

    // Le toit de chaume, en escalier (il deborde sur la case du dessus : la maison a du volume)
    for (int etage = 0; etage < 7; etage++) {
        float retrait = etage * 10.0f;
        DrawRectangleRec({x - 2 + retrait, y + 58 - (etage + 1) * 10, 2 * T + 4 - 2 * retrait, 11},
                         etage % 2 ? chaume : chaumeSombre);
    }
    if (brulee) {
        // Un trou dans le toit, et des braises
        DrawRectangleRec({x + 40, y + 18, 26, 20}, Color{20, 14, 14, 255});
        DrawRectangleRec({x + 46, y + 30, 4, 4}, ORANGE);
        DrawRectangleRec({x + 56, y + 24, 3, 3}, Color{255, 200, 80, 255});
    } else {
        // Une cheminee
        DrawRectangleRec({x + 2 * T - 44, y - 12, 14, 26}, Color{110, 100, 100, 255});
        DrawRectangleRec({x + 2 * T - 46, y - 14, 18, 5}, Color{80, 72, 72, 255});
    }
}

void dessinerTente(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    DrawEllipse(x + T, y + 2 * T - 6, T - 8, 12, Fade(BLACK, 0.35f));
    dessinerSpriteDecor(sprites().tente, {x - 6, y - 14, 2 * T + 12, 2 * T + 12}, WHITE);
}

bool estRempart(char lettre) {
    return lettre == '#' || lettre == 'I' || lettre == 'G';
}

void dessinerMur(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    Color pierre = {112, 106, 126, 255};
    Color joint = {76, 70, 88, 255};
    bool murAuDessus = estRempart(caseDuTerrain(jeu, c, l - 1));
    float haut = murAuDessus ? y : y + 10;
    DrawRectangleRec({x, haut, T, y + T - haut}, pierre);
    // Les rangees de pierres, decalees d'une rangee a l'autre
    for (int rang = 0; rang * 14 < T; rang++) {
        float ry = haut + rang * 14;
        if (ry > y + T) {
            break;
        }
        DrawRectangleRec({x, ry, T, 2}, joint);
        for (int i = 0; i < 4; i++) {
            float jx = x + i * 24 + (rang % 2) * 12;
            if (jx < x + T) {
                DrawRectangleRec({jx, ry, 2, 14}, joint);
            }
        }
    }
    // Les creneaux, seulement en haut du rempart
    if (!murAuDessus) {
        for (int i = 0; i < 3; i++) {
            DrawRectangleRec({x + 4 + i * 24, y - 6, 16, 18}, pierre);
            DrawRectangleLinesEx({x + 4 + i * 24, y - 6, 16, 18}, 2, joint);
        }
    }
    // De la mousse, et parfois une banniere haschen
    int h = hasardCase(c, l, jeu.salle);
    DrawRectangleRec({x + h % 50, y + T - 10, 14, 6}, Color{70, 104, 70, 255});
    if (h % 5 == 0 && !estRempart(caseDuTerrain(jeu, c, l + 1))) {
        DrawRectangleRec({x + 24, y + 14, 24, 40}, Color{140, 26, 40, 255});
        DrawTriangle({x + 24, y + 54}, {x + 36, y + 46}, {x + 48, y + 54}, pierre);
        DrawRectangleRec({x + 32, y + 24, 8, 8}, GOLD);
        DrawRectangleRec({x + 22, y + 12, 28, 3}, Color{70, 46, 28, 255});
    }
    // Le bas du mur, plus sombre (le mur touche le sol)
    DrawRectangleRec({x, y + T - 4, T, 4}, joint);
}

void dessinerTour(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    Color pierre = {126, 120, 140, 255};
    Color ombre = {96, 90, 110, 255};
    Color joint = {76, 70, 88, 255};
    DrawEllipse(x + T / 2, y + T - 2, T / 2, 10, Fade(BLACK, 0.4f));
    DrawRectangleRec({x + 2, y - 30, T - 4, T + 30}, pierre);
    DrawRectangleRec({x + T - 20, y - 30, 18, T + 30}, ombre);      // le cote dans l'ombre : du volume
    for (int rang = 0; rang < 7; rang++) {
        DrawRectangleRec({x + 2, y - 30 + rang * 15, T - 4, 2}, joint);
    }
    // Les creneaux du sommet, qui debordent encore plus haut
    for (int i = 0; i < 4; i++) {
        DrawRectangleRec({x + i * 18, y - 44, 14, 18}, pierre);
        DrawRectangleLinesEx({x + i * 18, y - 44, 14, 18}, 2, joint);
    }
    // Une meurtriere
    DrawRectangleRec({x + 30, y - 6, 8, 24}, Color{24, 20, 30, 255});
    DrawRectangleRec({x + 26, y + 2, 16, 6}, Color{24, 20, 30, 255});
}

void dessinerPorte(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    Color pierre = {126, 120, 140, 255};
    Color joint = {76, 70, 88, 255};
    // Le passage : des dalles sombres
    DrawRectangleRec({x + 14, y, T - 28, T}, Color{60, 56, 70, 255});
    for (int i = 0; i < 4; i++) {
        DrawRectangleRec({x + 14, y + i * 18, T - 28, 2}, joint);
    }
    // Les deux piliers et l'arche, avec la herse relevee
    bool verticale = estRempart(caseDuTerrain(jeu, c, l - 1)) || estRempart(caseDuTerrain(jeu, c, l + 1));
    if (!verticale) {
        DrawRectangleRec({x, y - 22, 14, T + 22}, pierre);
        DrawRectangleRec({x + T - 14, y - 22, 14, T + 22}, pierre);
        DrawRectangleRec({x, y - 30, T, 16}, pierre);
        DrawRectangleLinesEx({x, y - 30, T, 16}, 2, joint);
        for (int i = 0; i < 5; i++) {
            DrawRectangleRec({x + 18 + i * 9, y - 14, 3, 12}, Color{60, 60, 66, 255});
        }
    } else {
        DrawRectangleRec({x, y, T, 12}, pierre);
        DrawRectangleRec({x, y + T - 12, T, 12}, pierre);
    }
}

void dessinerCloture(const Jeu& jeu, int c, int l) {
    float x = c * T;
    float y = l * T;
    bool gauche = caseDuTerrain(jeu, c - 1, l) == 'f';
    bool droite = caseDuTerrain(jeu, c + 1, l) == 'f';
    bool haut = caseDuTerrain(jeu, c, l - 1) == 'f';
    bool bas = caseDuTerrain(jeu, c, l + 1) == 'f';
    bool horizontale = gauche || droite || !(haut || bas);
    if (jeu.lieu == 4) {
        // Au camp : une palissade de rondins tailles en pointe
        Color bois = {122, 86, 52, 255};
        Color boisSombre = {84, 58, 34, 255};
        if (horizontale) {
            for (int i = 0; i < 5; i++) {
                float bx = x + 1 + i * 14.2f;
                DrawRectangleRec({bx, y + 8, 12, T - 12}, i % 2 ? bois : boisSombre);
                DrawTriangle({bx, y + 8}, {bx + 12, y + 8}, {bx + 6, y - 4}, i % 2 ? bois : boisSombre);
            }
            DrawRectangleRec({x, y + 30, T, 4}, Color{60, 40, 24, 255});
        } else {
            for (int i = 0; i < 5; i++) {
                DrawRectangleRec({x + 24, y + i * 14.2f, 24, 12}, i % 2 ? bois : boisSombre);
            }
        }
    } else {
        // Au village : une barriere de bois
        Color bois = {140, 104, 66, 255};
        Color poteau = {96, 66, 40, 255};
        if (horizontale) {
            DrawRectangleRec({x, y + 30, T, 6}, bois);
            DrawRectangleRec({x, y + 48, T, 6}, bois);
            DrawRectangleRec({x + 6, y + 22, 8, 40}, poteau);
            DrawRectangleRec({x + T - 14, y + 22, 8, 40}, poteau);
        } else {
            DrawRectangleRec({x + 28, y, 6, T}, bois);
            DrawRectangleRec({x + 42, y, 6, T}, bois);
            DrawRectangleRec({x + 24, y + 8, 28, 8}, poteau);
            DrawRectangleRec({x + 24, y + T - 16, 28, 8}, poteau);
        }
    }
}

void dessinerPuits(float x, float y) {
    DrawEllipse(x + T / 2, y + T - 8, 30, 10, Fade(BLACK, 0.35f));
    DrawEllipse(x + T / 2, y + T / 2 + 10, 28, 16, Color{120, 116, 124, 255});
    DrawEllipse(x + T / 2, y + T / 2 + 8, 20, 10, Color{28, 50, 70, 255});
    DrawRectangleRec({x + 10, y + 6, 6, 40}, Color{96, 66, 40, 255});
    DrawRectangleRec({x + T - 16, y + 6, 6, 40}, Color{96, 66, 40, 255});
    DrawTriangle({x + 2, y + 10}, {x + T - 2, y + 10}, {x + T / 2, y - 12}, Color{120, 80, 44, 255});
    DrawRectangleRec({x + T / 2 - 1, y + 10, 2, 16}, Color{60, 50, 40, 255});
    DrawRectangleRec({x + T / 2 - 6, y + 24, 12, 10}, Color{110, 76, 44, 255});
}

void dessinerCercleDePierres(float x, float y) {
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        DrawCircle(x + T / 2 + std::cos(angle) * 24, y + T / 2 + 8 + std::sin(angle) * 12, 6, Color{90, 86, 96, 255});
    }
}

// ===================== Le terrain complet =====================

void dessinerTerrain(const Jeu& jeu) {
    const Sprites& s = sprites();
    int lieu = jeu.lieu;

    // ----- 1. Le sol -----
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            Rectangle caseEcran = {c * T, l * T, T, T};
            DrawTexturePro(s.sol[lieu][(c + l) % 2], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, caseEcran, {0, 0}, 0, WHITE);
        }
    }
    // De grandes taches sur le sol (mousse, cendre, neige...) : le sol n'est plus un simple damier
    const Color taches[NOMBRE_LIEUX][2] = {
        {{40, 120, 70, 255}, {25, 45, 35, 255}},        // foret : mousse et terre sombre
        {{130, 120, 70, 255}, {70, 60, 44, 255}},       // village : herbe seche et terre
        {{50, 130, 90, 255}, {90, 110, 70, 255}},       // gue : herbe grasse
        {{20, 24, 22, 255}, {60, 50, 40, 255}},         // bois : terre noire
        {{60, 50, 50, 255}, {140, 90, 50, 255}},        // camp : cendre et terre battue
        {{60, 58, 70, 255}, {110, 104, 120, 255}},      // forteresse : dalles sales
        {{210, 220, 240, 255}, {90, 100, 150, 255}},    // col : neige et glace
    };
    for (int i = 0; i < 14; i++) {
        float x = (float)(hasardCase(i, 3, jeu.salle) * 9 % LARGEUR_FENETRE);
        float y = (float)(hasardCase(3, i, jeu.salle) * 6 % HAUTEUR_ARENE);
        float largeur = 60.0f + hasardCase(i, i, jeu.salle) % 80;
        DrawEllipse(x, y, largeur, largeur * 0.55f, Fade(taches[lieu][i % 2], 0.22f));
    }
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            char lettre = caseDuTerrain(jeu, c, l);
            float x = c * T;
            float y = l * T;
            int h = hasardCase(c, l, jeu.salle);
            switch (lettre) {
                case ',': dessinerChemin(jeu, c, l); break;
                case '~': dessinerEau(jeu, c, l); break;
                case '=': dessinerPont(jeu, c, l); break;
                case 'w': dessinerToile(x, y); break;
                case 'x': dessinerSpriteDecor(s.decor[5][0], {x, y, T, T}, WHITE); break;
                case 'g': dessinerSpriteDecor(s.decor[lieu][0], {x, y - 6, T, T}, WHITE); break;
                case 'F': dessinerCercleDePierres(x, y); break;
                case '.':
                    if (h % 11 < (lieu == 5 ? 1 : 3)) {   // quelques petits details par terre (moins dans la forteresse)
                        dessinerSpriteDecor(s.decor[lieu][h % 2], {x + (h % 5 - 2) * 5.0f, y + (h % 3) * 6.0f, T, T}, WHITE);
                    }
                    break;
                default: break;
            }
        }
    }

    // ----- 2. Les constructions, ligne par ligne de haut en bas -----
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            char lettre = caseDuTerrain(jeu, c, l);
            float x = c * T;
            float y = l * T;
            int h = hasardCase(c, l, jeu.salle);
            switch (lettre) {
                case 'T':
                    // Les arbres sont plus grands qu'une case : ils debordent vers le haut
                    DrawEllipse(x + T / 2, y + T - 6, 26, 8, Fade(BLACK, 0.35f));
                    dessinerSpriteDecor(s.obstacle[lieu][h % 2], {x - 10, y - 30, T + 20, T + 26}, WHITE);
                    break;
                case 'r':
                    DrawEllipse(x + T / 2, y + T - 8, 28, 8, Fade(BLACK, 0.3f));
                    dessinerSpriteDecor(s.rocher, {x, y + 2, T, T}, WHITE);
                    break;
                case 'b':
                    dessinerSpriteDecor(s.tonneau, {x + 4, y + 2, T - 8, T - 8}, WHITE);
                    break;
                case 'H':
                    if (estCoinHautGauche(jeu, c, l, 'H')) {
                        dessinerMaison(jeu, c, l);
                    }
                    break;
                case 'A':
                    if (estCoinHautGauche(jeu, c, l, 'A')) {
                        dessinerTente(jeu, c, l);
                    }
                    break;
                case '#': dessinerMur(jeu, c, l); break;
                case 'I': dessinerTour(jeu, c, l); break;
                case 'G': dessinerPorte(jeu, c, l); break;
                case 'f': dessinerCloture(jeu, c, l); break;
                case 'o': dessinerPuits(x, y); break;
                default: break;
            }
        }
    }
}

// Les lumieres du decor, ajoutees a la carte de lumiere (les fenetres allumees du village)
void ajouterLumieresDuDecor(const Jeu& jeu) {
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            if (estCoinHautGauche(jeu, c, l, 'H') && !maisonBrulee(c, l, jeu.salle)) {
                float x = c * T;
                float y = l * T;
                Color chaud = {255, 190, 110, 255};
                DrawCircleGradient(x + 36, y + 90, 70, Fade(chaud, 0.45f), Fade(chaud, 0.0f));
                DrawCircleGradient(x + 2 * T - 36, y + 90, 70, Fade(chaud, 0.45f), Fade(chaud, 0.0f));
            }
        }
    }
}

// Les maisons brulees : pour la fumee (dessin.cpp)
std::vector<Vector2> cheminsDeFumee(const Jeu& jeu) {
    std::vector<Vector2> fumees;
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            if (estCoinHautGauche(jeu, c, l, 'H') && maisonBrulee(c, l, jeu.salle)) {
                fumees.push_back({c * T + 53, l * T + 20});
            }
        }
    }
    return fumees;
}
