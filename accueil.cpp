// accueil.cpp : l'ecran titre et les regles du jeu
#include <iostream>
#include <string>
#include "accueil.h"
#include "outils.h"
#include "couleurs.h"

int ecranTitre(bool sauvegardeDisponible) {
    // Le titre en grandes lettres. R"TITRE( ... )TITRE" est une "chaine brute" :
    // on peut y ecrire des \ et des " sans rien echapper, et les retours a la ligne sont gardes.
    const std::string titre = R"TITRE(
 __     __ _____  ____   ____   _____  ____      _     _   _   ____  _____
 \ \   / /| ____|/ ___| |  _ \ | ____||  _ \    / \   | \ | | / ___|| ____|
  \ \ / / |  _|  \___ \ | |_) ||  _|  | |_) |  / _ \  |  \| || |    |  _|
   \ V /  | |___  ___) ||  __/ | |___ |  _ <  / ___ \ | |\  || |___ | |___
    \_/   |_____||____/ |_|    |_____||_| \_\/_/   \_\|_| \_| \____||_____|
)TITRE";

    std::cout << "\n\n" << colorer(titre, VIOLET + GRAS) << "\n";
    std::cout << "                         " << colorer("AYLIS contre les Haschen", JAUNE + GRAS) << "\n";
    std::cout << "                   " << colorer("la route vers Vorgath le Destructeur", GRIS) << "\n\n";

    std::cout << "  1. Nouvelle partie";
    if (sauvegardeDisponible) {
        std::cout << colorer("   (remplace la partie sauvegardee)", GRIS);
    }
    std::cout << "\n";
    if (sauvegardeDisponible) {
        std::cout << "  2. " << colorer("Continuer la partie", VERT + GRAS) << "\n";
    } else {
        std::cout << "  2. " << colorer("Continuer la partie (aucune sauvegarde)", GRIS) << "\n";
    }
    std::cout << "  3. " << colorer("Defi du jour", JAUNE) << colorer("   (la meme route pour tout le monde aujourd'hui)", GRIS) << "\n";
    std::cout << "  4. Regles du jeu\n";
    std::cout << "  5. Quitter\n\n";

    while (true) {
        int choix = lireChoix(1, 5);
        if (choix == 2 && !sauvegardeDisponible) {
            std::cout << "Il n'y a pas de partie sauvegardee. Commence une nouvelle partie !\n";
            continue;
        }
        return choix;
    }
}

// Un titre de page des regles
void titreRegles(const std::string& texte) {
    std::cout << "\n" << colorer("=== " + texte + " ===", JAUNE + GRAS) << "\n\n";
}

void afficherRegles() {
    titreRegles("REGLES 1/5 : LE BUT DU JEU");
    std::cout << "Les Haschen envahissent la vallee. AYLIS doit traverser la route jusqu'a la forteresse\n";
    std::cout << "de " << colorer("Vorgath le Destructeur", ROUGE + GRAS) << " et l'abattre.\n\n";
    std::cout << "Au depart, choisis ta voie : " << colorer("l'epee", JAUNE) << " (solide), " << colorer("l'arc", VERT)
              << " (a distance) ou " << colorer("les arcanes", VIOLET) << " (sorts).\n\n";
    std::cout << "La route compte 11 etapes. A la plupart, tu choisis ton chemin parmi trois :\n";
    std::cout << "  - un combat (parfois contre une " << colorer("MEUTE", JAUNE) << " de deux Haschen)\n";
    std::cout << "  - une " << colorer("ELITE", JAUNE) << " : plus dangereuse, mais avec un objet RARE garanti\n";
    std::cout << "  - un repos, une halte chez les marchands, ou un sentier " << colorer("???", VIOLET) << " plein de surprises\n";
    std::cout << "Trois boss barrent la route : " << colorer("Ashka, la Matriarche", ROUGE) << ", "
              << colorer("Skarn, le Tisseur d'ombres", VIOLET) << ", puis Vorgath.\n";
    std::cout << "Une halte est toujours prevue avant chaque boss.\n";
    std::cout << "Le " << colorer("Defi du jour", JAUNE) << " propose la meme route a tout le monde, le meme jour.\n";
    attendreEntree();

    titreRegles("REGLES 2/5 : LE COMBAT");
    std::cout << "Chacun joue a son tour. A chaque tour, tu choisis une action :\n";
    std::cout << "  1. Attaque normale : ne rate jamais\n";
    std::cout << "  2. Attaque lourde  : degats x1.8, mais rate 4 fois sur 10\n";
    std::cout << "  3. Attaque en garde : petits degats, mais tu encaisses 2 fois moins\n";
    std::cout << "  4. Potion : +15 pv\n";
    std::cout << "  5. Sort : Boule de feu, puis Soin, Eclair et Bouclier quand tu les apprends (ils coutent du mana)\n";
    std::cout << "  6. Attaque speciale : quand ta jauge de " << colorer("rage", ROUGE) << " est pleine (elle monte quand tu prends des coups)\n";
    std::cout << "  7. Avancer vers les ennemis\n\n";
    std::cout << "Face a plusieurs ennemis, tu choisis ta cible.\n";
    attendreEntree();

    titreRegles("REGLES 3/5 : LA DISTANCE ET LES ARMES");
    std::cout << "Chaque combat commence de " << colorer("LOIN", CYAN) << ". Les Haschen s'approchent a chaque tour.\n\n";
    std::cout << "  - Arme de " << colorer("MELEE", JAUNE) << " (epee, hache...) : il faut etre " << colorer("AU CONTACT", CYAN)
              << " pour frapper.\n";
    std::cout << "    -> Au debut du combat, tape " << colorer("7", JAUNE + GRAS) << " pour avancer (ou lance un sort).\n";
    std::cout << "  - Arme a " << colorer("DISTANCE", JAUNE) << " (arc, arbalete...) : tu tires pendant qu'ils approchent,\n";
    std::cout << "    mais tu es moins efficace une fois au contact.\n\n";
    std::cout << "Attention : certains Haschen lancent des projectiles de loin, et Vorgath CHARGE.\n";
    attendreEntree();

    titreRegles("REGLES 4/5 : PROGRESSER");
    std::cout << "  - Chaque victoire donne de l'" << colorer("XP", CYAN) << ". A chaque niveau, tu gagnes 2 points\n";
    std::cout << "    a depenser : plus de vie, plus de degats, ou un nouveau sort.\n";
    std::cout << "  - Les ennemis laissent de l'or et des objets : commun, " << colorer("RARE", BLEU + GRAS)
              << " ou " << colorer("EPIQUE", VIOLET + GRAS) << ".\n";
    std::cout << "  - Aux haltes : " << colorer("Maren", CYAN + GRAS) << " vend des potions, "
              << colorer("Durgan", CYAN + GRAS) << " des armes et des armures,\n";
    std::cout << "    et " << colorer("Silas", CYAN + GRAS) << " rachete ton butin (plus cher pour les objets rares).\n";
    std::cout << "  - Parle aux marchands (\"Discuter\") : ils donnent des conseils sur les combats a venir !\n";
    attendreEntree();

    titreRegles("REGLES 5/5 : ETATS, COMPAGNONS ET CHOIX");
    std::cout << "  - Les " << colorer("etats", JAUNE) << " font effet au debut de chaque tour :\n";
    std::cout << "      " << colorer("POISON", VERT) << " (les chamans)  -3 pv par tour\n";
    std::cout << "      " << colorer("BRULURE", ROUGE) << " (la Boule de feu)  -4 pv par tour\n";
    std::cout << "      " << colorer("SAIGNE", ROUGE) << " (les armes qui frappent 2 fois)  -3 pv par tour\n";
    std::cout << "      " << colorer("BOUCLIER", CYAN) << " (le sort)  absorbe 15 points de degats\n";
    std::cout << "    Le sort Soin guerit AYLIS de tous ces etats.\n";
    std::cout << "  - Certaines rencontres peuvent te donner un " << colorer("compagnon", CYAN + GRAS)
              << " qui combat avec toi.\n";
    std::cout << "  - Tes " << colorer("choix", JAUNE) << " comptent : aider ou depouiller, epargner ou achever...\n";
    std::cout << "    Ils decident de la fin de l'histoire. Il y en a trois.\n";
    std::cout << "  - La partie est " << colorer("sauvegardee", VERT) << " a chaque etape. Tu peux quitter et la\n";
    std::cout << "    reprendre plus tard avec \"Continuer la partie\".\n";
    attendreEntree();
}
