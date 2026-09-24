// outils.cpp : les petites fonctions utiles partout
#include <iostream>
#include <cstdlib>
#include "outils.h"

// Lit un choix entre min et max. Redemande tant que ce n'est pas valide.
int lireChoix(int min, int max) {
    while (true) {
        std::cout << "Ton choix : ";
        int choix;
        std::cin >> choix;

        if (!std::cin) {
            // Plus rien a lire (console fermee) : on quitte le jeu
            if (std::cin.eof()) {
                std::exit(0);
            }
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        } else if (choix >= min && choix <= max) {
            return choix;
        }
        std::cout << "Tape un nombre entre " << min << " et " << max << ".\n";
    }
}

// Calcule les degats d'une attaque : attaque x puissance - defense,
// plus un peu de hasard et parfois un coup critique.
// La puissance est en pourcentage : 100 = normal, 180 = presque le double.
// chanceCritique est sur 100 : 10 = 1 chance sur 10 de faire x2.
int calculerDegats(int attaque, int puissance, int defense, int chanceCritique) {
    int degats = attaque * puissance / 100 - defense;

    // Hasard : -1, 0 ou +1
    degats = degats + (std::rand() % 3) - 1;

    // Coup critique : degats x2
    if (std::rand() % 100 < chanceCritique) {
        degats = degats * 2;
        std::cout << "COUP CRITIQUE ! ";
    }

    // On fait toujours au moins 1 point de degats
    if (degats < 1) {
        degats = 1;
    }

    return degats;
}

// Affiche une barre comme [##########----------]
void afficherBarre(int valeur, int maximum) {
    const int largeur = 20;
    if (valeur < 0) {
        valeur = 0;
    }
    int remplis = valeur * largeur / maximum;

    std::cout << "[";
    for (int i = 0; i < largeur; i++) {
        if (i < remplis) {
            std::cout << "#";
        } else {
            std::cout << "-";
        }
    }
    std::cout << "]";
}

// Affiche la description d'une arme sur une ligne
void afficherArme(const Arme& arme) {
    std::cout << arme.nom;
    if (arme.aDistance) {
        std::cout << " [DISTANCE]";
    } else {
        std::cout << " [MELEE]";
    }
    std::cout << "  attaque ";
    if (arme.bonusAttaque >= 0) {
        std::cout << "+";
    }
    std::cout << arme.bonusAttaque << ", critique " << arme.chanceCritique << "%";
    if (arme.nombreDeCoups > 1) {
        std::cout << ", frappe " << arme.nombreDeCoups << " fois";
    }
}

// Le nom d'une rarete
std::string nomRarete(int rarete) {
    if (rarete == EPIQUE) {
        return "EPIQUE";
    } else if (rarete == RARE) {
        return "RARE";
    }
    return "commun";
}

// Transforme une arme en objet d'inventaire (revendue a moitie prix)
Objet objetDepuisArme(const Arme& arme) {
    return {arme.nom, OBJET_ARME, arme.rarete, arme.prix / 2, arme};
}

// Affiche un objet sur une ligne, par exemple : [RARE] Griffe de berserker (25 or)
void afficherObjet(const Objet& objet) {
    std::cout << "[" << nomRarete(objet.rarete) << "] ";
    if (objet.type == OBJET_ARME) {
        afficherArme(objet.arme);
    } else {
        std::cout << objet.nom;
    }
    std::cout << "  (" << objet.valeur << " or)";
}

// Soigne un combattant sans depasser ses pv max
void soigner(Combattant& c, int quantite) {
    c.pv = c.pv + quantite;
    if (c.pv > c.pvMax) {
        c.pv = c.pvMax;
    }
}

// Boire une potion : +15 pv
void boirePotion(Combattant& c) {
    c.potions = c.potions - 1;
    soigner(c, 15);
    std::cout << c.nom << " boit une potion ! Retour a " << c.pv << " pv.\n";
}

// Attend que le joueur appuie sur Entree (pour que la fenetre ne se ferme pas toute seule)
void attendreEntree() {
    std::cout << "\nAppuie sur Entree pour quitter...";
    std::cin.ignore(1000, '\n');    // on vide la fin de la ligne du dernier choix
    std::cin.get();                 // puis on attend Entree
}
