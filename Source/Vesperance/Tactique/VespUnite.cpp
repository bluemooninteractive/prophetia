#include "VespUnite.h"
#include "VespGrille.h"
#include "VespEffet.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"

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
	Texte->SetRelativeLocation(FVector(0, 0, 240));
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
	IntensiteLueur = bAylis ? 1400.0f : 160.0f;
	Lueur->SetIntensity(IntensiteLueur);
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
		EchelleModele = Taille / Hauteur;
		Modele->SetRelativeScale3D(FVector(EchelleModele));
	}
	Modele->SetRelativeRotation(FRotator(0, CorrectionRotation, 0));
	if (bAylis)
	{
		TeindreEnBleu();
	}
	Jouer(AnimRepos, true);
}

// Une copie de la texture ou le vert devient bleu nuit (la peau, le cuir et le metal ne changent pas)
static UTexture2D* TextureEnBleu(UTexture2D* Source, UObject* Proprietaire)
{
#if WITH_EDITORONLY_DATA
	static TMap<UTexture2D*, UTexture2D*> DejaFaites;		// une seule copie par texture
	if (UTexture2D** Faite = DejaFaites.Find(Source))
	{
		return *Faite;
	}
	TArray64<uint8> Pixels;
	if (!Source->Source.IsValid() || Source->Source.GetFormat() != TSF_BGRA8 || !Source->Source.GetMipData(Pixels, 0))
	{
		return nullptr;		// un format qu'on ne sait pas lire : on garde la texture d'origine
	}
	const int32 Largeur = Source->Source.GetSizeX();
	const int32 Hauteur = Source->Source.GetSizeY();
	const FLinearColor Bleu(0.25f, 0.39f, 0.88f);
	for (int64 i = 0; i + 3 < Pixels.Num(); i += 4)
	{
		const float B = Pixels[i] / 255.0f, G = Pixels[i + 1] / 255.0f, R = Pixels[i + 2] / 255.0f;
		// "Combien ce pixel est vert" : 0 pour la peau ou le cuir, 1 pour la cape verte
		const float Vert = FMath::Clamp((G - FMath::Max(R, B) - 0.02f) * 6.0f, 0.0f, 1.0f);
		if (Vert <= 0.0f)
		{
			continue;
		}
		const float Lumiere = 0.3f * R + 0.59f * G + 0.11f * B;
		const float Force = 0.5f + Lumiere * 2.0f;
		Pixels[i] = (uint8)FMath::Clamp(FMath::Lerp(B, Bleu.B * Force, Vert) * 255.0f, 0.0f, 255.0f);
		Pixels[i + 1] = (uint8)FMath::Clamp(FMath::Lerp(G, Bleu.G * Force, Vert) * 255.0f, 0.0f, 255.0f);
		Pixels[i + 2] = (uint8)FMath::Clamp(FMath::Lerp(R, Bleu.R * Force, Vert) * 255.0f, 0.0f, 255.0f);
	}
	UTexture2D* Copie = UTexture2D::CreateTransient(Largeur, Hauteur, PF_B8G8R8A8);
	Copie->SRGB = Source->SRGB;
	Copie->Filter = Source->Filter;
	void* Donnees = Copie->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Donnees, Pixels.GetData(), (SIZE_T)Largeur * Hauteur * 4);
	Copie->GetPlatformData()->Mips[0].BulkData.Unlock();
	Copie->UpdateResource();
	Copie->AddToRoot();			// la garder en memoire tant que le jeu tourne
	DejaFaites.Add(Source, Copie);
	return Copie;
#else
	return nullptr;
#endif
}

void AVespUnite::TeindreEnBleu()
{
	TArray<USkeletalMeshComponent*> Morceaux = {Modele};
	for (USkeletalMeshComponent* P : Pieces)
	{
		Morceaux.Add(P);
	}
	for (USkeletalMeshComponent* Morceau : Morceaux)
	{
		for (int32 i = 0; i < Morceau->GetNumMaterials(); i++)
		{
			UMaterialInstance* Materiau = Cast<UMaterialInstance>(Morceau->GetMaterial(i));
			if (!Materiau)
			{
				continue;
			}
			UMaterialInstanceDynamic* Nouveau = nullptr;
			for (const FTextureParameterValue& P : Materiau->TextureParameterValues)
			{
				UTexture2D* Texture = Cast<UTexture2D>(P.ParameterValue);
				UTexture2D* EnBleu = Texture ? TextureEnBleu(Texture, this) : nullptr;
				if (EnBleu)
				{
					if (!Nouveau)
					{
						Nouveau = Morceau->CreateDynamicMaterialInstance(i, Materiau);
					}
					Nouveau->SetTextureParameterValueByInfo(P.ParameterInfo, EnBleu);
				}
			}
		}
	}
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
	Grille->Glisser(Case, CheminRestant);		// sur la glace, on glisse plus loin que prevu
	Case = CheminRestant.Last();		// la case d'arrivee est reservee tout de suite
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
	TempsVie += Secondes;
	if (TempsTexte > 0.0f)
	{
		TempsTexte -= Secondes;
		// le chiffre monte en s'eloignant
		Texte->SetRelativeLocation(FVector(0, 0, 240.0f + (1.0f - FMath::Max(0.0f, TempsTexte) / DureeTexte) * 80.0f));
		if (TempsTexte <= 0.0f)
		{
			MettreAJourTexte();
		}
	}
	if (TempsEclat > 0.0f)
	{
		TempsEclat -= Secondes;
		Lueur->SetIntensity(TempsEclat > 0.0f ? IntensiteLueur + 6000.0f : IntensiteLueur);
		Lueur->SetLightColor(TempsEclat > 0.0f ? FLinearColor(1.0f, 0.85f, 0.6f)
		                                       : (bAylis ? FLinearColor(0.55f, 0.45f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.15f)));
	}
	if (TempsAction > 0.0f)
	{
		TempsAction -= Secondes;
		if (TempsAction <= 0.0f && EstDebout())
		{
			Jouer(AnimRepos, true);
		}
	}

	if (!Grille)
	{
		return;
	}
	// Le corps : il respire, s'ecrase sous un coup, rebondit a chaque pas
	float Ecrase = 0.0f;
	if (TempsEcrase > 0.0f)
	{
		TempsEcrase -= Secondes;
		Ecrase = FMath::Sin(FMath::Clamp(1.0f - TempsEcrase / 0.25f, 0.0f, 1.0f) * PI) * 0.2f;
	}
	const float Souffle = EstDebout() ? FMath::Sin(TempsVie * 2.2f) * 0.015f : 0.0f;
	const float Haut = 1.0f - Ecrase + Souffle;
	const float Large = 1.0f + Ecrase * 0.8f - Souffle * 0.5f;
	if (Modele->GetSkeletalMeshAsset())
	{
		Modele->SetRelativeScale3D(FVector(Large, Large, Haut) * EchelleModele);
	}
	else
	{
		Corps->SetRelativeScale3D(FVector(0.45f * Large, 0.45f * Large, Haut));
	}

	if (CheminRestant.IsEmpty())
	{
		// A sa place : l'elan vers la cible (un aller-retour) et le recul d'un coup
		float Elan = 0.0f;
		if (TempsElan > 0.0f)
		{
			TempsElan -= Secondes;
			Elan = FMath::Sin(FMath::Clamp(1.0f - TempsElan / 0.35f, 0.0f, 1.0f) * PI) * 45.0f;
		}
		Recul = FMath::VInterpTo(Recul, FVector::ZeroVector, Secondes, 8.0f);
		Modele->SetRelativeLocation(FVector::ZeroVector);
		if (EstDebout() || !Recul.IsNearlyZero())
		{
			SetActorLocation(Grille->CentreDeCase(Case) + DirectionElan * Elan + Recul);
		}
		return;
	}
	// On avance vers la prochaine case du chemin ; une fois arrive, on passe a la suivante
	const FVector Cible = Grille->CentreDeCase(CheminRestant[0]);
	const FVector Ici = GetActorLocation();
	const FVector Vers = Cible - Ici;
	const float Pas = Vitesse * Secondes;
	DistanceMarche += Pas;
	Modele->SetRelativeLocation(FVector(0, 0, FMath::Abs(FMath::Sin(DistanceMarche / Grille->TailleCase * PI)) * 12.0f));	// un petit bond par case
	if (Vers.Size() <= Pas)
	{
		SetActorLocation(Cible);
		CheminRestant.RemoveAt(0);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, Cible, FVector::UpVector,
		                  bAylis ? FLinearColor(0.45f, 0.45f, 0.7f) : FLinearColor(0.5f, 0.35f, 0.3f));
		if (CheminRestant.IsEmpty())
		{
			DistanceMarche = 0.0f;
			Modele->SetRelativeLocation(FVector::ZeroVector);
			Jouer(AnimRepos, true);
			// Une faille du Voile : elle emporte qui s'y arrete vers une autre faille
			FIntPoint Sortie;
			if (Grille->TerrainSur(Case) == '@' && Grille->AutreFaille(Case, Sortie))
			{
				Bondir(Sortie);
			}
		}
	}
	else
	{
		SetActorLocation(Ici + Vers.GetSafeNormal() * Pas);
		Regarder(Cible);
	}
}

// ===================== Le combat (les memes regles que le prototype) =====================

int32 AVespUnite::Frapper(AVespUnite* Cible, int32 Puissance, bool* bCritiqueSortie)
{
	Regarder(Cible->GetActorLocation());
	Jouer(AnimAttaque, false);
	TempsAction = 0.9f;
	FVector Vers = Cible->GetActorLocation() - GetActorLocation();
	Vers.Z = 0.0f;
	const float Distance = Vers.Size();
	Vers = Vers.GetSafeNormal();
	const FLinearColor Teinte = bAylis ? FLinearColor(0.55f, 0.7f, 1.0f)
	                                   : (Stats.Effet == VespEffetCoup::Poison ? FLinearColor(0.55f, 1.0f, 0.35f)
	                                   : (Stats.Effet == VespEffetCoup::Gel ? FLinearColor(0.5f, 0.8f, 1.0f) : FLinearColor(1.0f, 0.4f, 0.2f)));
	Cible->DirectionCoup = Vers;
	Cible->CouleurCoup = Teinte;
	if (Distance <= Grille->TailleCase * 1.5f)
	{
		// Au contact : il se fend vers sa cible, et sa lame dessine un arc
		DirectionElan = Vers;
		TempsElan = 0.35f;
		AVespEffet::Jouer(GetWorld(), EVespEffet::Trainee, GetActorLocation() + FVector(0, 0, Taille * 0.6f), Vers, Teinte, 0.12f);
		Cible->RetardImpact = 0.16f;
	}
	else
	{
		// De loin : une fleche (ou un sort) vole jusqu'a la cible
		AVespEffet::Jouer(GetWorld(), EVespEffet::Projectile, GetActorLocation() + FVector(0, 0, Taille * 0.65f) + Vers * 40.0f, Vers, Teinte, 0.15f,
		                  Cible->GetActorLocation() + FVector(0, 0, Cible->Taille * 0.55f));
		Cible->RetardImpact = 0.45f;
	}
	if (Puissance <= 0)
	{
		return 0;		// un coup rate : l'elan, sans rien toucher
	}
	// attaque x puissance - defense, un peu de hasard, et parfois un critique (x2), comme dans le prototype
	int32 Degats = Stats.Attaque * Puissance / 100 - Cible->Stats.Defense + FMath::RandRange(-1, 1);
	const bool bCritique = FMath::RandRange(1, 100) <= Stats.ChanceCritique;
	if (bCritique)
	{
		Degats *= 2;
	}
	if (bCritiqueSortie)
	{
		*bCritiqueSortie = bCritique;
	}
	// L'armure : un coup ordinaire glisse dessus ; un coup lourd fissure une plaque
	bool bFissure = false;
	if (Cible->Stats.Armure > 0)
	{
		if (Puissance >= 180)
		{
			Cible->Stats.Armure--;
			bFissure = true;
		}
		else
		{
			Degats = FMath::Max(1, Degats / 4);
		}
	}
	if (Cible->TempsBrise > 0)
	{
		Degats = Degats * 3 / 2;		// sans armure, sonne : il encaisse davantage
	}
	Degats = FMath::Max(1, FMath::RoundToInt(FMath::Max(1, Degats) * Cible->ReductionDegats));
	Cible->Encaisser(Degats, bCritique);
	if (bFissure && Cible->EstDebout())
	{
		if (Cible->Stats.Armure == 0)
		{
			Cible->TempsBrise = 2;
			Cible->AfficherMessage(TEXT("ARMURE BRISEE"), FColor(255, 210, 120));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, Cible->GetActorLocation() + FVector(0, 0, Cible->Taille * 0.6f), FVector::UpVector, FLinearColor(0.8f, 0.8f, 0.9f), 0.2f);
		}
		else
		{
			Cible->AfficherMessage(TEXT("fissure"), FColor(220, 220, 235));
		}
	}
	if ((Stats.Capacites & VespCapacite::Vampire) && EstDebout())
	{
		Soigner(FMath::Max(1, Degats / 2));
	}
	return Degats;
}

void AVespUnite::Bondir(FIntPoint Arrivee)
{
	AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, GetActorLocation(), FVector::UpVector, FLinearColor(0.7f, 0.4f, 1.0f));
	Grille->Liberer(this);
	Case = Arrivee;
	CheminRestant.Reset();
	Recul = FVector::ZeroVector;
	SetActorLocation(Grille->CentreDeCase(Case));
	Grille->Occuper(this);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.7f, 0.4f, 1.0f));
	Jouer(AnimRepos, true);
}

void AVespUnite::Replacer(FIntPoint NouvelleCase)
{
	Grille->Liberer(this);
	Case = NouvelleCase;
	CheminRestant.Reset();
	Recul = FVector::ZeroVector;
	TempsElan = 0.0f;
	SetActorLocation(Grille->CentreDeCase(Case));
	SetActorRotation(FRotator::ZeroRotator);
	Grille->Occuper(this);
	Poison = 0;
	Brulure = 0;
	Gel = 0;
	Jouer(AnimRepos, true);
}

void AVespUnite::AfficherMessage(const FString& Message, FColor Couleur)
{
	Texte->SetText(FText::FromString(Message));
	Texte->SetTextRenderColor(Couleur);
	Texte->SetWorldSize(38.0f);
	TempsTexte = 1.0f;
	DureeTexte = 1.0f;
}

void AVespUnite::Soigner(int32 Quantite)
{
	const int32 Avant = Stats.Pv;
	Stats.Pv = FMath::Min(Stats.PvMax, Stats.Pv + Quantite);
	if (Stats.Pv > Avant)
	{
		AfficherMessage(FString::Printf(TEXT("+%d"), Stats.Pv - Avant), FColor(120, 230, 140));
		AVespEffet::Jouer(GetWorld(), EVespEffet::Soin, GetActorLocation(), FVector::UpVector, FLinearColor(0.35f, 1.0f, 0.5f));
	}
}

// Au debut de son tour : le poison (-3) et la brulure (-4), comme dans le prototype
int32 AVespUnite::SubirEtats()
{
	int32 Total = 0;
	if (Poison > 0)
	{
		Total += 3;
		Poison--;
	}
	if (Brulure > 0)
	{
		Total += 4;
		Brulure--;
	}
	if (Total > 0 && EstDebout())
	{
		Encaisser(Total, false);
		Texte->SetTextRenderColor(FColor(190, 110, 255));		// le violet des etats
		AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, GetActorLocation(), FVector::UpVector,
		                  Brulure > 0 ? FLinearColor(1.0f, 0.5f, 0.15f) : FLinearColor(0.6f, 1.0f, 0.3f));
	}
	return Total;
}

void AVespUnite::Encaisser(int32 Degats, bool bCritique)
{
	Stats.Pv = FMath::Max(0, Stats.Pv - Degats);
	TempsEclat = 0.18f;		// sa lueur eclate un instant (l'impact)
	TempsAction = 0.7f;
	Texte->SetText(FText::FromString(FString::Printf(TEXT("%s-%d"), bCritique ? TEXT("CRIT ") : TEXT(""), Degats)));
	Texte->SetTextRenderColor(bCritique ? FColor::Orange : FColor::Yellow);
	TempsTexte = 0.9f;
	DureeTexte = 0.9f;
	Texte->SetWorldSize(bCritique ? 58.0f : 40.0f);
	const FVector Poitrine = GetActorLocation() + FVector(0, 0, Taille * 0.55f);
	AVespEffet::Jouer(GetWorld(), bCritique ? EVespEffet::Critique : EVespEffet::Impact, Poitrine, DirectionCoup, CouleurCoup, RetardImpact);
	Recul = DirectionCoup * (bCritique ? 45.0f : 22.0f);
	TempsEcrase = 0.25f;
	RetardImpact = 0.0f;
	if (EstDebout())
	{
		Jouer(AnimTouche, false);
		return;
	}
	// L'ame s'echappe
	AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, GetActorLocation(), FVector::UpVector,
	                  bAylis ? FLinearColor(0.5f, 0.55f, 1.0f) : FLinearColor(0.75f, 0.3f, 1.0f), 0.3f);
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
	Texte->SetText(FText::GetEmpty());		// les pv sont affiches par l'interface (VespHUD) : ici, seulement les degats recus
}
