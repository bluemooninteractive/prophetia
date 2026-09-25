// VespGrille : l'arene de combat, une grille de 12 x 8 cases vue du dessus.
//
// C'est la meme idee que dans le prototype 2D (regles.cpp) : chaque case est libre ou bloquee (rocher, arbre...),
// et un "parcours en largeur" calcule les cases qu'AYLIS peut atteindre ce tour-ci.
// Les cases sont dessinees avec des "instances" : un seul modele (un carre plat), repete sur chaque case.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespGrille.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class AVespUnite;

UCLASS()
class VESPERANCE_API AVespGrille : public AActor
{
	GENERATED_BODY()

public:
	AVespGrille();

	// Les dimensions de l'arene (les memes que le prototype)
	static constexpr int32 Colonnes = 12;
	static constexpr int32 Lignes = 8;
	static constexpr float TailleCase = 100.0f;		// en centimetres (l'unite d'Unreal)

	// Passer d'une case a une position dans le monde, et inversement
	FVector CentreDeCase(FIntPoint Case) const;
	bool CaseSousPoint(const FVector& Point, FIntPoint& Case) const;

	bool EstDansArene(FIntPoint Case) const;
	bool EstBloquee(FIntPoint Case) const;

	// Qui est sur quelle case : une unite "occupe" sa case (on ne peut pas passer a travers)
	void Occuper(AVespUnite* Unite);
	void Liberer(AVespUnite* Unite);
	AVespUnite* UniteSur(FIntPoint Case) const;

	// Pour chaque case, le nombre de pas depuis Depart (-1 = inaccessible), sans depasser PasMax
	// (les cases occupees par une unite sont bloquees)
	TArray<int32> CasesAtteignables(FIntPoint Depart, int32 PasMax) const;

	// Le chemin case par case de Depart a Arrivee (vide si impossible), en PasMax pas au plus
	TArray<FIntPoint> Chemin(FIntPoint Depart, FIntPoint Arrivee, int32 PasMax) const;

	// Un Haschen s'approche de sa cible : le chemin (au plus PasMax cases) qui le rapproche le plus d'elle,
	// en contournant les obstacles. Il s'arrete au contact.
	TArray<FIntPoint> ApprocheVers(FIntPoint Depart, FIntPoint Cible, int32 PasMax) const;

	// Allume en bleu les cases accessibles (ou les eteint toutes avec un tableau vide)
	void AfficherCasesAtteignables(const TArray<int32>& Pas);

	// Encadre la case sous la souris (Case hors de l'arene = rien)
	void AfficherSurvol(FIntPoint Case);

protected:
	virtual void BeginPlay() override;

private:
	int32 Index(FIntPoint Case) const { return Case.Y * Colonnes + Case.X; }
	void ConstruireCarte();

	// La carte, une lettre par case : '.' = sol, '#' = rocher, 'T' = arbre
	TArray<TCHAR> Carte;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Occupants;	// une case par position : l'unite qui s'y tient (ou rien)

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Sol;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Rochers;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Arbres;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Accessibles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Survol;

	// Les couleurs : un materiau de base d'Unreal, recolore
	UMaterialInstanceDynamic* Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
};
