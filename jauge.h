// jauge.h : une jauge qui se remplit jusqu'a un maximum (la rage d'AYLIS)
#pragma once

// Une "class" ressemble a une struct, avec une grosse difference :
// ses donnees sont "private" (privees). Personne ne peut ecrire rage.valeurActuelle = 500
// depuis l'exterieur : il faut passer par les methodes "public", qui verifient tout.
// Ici, la jauge ne peut donc jamais depasser son maximum ni descendre sous 0.
class Jauge {
public:
    // Le "constructeur" : il porte le nom de la classe et prepare une jauge neuve (vide)
    Jauge(int maximum);

    void remplir(int quantite);         // ajoute, sans depasser le maximum
    void vider();                       // remet a 0
    void fixer(int nouvelleValeur);     // met une valeur precise (pour relire une sauvegarde)

    bool estPleine() const;
    int valeur() const;
    int maximum() const;

private:
    int valeurActuelle;
    int valeurMax;
};
