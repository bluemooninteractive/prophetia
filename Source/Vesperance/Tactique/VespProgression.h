// VespProgression : ce qui fait grandir AYLIS au fil de la partie.
//
//   Le Seuil : une constellation de 12 etoiles, en trois voies (la Lame, le Rempart, la Prophetie).
//   Chaque niveau gagne donne un point ; une etoile s'allume si la precedente de sa voie est allumee.
//   Trois etoiles sont des POUVOIRS (touches 1, 2, 3 ; manette : LB, RT, croix droite), les autres des talents.
//
//   Le butin : des objets qui tombent des Haschen, des coffres, des elites et des boss, avec leur rarete
//   (commun, rare, epique, legendaire). Six emplacements : tete, amulette, arme, anneau, main gauche, corps.
//   L'arme et le bouclier se VOIENT (le modele change, et la facon de se battre aussi : epee, hache,
//   dagues, arme a deux mains, baton de sorts) ; l'armure du corps change la couleur de la tenue.
#pragma once

#include "CoreMinimal.h"
#include "VespUnite.h"

// ===================== Le Seuil =====================

enum class EVespVoie : uint8 { Lame, Rempart, Prophetie };

struct FVespEtoile
{
	const TCHAR* Id;
	const TCHAR* Nom;
	const TCHAR* Aide;
	EVespVoie Voie;
	int32 Rang;				// 1 a 4 : il faut les rangs d'avant pour l'allumer
	int32 Pouvoir;			// 0 : un talent ; 1, 2, 3 : le pouvoir de cette touche
	float Recharge;			// un pouvoir : secondes avant de le relancer
};

namespace VespSeuil
{
	constexpr int32 Nombre = 12;
	const FVespEtoile& Etoile(int32 Index);
	int32 Index(const TCHAR* Id);
	FLinearColor CouleurVoie(EVespVoie Voie);
	const TCHAR* NomVoie(EVespVoie Voie);
}

// ===================== Le Veilleur =====================
// Entre deux visions, le Veilleur Oswin echange les Souvenirs (gagnes en route, gardes a la mort) contre des dons.
// Un don dure pour toutes les visions suivantes : c'est ce qui rend la suivante un peu plus forte.

struct FVespDon
{
	const TCHAR* Id;
	const TCHAR* Nom;
	const TCHAR* Aide;		// ce que donne chaque rang
	int32 RangMax;
	int32 Prix[5];			// le prix de chaque rang
};

namespace VespVeilleur
{
	constexpr int32 Nombre = 7;
	const FVespDon& Don(int32 Index);
	int32 Index(const TCHAR* Id);
	int32 Prix(int32 Index, int32 RangActuel);		// le prix du rang suivant (0 : deja au maximum)
}

// ===================== Le butin =====================

enum class EVespRarete : uint8 { Commun, Rare, Epique, Legendaire };

enum class EVespEmplacement : uint8 { Tete, Amulette, Arme, Anneau, MainGauche, Corps, Nombre };

struct FVespObjet
{
	FString Nom;
	FString Recit;						// une ligne d'histoire (en italique dans la fiche)
	EVespEmplacement Emplacement = EVespEmplacement::Arme;
	EVespRarete Rarete = EVespRarete::Commun;
	int32 Niveau = 1;
	// Ce qu'il donne
	int32 Attaque = 0;
	int32 Defense = 0;
	int32 PvMax = 0;
	int32 Critique = 0;					// en points de %
	float Vitesse = 0.0f;				// +0.05 = 5% plus vite
	int32 Effet = 0;					// VespEffetCoup : ses coups brulent, gelent, empoisonnent...
	float ChanceEffet = 0.0f;
	int32 VolDeVie = 0;					// pv rendus a chaque coup
	// Ce qu'on voit
	FString Modele;						// le chemin du modele (arme, bouclier)
	FString SecondModele;				// les dagues : celle de la main gauche
	float Longueur = 0.5f;
	EVespArme TypeArme = EVespArme::Epee;
	FLinearColor Teinte = FLinearColor::White;	// l'armure : la couleur de la tenue
	int32 Icone = 0;
	uint32 Graine = 0;

	FString Lignes() const;				// les effets, une ligne chacun
	int32 Valeur() const;				// pour comparer deux objets (et le prix au marchand)
};

namespace VespButin
{
	FLinearColor CouleurRarete(EVespRarete R);
	const TCHAR* NomRarete(EVespRarete R);
	const TCHAR* NomEmplacement(EVespEmplacement E);
	// Un objet au hasard, a la hauteur de l'acte (Chance : 0 = ordinaire, 1 = elite, 2 = boss)
	FVespObjet Tirer(int32 Acte, int32 Chance, FRandomStream& Hasard);
	FVespObjet Tirer(int32 Acte, int32 Chance, EVespEmplacement Emplacement, FRandomStream& Hasard);
	// L'equipement de depart d'AYLIS
	FVespObjet EpeeDeDepart();
	FVespObjet BouclierDeDepart();
}
