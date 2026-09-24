// jeu2d.h : les reglages et les donnees du jeu en vue du dessus
//
// On REUTILISE les structures du jeu console (types.h, dans le dossier du dessus) :
// Combattant, Arme, les enum class, la classe Jauge... Un pion sur l'arene, c'est simplement
// un Combattant (ses stats, son arme, ses etats) + une position + une couleur.
#pragma once

#include <string>
#include <vector>
#include "raylib.h"
#include "types.h"
#include "sprites.h"     // pour NOMBRE_LIEUX

// ===================== Les reglages =====================

const int COLONNES = 12;
const int LIGNES = 8;
const int TAILLE_CASE = 72;                         // en pixels
const int LARGEUR_FENETRE = COLONNES * TAILLE_CASE;
const int HAUTEUR_ARENE = LIGNES * TAILLE_CASE;
const int HAUTEUR_PANNEAU = 190;                    // la barre d'actions et le journal, en bas
const int HAUTEUR_FENETRE = HAUTEUR_ARENE + HAUTEUR_PANNEAU;

const int DEPLACEMENT_AYLIS = 3;                    // cases par tour
const int PORTEE_MELEE = 1;
const int PORTEE_DISTANCE = 4;                      // arc, arbalete, baton...
const int PORTEE_SORT = 5;
const int PORTEE_LANCEUR = 4;                       // les Haschen qui tirent de loin
const float PAUSE_ENTRE_ENNEMIS = 0.5f;             // en secondes
const float DUREE_FONDU = 2.2f;                     // la vision qui se brise, quand AYLIS tombe
const float DUREE_TEXTE_REVEIL = 6.0f;              // le temps pour que tout le texte du reveil s'ecrive

// ===================== Les donnees =====================

// Un pion sur l'arene : AYLIS ou un Haschen
struct Pion {
    Combattant stats;       // pv, attaque, arme, etats... : la meme struct que dans le jeu console
    int colonne;
    int ligne;
    Color couleur;

    // ----- Pour les animations (voir animations.cpp) -----
    float flash = 0.0f;         // entre 0 et 0.25 : le pion clignote en rouge (au-dessus : le coup n'est pas encore arrive)
    float xAffiche = -1.0f;     // la position a l'ecran, en pixels : elle "glisse" vers la case du pion
    float yAffiche = -1.0f;
    float elan = 0.0f;          // > 0 pendant un bond d'attaque vers la cible
    float elanX = 0.0f;         // la direction du bond
    float elanY = 0.0f;
    float recul = 0.0f;         // > 0 quand il vient d'etre frappe : il recule un peu
    float reculX = 0.0f;
    float reculY = 0.0f;
    float pvAffiches = -1.0f;   // la barre de vie "fantome", qui descend doucement apres un coup
    float disparition = 0.0f;   // > 0 pendant qu'il se dissout apres sa chute

    // ----- Pour les boss (voir boss.cpp) -----
    int boss = 0;               // 0 = pas un boss, sinon le numero du boss (BOSS_SKARN...)
    int compteur = 0;           // les tours du boss, pour ses attaques speciales
    bool phase2 = false;        // Vorgath : la deuxieme phase du combat
};

// ----- Les effets visuels -----

// Une particule : une etincelle, une flamme, un peu de fumee...
struct Particule {
    float x;
    float y;
    float vitesseX;
    float vitesseY;
    float vie;              // le temps qu'il lui reste, en secondes
    float vieMax;
    float taille;
    Color couleur;
    bool gravite = false;   // true : elle retombe
    bool fantome = false;   // true : c'est une silhouette d'AYLIS (la trainee pendant un deplacement)
};

enum class SorteProjectile {
    Fleche,
    Orbe,           // le tir du baton de mage
    BouleDeFeu,
    Javelot,
};

// Un projectile qui vole d'un pion a un autre
struct Projectile {
    float departX;
    float departY;
    float arriveeX;
    float arriveeY;
    float temps = 0.0f;     // depuis combien de temps il vole
    float duree;            // le temps total du vol
    SorteProjectile sorte;
};

enum class SorteEffet {
    Etincelles,
    Explosion,
    Fumee,
    Secousse,
    ArretSurImage,
    Recul,
};

// Un effet programme pour plus tard : par exemple les etincelles, au moment ou la fleche arrive
struct EffetEnAttente {
    float delai;            // dans combien de temps il se declenche
    SorteEffet sorte;
    float x;
    float y;
    Color couleur;
    float force;
    int pion;               // pour le recul : le numero du Haschen, ou -1 pour AYLIS
};

// Un eclair qui tombe du ciel
struct EffetEclair {
    float x;
    float y;
    float vie;
};

// Les grandes etapes de l'ecran
enum class Phase {
    ChoixVoie,      // l'ecran de depart
    Deplacement,    // tour d'AYLIS : deplacement encore possible
    Action,         // tour d'AYLIS : deplacement fait, il reste l'action
    TourEnnemi,     // les Haschen jouent, un par un
    CombatGagne,    // le combat vient d'etre gagne
    ChoixRune,      // AYLIS choisit une rune de prophetie (sa recompense)
    ChoixSalle,     // la vision : AYLIS choisit la prochaine salle
    Marchand,       // la boutique d'un marchand
    Rencontre,      // une rencontre sur la route (un choix a faire)
    Victoire,
    Defaite,        // AYLIS tombe : l'ecran se teinte de violet, la vision se brise...
    Reveil,         // ... et AYLIS se reveille, avec le souvenir de la vision
    Seuil,          // le monde entre les visions : on y depense les fragments de prophetie
    NouvelActe,     // le titre d'un nouvel acte de la route (apres chaque boss)
};

// ===================== La memoire (memoire.cpp) =====================
// Ce qu'AYLIS retient d'une course a l'autre. C'est enregistre dans un fichier :
// on le retrouve meme apres avoir ferme le jeu.
struct Memoire {
    int visions = 0;            // le nombre de courses terminees par une chute
    int victoires = 0;          // le nombre de courses gagnees (Vorgath vaincu)
    int meilleureSalle = 0;     // la salle la plus lointaine jamais atteinte
    int ashkaAffrontee = 0;     // combien de fois AYLIS a affronte Ashka (garde pour les vieilles memoires)
    int fragments = 0;          // les fragments de prophetie a depenser au Seuil
    int fragmentsTotal = 0;     // tous ceux gagnes depuis le debut
    int passagesAuSeuil = 0;    // le nombre de visites au Seuil (le monde entre les visions)
    int ameliorations[6] = {0, 0, 0, 0, 0, 0};  // le rang de chaque amelioration achetee au Seuil
    // Les choix des rencontres : le Seuil s'en souvient
    int voyageurAide = 0;
    int voyageurDepouille = 0;
    int deserteurEpargne = 0;
    int deserteurDepouille = 0;
    int bossAffrontes[4] = {0, 0, 0, 0};    // combien de fois AYLIS a affronte chaque boss
    int bossVaincus[4] = {0, 0, 0, 0};      // ... et combien de fois chacun est tombe
};

const int NOMBRE_AMELIORATIONS = 6;

// ===================== La route (route.cpp) =====================

// La route est coupee en 4 ACTES. Chaque acte traverse quelques lieux, et se termine par un boss.
const int NOMBRE_ACTES = 4;
const int BOSS_SKARN = 1;
const int BOSS_MATRIARCHE = 2;
const int BOSS_ASHKA = 3;
const int BOSS_VORGATH = 4;

struct Acte {
    std::string nom;
    std::vector<int> lieux;     // le lieu de chaque salle de l'acte (sans compter la salle du boss)
    int lieuDuBoss;
    int boss;                   // BOSS_SKARN, BOSS_MATRIARCHE...
};

const Acte& acte(int numero);                            // numero : 0 a NOMBRE_ACTES - 1
int nombreDeSalles();                                   // toutes les salles de la route (boss compris)
int lieuDeLaSalle(int salle);                           // le lieu prevu pour cette salle
int acteDeLaSalle(int salle);                           // la salle est numerotee de 1 a nombreDeSalles()
int premiereSalleDeLActe(int numero);
int nombreDeSallesDeLActe(int numero);                  // boss compris
bool estSalleDeBoss(int salle);
std::string nomDuBoss(int boss);

// Les sortes de salles que la vision peut montrer
enum class TypeSalle {
    Combat,     // quelques Haschen
    Elite,      // un Haschen d'elite et un compagnon : plus dur, mais une rune EPIQUE
    Repos,      // un feu de camp : AYLIS se soigne
    Oracle,     // une rune gratuite... mais la prophetie coute un peu de vie
    Marchand,   // Maren, Durgan ou Silas (selon le lieu)
    Rencontre,  // "???" : une surprise, avec un choix a faire
    Boss,       // Ashka
};

// Une salle proposee par la vision
struct Salle {
    TypeSalle type;
    int lieu;       // le numero du lieu (voir lieux.cpp)
};

// Une rune de prophetie : un bonus qu'AYLIS garde jusqu'a la fin de l'aventure
struct Rune {
    int numero;
    std::string nom;
    std::string description;
    bool epique;
};

// Un article dans la boutique d'un marchand (haltes.cpp)
struct Article {
    int numero;             // quel effet il a (voir acheter)
    std::string nom;
    std::string description;
    int prix;
    bool vendu = false;
};

// Les actions d'AYLIS (la barre en bas de l'ecran)
enum class Action {
    Attaque,
    AttaqueLourde,
    Garde,
    Potion,
    BouleDeFeu,
    Soin,
    Eclair,
    Bouclier,
    Speciale,
};

// Un texte qui s'envole au-dessus d'un pion (les degats, les soins...), puis s'efface
struct TexteFlottant {
    std::string texte;
    float x;
    float y;
    float tempsRestant;
    Color couleur;
    float delai = 0.0f;     // il n'apparait qu'une fois ce delai ecoule (quand le coup arrive vraiment)
    float age = 0.0f;       // depuis combien de temps il est affiche (pour le petit rebond)
};

// Tout l'etat du jeu
struct Jeu {
    Pion aylis;
    std::vector<Pion> haschen;
    std::vector<bool> rochers;          // une case par position : true = rocher
    Phase phase = Phase::ChoixVoie;
    Action actionChoisie = Action::Attaque;
    Jauge rage = Jauge(rageMax);        // la classe du jeu console
    bool enGarde = false;               // attaque en garde ce tour-ci : les coups recus sont divises par 2
    // La route
    int salle = 1;                      // le numero de la salle en cours (1 a nombreDeSalles())
    // Les attaques annoncees : les cases qui vont exploser au prochain tour du boss (a eviter !)
    std::vector<std::pair<int, int>> zonesDanger;
    int degatsDanger = 0;
    int lieu = 0;                       // le lieu de la salle (0 a NOMBRE_LIEUX - 1, voir lieux.cpp)
    std::vector<char> terrain;          // la carte du lieu : une lettre par case (voir lieux.cpp)
    TypeSalle typeSalle = TypeSalle::Combat;
    std::vector<Salle> propositions;    // les salles montrees par la vision
    std::vector<Rune> runesProposees;   // les runes parmi lesquelles choisir
    std::vector<Rune> runes;            // les runes qu'AYLIS possede
    int feuColonne = -1;                // la case du feu de camp (dans le camp haschen)
    int feuLigne = -1;
    std::vector<std::pair<int, int>> lumieres;  // les cases des sources de lumiere (champignons, braseros, cristaux)

    // Les effets des runes
    int deplacement = DEPLACEMENT_AYLIS;    // cases par tour (la rune du Vent l'augmente)
    int porteeBonus = 0;                    // la rune de l'Oeil : +1 de portee a distance
    bool runeFlamme = false;                // les attaques peuvent bruler
    bool runeSeve = false;                  // AYLIS se soigne a chaque Haschen abattu
    bool runeFureur = false;                // la rage monte 2 fois plus vite
    // Les haltes : marchands et rencontres
    int marchand = 0;                   // 0 = Maren, 1 = Durgan, 2 = Silas
    std::vector<Article> articles;
    int rencontre = 0;                  // le numero de la rencontre en cours
    std::string resultatRencontre;      // vide tant que le choix n'est pas fait
    bool runeOfferte = false;           // la rencontre se termine par le choix d'une rune
    std::vector<int> rencontresVues;    // pour ne pas vivre deux fois la meme rencontre dans une course
    // La memoire et la fin d'une course
    Memoire memoire;
    std::string derniereBlessure;       // qui a porte le dernier coup a AYLIS (pour le texte du reveil)
    int elitesVaincues = 0;             // pendant cette course
    int fragmentsGagnes = 0;            // pendant cette course
    int interlocuteur = -1;             // au Seuil : le personnage a qui AYLIS parle (-1 = personne)
    std::string messageSeuil;           // au Seuil : ce qui vient de se passer (achat, pas assez de fragments...)
    float fondu = 0.0f;                 // le fondu violet quand la vision se brise, puis le texte du reveil
    std::string messageRoute;          // ce qui vient de se passer sur la route (affiche sur les ecrans de choix)
    std::string nomDuLieu;
    int ennemiQuiJoue = 0;
    float minuteur = 0.0f;
    int tour = 1;
    std::vector<std::string> journal;
    std::vector<TexteFlottant> textes;
    std::string banniere;               // le grand titre anime au milieu ("TOUR 3", le nom du lieu...)
    float tempsBanniere = 0.0f;         // combien de temps il reste affiche, en secondes

    // Les effets visuels en cours
    std::vector<Particule> particules;
    std::vector<Projectile> projectiles;
    std::vector<EffetEnAttente> effets;
    std::vector<EffetEclair> eclairs;
    float secousse = 0.0f;              // la force du tremblement d'ecran
    float arretSurImage = 0.0f;         // > 0 : le jeu se fige une fraction de seconde (coup critique)
};

// ===================== regles.cpp : les regles du jeu =====================

// La distance en cases, sans diagonales
int distanceCases(int c1, int l1, int c2, int l2);
int distanceEntre(const Pion& a, const Pion& b);
bool estDansArene(int colonne, int ligne);
bool estRocher(const Jeu& jeu, int colonne, int ligne);
int haschenSurCase(const Jeu& jeu, int colonne, int ligne);     // son numero, ou -1

// Les cases ou AYLIS peut aller ce tour-ci : pour chaque case, le nombre de pas (-1 = inaccessible)
std::vector<int> casesAtteignables(const Jeu& jeu);

// Les actions
std::string nomAction(Action action);
std::string descriptionAction(Action action);           // pour la bulle d'aide au survol
int porteeAction(const Jeu& jeu, Action action);        // 0 = sur soi-meme
int coutMana(Action action);
bool actionDisponible(const Jeu& jeu, Action action);   // assez de mana, de rage, de potions, sort connu...
bool actionSurSoi(Action action);                       // potion, soin, bouclier : pas besoin de cible

// Les grandes etapes
void choisirVoie(Jeu& jeu, int voie);                   // 1 = epee, 2 = arc, 3 = arcanes
void preparerCombat(Jeu& jeu);                          // place AYLIS et les Haschen de la salle en cours
void apresCombat(Jeu& jeu);                             // apres "combat gagne" : la recompense (une rune)
void choisirRune(Jeu& jeu, int numero);                 // numero = 0, 1 ou 2
void choisirSalle(Jeu& jeu, int numero);
std::string nomLieu(int lieu);
std::string nomTypeSalle(TypeSalle type);
std::string descriptionSalle(TypeSalle type);
const std::vector<Rune>& catalogueRunes();
bool runeDisponible(const Jeu& jeu, const Rune& rune);
void proposerRunes(Jeu& jeu, bool epique);
void appliquerRune(Jeu& jeu, const Rune& rune);
void proposerSalles(Jeu& jeu);                          // la vision : les salles possibles pour jeu.salle
void allerPlusLoin(Jeu& jeu);                           // la salle est finie : la vision montre la suite

// ===================== haltes.cpp : les marchands et les rencontres =====================

void ouvrirBoutique(Jeu& jeu);
void acheter(Jeu& jeu, int numero);
std::string nomMarchand(int marchand);
std::string titreMarchand(int marchand);
std::string paroleMarchand(int marchand);
void commencerRencontre(Jeu& jeu);
std::string titreRencontre(int rencontre);
std::string texteRencontre(int rencontre);
std::string reponseRencontre(const Jeu& jeu, int rencontre, int reponse);   // le texte des 2 choix
void repondreRencontre(Jeu& jeu, int reponse);          // 0 ou 1
void finirRencontre(Jeu& jeu);                          // apres avoir lu le resultat
void gagnerOr(Jeu& jeu, int pieces);

// ===================== memoire.cpp : ce qu'AYLIS retient d'une course a l'autre =====================

void chargerMemoire(Memoire& memoire);
void enregistrerMemoire(const Memoire& memoire);
void terminerCourse(Jeu& jeu, bool victoire);           // compte les fragments et enregistre la memoire
std::string texteDuReveil(const Jeu& jeu);              // ce dont AYLIS se souvient en ouvrant les yeux
std::string phraseDeDepart(const Memoire& memoire);     // le sous-titre de l'ecran de depart

// Le tour d'AYLIS
void deplacerAylis(Jeu& jeu, int colonne, int ligne);
bool agirSurHaschen(Jeu& jeu, int numero);              // renvoie true si l'action a ete faite
bool agirSurSoi(Jeu& jeu);
void finirTourAylis(Jeu& jeu);

// Le tour des Haschen (appelee a chaque image)
void mettreAJourTourEnnemi(Jeu& jeu, float secondes);

void ecrireJournal(Jeu& jeu, const std::string& message);
void mettreAJourTextes(Jeu& jeu, float secondes);

// ===================== dessin.cpp : l'affichage =====================

void dessinerJeu(const Jeu& jeu, int colonneSouris, int ligneSouris);
void dechargerLumiere();                                // a appeler avant CloseWindow

// Les boutons de la barre d'actions : leur rectangle a l'ecran
Rectangle rectangleBouton(int numero);
const std::vector<Action>& actionsDeLaBarre();
Rectangle rectangleCarteVoie(int voie);                 // les 3 cartes de l'ecran de depart
Rectangle rectangleCarteChoix(int numero, int nombre);  // les cartes des salles et des runes
Rectangle rectangleCarteReponse(int numero);            // les 2 choix d'une rencontre
Rectangle rectangleEcho(int personnage);                 // au Seuil : pour cliquer sur Maren, Durgan ou Silas
Rectangle rectanglePortail();
Rectangle rectangleAmelioration(int choix);             // les 2 ameliorations de la discussion
Rectangle rectangleBoutonRoute();                       // "reprendre la route" (boutique, rencontre)

// ===================== seuil.cpp : le monde entre les visions =====================

std::string nomAmelioration(int numero);
std::string descriptionAmelioration(int numero);
int rangMaxAmelioration(int numero);
int coutAmelioration(const Memoire& memoire, int numero);   // 0 = rang maximum atteint
void acheterAmelioration(Jeu& jeu, int choix);              // choix 0 ou 1, chez le personnage a qui AYLIS parle
void appliquerAmeliorations(Jeu& jeu);                      // au depart d'une course
void entrerAuSeuil(Jeu& jeu);
void parlerA(Jeu& jeu, int personnage);                     // 0 = Maren, 1 = Durgan, 2 = Silas, -1 = personne
std::string paroleAuSeuil(const Memoire& m, int personnage);
std::string murmureDuSeuil(const Memoire& m);
Vector2 positionAuSeuil(int personnage);                    // 0 a 2 = les echos, 3 = AYLIS, 4 = le portail

// ===================== lieux.cpp : les lieux de la route et leurs cartes =====================

std::string nomCourtLieu(int lieu);
int marchandDuLieu(int lieu);                           // 0 = Maren, 1 = Durgan, 2 = Silas
void chargerCarte(Jeu& jeu);                            // choisit une carte du lieu et remplit le terrain
char caseDuTerrain(const Jeu& jeu, int colonne, int ligne);     // ' ' hors de l'arene
std::vector<bool> casesAccessibles(const Jeu& jeu);    // depuis la case de depart d'AYLIS

// ===================== decor.cpp : le terrain des lieux, dessine =====================

void dessinerTerrain(const Jeu& jeu);                   // le sol et les constructions (avant la penombre)
void ajouterLumieresDuDecor(const Jeu& jeu);            // les fenetres allumees... (dans la carte de lumiere)
std::vector<Vector2> cheminsDeFumee(const Jeu& jeu);    // d'ou monte la fumee des maisons brulees

// ===================== boss.cpp : les gardiens de la route =====================

void placerHaschen(Jeu& jeu, const std::vector<bool>& accessibles, Combattant stats, Color couleur);   // route.cpp
void toucherAylis(Jeu& jeu, const Pion& attaquant, int degats, bool critique, float delai);            // regles.cpp
void placerBossEtEscorte(Jeu& jeu, const std::vector<bool>& accessibles, int boss,
                         const std::vector<std::pair<Combattant, Color>>& groupe);
bool jouerTourDeBoss(Jeu& jeu, Pion& boss);              // true : le boss a fait une action speciale
void bossTombe(Jeu& jeu, Pion& boss);                    // son escorte s'enfuit
