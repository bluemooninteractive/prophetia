// route.cpp : la route d'une "course" (un essai d'AYLIS, du depart jusqu'a Ashka)
//
// Les lieux restent toujours dans l'ordre de l'histoire (lieux.cpp) : la foret, le village, le gue, le bois,
// le camp, la forteresse, puis le col ou Ashka attend.
// Ce qui change a chaque essai : les salles que la vision propose, les Haschen rencontres,
// et les runes de prophetie qu'AYLIS ramasse en chemin.
#include "jeu2d.h"

// ===================== Les noms =====================

std::string nomTypeSalle(TypeSalle type) {
    switch (type) {
        case TypeSalle::Combat: return "Combat";
        case TypeSalle::Elite: return "Elite";
        case TypeSalle::Repos: return "Feu de camp";
        case TypeSalle::Oracle: return "Oracle";
        case TypeSalle::Marchand: return "Marchand";
        case TypeSalle::Rencontre: return "???";
        case TypeSalle::Boss: return "Ashka";
    }
    return "";
}

std::string descriptionSalle(TypeSalle type) {
    switch (type) {
        case TypeSalle::Combat: return "Quelques Haschen. Une rune a la fin.";
        case TypeSalle::Elite: return "Un Haschen d'elite. Dur... mais une rune EPIQUE.";
        case TypeSalle::Repos: return "AYLIS se repose : +60% pv et une potion.";
        case TypeSalle::Oracle: return "Une rune sans combat. La vision coute 6 pv.";
        case TypeSalle::Marchand: return "De quoi depenser ton or.";
        case TypeSalle::Rencontre: return "Une surprise... bonne ou mauvaise ?";
        case TypeSalle::Boss: return "La cheffe de guerre des Haschen t'attend.";
    }
    return "";
}

// Le lieu d'une salle : un lieu par salle, dans l'ordre de la route. Ashka attend au col (la derniere salle).
int lieuDeLaSalle(int salle) {
    int lieu = salle - 1;
    return lieu > NOMBRE_LIEUX - 1 ? NOMBRE_LIEUX - 1 : lieu;
}

// ===================== Les runes de prophetie =====================

// Tout le catalogue. Les runes "uniques" ne peuvent etre prises qu'une fois.
const std::vector<Rune>& catalogueRunes() {
    static const std::vector<Rune> runes = {
        {0, "Vigueur", "+8 pv max", false},
        {1, "Tranchant", "+2 attaque", false},
        {2, "Esprit", "+1 sort connu et +4 mana", false},
        {3, "Pierre", "+2 defense", false},
        {4, "Vent", "+1 case de deplacement", false},
        {5, "Flamme", "Tes coups ont 1 chance sur 3 de bruler", false},
        {6, "Seve", "+5 pv a chaque Haschen abattu", false},
        {7, "Fureur", "La rage monte 2 fois plus vite", false},
        {8, "Oeil", "+1 portee a distance et +10% critique", false},
        {9, "Colosse", "+20 pv max et +2 defense", true},
        {10, "Lame prophetique", "+4 attaque et +10% critique", true},
        {11, "Ailes", "+2 cases de deplacement", true},
    };
    return runes;
}

bool possedeRune(const Jeu& jeu, int numero) {
    for (const Rune& r : jeu.runes) {
        if (r.numero == numero) {
            return true;
        }
    }
    return false;
}

// Une rune peut-elle encore etre proposee ?
bool runeDisponible(const Jeu& jeu, const Rune& rune) {
    bool unique = rune.epique || rune.numero >= 5;      // Flamme, Seve, Fureur, Oeil et les epiques
    if (unique && possedeRune(jeu, rune.numero)) {
        return false;
    }
    if (rune.numero == 2 && jeu.aylis.stats.sortsConnus >= nombreDeSorts && jeu.aylis.stats.manaMax >= 40) {
        return false;   // Esprit ne sert plus a rien
    }
    return true;
}

// Tire 3 runes differentes au hasard (dont une epique si "epique" est vrai)
void proposerRunes(Jeu& jeu, bool epique) {
    std::vector<Rune> normales;
    std::vector<Rune> epiques;
    for (const Rune& r : catalogueRunes()) {
        if (runeDisponible(jeu, r)) {
            if (r.epique) {
                epiques.push_back(r);
            } else {
                normales.push_back(r);
            }
        }
    }
    jeu.runesProposees.clear();
    if (epique && !epiques.empty()) {
        jeu.runesProposees.push_back(epiques[GetRandomValue(0, (int)epiques.size() - 1)]);
    }
    while (jeu.runesProposees.size() < 3 && !normales.empty()) {
        int tirage = GetRandomValue(0, (int)normales.size() - 1);
        jeu.runesProposees.push_back(normales[tirage]);
        normales.erase(normales.begin() + tirage);     // pour ne pas la tirer deux fois
    }
    jeu.phase = Phase::ChoixRune;
}

void appliquerRune(Jeu& jeu, const Rune& rune) {
    Combattant& aylis = jeu.aylis.stats;
    switch (rune.numero) {
        case 0: aylis.pvMax = aylis.pvMax + 8; aylis.soigner(8); break;
        case 1: aylis.attaque = aylis.attaque + 2; break;
        case 2:
            if (aylis.sortsConnus < nombreDeSorts) {
                aylis.sortsConnus = aylis.sortsConnus + 1;
            }
            aylis.manaMax = aylis.manaMax + 4;
            aylis.mana = aylis.manaMax;
            break;
        case 3: aylis.defense = aylis.defense + 2; break;
        case 4: jeu.deplacement = jeu.deplacement + 1; break;
        case 5: jeu.runeFlamme = true; break;
        case 6: jeu.runeSeve = true; break;
        case 7: jeu.runeFureur = true; break;
        case 8: jeu.porteeBonus = jeu.porteeBonus + 1; aylis.arme.chanceCritique = aylis.arme.chanceCritique + 10; break;
        case 9: aylis.pvMax = aylis.pvMax + 20; aylis.soigner(20); aylis.defense = aylis.defense + 2; break;
        case 10: aylis.attaque = aylis.attaque + 4; aylis.arme.chanceCritique = aylis.arme.chanceCritique + 10; break;
        case 11: jeu.deplacement = jeu.deplacement + 2; break;
    }
    jeu.runes.push_back(rune);
}

// ===================== Les salles =====================

// La vision montre les salles possibles pour la salle "jeu.salle"
void proposerSalles(Jeu& jeu) {
    jeu.propositions.clear();
    int lieu = lieuDeLaSalle(jeu.salle);
    jeu.lieu = lieu;
    jeu.phase = Phase::ChoixSalle;

    if (jeu.salle >= NOMBRE_SALLES) {
        jeu.propositions.push_back({TypeSalle::Boss, lieu});
        return;
    }

    // Juste avant Ashka, la vision montre toujours un feu de camp
    if (jeu.salle == NOMBRE_SALLES - 1) {
        jeu.propositions.push_back({TypeSalle::Repos, lieu});
    }
    int nombre = GetRandomValue(2, 3);
    int essais = 0;
    while ((int)jeu.propositions.size() < nombre && essais < 50) {
        essais = essais + 1;
        // Un tirage "pondere" : sur 100, 38 chances pour un combat, 16 pour une elite...
        int tirage = GetRandomValue(1, 100);
        TypeSalle type = TypeSalle::Combat;
        if (tirage > 89) {
            type = TypeSalle::Rencontre;
        } else if (tirage > 78) {
            type = TypeSalle::Marchand;
        } else if (tirage > 66) {
            type = TypeSalle::Oracle;
        } else if (tirage > 54) {
            type = TypeSalle::Repos;
        } else if (tirage > 38) {
            type = TypeSalle::Elite;
        }
        // Pas d'elite ni de repos tout au debut
        if (jeu.salle <= 2 && (type == TypeSalle::Elite || type == TypeSalle::Repos)) {
            continue;
        }
        bool dejaLa = false;
        for (const Salle& s : jeu.propositions) {
            dejaLa = dejaLa || s.type == type;
        }
        if (!dejaLa) {
            jeu.propositions.push_back({type, lieu});
        }
    }
    // Au moins un combat (ou une elite) : il faut bien gagner des runes !
    bool unCombat = false;
    for (const Salle& s : jeu.propositions) {
        unCombat = unCombat || s.type == TypeSalle::Combat || s.type == TypeSalle::Elite;
    }
    if (!unCombat) {
        jeu.propositions.back() = {TypeSalle::Combat, lieu};
    }
}

// La salle est finie : on passe a la suivante
void allerPlusLoin(Jeu& jeu) {
    jeu.salle = jeu.salle + 1;
    proposerSalles(jeu);
}

void choisirSalle(Jeu& jeu, int numero) {
    if (numero < 0 || numero >= (int)jeu.propositions.size()) {
        return;
    }
    Salle salle = jeu.propositions[numero];
    jeu.typeSalle = salle.type;
    jeu.lieu = salle.lieu;
    Combattant& aylis = jeu.aylis.stats;

    if (salle.type == TypeSalle::Repos) {
        int soin = aylis.pvMax * 60 / 100;
        aylis.soigner(soin);
        aylis.potions = aylis.potions + 1;
        jeu.messageRoute = "AYLIS se repose au coin du feu : +" + std::to_string(soin) + " pv et une potion.";
        allerPlusLoin(jeu);
    } else if (salle.type == TypeSalle::Oracle) {
        aylis.pv = aylis.pv > 7 ? aylis.pv - 6 : 1;
        jeu.messageRoute = "La vision brule les yeux d'AYLIS (-6 pv)... et montre trois runes.";
        proposerRunes(jeu, GetRandomValue(1, 100) <= 35);
    } else if (salle.type == TypeSalle::Marchand) {
        ouvrirBoutique(jeu);
    } else if (salle.type == TypeSalle::Rencontre) {
        commencerRencontre(jeu);
    } else {
        jeu.messageRoute = "";
        preparerCombat(jeu);
    }
}

void apresCombat(Jeu& jeu) {
    // AYLIS reprend son souffle (un quart de ses pv), puis choisit sa recompense
    if (jeu.typeSalle == TypeSalle::Elite) {
        jeu.elitesVaincues = jeu.elitesVaincues + 1;
    }
    int soin = jeu.aylis.stats.pvMax / 4;
    jeu.aylis.stats.soigner(soin);
    jeu.messageRoute = "AYLIS reprend son souffle : +" + std::to_string(soin) + " pv.";
    proposerRunes(jeu, jeu.typeSalle == TypeSalle::Elite);
}

void choisirRune(Jeu& jeu, int numero) {
    if (numero < 0 || numero >= (int)jeu.runesProposees.size()) {
        return;
    }
    Rune rune = jeu.runesProposees[numero];
    appliquerRune(jeu, rune);
    jeu.messageRoute = "Rune " + rune.nom + " : " + rune.description + ".";
    allerPlusLoin(jeu);
}

// ===================== Le depart =====================

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
    aylis.pieces = 15;
    jeu.aylis.stats = aylis;
    jeu.aylis.couleur = SKYBLUE;
    jeu.rencontresVues.clear();
    jeu.elitesVaincues = 0;
    jeu.fragmentsGagnes = 0;
    jeu.derniereBlessure = "";
    jeu.fondu = 0.0f;
    jeu.rage.vider();

    // Une nouvelle course : aucune rune, et la premiere salle est toujours un combat en foret
    jeu.runes.clear();
    jeu.deplacement = DEPLACEMENT_AYLIS;
    jeu.porteeBonus = 0;
    jeu.runeFlamme = false;
    jeu.runeSeve = false;
    jeu.runeFureur = false;
    jeu.salle = 1;
    jeu.lieu = 0;
    jeu.typeSalle = TypeSalle::Combat;
    jeu.messageRoute = "";
    appliquerAmeliorations(jeu);    // ce qu'AYLIS a gagne au Seuil
    preparerCombat(jeu);
    if (!jeu.messageRoute.empty()) {
        ecrireJournal(jeu, jeu.messageRoute);     // par exemple : la rune d'eveil
    }
}

// ===================== Preparer un combat =====================

// Une case libre au hasard, du cote des Haschen (a droite de l'arene), et qu'AYLIS peut atteindre
void placerHaschen(Jeu& jeu, const std::vector<bool>& accessibles, Combattant stats, Color couleur) {
    for (int essai = 0; essai < 200; essai++) {
        int c = GetRandomValue(essai < 100 ? 8 : 5, COLONNES - 1);     // si la droite est trop pleine : un peu plus a gauche
        int l = GetRandomValue(0, LIGNES - 1);
        if (!estRocher(jeu, c, l) && accessibles[l * COLONNES + c] && haschenSurCase(jeu, c, l) == -1) {
            jeu.haschen.push_back({stats, c, l, couleur});
            return;
        }
    }
}

void preparerCombat(Jeu& jeu) {
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

    // La carte du lieu : le terrain, les obstacles, les lumieres (lieux.cpp)
    chargerCarte(jeu);
    std::vector<bool> accessibles = casesAccessibles(jeu);

    // Les memes Haschen que dans le jeu console (sans le butin, pour le prototype)
    //                        nom                   pv  pvMax att def potions boss   xp  or
    Combattant eclaireur  = {"Haschen eclaireur",   18, 18,   8,  1, 0,      false, 20, 20};
    Combattant guerrier   = {"Haschen guerrier",    22, 22,   9,  2, 0,      false, 25, 25};
    Combattant traqueur   = {"Haschen traqueur",    20, 20,   9,  1, 0,      false, 25, 22};
    Combattant chaman     = {"Haschen chaman",      20, 20,  10,  1, 0,      false, 25, 25};
    Combattant louvetier  = {"Haschen louvetier",   22, 22,  10,  2, 0,      false, 25, 25};
    Combattant ashka      = {"Ashka",               55, 55,  15,  4, 1,      true,  50, 50};
    traqueur.style = Style::Lanceur;
    chaman.attaquePoison = true;
    louvetier.style = Style::Chargeur;
    ashka.style = Style::Lanceur;

    // Plus AYLIS avance, plus les Haschen sont coriaces
    int bonus = jeu.salle - 1;
    for (Combattant* h : {&eclaireur, &guerrier, &traqueur, &chaman, &louvetier}) {
        h->pvMax = h->pvMax + bonus * 2;
        h->pv = h->pvMax;
        h->attaque = h->attaque + bonus / 3;
    }

    // Les Haschen de chaque lieu, et leur couleur
    std::pair<Combattant, Color> E = {eclaireur, ORANGE};
    std::pair<Combattant, Color> G = {guerrier, RED};
    std::pair<Combattant, Color> T = {traqueur, GREEN};
    std::pair<Combattant, Color> C = {chaman, PURPLE};
    std::pair<Combattant, Color> L = {louvetier, BROWN};
    const std::vector<std::pair<Combattant, Color>> groupes[NOMBRE_LIEUX] = {
        {E, G, T},          // la foret : les eclaireurs d'Ashka
        {E, G, L},          // le village : des pillards
        {T, E, C},          // le gue : des tireurs caches dans les roseaux
        {C, L, T},          // le bois des Pendus : chamans et louvetiers
        {G, L, C, T},       // le camp : tout le monde
        {G, T, C, L},       // la forteresse
        {T, L, G, C},       // le col
    };
    const std::vector<std::pair<Combattant, Color>>& groupe = groupes[jeu.lieu];

    jeu.nomDuLieu = nomLieu(jeu.lieu);
    if (jeu.typeSalle == TypeSalle::Boss) {
        placerHaschen(jeu, accessibles, ashka, GOLD);
        placerHaschen(jeu, accessibles, traqueur, GREEN);
        placerHaschen(jeu, accessibles, eclaireur, ORANGE);
    } else if (jeu.typeSalle == TypeSalle::Elite) {
        // Un Haschen d'elite (un guerrier au debut de la route, un louvetier ensuite), et deux compagnons
        Combattant elite = jeu.lieu <= 1 || jeu.lieu == 5 ? guerrier : louvetier;
        elite.nom = elite.nom + " d'elite";
        elite.pvMax = elite.pvMax * 17 / 10;
        elite.pv = elite.pvMax;
        elite.attaque = elite.attaque + 2;
        elite.defense = elite.defense + 1;
        elite.orDonne = elite.orDonne * 3;
        placerHaschen(jeu, accessibles, elite, GOLD);
        for (int i = 0; i < 2; i++) {
            const auto& h = groupe[GetRandomValue(0, (int)groupe.size() - 1)];
            placerHaschen(jeu, accessibles, h.first, h.second);
        }
    } else {
        int nombre = jeu.salle == 1 ? 2 : (jeu.salle >= 6 ? 4 : 3);
        for (int i = 0; i < nombre; i++) {
            const auto& h = groupe[GetRandomValue(0, (int)groupe.size() - 1)];
            placerHaschen(jeu, accessibles, h.first, h.second);
        }
    }

    jeu.tour = 1;
    jeu.enGarde = false;
    jeu.actionChoisie = Action::Attaque;
    jeu.phase = Phase::Deplacement;
    jeu.banniere = jeu.typeSalle == TypeSalle::Boss ? "ASHKA" : jeu.nomDuLieu;
    jeu.tempsBanniere = 2.0f;
    ecrireJournal(jeu, "Salle " + std::to_string(jeu.salle) + "/" + std::to_string(NOMBRE_SALLES) + " : "
                       + nomTypeSalle(jeu.typeSalle) + " - " + jeu.nomDuLieu);
}
