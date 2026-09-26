// VespUsagesCommandlet : un outil (pas une partie du jeu) qui prepare les materiaux des packs.
//
// Le jeu pose des milliers d'arbres, d'herbes et de rochers en "instances". Un materiau doit etre autorise pour
// ca (le drapeau "Used with Instanced Static Meshes"). Dans l'editeur, Unreal l'ajoute tout seul au premier
// usage ; en jeu autonome, il ne peut pas, et met un materiau gris a la place.
// Cet outil l'ajoute a tous les materiaux des packs, et les enregistre :
//   UnrealEditor-Cmd.exe Vesperance.uproject -run=VespUsages
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "VespUsagesCommandlet.generated.h"

UCLASS()
class VESPERANCE_API UVespUsagesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Parametres) override;
};
