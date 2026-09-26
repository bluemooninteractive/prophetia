// VespUnite : un pion sur la grille (AYLIS ou un Haschen).
//
// Il a ses stats (les memes que le Combattant du prototype), il glisse de case en case le long d'un chemin,
// et il joue ses animations (repos, marche, attaque, coup recu, chute).
// Son apparence : le modele 3D trouve dans son dossier (par exemple /Game/Characters/Aylis),
// ou, s'il n'y en a pas encore, une silhouette simple (un cylindre et une sphere).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespUnite.generated.h"

class AVespGrille;
class UAnimSequence;
class UTextRenderComponent;
class UPointLightComponent;

// Comment un Haschen se bat (comme Style dans le prototype)
UENUM()
enum class EVespStyle : uint8
{
	Melee,		// il s'approche et frappe au contact
	Lanceur,	// il tire de loin (2 a 4 cases), une fois sur deux
	Chargeur,	// il avance de 4 cases d'un coup
};

// Les talents d'un Haschen (on peut en cumuler plusieurs : Soigneur | Invocateur...)
namespace VespCapacite
{
	constexpr int32 Soigneur = 1;		// il soigne ses allies blesses
	constexpr int32 Explosif = 2;		// il explose en tombant (les cases voisines encaissent)
	constexpr int32 Invocateur = 4;		// il appelle des renforts tous les 3 tours
	constexpr int32 Sauteur = 8;		// il bondit a cote d'AYLIS un tour sur deux
	constexpr int32 Vampire = 16;		// ses coups le soignent
	constexpr int32 Attire = 32;		// ses tirs tirent AYLIS vers lui
}

// Ce que ses coups infligent en plus des degats
namespace VespEffetCoup
{
	constexpr int32 Aucun = 0;
	constexpr int32 Poison = 1;
	constexpr int32 Gel = 2;			// la cible ne peut plus bouger a son prochain tour
	constexpr int32 Brulure = 3;
}

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
	UPROPERTY(EditAnywhere) EVespStyle Style = EVespStyle::Melee;
	UPROPERTY(EditAnywhere) int32 Effet = 0;				// VespEffetCoup : poison, gel, brulure
	UPROPERTY(EditAnywhere) int32 Capacites = 0;			// VespCapacite : soigneur, explosif...
	UPROPERTY(EditAnywhere) int32 Armure = 0;				// ses plaques : tant qu'il en reste, les coups font 4 fois moins mal
	UPROPERTY(EditAnywhere) int32 ArmureMax = 0;			// (seul un coup lourd fissure une plaque)
	UPROPERTY(EditAnywhere) int32 Pas = 0;				// cases par tour (0 : selon son style)
	UPROPERTY(EditAnywhere) int32 Portee = 4;				// un lanceur tire jusqu'a cette distance
	UPROPERTY(EditAnywhere) int32 Boss = 0;				// 0 = pas un boss, sinon le numero de l'acte
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
	void Replacer(FIntPoint NouvelleCase);			// au debut d'un nouveau combat

	void Suivre(const TArray<FIntPoint>& Chemin);		// il marche case par case

	// Une arme dans la main (pack StylizedCharacter) : son chemin, sa longueur (en part de la taille du personnage),
	// la main gauche (un arc) ou droite ; et un bouclier dans l'autre main
	void Equiper(const FString& Arme, float Longueur, bool bMainGauche = false, const FString& Bouclier = FString());

	// L'exploration : AYLIS se deplace librement dans le monde (hors de la grille, au clavier ou a la manette)
	void PasserEnModeLibre(bool bLeModeLibre);
	bool EstLibre() const { return bLibre; }
	// Deplacement : le pas de cette image (deja limite au praticable) ; Allure : 1 = marche, plus = course
	void DeplacerLibrement(const FVector& Deplacement, float Allure, float Secondes);
	bool EstEnMarche() const { return !CheminRestant.IsEmpty(); }
	bool EstOccupe() const { return EstEnMarche() || TempsAction > 0.0f; }	// en train de marcher ou d'attaquer
	FIntPoint GetCase() const { return Case; }
	bool EstDebout() const { return Stats.Pv > 0; }
	bool EstAylis() const { return bAylis; }

	// Le combat : il frappe une cible avec une "puissance" (100 = normal, 180 = lourd, 0 = rate), et renvoie les degats
	int32 Frapper(AVespUnite* Cible, int32 Puissance, bool* bCritiqueSortie = nullptr);
	void Encaisser(int32 Degats, bool bCritique);
	void Soigner(int32 Quantite);
	int32 SubirEtats();						// debut de son tour : le poison et la brulure font effet (renvoie les degats)
	void AfficherMessage(const FString& Message, FColor Couleur);	// un petit texte au-dessus de la tete

	FVespStats Stats;
	float ReductionDegats = 1.0f;		// 0.5 quand AYLIS est en garde : les coups font 2 fois moins mal
	int32 Poison = 0;					// les tours de poison qui restent (-3 pv par tour)
	int32 Brulure = 0;					// les tours de brulure qui restent (-4 pv par tour)
	int32 Compteur = 0;					// ses tours (pour les attaques speciales, les sauts, les invocations)
	int32 Gel = 0;						// > 0 : fige par le givre, il ne bouge pas a son prochain tour
	int32 TempsBrise = 0;				// > 0 : son armure vient de voler en eclats, il est sonne
	bool bAExplose = false;
	bool bPhaseDeux = false;			// un boss a moitie de ses pv change de strategie
	int32 Appels = 0;
	bool bVientDArriver = false;		// un renfort : il ne joue pas le tour de son arrivee					// les renforts deja appeles (Ashka)
	void Bondir(FIntPoint Arrivee);		// un saut (ou une faille) : il disparait, et reapparait ailleurs

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
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Texte;	// les degats recus, au-dessus de la tete
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lueur;	// la lumiere de la prophetie (AYLIS) ou les yeux (Haschen)

	UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Pieces;
	UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Armes;
	FName OsDeLaMain(bool bGauche) const;	// les autres morceaux du modele (bras, jambes, tete...)
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
	float TempsTexte = 0.0f;		// > 0 : le texte au-dessus de la tete est encore affiche
	float TempsEclat = 0.0f;		// > 0 : il vient d'etre touche, sa lueur eclate un instant
	float IntensiteLueur = 0.0f;

	// Le "jeu" du corps : ce qui rend chaque coup et chaque pas vivant
	float TempsElan = 0.0f;			// > 0 : il se fend vers sa cible
	FVector DirectionElan = FVector::ZeroVector;
	FVector Recul = FVector::ZeroVector;	// repousse par un coup, il revient doucement a sa place
	float TempsEcrase = 0.0f;		// > 0 : il s'ecrase un instant sous l'impact
	float EchelleModele = 1.0f;
	float DistanceMarche = 0.0f;	// pour le petit rebond a chaque pas
	float TempsVie = 0.0f;
	float DureeTexte = 1.0f;
	FVector DirectionCoup = FVector::ForwardVector;	// d'ou vient le coup qu'il encaisse
	FLinearColor CouleurCoup = FLinearColor::White;
	float RetardImpact = 0.0f;		// le coup vient de loin (fleche, sort) : l'eclat attend qu'il arrive
	bool bLibre = false;			// en exploration : hors de la grille
	bool bMarcheLibre = false;
	float DistancePoussiere = 0.0f;
};
