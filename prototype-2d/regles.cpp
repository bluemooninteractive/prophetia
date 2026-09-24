// regles.cpp : les regles du jeu en vue du dessus.
// Les formules (degats, critiques, etats, rage, sorts...) sont les memes que dans le jeu console :
// seule nouveaute, les distances se comptent maintenant en cases sur l'arene.
#include <cstdlib>
#include "jeu2d.h"
#include "animations.h"

// ===================== Les petites fonctions utiles =====================

int distanceCases(int c1, int l1, int c2, int l2) {
    return std::abs(c1 - c2) + std::abs(l1 - l2);
}

int distanceEntre(const Pion& a, const Pion& b) {
    return distanceCases(a.colonne, a.ligne, b.colonne, b.ligne);
}

bool estDansArene(int colonne, int ligne) {
    return colonne >= 0 && colonne < COLONNES && ligne >= 0 && ligne < LIGNES;
}

bool estRocher(const Jeu& jeu, int colonne, int ligne) {
    return jeu.rochers[ligne * COLONNES + colonne];
}

int haschenSurCase(const Jeu& jeu, int colonne, int ligne) {
    int nombre = jeu.haschen.size();
    for (int i = 0; i < nombre; i++) {
        const Pion& h = jeu.haschen[i];
        if (h.stats.estDebout() && h.colonne == colonne && h.ligne == ligne) {
            return i;
        }
    }
    return -1;
}

// Est-ce qu'un pion peut se poser sur cette case ?
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
    if (jeu.journal.size() > 3) {
        jeu.journal.erase(jeu.journal.begin());
    }
}

// Un texte qui s'envole au-dessus d'un pion
// (delai : il n'apparait qu'au moment ou le coup arrive vraiment, par exemple quand la fleche touche)
void ajouterTexte(Jeu& jeu, const Pion& pion, const std::string& texte, Color couleur, float delai = 0.0f) {
    float x = pion.colonne * TAILLE_CASE + TAILLE_CASE / 2.0f - 12;
    float y = pion.ligne * TAILLE_CASE;
    jeu.textes.push_back({texte, x, y, 1.2f, couleur, delai});
}

void mettreAJourTextes(Jeu& jeu, float secondes) {
    for (TexteFlottant& t : jeu.textes) {
        if (t.delai > 0) {
            t.delai = t.delai - secondes;   // pas encore apparu
            continue;
        }
        t.age = t.age + secondes;
        t.y = t.y - 40 * secondes;
        t.tempsRestant = t.tempsRestant - secondes;
    }
    while (!jeu.textes.empty() && jeu.textes.front().tempsRestant <= 0) {
        jeu.textes.erase(jeu.textes.begin());
    }

    jeu.tempsBanniere = jeu.tempsBanniere - secondes;

    // Le clignotement rouge des pions touches s'eteint peu a peu
    jeu.aylis.flash = jeu.aylis.flash - secondes;
    for (Pion& h : jeu.haschen) {
        h.flash = h.flash - secondes;
    }
}

// ===================== Les deplacements =====================

// On part de la case d'AYLIS et on "s'etend" case par case, comme une tache d'encre,
// sans traverser les rochers ni les Haschen. C'est un "parcours en largeur".
std::vector<int> casesAtteignables(const Jeu& jeu) {
    std::vector<int> pas(COLONNES * LIGNES, -1);
    std::vector<int> aVisiter;
    int depart = jeu.aylis.ligne * COLONNES + jeu.aylis.colonne;
    pas[depart] = 0;
    aVisiter.push_back(depart);

    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < (int)aVisiter.size(); i++) {
        int caseActuelle = aVisiter[i];
        int colonne = caseActuelle % COLONNES;
        int ligne = caseActuelle / COLONNES;
        if (pas[caseActuelle] == DEPLACEMENT_AYLIS) {
            continue;   // on ne peut pas aller plus loin
        }
        for (const auto& dir : directions) {
            int c = colonne + dir[0];
            int l = ligne + dir[1];
            if (caseLibre(jeu, c, l) && pas[l * COLONNES + c] == -1) {
                pas[l * COLONNES + c] = pas[caseActuelle] + 1;
                aVisiter.push_back(l * COLONNES + c);
            }
        }
    }
    return pas;
}

// Un Haschen fait un pas vers AYLIS, si une case libre le rapproche. Renvoie false s'il est bloque.
bool unPasVersAylis(Jeu& jeu, Pion& h) {
    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int meilleureDistance = distanceEntre(h, jeu.aylis);
    int meilleureColonne = h.colonne;
    int meilleureLigne = h.ligne;
    for (const auto& dir : directions) {
        int c = h.colonne + dir[0];
        int l = h.ligne + dir[1];
        if (caseLibre(jeu, c, l)) {
            int d = distanceCases(c, l, jeu.aylis.colonne, jeu.aylis.ligne);
            if (d < meilleureDistance) {
                meilleureDistance = d;
                meilleureColonne = c;
                meilleureLigne = l;
            }
        }
    }
    bool aBouge = meilleureColonne != h.colonne || meilleureLigne != h.ligne;
    h.colonne = meilleureColonne;
    h.ligne = meilleureLigne;
    return aBouge;
}

// Avance de "nombre" pas au maximum, en s'arretant au contact d'AYLIS
void avancer(Jeu& jeu, Pion& h, int nombre) {
    for (int pas = 0; pas < nombre; pas++) {
        if (distanceEntre(h, jeu.aylis) == 1 || !unPasVersAylis(jeu, h)) {
            return;
        }
    }
}

// ===================== Les degats et les etats (les memes regles que la console) =====================

// attaque x puissance - defense, un peu de hasard, et parfois un critique (x2)
int calculerDegats2D(int attaque, int puissance, int defense, int chanceCritique, bool& critique) {
    int degats = attaque * puissance / 100 - defense + GetRandomValue(-1, 1);
    critique = GetRandomValue(1, 100) <= chanceCritique;
    if (critique) {
        degats = degats * 2;
    }
    if (degats < 1) {
        degats = 1;
    }
    return degats;
}

// Un Haschen encaisse des degats
// (delai : le temps avant que le coup arrive, pour que les animations tombent au bon moment)
void blesserHaschen(Jeu& jeu, Pion& cible, int degats, bool critique, float delai) {
    cible.stats.pv = cible.stats.pv - degats;
    cible.flash = 0.25f + delai;
    ajouterTexte(jeu, cible, (critique ? "CRIT -" : "-") + std::to_string(degats), critique ? ORANGE : YELLOW, delai);

    int numero = &cible - &jeu.haschen[0];      // le numero du Haschen dans la liste
    animerImpact(jeu, numero, delai, critique, critique ? ORANGE : YELLOW);
    if (!cible.stats.estDebout()) {
        ecrireJournal(jeu, cible.stats.nom + " tombe !");
        animerChute(jeu, cible, delai);
    }
}

// AYLIS encaisse un coup : la garde divise par 2, le bouclier absorbe, la rage se remplit
void toucherAylis(Jeu& jeu, const Pion& attaquant, int degats, bool critique, float delai) {
    Combattant& aylis = jeu.aylis.stats;
    if (jeu.enGarde) {
        degats = degats / 2;
    }
    if (aylis.bouclier > 0) {
        int absorbe = degats < aylis.bouclier ? degats : aylis.bouclier;
        aylis.bouclier = aylis.bouclier - absorbe;
        degats = degats - absorbe;
    }
    aylis.pv = aylis.pv - degats;
    jeu.aylis.flash = 0.25f + delai;
    jeu.rage.remplir(degats * 4);
    ajouterTexte(jeu, jeu.aylis, (critique ? "CRIT -" : "-") + std::to_string(degats), RED, delai);
    animerImpact(jeu, -1, delai, critique, RED);
    ecrireJournal(jeu, attaquant.stats.nom + " touche AYLIS : -" + std::to_string(degats) + " pv");

    // Les coups de certains Haschen empoisonnent (une chance sur deux)
    if (attaquant.stats.attaquePoison && degats > 0 && GetRandomValue(0, 1) == 0) {
        aylis.poison = 3;
        ecrireJournal(jeu, "Poison ! AYLIS perdra des pv a chaque tour.");
    }
}

// Au debut de son tour, un pion subit ses etats (poison, brulure, saignement)
void subirEtats(Jeu& jeu, Pion& pion) {
    int total = 0;
    if (pion.stats.poison > 0) {
        total = total + 3;
        pion.stats.poison = pion.stats.poison - 1;
    }
    if (pion.stats.brulure > 0) {
        total = total + 4;
        pion.stats.brulure = pion.stats.brulure - 1;
    }
    if (pion.stats.saignement > 0) {
        total = total + 3;
        pion.stats.saignement = pion.stats.saignement - 1;
    }
    if (total > 0) {
        pion.stats.pv = pion.stats.pv - total;
        ajouterTexte(jeu, pion, "-" + std::to_string(total), PURPLE);
        animerEtat(jeu, pion, PURPLE);
        if (!pion.stats.estDebout()) {
            ecrireJournal(jeu, pion.stats.nom + " succombe a ses blessures !");
            if (&pion != &jeu.aylis) {
                animerChute(jeu, pion, 0.0f);
            }
        }
    }
}

// ===================== Les actions d'AYLIS =====================

std::string nomAction(Action action) {
    switch (action) {
        case Action::Attaque: return "Attaque";
        case Action::AttaqueLourde: return "Lourde";
        case Action::Garde: return "Garde";
        case Action::Potion: return "Potion";
        case Action::BouleDeFeu: return "Feu";
        case Action::Soin: return "Soin";
        case Action::Eclair: return "Eclair";
        case Action::Bouclier: return "Bouclier";
        case Action::Speciale: return "SPECIAL";
    }
    return "";
}

std::string descriptionAction(Action action) {
    switch (action) {
        case Action::Attaque: return "Un coup avec ton arme. Ne rate jamais.";
        case Action::AttaqueLourde: return "Degats x1.8, mais rate 4 fois sur 10.";
        case Action::Garde: return "Petits degats, et tu encaisses 2 fois moins jusqu'a ton prochain tour.";
        case Action::Potion: return "Rend 15 pv. Clique sur AYLIS (ou ENTREE).";
        case Action::BouleDeFeu: return "Degats x1.5 sans defense, sur la cible ET ses voisins. Fait bruler 2 tours.";
        case Action::Soin: return "Rend 20 pv et guerit poison, brulure et saignement.";
        case Action::Eclair: return "Degats normaux, et la cible est paralysee : elle passe son prochain tour.";
        case Action::Bouclier: return "Une barriere qui absorbe les 15 prochains points de degats.";
        case Action::Speciale: return "Quand la rage est pleine : degats x2.2, ne rate jamais.";
    }
    return "";
}

int porteeAction(const Jeu& jeu, Action action) {
    switch (action) {
        case Action::Attaque:
        case Action::AttaqueLourde:
        case Action::Garde:
        case Action::Speciale:
            return jeu.aylis.stats.arme.aDistance ? PORTEE_DISTANCE : PORTEE_MELEE;
        case Action::BouleDeFeu:
        case Action::Eclair:
            return PORTEE_SORT;
        default:
            return 0;   // potion, soin, bouclier : sur AYLIS
    }
}

bool actionSurSoi(Action action) {
    return action == Action::Potion || action == Action::Soin || action == Action::Bouclier;
}

int coutMana(Action action) {
    switch (action) {
        case Action::BouleDeFeu: return 4;
        case Action::Soin: return 5;
        case Action::Eclair: return 7;
        case Action::Bouclier: return 5;
        default: return 0;
    }
}

// Le numero du sort dans l'ordre d'apprentissage (1 = Boule de feu... 4 = Bouclier), 0 si ce n'est pas un sort
int numeroDuSort(Action action) {
    switch (action) {
        case Action::BouleDeFeu: return 1;
        case Action::Soin: return 2;
        case Action::Eclair: return 3;
        case Action::Bouclier: return 4;
        default: return 0;
    }
}

bool actionDisponible(const Jeu& jeu, Action action) {
    const Combattant& aylis = jeu.aylis.stats;
    if (numeroDuSort(action) > aylis.sortsConnus) {
        return false;
    }
    if (aylis.mana < coutMana(action)) {
        return false;
    }
    if (action == Action::Potion && aylis.potions == 0) {
        return false;
    }
    if (action == Action::Speciale && !jeu.rage.estPleine()) {
        return false;
    }
    return true;
}

// AYLIS frappe avec son arme (une arme a distance est moins efficace au contact ; 2 coups peuvent faire saigner)
void frapper(Jeu& jeu, Pion& cible, int puissance) {
    const Arme& arme = jeu.aylis.stats.arme;
    if (arme.aDistance && distanceEntre(jeu.aylis, cible) == 1) {
        puissance = puissance * 60 / 100;
    }
    int total = 0;
    bool unCritique = false;
    bool faitSaigner = false;
    for (int coup = 0; coup < arme.nombreDeCoups; coup++) {
        bool critique = false;
        total = total + calculerDegats2D(jeu.aylis.stats.attaque + arme.bonusAttaque, puissance,
                                         cible.stats.defense, arme.chanceCritique, critique);
        unCritique = unCritique || critique;
        if (arme.nombreDeCoups > 1 && GetRandomValue(1, 4) == 1) {
            faitSaigner = true;
        }
    }
    // L'animation : un bond au corps a corps, ou un projectile (une orbe pour le baton, une fleche sinon)
    SorteProjectile sorte = arme.nom.find("Baton") != std::string::npos ? SorteProjectile::Orbe : SorteProjectile::Fleche;
    float delai = animerAttaque(jeu, jeu.aylis, cible, arme.aDistance, sorte);

    ecrireJournal(jeu, "AYLIS frappe " + cible.stats.nom + " : -" + std::to_string(total) + " pv");
    blesserHaschen(jeu, cible, total, unCritique, delai);
    if (faitSaigner && cible.stats.estDebout()) {
        cible.stats.saignement = 3;
    }
}

bool resteDesHaschen(const Jeu& jeu) {
    for (const Pion& h : jeu.haschen) {
        if (h.stats.estDebout()) {
            return true;
        }
    }
    return false;
}

bool agirSurHaschen(Jeu& jeu, int numero) {
    Action action = jeu.actionChoisie;
    Pion& cible = jeu.haschen[numero];
    if (actionSurSoi(action) || !actionDisponible(jeu, action)) {
        return false;
    }
    if (distanceEntre(jeu.aylis, cible) > porteeAction(jeu, action)) {
        ecrireJournal(jeu, "Trop loin pour " + nomAction(action) + " !");
        return false;
    }

    Combattant& aylis = jeu.aylis.stats;
    aylis.mana = aylis.mana - coutMana(action);

    if (action == Action::Attaque) {
        frapper(jeu, cible, 100);
    } else if (action == Action::AttaqueLourde) {
        if (GetRandomValue(1, 100) <= 60) {
            frapper(jeu, cible, 180);
        } else {
            float delai = animerAttaque(jeu, jeu.aylis, cible, aylis.arme.aDistance, SorteProjectile::Fleche);
            ecrireJournal(jeu, "Attaque lourde... ratee !");
            ajouterTexte(jeu, cible, "rate", LIGHTGRAY, delai);
        }
    } else if (action == Action::Garde) {
        frapper(jeu, cible, 60);
        jeu.enGarde = true;
    } else if (action == Action::Speciale) {
        frapper(jeu, cible, 220);
        jeu.rage.vider();
    } else if (action == Action::BouleDeFeu) {
        // La boule de feu explose : la cible ET les Haschen juste a cote d'elle
        ecrireJournal(jeu, "BOULE DE FEU !");
        float delai = animerAttaque(jeu, jeu.aylis, cible, true, SorteProjectile::BouleDeFeu);
        animerExplosion(jeu, cible, delai);
        for (Pion& h : jeu.haschen) {
            if (h.stats.estDebout() && distanceEntre(h, cible) <= 1) {
                bool critique = false;
                int degats = calculerDegats2D(aylis.attaque, 150, 0, 10, critique);
                blesserHaschen(jeu, h, degats, critique, delai);
                h.stats.brulure = 2;
            }
        }
    } else if (action == Action::Eclair) {
        bool critique = false;
        int degats = calculerDegats2D(aylis.attaque, 100, cible.stats.defense, 10, critique);
        ecrireJournal(jeu, "ECLAIR ! " + cible.stats.nom + " est paralyse.");
        animerEclair(jeu, cible);
        blesserHaschen(jeu, cible, degats, critique, 0.05f);
        cible.stats.etourdi = true;
    }

    finirTourAylis(jeu);
    return true;
}

bool agirSurSoi(Jeu& jeu) {
    Action action = jeu.actionChoisie;
    Combattant& aylis = jeu.aylis.stats;
    if (!actionSurSoi(action) || !actionDisponible(jeu, action)) {
        return false;
    }
    aylis.mana = aylis.mana - coutMana(action);

    if (action == Action::Potion) {
        aylis.potions = aylis.potions - 1;
        aylis.soigner(15);                          // une methode du Combattant du jeu console
        ajouterTexte(jeu, jeu.aylis, "+15", GREEN);
        animerSoin(jeu, jeu.aylis);
        ecrireJournal(jeu, "AYLIS boit une potion.");
    } else if (action == Action::Soin) {
        aylis.soigner(20);
        aylis.poison = 0;
        aylis.brulure = 0;
        aylis.saignement = 0;
        ajouterTexte(jeu, jeu.aylis, "+20", GREEN);
        animerSoin(jeu, jeu.aylis);
        ecrireJournal(jeu, "SOIN ! Les blessures d'AYLIS se referment.");
    } else {
        aylis.bouclier = 15;
        ajouterTexte(jeu, jeu.aylis, "BOUCLIER", SKYBLUE);
        animerBouclier(jeu, jeu.aylis);
        ecrireJournal(jeu, "Une barriere de lumiere entoure AYLIS.");
    }
    finirTourAylis(jeu);
    return true;
}

void deplacerAylis(Jeu& jeu, int colonne, int ligne) {
    if (jeu.phase != Phase::Deplacement) {
        return;
    }
    std::vector<int> pas = casesAtteignables(jeu);
    if (pas[ligne * COLONNES + colonne] > 0) {
        jeu.aylis.colonne = colonne;
        jeu.aylis.ligne = ligne;
        jeu.phase = Phase::Action;
    }
}

void finirTourAylis(Jeu& jeu) {
    if (!resteDesHaschen(jeu)) {
        if (jeu.combat + 1 >= nombreDeCombats()) {
            jeu.phase = Phase::Victoire;
        } else {
            jeu.phase = Phase::CombatGagne;
        }
        return;
    }
    jeu.phase = Phase::TourEnnemi;
    jeu.ennemiQuiJoue = 0;
    jeu.minuteur = 0.0f;
}

// ===================== Le tour des Haschen =====================

void jouerHaschen(Jeu& jeu, Pion& h) {
    Combattant& stats = h.stats;

    subirEtats(jeu, h);
    if (!stats.estDebout()) {
        return;
    }
    if (stats.etourdi) {
        stats.etourdi = false;
        ajouterTexte(jeu, h, "paralyse", SKYBLUE);
        return;
    }

    // Un boss blesse s'enrage une fois, et boit sa potion s'il est en danger
    if (stats.estBoss && !stats.enrage && stats.pv <= stats.pvMax / 2) {
        stats.enrage = true;
        stats.attaque = stats.attaque + 3;
        ecrireJournal(jeu, stats.nom + " S'ENRAGE !");
        ajouterTexte(jeu, h, "RAGE", RED);
    }
    if (stats.estBoss && stats.pv <= 12 && stats.potions > 0) {
        stats.potions = stats.potions - 1;
        stats.soigner(15);
        ajouterTexte(jeu, h, "+15", GREEN);
        animerSoin(jeu, h);
        ecrireJournal(jeu, stats.nom + " boit une potion !");
        return;
    }

    int distanceDepart = distanceEntre(h, jeu.aylis);
    bool critique = false;

    // Un lanceur tire de loin une fois sur deux
    if (stats.style == Style::Lanceur && distanceDepart > 1 && distanceDepart <= PORTEE_LANCEUR
        && GetRandomValue(0, 1) == 0) {
        int degats = calculerDegats2D(stats.attaque, 80, jeu.aylis.stats.defense, 10, critique);
        SorteProjectile sorte = stats.estBoss ? SorteProjectile::Javelot : SorteProjectile::Fleche;
        float delai = animerAttaque(jeu, h, jeu.aylis, true, sorte);
        toucherAylis(jeu, h, degats, critique, delai);
        return;
    }

    // Sinon il avance : un chargeur va plus loin
    int pas = stats.style == Style::Chargeur ? 4 : 2;
    avancer(jeu, h, pas);

    if (distanceEntre(h, jeu.aylis) == 1) {
        int puissance = 100;
        // Un boss tente parfois une attaque lourde
        if (stats.estBoss && GetRandomValue(1, 4) == 1) {
            if (GetRandomValue(1, 100) > 60) {
                animerAttaque(jeu, h, jeu.aylis, false, SorteProjectile::Fleche);
                ecrireJournal(jeu, stats.nom + " rate son attaque lourde !");
                return;
            }
            puissance = 180;
        }
        if (stats.style == Style::Chargeur && distanceDepart > 1) {
            ecrireJournal(jeu, stats.nom + " CHARGE !");
        }
        int degats = calculerDegats2D(stats.attaque, puissance, jeu.aylis.stats.defense, 10, critique);
        float delai = animerAttaque(jeu, h, jeu.aylis, false, SorteProjectile::Fleche);
        toucherAylis(jeu, h, degats, critique, delai);
    }
}

void commencerTourAylis(Jeu& jeu) {
    jeu.tour = jeu.tour + 1;
    jeu.banniere = "TOUR " + std::to_string(jeu.tour);
    jeu.tempsBanniere = 1.2f;
    jeu.enGarde = false;
    jeu.phase = Phase::Deplacement;
    subirEtats(jeu, jeu.aylis);
    if (!jeu.aylis.stats.estDebout()) {
        jeu.phase = Phase::Defaite;
    }
}

void mettreAJourTourEnnemi(Jeu& jeu, float secondes) {
    jeu.minuteur = jeu.minuteur + secondes;
    if (jeu.minuteur < PAUSE_ENTRE_ENNEMIS) {
        return;
    }
    jeu.minuteur = 0.0f;

    int nombre = jeu.haschen.size();
    while (jeu.ennemiQuiJoue < nombre && !jeu.haschen[jeu.ennemiQuiJoue].stats.estDebout()) {
        jeu.ennemiQuiJoue = jeu.ennemiQuiJoue + 1;
    }
    if (jeu.ennemiQuiJoue >= nombre) {
        commencerTourAylis(jeu);
        return;
    }

    jouerHaschen(jeu, jeu.haschen[jeu.ennemiQuiJoue]);
    jeu.ennemiQuiJoue = jeu.ennemiQuiJoue + 1;

    if (!jeu.aylis.stats.estDebout()) {
        jeu.phase = Phase::Defaite;
    } else if (!resteDesHaschen(jeu)) {
        finirTourAylis(jeu);    // un Haschen a succombe a ses etats pendant son tour
    }
}

// ===================== Les voies et les combats =====================

void choisirVoie(Jeu& jeu, int voie) {
    //                   nom      pv  pvMax att def potions boss   xp or
    Combattant aylis = {"AYLIS",  40, 40,   12,  4,  3,      false, 0, 0};
    if (voie == 1) {
        aylis.voie = "de l'epee";
        aylis.pv = 44;
        aylis.pvMax = 44;
        aylis.attaque = 13;
        aylis.mana = 6;
        aylis.manaMax = 6;
        aylis.arme = {"Epee courte", false, 0, 10, 1, 20};
    } else if (voie == 2) {
        aylis.voie = "de l'arc";
        aylis.arme = {"Arc court", true, 0, 15, 1, 45};
    } else {
        aylis.voie = "des arcanes";
        aylis.pv = 34;
        aylis.pvMax = 34;
        aylis.defense = 3;
        aylis.potions = 2;
        aylis.mana = 20;
        aylis.manaMax = 20;
        aylis.sortsConnus = 4;      // dans le prototype, la voie des arcanes connait deja tous les sorts
        aylis.arme = {"Baton de mage", true, 0, 5, 1, 20};
    }
    jeu.aylis.stats = aylis;
    jeu.aylis.couleur = SKYBLUE;
    jeu.combat = 0;
    jeu.rage.vider();
    preparerCombat(jeu);
}

int nombreDeCombats() {
    return 3;
}

void preparerCombat(Jeu& jeu) {
    jeu.rochers.assign(COLONNES * LIGNES, false);
    jeu.haschen.clear();
    jeu.journal.clear();
    jeu.textes.clear();

    // On repart d'un ecran calme : plus de particules, de projectiles ni de secousse
    jeu.particules.clear();
    jeu.projectiles.clear();
    jeu.effets.clear();
    jeu.eclairs.clear();
    jeu.secousse = 0.0f;
    jeu.arretSurImage = 0.0f;
    jeu.aylis.xAffiche = -1.0f;     // AYLIS apparait directement a sa place de depart
    jeu.aylis.pvAffiches = -1.0f;
    jeu.aylis.elan = 0.0f;
    jeu.aylis.recul = 0.0f;
    jeu.aylis.flash = 0.0f;

    Combattant& aylis = jeu.aylis.stats;
    aylis.mana = aylis.manaMax;     // le mana se recharge a chaque combat
    aylis.poison = 0;
    aylis.brulure = 0;
    aylis.saignement = 0;
    aylis.bouclier = 0;
    jeu.aylis.colonne = 1;
    jeu.aylis.ligne = 3;

    // Les memes Haschen que dans le jeu console (sans le butin, pour le prototype)
    //                        nom                   pv  pvMax att def potions boss   xp  or
    Combattant eclaireur  = {"Haschen eclaireur",   18, 18,   8,  1, 0,      false, 20, 20};
    Combattant guerrier   = {"Haschen guerrier",    22, 22,   9,  2, 0,      false, 25, 25};
    Combattant traqueur   = {"Haschen traqueur",    20, 20,   9,  1, 0,      false, 25, 22};
    Combattant chaman     = {"Haschen chaman",      20, 20,  10,  1, 0,      false, 25, 25};
    Combattant louvetier  = {"Haschen louvetier",   22, 22,  10,  2, 0,      false, 25, 25};
    Combattant ashka      = {"Ashka",               40, 40,  14,  4, 1,      true,  50, 50};
    traqueur.style = Style::Lanceur;
    chaman.attaquePoison = true;
    louvetier.style = Style::Chargeur;
    ashka.style = Style::Lanceur;

    std::vector<std::pair<int, int>> rochers;
    if (jeu.combat == 0) {
        jeu.nomDuLieu = "La foret des Brumes";
        rochers = {{5, 1}, {5, 2}, {6, 5}, {6, 6}, {3, 5}, {8, 3}};
        jeu.haschen = {
            {eclaireur, 9, 1, ORANGE},
            {guerrier, 10, 4, RED},
            {traqueur, 9, 6, GREEN},
        };
    } else if (jeu.combat == 1) {
        jeu.nomDuLieu = "Le camp de guerre haschen";
        rochers = {{4, 3}, {4, 4}, {7, 1}, {7, 6}, {9, 3}};
        jeu.haschen = {
            {louvetier, 10, 2, BROWN},
            {chaman, 9, 5, PURPLE},
            {guerrier, 11, 4, RED},
        };
    } else {
        jeu.nomDuLieu = "Le col d'Ashka";
        rochers = {{4, 1}, {4, 6}, {6, 3}, {6, 4}, {8, 1}, {8, 6}};
        jeu.haschen = {
            {ashka, 10, 3, GOLD},
            {traqueur, 11, 6, GREEN},
            {eclaireur, 11, 0, ORANGE},
        };
    }
    for (const auto& rocher : rochers) {
        jeu.rochers[rocher.second * COLONNES + rocher.first] = true;
    }

    jeu.tour = 1;
    jeu.enGarde = false;
    jeu.actionChoisie = Action::Attaque;
    jeu.phase = Phase::Deplacement;
    jeu.banniere = jeu.nomDuLieu;
    jeu.tempsBanniere = 2.0f;
    ecrireJournal(jeu, "Combat " + std::to_string(jeu.combat + 1) + "/" + std::to_string(nombreDeCombats())
                       + " : " + jeu.nomDuLieu);
}

void combatSuivant(Jeu& jeu) {
    // Entre deux combats, AYLIS se repose (tous ses pv) et trouve une potion
    jeu.aylis.stats.soigner(jeu.aylis.stats.pvMax);
    jeu.aylis.stats.potions = jeu.aylis.stats.potions + 1;
    jeu.combat = jeu.combat + 1;
    preparerCombat(jeu);
    ecrireJournal(jeu, "AYLIS a recupere des forces et trouve une potion.");
}
