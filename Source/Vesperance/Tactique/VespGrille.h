// VespGrille : l'arene de combat, une grille de 12 x 8 cases vue du dessus. Elle apparait dans la clairiere ou
// un combat commence (on la deplace la), et disparait a la fin du combat.
//
// C'est la meme idee que dans le prototype 2D (regles.cpp) : chaque case a un terrain, et un "parcours en largeur"
// calcule les cases qu'AYLIS peut atteindre ce tour-ci. Les terrains :
//   '.' sol    '#' rocher    'T' arbre         (les rochers et les arbres bloquent)
//   'o' boue : on peut y entrer, mais la marche s'arrete la (les marais)
//   'x' eaux toxiques / lave / dechirure du Voile : on y encaisse des degats en finissant son tour dessus
//   '^' dalle piegee : les pointes se levent un tour sur deux (la forteresse)
//   '*' glace : on glisse plus loin que prevu (le col)
//   '@' faille du Voile : elle emporte qui s'y arrete vers une autre faille (Karn)
// Les cases sont dessinees avec des "instances" : un seul modele, repete des milliers de fois.
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
	TCHAR TerrainSur(FIntPoint Case) const;
	bool EstDangereuse(FIntPoint Case) const;		// des eaux toxiques, de la lave, ou une dalle levee
	void ChangerTerrain(FIntPoint Case, TCHAR Terrain);	// la maree monte, la lave coule...
	void LeverPieges(bool bLeves);
	bool PiegesLeves() const { return bPiegesLeves; }
	void Glisser(FIntPoint Depart, TArray<FIntPoint>& Chemin) const;	// la glace prolonge un chemin
	bool AutreFaille(FIntPoint Case, FIntPoint& Sortie) const;
	// Les cases libres (personne dessus, pas un obstacle), a partir d'une colonne ; sans les terrains dangereux
	TArray<FIntPoint> CasesLibres(int32 ColonneMin = 0) const;

	// Qui est sur quelle case : une unite "occupe" sa case (on ne peut pas passer a travers)
	void Occuper(AVespUnite* Unite);
	void Liberer(AVespUnite* Unite);
	AVespUnite* UniteSur(FIntPoint Case) const;

	// Pour chaque case, le nombre de pas depuis Depart (-1 = inaccessible), sans depasser PasMax
	// (les cases occupees par une unite sont bloquees ; la boue arrete la marche)
	TArray<int32> CasesAtteignables(FIntPoint Depart, int32 PasMax) const;

	// Le chemin case par case de Depart a Arrivee (vide si impossible), en PasMax pas au plus
	TArray<FIntPoint> Chemin(FIntPoint Depart, FIntPoint Arrivee, int32 PasMax) const;

	// Un Haschen s'approche de sa cible : le chemin (au plus PasMax cases) qui le rapproche le plus d'elle,
	// en contournant les obstacles et les terrains dangereux. Il s'arrete au contact.
	TArray<FIntPoint> ApprocheVers(FIntPoint Depart, FIntPoint Cible, int32 PasMax) const;

	// Un decor importe dans /Game/Decor : le premier dont le nom contient un des mots (ou rien)
	static UStaticMesh* ModeleDuDecor(std::initializer_list<const TCHAR*> Noms);
	static float EchelleSur(UStaticMesh* Modele, float Hauteur, float LargeurMax);

	// Allume en bleu les cases accessibles (ou les eteint toutes avec un tableau vide)
	void AfficherCasesAtteignables(const TArray<int32>& Pas);
	// Encadre la case sous la souris (Case hors de l'arene = rien)
	void AfficherSurvol(FIntPoint Case);
	// Les cases rouges d'une attaque annoncee (elles exploseront au prochain tour)
	void AfficherDanger(const TArray<FIntPoint>& Cases);

	// Une carte dessinee a la main (0 a 3 la foret, 4 le cercle de Skarn, 5 a 8 le Bois des Pendus, 9 la Matriarche)
	void ChangerCarte(int32 Numero);
	// La carte d'un combat : dessinee a la main (actes I et II) ou inventee, avec les terrains de l'acte
	void PreparerCarte(int32 LActe, bool bBoss);
	void ViderOccupants();
	void Effacer();		// la fin du combat : l'arene disparait
	int32 GetActe() const { return Acte; }

protected:
	virtual void BeginPlay() override;

private:
	int32 Index(FIntPoint Case) const { return Case.Y * Colonnes + Case.X; }
	void RemplirDepuis(int32 Numero);
	bool GenererCarte(int32 LActe, bool bBoss);
	void ConstruireCarte();
	void ConstruireTerrain();

	// La carte, une lettre par case
	TArray<TCHAR> Carte;
	int32 Acte = 1;
	bool bPiegesLeves = false;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Occupants;	// une case par position : l'unite qui s'y tient (ou rien)

	// Les modeles
	UPROPERTY() TObjectPtr<UStaticMesh> Sapin;			// les arbres vivants
	UPROPERTY() TObjectPtr<UStaticMesh> ArbreMort;		// les arbres morts
	UPROPERTY() TObjectPtr<UStaticMesh> Plan;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Cone;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylindre;
	bool bVraisArbres = false, bVraisRochers = false;

	// L'arene
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Sol;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Rochers;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Arbres;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Accessibles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Survol;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Danger;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TerrBoue;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TerrPoison;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TerrPieges;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TerrGlace;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TerrFailles;

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurSol;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurArbres;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CouleurPoison;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PiegeBaisse;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PiegeLeve;

	UMaterialInstanceDynamic* Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
	UMaterialInstanceDynamic* Lumineux(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte);
};
