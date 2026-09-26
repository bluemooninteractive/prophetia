// VespMonde : le monde d'un acte, d'un seul tenant (un "jeu couloir").
//
// Un acte est un reseau de clairieres (les zones) reliees par des sentiers : un sentier principal d'ouest en est
// jusqu'au boss, et des embranchements qui menent a d'autres clairieres (elites, tresors, marchands...).
// Tout le monde est construit d'un coup, au chargement de l'acte : le sol, les sentiers, la foret qui borde
// le couloir (elle sert de mur), les lanternes, et une balise lumineuse au-dessus de chaque clairiere
// (sa couleur et sa lettre annoncent ce qui s'y trouve, comme les portes de Hades).
// AYLIS ne peut marcher que dans les clairieres et sur les sentiers (EstPraticable).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespMonde.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UTextRenderComponent;

// Une clairiere du monde
struct FVespZone
{
	FVector Centre = FVector::ZeroVector;
	float Rayon = 1050.0f;
	int32 Type = 0;					// (int32) EVespSalle
	FString Lettre;
	FLinearColor Couleur = FLinearColor::White;
};

// Un sentier entre deux clairieres
struct FVespCouloir
{
	int32 A = 0;
	int32 B = 0;
};

UCLASS()
class VESPERANCE_API AVespMonde : public AActor
{
	GENERATED_BODY()

public:
	AVespMonde();
	virtual void Tick(float Secondes) override;

	// Construit tout le monde de l'acte (c'est le "chargement")
	void Construire(int32 Acte, const TArray<FVespZone>& LesZones, const TArray<FVespCouloir>& LesCouloirs);

	// Peut-on marcher ici ? (dans une clairiere, ou sur un sentier)
	bool EstPraticable(const FVector& P) const;
	// La distance au bord de ce qui est praticable (negative dedans)
	float DistanceAuPraticable(const FVector& P) const;
	// Le pas de Depuis vers Vers, sans sortir du praticable (on glisse le long des bords)
	FVector Contraindre(const FVector& Depuis, const FVector& Vers) const;

	// Une clairiere visitee : sa balise s'eteint (sauf chez le marchand)
	void MarquerZoneFaite(int32 Zone);
	// Les lettres des balises regardent la camera
	void OrienterTextes(const FVector& Camera);

	static constexpr float DemiLargeurSentier = 320.0f;

private:
	void Vider();
	float DistanceAuxSentiers(const FVector& P) const;
	void AjouterLumiere(const FVector& Position, const FLinearColor& Couleur, float Intensite, float Rayon);
	void AjouterBalise(int32 Index);

	int32 Acte = 1;
	TArray<FVespZone> Zones;
	TArray<FVespCouloir> Couloirs;
	float Temps = 0.0f;

	UPROPERTY() TObjectPtr<UStaticMesh> Plan;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Cone;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylindre;
	UPROPERTY() TObjectPtr<UStaticMesh> Sapin;
	UPROPERTY() TObjectPtr<UStaticMesh> ArbreMort;
	UPROPERTY() TObjectPtr<UStaticMesh> Lanterne;
	UPROPERTY() TObjectPtr<UStaticMesh> Autel;
	bool bVraisArbres = false, bVraisRochers = false, bVraisBuissons = false, bVraiesHerbes = false, bVraiesFleurs = false;
	bool bVraisChampignons = false, bVraisTroncs = false, bVraiesTombes = false;
	bool bPret = false;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvTerre;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvClairieres;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvChemin;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvTaches;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvArbres;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvRochers;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvBuissons;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvHerbes;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvFleurs;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvChampignons;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvTroncs;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvTombes;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvSpecial;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvBlocs;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvLanternes;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvAccessoires;	// buches, coffres, tentes, autels

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurArbres;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurTerre;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurClairieres;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurChemin;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurTaches;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurBuissons;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurHerbes;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurFleurs;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurChampignons;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SpecialMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SpecialLumineux;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurBlocs;

	// Les balises des clairieres, et toutes les lumieres du monde
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Orbes;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> LumieresBalises;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Lettres;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lumieres;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Feux;		// les feux de camp (ils vacillent)
	TArray<float> IntensitesBalises;

	UMaterialInstanceDynamic* Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
	UMaterialInstanceDynamic* Lumineux(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
};
