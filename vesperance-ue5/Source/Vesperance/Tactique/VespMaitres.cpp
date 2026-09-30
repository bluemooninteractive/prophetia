#include "VespMaitres.h"

using namespace VespMaitres;

// ===================== Les 28 dons =====================

static const FDon DONS[] = {
	// Brann, le feu : brulures, explosions
	{TEXT("b_attaque"), Brann, -1, Attaque, TEXT("Coups ardents"), TEXT("Les coups de l'enchaînement brûlent ({v} de chances)."), {30, 40, 55, 75}, TEXT(" %")},
	{TEXT("b_lourde"), Brann, -1, Lourde, TEXT("Choc de forge"), TEXT("L'attaque lourde explose autour de la cible ({v} de l'attaque) et brûle."), {80, 110, 140, 180}, TEXT(" %")},
	{TEXT("b_esquive"), Brann, -1, Esquive, TEXT("Traînée de braise"), TEXT("L'esquive laisse une gerbe de feu ({v} de l'attaque) qui brûle."), {50, 70, 90, 120}, TEXT(" %")},
	{TEXT("b_speciale"), Brann, -1, Speciale, TEXT("Nova de flammes"), TEXT("L'attaque spéciale frappe plus fort (+{v}) et brûle tout ce qu'elle touche."), {50, 80, 110, 150}, TEXT(" %")},
	{TEXT("b_passif"), Brann, -1, Passif, TEXT("Foyer"), TEXT("+{v} de dégâts contre les Haschen qui brûlent."), {15, 22, 30, 40}, TEXT(" %")},
	// Isaure, le givre : gel, parades
	{TEXT("i_attaque"), Isaure, -1, Attaque, TEXT("Coups de givre"), TEXT("Les coups de l'enchaînement gèlent ({v} de chances)."), {25, 35, 45, 60}, TEXT(" %")},
	{TEXT("i_lourde"), Isaure, -1, Lourde, TEXT("Étreinte glacée"), TEXT("L'attaque lourde gèle toujours, et frappe plus fort (+{v})."), {20, 35, 50, 70}, TEXT(" %")},
	{TEXT("i_esquive"), Isaure, -1, Esquive, TEXT("Piège de givre"), TEXT("L'esquive gèle les Haschen autour ({v} de l'attaque)."), {30, 40, 50, 70}, TEXT(" %")},
	{TEXT("i_speciale"), Isaure, -1, Speciale, TEXT("Tempête blanche"), TEXT("L'attaque spéciale gèle tout ce qu'elle touche, et frappe plus fort (+{v})."), {30, 50, 70, 100}, TEXT(" %")},
	{TEXT("i_passif"), Isaure, -1, Passif, TEXT("Patience"), TEXT("La parade parfaite est plus facile : +{v} pour la réussir."), {50, 80, 110, 150}, TEXT(" ms")},
	// Orme, la seve : soins, vol de vie
	{TEXT("o_attaque"), Orme, -1, Attaque, TEXT("Coups de sève"), TEXT("Chaque coup de l'enchaînement rend {v}."), {1, 2, 3, 4}, TEXT(" pv")},
	{TEXT("o_lourde"), Orme, -1, Lourde, TEXT("Racines"), TEXT("L'attaque lourde rend {v} des pv max par Haschen touché (3 au plus)."), {2, 3, 4, 6}, TEXT(" %")},
	{TEXT("o_esquive"), Orme, -1, Esquive, TEXT("Pas léger"), TEXT("Chaque esquive rend {v}."), {1, 2, 3, 4}, TEXT(" pv")},
	{TEXT("o_speciale"), Orme, -1, Speciale, TEXT("Floraison"), TEXT("L'attaque spéciale rend {v} des pv max."), {12, 18, 24, 32}, TEXT(" %")},
	{TEXT("o_passif"), Orme, -1, Passif, TEXT("Sève vive"), TEXT("Chaque Haschen abattu rend {v}."), {3, 5, 7, 10}, TEXT(" pv")},
	// Velka, l'orage : vitesse, foudre
	{TEXT("v_attaque"), Velka, -1, Attaque, TEXT("Coups d'orage"), TEXT("Le troisième coup de l'enchaînement appelle un éclair ({v} de l'attaque)."), {60, 80, 100, 130}, TEXT(" %")},
	{TEXT("v_lourde"), Velka, -1, Lourde, TEXT("Foudre en chaîne"), TEXT("L'attaque lourde lance un éclair sur 3 Haschen proches ({v} de l'attaque chacun)."), {50, 70, 90, 120}, TEXT(" %")},
	{TEXT("v_esquive"), Velka, -1, Esquive, TEXT("Bond de l'orage"), TEXT("L'esquive porte plus loin, et revient plus vite (-{v})."), {100, 150, 200, 250}, TEXT(" ms")},
	{TEXT("v_speciale"), Velka, -1, Speciale, TEXT("Pluie d'éclairs"), TEXT("L'attaque spéciale fait tomber la foudre sur 6 Haschen ({v} de l'attaque chacun)."), {80, 110, 140, 180}, TEXT(" %")},
	{TEXT("v_passif"), Velka, -1, Passif, TEXT("Vent"), TEXT("+{v} de vitesse."), {8, 12, 16, 22}, TEXT(" %")},
	// Aurel, l'aube : critiques, chance
	{TEXT("a_attaque"), Aurel, -1, Attaque, TEXT("Coups d'aube"), TEXT("+{v} de chances de critique sur l'enchaînement."), {8, 12, 16, 22}, TEXT(" %")},
	{TEXT("a_lourde"), Aurel, -1, Lourde, TEXT("Éclat révélé"), TEXT("+{v} de chances de critique sur l'attaque lourde."), {25, 40, 55, 75}, TEXT(" %")},
	{TEXT("a_esquive"), Aurel, -1, Esquive, TEXT("Présage d'aube"), TEXT("Après une esquive, le coup suivant a +{v} de chances de critique."), {40, 60, 80, 100}, TEXT(" %")},
	{TEXT("a_speciale"), Aurel, -1, Speciale, TEXT("Lever du jour"), TEXT("L'attaque spéciale porte plus loin (+{v}) et fait toujours un critique."), {20, 30, 40, 55}, TEXT(" %")},
	{TEXT("a_passif"), Aurel, -1, Passif, TEXT("Fortune de l'aube"), TEXT("+{v} d'éclats."), {20, 30, 45, 60}, TEXT(" %")},
	// Les dons doubles (il faut deja un don de chacun des deux maitres)
	{TEXT("d_vapeur"), Brann, Isaure, Passif, TEXT("Vapeur"), TEXT("Un Haschen qui brûle et gèle à la fois explose ({v} de l'attaque)."), {120, 150, 180, 220}, TEXT(" %")},
	{TEXT("d_pluie"), Orme, Velka, Passif, TEXT("Pluie chaude"), TEXT("Chaque Haschen touché par un éclair rend {v}."), {1, 2, 3, 4}, TEXT(" pv")},
	{TEXT("d_radieuse"), Aurel, Brann, Passif, TEXT("Aube radieuse"), TEXT("Les critiques brûlent, et font +{v} de dégâts."), {10, 15, 20, 30}, TEXT(" %")},
};

int32 VespMaitres::NombreDons()
{
	return UE_ARRAY_COUNT(DONS);
}

const FDon& VespMaitres::Don(int32 Index)
{
	return DONS[FMath::Clamp(Index, 0, NombreDons() - 1)];
}

int32 VespMaitres::Index(const TCHAR* Id)
{
	for (int32 i = 0; i < NombreDons(); i++)
	{
		if (FCString::Strcmp(DONS[i].Id, Id) == 0)
		{
			return i;
		}
	}
	return -1;
}

FString VespMaitres::Description(int32 Index, int32 Rarete)
{
	const FDon& D = Don(Index);
	const float V = D.Valeurs[FMath::Clamp(Rarete, 0, Raretes - 1)];
	return FString(D.Aide).Replace(TEXT("{v}"), *(FString::FromInt(FMath::RoundToInt(V)) + D.Unite));
}

// ===================== Les maitres =====================

const TCHAR* VespMaitres::Nom(int32 Maitre)
{
	static const TCHAR* NOMS[Nombre] = {TEXT("Brann"), TEXT("Isaure"), TEXT("Orme"), TEXT("Velka"), TEXT("Aurel")};
	return Maitre >= 0 && Maitre < Nombre ? NOMS[Maitre] : TEXT("Liss");
}

const TCHAR* VespMaitres::Titre(int32 Maitre)
{
	static const TCHAR* TITRES[Nombre] = {TEXT("Maître du Feu"), TEXT("Maîtresse du Givre"), TEXT("Maître de la Sève"), TEXT("Maîtresse de l'Orage"), TEXT("Maître de l'Aube")};
	return Maitre >= 0 && Maitre < Nombre ? TITRES[Maitre] : TEXT("la Fêlure");
}

FLinearColor VespMaitres::Couleur(int32 Maitre)
{
	switch (Maitre)
	{
		case Brann: return FLinearColor(1.0f, 0.5f, 0.2f);
		case Isaure: return FLinearColor(0.55f, 0.82f, 1.0f);
		case Orme: return FLinearColor(0.45f, 0.9f, 0.45f);
		case Velka: return FLinearColor(0.85f, 0.8f, 0.3f);
		case Aurel: return FLinearColor(1.0f, 0.88f, 0.6f);
		default: return FLinearColor(0.8f, 0.35f, 0.65f);		// Liss, la Felure
	}
}

FString VespMaitres::Replique(int32 Maitre)
{
	static const TCHAR* REPLIQUES[Nombre + 1][3] = {
		{TEXT("Tu frappes comme un marteau mal emmanché. Tiens, prends ça."), TEXT("Le feu ne pardonne pas. Moi non plus. Choisis."),
		 TEXT("Vorgath tenait sa masse comme ça, avant. Ne finis pas comme lui.")},
		{TEXT("Respire. Le froid n'est pas pressé. Toi non plus."), TEXT("La glace garde tout. Même les promesses d'Ashka."),
		 TEXT("Un seul geste, bien placé. C'est tout ce qu'il faut.")},
		{TEXT("Aucune vision ne m'avait écouté avant toi."), TEXT("Les racines se souviennent de toutes les visions. Prends ce qu'elles t'offrent."),
		 TEXT("La Matriarche a empoisonné mes bois. Ne la laisse pas faire avec toi.")},
		{TEXT("Plus vite ! Encore plus vite ! Ah, ça c'est une vision !"), TEXT("Le Gardien de Pierre ? Je l'ai déjà fait trembler. À ton tour."),
		 TEXT("Un éclair ne demande jamais la permission.")},
		{TEXT("Je ne vois plus grand-chose. Mais toi, je te vois."), TEXT("J'ai connu l'Oracle quand il marchait encore. Il avait ta lumière."),
		 TEXT("Le jour reviendra. Il faut juste quelqu'un pour le porter.")},
		{TEXT("Tout a un prix. Moi, j'ai payé d'avance."), TEXT("Le Voile ne ment pas. Il prend, puis il rend. Parfois."),
		 TEXT("Tu veux de la force ? Prête-moi un peu de toi.")},
	};
	const int32 M = Maitre >= 0 && Maitre < Nombre ? Maitre : Nombre;
	return REPLIQUES[M][FMath::RandRange(0, 2)];
}

const TCHAR* VespMaitres::NomGeste(int32 Geste)
{
	static const TCHAR* GESTES[] = {TEXT("ATTAQUE"), TEXT("ATTAQUE LOURDE"), TEXT("ESQUIVE"), TEXT("ATTAQUE SPÉCIALE"), TEXT("PASSIF")};
	return GESTES[FMath::Clamp(Geste, 0, 4)];
}

const TCHAR* VespMaitres::NomRarete(int32 Rarete)
{
	static const TCHAR* RARETES[Raretes] = {TEXT("COMMUN"), TEXT("RARE"), TEXT("ÉPIQUE"), TEXT("HÉROÏQUE")};
	return RARETES[FMath::Clamp(Rarete, 0, Raretes - 1)];
}

// ===================== Les pactes de Liss =====================

const TCHAR* VespMaitres::Malediction(int32 M)
{
	static const TCHAR* NOMS[NombreMaledictions] = {TEXT("Sang fragile"), TEXT("Soif"), TEXT("Aveuglement")};
	return NOMS[FMath::Clamp(M, 0, NombreMaledictions - 1)];
}

const TCHAR* VespMaitres::AideMalediction(int32 M)
{
	static const TCHAR* AIDES[NombreMaledictions] = {TEXT("AYLIS encaisse 30 % de dégâts en plus."), TEXT("Les potions et les soins rendent moitié moins."),
	                                                 TEXT("Plus aucun coup critique.")};
	return AIDES[FMath::Clamp(M, 0, NombreMaledictions - 1)];
}

const TCHAR* VespMaitres::Bonus(int32 B)
{
	static const TCHAR* BONUS[NombreBonus] = {TEXT("+30 pv max"), TEXT("+5 attaque"), TEXT("+2 points du Seuil"), TEXT("+150 éclats"),
	                                          TEXT("un don héroïque d'un maître")};
	return BONUS[FMath::Clamp(B, 0, NombreBonus - 1)];
}
