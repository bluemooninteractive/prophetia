// histoire.cpp : l'introduction et les differentes fins de l'aventure
#include <iostream>
#include <string>
#include "histoire.h"
#include "outils.h"
#include "couleurs.h"

void afficherIntro() {
    std::cout << "\n" << colorer("=== PROLOGUE ===", JAUNE + GRAS) << "\n\n";
    std::cout << "Autrefois, les Haschen vivaient en paix dans les collines de la vallee.\n";
    std::cout << "Puis " << colorer("Vorgath", ROUGE + GRAS) << " est arrive. Il a rassemble les clans, les a nourris de sa rage,\n";
    std::cout << "et les a lances contre les villages. Dans l'ombre, son sorcier " << colorer("Skarn", VIOLET + GRAS) << "\n";
    std::cout << "souffle cette rage dans l'esprit de chaque Haschen.\n\n";
    std::cout << "Les soldats du roi ont fui. Les portes des villes sont fermees.\n";
    std::cout << "Sur la route qui mene a la forteresse de Vorgath, il ne reste qu'une silhouette :\n";
    std::cout << colorer("AYLIS", JAUNE + GRAS) << ".\n\n";
    std::cout << colorer("Tes choix sur la route compteront. Les Haschen ne sont pas tous des monstres...", GRIS) << "\n";
    attendreEntree();
}

void afficherFin(const EtatPartie& etat) {
    const Combattant& aylis = etat.aylis;

    std::cout << "\n" << colorer("=== EPILOGUE ===", JAUNE + GRAS) << "\n\n";
    std::cout << "Vorgath s'effondre dans la poussiere de sa forteresse. Sa rage s'eteint avec lui.\n";
    if (aylis.voie != "") {
        std::cout << "On se souviendra d'AYLIS, qui a suivi la voie " << aylis.voie << " jusqu'au bout.\n";
    }
    std::cout << "\n";

    // La fin depend de l'honneur : les bons choix (+1) et les mauvais (-1) faits en route
    if (aylis.honneur >= 2) {
        std::cout << colorer("FIN HEROIQUE", VERT + GRAS) << "\n";
        std::cout << "AYLIS a tendu la main aux blesses, meme aux Haschen. La nouvelle court de clan en clan :\n";
        std::cout << "les Haschen deposent les armes et reviennent dans leurs collines.\n";
        std::cout << "Dans la vallee, on raconte l'histoire d'AYLIS, qui a gagne la guerre ET la paix.\n";
    } else if (aylis.honneur <= -2) {
        std::cout << colorer("FIN SOMBRE", ROUGE + GRAS) << "\n";
        std::cout << "AYLIS a depouille les faibles et ignore les supplications. Les Haschen survivants\n";
        std::cout << "fuient, terrorises... et certains murmurent deja qu'un nouveau Vorgath est ne.\n";
        std::cout << "La vallee est sauvee, mais personne n'ose croiser le regard d'AYLIS.\n";
    } else {
        std::cout << colorer("FIN DU VOYAGEUR", CYAN + GRAS) << "\n";
        std::cout << "AYLIS a fait ce qu'il fallait, sans plus. La guerre est finie.\n";
        std::cout << "Les Haschen se dispersent, les villages reconstruisent, et AYLIS reprend la route,\n";
        std::cout << "seule silhouette sur le chemin, vers une autre aventure.\n";
    }

    // Un mot sur le compagnon, s'il y en a un
    if (etat.avecCompagnon) {
        std::cout << "\n";
        if (etat.compagnon.nom == "Kerrak") {
            std::cout << colorer("Kerrak", CYAN + GRAS) << ", le jeune deserteur, rentre chez les siens. Grace a lui,\n";
            std::cout << "les Haschen savent qu'un humain les a epargnes. Il deviendra le chef de son clan.\n";
        } else {
            std::cout << colorer(etat.compagnon.nom, CYAN + GRAS) << " touche sa prime et leve son verre a AYLIS\n";
            std::cout << "dans toutes les tavernes de la vallee. \"La meilleure paye de ma vie !\"\n";
        }
    }

    std::cout << "\n" << colorer("Honneur final : " + std::to_string(aylis.honneur), GRIS)
              << colorer("   (aide les autres pour une fin heroique... ou pas)", GRIS) << "\n";
}
