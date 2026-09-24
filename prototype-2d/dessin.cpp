// dessin.cpp : tout ce qui s'affiche a l'ecran
#include <string>
#include <vector>
#include <cmath>
#include "jeu2d.h"
#include "sprites.h"

const Color FOND = {20, 18, 28, 255};
const Color PANNEAU = {14, 12, 20, 255};

// ===================== Les boutons et les cartes =====================

const std::vector<Action>& actionsDeLaBarre() {
    static const std::vector<Action> actions = {
        Action::Attaque, Action::AttaqueLourde, Action::Garde, Action::Potion, Action::BouleDeFeu,
        Action::Soin, Action::Eclair, Action::Bouclier, Action::Speciale,
    };
    return actions;
}

Rectangle rectangleBouton(int numero) {
    return {4.0f + numero * 95.0f, (float)HAUTEUR_ARENE + 40, 91, 44};
}

Rectangle rectangleCarteVoie(int voie) {
    return {42.0f + (voie - 1) * 270.0f, 190, 250, 330};
}

// ===================== L'ecran de depart =====================

void dessinerChoixVoie() {
    const char* titre = "PROPHETIA";
    DrawText(titre, (LARGEUR_FENETRE - MeasureText(titre, 72)) / 2, 50, 72, VIOLET);
    const char* sousTitre = "Choisis la voie d'AYLIS (clique sur une carte, ou tape 1, 2 ou 3)";
    DrawText(sousTitre, (LARGEUR_FENETRE - MeasureText(sousTitre, 20)) / 2, 140, 20, LIGHTGRAY);

    const char* noms[3] = {"L'EPEE", "L'ARC", "ARCANES"};
    const Color couleurs[3] = {GOLD, LIME, VIOLET};
    const char* lignes[3][5] = {
        {"44 pv  attaque 13", "defense 4", "Epee courte (melee)", "Solide au contact.", "Sort : Boule de feu"},
        {"40 pv  attaque 12", "defense 4", "Arc court (4 cases)", "Tire de loin.", "Sort : Boule de feu"},
        {"34 pv  attaque 12", "defense 3, 20 mana", "Baton (4 cases)", "Fragile mais puissant.", "Les 4 sorts !"},
    };

    Vector2 souris = GetMousePosition();
    for (int voie = 1; voie <= 3; voie++) {
        Rectangle carte = rectangleCarteVoie(voie);
        bool survol = CheckCollisionPointRec(souris, carte);
        DrawRectangleRec(carte, survol ? Color{40, 36, 56, 255} : Color{30, 27, 42, 255});
        DrawRectangleLinesEx(carte, survol ? 4 : 2, couleurs[voie - 1]);

        int x = carte.x + 20;
        DrawText(TextFormat("%i.", voie), x, carte.y + 20, 30, couleurs[voie - 1]);
        DrawText(noms[voie - 1], x + 40, carte.y + 22, 28, couleurs[voie - 1]);
        for (int i = 0; i < 5; i++) {
            DrawText(lignes[voie - 1][i], x, carte.y + 90 + i * 40, 20, i < 3 ? RAYWHITE : LIGHTGRAY);
        }
    }

    const char* aide = "Deplace AYLIS sur les cases bleues, puis choisis une action et clique sur un Haschen.";
    DrawText(aide, (LARGEUR_FENETRE - MeasureText(aide, 18)) / 2, 560, 18, GRAY);
}

// ===================== L'arene =====================

// Les etats d'un pion, sous forme de petites pastilles au-dessus de lui
void dessinerEtats(const Pion& pion) {
    const Combattant& s = pion.stats;
    int x = pion.colonne * TAILLE_CASE + 6;
    int y = pion.ligne * TAILLE_CASE + 12;
    struct Pastille { bool actif; const char* lettre; Color couleur; };
    const Pastille pastilles[5] = {
        {s.poison > 0, "P", GREEN},
        {s.brulure > 0, "B", ORANGE},
        {s.saignement > 0, "S", RED},
        {s.etourdi, "Z", SKYBLUE},
        {s.bouclier > 0, "O", WHITE},
    };
    for (const Pastille& p : pastilles) {
        if (p.actif) {
            DrawCircle(x + 7, y + 7, 8, Fade(BLACK, 0.7f));
            DrawText(p.lettre, x + 3, y, 16, p.couleur);
            x = x + 18;
        }
    }
}

void dessinerBarreDeVie(const Pion& pion) {
    int x = pion.colonne * TAILLE_CASE + 8;
    int y = pion.ligne * TAILLE_CASE + 4;
    int largeur = TAILLE_CASE - 16;
    int pv = pion.stats.pv < 0 ? 0 : pion.stats.pv;
    int remplie = largeur * pv / pion.stats.pvMax;
    Color couleur = GREEN;
    if (pv * 4 <= pion.stats.pvMax) {
        couleur = RED;
    } else if (pv * 2 <= pion.stats.pvMax) {
        couleur = YELLOW;
    }
    DrawRectangle(x, y, largeur, 6, DARKGRAY);
    DrawRectangle(x, y, remplie, 6, couleur);
}

// Dessine un sprite a l'ecran, agrandi. versLaGauche = true le retourne comme dans un miroir.
void dessinerSprite(const Texture2D& texture, Rectangle ecran, bool versLaGauche, Color teinte) {
    // Une largeur negative dans le rectangle source retourne l'image horizontalement
    Rectangle source = {0, 0, versLaGauche ? -(float)TAILLE_SPRITE : (float)TAILLE_SPRITE, (float)TAILLE_SPRITE};
    DrawTexturePro(texture, source, ecran, {0, 0}, 0, teinte);
}

void dessinerPion(const Pion& pion, bool estAylis, int colonneAylis) {
    const Sprites& s = sprites();
    float centreX = pion.colonne * TAILLE_CASE + TAILLE_CASE / 2.0f;
    float basY = pion.ligne * TAILLE_CASE + TAILLE_CASE - 6.0f;

    // Les personnages "respirent" : ils montent et descendent un tout petit peu, chacun a son rythme
    float respiration = std::sin(GetTime() * 3.0 + pion.colonne * 1.7 + pion.ligne) * 2.0f;
    float taille = pion.stats.estBoss ? 80.0f : 64.0f;
    Rectangle ecran = {centreX - taille / 2, basY - taille + respiration, taille, taille};

    // Touche il y a un instant : il clignote en rouge
    Color teinte = pion.flash > 0 ? Color{255, 90, 90, 255} : WHITE;

    // Une ombre au sol
    DrawEllipse(centreX, basY - 2, taille * 0.3f, 6, Fade(BLACK, 0.35f));

    if (estAylis) {
        dessinerSprite(s.aylis, ecran, false, teinte);
        const Arme& arme = pion.stats.arme;
        const Texture2D* armeDessinee = &s.epee;
        if (arme.nom.find("Baton") != std::string::npos) {
            armeDessinee = &s.baton;
        } else if (arme.aDistance) {
            armeDessinee = &s.arc;
        }
        dessinerSprite(*armeDessinee, ecran, false, teinte);
    } else {
        // Les Haschen regardent vers AYLIS
        bool versLaGauche = colonneAylis < pion.colonne;
        dessinerSprite(spriteHaschen(pion.stats.nom), ecran, versLaGauche, teinte);
        const Texture2D* arme = spriteArmeHaschen(pion.stats.nom);
        if (arme != nullptr) {
            dessinerSprite(*arme, ecran, versLaGauche, teinte);
        }
        if (pion.stats.estBoss) {
            dessinerSprite(s.couronne, ecran, versLaGauche, WHITE);
        }
    }
    dessinerBarreDeVie(pion);
    dessinerEtats(pion);
}

// L'ambiance du lieu, par-dessus l'arene : brume en foret, lueur du feu au camp, neige sur le col
void dessinerAmbiance(int lieu) {
    float temps = GetTime();
    if (lieu == 0) {
        for (int i = 0; i < 6; i++) {
            float x = std::fmod(temps * (12 + i * 3) + i * 170, LARGEUR_FENETRE + 300) - 150;
            float y = 60 + i * 85 + std::sin(temps * 0.5 + i) * 20;
            DrawEllipse(x, y, 160, 40, Fade(WHITE, 0.045f));
        }
    } else if (lieu == 1) {
        float pulsation = 0.05f + 0.02f * std::sin(temps * 6);
        DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_ARENE, Fade(ORANGE, pulsation));
    } else {
        for (int i = 0; i < 70; i++) {
            float x = std::fmod(i * 97.0f + temps * (10 + i % 5 * 4), (float)LARGEUR_FENETRE);
            float y = std::fmod(i * 53.0f + temps * (35 + i % 7 * 6), (float)HAUTEUR_ARENE);
            DrawRectangle(x, y, i % 3 == 0 ? 3 : 2, i % 3 == 0 ? 3 : 2, Fade(WHITE, 0.8f));
        }
    }
    // Les bords de l'arene un peu plus sombres : l'oeil se concentre sur le centre
    DrawRectangleGradientV(0, 0, LARGEUR_FENETRE, 50, Fade(BLACK, 0.4f), BLANK);
    DrawRectangleGradientV(0, HAUTEUR_ARENE - 50, LARGEUR_FENETRE, 50, BLANK, Fade(BLACK, 0.4f));
}

void dessinerArene(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    bool tourJoueur = jeu.phase == Phase::Deplacement || jeu.phase == Phase::Action;
    std::vector<int> pas = casesAtteignables(jeu);
    int portee = porteeAction(jeu, jeu.actionChoisie);
    bool actionVisee = tourJoueur && !actionSurSoi(jeu.actionChoisie) && actionDisponible(jeu, jeu.actionChoisie);

    // Le lieu du combat decide des dessins : 0 = foret, 1 = camp, 2 = col
    int lieu = jeu.combat < 3 ? jeu.combat : 2;
    const Sprites& s = sprites();

    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            int x = c * TAILLE_CASE;
            int y = l * TAILLE_CASE;
            Rectangle caseEcran = {(float)x, (float)y, (float)TAILLE_CASE, (float)TAILLE_CASE};
            DrawTexturePro(s.sol[lieu][(c + l) % 2], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, caseEcran, {0, 0}, 0, WHITE);

            // Un "hasard" fixe, calcule a partir de la position : les decors restent toujours au meme endroit
            int hasard = (c * 17 + l * 31 + jeu.combat * 7) % 11;

            if (estRocher(jeu, c, l)) {
                // Au milieu du camp, un obstacle est un feu de camp anime (2 images qui alternent)
                if (lieu == 1 && c == 9 && l == 3) {
                    int image = (int)(GetTime() * 5) % 2;
                    DrawTexturePro(s.feu[image], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, caseEcran, {0, 0}, 0, WHITE);
                } else {
                    DrawTexturePro(s.obstacle[lieu][hasard % 2], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, caseEcran,
                                   {0, 0}, 0, WHITE);
                }
                continue;
            }
            // Quelques decors par terre (ils ne bloquent pas le passage)
            if (hasard < 2) {
                DrawTexturePro(s.decor[lieu][hasard], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, caseEcran, {0, 0}, 0, WHITE);
            }
            // La portee de l'action choisie : une teinte rouge tres legere
            if (actionVisee && distanceCases(jeu.aylis.colonne, jeu.aylis.ligne, c, l) <= portee) {
                DrawRectangle(x, y, TAILLE_CASE, TAILLE_CASE, Fade(RED, 0.07f));
            }
            // Les cases ou AYLIS peut aller
            if (jeu.phase == Phase::Deplacement && pas[l * COLONNES + c] > 0) {
                DrawRectangle(x + 2, y + 2, TAILLE_CASE - 4, TAILLE_CASE - 4, Fade(SKYBLUE, 0.25f));
            }
        }
    }

    // Les Haschen a portee : un cadre rouge. Pour la boule de feu, la zone d'explosion sous la souris.
    int nombre = jeu.haschen.size();
    for (int i = 0; i < nombre; i++) {
        const Pion& h = jeu.haschen[i];
        if (actionVisee && h.stats.estDebout() && distanceEntre(jeu.aylis, h) <= portee) {
            DrawRectangleLinesEx({(float)h.colonne * TAILLE_CASE, (float)h.ligne * TAILLE_CASE,
                                  (float)TAILLE_CASE, (float)TAILLE_CASE}, 3, RED);
        }
    }
    if (actionVisee && jeu.actionChoisie == Action::BouleDeFeu && estDansArene(colonneSouris, ligneSouris)) {
        for (int l = 0; l < LIGNES; l++) {
            for (int c = 0; c < COLONNES; c++) {
                if (distanceCases(c, l, colonneSouris, ligneSouris) <= 1) {
                    DrawRectangle(c * TAILLE_CASE, l * TAILLE_CASE, TAILLE_CASE, TAILLE_CASE, Fade(ORANGE, 0.25f));
                }
            }
        }
    }

    if (tourJoueur && estDansArene(colonneSouris, ligneSouris)) {
        DrawRectangleLines(colonneSouris * TAILLE_CASE, ligneSouris * TAILLE_CASE, TAILLE_CASE, TAILLE_CASE, WHITE);
    }

    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout()) {
            dessinerPion(h, false, jeu.aylis.colonne);
        }
    }
    if (jeu.aylis.stats.estDebout()) {
        dessinerPion(jeu.aylis, true, jeu.aylis.colonne);
    }

    dessinerAmbiance(lieu);

    for (const TexteFlottant& t : jeu.textes) {
        float transparence = t.tempsRestant > 1.0f ? 1.0f : t.tempsRestant;
        DrawText(t.texte.c_str(), t.x, t.y, 24, Fade(t.couleur, transparence));
    }

    // Le Haschen sous la souris : son nom et ses pv
    int survole = haschenSurCase(jeu, colonneSouris, ligneSouris);
    if (survole != -1) {
        const Pion& h = jeu.haschen[survole];
        const char* info = TextFormat("%s  %i/%i pv  (attaque %i, defense %i)", h.stats.nom.c_str(),
                                      h.stats.pv, h.stats.pvMax, h.stats.attaque, h.stats.defense);
        DrawRectangle(0, 0, MeasureText(info, 18) + 16, 26, Fade(BLACK, 0.8f));
        DrawText(info, 8, 4, 18, WHITE);
    }
}

// ===================== Le panneau du bas =====================

void dessinerPanneau(const Jeu& jeu) {
    int y = HAUTEUR_ARENE;
    const Combattant& a = jeu.aylis.stats;
    DrawRectangle(0, y, LARGEUR_FENETRE, HAUTEUR_PANNEAU, PANNEAU);

    // La ligne des stats
    DrawText(TextFormat("AYLIS %i/%i pv", a.pv < 0 ? 0 : a.pv, a.pvMax), 10, y + 10, 20, SKYBLUE);
    DrawText(TextFormat("Mana %i/%i", a.mana, a.manaMax), 190, y + 10, 20, Color{120, 150, 255, 255});
    DrawText("Rage", 330, y + 10, 20, RED);
    DrawRectangle(385, y + 14, 100, 12, DARKGRAY);
    DrawRectangle(385, y + 14, jeu.rage.valeur(), 12, jeu.rage.estPleine() ? ORANGE : RED);
    DrawText(TextFormat("Potions %i", a.potions), 500, y + 10, 20, GREEN);
    DrawText(TextFormat("%s   Tour %i", a.arme.nom.c_str(), jeu.tour), 620, y + 10, 18, LIGHTGRAY);

    // La barre d'actions
    const std::vector<Action>& actions = actionsDeLaBarre();
    int nombre = actions.size();
    for (int i = 0; i < nombre; i++) {
        Action action = actions[i];
        Rectangle bouton = rectangleBouton(i);
        bool dispo = actionDisponible(jeu, action);
        bool choisie = jeu.actionChoisie == action;
        DrawRectangleRec(bouton, choisie ? Color{70, 50, 20, 255} : Color{32, 29, 44, 255});
        DrawRectangleLinesEx(bouton, choisie ? 3 : 1, choisie ? GOLD : GRAY);

        Color couleurTexte = dispo ? RAYWHITE : Color{90, 90, 90, 255};
        DrawText(TextFormat("%i %s", i + 1, nomAction(action).c_str()), bouton.x + 6, bouton.y + 5, 16, couleurTexte);
        std::string detail = "";
        if (coutMana(action) > 0) {
            detail = std::to_string(coutMana(action)) + " mana";
        } else if (action == Action::Potion) {
            detail = "+15 pv";
        } else if (action == Action::Speciale) {
            detail = "rage x2.2";
        } else {
            detail = "portee " + std::to_string(porteeAction(jeu, action));
        }
        DrawText(detail.c_str(), bouton.x + 6, bouton.y + 25, 14, dispo ? LIGHTGRAY : Color{80, 80, 80, 255});
    }

    // La consigne
    std::string consigne = "";
    if (jeu.phase == Phase::Deplacement) {
        consigne = "Deplace AYLIS (cases bleues), puis choisis une action (1-9) et clique sur une cible.";
    } else if (jeu.phase == Phase::Action) {
        consigne = "Clique un Haschen encadre (ou AYLIS pour potion/soin/bouclier). ESPACE = passer.";
    } else if (jeu.phase == Phase::TourEnnemi) {
        consigne = "Les Haschen jouent...";
    }
    DrawText(consigne.c_str(), 10, y + 92, 16, GOLD);

    int yJournal = y + 114;
    for (const std::string& ligne : jeu.journal) {
        DrawText(ligne.c_str(), 10, yJournal, 16, LIGHTGRAY);
        yJournal = yJournal + 18;
    }
}

// Un grand message au milieu de l'arene
void dessinerMessage(const char* titre, Color couleur, const char* aide) {
    DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_ARENE, Fade(BLACK, 0.6f));
    DrawText(titre, (LARGEUR_FENETRE - MeasureText(titre, 52)) / 2, HAUTEUR_ARENE / 2 - 50, 52, couleur);
    DrawText(aide, (LARGEUR_FENETRE - MeasureText(aide, 22)) / 2, HAUTEUR_ARENE / 2 + 20, 22, WHITE);
}

void dessinerJeu(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    ClearBackground(FOND);

    if (jeu.phase == Phase::ChoixVoie) {
        dessinerChoixVoie();
        return;
    }

    dessinerArene(jeu, colonneSouris, ligneSouris);
    dessinerPanneau(jeu);

    if (jeu.phase == Phase::CombatGagne) {
        dessinerMessage(TextFormat("%s : victoire !", jeu.nomDuLieu.c_str()), GREEN,
                        "Appuie sur ENTREE pour continuer la route");
    } else if (jeu.phase == Phase::Victoire) {
        dessinerMessage("ASHKA EST VAINCUE !", GREEN, "Fin du prototype. Appuie sur R pour rejouer");
    } else if (jeu.phase == Phase::Defaite) {
        dessinerMessage("AYLIS TOMBE AU COMBAT...", RED, "Appuie sur R pour rejouer");
    }
}
