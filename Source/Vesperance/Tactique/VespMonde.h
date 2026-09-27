// VespMonde : le monde d'un acte, d'un seul tenant (un "jeu couloir").
//
// Un acte est un reseau de clairieres (les zones) reliees par des sentiers : un sentier principal d'ouest en est
// jusqu'au boss, et des embranchements qui menent a d'autres clairieres (elites, tresors, marchands...).
// Tout le monde est construit d'un coup, au chargement de l'acte : le sol, les sentiers, la foret qui borde
// le couloir (elle sert de mur), les bougies le long du chemin, et une balise lumineuse au-dessus de chaque
// clairiere (sa couleur et sa lettre annoncent ce qui s'y trouve, comme les portes de Hades).
//
// La direction artistique : tout le monde "sain" vient des packs StylizedProvencal et Fantastic_Village_Pack
// (peints a la main, comme les personnages KayKit). Plus AYLIS approche du Voile (actes VI et VII), plus le monde se corrompt : on y melange
// les elements organiques et inquietants du pack Planet385CY (tentacules, cocons, yeux, insectes).
// Chaque acte a sa "palette" : quels arbres, quels rochers, quels decors (voir PaletteDeLActe).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespMonde.generated.h"

class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UTextRenderComponent;
class USkeletalMeshComponent;

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
	// Pendant un combat : la balise de la clairiere se cache (elle flotterait au milieu de l'arene)
	void CacherBalise(int32 Zone, bool bCachee);
	// Les lettres des balises regardent la camera
	void OrienterTextes(const FVector& Camera);

	// Un modele d'un pack, par son chemin (null s'il n'est pas dans le projet)
	static UStaticMesh* Modele(const FString& Chemin);
	// L'echelle pour qu'un modele fasse Hauteur cm de haut, sans depasser LargeurMax cm de large
	static float EchelleSur(UStaticMesh* M, float Hauteur, float LargeurMax);

	static constexpr float DemiLargeurSentier = 320.0f;

private:
	void Vider();
	float DistanceAuxSentiers(const FVector& P) const;
	void AjouterLumiere(const FVector& Position, const FLinearColor& Couleur, float Intensite, float Rayon);
	void AjouterBalise(int32 Index);
	void AjouterFlamme(const FVector& Position, float Echelle);
	// Le "catalogue" : un composant d'instances par modele, cree a la demande
	UHierarchicalInstancedStaticMeshComponent* Instances(UStaticMesh* M, bool bOmbre, float DistanceMax);
	void Poser(UStaticMesh* M, const FVector& Pied, float Hauteur, float LargeurMax, bool bOmbre = true, float DistanceMax = 16000.0f,
	           float Inclinaison = 0.0f, float Yaw = -1.0f);

	int32 Acte = 1;
	TArray<FVespZone> Zones;
	TArray<FVespCouloir> Couloirs;
	float Temps = 0.0f;
	FRandomStream Hasard;

	UPROPERTY() TObjectPtr<UStaticMesh> Plan;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Cone;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylindre;
	bool bPret = false;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvTerre;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvClairieres;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvChemin;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvTaches;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvChampignons;	// les champignons luminescents (la magie)
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EnvLueurs;		// ce qui brille : glace, lave, cristaux
	UPROPERTY() TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Catalogue;
	UPROPERTY() TMap<TObjectPtr<UMaterialInterface>, TObjectPtr<UMaterialInstanceDynamic>> Feuillages;	// le feuillage teint pour l'acte

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurTerre;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurClairieres;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurChemin;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurTaches;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurChampignons;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurLueurs;
	UPROPERTY() TObjectPtr<UMaterialInterface> SolPeint;		// le sol herbeux peint du pack (actes I a IV)
	// Le vrai sol : trois matieres (herbe, terre, et celle de l'acte) melangees par une carte peinte
	// d'apres les sentiers et les clairieres (la terre battue des chemins, l'herbe au bord, les taches de l'acte)
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SolVivant;
	UPROPERTY() TObjectPtr<class UTexture2D> CarteDuSol;
	void PeindreLeSol(const FBox2D& Limites);
	static UMaterialInterface* MateriauDuSol();
	UPROPERTY() TObjectPtr<UMaterialInterface> EauPeinte;		// l'eau peinte du pack (mares, ruisseaux)

	// Les balises des clairieres, et toutes les lumieres du monde
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Orbes;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> LumieresBalises;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Lettres;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lumieres;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Feux;		// les feux de camp (ils vacillent)
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Eaux;		// les mares (un materiau d'eau par plan)
	UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Insectes;	// les insectes du Voile (acte VII)
	UPROPERTY() TArray<TObjectPtr<AActor>> Maisons;						// les maisons du village (des blueprints du pack)
	UPROPERTY() TArray<TObjectPtr<class UParticleSystemComponent>> Flammes;	// le feu des braseros et des feux de camp
	TArray<FVector> CentresInsectes;
	TArray<float> IntensitesBalises;

	UMaterialInstanceDynamic* Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
	UMaterialInstanceDynamic* Lumineux(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
};
