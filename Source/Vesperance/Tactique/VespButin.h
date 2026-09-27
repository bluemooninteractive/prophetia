// VespButin : un objet tombe au sol. Un faisceau de lumiere de la couleur de sa rarete le signale de loin
// (gris, bleu, violet, or), et l'objet tourne doucement au-dessus du sol. AYLIS le ramasse en passant dessus.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespProgression.h"
#include "VespButin.generated.h"

class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class VESPERANCE_API AVespButin : public AActor
{
	GENERATED_BODY()

public:
	AVespButin();
	virtual void Tick(float Secondes) override;

	void Preparer(const FVespObjet& LObjet);
	FVespObjet Objet;
	float Age = 0.0f;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Faisceau;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Socle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gemme;		// les bijoux et armures : une gemme de lumiere
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Arme;	// les armes et boucliers : le vrai modele
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lumiere;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Couleur;
	FVector Depart = FVector::ZeroVector;
	FVector Vol = FVector::ZeroVector;		// il jaillit du Haschen, puis retombe
};
