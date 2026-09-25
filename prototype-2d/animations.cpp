// animations.cpp : tout ce qui bouge a l'ecran
//
// Quelques "trucs" utilises par beaucoup de jeux d'action pour que les coups aient du poids :
//   - le bond : l'attaquant s'elance vers sa cible puis revient,
//   - le recul : la cible est repoussee un instant,
//   - la secousse : tout l'ecran tremble un peu,
//   - l'arret sur image : sur un coup critique, le jeu se fige une fraction de seconde,
//   - les particules : des dizaines de petits points qui volent et s'effacent.
// La touche propre a VESPERANCE : les runes violettes de la prophetie, qui suivent AYLIS.
#include <cmath>
#include "animations.h"

const float DUREE_BOND = 0.24f;         // la duree d'un bond d'attaque, en secondes
const float DUREE_RECUL = 0.15f;
const float VITESSE_PROJECTILE = 900.0f; // en pixels par seconde
const int PARTICULES_MAX = 800;
const Color VIOLET_PROPHETIE = {175, 110, 255, 255};

// Un nombre au hasard entre min et max (a virgule)
float auHasard(float min, float max) {
    return min + (max - min) * GetRandomValue(0, 1000) / 1000.0f;
}

// Le centre de la case d'un pion, en pixels
Vector2 centreCase(const Pion& pion) {
    return {pion.colonne * TAILLE_CASE + TAILLE_CASE / 2.0f, pion.ligne * TAILLE_CASE + TAILLE_CASE / 2.0f};
}

Vector2 positionAffichee(const Pion& pion) {
    Vector2 position = {pion.xAffiche, pion.yAffiche};
    if (pion.xAffiche < 0) {
        position = centreCase(pion);    // pas encore place : directement au centre de sa case
    }
    // Le bond : une "bosse" qui monte puis redescend (un demi-sinus)
    if (pion.elan > 0) {
        float avancement = 1.0f - pion.elan / DUREE_BOND;
        float distance = std::sin(avancement * PI) * 22.0f;
        position.x = position.x + pion.elanX * distance;
        position.y = position.y + pion.elanY * distance;
    }
    if (pion.recul > 0) {
        float force = pion.recul / DUREE_RECUL * 7.0f;
        position.x = position.x + pion.reculX * force;
        position.y = position.y + pion.reculY * force;
    }
    return position;
}

// ===================== Les particules =====================

void ajouterParticule(Jeu& jeu, Particule particule) {
    if ((int)jeu.particules.size() < PARTICULES_MAX) {
        jeu.particules.push_back(particule);
    }
}

// Une gerbe de particules qui partent dans toutes les directions
void gerbe(Jeu& jeu, float x, float y, Color couleur, int nombre, float vitesse, bool gravite) {
    for (int i = 0; i < nombre; i++) {
        float angle = auHasard(0, 2 * PI);
        float v = auHasard(vitesse * 0.3f, vitesse);
        float vie = auHasard(0.25f, 0.6f);
        ajouterParticule(jeu, {x, y, std::cos(angle) * v, std::sin(angle) * v, vie, vie, auHasard(2, 5), couleur, gravite});
    }
}

// ===================== Les attaques =====================

float animerAttaque(Jeu& jeu, Pion& attaquant, const Pion& cible, bool aDistance, SorteProjectile sorte) {
    Vector2 depart = centreCase(attaquant);
    Vector2 arrivee = centreCase(cible);
    float dx = arrivee.x - depart.x;
    float dy = arrivee.y - depart.y;
    float longueur = std::sqrt(dx * dx + dy * dy);
    if (longueur < 1) {
        longueur = 1;
    }

    if (!aDistance) {
        // Au corps a corps : un bond vers la cible, qui touche au milieu du bond
        attaquant.elan = DUREE_BOND;
        attaquant.elanX = dx / longueur;
        attaquant.elanY = dy / longueur;
        return DUREE_BOND / 2;
    }

    // A distance : un projectile qui vole jusqu'a la cible
    float duree = 0.08f + longueur / VITESSE_PROJECTILE;
    if (sorte == SorteProjectile::BouleDeFeu) {
        duree = duree * 1.6f;   // la boule de feu est plus lente... et plus impressionnante
    }
    jeu.projectiles.push_back({depart.x, depart.y - 8, arrivee.x, arrivee.y - 8, 0.0f, duree, sorte});
    return duree;
}

void programmer(Jeu& jeu, float delai, SorteEffet sorte, float x, float y, Color couleur, float force, int pion) {
    jeu.effets.push_back({delai, sorte, x, y, couleur, force, pion});
}

void animerImpact(Jeu& jeu, int numeroPion, float delai, bool critique, Color couleur) {
    const Pion& pion = numeroPion == -1 ? jeu.aylis : (numeroPion == -2 ? jeu.allie : jeu.haschen[numeroPion]);
    Vector2 centre = centreCase(pion);
    programmer(jeu, delai, SorteEffet::Etincelles, centre.x, centre.y - 6, couleur, critique ? 24 : 12, numeroPion);
    programmer(jeu, delai, SorteEffet::Recul, centre.x, centre.y, couleur, 1, numeroPion);
    programmer(jeu, delai, SorteEffet::Secousse, 0, 0, couleur, critique ? 1.0f : 0.4f, numeroPion);
    if (critique) {
        programmer(jeu, delai, SorteEffet::ArretSurImage, 0, 0, couleur, 0.09f, numeroPion);
    }
}

void animerExplosion(Jeu& jeu, const Pion& cible, float delai) {
    Vector2 centre = centreCase(cible);
    programmer(jeu, delai, SorteEffet::Explosion, centre.x, centre.y, ORANGE, 1, -1);
    programmer(jeu, delai, SorteEffet::Secousse, 0, 0, ORANGE, 0.9f, -1);
}

void animerEclair(Jeu& jeu, const Pion& cible) {
    Vector2 centre = centreCase(cible);
    jeu.eclairs.push_back({centre.x, centre.y, 0.25f});
    gerbe(jeu, centre.x, centre.y, Color{200, 230, 255, 255}, 25, 260, false);
    jeu.secousse = jeu.secousse + 0.7f;
}

void animerSoin(Jeu& jeu, const Pion& pion) {
    Vector2 centre = centreCase(pion);
    for (int i = 0; i < 26; i++) {
        float vie = auHasard(0.6f, 1.1f);
        ajouterParticule(jeu, {centre.x + auHasard(-22, 22), centre.y + auHasard(-5, 25), auHasard(-8, 8),
                               auHasard(-90, -40), vie, vie, auHasard(3, 5), Color{110, 240, 130, 255}});
    }
}

void animerBouclier(Jeu& jeu, const Pion& pion) {
    Vector2 centre = centreCase(pion);
    for (int i = 0; i < 36; i++) {
        float angle = i * 2 * PI / 36;
        float vie = 0.7f;
        ajouterParticule(jeu, {centre.x + std::cos(angle) * 34, centre.y + std::sin(angle) * 34,
                               std::cos(angle) * 20, std::sin(angle) * 20, vie, vie, 4, Color{120, 210, 255, 255}});
    }
}

void animerEtat(Jeu& jeu, const Pion& pion, Color couleur) {
    Vector2 centre = centreCase(pion);
    gerbe(jeu, centre.x, centre.y, couleur, 10, 80, false);
}

void animerChute(Jeu& jeu, Pion& pion, float delai) {
    pion.disparition = 0.6f + delai;
    Vector2 centre = centreCase(pion);
    programmer(jeu, delai, SorteEffet::Fumee, centre.x, centre.y, GRAY, 1, -1);
}

// ===================== La mise a jour, a chaque image =====================

// Fait glisser un pion vers sa case (il parcourt une partie du chemin a chaque image : ca ralentit a l'arrivee)
void glisser(Pion& pion, float secondes) {
    Vector2 cible = centreCase(pion);
    if (pion.xAffiche < 0) {
        pion.xAffiche = cible.x;
        pion.yAffiche = cible.y;
        return;
    }
    float part = 1.0f - std::exp(-secondes * 14.0f);
    pion.xAffiche = pion.xAffiche + (cible.x - pion.xAffiche) * part;
    pion.yAffiche = pion.yAffiche + (cible.y - pion.yAffiche) * part;
}

// La barre de vie "fantome" descend doucement jusqu'aux vrais pv, une fois le coup arrive
void mettreAJourVieAffichee(Pion& pion, float secondes) {
    float pv = pion.stats.pv < 0 ? 0 : pion.stats.pv;
    if (pion.pvAffiches < 0 || pion.pvAffiches < pv) {
        pion.pvAffiches = pv;           // premier affichage, ou un soin : tout de suite
    } else if (pion.pvAffiches > pv && pion.flash < 0.1f) {
        pion.pvAffiches = pion.pvAffiches - pion.stats.pvMax * 0.9f * secondes;
        if (pion.pvAffiches < pv) {
            pion.pvAffiches = pv;
        }
    }
}

void mettreAJourPion(Pion& pion, float secondes) {
    glisser(pion, secondes);
    mettreAJourVieAffichee(pion, secondes);
    pion.elan = pion.elan - secondes;
    pion.recul = pion.recul - secondes;
    if (pion.disparition > 0) {
        pion.disparition = pion.disparition - secondes;
    }
}

// Declenche un effet programme, quand son delai est ecoule
void declencher(Jeu& jeu, const EffetEnAttente& effet) {
    if (effet.sorte == SorteEffet::Etincelles) {
        gerbe(jeu, effet.x, effet.y, effet.couleur, (int)effet.force, 230, true);
        gerbe(jeu, effet.x, effet.y, WHITE, (int)effet.force / 3, 300, false);
    } else if (effet.sorte == SorteEffet::Explosion) {
        gerbe(jeu, effet.x, effet.y, ORANGE, 45, 320, false);
        gerbe(jeu, effet.x, effet.y, YELLOW, 30, 220, false);
        gerbe(jeu, effet.x, effet.y, Color{90, 80, 80, 255}, 20, 120, false);
    } else if (effet.sorte == SorteEffet::Fumee) {
        for (int i = 0; i < 30; i++) {
            float vie = auHasard(0.5f, 1.0f);
            ajouterParticule(jeu, {effet.x + auHasard(-20, 20), effet.y + auHasard(-10, 20), auHasard(-25, 25),
                                   auHasard(-70, -20), vie, vie, auHasard(4, 8), Color{140, 130, 125, 255}});
        }
    } else if (effet.sorte == SorteEffet::Secousse) {
        jeu.secousse = jeu.secousse + effet.force;
    } else if (effet.sorte == SorteEffet::ArretSurImage) {
        jeu.arretSurImage = effet.force;
    } else if (effet.sorte == SorteEffet::Recul) {
        Pion& pion = effet.pion == -1 ? jeu.aylis : (effet.pion == -2 ? jeu.allie : jeu.haschen[effet.pion]);
        float angle = auHasard(0, 2 * PI);
        pion.recul = DUREE_RECUL;
        pion.reculX = std::cos(angle);
        pion.reculY = std::sin(angle);
    }
}

void mettreAJourAnimations(Jeu& jeu, float secondes) {
    // Les pions glissent, bondissent, reculent...
    float ancienX = jeu.aylis.xAffiche;
    float ancienY = jeu.aylis.yAffiche;
    mettreAJourPion(jeu.aylis, secondes);
    if (jeu.compagnon != COMPAGNON_AUCUN) {
        mettreAJourPion(jeu.allie, secondes);
    }
    for (Pion& h : jeu.haschen) {
        mettreAJourPion(h, secondes);
    }

    // AYLIS qui se deplace laisse une trainee de runes violettes (la prophetie qui l'accompagne)
    float vitesse = std::sqrt((jeu.aylis.xAffiche - ancienX) * (jeu.aylis.xAffiche - ancienX)
                              + (jeu.aylis.yAffiche - ancienY) * (jeu.aylis.yAffiche - ancienY));
    if (ancienX >= 0 && vitesse > 1.5f) {
        for (int i = 0; i < 2; i++) {
            float vie = auHasard(0.3f, 0.6f);
            Particule rune = {jeu.aylis.xAffiche + auHasard(-14, 14), jeu.aylis.yAffiche + auHasard(-6, 22),
                              auHasard(-10, 10), auHasard(-40, -10), vie, vie, auHasard(3, 5), VIOLET_PROPHETIE};
            rune.fantome = true;
            ajouterParticule(jeu, rune);
        }
    }

    // Les effets programmes : on les declenche quand leur delai est ecoule
    for (int i = (int)jeu.effets.size() - 1; i >= 0; i--) {
        jeu.effets[i].delai = jeu.effets[i].delai - secondes;
        if (jeu.effets[i].delai <= 0) {
            EffetEnAttente effet = jeu.effets[i];       // une copie : on l'efface de la liste juste apres
            jeu.effets.erase(jeu.effets.begin() + i);
            declencher(jeu, effet);
        }
    }

    // Les projectiles avancent ; la boule de feu laisse des flammes derriere elle
    for (int i = (int)jeu.projectiles.size() - 1; i >= 0; i--) {
        Projectile& p = jeu.projectiles[i];
        p.temps = p.temps + secondes;
        if (p.sorte == SorteProjectile::BouleDeFeu) {
            float t = p.temps / p.duree;
            float x = p.departX + (p.arriveeX - p.departX) * t;
            float y = p.departY + (p.arriveeY - p.departY) * t - std::sin(t * PI) * 40;
            float vie = auHasard(0.2f, 0.4f);
            ajouterParticule(jeu, {x, y, auHasard(-20, 20), auHasard(-40, 0), vie, vie, auHasard(3, 7),
                                   GetRandomValue(0, 1) ? ORANGE : YELLOW});
        }
        if (p.temps >= p.duree) {
            jeu.projectiles.erase(jeu.projectiles.begin() + i);
        }
    }

    // Les particules bougent et s'effacent
    for (int i = (int)jeu.particules.size() - 1; i >= 0; i--) {
        Particule& p = jeu.particules[i];
        p.x = p.x + p.vitesseX * secondes;
        p.y = p.y + p.vitesseY * secondes;
        if (p.gravite) {
            p.vitesseY = p.vitesseY + 500 * secondes;
        }
        p.vitesseX = p.vitesseX * (1.0f - 2.0f * secondes);     // un peu de frottement
        p.vie = p.vie - secondes;
        if (p.vie <= 0) {
            jeu.particules.erase(jeu.particules.begin() + i);
        }
    }

    for (int i = (int)jeu.eclairs.size() - 1; i >= 0; i--) {
        jeu.eclairs[i].vie = jeu.eclairs[i].vie - secondes;
        if (jeu.eclairs[i].vie <= 0) {
            jeu.eclairs.erase(jeu.eclairs.begin() + i);
        }
    }

    // La secousse s'apaise
    jeu.secousse = jeu.secousse - secondes * 3.0f;
    if (jeu.secousse < 0) {
        jeu.secousse = 0;
    }
    if (jeu.secousse > 1.5f) {
        jeu.secousse = 1.5f;
    }
}

bool animationsEnCours(const Jeu& jeu) {
    if (!jeu.projectiles.empty() || !jeu.effets.empty()) {
        return true;
    }
    for (const Pion& h : jeu.haschen) {
        if (h.disparition > 0 || h.elan > 0) {
            return true;
        }
    }
    return jeu.aylis.elan > 0 || (jeu.compagnon != COMPAGNON_AUCUN && jeu.allie.elan > 0);
}

// ===================== Le dessin =====================

void dessinerParticules(const Jeu& jeu) {
    for (const Particule& p : jeu.particules) {
        float visibilite = p.vie / p.vieMax;
        if (p.fantome) {
            // Une rune : un petit losange qui tourne sur lui-meme
            DrawPoly({p.x, p.y}, 4, p.taille, p.vie * 400, Fade(p.couleur, visibilite * 0.9f));
        } else {
            DrawRectangle(p.x - p.taille / 2, p.y - p.taille / 2, p.taille, p.taille, Fade(p.couleur, visibilite));
        }
    }
}

void dessinerProjectiles(const Jeu& jeu) {
    for (const Projectile& p : jeu.projectiles) {
        float t = p.temps / p.duree;
        float x = p.departX + (p.arriveeX - p.departX) * t;
        float y = p.departY + (p.arriveeY - p.departY) * t;
        float dx = p.arriveeX - p.departX;
        float dy = p.arriveeY - p.departY;
        float longueur = std::sqrt(dx * dx + dy * dy);
        if (longueur < 1) {
            longueur = 1;
        }
        dx = dx / longueur;
        dy = dy / longueur;

        if (p.sorte == SorteProjectile::Fleche || p.sorte == SorteProjectile::Javelot) {
            // Une tige, et une pointe au bout
            float taille = p.sorte == SorteProjectile::Javelot ? 30 : 22;
            float epaisseur = p.sorte == SorteProjectile::Javelot ? 4 : 2;
            Color bois = p.sorte == SorteProjectile::Javelot ? Color{210, 170, 70, 255} : Color{150, 105, 60, 255};
            DrawLineEx({x - dx * taille, y - dy * taille}, {x, y}, epaisseur, bois);
            DrawTriangle({x + dx * 8, y + dy * 8}, {x - dy * 5, y + dx * 5}, {x + dy * 5, y - dx * 5}, LIGHTGRAY);
            DrawTriangle({x + dx * 8, y + dy * 8}, {x + dy * 5, y - dx * 5}, {x - dy * 5, y + dx * 5}, LIGHTGRAY);
        } else if (p.sorte == SorteProjectile::Orbe) {
            DrawCircle(x, y, 10, Fade(VIOLET_PROPHETIE, 0.35f));
            DrawCircle(x, y, 6, VIOLET_PROPHETIE);
            DrawCircle(x, y, 3, WHITE);
        } else {
            // La boule de feu suit une courbe (elle monte puis redescend)
            y = y - std::sin(t * PI) * 40;
            DrawCircle(x, y, 16, Fade(ORANGE, 0.35f));
            DrawCircle(x, y, 11, ORANGE);
            DrawCircle(x, y, 6, YELLOW);
        }
    }
}

void dessinerEclairs(const Jeu& jeu) {
    for (const EffetEclair& e : jeu.eclairs) {
        float visibilite = e.vie / 0.25f;
        // Tout l'ecran s'illumine un instant
        DrawRectangle(0, 0, LARGEUR_FENETRE, HAUTEUR_ARENE, Fade(Color{200, 225, 255, 255}, 0.25f * visibilite));
        // Un zigzag qui descend du haut de l'ecran jusqu'a la cible
        Vector2 precedent = {e.x, 0};
        int segments = 8;
        for (int i = 1; i <= segments; i++) {
            float y = e.y * i / segments;
            float x = i == segments ? e.x : e.x + GetRandomValue(-18, 18);
            DrawLineEx(precedent, {x, y}, 6, Fade(Color{150, 200, 255, 255}, visibilite));
            DrawLineEx(precedent, {x, y}, 2, Fade(WHITE, visibilite));
            precedent = {x, y};
        }
    }
}

Camera2D cameraAvecSecousse(const Jeu& jeu) {
    // Sur un ecran agrandi par Windows (150%, 200%...), la camera doit agrandir pareil,
    // sinon l'arene serait dessinee en tout petit dans un coin
    Vector2 echelle = GetWindowScaleDPI();
    Camera2D camera = {};
    camera.zoom = echelle.x;
    if (jeu.secousse > 0) {
        float force = jeu.secousse * 8.0f;
        camera.offset = {auHasard(-force, force) * echelle.x, auHasard(-force, force) * echelle.y};
    }
    return camera;
}
