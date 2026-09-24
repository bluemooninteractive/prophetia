// jauge.cpp : les methodes de la classe Jauge
#include "jauge.h"

// Le constructeur : ": valeurActuelle(0), valeurMax(maximum)" donne leur valeur de depart aux donnees
Jauge::Jauge(int maximum) : valeurActuelle(0), valeurMax(maximum) {
}

void Jauge::remplir(int quantite) {
    fixer(valeurActuelle + quantite);
}

void Jauge::vider() {
    valeurActuelle = 0;
}

void Jauge::fixer(int nouvelleValeur) {
    valeurActuelle = nouvelleValeur;
    if (valeurActuelle > valeurMax) {
        valeurActuelle = valeurMax;
    }
    if (valeurActuelle < 0) {
        valeurActuelle = 0;
    }
}

bool Jauge::estPleine() const {
    return valeurActuelle >= valeurMax;
}

int Jauge::valeur() const {
    return valeurActuelle;
}

int Jauge::maximum() const {
    return valeurMax;
}
