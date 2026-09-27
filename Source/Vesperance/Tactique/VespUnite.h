// VespUnite : un personnage du monde (AYLIS ou un Haschen), en temps reel.
//
// Il se deplace librement (sans sortir de ce qui est praticable), joue ses gestes (attaques, esquives, garde,
// coups recus, chute...) grace a VespAnim, encaisse les coups (recul, eclat de lumiere, chiffre au-dessus de
// la tete) et subit les etats (poison, brulure, gel) seconde apres seconde.
// Ce qu'il DECIDE de faire ne vient pas de lui : le joueur dirige AYLIS (VespPlayerController), et le combat
// (VespCombat) fait penser les Haschen.
// Son apparence : le modele 3D trouve dans son dossier (par exemple /Game/Characters/Aylis), avec ses animations
// KayKit ; ou, s'il n'y en a pas, une silhouette simple (un cylindre et une sphere).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespUnite.generated.h"

class AVespMonde;
class UAnimSequence;
class UTextRenderComponent;
class UPointLightComponent;
class UVespAnimInstance;
class UMaterialInstanceDynamic;

// Comment un Haschen se bat
UENUM()
enum class EVespStyle : uint8
{
	Melee,		// il s'approche et frappe au contact
	Lanceur,	// il tire de loin (fleches, sorts), et recule si on l'approche
	Chargeur,	// il fonce en ligne droite
};

// Les talents d'un Haschen (on peut en cumuler plusieurs : Soigneur | Invocateur...)
namespace VespCapacite
{
	constexpr int32 Soigneur = 1;		// il soigne ses allies blesses
	constexpr int32 Explosif = 2;		// il explose en tombant (tout ce qui est autour encaisse)
	constexpr int32 Invocateur = 4;		// il appelle des renforts
	constexpr int32 Sauteur = 8;		// il bondit sur AYLIS
	constexpr int32 Vampire = 16;		// ses coups le soignent
	constexpr int32 Attire = 32;		// ses tirs tirent AYLIS vers lui
}

// Ce que ses coups infligent en plus des degats
namespace VespEffetCoup
{
	constexpr int32 Aucun = 0;
	constexpr int32 Poison = 1;
	constexpr int32 Gel = 2;
	constexpr int32 Brulure = 3;
}

// Les stats d'un combattant
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
	UPROPERTY(EditAnywhere) int32 Pas = 0;				// un bonus de vitesse (0 : normal)
	UPROPERTY(EditAnywhere) int32 Portee = 4;				// un tireur tire jusqu'a Portee x 1,7 m
	UPROPERTY(EditAnywhere) int32 Boss = 0;				// 0 = pas un boss, sinon le numero de l'acte
};

// Ce qu'il tient en main : ca choisit ses animations d'attaque
UENUM()
enum class EVespArme : uint8
{
	Poings,
	Epee,			// une main (epee, hache, masse)
	Dagues,			// une dans chaque main : des coups rapides
	DeuxMains,		// epee ou hache a deux mains : lent, large, lourd
	Baton,			// les sorts
	Arc,
};

// Les gestes qu'il sait faire
UENUM()
enum class EVespGeste : uint8
{
	Attaque1, Attaque2, Attaque3,	// l'enchainement
	Lourde,
	Tourbillon,						// une attaque tout autour de soi
	Tir,
	Sort,
	Invocation,
	Lancer,
	EsquiveAvant, EsquiveArriere, EsquiveGauche, EsquiveDroite,
	GardeLevee,						// il leve le bouclier
	GardeTouchee,					// un coup frappe le bouclier
	Riposte,
	Touche,
	Mort,
	Potion,
	Ramasser,
	Interagir,
	Victoire,
	Resurrection,					// un Haschen qui sort de terre
	Saut,
};

UCLASS()
class VESPERANCE_API AVespUnite : public AActor
{
	GENERATED_BODY()

public:
	AVespUnite();
	virtual void Tick(float Secondes) override;

	// Mise en place : ses stats, son dossier de modele 3D, sa couleur de secours
	void Preparer(AVespMonde* LeMonde, const FVespStats& LesStats, const FString& Dossier, FLinearColor Teinte, bool bEstAylis);

	// ----- Le mouvement -----
	// Un pas (vitesse en cm/s dans le plan) : il ne sort jamais de ce qui est praticable. Il se tourne vers ou il va
	// (sauf s'il vise ailleurs : SetRegard). Renvoie le deplacement vraiment fait.
	FVector Deplacer(const FVector& Vitesse, float Secondes, bool bTourner = true);
	void Tourner(const FVector& Direction, float Secondes, float Vivacite = 14.0f);
	void TournerDUnCoup(const FVector& Direction);
	FVector Avant() const { return GetActorForwardVector(); }
	void Teleporter(const FVector& Position);
	// Une poussee (un coup, une explosion) : il glisse, puis s'arrete
	void Pousser(const FVector& Elan) { Poussee += FVector(Elan.X, Elan.Y, 0.0f); }
	FVector GetPoussee() const { return Poussee; }
	// Un saut en arc de cercle (les sauteurs, les boss) : il arrive en Duree secondes
	void Bondir(const FVector& Arrivee, float Duree, float Hauteur);
	bool EstEnLAir() const { return TempsSaut < DureeSaut; }

	// ----- Les gestes -----
	float Jouer(EVespGeste Geste, float Vitesse = 1.0f, bool bHautDuCorps = false, bool bTenir = false);
	void ArreterGeste();
	float DureeGeste(EVespGeste Geste) const;		// sa duree a vitesse normale (0.35 s sans animation)
	void AccelererGeste(float Vitesse);				// change la vitesse du geste en cours (apres l'impact : plus vif)
	bool GesteEnCours() const;
	float ProgressionGeste() const;
	void TenirGarde(bool bLevee);				// la posture de garde (bouclier leve), sur le haut du corps
	bool ADesAnimations() const { return bAnime; }

	// ----- Les armes -----
	// Une arme dans la main (pack StylizedCharacter) : son chemin, sa longueur (en part de la taille du personnage),
	// la main gauche (un arc) ou droite ; et un bouclier dans l'autre main. Remplace ce qu'il tenait.
	void Equiper(const FString& Arme, float Longueur, bool bMainGauche = false, const FString& Bouclier = FString(), EVespArme Type = EVespArme::Epee);
	void EquiperSecondeArme(const FString& Arme, float Longueur);		// une dague dans l'autre main
	void Desarmer();
	// La couleur d'une arme tenue (les variantes du pack : "Cl", "Bl", "Gn", "Rd")
	void ColorerArme(const FString& Modele, const TCHAR* Suffixe);
	EVespArme GetTypeArme() const { return TypeArme; }
	bool ABouclier() const { return bBouclier; }
	// La couleur de sa tenue (AYLIS : l'armure portee change la teinte de la cape et du capuchon)
	void TeindreTenue(const FLinearColor& Couleur, float Force = 1.0f);

	// ----- Les coups -----
	// Il encaisse : le recul (Direction x Poussee), l'eclat, le chiffre. bSansFlechir : il ne bronche pas (boss, armure).
	void Encaisser(int32 Degats, bool bCritique, const FVector& Direction, float Poussee, bool bSansFlechir = false);
	void Soigner(int32 Quantite);
	void AfficherMessage(const FString& Message, FColor Couleur, float Taille = 38.0f);
	// Il se prepare a frapper : ses yeux rougeoient (une attaque annoncee), pendant Duree secondes
	void Annoncer(float Duree, const FLinearColor& Couleur = FLinearColor(1.0f, 0.15f, 0.05f));
	void Surbrillance(float Duree, const FLinearColor& Couleur);		// un eclat (un soin, une rune, un coup)
	bool EstDebout() const { return Stats.Pv > 0; }
	bool EstAylis() const { return bAylis; }
	float TempsDepuisMort() const { return TempsMort; }
	float GetVitesseActuelle() const { return VitesseActuelle; }

	// Les etats, en secondes
	void Empoisonner(float Duree) { Poison = FMath::Max(Poison, Duree); }
	void Bruler(float Duree) { Brulure = FMath::Max(Brulure, Duree); }
	void Geler(float Duree);			// ralenti ; s'il l'etait deja, il est fige un instant
	float Ralentissement() const;		// 1 = normal, 0.5 = gele...

	FVespStats Stats;
	float Poison = 0.0f;				// secondes de poison qui restent (-2 pv par seconde)
	float Brulure = 0.0f;				// secondes de brulure (-3 pv par seconde)
	float Gel = 0.0f;					// secondes de gel (ralenti de moitie)
	float Fige = 0.0f;					// secondes pendant lesquelles il ne bouge plus du tout
	float Etourdi = 0.0f;				// secondes d'etourdissement (armure brisee, parade...)
	float Invulnerable = 0.0f;			// secondes d'invulnerabilite (l'esquive d'AYLIS)
	float ReductionDegats = 1.0f;		// la garde : les coups font moins mal
	float Equilibre = 0.0f;				// ce qui s'accumule a chaque coup ; au-dela du seuil, il flechit
	float SeuilEquilibre = 0.0f;		// 0 : il flechit a chaque coup ; les grands et les boss tiennent plus longtemps
	float TempsBrise = 0.0f;			// > 0 : son armure vient de voler en eclats, il est sonne
	bool bAExplose = false;
	bool bPhaseDeux = false;			// un boss a moitie de ses pv change de strategie
	float DegatsRecus = 0.0f;			// pour les barres de vie : le morceau blanc qui s'efface derriere la vraie vie

	UPROPERTY(EditAnywhere, Category = "Vesperance") float Vitesse = 380.0f;			// cm par seconde
	UPROPERTY(EditAnywhere, Category = "Vesperance") float Taille = 170.0f;			// la hauteur du modele, en cm
	UPROPERTY(EditAnywhere, Category = "Vesperance") float CorrectionRotation = -90.0f;	// si le modele regarde de cote
	float Rayon() const { return Taille * 0.22f; }			// sa "place" au sol (pour ne pas se marcher dessus)

protected:
	virtual void BeginPlay() override;

private:
	void Habiller(const FString& Dossier);						// cherche le modele et ses animations
	const UAnimSequence* Anim(std::initializer_list<const TCHAR*> Noms) const;
	const UAnimSequence* AnimDuGeste(EVespGeste Geste) const;
	void MettreEnPlaceLocomotion();
	void Mourir();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Modele;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Corps;	// la silhouette de secours
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Tete;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Texte;	// les degats recus, au-dessus de la tete
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lueur;	// la lumiere de la prophetie (AYLIS) ou les yeux (Haschen)

	UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Pieces;		// les autres morceaux du modele (bras, jambes, tete...)
	UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Armes;
	UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Animations;				// toutes celles de son dossier
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Tenue;		// les materiaux teintables (AYLIS)
	mutable TMap<FString, TObjectPtr<UAnimSequence>> AnimsTrouvees;
	FName OsDeLaMain(bool bGauche) const;
	UVespAnimInstance* Animateur() const;

	UPROPERTY() TObjectPtr<AVespMonde> Monde;
	bool bAylis = false;
	bool bAnime = false;
	bool bBouclier = false;
	EVespArme TypeArme = EVespArme::Poings;
	float TempsTexte = 0.0f;		// > 0 : le texte au-dessus de la tete est encore affiche
	float DureeTexte = 1.0f;
	float TempsEclat = 0.0f;		// > 0 : il vient d'etre touche, sa lueur eclate un instant
	FLinearColor CouleurEclat = FLinearColor::White;
	float TempsAnnonce = 0.0f;		// > 0 : il prepare une attaque
	FLinearColor CouleurAnnonce = FLinearColor::Red;
	float IntensiteLueur = 0.0f;
	FLinearColor CouleurLueur = FLinearColor::White;
	float TempsEcrase = 0.0f;		// > 0 : il s'ecrase un instant sous l'impact
	float EchelleModele = 1.0f;
	float TempsVie = 0.0f;
	float TempsMort = 0.0f;
	float TicEtats = 0.0f;			// les degats des etats tombent une fois par seconde
	FVector Poussee = FVector::ZeroVector;
	float VitesseActuelle = 0.0f;
	FVector DernierePosition = FVector::ZeroVector;
	float DistancePoussiere = 0.0f;
	FVector DirectionCoup = FVector::ForwardVector;
	// Le saut
	FVector DepartSaut = FVector::ZeroVector, ArriveeSaut = FVector::ZeroVector;
	float TempsSaut = 1.0f, DureeSaut = 0.0f, HauteurSaut = 0.0f;
};
