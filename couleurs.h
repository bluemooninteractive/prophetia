// couleurs.h : des couleurs dans la console, avec les codes "ANSI"
#pragma once

#include <string>

// Chaque couleur est une petite suite de caracteres speciaux que la console comprend.
// "\033" est le caractere "Echap" : il annonce a la console qu'une commande arrive.
const std::string ROUGE = "\033[91m";
const std::string VERT = "\033[92m";
const std::string JAUNE = "\033[93m";
const std::string BLEU = "\033[94m";
const std::string VIOLET = "\033[95m";
const std::string CYAN = "\033[96m";
const std::string GRIS = "\033[90m";
const std::string GRAS = "\033[1m";
const std::string NORMAL = "\033[0m";      // revient a l'affichage normal

// A appeler une fois au debut : allume les couleurs si la console sait les afficher
void activerCouleurs();

// Renvoie le texte dans la couleur demandee (ou tel quel si les couleurs sont eteintes).
// Exemple : std::cout << colorer("VICTOIRE", VERT);
std::string colorer(const std::string& texte, const std::string& couleur);

// Pareil pour un nombre
std::string colorer(int nombre, const std::string& couleur);
