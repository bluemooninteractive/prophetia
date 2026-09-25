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

void dessinerChoixVoie(const Jeu& jeu) {
    const char* titre = "VESPERANCE";
    DrawText(titre, (LARGEUR_FENETRE - MeasureText(titre, 70)) / 2, 40, 70, Color{170, 110, 240, 255});
    std::string phrase = phraseDeDepart(jeu.memoire);     // elle change avec les souvenirs d'AYLIS
    const char* sousTitre = phrase.c_str();
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
        {"34 pv  -  attaque 12", "defense 3  -  20 mana", "Peu de pv, beaucoup de magie.", "Connait les 4 sorts !"},
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

    // Les souvenirs des courses precedentes
    const Memoire& m = jeu.memoire;
    if (m.visions > 0 || m.victoires > 0) {
        Rectangle cadre = {40, 640, LARGEUR_FENETRE - 80.0f, 90};
        dessinerCadre(cadre, Fade(PANNEAU, 0.9f), Fade(Color{175, 115, 240, 255}, 0.6f), 2);
        texteCentre("SOUVENIRS DE LA PROPHETIE", {cadre.x, cadre.y + 10, cadre.width, 20}, 20, Color{175, 115, 240, 255});
        std::string ligne = TextFormat("Visions : %i   Record : acte %i   Vorgath vaincu : %i   Fragments : %i",
                                       m.visions, acteDeLaSalle(m.meilleureSalle > 0 ? m.meilleureSalle : 1) + 1,
                                       m.victoires, m.fragments);
        texteCentre(ligne, {cadre.x, cadre.y + 50, cadre.width, 20}, 20, RAYWHITE);
    }
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
    float taille = 64.0f;
    if (pion.boss == BOSS_VORGATH) {
        taille = 104.0f;        // Vorgath domine l'arene
    } else if (pion.stats.estBoss) {
        taille = 84.0f;
    } else if (pion.stats.nom.find("elite") != std::string::npos || pion.stats.nom.find("brute") != std::string::npos) {
        taille = 74.0f;
    }

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

    if (pion.stats.nom == "Kerrak" || pion.stats.nom == "Brenna") {
        // Les compagnons : Kerrak est un eclaireur haschen, Brenna une mercenaire en rouge et or
        // Un cercle bleu a leurs pieds : ils sont du cote d'AYLIS
        DrawEllipseLines(centreX, basY - 2, taille * 0.34f, 8, Fade(Color{120, 180, 255, 255}, 0.8f * visibilite));
        bool kerrak = pion.stats.nom == "Kerrak";
        Color teinteCompagnon = kerrak ? teinte : Color{(unsigned char)(teinte.r), (unsigned char)(teinte.g * 0.72f),
                                                        (unsigned char)(teinte.b * 0.5f), teinte.a};
        dessinerSprite(kerrak ? s.eclaireur : s.aylis, ecran, false, teinteCompagnon);
        dessinerSprite(s.epee, ecran, false, teinte);
        return;
    }
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
        const Texture2D* coiffe = spriteCoiffe(pion.stats.nom);
        if (coiffe != nullptr) {
            dessinerSprite(*coiffe, ecran, versLaGauche, teinte);
        }
    }
}

// ===================== La lumiere =====================
//
// L'astuce qui donne toute l'ambiance : on dessine d'abord une "carte de lumiere", une image a part
// (une RenderTexture) remplie d'une couleur sombre, sur laquelle on ajoute des taches de lumiere
// (AYLIS, les feux, les champignons, les cristaux...). Ensuite on la pose par-dessus l'arene en mode
// "multiplication" : ou la carte est sombre l'arene devient sombre, ou elle est claire l'arene reste eclairee.

RenderTexture2D carteLumiere;
bool carteLumiereCreee = false;

// La couleur de la penombre de chaque lieu : nuit bleutee en foret, braise au camp, glace sur le col
const Color PENOMBRE[NOMBRE_LIEUX] = {
    {42, 54, 84, 255},      // foret : nuit bleutee
    {84, 62, 80, 255},      // village : un crepuscule mauve
    {44, 70, 94, 255},      // gue : bleu d'eau
    {28, 32, 42, 255},      // bois des Pendus : presque le noir
    {74, 48, 62, 255},      // camp : la braise
    {58, 54, 78, 255},      // forteresse : pierre froide
    {56, 60, 100, 255},     // col : la glace
    {78, 38, 36, 255},      // cendres : le rougeoiement de la lave
    {46, 30, 58, 255},      // citadelle : obsidienne et braise
};
// La couleur des sources de lumiere de chaque lieu
const Color LUEUR[NOMBRE_LIEUX] = {{90, 230, 240, 255}, {255, 196, 120, 255}, {140, 240, 215, 255}, {170, 255, 110, 255},
                                   {255, 150, 60, 255}, {255, 165, 80, 255}, {190, 120, 255, 255},
                                   {255, 120, 40, 255}, {255, 110, 60, 255}};

// Une tache de lumiere ronde, forte au centre et qui s'efface vers le bord
void tacheDeLumiere(float x, float y, float rayon, Color couleur, float force) {
    DrawCircleGradient(x, y, rayon, Fade(couleur, force), Fade(couleur, 0.0f));
}

// Un petit tremblement, comme une flamme (chaque source a son propre rythme)
float vacillement(float vitesse, float decalage) {
    float t = GetTime() * vitesse + decalage;
    return 0.85f + 0.1f * std::sin(t) + 0.05f * std::sin(t * 2.7f);
}

Vector2 centreDeCase(int colonne, int ligne) {
    return {colonne * TAILLE_CASE + TAILLE_CASE / 2.0f, ligne * TAILLE_CASE + TAILLE_CASE / 2.0f};
}

// A appeler AVANT BeginMode2D : on dessine dans la carte de lumiere, pas sur l'ecran
void preparerLumiere(const Jeu& jeu) {
    if (!carteLumiereCreee) {
        carteLumiere = LoadRenderTexture(LARGEUR_FENETRE, HAUTEUR_ARENE);
        SetTextureFilter(carteLumiere.texture, TEXTURE_FILTER_BILINEAR);
        carteLumiereCreee = true;
    }
    int lieu = jeu.lieu;
    BeginTextureMode(carteLumiere);
    ClearBackground(PENOMBRE[lieu]);
    BeginBlendMode(BLEND_ADDITIVE);     // les lumieres s'ajoutent les unes aux autres

    // Sur le col, un clair de lune qui tombe d'en haut
    if (lieu == 6) {
        tacheDeLumiere(LARGEUR_FENETRE / 2.0f, -120, 620, Color{150, 170, 255, 255}, 0.35f);
    }
    // En foret, les rayons de lune qui percent les arbres
    if (lieu == 0) {
        for (int i = 0; i < 3; i++) {
            float x = 180.0f + i * 290 + std::sin(GetTime() * 0.3f + i) * 20;
            tacheDeLumiere(x, 250 + i * 60.0f, 170, Color{170, 220, 200, 255}, 0.18f);
        }
    }

    // AYLIS porte la lumiere : un grand halo chaud, et un coeur violet (la prophetie)
    if (jeu.aylis.stats.estDebout()) {
        Vector2 a = positionAffichee(jeu.aylis);
        tacheDeLumiere(a.x, a.y, 250, Color{255, 236, 210, 255}, 0.75f);
        tacheDeLumiere(a.x, a.y + 10, 90, Color{190, 130, 255, 255}, 0.45f);
    }
    // Les Haschen : juste assez de lumiere pour deviner leurs silhouettes
    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout()) {
            Vector2 p = positionAffichee(h);
            tacheDeLumiere(p.x, p.y, 95, Color{255, 190, 170, 255}, h.stats.estBoss ? 0.5f : 0.35f);
        }
    }
    // Les fenetres allumees du village
    ajouterLumieresDuDecor(jeu);
    // Les sources de lumiere du lieu
    for (int i = 0; i < (int)jeu.lumieres.size(); i++) {
        Vector2 p = centreDeCase(jeu.lumieres[i].first, jeu.lumieres[i].second);
        bool flamme = lieu == 1 || lieu == 4 || lieu == 5 || lieu >= 7;     // lanternes, braseros, torches, lave : ca vacille
        float force = flamme ? vacillement(9, i * 2.1f) : 0.8f + 0.2f * std::sin(GetTime() * 1.5f + i);
        tacheDeLumiere(p.x, p.y, 200, LUEUR[lieu], 0.8f * force);
    }
    // Le grand feu du camp
    if (jeu.feuColonne >= 0) {
        Vector2 p = centreDeCase(jeu.feuColonne, jeu.feuLigne);
        tacheDeLumiere(p.x, p.y, 330 * vacillement(7, 0), Color{255, 140, 50, 255}, 0.95f);
    }
    // Les projectiles magiques eclairent leur chemin
    for (const Projectile& p : jeu.projectiles) {
        float t = p.duree > 0 ? p.temps / p.duree : 1;
        float x = p.departX + (p.arriveeX - p.departX) * t;
        float y = p.departY + (p.arriveeY - p.departY) * t;
        if (p.sorte == SorteProjectile::BouleDeFeu) {
            tacheDeLumiere(x, y, 190, Color{255, 140, 40, 255}, 0.9f);
        } else if (p.sorte == SorteProjectile::Orbe) {
            tacheDeLumiere(x, y, 120, Color{190, 130, 255, 255}, 0.7f);
        }
    }
    // Les eclairs illuminent tout
    for (const EffetEclair& e : jeu.eclairs) {
        tacheDeLumiere(e.x, e.y, 420, Color{210, 230, 255, 255}, e.vie * 3);
    }
    EndBlendMode();
    EndTextureMode();
}

// Pose la carte de lumiere sur l'arene (a appeler DANS BeginMode2D, pour qu'elle tremble avec l'arene)
void appliquerLumiere() {
    if (!carteLumiereCreee) {
        return;
    }
    BeginBlendMode(BLEND_MULTIPLIED);
    // Une RenderTexture est rangee "a l'envers" : une hauteur negative la remet a l'endroit
    Rectangle source = {0, 0, (float)LARGEUR_FENETRE, -(float)HAUTEUR_ARENE};
    DrawTextureRec(carteLumiere.texture, source, {0, 0}, WHITE);
    EndBlendMode();
}

void dechargerLumiere() {
    if (carteLumiereCreee) {
        UnloadRenderTexture(carteLumiere);
        carteLumiereCreee = false;
    }
}

// ===================== L'ambiance magique =====================

// Un nombre "au hasard" mais toujours le meme pour un meme numero (pour placer lucioles, braises...)
float hasardFixe(int numero) {
    float valeur = std::sin(numero * 12.9898f) * 43758.5453f;
    return valeur - std::floor(valeur);     // la partie apres la virgule : entre 0 et 1
}

// Le cercle de runes de la prophetie, au sol sous AYLIS : il tourne doucement
void dessinerCercleDeRunes(const Jeu& jeu) {
    if (!jeu.aylis.stats.estDebout()) {
        return;
    }
    Vector2 a = positionAffichee(jeu.aylis);
    float cx = a.x;
    float cy = a.y + TAILLE_CASE / 2.0f - 8;
    float temps = GetTime();
    Color violet = {190, 130, 255, 255};
    BeginBlendMode(BLEND_ADDITIVE);
    for (int anneau = 0; anneau < 2; anneau++) {
        float rayon = 36.0f - anneau * 10;
        float sens = anneau == 0 ? 0.4f : -0.7f;
        // Un anneau = une ellipse (le sol est vu de biais) faite de petits traits
        for (int i = 0; i < 24; i++) {
            float angle = i * 2 * PI / 24 + temps * sens;
            float angle2 = angle + (i % 3 == 0 ? 0.18f : 0.1f);
            DrawLineEx({cx + std::cos(angle) * rayon, cy + std::sin(angle) * rayon * 0.42f},
                       {cx + std::cos(angle2) * rayon, cy + std::sin(angle2) * rayon * 0.42f}, 2,
                       Fade(violet, 0.55f - anneau * 0.15f));
        }
    }
    // Quatre runes qui brillent sur l'anneau exterieur
    for (int i = 0; i < 4; i++) {
        float angle = i * PI / 2 + temps * 0.4f;
        DrawCircle(cx + std::cos(angle) * 36, cy + std::sin(angle) * 36 * 0.42f, 2.5f, Fade(WHITE, 0.8f));
    }
    EndBlendMode();
}

// Les sources de lumiere : leurs dessins avec le decor (avant la penombre),
// puis leur halo par-dessus la penombre (halo = true) : c'est ce qui les fait briller
void dessinerSourcesDeLumiere(const Jeu& jeu, bool halo) {
    const Sprites& s = sprites();
    int lieu = jeu.lieu;
    Rectangle source = {0, 0, TAILLE_SPRITE, TAILLE_SPRITE};
    if (halo) {
        BeginBlendMode(BLEND_ADDITIVE);
    }
    for (int i = 0; i < (int)jeu.lumieres.size(); i++) {
        Vector2 p = centreDeCase(jeu.lumieres[i].first, jeu.lumieres[i].second);
        if (halo) {
            float pulsation = 0.28f + 0.08f * std::sin(GetTime() * 2 + i);
            DrawCircleGradient(p.x, p.y + 4, 40, Fade(LUEUR[lieu], pulsation), Fade(LUEUR[lieu], 0.0f));
        } else {
            DrawTexturePro(s.lumineux[lieu], source, {p.x - 32, p.y - 32, 64, 64}, {0, 0}, 0, WHITE);
        }
    }
    if (jeu.feuColonne >= 0) {
        Vector2 p = centreDeCase(jeu.feuColonne, jeu.feuLigne);
        if (halo) {
            DrawCircleGradient(p.x, p.y, 60 * vacillement(8, 1), Fade(ORANGE, 0.45f), Fade(ORANGE, 0.0f));
        } else {
            int image = (int)(GetTime() * 5) % 2;
            DrawTexturePro(s.feu[image], source, {p.x - 36, p.y - 36, 72, 72}, {0, 0}, 0, WHITE);
        }
    }
    if (halo) {
        EndBlendMode();
    }
}

// ----- Les petits morceaux d'ambiance, reutilises par plusieurs lieux -----

// Des lucioles qui errent et clignotent
void dessinerLucioles(int nombre, Color couleur) {
    float temps = GetTime();
    for (int i = 0; i < nombre; i++) {
        float x = hasardFixe(i) * LARGEUR_FENETRE + std::sin(temps * 0.6f + i) * 40;
        float y = hasardFixe(i + 100) * HAUTEUR_ARENE + std::cos(temps * 0.45f + i * 1.3f) * 30;
        float eclat = std::fmax(0.0f, std::sin(temps * 2.2f + i * 0.9f));
        DrawCircleGradient(x, y, 9, Fade(couleur, 0.35f * eclat), BLANK);
        DrawCircle(x, y, 1.6f, Fade(WHITE, eclat * 0.9f));
    }
}

// Des braises qui montent des feux, des braseros et des torches
void dessinerBraises(const Jeu& jeu) {
    float temps = GetTime();
    std::vector<Vector2> foyers;
    for (const auto& l : jeu.lumieres) {
        foyers.push_back(centreDeCase(l.first, l.second));
    }
    if (jeu.feuColonne >= 0) {
        foyers.push_back(centreDeCase(jeu.feuColonne, jeu.feuLigne));
    }
    for (int f = 0; f < (int)foyers.size(); f++) {
        for (int i = 0; i < 10; i++) {
            int n = f * 10 + i;
            float montee = std::fmod(temps * (30 + hasardFixe(n) * 40) + hasardFixe(n + 50) * 200, 160.0f);
            float x = foyers[f].x + std::sin(temps * 2 + n) * 10 + (hasardFixe(n + 7) - 0.5f) * 30;
            float y = foyers[f].y - 20 - montee;
            float vie = 1.0f - montee / 160.0f;
            DrawCircle(x, y, 1.5f + vie, Fade(Color{255, 170, 70, 255}, vie));
        }
    }
}

// Des chauves-souris qui traversent l'ecran en battant des ailes
void dessinerChauvesSouris() {
    float temps = GetTime();
    for (int i = 0; i < 4; i++) {
        float x = std::fmod(temps * (60 + i * 15) + i * 300, LARGEUR_FENETRE + 200.0f) - 100;
        float y = 60 + i * 110 + std::sin(temps * 2 + i) * 30;
        float battement = std::sin(temps * 18 + i * 2) * 6;
        Color noir = {12, 10, 18, 230};
        DrawTriangle({x, y}, {x - 12, y - 4 + battement}, {x - 4, y + 3}, noir);
        DrawTriangle({x, y}, {x + 4, y + 3}, {x + 12, y - 4 + battement}, noir);
        DrawCircle(x, y + 1, 3, noir);
    }
}

// Des yeux rouges qui s'ouvrent un instant dans l'obscurite, tout autour du bois des Pendus
void dessinerYeuxDansLeNoir() {
    float temps = GetTime();
    for (int i = 0; i < 9; i++) {
        // Toujours pres des bords, la ou il fait le plus noir
        float x = i % 2 == 0 ? 20 + hasardFixe(i) * 70 : LARGEUR_FENETRE - 90 + hasardFixe(i) * 70;
        float y = 40 + hasardFixe(i + 30) * (HAUTEUR_ARENE - 80);
        float cycle = std::fmod(temps * 0.35f + hasardFixe(i + 60) * 7, 7.0f);
        float ouverture = cycle < 2.5f ? std::sin(cycle / 2.5f * PI) : 0;
        if (ouverture <= 0.05f) {
            continue;
        }
        Color rouge = {255, 50, 50, 255};
        DrawCircleGradient(x, y, 14, Fade(rouge, 0.3f * ouverture), BLANK);
        DrawCircleGradient(x + 16, y, 14, Fade(rouge, 0.3f * ouverture), BLANK);
        DrawRectangleRec({x - 3, y - 1.5f * ouverture, 6, 3 * ouverture}, Fade(rouge, ouverture));
        DrawRectangleRec({x + 13, y - 1.5f * ouverture, 6, 3 * ouverture}, Fade(rouge, ouverture));
    }
}

void dessinerAmbiance(const Jeu& jeu) {
    int lieu = jeu.lieu;
    float temps = GetTime();
    BeginBlendMode(BLEND_ADDITIVE);

    if (lieu == 0) {
        // La foret : des rayons de lune en biais, et des lucioles
        for (int i = 0; i < 3; i++) {
            float x = 120.0f + i * 290 + std::sin(temps * 0.3f + i) * 20;
            float force = 0.022f + 0.012f * std::sin(temps * 0.7f + i * 2);
            Color couleur = Fade(Color{200, 240, 220, 255}, force);
            DrawTriangle({x, 0}, {x + 150, (float)HAUTEUR_ARENE}, {x + 290, (float)HAUTEUR_ARENE}, couleur);
            DrawTriangle({x, 0}, {x + 290, (float)HAUTEUR_ARENE}, {x + 70, 0}, couleur);
        }
        dessinerLucioles(28, Color{200, 255, 120, 255});
    } else if (lieu == 1) {
        // Le village : des papillons de nuit, et des braises au-dessus des maisons brulees
        dessinerLucioles(14, Color{255, 220, 150, 255});
        for (const Vector2& f : cheminsDeFumee(jeu)) {
            for (int i = 0; i < 6; i++) {
                float montee = std::fmod(temps * 25 + i * 20, 120.0f);
                DrawCircle(f.x + std::sin(temps * 3 + i) * 8, f.y + 14 - montee, 1.8f, Fade(ORANGE, 1 - montee / 120));
            }
        }
    } else if (lieu == 2) {
        // Le gue : des feux follets bleus qui dansent au-dessus de l'eau
        dessinerLucioles(22, Color{120, 240, 255, 255});
    } else if (lieu == 3) {
        // Le bois des Pendus : des feux follets verdatres, et des yeux dans le noir
        dessinerLucioles(10, Color{170, 255, 110, 255});
        dessinerYeuxDansLeNoir();
    } else if (lieu == 4 || lieu == 5 || lieu >= 7) {
        // Le camp, la forteresse, et le domaine de Vorgath : des braises qui montent des feux et de la lave
        dessinerBraises(jeu);
        if (lieu >= 7) {
            for (int i = 0; i < 40; i++) {
                float x = std::fmod(hasardFixe(i + 700) * LARGEUR_FENETRE + std::sin(temps * 0.7f + i) * 30, (float)LARGEUR_FENETRE);
                float y = std::fmod(hasardFixe(i + 800) * HAUTEUR_ARENE - temps * (14 + i % 5 * 6) + HAUTEUR_ARENE * 8,
                                    (float)HAUTEUR_ARENE);
                DrawCircle(x, y, 1.6f, Fade(Color{255, 150, 60, 255}, 0.5f + 0.4f * std::sin(temps * 3 + i)));
            }
        }
    } else {
        // Le col : une aurore violette et verte, et des volutes autour des cristaux
        for (int x = 0; x < LARGEUR_FENETRE; x = x + 6) {
            float onde = std::sin(x * 0.012f + temps * 0.6f) * 0.5f + 0.5f;
            float hauteur = 70 + 50 * std::sin(x * 0.02f + temps * 0.4f);
            Color couleur = onde > 0.5f ? Color{140, 90, 255, 255} : Color{80, 230, 170, 255};
            DrawRectangleGradientV(x, 0, 6, hauteur, Fade(couleur, 0.10f * onde + 0.03f), BLANK);
        }
        for (int f = 0; f < (int)jeu.lumieres.size(); f++) {
            Vector2 c = centreDeCase(jeu.lumieres[f].first, jeu.lumieres[f].second);
            for (int i = 0; i < 6; i++) {
                float angle = temps * (0.8f + i * 0.1f) + i * PI / 3 + f;
                DrawCircle(c.x + std::cos(angle) * (22 + i * 3), c.y - 4 + std::sin(angle) * 10 - i * 3, 1.8f,
                           Fade(Color{220, 180, 255, 255}, 0.8f));
            }
        }
    }

    // Partout : une poussiere de prophetie, quelques grains violets qui flottent
    for (int i = 0; i < 18; i++) {
        float x = std::fmod(hasardFixe(i + 300) * LARGEUR_FENETRE + temps * (6 + i % 4 * 3), (float)LARGEUR_FENETRE);
        float y = std::fmod(hasardFixe(i + 400) * HAUTEUR_ARENE - temps * (8 + i % 3 * 4) + HAUTEUR_ARENE * 4,
                            (float)HAUTEUR_ARENE);
        float eclat = 0.4f + 0.4f * std::sin(temps * 1.7f + i);
        DrawCircle(x, y, 1.5f, Fade(Color{200, 160, 255, 255}, eclat));
    }
    EndBlendMode();

    // ----- Ce qui est dessine "normalement" (pas en lumiere) -----
    // La fumee grise qui monte des maisons brulees du village
    if (lieu == 1) {
        for (const Vector2& f : cheminsDeFumee(jeu)) {
            for (int i = 0; i < 7; i++) {
                float montee = std::fmod(temps * 16 + i * 26, 180.0f);
                float vie = 1 - montee / 180;
                DrawCircle(f.x + std::sin(temps * 0.8f + i) * 14 + montee * 0.3f, f.y - montee, 10 + montee * 0.12f,
                           Fade(Color{70, 66, 74, 255}, 0.35f * vie));
            }
        }
    }
    // Des feuilles mortes qui tombent en se balancant, dans le bois des Pendus
    if (lieu == 3) {
        for (int i = 0; i < 16; i++) {
            float chute = std::fmod(temps * (20 + i % 5 * 6) + hasardFixe(i + 200) * HAUTEUR_ARENE, (float)HAUTEUR_ARENE);
            float x = hasardFixe(i + 210) * LARGEUR_FENETRE + std::sin(temps * 1.5f + i) * 20;
            DrawRectangleRec({x, chute, 5, 3}, Fade(i % 2 ? Color{150, 90, 40, 255} : Color{100, 70, 40, 255}, 0.8f));
        }
    }
    if (lieu == 5 || lieu == 8) {
        dessinerChauvesSouris();
    }
    // De la cendre grise qui tombe doucement, dans le domaine de Vorgath
    if (lieu >= 7) {
        for (int i = 0; i < 50; i++) {
            float x = std::fmod(i * 131.0f + temps * (8 + i % 4 * 3), (float)LARGEUR_FENETRE);
            float y = std::fmod(i * 67.0f + temps * (20 + i % 6 * 5), (float)HAUTEUR_ARENE);
            DrawRectangle(x, y, 2, 2, Fade(Color{150, 140, 140, 255}, 0.6f));
        }
    }
    // La brume : au ras du sol partout, epaisse en foret, sur le gue et dans le bois des Pendus
    bool brumeEpaisse = lieu == 0 || lieu == 2 || lieu == 3;
    int nappes = lieu == 3 ? 11 : 7;
    for (int i = 0; i < nappes; i++) {
        float x = std::fmod(temps * (10 + i * 3) + i * 170, LARGEUR_FENETRE + 400.0f) - 200;
        float y = 60 + i * (HAUTEUR_ARENE - 80.0f) / nappes + std::sin(temps * 0.5f + i) * 20;
        Color couleur = lieu == 3 ? Color{150, 170, 160, 255} : Color{190, 215, 230, 255};
        DrawEllipse(x, y, 190, 36, Fade(couleur, brumeEpaisse ? 0.065f : 0.03f));
    }
    // La neige sur le col
    if (lieu == 6) {
        for (int i = 0; i < 60; i++) {
            float x = std::fmod(i * 97.0f + temps * (10 + i % 5 * 4), (float)LARGEUR_FENETRE);
            float y = std::fmod(i * 53.0f + temps * (35 + i % 7 * 6), (float)HAUTEUR_ARENE);
            DrawRectangle(x, y, i % 3 == 0 ? 3 : 2, i % 3 == 0 ? 3 : 2, Fade(WHITE, 0.7f));
        }
    }
    // Un vignettage : les bords s'enfoncent dans le noir, l'oeil reste au centre (plus fort dans le bois)
    float vignette = lieu == 3 ? 0.75f : 0.55f;
    DrawRectangleGradientV(0, 0, LARGEUR_FENETRE, 70, Fade(BLACK, vignette), BLANK);
    DrawRectangleGradientV(0, HAUTEUR_ARENE - 70, LARGEUR_FENETRE, 70, BLANK, Fade(BLACK, vignette));
    DrawRectangleGradientH(0, 0, 70, HAUTEUR_ARENE, Fade(BLACK, vignette - 0.05f), BLANK);
    DrawRectangleGradientH(LARGEUR_FENETRE - 70, 0, 70, HAUTEUR_ARENE, BLANK, Fade(BLACK, vignette - 0.05f));
}

void dessinerArene(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    bool tourJoueur = jeu.phase == Phase::Deplacement || jeu.phase == Phase::Action;
    std::vector<int> pas = casesAtteignables(jeu);
    int portee = porteeAction(jeu, jeu.actionChoisie);
    bool actionVisee = tourJoueur && !actionSurSoi(jeu.actionChoisie) && actionDisponible(jeu, jeu.actionChoisie);

    // ----- 1. Le decor : le terrain du lieu, dessine a partir de sa carte (decor.cpp) -----
    dessinerTerrain(jeu);
    dessinerSourcesDeLumiere(jeu, false);
    dessinerCercleDeRunes(jeu);

    // ----- 2. Les personnages -----
    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout() || h.disparition > 0) {
            dessinerPion(h, false, jeu.aylis.colonne);
        }
    }
    bool allieVisible = jeu.compagnon != COMPAGNON_AUCUN && estDansArene(jeu.allie.colonne, jeu.allie.ligne)
                        && (jeu.allie.stats.estDebout() || jeu.allie.disparition > 0);
    if (allieVisible) {
        dessinerPion(jeu.allie, false, jeu.aylis.colonne);
    }
    bool aylisVisible = jeu.aylis.stats.estDebout() || animationsEnCours(jeu);
    if (aylisVisible) {
        dessinerPion(jeu.aylis, true, jeu.aylis.colonne);
    }

    // ----- 3. La penombre et les lumieres -----
    appliquerLumiere();
    dessinerSourcesDeLumiere(jeu, true);

    // ----- Les attaques annoncees par les boss : des cases rouges qui pulsent (a eviter !) -----
    {
        float pulsation = 0.5f + 0.5f * std::sin(GetTime() * 8);
        for (const auto& zone : jeu.zonesDanger) {
            float x = zone.first * TAILLE_CASE;
            float y = zone.second * TAILLE_CASE;
            DrawRectangle(x + 2, y + 2, TAILLE_CASE - 4, TAILLE_CASE - 4, Fade(Color{255, 40, 30, 255}, 0.25f + 0.2f * pulsation));
            DrawRectangleLinesEx({x + 2, y + 2, TAILLE_CASE - 4.0f, TAILLE_CASE - 4.0f}, 3, Fade(Color{255, 90, 60, 255}, 0.9f));
            DrawTriangleLines({x + 36, y + 20}, {x + 22, y + 48}, {x + 50, y + 48}, Fade(WHITE, 0.8f));
            DrawRectangle(x + 35, y + 29, 2, 10, Fade(WHITE, 0.9f));
            DrawRectangle(x + 35, y + 42, 2, 3, Fade(WHITE, 0.9f));
        }
    }

    // ----- 4. Les aides de jeu, par-dessus (elles doivent rester bien lisibles) -----
    for (int l = 0; l < LIGNES; l++) {
        for (int c = 0; c < COLONNES; c++) {
            int x = c * TAILLE_CASE;
            int y = l * TAILLE_CASE;
            // La portee de l'action choisie : une teinte rouge tres legere
            if (actionVisee && distanceCases(jeu.aylis.colonne, jeu.aylis.ligne, c, l) <= portee) {
                DrawRectangle(x, y, TAILLE_CASE, TAILLE_CASE, Fade(RED, 0.06f));
            }
            // Les cases ou AYLIS peut aller
            if (jeu.phase == Phase::Deplacement && pas[l * COLONNES + c] > 0) {
                DrawRectangle(x + 3, y + 3, TAILLE_CASE - 6, TAILLE_CASE - 6, Fade(SKYBLUE, 0.16f));
                DrawRectangleLinesEx({(float)x + 3, (float)y + 3, TAILLE_CASE - 6.0f, TAILLE_CASE - 6.0f}, 1,
                                     Fade(SKYBLUE, 0.5f));
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

    // ----- 5. Les effets qui brillent, l'ambiance, et les infos des pions -----
    dessinerProjectiles(jeu);
    dessinerParticules(jeu);
    dessinerEclairs(jeu);
    dessinerAmbiance(jeu);
    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout()) {
            dessinerBarreDeVie(h);
            dessinerEtats(h);
        }
    }
    if (aylisVisible) {
        dessinerBarreDeVie(jeu.aylis);
        dessinerEtats(jeu.aylis);
    }
    if (allieDebout(jeu)) {
        dessinerBarreDeVie(jeu.allie);
    }

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
    int numeroActe = acteDeLaSalle(jeu.salle);
    std::string repere = TextFormat("ACTE %i - %i/%i", numeroActe + 1, jeu.salle - premiereSalleDeLActe(numeroActe) + 1,
                                    nombreDeSallesDeLActe(numeroActe));
    DrawText(repere.c_str(), 12, 8, 20, OR);
    DrawText(jeu.nomDuLieu.c_str(), 12 + MeasureText(repere.c_str(), 20) + 18, 8, 20, RAYWHITE);

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
    if (jeu.compagnon != COMPAGNON_AUCUN) {
        const Combattant& c = jeu.allie.stats;
        std::string compagnon = c.estDebout() ? TextFormat("%s  %i/%i pv", c.nom.c_str(), c.pv, c.pvMax)
                                              : TextFormat("%s  K.O. (revient au prochain combat)", c.nom.c_str());
        DrawText(compagnon.c_str(), x, y0 + 164, 10, Color{120, 180, 255, 255});
    }

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

// La frise de l'acte en cours : une pierre par salle, reliees par un chemin. Les salles passees brillent.
void dessinerFrise(const Jeu& jeu) {
    float y = 200;
    float gauche = 150;
    int numeroActe = acteDeLaSalle(jeu.salle);
    int premiere = premiereSalleDeLActe(numeroActe);
    int nombre = nombreDeSallesDeLActe(numeroActe);
    int derniere = premiere + nombre - 1;
    float pas = (LARGEUR_FENETRE - 2 * gauche) / (nombre - 1);
    DrawLineEx({gauche, y}, {gauche + pas * (nombre - 1), y}, 3, Fade(BORD, 0.7f));
    for (int salle = premiere; salle <= derniere; salle++) {
        float x = gauche + pas * (salle - premiere);
        Color couleur = BORD;
        if (salle < jeu.salle) {
            couleur = VIOLET_VISION;
        } else if (salle == jeu.salle) {
            couleur = OR;
        }
        float rayon = salle == derniere ? 13.0f : 9.0f;
        if (salle == jeu.salle) {
            DrawCircle(x, y, rayon + 6 + 2 * std::sin(GetTime() * 4), Fade(OR, 0.25f));
        }
        DrawCircle(x, y, rayon, salle <= jeu.salle ? couleur : CADRE);
        DrawCircleLines(x, y, rayon, couleur);
    }
    // Le nom du lieu sous chaque salle (et le boss tout au bout)
    for (int salle = premiere; salle <= derniere; salle++) {
        float x = gauche + pas * (salle - premiere);
        std::string nom = salle == derniere ? "Boss" : nomCourtLieu(lieuDeLaSalle(salle));
        texteCentre(nom, {x - 50, y + 16, 100, 20}, 10, salle == jeu.salle ? OR : TEXTE_GRIS);
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
// Le portrait d'un boss : son sprite, son arme et sa coiffe
void dessinerPortraitBoss(int boss, Rectangle ecran) {
    const char* noms[5] = {"", "Skarn", "Matriarche", "Ashka", "Vorgath"};
    Rectangle source = {0, 0, TAILLE_SPRITE, TAILLE_SPRITE};
    DrawTexturePro(spriteHaschen(noms[boss]), source, ecran, {0, 0}, 0, WHITE);
    const Texture2D* arme = spriteArmeHaschen(noms[boss]);
    if (arme != nullptr) {
        DrawTexturePro(*arme, source, ecran, {0, 0}, 0, WHITE);
    }
    const Texture2D* coiffe = spriteCoiffe(noms[boss]);
    if (coiffe != nullptr) {
        DrawTexturePro(*coiffe, source, ecran, {0, 0}, 0, WHITE);
    }
}

void dessinerImageSalle(TypeSalle type, int lieu, int boss, Rectangle zone) {
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
        DrawTexturePro(s.marchands[marchandDuLieu(lieu)], source, ecran, {0, 0}, 0, WHITE);
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
        dessinerPortraitBoss(boss, ecran);
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
std::vector<std::string> decouperEnLignes(const std::string& texte, float largeurMax, int taille) {
    std::vector<std::string> lignes;
    std::string ligne;
    std::string mot;
    for (int i = 0; i <= (int)texte.size(); i++) {
        if (i == (int)texte.size() || texte[i] == ' ') {
            std::string essai = ligne.empty() ? mot : ligne + " " + mot;
            if (MeasureText(essai.c_str(), taille) > largeurMax && !ligne.empty()) {
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
    return lignes;
}

void texteSurPlusieursLignes(const std::string& texte, Rectangle zone, int taille, Color couleur) {
    std::vector<std::string> lignes = decouperEnLignes(texte, zone.width, taille);
    for (int i = 0; i < (int)lignes.size(); i++) {
        texteCentre(lignes[i], {zone.x, zone.y + i * (taille + 6.0f), zone.width, (float)taille}, taille, couleur);
    }
}

void dessinerChoixSalle(const Jeu& jeu) {
    dessinerFondVision();
    const char* titre = estSalleDeBoss(jeu.salle) ? "LA VISION S'ASSOMBRIT" : "UNE VISION";
    texteCentre(titre, {0, 40, (float)LARGEUR_FENETRE, 50}, 50, VIOLET_VISION);
    texteCentre(TextFormat("Acte %i : %s  -  %s", acteDeLaSalle(jeu.salle) + 1, acte(acteDeLaSalle(jeu.salle)).nom.c_str(),
                           nomLieu(jeu.lieu).c_str()),
                {0, 100, (float)LARGEUR_FENETRE, 24}, 20, LIGHTGRAY);
    dessinerFrise(jeu);

    int nombre = jeu.propositions.size();
    for (int i = 0; i < nombre; i++) {
        TypeSalle type = jeu.propositions[i].type;
        Color couleur = couleurSalle(type);
        Rectangle carte = dessinerCarteChoix(i, nombre, couleur);
        dessinerImageSalle(type, jeu.lieu, acte(acteDeLaSalle(jeu.salle)).boss, {carte.x, carte.y + 30, carte.width, 120});
        std::string titreCarte = type == TypeSalle::Boss ? nomDuBoss(acte(acteDeLaSalle(jeu.salle)).boss) : nomTypeSalle(type);
        texteCentre(titreCarte, {carte.x, carte.y + 165, carte.width, 30}, titreCarte.size() > 14 ? 20 : 30, couleur);
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

// ===================== Les dialogues : un parchemin magique =====================
//
// Le personnage qui parle en grand a gauche, AYLIS a droite, et le texte qui s'ecrit a la plume sur un parchemin :
// bords brules, double cadre a l'encre doree, une grande lettrine, des runes et des etincelles de la couleur
// de celui qui parle, et son nom sur un ruban scelle a la cire.

const Color PARCHEMIN_CLAIR = {234, 216, 174, 255};
const Color PARCHEMIN_FONCE = {200, 172, 118, 255};
const Color ENCRE = {52, 32, 16, 255};
const Color ENCRE_DOREE = {176, 124, 40, 255};

// Une encre sombre de la couleur du personnage : lisible sur le parchemin
Color encreDe(Color c) {
    return Color{(unsigned char)(c.r * 0.42f), (unsigned char)(c.g * 0.42f), (unsigned char)(c.b * 0.42f), 255};
}

Rectangle rectangleChoixDialogue(int numero) {
    return {300, 290.0f + numero * 84, 534, 70};
}

// Le portrait d'un personnage (sprite 16x16 agrandi)
void dessinerPortraitOrateur(const Jeu& jeu, int orateur, Rectangle ecran, Color teinte) {
    const Sprites& s = sprites();
    Rectangle source = {0, 0, TAILLE_SPRITE, TAILLE_SPRITE};
    if (orateur >= BOSS_SKARN && orateur <= BOSS_VORGATH) {
        dessinerPortraitBoss(orateur, ecran);
    } else if (orateur == ORATEUR_AYLIS) {
        dessinerPortraitAylis(jeu.aylis.stats.arme, ecran, teinte);
    } else if (orateur >= ORATEUR_MAREN && orateur <= ORATEUR_SILAS) {
        DrawTexturePro(s.marchands[orateur - ORATEUR_MAREN], source, ecran, {0, 0}, 0, teinte);
    } else if (orateur == ORATEUR_DESERTEUR || orateur == ORATEUR_KERRAK) {
        DrawTexturePro(s.eclaireur, source, ecran, {0, 0}, 0, teinte);
        if (orateur == ORATEUR_KERRAK) {
            DrawTexturePro(s.epee, source, ecran, {0, 0}, 0, teinte);
        }
    } else if (orateur == ORATEUR_BRENNA) {
        DrawTexturePro(s.aylis, source, ecran, {0, 0}, 0, Color{teinte.r, (unsigned char)(teinte.g * 0.72f), (unsigned char)(teinte.b * 0.5f), 255});
        DrawTexturePro(s.epee, source, ecran, {0, 0}, 0, teinte);
    }
}

// Un parchemin : fond degrade, taches, bords brules, double cadre dore et arabesques dans les coins
void dessinerParchemin(Rectangle r, Color lueur, bool runes) {
    float temps = GetTime();
    // Le halo magique autour de la page, et son ombre
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRounded({r.x - 10, r.y - 10, r.width + 20, r.height + 20}, 0.08f, 8, Fade(lueur, 0.10f + 0.03f * std::sin(temps * 2)));
    EndBlendMode();
    DrawRectangleRounded({r.x + 5, r.y + 8, r.width, r.height}, 0.06f, 8, Fade(BLACK, 0.55f));
    // Le papier
    DrawRectangleRounded(r, 0.06f, 8, PARCHEMIN_FONCE);
    DrawRectangleGradientV(r.x + 6, r.y + 6, r.width - 12, r.height - 12, PARCHEMIN_CLAIR, PARCHEMIN_FONCE);
    // Des taches et le grain du papier (toujours aux memes endroits)
    for (int i = 0; i < 70; i++) {
        float x = r.x + 10 + hasardFixe(i) * (r.width - 20);
        float y = r.y + 10 + hasardFixe(i + 300) * (r.height - 20);
        DrawCircle(x, y, 1 + hasardFixe(i + 600) * (i % 9 == 0 ? 14 : 2), Fade(Color{120, 80, 30, 255}, i % 9 == 0 ? 0.07f : 0.15f));
    }
    // Les bords brules : plusieurs traits bruns de plus en plus pales vers l'interieur
    for (int i = 0; i < 6; i++) {
        Rectangle bord = {r.x + i * 2.0f, r.y + i * 2.0f, r.width - i * 4.0f, r.height - i * 4.0f};
        DrawRectangleRoundedLinesEx(bord, 0.06f, 8, 3, Fade(Color{90, 48, 16, 255}, 0.55f - i * 0.09f));
    }
    // Le double cadre a l'encre doree
    DrawRectangleLinesEx({r.x + 14, r.y + 14, r.width - 28, r.height - 28}, 2, Fade(ENCRE_DOREE, 0.85f));
    DrawRectangleLinesEx({r.x + 20, r.y + 20, r.width - 40, r.height - 40}, 1, Fade(ENCRE_DOREE, 0.6f));
    // Les arabesques des coins : des boucles dorees
    const float coins[4][2] = {{r.x + 14, r.y + 14}, {r.x + r.width - 14, r.y + 14}, {r.x + 14, r.y + r.height - 14},
                               {r.x + r.width - 14, r.y + r.height - 14}};
    for (int i = 0; i < 4; i++) {
        float sx = i % 2 == 0 ? 1 : -1;
        float sy = i < 2 ? 1 : -1;
        float cx = coins[i][0] + sx * 16;
        float cy = coins[i][1] + sy * 16;
        DrawRing({cx, cy}, 8, 10, 0, 360, 24, ENCRE_DOREE);
        DrawCircle(cx, cy, 3, Color{232, 184, 74, 255});
        DrawLineEx({cx + sx * 10, cy}, {cx + sx * 40, cy}, 2, ENCRE_DOREE);
        DrawLineEx({cx, cy + sy * 10}, {cx, cy + sy * 30}, 2, ENCRE_DOREE);
        DrawCircle(cx + sx * 44, cy, 2.5f, ENCRE_DOREE);
    }
    if (!runes) {
        return;
    }
    // Des runes a l'encre magique, le long du bas de la page : elles brillent de la couleur du personnage
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 9; i++) {
        if (i == 4) {
            continue;
        }
        float x = r.x + 110 + i * (r.width - 220) / 8;
        float y = r.y + r.height - 16;
        float lueurRune = 0.5f + 0.3f * std::sin(temps * 2.5f + i);
        DrawCircleGradient(x, y, 12, Fade(lueur, 0.35f * lueurRune), BLANK);
    }
    EndBlendMode();
    for (int i = 0; i < 9; i++) {
        if (i == 4) {
            continue;
        }
        float x = r.x + 110 + i * (r.width - 220) / 8;
        dessinerGlyphe(x, r.y + r.height - 16, 7, i * 3, encreDe(lueur));
    }
    // Des etincelles qui s'echappent des bords de la page
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 18; i++) {
        float x = r.x + hasardFixe(i + 900) * r.width;
        float cycle = std::fmod(temps * (0.3f + hasardFixe(i + 950) * 0.4f) + hasardFixe(i + 990), 1.0f);
        float y = r.y + 4 - cycle * 40;
        float taille = 2 + 3 * (1 - cycle);
        Color c = i % 3 == 0 ? Color{255, 230, 160, 255} : lueur;
        DrawPoly({x, y}, 4, taille, 45, Fade(c, 0.9f * (1 - cycle)));
    }
    EndBlendMode();
}

// Un sceau de cire (sur le ruban du nom et devant les reponses)
void dessinerSceau(float x, float y, float rayon, Color couleur, const std::string& signe) {
    DrawCircle(x + 2, y + 3, rayon, Fade(BLACK, 0.5f));
    DrawPoly({x, y}, 10, rayon, 9, encreDe(couleur));
    DrawCircle(x, y, rayon - 3, Color{(unsigned char)(couleur.r * 0.6f), (unsigned char)(couleur.g * 0.6f), (unsigned char)(couleur.b * 0.6f), 255});
    DrawRing({x, y}, rayon - 8, rayon - 6, 0, 360, 24, Fade(BLACK, 0.3f));
    DrawCircle(x - rayon * 0.3f, y - rayon * 0.35f, rayon * 0.25f, Fade(WHITE, 0.25f));
    texteCentre(signe, {x - rayon, y - rayon, rayon * 2, rayon * 2}, (int)rayon, Color{250, 232, 200, 255});
}

void dessinerDialogue(const Jeu& jeu) {
    const Dialogue& d = jeu.dialogue;
    const Replique& replique = d.repliques[d.ligne];
    int orateur = replique.orateur;
    Color lueur = couleurOrateur(orateur);
    float temps = GetTime();

    dessinerFondVision();

    // Le personnage en face d'AYLIS : le premier qui parle dans ce dialogue (ni AYLIS, ni la prophetie)
    int enFace = -1;
    for (const Replique& r : d.repliques) {
        if (r.orateur != ORATEUR_AYLIS && r.orateur != ORATEUR_PROPHETIE) {
            enFace = r.orateur;
            break;
        }
    }
    if (orateur != ORATEUR_AYLIS && orateur != ORATEUR_PROPHETIE) {
        enFace = orateur;
    }

    // Le titre (l'epilogue)
    if (!d.titre.empty()) {
        texteCentre(d.titre, {0, 26, (float)LARGEUR_FENETRE, 44}, 40, OR);
    }

    // A gauche : celui qui parle a AYLIS, dans sa lumiere
    if (enFace >= 0) {
        Color sa = couleurOrateur(enFace);
        bool parle = orateur == enFace;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(190, 300, 230, Fade(sa, parle ? 0.30f : 0.12f), BLANK);
        EndBlendMode();
        float bond = parle ? 4 * std::sin(temps * 3) : 0;
        dessinerPortraitOrateur(jeu, enFace, {40, 96 + bond, 300, 300}, parle ? WHITE : Color{140, 140, 150, 255});
    }
    // A droite : AYLIS, qui ecoute (ou qui parle)
    {
        bool parle = orateur == ORATEUR_AYLIS;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(712, 330, 150, Fade(Color{120, 160, 255, 255}, parle ? 0.30f : 0.10f), BLANK);
        EndBlendMode();
        float bond = parle ? 4 * std::sin(temps * 3) : 0;
        dessinerPortraitOrateur(jeu, ORATEUR_AYLIS, {602, 200 + bond, 220, 220}, parle ? WHITE : Color{150, 150, 170, 255});
    }
    // La prophetie parle : un grand glyphe lumineux au centre
    if (orateur == ORATEUR_PROPHETIE) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(LARGEUR_FENETRE / 2, 250, 140, Fade(lueur, 0.35f + 0.1f * std::sin(temps * 2)), BLANK);
        EndBlendMode();
        dessinerGlyphe(LARGEUR_FENETRE / 2.0f, 250, 70, 7, lueur);
    }

    // Le parchemin et le texte
    Rectangle page = {32, 474, LARGEUR_FENETRE - 64.0f, 258};
    dessinerParchemin(page, lueur, true);

    // Le ruban du nom, scelle a la cire
    std::string nom = nomOrateur(orateur);
    std::string titre = titreOrateur(orateur);
    float largeurRuban = MeasureText(nom.c_str(), 30) + MeasureText(titre.c_str(), 20) + 70;
    Rectangle ruban = {page.x + 50, page.y - 26, largeurRuban, 44};
    DrawRectangle(ruban.x + 3, ruban.y + 4, ruban.width, ruban.height, Fade(BLACK, 0.5f));
    DrawRectangleGradientV(ruban.x, ruban.y, ruban.width, ruban.height, encreDe(lueur), Color{20, 10, 12, 255});
    DrawTriangle({ruban.x + ruban.width, ruban.y}, {ruban.x + ruban.width - 16, ruban.y + ruban.height / 2},
                 {ruban.x + ruban.width + 16, ruban.y}, encreDe(lueur));
    DrawTriangle({ruban.x + ruban.width - 16, ruban.y + ruban.height / 2}, {ruban.x + ruban.width, ruban.y + ruban.height},
                 {ruban.x + ruban.width + 16, ruban.y + ruban.height}, Color{20, 10, 12, 255});
    DrawText(nom.c_str(), ruban.x + 34, ruban.y + 8, 30, Color{255, 240, 216, 255});
    DrawText(titre.c_str(), ruban.x + 44 + MeasureText(nom.c_str(), 30), ruban.y + 16, 20, Color{240, 216, 192, 255});
    dessinerSceau(ruban.x + 4, ruban.y + ruban.height / 2, 28, lueur, nom.substr(0, 1));

    // Le texte, ecrit a la plume : une grande lettrine, puis le reste (les 2 premieres lignes laissent sa place)
    const std::string& texte = replique.texte;
    int visibles = (int)d.ecriture;
    std::string lettrine = texte.substr(0, 1);
    std::string suite = texte.substr(1);
    const int taille = 20;
    float gauche = page.x + 44;
    float largeurTexte = page.width - 88;
    float retrait = MeasureText(lettrine.c_str(), 64) + 14.0f;     // la place de la lettrine
    // Le decoupage en lignes : les 2 premieres sont plus courtes, a cause de la lettrine
    std::vector<std::string> lignes;
    {
        std::string ligne;
        std::string mot;
        for (int i = 0; i <= (int)suite.size(); i++) {
            if (i == (int)suite.size() || suite[i] == ' ') {
                float maximum = lignes.size() < 2 ? largeurTexte - retrait : largeurTexte;
                std::string essai = ligne.empty() ? mot : ligne + " " + mot;
                if (MeasureText(essai.c_str(), taille) > maximum && !ligne.empty()) {
                    lignes.push_back(ligne);
                    ligne = mot;
                } else {
                    ligne = essai;
                }
                mot = "";
            } else {
                mot = mot + suite[i];
            }
        }
        lignes.push_back(ligne);
    }
    if (visibles > 0) {
        // La lettrine brille de la couleur du personnage
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(gauche + 24, page.y + 72, 40, Fade(lueur, 0.35f), BLANK);
        EndBlendMode();
        DrawText(lettrine.c_str(), gauche + 4, page.y + 34, 64, encreDe(lueur));
        DrawText(lettrine.c_str(), gauche + 2, page.y + 32, 64, Fade(lueur, 0.35f));
    }
    int restantes = visibles - 1;
    for (int i = 0; i < (int)lignes.size() && restantes > 0; i++) {
        std::string morceau = lignes[i].substr(0, restantes);
        float x = gauche + (i < 2 ? retrait : 0);
        DrawText(morceau.c_str(), x, page.y + 46 + i * 30, taille, ENCRE);
        restantes = restantes - (int)lignes[i].size() - 1;
    }

    // En bas a droite : "la suite", quand tout est ecrit
    bool ecrit = d.ecriture >= (float)texte.size();
    if (ecrit && !attendUnChoix(jeu)) {
        float saut = 3 * std::sin(temps * 5);
        float x = page.x + page.width - 60;
        float y = page.y + page.height - 46 + saut;
        DrawTriangle({x, y}, {x + 20, y}, {x + 10, y + 12}, encreDe(lueur));
        DrawText("ENTREE", x - 70, y - 2, 10, Color{110, 80, 50, 255});
    }

    // Les reponses : deux bandes de parchemin, chacune avec son sceau numerote
    if (attendUnChoix(jeu)) {
        const char* chiffres[2] = {"I", "II"};
        for (int i = 0; i < (int)d.choix.size() && i < 2; i++) {
            Rectangle bande = rectangleChoixDialogue(i);
            bool survol = CheckCollisionPointRec(GetMousePosition(), bande);
            if (survol) {
                bande.x = bande.x - 6;
            }
            Color couleurChoix = i == 0 ? Color{110, 224, 208, 255} : Color{255, 110, 80, 255};
            dessinerParchemin(bande, couleurChoix, false);
            dessinerSceau(bande.x + 34, bande.y + bande.height / 2, 22, couleurChoix, chiffres[i]);
            DrawText(d.choix[i].c_str(), bande.x + 66, bande.y + 14, 20, ENCRE);
            if (i < (int)d.effets.size()) {
                DrawText(d.effets[i].c_str(), bande.x + 66, bande.y + 42, 10, encreDe(couleurChoix));
            }
        }
        Rectangle premiere = rectangleChoixDialogue(0);
        texteCentre("Que repond AYLIS ? Tape 1 ou 2, ou clique", {premiere.x, premiere.y - 32, premiere.width, 20}, 20,
                    Fade(RAYWHITE, 0.85f));
    }
}

// ===================== Le Seuil : le monde entre les visions =====================
//
// Une ile qui flotte dans un ciel d'etoiles. Tout y est dessine avec des formes simples (cercles, triangles,
// degrades) et beaucoup de lumiere "additive" : les couleurs s'ajoutent, ce qui donne cet aspect lumineux.

const Color SEUIL_VIOLET = {190, 140, 255, 255};
const Color SEUIL_CYAN = {110, 230, 240, 255};
const Color SEUIL_ROSE = {255, 150, 210, 255};

// Les positions (les pieds) des echos, d'AYLIS et du portail
Vector2 positionAuSeuil(int personnage) {
    const Vector2 positions[5] = {{200, 500}, {318, 556}, {566, 552}, {432, 505}, {712, 470}};
    return positions[personnage];
}

// Le petit balancement de l'ile, qui flotte
float balancementIle() {
    return 5 * std::sin(GetTime() * 0.8f);
}

// Un eclat de prophetie : un petit losange lumineux (l'icone des fragments)
void dessinerFragment(float x, float y, float taille) {
    DrawPoly({x, y}, 4, taille, 0, SEUIL_VIOLET);
    DrawPoly({x, y - taille * 0.25f}, 4, taille * 0.45f, 0, Fade(WHITE, 0.9f));
}

// Une petite ile lointaine, avec son arbre lumineux
void dessinerIlot(float x, float y, float taille, int numero) {
    float b = 4 * std::sin(GetTime() * 0.6f + numero * 1.7f);
    y = y + b;
    DrawTriangle({x - taille, y}, {x, y + taille * 1.3f}, {x + taille, y}, Color{34, 26, 54, 255});
    DrawEllipse(x, y, taille, taille * 0.28f, Color{44, 58, 78, 255});
    DrawLine(x + taille * 0.2f, y - 2, x + taille * 0.2f, y - taille * 0.6f, Color{70, 50, 90, 255});
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(x + taille * 0.2f, y - taille * 0.7f, taille * 0.5f, Fade(numero % 2 ? SEUIL_CYAN : SEUIL_ROSE, 0.5f), BLANK);
    EndBlendMode();
}

void dessinerCielDuSeuil() {
    float temps = GetTime();
    DrawRectangleGradientV(0, 0, LARGEUR_FENETRE, HAUTEUR_FENETRE, Color{8, 6, 26, 255}, Color{30, 14, 52, 255});

    BeginBlendMode(BLEND_ADDITIVE);
    // Les nebuleuses : de grandes taches de couleur qui derivent tres lentement
    DrawCircleGradient(180 + 20 * std::sin(temps * 0.1f), 150, 260, Fade(Color{120, 60, 200, 255}, 0.28f), BLANK);
    DrawCircleGradient(700, 230 + 20 * std::sin(temps * 0.13f), 230, Fade(Color{40, 150, 190, 255}, 0.22f), BLANK);
    DrawCircleGradient(120, 640, 220, Fade(Color{200, 70, 150, 255}, 0.16f), BLANK);
    DrawCircleGradient(780, 690, 200, Fade(Color{90, 70, 210, 255}, 0.2f), BLANK);

    // Les etoiles, qui scintillent chacune a son rythme
    for (int i = 0; i < 150; i++) {
        float x = hasardFixe(i + 1000) * LARGEUR_FENETRE;
        float y = hasardFixe(i + 2000) * HAUTEUR_FENETRE;
        float eclat = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(temps * (1 + hasardFixe(i) * 2) + i));
        float taille = i % 11 == 0 ? 2.2f : 1.1f;
        DrawCircle(x, y, taille, Fade(WHITE, eclat));
        if (i % 11 == 0) {  // les plus grosses ont une petite croix de lumiere
            DrawLine(x - 6 * eclat, y, x + 6 * eclat, y, Fade(SEUIL_CYAN, eclat * 0.6f));
            DrawLine(x, y - 6 * eclat, x, y + 6 * eclat, Fade(SEUIL_CYAN, eclat * 0.6f));
        }
    }

    // Une aurore : un ruban qui ondule en travers du ciel
    for (int x = 0; x < LARGEUR_FENETRE; x = x + 4) {
        float y = 170 + 45 * std::sin(x * 0.009f + temps * 0.3f) + 15 * std::sin(x * 0.023f - temps * 0.5f);
        float force = 0.07f + 0.05f * std::sin(x * 0.015f + temps);
        Color couleur = x < LARGEUR_FENETRE / 2 ? SEUIL_CYAN : SEUIL_VIOLET;
        DrawRectangleGradientV(x, y - 90, 4, 90, BLANK, Fade(couleur, force));
    }
    EndBlendMode();

    // Des ilots au loin
    dessinerIlot(80, 290, 34, 0);
    dessinerIlot(800, 200, 28, 1);
    dessinerIlot(760, 650, 40, 2);
    dessinerIlot(60, 700, 26, 3);
}

// L'ile du Seuil : un dessous de roche qui pend dans le vide, et un dessus couvert de mousse et de fleurs
void dessinerIleDuSeuil() {
    float temps = GetTime();
    float b = balancementIle();
    float cx = 432;
    float cy = 490 + b;

    // Des cascades de lumiere qui tombent du bord de l'ile dans le vide
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 5; i++) {
        float x = cx - 250 + i * 125 + 20 * std::sin(i * 3.1f);
        float largeur = 10 + i % 3 * 6;
        DrawRectangleGradientV(x, cy + 60, largeur, 220, Fade(SEUIL_CYAN, 0.22f), BLANK);
        for (int j = 0; j < 4; j++) {   // des gouttes de lumiere qui descendent
            float y = cy + 60 + std::fmod(temps * 60 + j * 55 + i * 20, 220.0f);
            DrawCircle(x + largeur / 2, y, 2, Fade(WHITE, 0.7f * (1 - (y - cy - 60) / 220)));
        }
    }
    EndBlendMode();

    // Le dessous de roche : des triangles irreguliers, de plus en plus pointus
    Color roche = {38, 30, 58, 255};
    Color rocheSombre = {26, 20, 42, 255};
    const float pointes[9][2] = {{-360, 40}, {-280, 150}, {-200, 110}, {-120, 230}, {-40, 290},
                                 {40, 250}, {130, 200}, {220, 140}, {320, 70}};
    for (int i = 0; i < 9; i++) {
        float x = cx + pointes[i][0];
        float gauche = cx + (i == 0 ? -370 : pointes[i - 1][0]);
        float droite = cx + (i == 8 ? 370 : pointes[i + 1][0]);
        DrawTriangle({gauche, cy}, {x, cy + pointes[i][1]}, {droite, cy}, i % 2 ? roche : rocheSombre);
    }
    // Des racines qui pendent, avec une lueur au bout
    for (int i = 0; i < 7; i++) {
        float x = cx - 240 + i * 80;
        float longueur = 60 + 40 * hasardFixe(i + 70);
        float balance = 6 * std::sin(temps * 0.9f + i);
        DrawLineEx({x, cy + 20}, {x + balance, cy + 20 + longueur}, 2, Color{70, 55, 80, 255});
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(x + balance, cy + 22 + longueur, 9, Fade(i % 2 ? SEUIL_CYAN : SEUIL_ROSE, 0.8f), BLANK);
        EndBlendMode();
    }
    // Des cristaux pris dans la roche
    for (int i = 0; i < 5; i++) {
        float x = cx - 180 + i * 95;
        float y = cy + 70 + 60 * hasardFixe(i + 40);
        DrawPoly({x, y}, 4, 7, 0, Fade(SEUIL_VIOLET, 0.9f));
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(x, y, 22, Fade(SEUIL_VIOLET, 0.35f + 0.15f * std::sin(temps * 2 + i)), BLANK);
        EndBlendMode();
    }

    // Le dessus : une grande ellipse de mousse, avec un bord eclaire
    DrawEllipse(cx, cy + 8, 372, 118, Color{30, 40, 56, 255});
    DrawEllipse(cx, cy, 370, 112, Color{44, 72, 80, 255});
    DrawEllipse(cx, cy - 6, 330, 92, Color{52, 86, 90, 255});
    DrawEllipseLines(cx, cy, 370, 112, Fade(SEUIL_CYAN, 0.35f));

    // Un chemin de dalles lumineuses, d'AYLIS jusqu'au portail
    Vector2 depart = positionAuSeuil(3);
    Vector2 portail = positionAuSeuil(4);
    for (int i = 1; i < 7; i++) {
        float t = i / 7.0f;
        float x = depart.x + (portail.x - depart.x) * t;
        float y = depart.y + b + (portail.y - depart.y) * t + 10 * std::sin(t * PI);
        DrawEllipse(x, y, 16, 6, Color{80, 90, 120, 255});
        BeginBlendMode(BLEND_ADDITIVE);
        DrawEllipse(x, y, 10, 3, Fade(SEUIL_VIOLET, 0.25f + 0.2f * std::sin(temps * 3 - i)));
        EndBlendMode();
    }

    // Des herbes et des fleurs qui brillent
    for (int i = 0; i < 40; i++) {
        float angle = hasardFixe(i + 500) * 2 * PI;
        float distance = std::sqrt(hasardFixe(i + 600)) * 0.9f;
        float x = cx + std::cos(angle) * 340 * distance;
        float y = cy + std::sin(angle) * 95 * distance;
        if (i % 3 == 0) {
            Color couleur = i % 2 ? SEUIL_CYAN : SEUIL_ROSE;
            DrawLine(x, y, x, y - 8, Color{60, 110, 90, 255});
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleGradient(x, y - 9, 8, Fade(couleur, 0.6f + 0.3f * std::sin(temps * 2 + i)), BLANK);
            EndBlendMode();
            DrawCircle(x, y - 9, 1.6f, WHITE);
        } else {
            DrawLine(x, y, x - 2, y - 7, Color{80, 130, 110, 255});
            DrawLine(x + 2, y, x + 3, y - 6, Color{70, 120, 100, 255});
        }
    }
}

// L'arbre du Seuil : il grandit avec les visions, et ses petales lumineux tombent doucement
void dessinerArbreDuSeuil(const Memoire& m) {
    float temps = GetTime();
    float b = balancementIle();
    int souvenirs = m.visions + m.victoires;
    float echelle = 0.72f + (souvenirs > 12 ? 12 : souvenirs) * 0.025f;
    float x = 432;
    float pied = 440 + b;
    float haut = pied - 150 * echelle;

    // Le tronc et les branches
    Color bois = {74, 52, 92, 255};
    DrawTriangle({x - 16 * echelle, pied}, {x + 16 * echelle, pied}, {x, haut}, bois);
    DrawLineEx({x, pied - 60 * echelle}, {x - 70 * echelle, haut + 10}, 6 * echelle, bois);
    DrawLineEx({x, pied - 80 * echelle}, {x + 75 * echelle, haut + 5}, 6 * echelle, bois);
    DrawLineEx({x, haut + 20}, {x + 10, haut - 40 * echelle}, 5 * echelle, bois);

    // Le feuillage : des grappes de lumiere (d'abord un fond sombre, puis la lumiere par-dessus)
    const float grappes[9][3] = {{0, -40, 70}, {-80, -10, 55}, {80, -15, 58}, {-45, -80, 52}, {50, -85, 55},
                                 {0, -115, 45}, {-120, 20, 38}, {120, 15, 40}, {0, 5, 50}};
    for (const auto& g : grappes) {
        DrawCircle(x + g[0] * echelle, haut + g[1] * echelle, g[2] * echelle, Color{60, 40, 96, 255});
    }
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 9; i++) {
        const auto& g = grappes[i];
        Color couleur = i % 3 == 0 ? SEUIL_CYAN : (i % 3 == 1 ? SEUIL_VIOLET : SEUIL_ROSE);
        float pulsation = 0.35f + 0.12f * std::sin(temps * 1.3f + i);
        DrawCircleGradient(x + g[0] * echelle, haut + g[1] * echelle, g[2] * echelle * 1.25f, Fade(couleur, pulsation), BLANK);
    }
    // Les fleurs de l'arbre : de petits points blancs qui scintillent
    for (int i = 0; i < 45; i++) {
        float fx = x + (hasardFixe(i + 800) - 0.5f) * 260 * echelle;
        float fy = haut - 10 + (hasardFixe(i + 900) - 0.7f) * 170 * echelle;
        DrawCircle(fx, fy, 1.8f, Fade(WHITE, 0.5f + 0.5f * std::sin(temps * 3 + i)));
    }
    // Les petales qui tombent en tournoyant
    for (int i = 0; i < 26; i++) {
        float chute = std::fmod(temps * (18 + hasardFixe(i + 50) * 14) + hasardFixe(i + 60) * 300, 300.0f);
        float px = x + (hasardFixe(i + 70) - 0.5f) * 300 * echelle + 25 * std::sin(temps * 1.2f + i);
        float py = haut - 40 + chute;
        float vie = 1 - chute / 300;
        Color couleur = i % 2 ? SEUIL_ROSE : SEUIL_CYAN;
        DrawCircle(px, py, 2.2f, Fade(couleur, vie));
        DrawCircleGradient(px, py, 7, Fade(couleur, vie * 0.4f), BLANK);
    }
    EndBlendMode();
}

// Le portail de runes : c'est par la qu'AYLIS repart vers une nouvelle vision
void dessinerPortail(bool survole) {
    float temps = GetTime();
    Vector2 p = positionAuSeuil(4);
    float cx = p.x;
    float cy = p.y + balancementIle() - 70;
    float rx = 46;
    float ry = 72;

    // L'interieur : un tourbillon de lumiere
    BeginBlendMode(BLEND_ADDITIVE);
    DrawEllipse(cx, cy, rx, ry, Fade(Color{80, 40, 160, 255}, 0.6f));
    for (int i = 0; i < 40; i++) {
        float angle = i * 0.5f + temps * 1.8f;
        float rayon = std::fmod(i * 7.0f + temps * 25, 60.0f) / 60.0f;
        DrawCircle(cx + std::cos(angle) * rx * rayon, cy + std::sin(angle) * ry * rayon, 2,
                   Fade(i % 2 ? SEUIL_CYAN : WHITE, 1 - rayon));
    }
    DrawCircleGradient(cx, cy, 90, Fade(SEUIL_VIOLET, survole ? 0.5f : 0.3f), BLANK);
    EndBlendMode();

    // L'arche de pierre, faite de petits blocs, et ses runes qui brillent
    for (int i = 0; i < 20; i++) {
        float angle = PI + i * PI / 19.0f;      // un demi-cercle, par le haut
        Vector2 bloc = {cx + std::cos(angle) * (rx + 8), cy + std::sin(angle) * (ry + 8)};
        DrawCircle(bloc.x, bloc.y, 8, Color{70, 64, 96, 255});
        DrawCircle(bloc.x, bloc.y - 2, 5, Color{96, 90, 124, 255});
        if (i % 4 == 2) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleGradient(bloc.x, bloc.y, 10, Fade(SEUIL_VIOLET, 0.6f + 0.4f * std::sin(temps * 3 + i)), BLANK);
            EndBlendMode();
        }
    }
    // Les deux piliers
    DrawRectangle(cx - rx - 16, cy, 16, ry + 4, Color{70, 64, 96, 255});
    DrawRectangle(cx + rx, cy, 16, ry + 4, Color{70, 64, 96, 255});
    if (survole) {
        texteCentre("Franchir le portail", {cx - 110, cy - ry - 44, 220, 20}, 20, WHITE);
    }
}

// Un echo (Maren, Durgan ou Silas) : un peu transparent, entoure d'une lueur, il flotte legerement
void dessinerEcho(int personnage, bool survole) {
    const Sprites& s = sprites();
    float temps = GetTime();
    Vector2 p = positionAuSeuil(personnage);
    float y = p.y + balancementIle();
    float flotte = 3 * std::sin(temps * 1.5f + personnage * 2);
    Color couleurs[3] = {Color{120, 230, 150, 255}, Color{255, 170, 80, 255}, SEUIL_VIOLET};

    DrawEllipse(p.x, y, 26, 8, Fade(BLACK, 0.35f));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(p.x, y - 40, survole ? 70 : 52, Fade(couleurs[personnage], survole ? 0.45f : 0.22f), BLANK);
    EndBlendMode();
    if (survole) {
        DrawEllipseLines(p.x, y, 34, 11, OR);
    }
    DrawTexturePro(s.marchands[personnage], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE},
                   {p.x - 40, y - 80 + flotte, 80, 80}, {0, 0}, 0, Fade(WHITE, 0.92f));
    texteCentre(TextFormat("%i. %s", personnage + 1, nomMarchand(personnage).c_str()), {p.x - 80, y + 12, 160, 20}, 20,
                survole ? OR : Fade(WHITE, 0.75f));
}

Rectangle rectangleEcho(int personnage) {
    Vector2 p = positionAuSeuil(personnage);
    return {p.x - 40, p.y - 85, 80, 110};
}

Rectangle rectanglePortail() {
    Vector2 p = positionAuSeuil(4);
    return {p.x - 60, p.y - 150, 120, 160};
}

Rectangle rectangleAmelioration(int choix) {
    return {100.0f + choix * 340, 390, 324, 150};
}

// La fenetre de discussion avec un echo, et ses deux ameliorations
void dessinerDiscussion(const Jeu& jeu) {
    int qui = jeu.interlocuteur;
    const Memoire& m = jeu.memoire;
    DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_FENETRE, Fade(BLACK, 0.5f));
    Rectangle cadre = {70, 150, LARGEUR_FENETRE - 140.0f, 460};
    dessinerCadre(cadre, Fade(Color{20, 14, 36, 255}, 0.97f), SEUIL_VIOLET, 3);

    // Le portrait, dans son halo
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(160, 240, 80, Fade(SEUIL_VIOLET, 0.35f), BLANK);
    EndBlendMode();
    DrawTexturePro(sprites().marchands[qui], {0, 0, TAILLE_SPRITE, TAILLE_SPRITE}, {96, 176, 128, 128}, {0, 0}, 0, WHITE);
    DrawText(nomMarchand(qui).c_str(), 250, 172, 40, OR);
    DrawText(TextFormat("%s  -  un echo du Seuil", titreMarchand(qui).c_str()), 252, 214, 20, TEXTE_GRIS);
    texteSurPlusieursLignes("\"" + paroleAuSeuil(m, qui) + "\"", {250, 250, cadre.width - 210, 110}, 20, RAYWHITE);

    for (int choix = 0; choix < 2; choix++) {
        int numero = qui * 2 + choix;
        int rang = m.ameliorations[numero];
        int cout = coutAmelioration(m, numero);
        bool possible = cout > 0 && m.fragments >= cout;
        Rectangle r = rectangleAmelioration(choix);
        bool survol = CheckCollisionPointRec(GetMousePosition(), r);
        Color bord = cout == 0 ? OR : (possible ? SEUIL_VIOLET : BORD);
        dessinerCadre(r, survol ? CADRE_CLAIR : CADRE, bord, survol ? 3 : 2);
        DrawText(TextFormat("%i", choix + 1), r.x + 12, r.y + 8, 20, Fade(bord, 0.9f));
        texteCentre(nomAmelioration(numero), {r.x, r.y + 14, r.width, 26}, 20, RAYWHITE);
        texteCentre(descriptionAmelioration(numero), {r.x, r.y + 46, r.width, 20}, 20, TEXTE_GRIS);
        // Les rangs : un petit losange par rang, allume s'il est achete
        int rangMax = rangMaxAmelioration(numero);
        for (int i = 0; i < rangMax; i++) {
            float x = r.x + r.width / 2 + (i - (rangMax - 1) / 2.0f) * 26;
            if (i < rang) {
                dessinerFragment(x, r.y + 88, 9);
            } else {
                DrawPolyLines({x, r.y + 88}, 4, 9, 0, BORD);
            }
        }
        if (cout == 0) {
            texteCentre("MAXIMUM", {r.x, r.y + 112, r.width, 24}, 20, OR);
        } else {
            std::string prix = std::to_string(cout);
            int largeur = MeasureText(prix.c_str(), 20) + 24;
            float x = r.x + (r.width - largeur) / 2;
            dessinerFragment(x + 8, r.y + 122, 8);
            DrawText(prix.c_str(), x + 24, r.y + 112, 20, possible ? RAYWHITE : Color{200, 100, 100, 255});
        }
    }
    texteCentre(jeu.messageSeuil, {cadre.x, 552, cadre.width, 20}, 20, Color{140, 230, 160, 255});
    texteCentre("1 / 2 : acheter      ECHAP : revenir", {cadre.x, 580, cadre.width, 20}, 20, TEXTE_GRIS);
}

void dessinerSeuil(const Jeu& jeu) {
    const Memoire& m = jeu.memoire;
    Vector2 souris = GetMousePosition();
    bool libre = jeu.interlocuteur < 0;     // pas de discussion en cours : on peut survoler les personnages

    dessinerCielDuSeuil();
    dessinerIleDuSeuil();
    dessinerArbreDuSeuil(m);
    dessinerPortail(libre && CheckCollisionPointRec(souris, rectanglePortail()));

    // AYLIS, devant l'arbre, dans la lumiere de la prophetie
    Vector2 a = positionAuSeuil(3);
    float ay = a.y + balancementIle();
    DrawEllipse(a.x, ay, 26, 8, Fade(BLACK, 0.35f));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(a.x, ay - 40, 60, Fade(Color{255, 236, 210, 255}, 0.25f), BLANK);
    EndBlendMode();
    dessinerPortraitAylis(jeu.aylis.stats.arme, {a.x - 40, ay - 80, 80, 80}, WHITE);

    for (int personnage = 0; personnage < 3; personnage++) {
        dessinerEcho(personnage, libre && CheckCollisionPointRec(souris, rectangleEcho(personnage)));
    }

    // Des feux follets qui tournent autour de l'ile
    float temps = GetTime();
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 9; i++) {
        float angle = temps * (0.25f + i * 0.03f) + i * 0.7f;
        float x = 432 + std::cos(angle) * (330 + i * 8);
        float y = 470 + std::sin(angle) * (120 + i * 6) - 40;
        Color couleur = i % 3 == 0 ? SEUIL_CYAN : (i % 3 == 1 ? SEUIL_VIOLET : SEUIL_ROSE);
        DrawCircleGradient(x, y, 14, Fade(couleur, 0.5f), BLANK);
        DrawCircle(x, y, 2.5f, WHITE);
    }
    EndBlendMode();

    // Le titre, le murmure, et les fragments
    texteCentre("LE SEUIL", {0, 22, (float)LARGEUR_FENETRE, 44}, 40, Color{225, 205, 255, 255});
    texteCentre(murmureDuSeuil(m), {0, 72, (float)LARGEUR_FENETRE, 20}, 20, Color{180, 170, 215, 255});
    dessinerFragment(LARGEUR_FENETRE - 110, 34, 11);
    DrawText(TextFormat("%i", m.fragments), LARGEUR_FENETRE - 92, 24, 20, RAYWHITE);
    DrawText("fragments", LARGEUR_FENETRE - 92, 46, 10, TEXTE_GRIS);

    if (libre) {
        texteCentre("1, 2, 3 : parler aux echos      ENTREE : franchir le portail",
                    {0, 736, (float)LARGEUR_FENETRE, 20}, 20, Color{190, 180, 220, 255});
        texteCentre(jeu.messageSeuil, {0, 712, (float)LARGEUR_FENETRE, 20}, 20, Color{140, 230, 160, 255});
    } else {
        dessinerDiscussion(jeu);
    }
}

// ===================== La chute et le reveil =====================

// AYLIS tombe : des anneaux de runes s'echappent de son corps, et tout se teinte de violet
void dessinerVisionBrisee(const Jeu& jeu) {
    float avancement = jeu.fondu / DUREE_FONDU;         // de 0 a 1
    if (avancement > 1) {
        avancement = 1;
    }
    Vector2 centre = positionAffichee(jeu.aylis);
    for (int i = 0; i < 3; i++) {
        float rayon = (avancement * 1.6f - i * 0.25f) * LARGEUR_FENETRE;
        if (rayon > 0) {
            DrawRing(centre, rayon, rayon + 6, 0, 360, 64, Fade(VIOLET_VISION, 0.6f * (1 - avancement)));
        }
    }
    DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_FENETRE, Fade(Color{20, 8, 36, 255}, avancement * 0.95f));
    // Des fissures de lumiere, comme une vitre qui se brise
    for (int i = 0; i < 9; i++) {
        float angle = i * 0.7f + 0.3f;
        float longueur = avancement * (200 + i * 40);
        Vector2 bout = {centre.x + std::cos(angle * 2.3f) * longueur, centre.y + std::sin(angle * 2.3f) * longueur};
        DrawLineEx(centre, bout, 2, Fade(VIOLET_VISION, 0.8f * avancement));
    }
    if (avancement > 0.35f) {
        float apparition = (avancement - 0.35f) / 0.65f;
        texteCentre("La vision se brise...", {0, HAUTEUR_FENETRE / 2.0f - 30, (float)LARGEUR_FENETRE, 40}, 40,
                    Fade(Color{220, 200, 255, 255}, apparition));
    }
}

void dessinerReveil(const Jeu& jeu) {
    dessinerFondVision();
    texteCentre("AYLIS SE REVEILLE", {0, 30, (float)LARGEUR_FENETRE, 50}, 50, VIOLET_VISION);

    // AYLIS, dans le halo de la prophetie
    float pulsation = 0.15f + 0.05f * std::sin(GetTime() * 2);
    DrawCircle(LARGEUR_FENETRE / 2, 150, 56, Fade(VIOLET_VISION, pulsation));
    dessinerPortraitAylis(jeu.aylis.stats.arme, {LARGEUR_FENETRE / 2.0f - 48, 100, 96, 96}, WHITE);

    // Le souvenir, qui s'ecrit lettre par lettre (aligne a gauche pour que les mots ne bougent pas)
    Rectangle cadre = {70, 220, LARGEUR_FENETRE - 140.0f, 190};
    dessinerCadre(cadre, Fade(PANNEAU, 0.9f), Fade(VIOLET_VISION, 0.6f), 2);
    std::vector<std::string> lignes = decouperEnLignes(texteDuReveil(jeu), cadre.width - 48, 20);
    int total = 0;
    for (const std::string& ligne : lignes) {
        total = total + ligne.size();
    }
    float part = jeu.fondu / DUREE_TEXTE_REVEIL;
    int visibles = part >= 1 ? total : (int)(total * part);
    for (int i = 0; i < (int)lignes.size() && visibles > 0; i++) {
        std::string morceau = lignes[i].substr(0, visibles);
        DrawText(morceau.c_str(), cadre.x + 24, cadre.y + 22 + i * 28, 20, RAYWHITE);
        visibles = visibles - lignes[i].size();
    }

    // Ce que cette vision a apporte
    Rectangle bilan = {70, 430, LARGEUR_FENETRE - 140.0f, 150};
    dessinerCadre(bilan, Fade(PANNEAU, 0.9f), Fade(OR, 0.6f), 2);
    texteCentre("CETTE VISION", {bilan.x, bilan.y + 12, bilan.width, 20}, 20, OR);
    texteCentre(TextFormat("Acte %i : %s  -  %s", acteDeLaSalle(jeu.salle) + 1, acte(acteDeLaSalle(jeu.salle)).nom.c_str(),
                           nomLieu(jeu.lieu).c_str()),
                {bilan.x, bilan.y + 44, bilan.width, 20}, 20, RAYWHITE);
    std::string runes = TextFormat("%i rune(s) : ", (int)jeu.runes.size());
    for (int i = 0; i < (int)jeu.runes.size(); i++) {
        runes = runes + (i > 0 ? ", " : "") + jeu.runes[i].nom;
    }
    if (jeu.runes.empty()) {
        runes = "Aucune rune";
    }
    texteCentre(runes, {bilan.x, bilan.y + 74, bilan.width, 20}, 20, TEXTE_GRIS);
    texteCentre(TextFormat("+%i fragments de prophetie  (total : %i)", jeu.fragmentsGagnes, jeu.memoire.fragments),
                {bilan.x, bilan.y + 108, bilan.width, 24}, 20, OR);

    if (jeu.fondu >= DUREE_TEXTE_REVEIL) {
        float clignote = 0.6f + 0.4f * std::sin(GetTime() * 4);
        texteCentre("ENTREE : reprendre la route", {0, 620, (float)LARGEUR_FENETRE, 24}, 20, Fade(RAYWHITE, clignote));
    }
    texteCentre(TextFormat("Visions vecues : %i", jeu.memoire.visions), {0, 700, (float)LARGEUR_FENETRE, 20}, 20, TEXTE_GRIS);
}

// ===================== Un nouvel acte =====================

void dessinerNouvelActe(const Jeu& jeu) {
    dessinerFondVision();
    int numeroActe = acteDeLaSalle(jeu.salle);
    const Acte& a = acte(numeroActe);
    const char* chiffres[NOMBRE_ACTES] = {"I", "II", "III", "IV"};
    float apparition = jeu.fondu > 1 ? 1 : jeu.fondu;
    texteCentre(TextFormat("ACTE %s", chiffres[numeroActe]), {0, 120, (float)LARGEUR_FENETRE, 70}, 70,
                Fade(Color{225, 205, 255, 255}, apparition));
    texteCentre(a.nom, {0, 210, (float)LARGEUR_FENETRE, 40}, 40, Fade(OR, apparition));
    // les lieux de l'acte
    std::string lieux;
    int dernier = -1;
    for (int lieu : a.lieux) {
        if (lieu != dernier) {
            lieux = lieux + (lieux.empty() ? "" : "   -   ") + nomLieu(lieu);
            dernier = lieu;
        }
    }
    texteCentre(lieux, {0, 280, (float)LARGEUR_FENETRE, 20}, 20, Fade(LIGHTGRAY, apparition));
    // le gardien qui attend au bout
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(LARGEUR_FENETRE / 2, 440, 110, Fade(Color{255, 60, 60, 255}, 0.3f * apparition), BLANK);
    EndBlendMode();
    dessinerPortraitBoss(a.boss, {LARGEUR_FENETRE / 2.0f - 70, 360, 140, 140});
    texteCentre(TextFormat("Au bout de la route : %s", nomDuBoss(a.boss).c_str()), {0, 520, (float)LARGEUR_FENETRE, 24}, 20,
                Color{255, 140, 130, 255});
    texteCentre("AYLIS reprend des forces : tous les pv, et une potion.", {0, 580, (float)LARGEUR_FENETRE, 20}, 20,
                Color{140, 230, 160, 255});
    if (jeu.fondu > 1.2f) {
        texteCentre("ENTREE : reprendre la route", {0, 660, (float)LARGEUR_FENETRE, 24}, 20,
                    Fade(RAYWHITE, 0.6f + 0.4f * std::sin(GetTime() * 4)));
    }
}

void dessinerJeu(const Jeu& jeu, int colonneSouris, int ligneSouris) {
    ClearBackground(FOND);

    if (jeu.phase == Phase::ChoixVoie) {
        dessinerChoixVoie(jeu);
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
    if (jeu.phase == Phase::Reveil) {
        dessinerReveil(jeu);
        return;
    }
    if (jeu.phase == Phase::Seuil) {
        dessinerSeuil(jeu);
        return;
    }
    if (jeu.phase == Phase::NouvelActe) {
        dessinerNouvelActe(jeu);
        return;
    }
    if (jeu.phase == Phase::Rencontre) {
        dessinerRencontre(jeu);
        return;
    }
    if (jeu.phase == Phase::Dialogue) {
        dessinerDialogue(jeu);
        return;
    }

    // L'arene est dessinee a travers une "camera" qui tremble quand un coup porte.
    // L'interface (bandeau, panneau...) reste immobile : elle est dessinee apres, hors camera.
    preparerLumiere(jeu);       // la carte de lumiere se dessine a part, avant la camera
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
        dessinerMessage("VORGATH EST VAINCU !", OR,
                        TextFormat("+%i fragments de prophetie - ENTREE : la fin de la route", jeu.fragmentsGagnes));
    } else if (jeu.phase == Phase::Defaite) {
        dessinerVisionBrisee(jeu);
    }
}
