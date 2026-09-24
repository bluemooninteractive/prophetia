// dessin.cpp : tout ce qui s'affiche a l'ecran
#include <string>
#include <vector>
#include <cmath>
#include "jeu2d.h"
#include "sprites.h"
#include "animations.h"

// Les couleurs de l'interface
const Color FOND = {20, 18, 28, 255};
const Color PANNEAU = {16, 14, 23, 255};
const Color CADRE = {32, 29, 44, 255};
const Color CADRE_CLAIR = {48, 43, 66, 255};
const Color BORD = {88, 80, 112, 255};
const Color OR = {232, 192, 84, 255};
const Color TEXTE_GRIS = {150, 145, 165, 255};

// ===================== Les petites briques de l'interface =====================

// Un cadre aux coins arrondis, avec une bordure
void dessinerCadre(Rectangle r, Color fond, Color bord, float epaisseur) {
    DrawRectangleRounded(r, 0.18f, 6, fond);
    DrawRectangleRoundedLinesEx(r, 0.18f, 6, epaisseur, bord);
}

// Une jauge (pv, mana, rage) : un fond sombre, la partie remplie, et un texte au milieu
void dessinerJauge(Rectangle r, int valeur, int maximum, Color couleur, const std::string& texte) {
    if (valeur < 0) {
        valeur = 0;
    }
    DrawRectangleRec(r, Color{10, 9, 15, 255});
    float remplie = maximum > 0 ? r.width * valeur / maximum : 0;
    DrawRectangle(r.x, r.y, remplie, r.height, couleur);
    DrawRectangle(r.x, r.y, remplie, r.height / 3, Fade(WHITE, 0.18f));     // un reflet en haut
    DrawRectangleLinesEx(r, 1, BORD);
    int largeurTexte = MeasureText(texte.c_str(), 10);
    DrawText(texte.c_str(), r.x + (r.width - largeurTexte) / 2, r.y + (r.height - 10) / 2, 10, WHITE);
}

// Un texte centre dans un rectangle
void texteCentre(const std::string& texte, Rectangle r, int taille, Color couleur) {
    int largeur = MeasureText(texte.c_str(), taille);
    DrawText(texte.c_str(), r.x + (r.width - largeur) / 2, r.y + (r.height - taille) / 2, taille, couleur);
}

// La couleur d'une barre de vie : verte, puis jaune, puis rouge
Color couleurVie(int pv, int pvMax) {
    if (pv * 4 <= pvMax) {
        return Color{210, 60, 60, 255};
    }
    if (pv * 2 <= pvMax) {
        return Color{220, 190, 60, 255};
    }
    return Color{80, 190, 90, 255};
}

// Le portrait d'AYLIS avec l'arme de sa voie
void dessinerPortraitAylis(const Arme& arme, Rectangle ecran, Color teinte) {
    const Sprites& s = sprites();
    Rectangle source = {0, 0, TAILLE_SPRITE, TAILLE_SPRITE};
    DrawTexturePro(s.aylis, source, ecran, {0, 0}, 0, teinte);
    const Texture2D* armeDessinee = &s.epee;
    if (arme.nom.find("Baton") != std::string::npos) {
        armeDessinee = &s.baton;
    } else if (arme.aDistance) {
        armeDessinee = &s.arc;
    }
    DrawTexturePro(*armeDessinee, source, ecran, {0, 0}, 0, teinte);
}

// ===================== Les boutons et les cartes =====================

const std::vector<Action>& actionsDeLaBarre() {
    static const std::vector<Action> actions = {
        Action::Attaque, Action::AttaqueLourde, Action::Garde, Action::Potion, Action::BouleDeFeu,
        Action::Soin, Action::Eclair, Action::Bouclier, Action::Speciale,
    };
    return actions;
}

// Les 9 actions sont rangees en 3 lignes de 3 cartes, a droite du panneau
Rectangle rectangleBouton(int numero) {
    int colonne = numero % 3;
    int ligne = numero / 3;
    return {408.0f + colonne * 150.0f, (float)HAUTEUR_ARENE + 12 + ligne * 56.0f, 146, 52};
}

Rectangle rectangleCarteVoie(int voie) {
    return {42.0f + (voie - 1) * 270.0f, 170, 250, 380};
}

// ===================== L'ecran de depart =====================

void dessinerChoixVoie() {
    const char* titre = "VESPERANCE";
    DrawText(titre, (LARGEUR_FENETRE - MeasureText(titre, 70)) / 2, 40, 70, Color{170, 110, 240, 255});
    const char* sousTitre = "Choisis la voie d'AYLIS";
    DrawText(sousTitre, (LARGEUR_FENETRE - MeasureText(sousTitre, 20)) / 2, 125, 20, LIGHTGRAY);

    const char* noms[3] = {"L'EPEE", "L'ARC", "LES ARCANES"};
    const Color couleurs[3] = {OR, Color{110, 210, 110, 255}, Color{175, 115, 240, 255}};
    const Arme armes[3] = {
        {"Epee courte", false, 0, 10, 1, 20},
        {"Arc court", true, 0, 15, 1, 45},
        {"Baton de mage", true, 0, 5, 1, 20},
    };
    const char* lignes[3][4] = {
        {"44 pv  -  attaque 13", "defense 4", "Frappe au contact.", "Sort : Boule de feu"},
        {"40 pv  -  attaque 12", "defense 4", "Tire a 4 cases.", "Sort : Boule de feu"},
        {"34 pv  -  attaque 12", "defense 3  -  20 mana", "Fragile mais puissant.", "Connait les 4 sorts !"},
    };

    Vector2 souris = GetMousePosition();
    for (int voie = 1; voie <= 3; voie++) {
        Rectangle carte = rectangleCarteVoie(voie);
        bool survol = CheckCollisionPointRec(souris, carte);
        Color couleur = couleurs[voie - 1];

        // La carte survolee se souleve un peu
        if (survol) {
            carte.y = carte.y - 6;
        }
        dessinerCadre(carte, survol ? CADRE_CLAIR : CADRE, couleur, survol ? 3 : 2);

        // Le portrait, sur un petit halo de la couleur de la voie
        float centreX = carte.x + carte.width / 2;
        DrawCircle(centreX, carte.y + 100, 62, Fade(couleur, 0.15f));
        dessinerPortraitAylis(armes[voie - 1], {centreX - 64, carte.y + 36, 128, 128}, WHITE);

        texteCentre(TextFormat("%i. %s", voie, noms[voie - 1]), {carte.x, carte.y + 180, carte.width, 30}, 20, couleur);
        DrawLine(carte.x + 30, carte.y + 216, carte.x + carte.width - 30, carte.y + 216, Fade(couleur, 0.5f));
        for (int i = 0; i < 4; i++) {
            texteCentre(lignes[voie - 1][i], {carte.x, carte.y + 230 + i * 34.0f, carte.width, 24}, 20,
                        i < 2 ? RAYWHITE : TEXTE_GRIS);
        }
    }

    const char* aide = "Clique sur une carte, ou tape 1, 2 ou 3";
    DrawText(aide, (LARGEUR_FENETRE - MeasureText(aide, 20)) / 2, 590, 20, TEXTE_GRIS);
}

// ===================== L'arene =====================

// Les etats d'un pion, sous forme de petites pastilles au-dessus de lui
void dessinerEtats(const Pion& pion) {
    const Combattant& s = pion.stats;
    Vector2 position = positionAffichee(pion);
    int x = position.x - TAILLE_CASE / 2 + 6;
    int y = position.y - TAILLE_CASE / 2 + 12;
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

// La barre de vie au-dessus d'un pion. La partie blanche, c'est la vie "fantome" :
// elle montre ce qui vient d'etre perdu, puis s'efface doucement.
void dessinerBarreDeVie(const Pion& pion) {
    Vector2 position = positionAffichee(pion);
    int x = position.x - TAILLE_CASE / 2 + 8;
    int y = position.y - TAILLE_CASE / 2 + 4;
    int largeur = TAILLE_CASE - 16;
    int pv = pion.stats.pv < 0 ? 0 : pion.stats.pv;
    float pvAffiches = pion.pvAffiches < 0 ? pv : pion.pvAffiches;
    // Tant que le coup n'est pas encore arrive (fleche en vol), la barre ne bouge pas
    if (pion.flash > 0.25f) {
        pvAffiches = pion.pvAffiches < 0 ? pv : pion.pvAffiches;
        pv = (int)pvAffiches;
    }
    int remplie = largeur * pv / pion.stats.pvMax;
    int fantome = largeur * pvAffiches / pion.stats.pvMax;
    Color couleur = GREEN;
    if (pv * 4 <= pion.stats.pvMax) {
        couleur = RED;
    } else if (pv * 2 <= pion.stats.pvMax) {
        couleur = YELLOW;
    }
    DrawRectangle(x - 1, y - 1, largeur + 2, 8, Fade(BLACK, 0.7f));
    DrawRectangle(x, y, largeur, 6, DARKGRAY);
    DrawRectangle(x, y, fantome, 6, Color{240, 235, 225, 255});
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
    // La position animee : le pion glisse d'une case a l'autre, bondit quand il attaque, recule quand il est touche
    Vector2 position = positionAffichee(pion);
    float centreX = position.x;
    float basY = position.y + TAILLE_CASE / 2.0f - 6.0f;

    // Les personnages "respirent" : ils montent et descendent un tout petit peu, chacun a son rythme
    float respiration = std::sin(GetTime() * 3.0 + pion.colonne * 1.7 + pion.ligne) * 2.0f;
    float taille = pion.stats.estBoss ? 80.0f : (pion.stats.nom.find("elite") != std::string::npos ? 74.0f : 64.0f);

    // Un Haschen vaincu se dissout : il s'eleve, retrecit et devient transparent
    float visibilite = 1.0f;
    if (!pion.stats.estDebout() && pion.disparition > 0) {
        visibilite = pion.disparition < 0.6f ? pion.disparition / 0.6f : 1.0f;
        taille = taille * (0.6f + 0.4f * visibilite);
        basY = basY - (1.0f - visibilite) * 20;
    }
    Rectangle ecran = {centreX - taille / 2, basY - taille + respiration, taille, taille};

    // Touche il y a un instant : il clignote en rouge (au-dessus de 0.25, le coup n'est pas encore arrive)
    Color teinte = pion.flash > 0 && pion.flash <= 0.25f ? Color{255, 90, 90, 255} : WHITE;
    teinte = Fade(teinte, visibilite);

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
    int lieu = jeu.lieu;
    const Sprites& s = sprites();

    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            int x = c * TAILLE_CASE;
            int y = l * TAILLE_CASE;
            Rectangle caseEcran = {(float)x, (float)y, (float)TAILLE_CASE, (float)TAILLE_CASE};
            DrawTexturePro(s.sol[lieu][(c + l) % 2], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, caseEcran, {0, 0}, 0, WHITE);

            // Un "hasard" fixe, calcule a partir de la position : les decors restent toujours au meme endroit
            int hasard = (c * 17 + l * 31 + jeu.salle * 7) % 11;

            if (estRocher(jeu, c, l)) {
                // Au milieu du camp, un obstacle est un feu de camp anime (2 images qui alternent)
                if (c == jeu.feuColonne && l == jeu.feuLigne) {
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

    // Les Haschen debout, et ceux qui sont en train de se dissoudre
    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout() || h.disparition > 0) {
            dessinerPion(h, false, jeu.aylis.colonne);
        }
    }
    if (jeu.aylis.stats.estDebout() || animationsEnCours(jeu)) {
        dessinerPion(jeu.aylis, true, jeu.aylis.colonne);
    }

    dessinerProjectiles(jeu);
    dessinerParticules(jeu);
    dessinerEclairs(jeu);
    dessinerAmbiance(lieu);

    // Les chiffres des degats : ils apparaissent avec un petit rebond (plus gros au debut)
    for (const TexteFlottant& t : jeu.textes) {
        if (t.delai > 0) {
            continue;   // le coup n'est pas encore arrive
        }
        float transparence = t.tempsRestant > 1.0f ? 1.0f : t.tempsRestant;
        float rebond = t.age < 0.15f ? 1.0f + 0.6f * (1.0f - t.age / 0.15f) : 1.0f;
        int taille = (int)(24 * rebond);
        if (t.texte.rfind("CRIT", 0) == 0) {
            taille = (int)(taille * 1.3f);  // les critiques s'affichent en plus gros
        }
        DrawText(t.texte.c_str(), t.x - (taille - 24), t.y - (taille - 24), taille, Fade(BLACK, transparence * 0.6f));
        DrawText(t.texte.c_str(), t.x - (taille - 24) - 2, t.y - (taille - 24) - 2, taille, Fade(t.couleur, transparence));
    }

}

// ===================== Par-dessus l'arene =====================

// Le bandeau du haut : le lieu, le tour, et a qui c'est de jouer
void dessinerBandeau(const Jeu& jeu) {
    DrawRectangleGradientV(0, 0, LARGEUR_FENETRE, 44, Fade(BLACK, 0.8f), Fade(BLACK, 0.0f));
    DrawText(TextFormat("SALLE %i/%i", jeu.salle, NOMBRE_SALLES), 12, 8, 20, OR);
    DrawText(jeu.nomDuLieu.c_str(), 150, 8, 20, RAYWHITE);

    std::string qui = "TON TOUR";
    Color couleur = Color{90, 200, 110, 255};
    if (jeu.phase == Phase::TourEnnemi) {
        qui = "TOUR DES HASCHEN";
        couleur = Color{220, 80, 80, 255};
    }
    int largeur = MeasureText(qui.c_str(), 20) + 24;
    Rectangle pilule = {(float)LARGEUR_FENETRE - largeur - 10, 5, (float)largeur, 26};
    DrawRectangleRounded(pilule, 1.0f, 8, Fade(couleur, 0.25f));
    DrawRectangleRoundedLinesEx(pilule, 1.0f, 8, 2, couleur);
    texteCentre(qui, pilule, 20, couleur);

    std::string tour = "Tour " + std::to_string(jeu.tour);
    DrawText(tour.c_str(), pilule.x - MeasureText(tour.c_str(), 20) - 14, 8, 20, TEXTE_GRIS);
}

// Le journal : les 3 derniers messages, en haut a gauche. Les plus anciens sont plus pales.
void dessinerJournal(const Jeu& jeu) {
    int nombre = jeu.journal.size();
    for (int i = 0; i < nombre; i++) {
        const std::string& ligne = jeu.journal[i];
        float transparence = 0.45f + 0.55f * (i + 1) / nombre;

        Color couleur = LIGHTGRAY;
        if (ligne.find("touche AYLIS") != std::string::npos || ligne.find("Poison") != std::string::npos) {
            couleur = Color{255, 125, 125, 255};
        } else if (ligne.find("tombe") != std::string::npos || ligne.find("succombe") != std::string::npos) {
            couleur = Color{140, 230, 140, 255};
        } else if (ligne.find("!") != std::string::npos) {
            couleur = OR;
        }

        int y = 40 + i * 24;
        DrawRectangle(8, y, MeasureText(ligne.c_str(), 20) + 16, 22, Fade(BLACK, 0.5f * transparence));
        DrawText(ligne.c_str(), 16, y + 1, 20, Fade(couleur, transparence));
    }
}

// La consigne, en bas de l'arene, pendant le tour d'AYLIS
void dessinerConsigne(const Jeu& jeu) {
    std::string consigne = "";
    if (jeu.phase == Phase::Deplacement) {
        consigne = "Deplace AYLIS (cases bleues), puis choisis une action et une cible";
    } else if (jeu.phase == Phase::Action) {
        consigne = "Clique sur un Haschen encadre   -   ESPACE pour passer";
    }
    if (actionSurSoi(jeu.actionChoisie) && (jeu.phase == Phase::Deplacement || jeu.phase == Phase::Action)) {
        consigne = nomAction(jeu.actionChoisie) + " : clique sur AYLIS ou appuie sur ENTREE";
    }
    if (consigne == "") {
        return;
    }
    int largeur = MeasureText(consigne.c_str(), 20) + 24;
    Rectangle bande = {(LARGEUR_FENETRE - largeur) / 2.0f, HAUTEUR_ARENE - 34.0f, (float)largeur, 26};
    DrawRectangleRounded(bande, 1.0f, 8, Fade(BLACK, 0.6f));
    texteCentre(consigne, bande, 20, OR);
}

// La fiche d'un Haschen, quand la souris passe dessus
void dessinerFicheHaschen(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    int numero = haschenSurCase(jeu, colonneSouris, ligneSouris);
    if (numero == -1) {
        return;
    }
    const Pion& h = jeu.haschen[numero];
    const Combattant& s = h.stats;

    // La fiche se place a droite du Haschen, ou a gauche s'il n'y a pas la place
    Rectangle fiche = {(h.colonne + 1.0f) * TAILLE_CASE + 6, h.ligne * (float)TAILLE_CASE, 240, 118};
    if (fiche.x + fiche.width > LARGEUR_FENETRE) {
        fiche.x = h.colonne * TAILLE_CASE - fiche.width - 6;
    }
    if (fiche.y + fiche.height > HAUTEUR_ARENE) {
        fiche.y = HAUTEUR_ARENE - fiche.height - 4;
    }
    dessinerCadre(fiche, Fade(PANNEAU, 0.95f), s.estBoss ? OR : BORD, 2);

    DrawText(s.nom.c_str(), fiche.x + 10, fiche.y + 8, 20, s.estBoss ? OR : RAYWHITE);
    dessinerJauge({fiche.x + 10, fiche.y + 34, fiche.width - 20, 14}, s.pv, s.pvMax, couleurVie(s.pv, s.pvMax),
                  TextFormat("%i / %i pv", s.pv < 0 ? 0 : s.pv, s.pvMax));
    DrawText(TextFormat("ATQ %i    DEF %i", s.attaque, s.defense), fiche.x + 10, fiche.y + 56, 20, LIGHTGRAY);

    std::string style = "Combat au corps a corps";
    if (s.estBoss) {
        style = "BOSS : s'enrage a mi-vie";
    } else if (s.style == Style::Lanceur) {
        style = "Tire de loin (4 cases)";
    } else if (s.style == Style::Chargeur) {
        style = "Charge (4 cases d'un coup)";
    } else if (s.attaquePoison) {
        style = "Ses coups empoisonnent";
    }
    DrawText(style.c_str(), fiche.x + 10, fiche.y + 82, 10, TEXTE_GRIS);

    std::string etats = "";
    if (s.poison > 0) etats = etats + "Poison  ";
    if (s.brulure > 0) etats = etats + "Brule  ";
    if (s.saignement > 0) etats = etats + "Saigne  ";
    if (s.etourdi) etats = etats + "Paralyse";
    DrawText(etats.c_str(), fiche.x + 10, fiche.y + 98, 10, Color{255, 150, 90, 255});
}

// La grande banniere animee au milieu ("TOUR 3", le nom du lieu...)
void dessinerBanniere(const Jeu& jeu) {
    if (jeu.tempsBanniere <= 0) {
        return;
    }
    float visibilite = jeu.tempsBanniere > 0.4f ? 1.0f : jeu.tempsBanniere / 0.4f;
    float y = HAUTEUR_ARENE / 2.0f - 40;
    DrawRectangleGradientH(0, y, LARGEUR_FENETRE / 2, 70, Fade(BLACK, 0.0f), Fade(BLACK, 0.6f * visibilite));
    DrawRectangleGradientH(LARGEUR_FENETRE / 2, y, LARGEUR_FENETRE / 2, 70, Fade(BLACK, 0.6f * visibilite), Fade(BLACK, 0.0f));
    texteCentre(jeu.banniere, {0, y, (float)LARGEUR_FENETRE, 70}, 40, Fade(OR, visibilite));
}

// ===================== Le panneau du bas =====================

void dessinerPanneau(const Jeu& jeu) {
    int y0 = HAUTEUR_ARENE;
    const Combattant& a = jeu.aylis.stats;
    Vector2 souris = GetMousePosition();

    DrawRectangle(0, y0, LARGEUR_FENETRE, HAUTEUR_PANNEAU, PANNEAU);
    DrawRectangle(0, y0, LARGEUR_FENETRE, 2, BORD);

    // ----- Le portrait -----
    Rectangle cadrePortrait = {8, (float)y0 + 10, 120, 170};
    dessinerCadre(cadrePortrait, CADRE, BORD, 2);
    Color teinte = jeu.aylis.flash > 0 ? Color{255, 90, 90, 255} : WHITE;
    dessinerPortraitAylis(a.arme, {20, (float)y0 + 14, 96, 96}, teinte);
    texteCentre("AYLIS", {8, (float)y0 + 114, 120, 24}, 20, Color{120, 180, 255, 255});
    texteCentre("voie " + a.voie, {8, (float)y0 + 140, 120, 16}, 10, TEXTE_GRIS);
    texteCentre(TextFormat("%i potions - %i or", a.potions, a.pieces), {8, (float)y0 + 158, 120, 16}, 10, Color{230, 200, 110, 255});

    // ----- Les jauges -----
    float x = 140;
    DrawText("PV", x, y0 + 16, 10, TEXTE_GRIS);
    dessinerJauge({x + 36, (float)y0 + 12, 220, 20}, a.pv, a.pvMax, couleurVie(a.pv, a.pvMax),
                  TextFormat("%i / %i", a.pv < 0 ? 0 : a.pv, a.pvMax));
    DrawText("MANA", x, y0 + 44, 10, TEXTE_GRIS);
    dessinerJauge({x + 36, (float)y0 + 40, 220, 20}, a.mana, a.manaMax, Color{80, 120, 230, 255},
                  TextFormat("%i / %i", a.mana, a.manaMax));
    DrawText("RAGE", x, y0 + 72, 10, TEXTE_GRIS);
    Color couleurRage = Color{200, 70, 50, 255};
    std::string texteRage = TextFormat("%i%%", jeu.rage.valeur());
    if (jeu.rage.estPleine()) {
        // Pleine : elle pulse
        float pulsation = 0.5f + 0.5f * std::sin(GetTime() * 8);
        couleurRage = Color{255, (unsigned char)(120 + 80 * pulsation), 40, 255};
        texteRage = "PLEINE ! (touche 9)";
    }
    dessinerJauge({x + 36, (float)y0 + 68, 220, 20}, jeu.rage.valeur(), jeu.rage.maximum(), couleurRage, texteRage);

    // ----- L'arme et les effets en cours -----
    DrawText(a.arme.nom.c_str(), x, y0 + 102, 20, RAYWHITE);
    std::string portee = a.arme.aDistance ? TextFormat("portee %i cases", PORTEE_DISTANCE + jeu.porteeBonus) : "corps a corps";
    DrawText(portee.c_str(), x, y0 + 126, 10, TEXTE_GRIS);

    std::string effets = "";
    if (jeu.enGarde) effets = effets + "EN GARDE  ";
    if (a.bouclier > 0) effets = effets + TextFormat("BOUCLIER %i  ", a.bouclier);
    if (a.poison > 0) effets = effets + "POISON  ";
    if (a.brulure > 0) effets = effets + "BRULE  ";
    if (a.saignement > 0) effets = effets + "SAIGNE";
    DrawText(effets.c_str(), x, y0 + 146, 10, Color{255, 170, 90, 255});

    // ----- Les cartes d'actions -----
    const std::vector<Action>& actions = actionsDeLaBarre();
    int nombre = actions.size();
    int survolee = -1;
    for (int i = 0; i < nombre; i++) {
        Action action = actions[i];
        Rectangle carte = rectangleBouton(i);
        bool dispo = actionDisponible(jeu, action);
        bool choisie = jeu.actionChoisie == action;
        bool survol = CheckCollisionPointRec(souris, carte);
        if (survol) {
            survolee = i;
        }

        Color fond = choisie ? Color{72, 54, 24, 255} : (survol ? CADRE_CLAIR : CADRE);
        dessinerCadre(carte, fond, choisie ? OR : BORD, choisie ? 3 : 1);

        Color teinteIcone = dispo ? WHITE : Color{70, 70, 80, 255};
        DrawTexturePro(sprites().icones[i], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, {carte.x + 8, carte.y + 10, 32, 32},
                       {0, 0}, 0, teinteIcone);

        // Le numero de la touche, dans une pastille
        DrawCircle(carte.x + 9, carte.y + 9, 8, Fade(BLACK, 0.8f));
        DrawText(TextFormat("%i", i + 1), carte.x + 6, carte.y + 4, 10, OR);

        DrawText(nomAction(action).c_str(), carte.x + 48, carte.y + 8, 20, dispo ? RAYWHITE : Color{95, 92, 105, 255});
        std::string detail = "";
        if (coutMana(action) > 0) {
            detail = std::to_string(coutMana(action)) + " mana";
        } else if (action == Action::Potion) {
            detail = TextFormat("+15 pv  (x%i)", a.potions);
        } else if (action == Action::Speciale) {
            detail = "rage pleine";
        } else {
            detail = "portee " + std::to_string(porteeAction(jeu, action));
        }
        DrawText(detail.c_str(), carte.x + 48, carte.y + 34, 10, dispo ? TEXTE_GRIS : Color{80, 78, 90, 255});
    }

    // ----- La bulle d'aide de la carte survolee -----
    if (survolee != -1) {
        Action action = actions[survolee];
        std::string texte = nomAction(action) + " : " + descriptionAction(action);
        int largeur = MeasureText(texte.c_str(), 20) + 24;
        float bx = LARGEUR_FENETRE - largeur - 8.0f;
        if (bx < 8) {
            bx = 8;
        }
        Rectangle bulle = {bx, y0 - 42.0f, (float)largeur, 32};
        dessinerCadre(bulle, Fade(PANNEAU, 0.95f), OR, 2);
        texteCentre(texte, bulle, 20, RAYWHITE);
    }
}

// Un grand message au milieu de l'arene (combat gagne, victoire, defaite)
void dessinerMessage(const std::string& titre, Color couleur, const std::string& aide) {
    DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_ARENE, Fade(BLACK, 0.55f));
    Rectangle cadre = {LARGEUR_FENETRE / 2.0f - 300, HAUTEUR_ARENE / 2.0f - 90, 600, 170};
    dessinerCadre(cadre, Fade(PANNEAU, 0.95f), couleur, 3);
    texteCentre(titre, {cadre.x, cadre.y + 30, cadre.width, 50}, 40, couleur);
    texteCentre(aide, {cadre.x, cadre.y + 105, cadre.width, 30}, 20, RAYWHITE);
}

// ===================== Les ecrans de la route : la vision et les runes =====================

const Color VIOLET_VISION = {175, 115, 240, 255};
const Color OR_EPIQUE = {255, 200, 80, 255};

// Les cartes de choix (1 a 3), centrees sur l'ecran
Rectangle rectangleCarteChoix(int numero, int nombre) {
    float largeur = 240;
    float ecart = 30;
    float total = nombre * largeur + (nombre - 1) * ecart;
    return {(LARGEUR_FENETRE - total) / 2 + numero * (largeur + ecart), 250, largeur, 300};
}

// Un fond de nuit violette, avec des lueurs qui montent doucement
void dessinerFondVision() {
    DrawRectangleGradientV(0, 0, LARGEUR_FENETRE, HAUTEUR_FENETRE, Color{28, 16, 44, 255}, Color{10, 8, 16, 255});
    float temps = GetTime();
    for (int i = 0; i < 40; i++) {
        float x = std::fmod(i * 137.0f, (float)LARGEUR_FENETRE);
        float y = HAUTEUR_FENETRE - std::fmod(i * 71.0f + temps * (12 + i % 5 * 5), (float)HAUTEUR_FENETRE);
        float lueur = 0.25f + 0.2f * std::sin(temps * 2 + i);
        DrawCircle(x, y, i % 3 == 0 ? 2.5f : 1.5f, Fade(VIOLET_VISION, lueur));
    }
}

// La frise de la route : une pierre par salle, reliees par un chemin. Les salles passees brillent.
void dessinerFrise(const Jeu& jeu) {
    float y = 200;
    float gauche = 150;
    float pas = (LARGEUR_FENETRE - 2 * gauche) / (NOMBRE_SALLES - 1);
    DrawLineEx({gauche, y}, {gauche + pas * (NOMBRE_SALLES - 1), y}, 3, Fade(BORD, 0.7f));
    for (int salle = 1; salle <= NOMBRE_SALLES; salle++) {
        float x = gauche + pas * (salle - 1);
        Color couleur = BORD;
        if (salle < jeu.salle) {
            couleur = VIOLET_VISION;
        } else if (salle == jeu.salle) {
            couleur = OR;
        }
        float rayon = salle == NOMBRE_SALLES ? 13.0f : 9.0f;
        if (salle == jeu.salle) {
            DrawCircle(x, y, rayon + 6 + 2 * std::sin(GetTime() * 4), Fade(OR, 0.25f));
        }
        DrawCircle(x, y, rayon, salle <= jeu.salle ? couleur : CADRE);
        DrawCircleLines(x, y, rayon, couleur);
    }
    // Le nom des lieux sous la frise, dans l'ordre de l'histoire
    const char* lieux[3] = {"Foret", "Camp", "Col"};
    for (int lieu = 0; lieu < 3; lieu++) {
        float x = gauche + pas * (lieu * 2 + 0.5f);
        if (lieu == 2) {
            x = gauche + pas * 5;
        }
        texteCentre(lieux[lieu], {x - 60, y + 16, 120, 20}, 10, lieu == jeu.lieu ? OR : TEXTE_GRIS);
    }
}

// Le glyphe d'une rune : un cercle et quelques traits, differents pour chaque rune
void dessinerGlyphe(float x, float y, float rayon, int numero, Color couleur) {
    DrawCircleLines(x, y, rayon, couleur);
    DrawCircleLines(x, y, rayon - 4, Fade(couleur, 0.5f));
    int branches = 3 + numero % 4;
    for (int i = 0; i < branches; i++) {
        float angle = i * 2 * PI / branches + numero * 0.7f;
        float interieur = (numero % 2 == 0) ? 0.0f : rayon * 0.35f;
        DrawLineEx({x + std::cos(angle) * interieur, y + std::sin(angle) * interieur},
                   {x + std::cos(angle) * (rayon - 8), y + std::sin(angle) * (rayon - 8)}, 3, couleur);
    }
    DrawCircle(x, y, 4, couleur);
}

// Une carte a choisir : un cadre qui se souleve au survol
Rectangle dessinerCarteChoix(int numero, int nombre, Color couleur) {
    Rectangle carte = rectangleCarteChoix(numero, nombre);
    bool survol = CheckCollisionPointRec(GetMousePosition(), carte);
    if (survol) {
        carte.y = carte.y - 6;
    }
    dessinerCadre(carte, survol ? CADRE_CLAIR : CADRE, couleur, survol ? 3 : 2);
    DrawText(TextFormat("%i", numero + 1), carte.x + 14, carte.y + 10, 20, Fade(couleur, 0.8f));
    return carte;
}

// En bas : ce que possede AYLIS (pv, stats, runes)
void dessinerResumeAylis(const Jeu& jeu, const std::string& aide, float yMessage = 150) {
    const Combattant& a = jeu.aylis.stats;
    std::string stats = TextFormat("AYLIS   %i/%i pv   attaque %i   defense %i   potions %i   %i or",
                                   a.pv, a.pvMax, a.attaque, a.defense, a.potions, a.pieces);
    texteCentre(stats, {0, 590, (float)LARGEUR_FENETRE, 24}, 20, RAYWHITE);
    std::string runes = "Runes : ";
    if (jeu.runes.empty()) {
        runes = runes + "aucune";
    }
    for (int i = 0; i < (int)jeu.runes.size(); i++) {
        runes = runes + (i > 0 ? ", " : "") + jeu.runes[i].nom;
    }
    texteCentre(runes, {0, 620, (float)LARGEUR_FENETRE, 24}, 20, VIOLET_VISION);
    texteCentre(jeu.messageRoute, {0, yMessage, (float)LARGEUR_FENETRE, 24}, 20, Color{120, 220, 140, 255});
    texteCentre(aide, {0, 700, (float)LARGEUR_FENETRE, 24}, 20, TEXTE_GRIS);
}

// L'image d'une salle, au milieu de sa carte
void dessinerImageSalle(TypeSalle type, int lieu, Rectangle zone) {
    const Sprites& s = sprites();
    Rectangle source = {0, 0, TAILLE_SPRITE, TAILLE_SPRITE};
    float centreX = zone.x + zone.width / 2;
    float centreY = zone.y + zone.height / 2;
    Rectangle ecran = {centreX - 48, centreY - 48, 96, 96};
    if (type == TypeSalle::Combat) {
        DrawTexturePro(s.guerrier, source, ecran, {0, 0}, 0, WHITE);
        DrawTexturePro(s.epee, source, ecran, {0, 0}, 0, WHITE);
    } else if (type == TypeSalle::Elite) {
        DrawCircle(centreX, centreY, 50, Fade(OR_EPIQUE, 0.15f));
        DrawTexturePro(s.louvetier, source, ecran, {0, 0}, 0, WHITE);
    } else if (type == TypeSalle::Repos) {
        DrawCircle(centreX, centreY + 10, 50, Fade(ORANGE, 0.12f + 0.05f * std::sin(GetTime() * 6)));
        DrawTexturePro(s.feu[(int)(GetTime() * 5) % 2], source, ecran, {0, 0}, 0, WHITE);
    } else if (type == TypeSalle::Marchand) {
        // Le marchand du lieu : Maren en foret, Durgan au camp, Silas sur le col
        DrawCircle(centreX, centreY, 50, Fade(OR, 0.15f));
        DrawTexturePro(s.marchands[lieu], source, ecran, {0, 0}, 0, WHITE);
    } else if (type == TypeSalle::Rencontre) {
        DrawCircle(centreX, centreY, 50, Fade(SKYBLUE, 0.12f));
        float saut = 4 * std::sin(GetTime() * 3);
        texteCentre("?", {centreX - 40, centreY - 40 + saut, 80, 80}, 80, Color{120, 200, 230, 255});
    } else if (type == TypeSalle::Oracle) {
        // Un oeil ouvert, entoure de la lumiere de la prophetie
        DrawCircle(centreX, centreY, 50, Fade(VIOLET_VISION, 0.15f));
        DrawEllipse(centreX, centreY, 44, 20, Fade(VIOLET_VISION, 0.9f));
        DrawEllipse(centreX, centreY, 36, 14, CADRE);
        DrawCircle(centreX, centreY, 11 + 2 * std::sin(GetTime() * 3), VIOLET_VISION);
        DrawCircle(centreX, centreY, 5, BLACK);
    } else {
        DrawCircle(centreX, centreY, 54, Fade(RED, 0.15f));
        DrawTexturePro(s.ashka, source, ecran, {0, 0}, 0, WHITE);
        DrawTexturePro(s.arc, source, ecran, {0, 0}, 0, WHITE);
        DrawTexturePro(s.couronne, source, ecran, {0, 0}, 0, WHITE);
    }
}

Color couleurSalle(TypeSalle type) {
    switch (type) {
        case TypeSalle::Combat: return Color{220, 90, 80, 255};
        case TypeSalle::Elite: return OR_EPIQUE;
        case TypeSalle::Repos: return Color{120, 210, 120, 255};
        case TypeSalle::Oracle: return VIOLET_VISION;
        case TypeSalle::Marchand: return Color{230, 200, 110, 255};
        case TypeSalle::Rencontre: return Color{120, 200, 230, 255};
        case TypeSalle::Boss: return RED;
    }
    return WHITE;
}

// Les mots d'une description, repartis sur plusieurs lignes pour tenir dans la carte
void texteSurPlusieursLignes(const std::string& texte, Rectangle zone, int taille, Color couleur) {
    std::vector<std::string> lignes;
    std::string ligne;
    std::string mot;
    for (int i = 0; i <= (int)texte.size(); i++) {
        if (i == (int)texte.size() || texte[i] == ' ') {
            std::string essai = ligne.empty() ? mot : ligne + " " + mot;
            if (MeasureText(essai.c_str(), taille) > zone.width && !ligne.empty()) {
                lignes.push_back(ligne);
                ligne = mot;
            } else {
                ligne = essai;
            }
            mot = "";
        } else {
            mot = mot + texte[i];
        }
    }
    lignes.push_back(ligne);
    for (int i = 0; i < (int)lignes.size(); i++) {
        texteCentre(lignes[i], {zone.x, zone.y + i * (taille + 6.0f), zone.width, (float)taille}, taille, couleur);
    }
}

void dessinerChoixSalle(const Jeu& jeu) {
    dessinerFondVision();
    const char* titre = jeu.salle >= NOMBRE_SALLES ? "LA VISION S'ASSOMBRIT" : "UNE VISION";
    texteCentre(titre, {0, 40, (float)LARGEUR_FENETRE, 50}, 50, VIOLET_VISION);
    texteCentre(TextFormat("Salle %i/%i  -  %s", jeu.salle, NOMBRE_SALLES, nomLieu(jeu.lieu).c_str()),
                {0, 100, (float)LARGEUR_FENETRE, 24}, 20, LIGHTGRAY);
    dessinerFrise(jeu);

    int nombre = jeu.propositions.size();
    for (int i = 0; i < nombre; i++) {
        TypeSalle type = jeu.propositions[i].type;
        Color couleur = couleurSalle(type);
        Rectangle carte = dessinerCarteChoix(i, nombre, couleur);
        dessinerImageSalle(type, jeu.lieu, {carte.x, carte.y + 30, carte.width, 120});
        texteCentre(nomTypeSalle(type), {carte.x, carte.y + 165, carte.width, 30}, 30, couleur);
        texteSurPlusieursLignes(descriptionSalle(type), {carte.x + 18, carte.y + 215, carte.width - 36, 60}, 20, RAYWHITE);
    }
    dessinerResumeAylis(jeu, nombre == 1 ? "Clique sur la carte, ou tape 1" : "Ou aller ? Clique sur une carte, ou tape son numero");
}

void dessinerChoixRune(const Jeu& jeu) {
    dessinerFondVision();
    texteCentre("RUNES DE PROPHETIE", {0, 40, (float)LARGEUR_FENETRE, 50}, 50, VIOLET_VISION);
    texteCentre("Choisis une rune : AYLIS la garde jusqu'a la fin de la route", {0, 100, (float)LARGEUR_FENETRE, 24}, 20,
                LIGHTGRAY);
    dessinerFrise(jeu);

    int nombre = jeu.runesProposees.size();
    for (int i = 0; i < nombre; i++) {
        const Rune& rune = jeu.runesProposees[i];
        Color couleur = rune.epique ? OR_EPIQUE : VIOLET_VISION;
        Rectangle carte = dessinerCarteChoix(i, nombre, couleur);
        float centreX = carte.x + carte.width / 2;
        float pulsation = 0.12f + 0.06f * std::sin(GetTime() * 3 + i);
        DrawCircle(centreX, carte.y + 90, 58, Fade(couleur, pulsation));
        dessinerGlyphe(centreX, carte.y + 90, 46, rune.numero, couleur);
        if (rune.epique) {
            texteCentre("EPIQUE", {carte.x, carte.y + 148, carte.width, 20}, 20, OR_EPIQUE);
        }
        texteCentre(rune.nom, {carte.x, carte.y + 172, carte.width, 30}, rune.nom.size() > 12 ? 20 : 30, couleur);
        texteSurPlusieursLignes(rune.description, {carte.x + 18, carte.y + 220, carte.width - 36, 60}, 20, RAYWHITE);
    }
    dessinerResumeAylis(jeu, "Clique sur une rune, ou tape son numero");
}

// ===================== Les haltes : la boutique et les rencontres =====================

// Le bouton pour quitter la boutique ou continuer apres une rencontre
Rectangle rectangleBoutonRoute() {
    return {LARGEUR_FENETRE / 2.0f - 140, 648, 280, 40};
}

Rectangle rectangleCarteReponse(int numero) {
    return {LARGEUR_FENETRE / 2.0f - 380 + numero * 390, 270, 370, 180};
}

void dessinerBoutonRoute(const std::string& texte) {
    Rectangle bouton = rectangleBoutonRoute();
    bool survol = CheckCollisionPointRec(GetMousePosition(), bouton);
    dessinerCadre(bouton, survol ? CADRE_CLAIR : CADRE, OR, survol ? 3 : 2);
    texteCentre(texte, bouton, 20, OR);
}

// Un fond chaud, avec des braises qui montent
void dessinerFondHalte() {
    DrawRectangleGradientV(0, 0, LARGEUR_FENETRE, HAUTEUR_FENETRE, Color{40, 26, 18, 255}, Color{12, 9, 8, 255});
    float temps = GetTime();
    for (int i = 0; i < 30; i++) {
        float x = std::fmod(i * 151.0f + std::sin(temps + i) * 10, (float)LARGEUR_FENETRE);
        float y = HAUTEUR_FENETRE - std::fmod(i * 67.0f + temps * (18 + i % 4 * 6), (float)HAUTEUR_FENETRE);
        DrawCircle(x, y, i % 3 == 0 ? 2.0f : 1.2f, Fade(ORANGE, 0.3f + 0.2f * std::sin(temps * 3 + i)));
    }
}

void dessinerBoutique(const Jeu& jeu) {
    dessinerFondHalte();
    const Sprites& s = sprites();

    // Le marchand et ce qu'il dit
    DrawCircle(110, 100, 66, Fade(OR, 0.12f));
    DrawTexturePro(s.marchands[jeu.marchand], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, {46, 36, 128, 128}, {0, 0}, 0, WHITE);
    DrawText(nomMarchand(jeu.marchand).c_str(), 200, 36, 40, OR);
    DrawText(titreMarchand(jeu.marchand).c_str(), 204, 80, 20, TEXTE_GRIS);
    Rectangle bulle = {200, 110, LARGEUR_FENETRE - 240.0f, 76};
    dessinerCadre(bulle, Fade(PANNEAU, 0.9f), Fade(OR, 0.6f), 2);
    texteSurPlusieursLignes("\"" + paroleMarchand(jeu.marchand) + "\"", {bulle.x + 14, bulle.y + 12, bulle.width - 28, 60}, 20,
                            RAYWHITE);

    int nombre = jeu.articles.size();
    for (int i = 0; i < nombre; i++) {
        const Article& article = jeu.articles[i];
        bool assezDOr = jeu.aylis.stats.pieces >= article.prix;
        Color couleur = article.vendu ? BORD : (assezDOr ? OR : Color{190, 90, 80, 255});
        Rectangle carte = dessinerCarteChoix(i, nombre, couleur);
        texteSurPlusieursLignes(article.nom, {carte.x + 14, carte.y + 50, carte.width - 28, 70}, 30,
                                article.vendu ? TEXTE_GRIS : RAYWHITE);
        texteSurPlusieursLignes(article.description, {carte.x + 18, carte.y + 140, carte.width - 36, 80}, 20, TEXTE_GRIS);
        if (article.vendu) {
            texteCentre("VENDU", {carte.x, carte.y + 235, carte.width, 40}, 30, BORD);
        } else {
            // Le prix, avec une petite piece d'or
            std::string prix = std::to_string(article.prix) + " or";
            int largeur = MeasureText(prix.c_str(), 30) + 30;
            float x = carte.x + (carte.width - largeur) / 2;
            DrawCircle(x + 11, carte.y + 255, 11, OR);
            DrawCircle(x + 11, carte.y + 255, 6, Color{190, 150, 50, 255});
            DrawText(prix.c_str(), x + 30, carte.y + 240, 30, couleur);
        }
    }
    dessinerResumeAylis(jeu, "", 212);
    dessinerBoutonRoute(TextFormat("%i. Reprendre la route", nombre + 1));
}

void dessinerRencontre(const Jeu& jeu) {
    dessinerFondVision();
    const Color BLEU = {120, 200, 230, 255};
    texteCentre(titreRencontre(jeu.rencontre), {0, 36, (float)LARGEUR_FENETRE, 44}, 40, BLEU);
    Rectangle recit = {90, 100, LARGEUR_FENETRE - 180.0f, 100};
    dessinerCadre(recit, Fade(PANNEAU, 0.9f), Fade(BLEU, 0.6f), 2);
    texteSurPlusieursLignes(texteRencontre(jeu.rencontre), {recit.x + 20, recit.y + 16, recit.width - 40, 70}, 20, RAYWHITE);

    if (jeu.resultatRencontre.empty()) {
        for (int i = 0; i < 2; i++) {
            Rectangle carte = rectangleCarteReponse(i);
            bool survol = CheckCollisionPointRec(GetMousePosition(), carte);
            if (survol) {
                carte.y = carte.y - 4;
            }
            dessinerCadre(carte, survol ? CADRE_CLAIR : CADRE, BLEU, survol ? 3 : 2);
            DrawText(TextFormat("%i", i + 1), carte.x + 14, carte.y + 10, 20, Fade(BLEU, 0.8f));
            texteSurPlusieursLignes(reponseRencontre(jeu, jeu.rencontre, i), {carte.x + 24, carte.y + 50, carte.width - 48, 80},
                                    30, RAYWHITE);
        }
        dessinerResumeAylis(jeu, "Que fait AYLIS ? Clique sur un choix, ou tape 1 ou 2", 212);
    } else {
        Rectangle resultat = {120, 280, LARGEUR_FENETRE - 240.0f, 150};
        dessinerCadre(resultat, Fade(PANNEAU, 0.95f), Color{120, 220, 140, 255}, 3);
        texteSurPlusieursLignes(jeu.resultatRencontre, {resultat.x + 24, resultat.y + 40, resultat.width - 48, 80}, 20,
                                Color{120, 220, 140, 255});
        dessinerResumeAylis(jeu, "", 212);
        dessinerBoutonRoute("ENTREE : continuer");
    }
}

void dessinerJeu(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    ClearBackground(FOND);

    if (jeu.phase == Phase::ChoixVoie) {
        dessinerChoixVoie();
        return;
    }
    if (jeu.phase == Phase::ChoixSalle) {
        dessinerChoixSalle(jeu);
        return;
    }
    if (jeu.phase == Phase::ChoixRune) {
        dessinerChoixRune(jeu);
        return;
    }
    if (jeu.phase == Phase::Marchand) {
        dessinerBoutique(jeu);
        return;
    }
    if (jeu.phase == Phase::Rencontre) {
        dessinerRencontre(jeu);
        return;
    }

    // L'arene est dessinee a travers une "camera" qui tremble quand un coup porte.
    // L'interface (bandeau, panneau...) reste immobile : elle est dessinee apres, hors camera.
    BeginMode2D(cameraAvecSecousse(jeu));
    dessinerArene(jeu, colonneSouris, ligneSouris);
    EndMode2D();

    dessinerBandeau(jeu);
    dessinerJournal(jeu);
    dessinerConsigne(jeu);
    dessinerFicheHaschen(jeu, colonneSouris, ligneSouris);
    dessinerBanniere(jeu);
    dessinerPanneau(jeu);

    // Les grands messages attendent la fin des animations (le dernier Haschen doit finir de tomber)
    if (animationsEnCours(jeu)) {
        return;
    }
    if (jeu.phase == Phase::CombatGagne) {
        dessinerMessage(jeu.nomDuLieu + " : victoire !", Color{110, 220, 120, 255}, "Appuie sur ENTREE pour choisir ta rune");
    } else if (jeu.phase == Phase::Victoire) {
        dessinerMessage("ASHKA EST VAINCUE !", OR, "Fin du prototype - appuie sur R pour rejouer");
    } else if (jeu.phase == Phase::Defaite) {
        dessinerMessage("AYLIS TOMBE AU COMBAT...", Color{220, 80, 80, 255}, "Appuie sur R pour rejouer");
    }
}
