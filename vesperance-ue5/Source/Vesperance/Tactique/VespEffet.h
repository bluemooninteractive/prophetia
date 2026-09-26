// VespEffet : les effets visuels (des "particules" faites a la main).
//
// Chaque effet est un petit acteur qui lance des grains lumineux (des spheres qui brillent), les fait voler,
// retomber et s'eteindre, avec parfois un eclair de lumiere, puis disparait tout seul.
//   Impact, Critique : l'eclat d'un coup qui touche
//   Trainee          : l'arc d'une lame devant l'attaquant
//   Projectile       : une fleche ou un sort qui vole d'un point a un autre
//   Poussiere        : un nuage sous les pas
//   Soin, Poison     : des bulles qui montent
//   Mort             : une ame qui s'echappe
//   Onde             : un cercle qui s'elargit au sol (un choc)
//   Etincelles       : une gerbe (une rune, un achat)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespEffet.generated.h"

class UPointLightComponent;
class UMaterialInstanceDynamic;

UENUM()
enum class EVespEffet : uint8
{
	Impact,
	Critique,
	Trainee,
	Projectile,
	Poussiere,
	Soin,
	Poison,
	Mort,
	Onde,
	Etincelles,
};

UCLASS()
class VESPERANCE_API AVespEffet : public AActor
{
	GENERATED_BODY()

public:
	AVespEffet();
	virtual void Tick(float Secondes) override;

	// Lance un effet. Direction : vers ou il part (la cible d'un coup). Arrivee : pour un projectile.
	// Retard : l'effet attend un peu avant de commencer (un impact qui attend que la fleche arrive).
	static void Jouer(UWorld* Monde, EVespEffet Type, FVector Position, FVector Direction = FVector::UpVector,
	                  FLinearColor Couleur = FLinearColor::White, float Retard = 0.0f, FVector Arrivee = FVector::ZeroVector);

	// Un materiau qui brille (additif, sans ombre), avec un parametre "Color". Partage par tout le jeu.
	static UMaterialInterface* MateriauLumineux();

private:
	struct FGrain
	{
		UStaticMeshComponent* Mesh = nullptr;
		UMaterialInstanceDynamic* Materiau = nullptr;
		FVector Vitesse = FVector::ZeroVector;
		float Retard = 0.0f;
		float Vie = 0.0f;
		float VieMax = 0.5f;
		float TailleDebut = 0.1f;
		float TailleFin = 0.0f;
		float Gravite = 0.0f;
		float Frottement = 0.0f;
		FVector Etirement = FVector(1.0f);	// > 1 en X : un trait allonge dans le sens de sa vitesse
	};

	void Ajouter(const FVector& Position, const FVector& Vitesse, float VieMax, float TailleDebut, float TailleFin,
	             float Gravite = 0.0f, float Retard = 0.0f, float Frottement = 0.0f, FVector Etirement = FVector(1.0f));
	void Eclairer(float Intensite, float Rayon, float Duree, float Retard = 0.0f, FVector Vitesse = FVector::ZeroVector);

	TArray<FGrain> Grains;
	FLinearColor Couleur = FLinearColor::White;
	float Intensite = 6.0f;				// l'eclat des grains (au-dessus de 1 : le "bloom" les fait briller)
	UPROPERTY() TObjectPtr<UPointLightComponent> Lumiere;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
	float LumiereMax = 0.0f;
	float LumiereDuree = 0.0f;
	float LumiereRetard = 0.0f;
	float LumiereTemps = 0.0f;
	FVector LumiereVitesse = FVector::ZeroVector;
	float Temps = 0.0f;
};
