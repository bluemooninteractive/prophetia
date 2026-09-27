// VespMeteo : le temps qu'il fait dans chaque acte, autour d'AYLIS.
//
//   I   la Foret des Brumes : des feuilles qui tombent doucement
//   II  le Bois des Pendus  : des feuilles mortes, et des averses (avec des eclairs lointains)
//   III les Marais          : une bruine continue
//   IV  la forteresse       : des escarbilles (petites braises) qui montent des braseros, et de la poussiere
//   V   le col gele         : la neige ; pendant le blizzard, elle devient une tempete couchee par le vent
//   VI  les Terres de Cendre: une pluie de cendre grise, et des braises qui montent
//   VII Karn                : des poussieres violettes du Voile qui montent, et des eclairs
//
// Des milliers de petits grains (une seule "instance" de modele repetee), qui vivent dans une boite autour
// d'AYLIS : quand AYLIS avance, ceux qui sortent de la boite reapparaissent de l'autre cote.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespMeteo.generated.h"

class UInstancedStaticMeshComponent;
class ADirectionalLight;

UCLASS()
class VESPERANCE_API AVespMeteo : public AActor
{
	GENERATED_BODY()

public:
	AVespMeteo();
	virtual void Tick(float Secondes) override;

	void Configurer(int32 Acte);
	void Suivre(AActor* LaCible) { Cible = LaCible; }
	void Blizzard(bool bOui) { bBlizzard = bOui; }

private:
	struct FCouche
	{
		UInstancedStaticMeshComponent* Grains = nullptr;
		TArray<FVector> Positions;
		TArray<FVector> Vitesses;
		TArray<float> Phases;
		FVector Vent = FVector::ZeroVector;
		FVector Echelle = FVector(0.05f);
		bool bMonte = false;			// les braises et les poussieres montent au lieu de tomber
		bool bEtire = false;			// la pluie : un trait dans le sens de la chute
		bool bTournoie = false;			// les feuilles : elles tournent en tombant
		float Vitesse = 400.0f;
		float Ondulation = 0.0f;
	};
	void AjouterCouche(UStaticMesh* Modele, UMaterialInterface* Materiau, int32 Nombre, const FVector& Echelle, float Vitesse,
	                   bool bMonte, bool bEtire, bool bTournoie, float Ondulation);
	void Vider();
	void Placer(FCouche& C, int32 i, bool bAuHasard);

	TArray<FCouche> Couches;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Composants;
	UPROPERTY() TObjectPtr<AActor> Cible;
	UPROPERTY() TObjectPtr<ADirectionalLight> Lune;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	int32 Acte = 1;
	bool bBlizzard = false;
	float ForceBlizzard = 0.0f;
	float Temps = 0.0f;
	// L'orage
	bool bEclairs = false;
	float ProchainEclair = 8.0f;
	float Eclair = 0.0f;			// > 0 : le ciel s'illumine
	float Tonnerre = -1.0f;			// > 0 : le tonnerre arrive (le son voyage moins vite que la lumiere)
	float IntensiteLune = 3.0f;
	// Les averses (le Bois des Pendus)
	float Averse = 0.0f;
	float ProchaineAverse = 20.0f;
	bool bAverses = false;
	float ForcePluie = 0.0f;
	static constexpr float DemiBoite = 1700.0f;
	static constexpr float Hauteur = 1300.0f;
};
