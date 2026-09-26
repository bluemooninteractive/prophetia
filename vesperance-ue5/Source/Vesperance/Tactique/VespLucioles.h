// VespLucioles : la magie de la prophetie. Des feux follets (de petites lumieres colorees) qui flottent
// lentement autour d'AYLIS et l'accompagnent sur la route, en decrivant des boucles, et qui palpitent.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespLucioles.generated.h"

class UPointLightComponent;

UCLASS()
class VESPERANCE_API AVespLucioles : public AActor
{
	GENERATED_BODY()

public:
	AVespLucioles();
	virtual void Tick(float Secondes) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FFeuFollet
	{
		FVector Centre;		// le centre de sa boucle
		float Rayon;
		float Vitesse;
		float Phase;
	};
	TArray<FFeuFollet> Feux;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lumieres;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Boules;
	float Temps = 0.0f;
	TWeakObjectPtr<class AVespUnite> Cible;	// AYLIS
};
