// VespGameMode : met en place un combat. Il cree la grille, AYLIS, et une camera vue de haut, un peu de biais
// (comme dans Hades). Pour l'utiliser : dans un niveau, World Settings > GameMode Override > VespGameMode.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VespGameMode.generated.h"

UCLASS()
class VESPERANCE_API AVespGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVespGameMode();
	virtual void BeginPlay() override;

	// La camera : sa distance, sa hauteur (en degres) et son angle autour de l'arene
	UPROPERTY(EditAnywhere, Category = "Vesperance") float DistanceCamera = 2300.0f;
	UPROPERTY(EditAnywhere, Category = "Vesperance") float ElevationCamera = 55.0f;
	UPROPERTY(EditAnywhere, Category = "Vesperance") float AzimutCamera = -15.0f;
};
