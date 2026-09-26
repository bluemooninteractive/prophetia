// VespStyles : le style de chaque acte (partage entre le monde et l'arene de combat).
// Les couleurs du monde, la densite de la foret, et son "decor special" :
//   3 = des mares d'eau noire, 4 = des colonnes et des blocs tombes, 5 = des pics de glace et des congeres,
//   6 = des coulees de lave et de l'obsidienne, 7 = des cristaux du Voile et des colonnes brisees.
#pragma once

#include "CoreMinimal.h"

struct FVespStyleActe
{
	FLinearColor Terre, Sol, Herbes, Buissons, Taches, Chemin;
	FLinearColor Fleurs, Champignons, Poison;		// ce qui brille
	float DensiteArbres;							// 1 = une foret dense
	bool bArbresMorts;
	float DensiteHerbes;
	float PartFleurs;
	float DensiteBuissons;
	float DensiteChampignons;
	int32 Special;
	bool bTombes;
};

inline const FVespStyleActe STYLES_ACTES[7] = {
	// I. La Foret des Brumes
	{FLinearColor(0.02f, 0.05f, 0.035f), FLinearColor(0.10f, 0.16f, 0.12f), FLinearColor(0.06f, 0.2f, 0.09f), FLinearColor(0.03f, 0.09f, 0.05f),
	 FLinearColor(0.035f, 0.075f, 0.05f), FLinearColor(0.13f, 0.1f, 0.07f), FLinearColor(0.7f, 0.55f, 1.0f) * 3.0f, FLinearColor(0.3f, 0.9f, 1.0f) * 4.0f,
	 FLinearColor(0.4f, 1.0f, 0.2f) * 1.2f, 1.0f, false, 1.0f, 0.14f, 1.0f, 1.0f, 0, false},
	// II. Le Bois des Pendus
	{FLinearColor(0.03f, 0.035f, 0.022f), FLinearColor(0.12f, 0.13f, 0.09f), FLinearColor(0.12f, 0.13f, 0.06f), FLinearColor(0.07f, 0.06f, 0.04f),
	 FLinearColor(0.06f, 0.06f, 0.035f), FLinearColor(0.12f, 0.1f, 0.07f), FLinearColor(0.9f, 0.9f, 0.6f) * 3.0f, FLinearColor(0.5f, 1.0f, 0.3f) * 4.0f,
	 FLinearColor(0.4f, 1.0f, 0.2f) * 1.2f, 0.9f, true, 0.8f, 0.06f, 0.85f, 1.0f, 0, true},
	// III. Les Marais de Sombreval
	{FLinearColor(0.015f, 0.035f, 0.03f), FLinearColor(0.07f, 0.11f, 0.09f), FLinearColor(0.05f, 0.15f, 0.08f), FLinearColor(0.03f, 0.08f, 0.05f),
	 FLinearColor(0.02f, 0.05f, 0.045f), FLinearColor(0.08f, 0.07f, 0.05f), FLinearColor(0.6f, 1.0f, 0.8f) * 2.5f, FLinearColor(0.6f, 1.0f, 0.4f) * 4.0f,
	 FLinearColor(0.35f, 1.0f, 0.25f) * 1.4f, 0.55f, true, 1.3f, 0.05f, 0.65f, 1.3f, 3, false},
	// IV. La forteresse d'Ashka
	{FLinearColor(0.045f, 0.04f, 0.035f), FLinearColor(0.17f, 0.16f, 0.14f), FLinearColor(0.1f, 0.14f, 0.06f), FLinearColor(0.05f, 0.08f, 0.04f),
	 FLinearColor(0.07f, 0.065f, 0.055f), FLinearColor(0.2f, 0.18f, 0.15f), FLinearColor(1.0f, 0.8f, 0.5f) * 2.0f, FLinearColor(1.0f, 0.7f, 0.3f) * 3.0f,
	 FLinearColor(1.0f, 0.3f, 0.1f) * 1.5f, 0.4f, false, 0.6f, 0.05f, 0.5f, 0.4f, 4, false},
	// V. Le col gele
	{FLinearColor(0.3f, 0.33f, 0.4f), FLinearColor(0.42f, 0.47f, 0.55f), FLinearColor(0.3f, 0.36f, 0.33f), FLinearColor(0.25f, 0.3f, 0.3f),
	 FLinearColor(0.38f, 0.42f, 0.5f), FLinearColor(0.2f, 0.2f, 0.24f), FLinearColor(0.6f, 0.8f, 1.0f) * 2.0f, FLinearColor(0.5f, 0.8f, 1.0f) * 3.0f,
	 FLinearColor(0.5f, 0.8f, 1.0f) * 1.5f, 0.75f, false, 0.35f, 0.03f, 0.45f, 0.4f, 5, false},
	// VI. Les Terres de Cendre
	{FLinearColor(0.025f, 0.015f, 0.015f), FLinearColor(0.11f, 0.07f, 0.06f), FLinearColor(0.1f, 0.04f, 0.03f), FLinearColor(0.06f, 0.03f, 0.02f),
	 FLinearColor(0.05f, 0.02f, 0.015f), FLinearColor(0.05f, 0.035f, 0.03f), FLinearColor(1.0f, 0.5f, 0.2f) * 2.0f, FLinearColor(1.0f, 0.4f, 0.1f) * 3.0f,
	 FLinearColor(1.0f, 0.35f, 0.05f) * 3.0f, 0.25f, true, 0.25f, 0.03f, 0.25f, 0.3f, 6, false},
	// VII. Karn, la cite voilee
	{FLinearColor(0.025f, 0.015f, 0.04f), FLinearColor(0.12f, 0.1f, 0.16f), FLinearColor(0.1f, 0.06f, 0.16f), FLinearColor(0.06f, 0.03f, 0.08f),
	 FLinearColor(0.05f, 0.03f, 0.08f), FLinearColor(0.14f, 0.12f, 0.18f), FLinearColor(0.8f, 0.5f, 1.0f) * 3.0f, FLinearColor(0.6f, 0.3f, 1.0f) * 4.0f,
	 FLinearColor(0.7f, 0.25f, 1.0f) * 2.0f, 0.3f, true, 0.55f, 0.12f, 0.4f, 1.1f, 7, false},
};

inline const FVespStyleActe& StyleDeLActe(int32 Acte)
{
	return STYLES_ACTES[FMath::Clamp(Acte, 1, 7) - 1];
}
