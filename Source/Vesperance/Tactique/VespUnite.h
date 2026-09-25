// VespUnite : un pion sur la grille (AYLIS ou un Haschen).
//
// Il a ses stats (les memes que le Combattant du prototype), il glisse de case en case le long d'un chemin,
// et il joue ses animations (repos, marche, attaque, coup recu, chute).
// Son apparence : le modele 3D trouve dans son dossier (par exemple /Game/Vesperance/Personnages/Aylis),
// ou, s'il n'y en a pas encore, une silhouette simple (un cylindre et une sphere).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespUnite.generated.h"

class AVespGrille;
class UAnimSequence;
class UTextRenderComponent;
class UPointLightComponent;

// Les stats d'un combattant, comme dans le prototype (types.h)
USTRUCT(BlueprintType)
struct FVespStats
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) FString Nom = TEXT("Haschen");
	UPROPERTY(EditAnywhere) int32 Pv = 20;
	UPROPERTY(EditAnywhere) int32 PvMax = 20;
	UPROPERTY(EditAnywhere) int32 Attaque = 9;
	UPROPERTY(EditAnywhere) int32 Defense = 1;
	UPROPERTY(EditAnywhere) int32 ChanceCritique = 10;	// sur 100
};

UCLASS()
class VESPERANCE_API AVespUnite : public AActor
{
	GENERATED_BODY()

public:
	AVespUnite();
	virtual void Tick(float Secondes) override;

	// Mise en place : sa grille, sa case, ses stats, son dossier de modele 3D, sa couleur de secours
	void Preparer(AVespGrille* LaGrille, FIntPoint Case, const FVespStats& LesStats, const FString& Dossier, FLinearColor Teinte, bool bEstAylis);

	void Suivre(const TArray<FIntPoint>& Chemin);		// il marche case par case
	bool EstEnMarche() const { return !CheminRestant.IsEmpty(); }
	bool EstOccupe() const { return EstEnMarche() || TempsAction > 0.0f; }	// en train de marcher ou d'attaquer
	FIntPoint GetCase() const { return Case; }
	bool EstDebout() const { return Stats.Pv > 0; }
	bool EstAylis() const { return bAylis; }

	// Le combat : il frappe une cible avec une "puissance" (100 = normal, 180 = lourd, 0 = rate), et renvoie les degats
	int32 Frapper(AVespUnite* Cible, int32 Puissance);
	void Encaisser(int32 Degats, bool bCritique);

	FVespStats Stats;
	float ReductionDegats = 1.0f;		// 0.5 quand AYLIS est en garde : les coups font 2 fois moins mal

	UPROPERTY(EditAnywhere, Category = "Vesperance") float Vitesse = 380.0f;			// cm par seconde
	UPROPERTY(EditAnywhere, Category = "Vesperance") float Taille = 170.0f;			// la hauteur du modele, en cm
	UPROPERTY(EditAnywhere, Category = "Vesperance") float CorrectionRotation = -90.0f;	// si le modele regarde de cote

protected:
	virtual void BeginPlay() override;

private:
	void Habiller(const FString& Dossier);						// cherche le modele et ses animations
	void Jouer(UAnimSequence* Animation, bool bEnBoucle);
	void Regarder(const FVector& Point);
	void MettreAJourTexte();
	void TeindreEnBleu();			// AYLIS : le vert du modele KayKit devient bleu nuit

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Modele;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Corps;	// la silhouette de secours
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Tete;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Texte;	// les pv au-dessus de la tete
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lueur;	// la lumiere de la prophetie (AYLIS) ou les yeux (Haschen)

	UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Pieces;	// les autres morceaux du modele (bras, jambes, tete...)
	UPROPERTY() TObjectPtr<UAnimSequence> AnimRepos;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimMarche;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimAttaque;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimTouche;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimChute;

	UPROPERTY() TObjectPtr<AVespGrille> Grille;
	FIntPoint Case;
	TArray<FIntPoint> CheminRestant;
	bool bAylis = false;
	float TempsAction = 0.0f;		// > 0 : une attaque ou un coup recu est en train de se jouer
	float TempsTexte = 0.0f;		// > 0 : le texte affiche les degats recus, au lieu des pv
};
