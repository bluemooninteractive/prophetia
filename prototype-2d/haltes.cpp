// haltes.cpp : les marchands et les rencontres de la route
//
// Les memes personnages que dans le jeu console : Maren l'herboriste attend dans la foret,
// Durgan le forgeron s'est installe pres du camp, et Silas le collectionneur rode sur le col.
#include "jeu2d.h"

// ===================== L'or =====================

void gagnerOr(Jeu& jeu, int pieces) {
    jeu.aylis.stats.pieces = jeu.aylis.stats.pieces + pieces;
}

// ===================== Les marchands =====================

std::string nomMarchand(int marchand) {
    switch (marchand) {
        case 0: return "Maren";
        case 1: return "Durgan";
        default: return "Silas";
    }
}

std::string titreMarchand(int marchand) {
    switch (marchand) {
        case 0: return "l'herboriste";
        case 1: return "le forgeron";
        default: return "le collectionneur";
    }
}

std::string paroleMarchand(int marchand) {
    switch (marchand) {
        case 0: return "Encore debout sur cette route ? Les Haschen ont brule mon jardin, mais pas mes potions !";
        case 1: return "Hmpf. Durgan, forgeron. Ton arme a besoin de mes mains, ca se voit d'ici.";
        default: return "Des runes, des secrets, des trophees... tout se vend. Tout s'achete.";
    }
}

// Une rune au hasard pour la boutique de Silas (numero = -1 s'il n'y en a plus)
Rune runeAuHasard(const Jeu& jeu, bool epique) {
    std::vector<Rune> possibles;
    for (const Rune& r : catalogueRunes()) {
        if (r.epique == epique && runeDisponible(jeu, r)) {
            possibles.push_back(r);
        }
    }
    if (possibles.empty()) {
        return {-1, "", "", false};
    }
    return possibles[GetRandomValue(0, (int)possibles.size() - 1)];
}

// Les articles de chaque marchand. Un numero >= 100 veut dire : "la rune numero - 100".
void ouvrirBoutique(Jeu& jeu) {
    jeu.marchand = jeu.lieu;
    jeu.articles.clear();
    if (jeu.marchand == 0) {
        jeu.articles.push_back({0, "Potion", "Rend 15 pv en combat", 20});
        jeu.articles.push_back({1, "Tisane de brume", "Rend la moitie des pv tout de suite", 20});
        jeu.articles.push_back({2, "Elixir d'esprit", "+6 mana max et un nouveau sort", 45});
    } else if (jeu.marchand == 1) {
        const Arme& arme = jeu.aylis.stats.arme;
        std::string nouvelle = "Epee longue";
        if (arme.nom.find("Baton") != std::string::npos) {
            nouvelle = "Baton runique";
        } else if (arme.aDistance) {
            nouvelle = "Arc long";
        }
        if (arme.nom != nouvelle) {
            jeu.articles.push_back({3, nouvelle, "+3 degats d'arme et +5% critique", 55});
        }
        jeu.articles.push_back({4, "Armure de cuir", "+2 defense", 40});
        jeu.articles.push_back({5, "Pierre a aiguiser", "+2 attaque", 45});
    } else {
        Rune rune = runeAuHasard(jeu, false);
        if (rune.numero >= 0) {
            jeu.articles.push_back({100 + rune.numero, "Rune " + rune.nom, rune.description, 50});
        }
        Rune epique = runeAuHasard(jeu, true);
        if (epique.numero >= 0) {
            jeu.articles.push_back({100 + epique.numero, "Rune " + epique.nom, epique.description + " (EPIQUE)", 110});
        }
        jeu.articles.push_back({0, "Potion", "Rend 15 pv en combat", 25});
    }
    jeu.messageRoute = "";
    jeu.phase = Phase::Marchand;
}

void acheter(Jeu& jeu, int numero) {
    if (numero < 0 || numero >= (int)jeu.articles.size()) {
        return;
    }
    Article& article = jeu.articles[numero];
    Combattant& aylis = jeu.aylis.stats;
    if (article.vendu) {
        jeu.messageRoute = "Deja vendu !";
        return;
    }
    if (aylis.pieces < article.prix) {
        jeu.messageRoute = "Pas assez d'or pour " + article.nom + ".";
        return;
    }
    aylis.pieces = aylis.pieces - article.prix;
    article.vendu = true;
    jeu.messageRoute = article.nom + " : achete !";

    if (article.numero >= 100) {
        appliquerRune(jeu, catalogueRunes()[article.numero - 100]);
        return;
    }
    switch (article.numero) {
        case 0:
            aylis.potions = aylis.potions + 1;
            article.vendu = false;      // les potions : on peut en racheter
            break;
        case 1: aylis.soigner(aylis.pvMax / 2); break;
        case 2:
            aylis.manaMax = aylis.manaMax + 6;
            aylis.mana = aylis.manaMax;
            if (aylis.sortsConnus < nombreDeSorts) {
                aylis.sortsConnus = aylis.sortsConnus + 1;
            }
            break;
        case 3:
            aylis.arme.nom = article.nom;
            aylis.arme.bonusAttaque = aylis.arme.bonusAttaque + 3;
            aylis.arme.chanceCritique = aylis.arme.chanceCritique + 5;
            break;
        case 4: aylis.defense = aylis.defense + 2; break;
        case 5: aylis.attaque = aylis.attaque + 2; break;
    }
}

// ===================== Les rencontres =====================

const int NOMBRE_RENCONTRES = 5;

std::string titreRencontre(int rencontre) {
    switch (rencontre) {
        case 0: return "Le voyageur blesse";
        case 1: return "Le coffre piege";
        case 2: return "L'autel mysterieux";
        case 3: return "Le deserteur haschen";
        default: return "Le puits aux souhaits";
    }
}

std::string texteRencontre(int rencontre) {
    switch (rencontre) {
        case 0: return "Un voyageur est adosse a un arbre, une fleche haschen plantee dans l'epaule. Il regarde AYLIS sans rien dire.";
        case 1: return "Un coffre cercle de fer, au milieu du chemin. Trop beau pour etre honnete... un fil brille pres de la serrure.";
        case 2: return "Un autel de pierre noire, couvert des memes runes que la prophetie. Une coupe vide attend une offrande.";
        case 3: return "Un jeune Haschen sans arme tremble derriere un rocher. Il a fui le camp d'Ashka.";
        default: return "Un vieux puits couvert de mousse. Au fond, des pieces d'or scintillent sous l'eau.";
    }
}

std::string reponseRencontre(const Jeu& jeu, int rencontre, int reponse) {
    const Combattant& aylis = jeu.aylis.stats;
    switch (rencontre) {
        case 0:
            if (reponse == 0) {
                return aylis.potions > 0 ? "Lui donner une potion" : "Rester a ses cotes (pas de potion)";
            }
            return "Fouiller ses affaires";
        case 1: return reponse == 0 ? "L'ouvrir prudemment" : "Forcer la serrure";
        case 2: return reponse == 0 ? "Offrir son sang (-10 pv)" : "Prier en silence";
        case 3: return reponse == 0 ? "L'epargner" : "Le depouiller";
        default:
            if (reponse == 0) {
                return aylis.pieces >= 20 ? "Jeter 20 pieces et faire un voeu" : "Faire un voeu (pas assez d'or)";
            }
            return "Boire son eau";
    }
}

// Un bon ou un mauvais choix : l'honneur decidera de la fin de l'histoire
void changerHonneur(Jeu& jeu, int changement) {
    jeu.aylis.stats.honneur = jeu.aylis.stats.honneur + changement;
}

// Une rencontre ne peut pas tuer AYLIS : il reste toujours au moins 1 pv
void blesserSansTuer(Jeu& jeu, int degats) {
    Combattant& aylis = jeu.aylis.stats;
    aylis.pv = aylis.pv > degats ? aylis.pv - degats : 1;
}

void commencerRencontre(Jeu& jeu) {
    // Une rencontre pas encore vue pendant cette course
    std::vector<int> possibles;
    for (int i = 0; i < NOMBRE_RENCONTRES; i++) {
        bool vue = false;
        for (int deja : jeu.rencontresVues) {
            vue = vue || deja == i;
        }
        if (!vue) {
            possibles.push_back(i);
        }
    }
    if (possibles.empty()) {
        jeu.rencontresVues.clear();
        possibles = {0, 1, 2, 3, 4};
    }
    jeu.rencontre = possibles[GetRandomValue(0, (int)possibles.size() - 1)];
    jeu.rencontresVues.push_back(jeu.rencontre);
    jeu.resultatRencontre = "";
    jeu.runeOfferte = false;
    jeu.messageRoute = "";
    jeu.phase = Phase::Rencontre;
}

void repondreRencontre(Jeu& jeu, int reponse) {
    if (!jeu.resultatRencontre.empty() || reponse < 0 || reponse > 1) {
        return;     // deja repondu
    }
    Combattant& aylis = jeu.aylis.stats;
    std::string& resultat = jeu.resultatRencontre;

    if (jeu.rencontre == 0) {
        if (reponse == 0) {
            changerHonneur(jeu, 1);
            jeu.memoire.voyageurAide = jeu.memoire.voyageurAide + 1;     // le Seuil s'en souviendra
            if (aylis.potions > 0) {
                aylis.potions = aylis.potions - 1;
                resultat = "Le voyageur reprend des couleurs. Il tend une pierre gravee : \"Elle t'attendait, je crois.\"";
                jeu.runeOfferte = true;
            } else {
                resultat = "AYLIS reste jusqu'a ce que la fievre tombe. Le voyageur s'en souviendra.";
            }
        } else {
            changerHonneur(jeu, -1);
            jeu.memoire.voyageurDepouille = jeu.memoire.voyageurDepouille + 1;
            gagnerOr(jeu, 35);
            resultat = "35 pieces d'or... et un regard qu'AYLIS n'oubliera pas de sitot.";
        }
    } else if (jeu.rencontre == 1) {
        if (reponse == 0) {
            gagnerOr(jeu, 20);
            resultat = "Le fil est coupe a temps : 20 pieces d'or.";
            if (GetRandomValue(1, 4) == 1) {
                blesserSansTuer(jeu, 5);
                resultat = "Une aiguille jaillit (-5 pv)... mais 20 pieces d'or dorment au fond.";
            }
        } else if (GetRandomValue(0, 1) == 0) {
            gagnerOr(jeu, 60);
            resultat = "La serrure cede dans un craquement : 60 pieces d'or !";
        } else {
            blesserSansTuer(jeu, 12);
            resultat = "BOUM ! Le piege explose (-12 pv). Le coffre etait vide.";
        }
    } else if (jeu.rencontre == 2) {
        if (reponse == 0) {
            blesserSansTuer(jeu, 10);
            aylis.attaque = aylis.attaque + 2;
            resultat = "Le sang coule dans la coupe. Les runes s'allument : +2 attaque.";
        } else {
            aylis.soigner(15);
            resultat = "Une chaleur douce enveloppe AYLIS : +15 pv.";
        }
    } else if (jeu.rencontre == 3) {
        if (reponse == 0) {
            changerHonneur(jeu, 1);
            jeu.memoire.deserteurEpargne = jeu.memoire.deserteurEpargne + 1;
            gagnerOr(jeu, 15);
            resultat = "Il glisse 15 pieces dans la main d'AYLIS : \"Ashka a peur de la prophetie... elle a peur de toi.\"";
        } else {
            changerHonneur(jeu, -1);
            jeu.memoire.deserteurDepouille = jeu.memoire.deserteurDepouille + 1;
            gagnerOr(jeu, 30);
            resultat = "30 pieces d'or. Le Haschen s'enfuit en pleurant vers le camp.";
        }
    } else {
        if (reponse == 0) {
            if (aylis.pieces >= 20) {
                aylis.pieces = aylis.pieces - 20;
                resultat = "Les pieces tombent en tintant... et l'eau se met a briller d'une lueur violette.";
                jeu.runeOfferte = true;
            } else {
                resultat = "Sans offrande, le puits reste silencieux.";
            }
        } else if (GetRandomValue(0, 1) == 0) {
            aylis.soigner(12);
            resultat = "Une eau glacee et pure : +12 pv.";
        } else {
            blesserSansTuer(jeu, 5);
            resultat = "L'eau est croupie ! (-5 pv)";
        }
    }
}

void finirRencontre(Jeu& jeu) {
    if (jeu.resultatRencontre.empty()) {
        return;
    }
    if (jeu.runeOfferte) {
        jeu.messageRoute = "";
        proposerRunes(jeu, false);
    } else {
        allerPlusLoin(jeu);
    }
}
