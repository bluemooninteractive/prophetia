// PROPHETIA 2D - prototype en vue du dessus, au tour par tour
//
// Une arene en cases. A ton tour :
//   1. clique sur une case en surbrillance pour deplacer AYLIS (3 cases maximum),
//   2. puis clique sur un Haschen a cote de toi pour l'attaquer,
//      ou appuie sur ESPACE pour finir ton tour sans attaquer.
// Ensuite, chaque Haschen s'approche et attaque s'il est a cote d'AYLIS.
//
// raylib fonctionne avec une "boucle de jeu" : environ 60 fois par seconde,
// on lit la souris et le clavier, on met a jour le jeu, puis on redessine tout l'ecran.
#include <string>
#include <vector>
#include <cstdlib>
#include "raylib.h"

// ===================== Les reglages =====================

const int COLONNES = 12;
const int LIGNES = 8;
const int TAILLE_CASE = 72;                         // en pixels
const int LARGEUR_FENETRE = COLONNES * TAILLE_CASE;
const int HAUTEUR_ARENE = LIGNES * TAILLE_CASE;
const int HAUTEUR_PANNEAU = 150;                    // la zone de texte en bas
const int HAUTEUR_FENETRE = HAUTEUR_ARENE + HAUTEUR_PANNEAU;

const int DEPLACEMENT_AYLIS = 3;                    // cases par tour
const int DEPLACEMENT_HASCHEN = 2;
const float PAUSE_ENTRE_ENNEMIS = 0.5f;             // en secondes, pour voir chaque Haschen jouer

// ===================== Les donnees =====================

// Une unite sur l'arene : AYLIS ou un Haschen
struct Unite {
    std::string nom;
    int colonne;
    int ligne;
    int pv;
    int pvMax;
    int attaque;
    int defense;
    Color couleur;

    bool estDebout() const {
        return pv > 0;
    }
};

// Un nombre qui s'envole au-dessus d'une unite touchee (les degats), puis disparait
struct TexteFlottant {
    std::string texte;
    float x;
    float y;
    float tempsRestant;
    Color couleur;
};

// Les phases d'un tour
enum class Phase {
    Deplacement,    // AYLIS peut bouger (ou attaquer directement si un Haschen est deja a cote)
    Action,         // AYLIS a bouge : elle peut encore attaquer, ou finir son tour
    TourEnnemi,     // les Haschen jouent, un par un
    Victoire,
    Defaite,
};

// Tout l'etat du jeu
struct Jeu {
    Unite aylis;
    std::vector<Unite> haschen;
    std::vector<bool> rochers;          // une case par position : true = rocher infranchissable
    Phase phase = Phase::Deplacement;
    int ennemiQuiJoue = 0;              // pendant le tour ennemi : quel Haschen joue
    float minuteur = 0.0f;
    int tour = 1;
    std::vector<std::string> journal;   // les derniers messages
    std::vector<TexteFlottant> textes;
};

// ===================== Les petites fonctions utiles =====================

// La distance en cases, sans diagonales ("distance de Manhattan")
int distance(int c1, int l1, int c2, int l2) {
    return std::abs(c1 - c2) + std::abs(l1 - l2);
}

bool estDansArene(int colonne, int ligne) {
    return colonne >= 0 && colonne < COLONNES && ligne >= 0 && ligne < LIGNES;
}

bool estRocher(const Jeu& jeu, int colonne, int ligne) {
    return jeu.rochers[ligne * COLONNES + colonne];
}

// Le Haschen debout sur cette case, ou -1 s'il n'y en a pas
int haschenSurCase(const Jeu& jeu, int colonne, int ligne) {
    int nombre = jeu.haschen.size();
    for (int i = 0; i < nombre; i++) {
        const Unite& h = jeu.haschen[i];
        if (h.estDebout() && h.colonne == colonne && h.ligne == ligne) {
            return i;
        }
    }
    return -1;
}

// Est-ce qu'on peut se poser sur cette case ? (dans l'arene, pas de rocher, personne dessus)
bool caseLibre(const Jeu& jeu, int colonne, int ligne) {
    if (!estDansArene(colonne, ligne) || estRocher(jeu, colonne, ligne)) {
        return false;
    }
    if (jeu.aylis.colonne == colonne && jeu.aylis.ligne == ligne) {
        return false;
    }
    return haschenSurCase(jeu, colonne, ligne) == -1;
}

void ecrireJournal(Jeu& jeu, const std::string& message) {
    jeu.journal.push_back(message);
    if (jeu.journal.size() > 4) {
        jeu.journal.erase(jeu.journal.begin());
    }
}

// Le centre d'une case, en pixels
float centreX(int colonne) {
    return colonne * TAILLE_CASE + TAILLE_CASE / 2.0f;
}

float centreY(int ligne) {
    return ligne * TAILLE_CASE + TAILLE_CASE / 2.0f;
}

// Les memes regles que dans le jeu console : attaque - defense, un peu de hasard, et parfois un critique
void attaquer(Jeu& jeu, const Unite& attaquant, Unite& cible) {
    int degats = attaquant.attaque - cible.defense + GetRandomValue(-1, 1);
    bool critique = GetRandomValue(1, 100) <= 10;
    if (critique) {
        degats = degats * 2;
    }
    if (degats < 1) {
        degats = 1;
    }
    cible.pv = cible.pv - degats;

    std::string message = attaquant.nom + " frappe " + cible.nom + " : -" + std::to_string(degats) + " pv";
    if (critique) {
        message = message + " (CRITIQUE !)";
    }
    ecrireJournal(jeu, message);

    Color couleur = cible.nom == "AYLIS" ? RED : YELLOW;
    jeu.textes.push_back({"-" + std::to_string(degats), centreX(cible.colonne) - 10, centreY(cible.ligne) - 30, 1.0f, couleur});

    if (!cible.estDebout()) {
        ecrireJournal(jeu, cible.nom + " tombe !");
    }
}

// ===================== Preparer une partie =====================

void nouvellePartie(Jeu& jeu) {
    //                nom                  col lig pv  pvMax att def couleur
    jeu.aylis =      {"AYLIS",              1,  3, 40, 40,   12,  4, SKYBLUE};
    jeu.haschen = {
        {"Haschen eclaireur",              9,  1, 18, 18,    8,  1, ORANGE},
        {"Haschen guerrier",              10,  4, 22, 22,    9,  2, RED},
        {"Haschen chaman",                 9,  6, 20, 20,   10,  1, PURPLE},
    };

    // Quelques rochers pour que le placement compte
    jeu.rochers.assign(COLONNES * LIGNES, false);
    const int rochers[][2] = {{5, 1}, {5, 2}, {6, 5}, {6, 6}, {3, 5}, {8, 3}};
    for (const auto& rocher : rochers) {
        jeu.rochers[rocher[1] * COLONNES + rocher[0]] = true;
    }

    jeu.phase = Phase::Deplacement;
    jeu.tour = 1;
    jeu.journal.clear();
    jeu.textes.clear();
    ecrireJournal(jeu, "Les Haschen approchent ! Clique sur une case bleue pour deplacer AYLIS.");
}

// ===================== Le tour d'AYLIS =====================

bool haschenAdjacent(const Jeu& jeu, int indexHaschen) {
    const Unite& h = jeu.haschen[indexHaschen];
    return h.estDebout() && distance(h.colonne, h.ligne, jeu.aylis.colonne, jeu.aylis.ligne) == 1;
}

bool resteDesHaschen(const Jeu& jeu) {
    for (const Unite& h : jeu.haschen) {
        if (h.estDebout()) {
            return true;
        }
    }
    return false;
}

void commencerTourEnnemi(Jeu& jeu) {
    if (!resteDesHaschen(jeu)) {
        jeu.phase = Phase::Victoire;
        ecrireJournal(jeu, "Tous les Haschen sont vaincus !");
        return;
    }
    jeu.phase = Phase::TourEnnemi;
    jeu.ennemiQuiJoue = 0;
    jeu.minuteur = 0.0f;
}

// Un clic pendant le tour d'AYLIS
void clicJoueur(Jeu& jeu, int colonne, int ligne) {
    // Clic sur un Haschen a cote : attaque (possible avant ou apres le deplacement)
    int cible = haschenSurCase(jeu, colonne, ligne);
    if (cible != -1) {
        if (haschenAdjacent(jeu, cible)) {
            attaquer(jeu, jeu.aylis, jeu.haschen[cible]);
            commencerTourEnnemi(jeu);
        } else {
            ecrireJournal(jeu, "Trop loin ! Il faut etre juste a cote pour frapper.");
        }
        return;
    }

    // Clic sur une case : deplacement (une seule fois par tour)
    if (jeu.phase != Phase::Deplacement) {
        return;
    }
    int d = distance(jeu.aylis.colonne, jeu.aylis.ligne, colonne, ligne);
    if (d >= 1 && d <= DEPLACEMENT_AYLIS && caseLibre(jeu, colonne, ligne)) {
        jeu.aylis.colonne = colonne;
        jeu.aylis.ligne = ligne;
        jeu.phase = Phase::Action;
        ecrireJournal(jeu, "AYLIS se deplace. Attaque un Haschen voisin, ou ESPACE pour finir le tour.");
    }
}

// ===================== Le tour des Haschen =====================

// Un Haschen avance d'une case vers AYLIS, si une case libre le rapproche
void avancerVersAylis(Jeu& jeu, Unite& h) {
    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int meilleureDistance = distance(h.colonne, h.ligne, jeu.aylis.colonne, jeu.aylis.ligne);
    int meilleureColonne = h.colonne;
    int meilleureLigne = h.ligne;

    for (const auto& dir : directions) {
        int c = h.colonne + dir[0];
        int l = h.ligne + dir[1];
        if (caseLibre(jeu, c, l)) {
            int d = distance(c, l, jeu.aylis.colonne, jeu.aylis.ligne);
            if (d < meilleureDistance) {
                meilleureDistance = d;
                meilleureColonne = c;
                meilleureLigne = l;
            }
        }
    }
    h.colonne = meilleureColonne;
    h.ligne = meilleureLigne;
}

void jouerHaschen(Jeu& jeu, Unite& h) {
    // Il avance jusqu'a 2 cases, puis frappe s'il est a cote d'AYLIS
    for (int pas = 0; pas < DEPLACEMENT_HASCHEN; pas++) {
        if (distance(h.colonne, h.ligne, jeu.aylis.colonne, jeu.aylis.ligne) == 1) {
            break;
        }
        avancerVersAylis(jeu, h);
    }
    if (distance(h.colonne, h.ligne, jeu.aylis.colonne, jeu.aylis.ligne) == 1) {
        attaquer(jeu, h, jeu.aylis);
    }
}

// Appelee a chaque image pendant le tour ennemi : un Haschen joue toutes les PAUSE_ENTRE_ENNEMIS secondes
void mettreAJourTourEnnemi(Jeu& jeu, float secondes) {
    jeu.minuteur = jeu.minuteur + secondes;
    if (jeu.minuteur < PAUSE_ENTRE_ENNEMIS) {
        return;
    }
    jeu.minuteur = 0.0f;

    // On saute les Haschen deja tombes
    int nombre = jeu.haschen.size();
    while (jeu.ennemiQuiJoue < nombre && !jeu.haschen[jeu.ennemiQuiJoue].estDebout()) {
        jeu.ennemiQuiJoue = jeu.ennemiQuiJoue + 1;
    }

    if (jeu.ennemiQuiJoue >= nombre) {
        // Tout le monde a joue : c'est de nouveau a AYLIS
        jeu.phase = Phase::Deplacement;
        jeu.tour = jeu.tour + 1;
        ecrireJournal(jeu, "Tour " + std::to_string(jeu.tour) + " : a toi de jouer !");
        return;
    }

    jouerHaschen(jeu, jeu.haschen[jeu.ennemiQuiJoue]);
    jeu.ennemiQuiJoue = jeu.ennemiQuiJoue + 1;

    if (!jeu.aylis.estDebout()) {
        jeu.phase = Phase::Defaite;
    }
}

// ===================== Le dessin =====================

void dessinerBarreDeVie(const Unite& u) {
    int x = u.colonne * TAILLE_CASE + 8;
    int y = u.ligne * TAILLE_CASE + 4;
    int largeur = TAILLE_CASE - 16;
    int remplie = largeur * u.pv / u.pvMax;
    DrawRectangle(x, y, largeur, 6, DARKGRAY);
    Color couleur = GREEN;
    if (u.pv * 4 <= u.pvMax) {
        couleur = RED;
    } else if (u.pv * 2 <= u.pvMax) {
        couleur = YELLOW;
    }
    DrawRectangle(x, y, remplie, 6, couleur);
}

void dessinerUnite(const Unite& u, bool estAylis) {
    float x = centreX(u.colonne);
    float y = centreY(u.ligne) + 4;
    if (estAylis) {
        DrawCircle(x, y, TAILLE_CASE * 0.32f, u.couleur);
        DrawCircleLines(x, y, TAILLE_CASE * 0.32f, WHITE);
    } else {
        float cote = TAILLE_CASE * 0.56f;
        DrawRectangle(x - cote / 2, y - cote / 2, cote, cote, u.couleur);
        DrawRectangleLines(x - cote / 2, y - cote / 2, cote, cote, BLACK);
    }
    dessinerBarreDeVie(u);
}

void dessiner(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    ClearBackground(Color{20, 18, 28, 255});

    bool tourJoueur = jeu.phase == Phase::Deplacement || jeu.phase == Phase::Action;

    // Les cases : un damier sombre, les rochers, et les cases ou AYLIS peut aller
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            int x = c * TAILLE_CASE;
            int y = l * TAILLE_CASE;
            Color fond = (c + l) % 2 == 0 ? Color{48, 44, 60, 255} : Color{40, 37, 51, 255};
            DrawRectangle(x, y, TAILLE_CASE, TAILLE_CASE, fond);

            if (estRocher(jeu, c, l)) {
                DrawRectangle(x + 6, y + 6, TAILLE_CASE - 12, TAILLE_CASE - 12, Color{100, 94, 88, 255});
                continue;
            }

            int d = distance(jeu.aylis.colonne, jeu.aylis.ligne, c, l);
            if (jeu.phase == Phase::Deplacement && d >= 1 && d <= DEPLACEMENT_AYLIS && caseLibre(jeu, c, l)) {
                DrawRectangle(x + 2, y + 2, TAILLE_CASE - 4, TAILLE_CASE - 4, Fade(SKYBLUE, 0.25f));
            }
        }
    }

    // Les Haschen qu'AYLIS peut frapper : un cadre rouge
    int nombre = jeu.haschen.size();
    for (int i = 0; i < nombre; i++) {
        if (tourJoueur && haschenAdjacent(jeu, i)) {
            const Unite& h = jeu.haschen[i];
            DrawRectangleLinesEx({(float)h.colonne * TAILLE_CASE, (float)h.ligne * TAILLE_CASE,
                                  (float)TAILLE_CASE, (float)TAILLE_CASE}, 3, RED);
        }
    }

    // La case sous la souris
    if (tourJoueur && estDansArene(colonneSouris, ligneSouris)) {
        DrawRectangleLines(colonneSouris * TAILLE_CASE, ligneSouris * TAILLE_CASE, TAILLE_CASE, TAILLE_CASE, WHITE);
    }

    // Les unites
    for (const Unite& h : jeu.haschen) {
        if (h.estDebout()) {
            dessinerUnite(h, false);
        }
    }
    if (jeu.aylis.estDebout()) {
        dessinerUnite(jeu.aylis, true);
    }

    // Les degats qui s'envolent
    for (const TexteFlottant& t : jeu.textes) {
        DrawText(t.texte.c_str(), t.x, t.y, 26, Fade(t.couleur, t.tempsRestant));
    }

    // Le panneau du bas
    int yPanneau = HAUTEUR_ARENE;
    DrawRectangle(0, yPanneau, LARGEUR_FENETRE, HAUTEUR_PANNEAU, Color{14, 12, 20, 255});
    DrawText(TextFormat("AYLIS  %i / %i pv", jeu.aylis.pv, jeu.aylis.pvMax), 16, yPanneau + 12, 22, SKYBLUE);
    DrawText(TextFormat("Tour %i", jeu.tour), LARGEUR_FENETRE - 110, yPanneau + 12, 22, LIGHTGRAY);

    std::string consigne = "";
    if (jeu.phase == Phase::Deplacement) {
        consigne = "Ton tour : clique une case bleue pour bouger, ou un Haschen voisin pour frapper.";
    } else if (jeu.phase == Phase::Action) {
        consigne = "Attaque un Haschen encadre de rouge, ou ESPACE pour finir le tour.";
    } else if (jeu.phase == Phase::TourEnnemi) {
        consigne = "Les Haschen jouent...";
    }
    DrawText(consigne.c_str(), 16, yPanneau + 40, 18, GOLD);

    int yJournal = yPanneau + 66;
    for (const std::string& ligne : jeu.journal) {
        DrawText(ligne.c_str(), 16, yJournal, 16, LIGHTGRAY);
        yJournal = yJournal + 20;
    }

    // Victoire ou defaite : un grand message au milieu
    if (jeu.phase == Phase::Victoire || jeu.phase == Phase::Defaite) {
        DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_ARENE, Fade(BLACK, 0.6f));
        const char* titre = jeu.phase == Phase::Victoire ? "VICTOIRE !" : "AYLIS TOMBE AU COMBAT...";
        Color couleur = jeu.phase == Phase::Victoire ? GREEN : RED;
        int largeurTitre = MeasureText(titre, 56);
        DrawText(titre, (LARGEUR_FENETRE - largeurTitre) / 2, HAUTEUR_ARENE / 2 - 50, 56, couleur);
        const char* aide = "Appuie sur R pour rejouer";
        DrawText(aide, (LARGEUR_FENETRE - MeasureText(aide, 24)) / 2, HAUTEUR_ARENE / 2 + 20, 24, WHITE);
    }
}

// ===================== La boucle de jeu =====================

int main() {
    // Sur les ecrans "agrandis" par Windows (125%, 150%, 200%...), le jeu occupe bien toute la fenetre
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(LARGEUR_FENETRE, HAUTEUR_FENETRE, "PROPHETIA - prototype 2D");
    SetTargetFPS(60);

    Jeu jeu;
    nouvellePartie(jeu);

    while (!WindowShouldClose()) {
        float secondes = GetFrameTime();    // le temps ecoule depuis l'image precedente

        // 1. Lire la souris et le clavier
        Vector2 souris = GetMousePosition();
        int colonneSouris = (int)(souris.x / TAILLE_CASE);
        int ligneSouris = (int)(souris.y / TAILLE_CASE);

        bool tourJoueur = jeu.phase == Phase::Deplacement || jeu.phase == Phase::Action;
        if (tourJoueur && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && estDansArene(colonneSouris, ligneSouris)) {
            clicJoueur(jeu, colonneSouris, ligneSouris);
        }
        if (jeu.phase == Phase::Action && IsKeyPressed(KEY_SPACE)) {
            ecrireJournal(jeu, "AYLIS attend.");
            commencerTourEnnemi(jeu);
        }
        if ((jeu.phase == Phase::Victoire || jeu.phase == Phase::Defaite) && IsKeyPressed(KEY_R)) {
            nouvellePartie(jeu);
        }

        // 2. Mettre le jeu a jour
        if (jeu.phase == Phase::TourEnnemi) {
            mettreAJourTourEnnemi(jeu, secondes);
        }
        for (TexteFlottant& t : jeu.textes) {
            t.y = t.y - 40 * secondes;          // le texte monte...
            t.tempsRestant = t.tempsRestant - secondes;    // ... et s'efface
        }
        while (!jeu.textes.empty() && jeu.textes.front().tempsRestant <= 0) {
            jeu.textes.erase(jeu.textes.begin());
        }

        // 3. Tout redessiner
        BeginDrawing();
        dessiner(jeu, colonneSouris, ligneSouris);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
