// VespSauvegarde : la memoire de la boucle, qui survit a la mort d'AYLIS.
//
// Chaque partie est une nouvelle "vision" envoyee par la prophetie. Le monde s'en souvient : le numero de la vision,
// ce que chaque gardien a fait a AYLIS (et subi), et les temps (par acte, par partie). Rien de ce qu'AYLIS porte
// (niveau, objets, etoiles du Seuil) ne passe d'une vision a l'autre : seulement la memoire, et les Souvenirs
// gagnes en route, que le Veilleur echange entre deux visions contre des dons qui durent (VespVeilleur).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VespSauvegarde.generated.h"

UCLASS()
class VESPERANCE_API UVespSauvegarde : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 NombreDActes = 7;
	static constexpr int32 NombreDeDons = 7;

	// Le numero de la derniere vision lancee (0 : jamais joue)
	UPROPERTY() int32 Visions = 0;
	// Les parties allees au bout
	UPROPERTY() int32 Victoires = 0;
	// Par gardien (un par acte) : combien de fois il a tue AYLIS, et s'il a deja ete vaincu
	UPROPERTY() TArray<int32> MortsParGardien;
	UPROPERTY() TArray<bool> GardiensVaincus;
	// Ou est tombee la derniere vision (0 : aucune)
	UPROPERTY() int32 DerniereChuteActe = 0;
	UPROPERTY() bool bDerniereChuteBoss = false;
	// Les temps, en secondes : le meilleur pour chaque acte, ceux de la derniere partie, le meilleur temps d'une partie gagnee
	UPROPERTY() TArray<float> MeilleursActes;
	UPROPERTY() TArray<float> DerniersActes;
	UPROPERTY() float MeilleurePartie = 0.0f;
	UPROPERTY() float TempsDeJeu = 0.0f;		// tout confondu, depuis la premiere vision
	// Ce qui survit vraiment a la mort : les Souvenirs (gagnes en route, jamais perdus) et le rang de chaque don du Veilleur
	UPROPERTY() int32 Souvenirs = 0;
	UPROPERTY() int32 SouvenirsGagnes = 0;		// depuis la premiere vision
	UPROPERTY() TArray<int32> Dons;

	// Les tableaux ont toujours une case par acte (une vieille sauvegarde peut en avoir moins)
	void Completer()
	{
		MortsParGardien.SetNum(NombreDActes);
		GardiensVaincus.SetNum(NombreDActes);
		MeilleursActes.SetNum(NombreDActes);
		DerniersActes.SetNum(NombreDActes);
		Dons.SetNum(NombreDeDons);
	}

	int32 Don(int32 Index) const { return Dons.IsValidIndex(Index) ? Dons[Index] : 0; }

	int32 GardiensDejaVaincus() const
	{
		int32 N = 0;
		for (bool b : GardiensVaincus)
		{
			N += b ? 1 : 0;
		}
		return N;
	}

	// 754 secondes -> "12:34" ; au-dela d'une heure -> "1:02:34"
	static FString Duree(float Secondes)
	{
		const int32 S = FMath::Max(0, FMath::FloorToInt(Secondes));
		return S >= 3600 ? FString::Printf(TEXT("%d:%02d:%02d"), S / 3600, (S / 60) % 60, S % 60) : FString::Printf(TEXT("%d:%02d"), S / 60, S % 60);
	}
};
