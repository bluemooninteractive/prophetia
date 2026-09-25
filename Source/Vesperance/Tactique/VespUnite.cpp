#include "VespUnite.h"
#include "VespGrille.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AVespUnite::AVespUnite()
{
	PrimaryActorTick.bCanEverTick = true;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;

	Modele = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Modele"));
	Modele->SetupAttachment(Racine);
	Modele->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// La silhouette de secours : un cylindre (le corps) et une sphere (la tete)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylindre(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Corps = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Corps"));
	Corps->SetupAttachment(Racine);
	Corps->SetStaticMesh(Cylindre.Object);
	Corps->SetRelativeLocation(FVector(0, 0, 50));
	Corps->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.0f));
	Corps->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tete = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tete"));
	Tete->SetupAttachment(Racine);
	Tete->SetStaticMesh(Sphere.Object);
	Tete->SetRelativeLocation(FVector(0, 0, 120));
	Tete->SetRelativeScale3D(FVector(0.42f));
	Tete->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Lueur = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lueur"));
	Lueur->SetupAttachment(Racine);
	Lueur->SetRelativeLocation(FVector(0, 0, 150));
	Lueur->SetAttenuationRadius(420.0f);
	Lueur->SetCastShadows(false);

	Texte = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Texte"));
	Texte->SetupAttachment(Racine);
	Texte->SetRelativeLocation(FVector(0, 0, 215));
	Texte->SetHorizontalAlignment(EHTA_Center);
	Texte->SetWorldSize(34.0f);
}

void AVespUnite::BeginPlay()
{
	Super::BeginPlay();
}

void AVespUnite::Preparer(AVespGrille* LaGrille, FIntPoint NouvelleCase, const FVespStats& LesStats, const FString& Dossier,
                          FLinearColor Teinte, bool bEstAylis)
{
	Grille = LaGrille;
	Case = NouvelleCase;
	Stats = LesStats;
	bAylis = bEstAylis;
	SetActorLocation(Grille->CentreDeCase(Case));
	Grille->Occuper(this);

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Teinte);
	Corps->SetMaterial(0, Materiau);
	Tete->SetMaterial(0, Materiau);
	Texte->SetTextRenderColor(bAylis ? FColor(120, 180, 255) : FColor(255, 110, 90));
	// AYLIS porte la lumiere violette de la prophetie ; les Haschen, une faible lueur rouge (leurs yeux)
	Lueur->SetLightColor(bAylis ? FLinearColor(0.55f, 0.45f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.15f));
	Lueur->SetIntensity(bAylis ? 1400.0f : 160.0f);
	Lueur->SetAttenuationRadius(bAylis ? 520.0f : 220.0f);

	Habiller(Dossier);
	MettreAJourTexte();
}

// ===================== Le modele 3D et ses animations =====================
// On cherche dans le dossier : le premier modele "squelettique" (un personnage avec des os), et les animations
// dont le nom finit par Idle, Walking_A... (les noms des animations KayKit). S'il y en a plusieurs, on prend
// le nom le plus court : "Idle" plutot que "Jump_Idle" ou "Sit_Chair_Idle".

void AVespUnite::Habiller(const FString& Dossier)
{
	IAssetRegistry& Registre = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registre.GetAssetsByPath(FName(*Dossier), Assets, true);

	// Unreal importe les personnages KayKit en plusieurs morceaux (le corps, la tete, les bras, les jambes),
	// qui partagent le meme squelette. Le corps "mene" : les autres morceaux suivent sa pose et ses animations.
	TArray<USkeletalMesh*> Morceaux;
	USkeletalMesh* Maillage = nullptr;
	for (const FAssetData& A : Assets)
	{
		if (A.IsInstanceOf(USkeletalMesh::StaticClass()))
		{
			USkeletalMesh* M = Cast<USkeletalMesh>(A.GetAsset());
			Morceaux.Add(M);
			if (!Maillage || A.AssetName.ToString().Contains(TEXT("Body")))
			{
				Maillage = M;
			}
		}
	}
	if (!Maillage)
	{
		return;		// pas encore de modele importe : la silhouette de secours reste
	}

	auto Chercher = [&Assets](std::initializer_list<const TCHAR*> Noms) -> UAnimSequence* {
		for (const TCHAR* Nom : Noms)
		{
			UAnimSequence* Meilleure = nullptr;
			int32 LongueurMin = MAX_int32;
			for (const FAssetData& A : Assets)
			{
				const FString NomAsset = A.AssetName.ToString();
				if (A.IsInstanceOf(UAnimSequence::StaticClass()) && NomAsset.EndsWith(Nom, ESearchCase::IgnoreCase) && NomAsset.Len() < LongueurMin)
				{
					LongueurMin = NomAsset.Len();
					Meilleure = Cast<UAnimSequence>(A.GetAsset());
				}
			}
			if (Meilleure)
			{
				return Meilleure;
			}
		}
		return nullptr;
	};
	AnimRepos = Chercher({TEXT("Idle_Combat"), TEXT("2H_Melee_Idle"), TEXT("Idle")});
	AnimMarche = Chercher({TEXT("Walking_A"), TEXT("Walking_B"), TEXT("Running_A")});
	AnimAttaque = Chercher({TEXT("1H_Melee_Attack_Chop"), TEXT("1H_Melee_Attack_Slice_Diagonal"), TEXT("Unarmed_Melee_Attack_Punch_A")});
	AnimTouche = Chercher({TEXT("Hit_A"), TEXT("Hit_B")});
	AnimChute = Chercher({TEXT("Death_A"), TEXT("Death_B")});

	Modele->SetSkeletalMesh(Maillage);
	FBox Boite = Maillage->GetBounds().GetBox();
	for (USkeletalMesh* M : Morceaux)
	{
		if (M == Maillage)
		{
			continue;
		}
		USkeletalMeshComponent* Piece = NewObject<USkeletalMeshComponent>(this);
		Piece->SetupAttachment(Modele);
		Piece->SetSkeletalMesh(M);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->RegisterComponent();
		Piece->SetLeaderPoseComponent(Modele);
		Pieces.Add(Piece);
		Boite += M->GetBounds().GetBox();
	}
	Corps->SetVisibility(false);
	Tete->SetVisibility(false);
	// A la bonne taille : on mesure le modele entier et on l'agrandit (ou le reduit) pour qu'il fasse "Taille" cm
	const float Hauteur = Boite.GetSize().Z;
	if (Hauteur > 1.0f)
	{
		Modele->SetRelativeScale3D(FVector(Taille / Hauteur));
	}
	Modele->SetRelativeRotation(FRotator(0, CorrectionRotation, 0));
	Jouer(AnimRepos, true);
}

void AVespUnite::Jouer(UAnimSequence* Animation, bool bEnBoucle)
{
	if (Animation && Modele->GetSkeletalMeshAsset())
	{
		Modele->PlayAnimation(Animation, bEnBoucle);
	}
}

void AVespUnite::Regarder(const FVector& Point)
{
	FVector Vers = Point - GetActorLocation();
	Vers.Z = 0;
	if (!Vers.IsNearlyZero())
	{
		SetActorRotation(Vers.Rotation());
	}
}

// ===================== La marche =====================

void AVespUnite::Suivre(const TArray<FIntPoint>& Chemin)
{
	if (Chemin.IsEmpty())
	{
		return;
	}
	CheminRestant = Chemin;
	Grille->Liberer(this);
	Case = Chemin.Last();		// la case d'arrivee est reservee tout de suite
	Grille->Occuper(this);
	Jouer(AnimMarche, true);
}

void AVespUnite::Tick(float Secondes)
{
	Super::Tick(Secondes);

	// Le texte au-dessus de la tete regarde toujours la camera
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Texte->SetWorldRotation((Camera->GetCameraLocation() - Texte->GetComponentLocation()).Rotation());
	}
	if (TempsTexte > 0.0f)
	{
		TempsTexte -= Secondes;
		if (TempsTexte <= 0.0f)
		{
			MettreAJourTexte();
		}
	}
	if (TempsAction > 0.0f)
	{
		TempsAction -= Secondes;
		if (TempsAction <= 0.0f && EstDebout())
		{
			Jouer(AnimRepos, true);
		}
	}

	if (CheminRestant.IsEmpty() || !Grille)
	{
		return;
	}
	// On avance vers la prochaine case du chemin ; une fois arrive, on passe a la suivante
	const FVector Cible = Grille->CentreDeCase(CheminRestant[0]);
	const FVector Ici = GetActorLocation();
	const FVector Vers = Cible - Ici;
	const float Pas = Vitesse * Secondes;
	if (Vers.Size() <= Pas)
	{
		SetActorLocation(Cible);
		CheminRestant.RemoveAt(0);
		if (CheminRestant.IsEmpty())
		{
			Jouer(AnimRepos, true);
		}
	}
	else
	{
		SetActorLocation(Ici + Vers.GetSafeNormal() * Pas);
		Regarder(Cible);
	}
}

// ===================== Le combat (les memes regles que le prototype) =====================

int32 AVespUnite::Frapper(AVespUnite* Cible)
{
	Regarder(Cible->GetActorLocation());
	Jouer(AnimAttaque, false);
	TempsAction = 0.9f;
	// attaque - defense, un peu de hasard, et parfois un critique (x2)
	int32 Degats = Stats.Attaque - Cible->Stats.Defense + FMath::RandRange(-1, 1);
	const bool bCritique = FMath::RandRange(1, 100) <= Stats.ChanceCritique;
	if (bCritique)
	{
		Degats *= 2;
	}
	Degats = FMath::Max(1, Degats);
	Cible->Encaisser(Degats, bCritique);
	return Degats;
}

void AVespUnite::Encaisser(int32 Degats, bool bCritique)
{
	Stats.Pv = FMath::Max(0, Stats.Pv - Degats);
	TempsAction = 0.7f;
	Texte->SetText(FText::FromString(FString::Printf(TEXT("%s-%d"), bCritique ? TEXT("CRIT ") : TEXT(""), Degats)));
	Texte->SetTextRenderColor(bCritique ? FColor::Orange : FColor::Yellow);
	TempsTexte = 0.8f;
	if (EstDebout())
	{
		Jouer(AnimTouche, false);
		return;
	}
	// Il tombe : il libere sa case, et reste au sol (sa lueur s'eteint)
	Grille->Liberer(this);
	Lueur->SetVisibility(false);
	CheminRestant.Reset();
	if (AnimChute && Modele->GetSkeletalMeshAsset())
	{
		Jouer(AnimChute, false);
	}
	else
	{
		SetActorRotation(FRotator(0, GetActorRotation().Yaw, 90));	// la silhouette se couche
	}
}

void AVespUnite::MettreAJourTexte()
{
	Texte->SetTextRenderColor(bAylis ? FColor(120, 180, 255) : FColor(255, 110, 90));
	Texte->SetText(EstDebout() ? FText::FromString(FString::Printf(TEXT("%d / %d"), Stats.Pv, Stats.PvMax)) : FText::GetEmpty());
}
