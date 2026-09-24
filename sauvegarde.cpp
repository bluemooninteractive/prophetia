// sauvegarde.cpp : enregistrer la partie dans un fichier texte, et la relire
//
// Le fichier contient une valeur par ligne, toujours dans le meme ordre.
// Pour relire, il suffit de lire les lignes dans ce meme ordre.
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "sauvegarde.h"

// Le nom du fichier, cree a cote du jeu
const std::string fichierSauvegarde = "vesperance_sauvegarde.txt";

// La premiere ligne du fichier : elle permet de verifier que c'est bien une sauvegarde du jeu
const std::string entete = "VESPERANCE-SAUVEGARDE-2";

// ===================== Ecrire =====================
// std::ofstream = "output file stream" : un fichier dans lequel on ecrit, comme on ecrit dans std::cout

void ecrireArme(std::ofstream& fichier, const Arme& arme) {
    fichier << arme.nom << "\n";
    fichier << arme.aDistance << "\n";
    fichier << arme.bonusAttaque << "\n";
    fichier << arme.chanceCritique << "\n";
    fichier << arme.nombreDeCoups << "\n";
    fichier << arme.prix << "\n";
    fichier << static_cast<int>(arme.rarete) << "\n";   // un enum s'ecrit sous forme de nombre...
}

// Un combattant : ses stats, son arme et son inventaire
void ecrireCombattant(std::ofstream& fichier, const Combattant& c) {
    fichier << c.nom << "\n";
    fichier << c.pv << "\n" << c.pvMax << "\n";
    fichier << c.attaque << "\n" << c.defense << "\n";
    fichier << c.potions << "\n";
    fichier << c.niveau << "\n" << c.xp << "\n" << c.points << "\n";
    fichier << c.pieces << "\n";
    fichier << c.mana << "\n" << c.manaMax << "\n" << c.sortsConnus << "\n";
    fichier << c.honneur << "\n";
    fichier << c.voie << "\n";
    ecrireArme(fichier, c.arme);

    // L'inventaire : d'abord le nombre d'objets, puis chaque objet
    fichier << c.inventaire.size() << "\n";
    for (const Objet& objet : c.inventaire) {
        fichier << objet.nom << "\n";
        fichier << static_cast<int>(objet.type) << "\n";
        fichier << static_cast<int>(objet.rarete) << "\n";
        fichier << objet.valeur << "\n";
        ecrireArme(fichier, objet.arme);
    }
}

void sauvegarder(const EtatPartie& etat) {
    std::ofstream fichier(fichierSauvegarde);
    if (!fichier) {
        return;     // impossible d'ecrire le fichier : tant pis, on continue sans sauvegarder
    }

    fichier << entete << "\n";
    fichier << etat.etape << "\n";
    fichier << etat.haltesVisitees << "\n";
    fichier << etat.ashkaVaincue << "\n";
    fichier << etat.skarnVaincu << "\n";
    fichier << etat.difficulte << "\n";
    fichier << etat.rage.valeur() << "\n";
    ecrireCombattant(fichier, etat.aylis);

    fichier << etat.avecCompagnon << "\n";
    if (etat.avecCompagnon) {
        ecrireCombattant(fichier, etat.compagnon);
    }
}   // en sortant de la fonction, le fichier est ferme tout seul

// ===================== Lire =====================
// std::ifstream = "input file stream" : un fichier qu'on lit, comme on lit std::cin

// Lit une ligne de texte
std::string lireTexte(std::ifstream& fichier) {
    std::string ligne;
    std::getline(fichier, ligne);
    return ligne;
}

// Lit une ligne et la transforme en nombre ("12" -> 12)
int lireNombre(std::ifstream& fichier) {
    return std::atoi(lireTexte(fichier).c_str());
}

void lireArme(std::ifstream& fichier, Arme& arme) {
    arme.nom = lireTexte(fichier);
    arme.aDistance = lireNombre(fichier) == 1;
    arme.bonusAttaque = lireNombre(fichier);
    arme.chanceCritique = lireNombre(fichier);
    arme.nombreDeCoups = lireNombre(fichier);
    arme.prix = lireNombre(fichier);
    arme.rarete = static_cast<Rarete>(lireNombre(fichier));     // ... et se relit a partir du nombre
}

void lireCombattant(std::ifstream& fichier, Combattant& c) {
    c.nom = lireTexte(fichier);
    c.pv = lireNombre(fichier);
    c.pvMax = lireNombre(fichier);
    c.attaque = lireNombre(fichier);
    c.defense = lireNombre(fichier);
    c.potions = lireNombre(fichier);
    c.niveau = lireNombre(fichier);
    c.xp = lireNombre(fichier);
    c.points = lireNombre(fichier);
    c.pieces = lireNombre(fichier);
    c.mana = lireNombre(fichier);
    c.manaMax = lireNombre(fichier);
    c.sortsConnus = lireNombre(fichier);
    c.honneur = lireNombre(fichier);
    c.voie = lireTexte(fichier);
    lireArme(fichier, c.arme);

    int nombreObjets = lireNombre(fichier);
    c.inventaire.clear();
    for (int i = 0; i < nombreObjets; i++) {
        Objet objet = {};
        objet.nom = lireTexte(fichier);
        objet.type = static_cast<TypeObjet>(lireNombre(fichier));
        objet.rarete = static_cast<Rarete>(lireNombre(fichier));
        objet.valeur = lireNombre(fichier);
        lireArme(fichier, objet.arme);
        c.inventaire.push_back(objet);
    }
}

bool sauvegardeExiste() {
    std::ifstream fichier(fichierSauvegarde);
    return fichier && lireTexte(fichier) == entete;
}

bool charger(EtatPartie& etat) {
    std::ifstream fichier(fichierSauvegarde);
    if (!fichier || lireTexte(fichier) != entete) {
        return false;
    }

    etat.etape = lireNombre(fichier);
    etat.haltesVisitees = lireNombre(fichier);
    etat.ashkaVaincue = lireNombre(fichier) == 1;
    etat.skarnVaincu = lireNombre(fichier) == 1;
    etat.difficulte = lireNombre(fichier);
    etat.rage.fixer(lireNombre(fichier));
    lireCombattant(fichier, etat.aylis);

    etat.avecCompagnon = lireNombre(fichier) == 1;
    if (etat.avecCompagnon) {
        lireCombattant(fichier, etat.compagnon);
    }

    // Si la lecture a echoue en route (fichier coupe ou abime), on refuse la sauvegarde
    return !fichier.fail() && etat.aylis.pvMax > 0;
}

void effacerSauvegarde() {
    std::remove(fichierSauvegarde.c_str());
}
