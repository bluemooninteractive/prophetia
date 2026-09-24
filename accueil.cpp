// accueil.cpp : l'ecran titre et les regles du jeu
#include <iostream>
#include <string>
#include "accueil.h"
#include "outils.h"
#include "couleurs.h"

int ecranTitre() {
    // Le titre en grandes lettres. R"TITRE( ... )TITRE" est une "chaine brute" :
    // on peut y ecrire des \ et des " sans rien echapper, et les retours a la ligne sont gardes.
    const std::string titre = R"TITRE(
  ____  ____   ___  ____  _   _ _____ _____ ___    _
 |  _ \|  _ \ / _ \|  _ \| | | | ____|_   _|_ _|  / \
 | |_) | |_) | | | | |_) | |_| |  _|   | |  | |  / _ \
 |  __/|  _ <| |_| |  __/|  _  | |___  | |  | | / ___ \
 |_|   |_| \_\\___/|_|   |_| |_|_____| |_| |___/_/   \_\
)TITRE";

    std::cout << "\n\n" << colorer(titre, VIOLET + GRAS) << "\n";
    std::cout << "           " << colorer("AYLIS contre les Haschen", JAUNE + GRAS) << "\n";
    std::cout << "        " << colorer("la route vers Vorgath le Destructeur", GRIS) << "\n\n";

    std::cout << "  1. Nouvelle partie\n";
    std::cout << "  2. Regles du jeu\n";
    std::cout << "  3. Quitter\n\n";

    return lireChoix(1, 3);
}

// Un titre de page des regles
void titreRegles(const std::string& texte) {
    std::cout << "\n" << colorer("=== " + texte + " ===", JAUNE + GRAS) << "\n\n";
}

void afficherRegles() {
    titreRegles("REGLES 1/4 : LE BUT DU JEU");
    std::cout << "Les Haschen envahissent la vallee. AYLIS doit traverser la route jusqu'a la forteresse\n";
    std::cout << "de " << colorer("Vorgath le Destructeur", ROUGE + GRAS) << " et l'abattre.\n\n";
    std::cout << "La route compte 8 etapes. A la plupart, tu choisis ton chemin parmi trois :\n";
    std::cout << "  - un combat (parfois contre une " << colorer("MEUTE", JAUNE) << " de deux Haschen)\n";
    std::cout << "  - une " << colorer("ELITE", JAUNE) << " : plus dangereuse, mais avec un objet RARE garanti\n";
    std::cout << "  - un repos, une halte chez les marchands, ou un sentier " << colorer("???", VIOLET) << " plein de surprises\n";
    std::cout << "A mi-chemin, " << colorer("Ashka, la Matriarche", ROUGE) << " t'attend. Une halte est toujours prevue avant chaque boss.\n";
    attendreEntree();

    titreRegles("REGLES 2/4 : LE COMBAT");
    std::cout << "Chacun joue a son tour. A chaque tour, tu choisis une action :\n";
    std::cout << "  1. Attaque normale : ne rate jamais\n";
    std::cout << "  2. Attaque lourde  : degats x1.8, mais rate 4 fois sur 10\n";
    std::cout << "  3. Attaque en garde : petits degats, mais tu encaisses 2 fois moins\n";
    std::cout << "  4. Potion : +15 pv\n";
    std::cout << "  5. Sort : Boule de feu, puis Soin et Eclair quand tu les apprends (ils coutent du mana)\n";
    std::cout << "  6. Attaque speciale : quand ta jauge de " << colorer("rage", ROUGE) << " est pleine (elle monte quand tu prends des coups)\n";
    std::cout << "  7. Avancer vers les ennemis\n\n";
    std::cout << "Face a plusieurs ennemis, tu choisis ta cible.\n";
    attendreEntree();

    titreRegles("REGLES 3/4 : LA DISTANCE ET LES ARMES");
    std::cout << "Chaque combat commence de " << colorer("LOIN", CYAN) << ". Les Haschen s'approchent a chaque tour.\n\n";
    std::cout << "  - Arme de " << colorer("MELEE", JAUNE) << " (epee, hache...) : il faut etre " << colorer("AU CONTACT", CYAN)
              << " pour frapper.\n";
    std::cout << "    -> Au debut du combat, tape " << colorer("7", JAUNE + GRAS) << " pour avancer (ou lance un sort).\n";
    std::cout << "  - Arme a " << colorer("DISTANCE", JAUNE) << " (arc, arbalete...) : tu tires pendant qu'ils approchent,\n";
    std::cout << "    mais tu es moins efficace une fois au contact.\n\n";
    std::cout << "Attention : certains Haschen lancent des projectiles de loin, et Vorgath CHARGE.\n";
    attendreEntree();

    titreRegles("REGLES 4/4 : PROGRESSER");
    std::cout << "  - Chaque victoire donne de l'" << colorer("XP", CYAN) << ". A chaque niveau, tu gagnes 2 points\n";
    std::cout << "    a depenser : plus de vie, plus de degats, ou un nouveau sort.\n";
    std::cout << "  - Les ennemis laissent de l'or et des objets : commun, " << colorer("RARE", BLEU + GRAS)
              << " ou " << colorer("EPIQUE", VIOLET + GRAS) << ".\n";
    std::cout << "  - Aux haltes : " << colorer("Maren", CYAN + GRAS) << " vend des potions, "
              << colorer("Durgan", CYAN + GRAS) << " des armes et des armures,\n";
    std::cout << "    et " << colorer("Silas", CYAN + GRAS) << " rachete ton butin (plus cher pour les objets rares).\n";
    std::cout << "  - Parle aux marchands (\"Discuter\") : ils donnent des conseils sur les combats a venir !\n";
    attendreEntree();
}
