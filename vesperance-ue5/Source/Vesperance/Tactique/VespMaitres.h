// VespMaitres : les maitres des elements. Pendant une vision, ils apparaissent par projection dans les clairieres
// et offrent un don parmi trois (a la maniere des dieux de Hades).
//
//   Brann, le Forgeron (le feu)        Isaure, l'Ermite des cimes (le givre)     Orme, le Vieux Druide (la seve)
//   Velka, la Vagabonde (l'orage)      Aurel, le Premier Temoin (l'aube)
//   et Liss, qu'on appelle « la Felure » : des pactes (une malediction pendant quelques clairieres, puis un gros bonus).
//
// Un don se greffe sur un geste d'AYLIS : l'attaque, la lourde, l'esquive, la speciale (un seul par geste : le nouveau
// remplace l'ancien) ; ou il reste passif. Il a une rarete (commun, rare, epique, heroique) qui fixe sa force.
// Deux maitres suivis ensemble ouvrent un don double.
#pragma once

#include "CoreMinimal.h"

namespace VespMaitres
{
	constexpr int32 Nombre = 5;
	constexpr int32 Brann = 0, Isaure = 1, Orme = 2, Velka = 3, Aurel = 4;
	constexpr int32 Liss = -1;			// (celle qui offre des pactes)

	// Les gestes d'AYLIS qui portent un don
	constexpr int32 Attaque = 0, Lourde = 1, Esquive = 2, Speciale = 3, Passif = 4;
	constexpr int32 GestesUniques = 4;	// un seul don par geste ; les passifs se cumulent
	constexpr int32 Raretes = 4;		// commun, rare, epique, heroique

	struct FDon
	{
		const TCHAR* Id;
		int32 Maitre;
		int32 Maitre2;					// -1, ou l'autre maitre d'un don double
		int32 Geste;
		const TCHAR* Nom;
		const TCHAR* Aide;				// {v} : la valeur, selon la rarete
		float Valeurs[Raretes];
		const TCHAR* Unite;
	};

	int32 NombreDons();
	const FDon& Don(int32 Index);
	int32 Index(const TCHAR* Id);
	FString Description(int32 Don, int32 Rarete);
	const TCHAR* Nom(int32 Maitre);
	const TCHAR* Titre(int32 Maitre);
	FLinearColor Couleur(int32 Maitre);
	FString Replique(int32 Maitre);			// une phrase au hasard quand il apparait (Liss : -1)
	const TCHAR* NomGeste(int32 Geste);
	const TCHAR* NomRarete(int32 Rarete);

	// Les pactes de Liss
	constexpr int32 NombreMaledictions = 3;		// sang fragile, soif, aveuglement
	constexpr int32 NombreBonus = 5;
	constexpr int32 DureePacte = 3;				// en clairieres liberees
	const TCHAR* Malediction(int32 M);
	const TCHAR* AideMalediction(int32 M);
	const TCHAR* Bonus(int32 B);
}
