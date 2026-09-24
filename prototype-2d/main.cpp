// VESPERANCE 2D - prototype en vue du dessus, au tour par tour
//
// main.cpp : la boucle de jeu et les commandes (souris et clavier).
// Les regles sont dans regles.cpp, l'affichage dans dessin.cpp,
// et les structures (Combattant, Arme, Jauge...) viennent du jeu console (dossier du dessus).
//
// raylib fonctionne avec une "boucle de jeu" : environ 60 fois par seconde,
// on lit la souris et le clavier, on met a jour le jeu, puis on redessine tout l'ecran.
#include "raylib.h"
#include "jeu2d.h"
#include "sprites.h"
#include "animations.h"

// Choisit une action de la barre (numero 0 a 8)
void selectionnerAction(Jeu& jeu, int numero) {
    Action action = actionsDeLaBarre()[numero];
    if (!actionDisponible(jeu, action)) {
        ecrireJournal(jeu, nomAction(action) + " : pas disponible (sort pas appris, mana, potions ou rage).");
        return;
    }
    jeu.actionChoisie = action;
    if (actionSurSoi(action)) {
        ecrireJournal(jeu, nomAction(action) + " : clique sur AYLIS (ou ENTREE) pour l'utiliser.");
    }
}

// Les commandes pendant le tour d'AYLIS
void commandesJoueur(Jeu& jeu, int colonneSouris, int ligneSouris) {
    // Les touches 1 a 9 choisissent une action
    for (int numero = 0; numero < 9; numero++) {
        if (IsKeyPressed(KEY_ONE + numero) || IsKeyPressed(KEY_KP_1 + numero)) {
            selectionnerAction(jeu, numero);
        }
    }
    if (IsKeyPressed(KEY_ENTER) && actionSurSoi(jeu.actionChoisie)) {
        agirSurSoi(jeu);
        return;
    }
    if (IsKeyPressed(KEY_SPACE)) {
        ecrireJournal(jeu, "AYLIS attend.");
        finirTourAylis(jeu);
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    // Un clic sur un bouton de la barre
    Vector2 souris = GetMousePosition();
    int nombre = actionsDeLaBarre().size();
    for (int numero = 0; numero < nombre; numero++) {
        if (CheckCollisionPointRec(souris, rectangleBouton(numero))) {
            selectionnerAction(jeu, numero);
            return;
        }
    }

    if (!estDansArene(colonneSouris, ligneSouris)) {
        return;
    }

    // Un clic sur un Haschen : l'action choisie
    int cible = haschenSurCase(jeu, colonneSouris, ligneSouris);
    if (cible != -1) {
        agirSurHaschen(jeu, cible);
        return;
    }
    // Un clic sur AYLIS : potion, soin ou bouclier
    if (colonneSouris == jeu.aylis.colonne && ligneSouris == jeu.aylis.ligne) {
        agirSurSoi(jeu);
        return;
    }
    // Sinon : un deplacement
    deplacerAylis(jeu, colonneSouris, ligneSouris);
}

int main() {
    // Sur les ecrans "agrandis" par Windows (125%, 150%, 200%...), le jeu occupe bien toute la fenetre
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(LARGEUR_FENETRE, HAUTEUR_FENETRE, "VESPERANCE - prototype 2D");
    SetTargetFPS(60);
    chargerSprites();       // fabrique les dessins (il faut que la fenetre soit ouverte)

    Jeu jeu;

    while (!WindowShouldClose()) {
        float secondes = GetFrameTime();    // le temps ecoule depuis l'image precedente

        // 1. Lire la souris et le clavier
        Vector2 souris = GetMousePosition();
        int colonneSouris = (int)(souris.x / TAILLE_CASE);
        int ligneSouris = (int)(souris.y / TAILLE_CASE);
        if (souris.y >= HAUTEUR_ARENE) {
            ligneSouris = -1;   // la souris est sur le panneau, pas sur l'arene
        }

        if (jeu.phase == Phase::ChoixVoie) {
            for (int voie = 1; voie <= 3; voie++) {
                bool clic = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(souris, rectangleCarteVoie(voie));
                if (clic || IsKeyPressed(KEY_ZERO + voie) || IsKeyPressed(KEY_KP_0 + voie)) {
                    choisirVoie(jeu, voie);
                    break;
                }
            }
        } else if (jeu.phase == Phase::Deplacement || jeu.phase == Phase::Action) {
            commandesJoueur(jeu, colonneSouris, ligneSouris);
        } else if (jeu.phase == Phase::CombatGagne && IsKeyPressed(KEY_ENTER) && !animationsEnCours(jeu)) {
            combatSuivant(jeu);
        } else if ((jeu.phase == Phase::Victoire || jeu.phase == Phase::Defaite) && IsKeyPressed(KEY_R) && !animationsEnCours(jeu)) {
            jeu.phase = Phase::ChoixVoie;
        }

        // 2. Mettre le jeu a jour
        // L'arret sur image : sur un coup critique, le temps du jeu s'arrete une fraction de seconde
        float tempsDuJeu = secondes;
        if (jeu.arretSurImage > 0) {
            jeu.arretSurImage = jeu.arretSurImage - secondes;
            tempsDuJeu = 0.0f;
        }
        if (jeu.phase == Phase::TourEnnemi) {
            mettreAJourTourEnnemi(jeu, tempsDuJeu);
        }
        mettreAJourTextes(jeu, tempsDuJeu);
        mettreAJourAnimations(jeu, tempsDuJeu);

        // 3. Tout redessiner
        BeginDrawing();
        dessinerJeu(jeu, colonneSouris, ligneSouris);
        EndDrawing();
    }

    dechargerSprites();
    CloseWindow();
    return 0;
}
