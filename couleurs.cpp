// couleurs.cpp : allumer les couleurs de la console et colorer du texte
#include <string>
#include "couleurs.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Les couleurs sont eteintes tant qu'on n'a pas verifie que la console les accepte
bool couleursAllumees = false;

void activerCouleurs() {
#ifdef _WIN32
    // Sous Windows, il faut demander a la console d'interpreter les codes ANSI
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(console, &mode)) {
        return;     // ce n'est pas une vraie console (par exemple une sortie redirigee) : pas de couleurs
    }
    if (!SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return;     // console trop ancienne : pas de couleurs
    }
#endif
    couleursAllumees = true;
}

std::string colorer(const std::string& texte, const std::string& couleur) {
    if (!couleursAllumees) {
        return texte;
    }
    return couleur + texte + NORMAL;
}

std::string colorer(int nombre, const std::string& couleur) {
    return colorer(std::to_string(nombre), couleur);
}
