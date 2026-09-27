#include "VespUnite.h"
#include "VespAnim.h"
#include "VespMonde.h"
#include "VespEffet.h"
#include "VespSons.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

void AVespUnite::Preparer(AVespMonde* LeMonde, const FVespStats& LesStats, const FString& Dossier, FLinearColor Teinte, bool bEstAylis)
{
	Monde = LeMonde;
	Stats = LesStats;
	bAylis = bEstAylis;

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Teinte);
	Corps->SetMaterial(0, Materiau);
	Tete->SetMaterial(0, Materiau);
	// AYLIS porte la lumiere violette de la prophetie ; les Haschen, une faible lueur rouge (leurs yeux)
	CouleurLueur = bAylis ? FLinearColor(0.55f, 0.45f, 1.0f) : FLinearColor(1.0f, 0.25f, 0.15f);
	IntensiteLueur = bAylis ? 1400.0f : (Stats.Boss > 0 ? 900.0f : 160.0f);
	Lueur->SetLightColor(CouleurLueur);
	Lueur->SetIntensity(IntensiteLueur);
	Lueur->SetAttenuationRadius(bAylis ? 560.0f : (Stats.Boss > 0 ? 600.0f : 240.0f));
	// Les grands (elites, boss) ne flechissent pas a chaque coup
	SeuilEquilibre = bAylis ? 0.0f : (Stats.Boss > 0 ? Stats.PvMax * 0.12f : (Taille >= 205.0f ? Stats.PvMax * 0.3f : 0.0f));

	Habiller(Dossier);
	Texte->SetText(FText::GetEmpty());
}

// ===================== Le modele 3D et ses animations =====================
// On cherche dans le dossier : le premier modele "squelettique" (un personnage avec des os), et toutes les
// animations (les noms KayKit : Idle, Walking_A, 1H_Melee_Attack_Chop...).

void AVespUnite::Habiller(const FString& Dossier)
{
	IAssetRegistry& Registre = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registre.ScanPathsSynchronous({Dossier}, false);
	Registre.GetAssetsByPath(FName(*Dossier), Assets, true);

	// Unreal importe les personnages KayKit en plusieurs morceaux (le corps, la tete, les bras, les jambes),
	// qui partagent le meme squelette. Le corps "mene" : les autres morceaux suivent sa pose.
	TArray<USkeletalMesh*> Morceaux;
	USkeletalMesh* Maillage = nullptr;
	for (const FAssetData& A : Assets)
	{
		if (A.IsInstanceOf(USkeletalMesh::StaticClass()))
		{
			const FString Nom = A.AssetName.ToString();
			// (les accessoires du pack : arbaletes, couteaux, cape... ne font pas partie du corps)
			if (Nom.Contains(TEXT("Crossbow")) || Nom.Contains(TEXT("Knife")) || Nom.Contains(TEXT("Throwable")) || Nom.EndsWith(TEXT("_Cape")))
			{
				continue;
			}
			USkeletalMesh* M = Cast<USkeletalMesh>(A.GetAsset());
			Morceaux.Add(M);
			if (!Maillage || Nom.Contains(TEXT("Body")))
			{
				Maillage = M;
			}
		}
		else if (A.IsInstanceOf(UAnimSequence::StaticClass()))
		{
			if (UAnimSequence* S = Cast<UAnimSequence>(A.GetAsset()))
			{
				Animations.Add(S);
			}
		}
	}
	if (!Maillage)
	{
		return;		// pas encore de modele importe : la silhouette de secours reste
	}
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
	// Les animations : notre "animateur" les melange en douceur
	Modele->SetAnimInstanceClass(UVespAnimInstance::StaticClass());
	bAnime = Animateur() != nullptr && Animations.Num() > 0;
	MettreEnPlaceLocomotion();
	if (bAylis)
	{
		TeindreTenue(FLinearColor(0.25f, 0.39f, 0.88f));		// le bleu nuit d'AYLIS
	}
}

UVespAnimInstance* AVespUnite::Animateur() const
{
	return Cast<UVespAnimInstance>(Modele->GetAnimInstance());
}

// Une animation par la fin de son nom (le nom le plus court gagne : "Idle" plutot que "Jump_Idle")
const UAnimSequence* AVespUnite::Anim(std::initializer_list<const TCHAR*> Noms) const
{
	for (const TCHAR* Nom : Noms)
	{
		if (const TObjectPtr<UAnimSequence>* Deja = AnimsTrouvees.Find(Nom))
		{
			if (*Deja)
			{
				return *Deja;
			}
			continue;
		}
		UAnimSequence* Meilleure = nullptr;
		int32 LongueurMin = MAX_int32;
		for (UAnimSequence* S : Animations)
		{
			const FString NomAsset = S->GetName();
			if (NomAsset.EndsWith(Nom, ESearchCase::IgnoreCase) && NomAsset.Len() < LongueurMin)
			{
				LongueurMin = NomAsset.Len();
				Meilleure = S;
			}
		}
		AnimsTrouvees.Add(Nom, Meilleure);
		if (Meilleure)
		{
			return Meilleure;
		}
	}
	return nullptr;
}

void AVespUnite::MettreEnPlaceLocomotion()
{
	UVespAnimInstance* A = Animateur();
	if (!A)
	{
		return;
	}
	const UAnimSequence* Repos = TypeArme == EVespArme::DeuxMains ? Anim({TEXT("2H_Melee_Idle"), TEXT("Idle_Combat"), TEXT("Idle")})
	                                                              : Anim({TEXT("Idle_Combat"), TEXT("Idle")});
	if (bAylis && TypeArme != EVespArme::DeuxMains)
	{
		Repos = Anim({TEXT("Idle")});
	}
	A->Locomotion(Repos, Anim({TEXT("Walking_A"), TEXT("Walking_B"), TEXT("Walking_C")}), Anim({TEXT("Running_A"), TEXT("Running_B")}),
	              Vitesse * 0.62f, Vitesse * 1.35f);
}

const UAnimSequence* AVespUnite::AnimDuGeste(EVespGeste Geste) const
{
	const EVespArme T = TypeArme;
	switch (Geste)
	{
		case EVespGeste::Attaque1:
			if (T == EVespArme::Dagues) return Anim({TEXT("Dualwield_Melee_Attack_Slice"), TEXT("1H_Melee_Attack_Slice_Diagonal")});
			if (T == EVespArme::DeuxMains) return Anim({TEXT("2H_Melee_Attack_Slice"), TEXT("1H_Melee_Attack_Slice_Diagonal")});
			if (T == EVespArme::Baton) return Anim({TEXT("Spellcast_Shoot"), TEXT("1H_Melee_Attack_Stab")});
			if (T == EVespArme::Arc) return Anim({TEXT("2H_Ranged_Shoot"), TEXT("1H_Ranged_Shoot"), TEXT("Throw")});
			if (T == EVespArme::Poings) return Anim({TEXT("Unarmed_Melee_Attack_Punch_A"), TEXT("1H_Melee_Attack_Chop")});
			return Anim({TEXT("1H_Melee_Attack_Slice_Diagonal"), TEXT("1H_Melee_Attack_Chop"), TEXT("Unarmed_Melee_Attack_Punch_A")});
		case EVespGeste::Attaque2:
			if (T == EVespArme::Dagues) return Anim({TEXT("Dualwield_Melee_Attack_Stab"), TEXT("1H_Melee_Attack_Stab")});
			if (T == EVespArme::DeuxMains) return Anim({TEXT("2H_Melee_Attack_Stab"), TEXT("2H_Melee_Attack_Slice")});
			if (T == EVespArme::Baton || T == EVespArme::Arc) return AnimDuGeste(EVespGeste::Attaque1);
			if (T == EVespArme::Poings) return Anim({TEXT("Unarmed_Melee_Attack_Punch_B"), TEXT("Unarmed_Melee_Attack_Punch_A")});
			return Anim({TEXT("1H_Melee_Attack_Slice_Horizontal"), TEXT("1H_Melee_Attack_Stab"), TEXT("1H_Melee_Attack_Chop")});
		case EVespGeste::Attaque3:
			if (T == EVespArme::Dagues) return Anim({TEXT("Dualwield_Melee_Attack_Chop"), TEXT("1H_Melee_Attack_Chop")});
			if (T == EVespArme::DeuxMains) return Anim({TEXT("2H_Melee_Attack_Chop"), TEXT("2H_Melee_Attack_Slice")});
			if (T == EVespArme::Baton) return Anim({TEXT("Spellcast_Long"), TEXT("Spellcast_Shoot")});
			if (T == EVespArme::Arc) return AnimDuGeste(EVespGeste::Attaque1);
			if (T == EVespArme::Poings) return Anim({TEXT("Unarmed_Melee_Attack_Kick"), TEXT("Unarmed_Melee_Attack_Punch_A")});
			return Anim({TEXT("1H_Melee_Attack_Chop"), TEXT("1H_Melee_Attack_Stab")});
		case EVespGeste::Lourde:
			if (T == EVespArme::Baton) return Anim({TEXT("Spellcast_Long"), TEXT("Spellcast_Shoot")});
			if (T == EVespArme::Arc) return AnimDuGeste(EVespGeste::Attaque1);
			return Anim({TEXT("2H_Melee_Attack_Chop"), TEXT("1H_Melee_Attack_Chop"), TEXT("Unarmed_Melee_Attack_Kick")});
		case EVespGeste::Tourbillon: return Anim({TEXT("2H_Melee_Attack_Spin"), TEXT("2H_Melee_Attack_Spinning"), TEXT("1H_Melee_Attack_Slice_Horizontal")});
		case EVespGeste::Tir: return Anim({TEXT("2H_Ranged_Shoot"), TEXT("1H_Ranged_Shoot"), TEXT("Spellcast_Shoot"), TEXT("Throw")});
		case EVespGeste::Sort: return Anim({TEXT("Spellcast_Shoot"), TEXT("Spellcast_Long"), TEXT("Throw")});
		case EVespGeste::Invocation: return Anim({TEXT("Spellcast_Raise"), TEXT("Spellcast_Long"), TEXT("Cheer")});
		case EVespGeste::Lancer: return Anim({TEXT("Throw"), TEXT("Spellcast_Shoot")});
		case EVespGeste::EsquiveAvant: return Anim({TEXT("Dodge_Forward"), TEXT("Dodge_Left")});
		case EVespGeste::EsquiveArriere: return Anim({TEXT("Dodge_Backward"), TEXT("Dodge_Forward")});
		case EVespGeste::EsquiveGauche: return Anim({TEXT("Dodge_Left"), TEXT("Dodge_Forward")});
		case EVespGeste::EsquiveDroite: return Anim({TEXT("Dodge_Right"), TEXT("Dodge_Forward")});
		case EVespGeste::GardeLevee: return Anim({TEXT("Block"), TEXT("Blocking")});
		case EVespGeste::GardeTouchee: return Anim({TEXT("Block_Hit"), TEXT("Hit_A")});
		case EVespGeste::Riposte: return Anim({TEXT("Block_Attack"), TEXT("1H_Melee_Attack_Stab")});
		case EVespGeste::Touche: return FMath::RandBool() ? Anim({TEXT("Hit_A"), TEXT("Hit_B")}) : Anim({TEXT("Hit_B"), TEXT("Hit_A")});
		case EVespGeste::Mort: return FMath::RandBool() ? Anim({TEXT("Death_A"), TEXT("Death_B")}) : Anim({TEXT("Death_B"), TEXT("Death_A")});
		case EVespGeste::Potion: return Anim({TEXT("Use_Item"), TEXT("Interact")});
		case EVespGeste::Ramasser: return Anim({TEXT("PickUp"), TEXT("Interact")});
		case EVespGeste::Interagir: return Anim({TEXT("Interact"), TEXT("PickUp")});
		case EVespGeste::Victoire: return Anim({TEXT("Cheer"), TEXT("Spellcast_Raise")});
		case EVespGeste::Resurrection: return Anim({TEXT("Skeletons_Resurrect"), TEXT("Lie_StandUp"), TEXT("Sit_Floor_StandUp")});
		case EVespGeste::Saut: return Anim({TEXT("Jump_Full_Short"), TEXT("Jump_Full_Long")});
		default: return nullptr;
	}
}

float AVespUnite::Jouer(EVespGeste Geste, float VitesseGeste, bool bHautDuCorps, bool bTenir)
{
	UVespAnimInstance* A = Animateur();
	const UAnimSequence* S = AnimDuGeste(Geste);
	if (!A || !S)
	{
		return 0.35f / FMath::Max(0.1f, VitesseGeste);
	}
	const bool bEsquive = Geste >= EVespGeste::EsquiveAvant && Geste <= EVespGeste::EsquiveDroite;
	return A->JouerAction(S, VitesseGeste, bHautDuCorps, bTenir, bEsquive ? 0.05f : 0.08f, Geste == EVespGeste::Mort ? 0.3f : 0.16f);
}

float AVespUnite::DureeGeste(EVespGeste Geste) const
{
	const UAnimSequence* S = AnimDuGeste(Geste);
	return S ? FMath::Max(0.05f, S->GetPlayLength()) : 0.35f;
}

void AVespUnite::AccelererGeste(float VitesseGeste)
{
	if (UVespAnimInstance* A = Animateur())
	{
		A->SetVitesseAction(VitesseGeste);
	}
}

void AVespUnite::ArreterGeste()
{
	if (UVespAnimInstance* A = Animateur())
	{
		A->ArreterAction(0.1f);
	}
}

bool AVespUnite::GesteEnCours() const
{
	const UVespAnimInstance* A = Animateur();
	return A && A->ActionEnCours();
}

float AVespUnite::ProgressionGeste() const
{
	const UVespAnimInstance* A = Animateur();
	return A ? A->ProgressionAction() : 1.0f;
}

void AVespUnite::TenirGarde(bool bLevee)
{
	if (UVespAnimInstance* A = Animateur())
	{
		A->SetPosture(bLevee ? Anim({TEXT("Blocking"), TEXT("Block")}) : nullptr);
	}
}

// ===================== La teinte de la tenue =====================
// Une copie de la texture ou le vert (la couleur d'origine de la cape KayKit) prend la couleur voulue ;
// la peau, le cuir et le metal ne changent pas. Une copie par texture et par couleur.

static UTexture2D* TextureTeinte(UTexture2D* Source, const FLinearColor& Couleur)
{
#if WITH_EDITORONLY_DATA
	static TMap<FString, UTexture2D*> DejaFaites;
	const FString Cle = FString::Printf(TEXT("%s_%d_%d_%d"), *Source->GetPathName(), FMath::RoundToInt(Couleur.R * 255), FMath::RoundToInt(Couleur.G * 255),
	                                    FMath::RoundToInt(Couleur.B * 255));
	if (UTexture2D** Faite = DejaFaites.Find(Cle))
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
		Pixels[i] = (uint8)FMath::Clamp(FMath::Lerp(B, Couleur.B * Force, Vert) * 255.0f, 0.0f, 255.0f);
		Pixels[i + 1] = (uint8)FMath::Clamp(FMath::Lerp(G, Couleur.G * Force, Vert) * 255.0f, 0.0f, 255.0f);
		Pixels[i + 2] = (uint8)FMath::Clamp(FMath::Lerp(R, Couleur.R * Force, Vert) * 255.0f, 0.0f, 255.0f);
	}
	UTexture2D* Copie = UTexture2D::CreateTransient(Largeur, Hauteur, PF_B8G8R8A8);
	Copie->SRGB = Source->SRGB;
	Copie->Filter = Source->Filter;
	void* Donnees = Copie->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Donnees, Pixels.GetData(), (SIZE_T)Largeur * Hauteur * 4);
	Copie->GetPlatformData()->Mips[0].BulkData.Unlock();
	Copie->UpdateResource();
	Copie->AddToRoot();			// la garder en memoire tant que le jeu tourne
	DejaFaites.Add(Cle, Copie);
	return Copie;
#else
	return nullptr;
#endif
}

void AVespUnite::TeindreTenue(const FLinearColor& Couleur, float Force)
{
	TArray<USkeletalMeshComponent*> Morceaux = {Modele};
	for (USkeletalMeshComponent* P : Pieces)
	{
		Morceaux.Add(P);
	}
	const FLinearColor Voulue = FMath::Lerp(FLinearColor(0.25f, 0.39f, 0.88f), Couleur, FMath::Clamp(Force, 0.0f, 1.0f));
	for (USkeletalMeshComponent* Morceau : Morceaux)
	{
		for (int32 i = 0; i < Morceau->GetNumMaterials(); i++)
		{
			// Le materiau d'origine (pas une copie deja teinte)
			UMaterialInstance* Materiau = Cast<UMaterialInstance>(Morceau->GetMaterial(i));
			if (UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(Materiau))
			{
				Materiau = Cast<UMaterialInstance>(Dyn->Parent);
			}
			if (!Materiau)
			{
				continue;
			}
			UMaterialInstanceDynamic* Nouveau = nullptr;
			for (const FTextureParameterValue& P : Materiau->TextureParameterValues)
			{
				UTexture2D* Texture = Cast<UTexture2D>(P.ParameterValue);
				UTexture2D* Teinte = Texture ? TextureTeinte(Texture, Voulue) : nullptr;
				if (Teinte)
				{
					if (!Nouveau)
					{
						Nouveau = Morceau->CreateDynamicMaterialInstance(i, Materiau);
						Tenue.Add(Nouveau);
					}
					Nouveau->SetTextureParameterValueByInfo(P.ParameterInfo, Teinte);
				}
			}
		}
	}
}

// ===================== Les armes =====================
// Les personnages KayKit ont un os "handslot" dans chaque main, fait pour tenir une arme.

FName AVespUnite::OsDeLaMain(bool bGauche) const
{
	FName Main = NAME_None;
	for (int32 i = 0; i < Modele->GetNumBones(); i++)
	{
		const FName Os = Modele->GetBoneName(i);
		const FString Nom = Os.ToString().ToLower();
		const bool bCote = bGauche ? (Nom.EndsWith(TEXT(".l")) || Nom.EndsWith(TEXT("_l")) || Nom.Contains(TEXT("left")))
		                           : (Nom.EndsWith(TEXT(".r")) || Nom.EndsWith(TEXT("_r")) || Nom.Contains(TEXT("right")));
		if (!bCote)
		{
			continue;
		}
		if (Nom.Contains(TEXT("handslot")))
		{
			return Os;
		}
		if (Main.IsNone() && Nom.Contains(TEXT("hand")))
		{
			Main = Os;
		}
	}
	return Main;
}

void AVespUnite::Desarmer()
{
	for (USkeletalMeshComponent* A : Armes)
	{
		if (A)
		{
			A->DestroyComponent();
		}
	}
	Armes.Reset();
	for (UStaticMeshComponent* A : ArmesFixes)
	{
		if (A)
		{
			A->DestroyComponent();
		}
	}
	ArmesFixes.Reset();
	bBouclier = false;
	TypeArme = EVespArme::Poings;
}

void AVespUnite::Equiper(const FString& Arme, float Longueur, bool bMainGauche, const FString& Bouclier, EVespArme Type)
{
	Desarmer();
	TypeArme = Type;
	if (!Modele->GetSkeletalMeshAsset())
	{
		return;		// la silhouette de secours n'a pas de mains
	}
	auto Attacher = [this](const FString& Chemin, bool bGauche, float LongueurVoulue) {
		USkeletalMesh* M = Chemin.IsEmpty() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *Chemin, nullptr, LOAD_NoWarn | LOAD_Quiet);
		const FName Os = OsDeLaMain(bGauche);
		if (!M && !Chemin.IsEmpty() && !Os.IsNone())
		{
			return AttacherFixe(Chemin, bGauche, LongueurVoulue, LongueurVoulue < 0.5f * Taille && bGauche);
		}
		if (!M || Os.IsNone())
		{
			return false;
		}
		USkeletalMeshComponent* A = NewObject<USkeletalMeshComponent>(this);
		A->SetSkeletalMesh(M);
		A->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		A->SetupAttachment(Modele, Os);
		A->RegisterComponent();
		// A la bonne taille, quelle que soit l'echelle du modele
		const float Mesure = M->GetBounds().GetBox().GetSize().GetMax();
		if (Mesure > 1.0f)
		{
			A->SetWorldScale3D(FVector(LongueurVoulue / Mesure));
		}
		Armes.Add(A);
		return true;
	};
	Attacher(Arme, bMainGauche, Longueur * Taille);
	bBouclier = Attacher(Bouclier, !bMainGauche, 0.38f * Taille);
	MettreEnPlaceLocomotion();
}

bool AVespUnite::AttacherFixe(const FString& Chemin, bool bGauche, float LongueurVoulue, bool bBouclierFixe)
{
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Chemin, nullptr, LOAD_NoWarn | LOAD_Quiet);
	const FName Os = OsDeLaMain(bGauche);
	if (!M || Os.IsNone())
	{
		return false;
	}
	UStaticMeshComponent* A = NewObject<UStaticMeshComponent>(this);
	A->SetStaticMesh(M);
	A->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	A->SetupAttachment(Modele, Os);
	A->RegisterComponent();
	// Les armes KayKit pointent vers +Y depuis la main, et un bouclier montre sa face vers -X.
	// Les autres packs font comme ils veulent : on cherche l'axe long (ou l'axe mince d'un bouclier) et le cote le plus loin de la prise.
	const FBox B = M->GetBoundingBox();
	const FVector T = B.GetSize();
	int32 Axe = 0;
	for (int32 i = 1; i < 3; i++)
	{
		if (bBouclierFixe ? T[i] < T[Axe] : T[i] > T[Axe])
		{
			Axe = i;
		}
	}
	FVector Dir = FVector::ZeroVector;
	Dir[Axe] = FMath::Abs(B.Max[Axe]) >= FMath::Abs(B.Min[Axe]) ? 1.0f : -1.0f;
	const FQuat Q = FQuat::FindBetweenNormals(Dir, bBouclierFixe ? FVector(-1, 0, 0) : FVector(0, 1, 0));
	A->SetRelativeRotation(Q);
	const float Mesure = T.GetMax();
	if (Mesure > 1.0f)
	{
		A->SetWorldScale3D(FVector(LongueurVoulue / Mesure));
	}
	ArmesFixes.Add(A);
	return true;
}

void AVespUnite::ColorerArme(const FString& Chemin, const TCHAR* Suffixe)
{
	static const TCHAR* Familles[6][2] = {{TEXT("Sword"), TEXT("MI_Sword_Newbie_")}, {TEXT("Axe"), TEXT("MI_Axe_Newbie_")}, {TEXT("Dagger"), TEXT("MI_Dagger_1H_Newbie_")},
	                                      {TEXT("Shield"), TEXT("MI_Shield_Newbie_")}, {TEXT("Staff"), TEXT("MI_Staff_2HL_Newbie_")}, {TEXT("Bow"), TEXT("MI_Bow_Newbie_")}};
	for (USkeletalMeshComponent* A : Armes)
	{
		if (!A || !A->GetSkeletalMeshAsset() || A->GetSkeletalMeshAsset()->GetPathName() != Chemin)
		{
			continue;
		}
		const FString Nom = A->GetSkeletalMeshAsset()->GetName();
		for (const auto& F : Familles)
		{
			if (Nom.Contains(F[0]))
			{
				const FString Mi = FString(F[1]) + Suffixe;
				if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/StylizedCharacter/Materials/Instances/Item/Weapon/%s.%s"), *Mi, *Mi),
				                                                          nullptr, LOAD_NoWarn | LOAD_Quiet))
				{
					A->SetMaterial(0, M);
				}
				break;
			}
		}
	}
}

void AVespUnite::EquiperSecondeArme(const FString& Arme, float Longueur)
{
	USkeletalMesh* M = LoadObject<USkeletalMesh>(nullptr, *Arme, nullptr, LOAD_NoWarn | LOAD_Quiet);
	const FName Os = OsDeLaMain(true);
	if (!M && !Os.IsNone())
	{
		AttacherFixe(Arme, true, Longueur * Taille, false);
		return;
	}
	if (!M || Os.IsNone())
	{
		return;
	}
	USkeletalMeshComponent* A = NewObject<USkeletalMeshComponent>(this);
	A->SetSkeletalMesh(M);
	A->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	A->SetupAttachment(Modele, Os);
	A->RegisterComponent();
	const float Mesure = M->GetBounds().GetBox().GetSize().GetMax();
	if (Mesure > 1.0f)
	{
		A->SetWorldScale3D(FVector(Longueur * Taille / Mesure));
	}
	Armes.Add(A);
}

// ===================== Le mouvement =====================

float AVespUnite::Ralentissement() const
{
	if (Fige > 0.0f || Etourdi > 0.0f)
	{
		return 0.0f;
	}
	return Gel > 0.0f ? 0.5f : 1.0f;
}

void AVespUnite::Geler(float Duree)
{
	if (Gel > 0.0f)
	{
		Fige = FMath::Max(Fige, 0.9f);			// deja gele : il est pris dans la glace un instant
		AfficherMessage(TEXT("FIGE"), FColor(150, 210, 255));
	}
	Gel = FMath::Max(Gel, Duree);
}

FVector AVespUnite::Deplacer(const FVector& VitesseVoulue, float Secondes, bool bTourner)
{
	if (!EstDebout() || EstEnLAir())
	{
		return FVector::ZeroVector;
	}
	const FVector Pas = FVector(VitesseVoulue.X, VitesseVoulue.Y, 0.0f) * Secondes * Ralentissement();
	const FVector Ici = GetActorLocation();
	const FVector Vers = Monde ? Monde->Contraindre(Ici, Ici + Pas) : Ici + Pas;
	SetActorLocation(Vers);
	if (bTourner && Pas.SizeSquared2D() > 0.01f && Ralentissement() > 0.0f)
	{
		Tourner(Pas, Secondes);
	}
	const FVector Fait = Vers - Ici;
	// Un peu de poussiere sous les pas
	DistancePoussiere += Fait.Size2D();
	if (DistancePoussiere > (bAylis ? 210.0f : 260.0f))
	{
		DistancePoussiere = 0.0f;
		AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, GetActorLocation(), FVector::UpVector,
		                  bAylis ? FLinearColor(0.45f, 0.45f, 0.7f) : FLinearColor(0.5f, 0.35f, 0.3f));
		if (bAylis)
		{
			UVespSons::Jouer(this, EVespSon::Pas, GetActorLocation(), 0.35f);
		}
	}
	return Fait;
}

void AVespUnite::Tourner(const FVector& Direction, float Secondes, float Vivacite)
{
	if (Direction.SizeSquared2D() < 0.0001f)
	{
		return;
	}
	const FRotator Vise(0, Direction.Rotation().Yaw, 0);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Vise, Secondes, Vivacite));
}

void AVespUnite::TournerDUnCoup(const FVector& Direction)
{
	if (Direction.SizeSquared2D() > 0.0001f)
	{
		SetActorRotation(FRotator(0, Direction.Rotation().Yaw, 0));
	}
}

void AVespUnite::Teleporter(const FVector& Position)
{
	SetActorLocation(Position);
	DernierePosition = Position;
	Poussee = FVector::ZeroVector;
	TempsSaut = DureeSaut = 0.0f;
}

void AVespUnite::Bondir(const FVector& Arrivee, float Duree, float Hauteur)
{
	DepartSaut = GetActorLocation();
	ArriveeSaut = FVector(Arrivee.X, Arrivee.Y, DepartSaut.Z);
	DureeSaut = FMath::Max(0.1f, Duree);
	TempsSaut = 0.0f;
	HauteurSaut = Hauteur;
	Jouer(EVespGeste::Saut, 1.0f);
}

// ===================== A chaque image =====================

void AVespUnite::Tick(float Secondes)
{
	Super::Tick(Secondes);
	TempsVie += Secondes;

	// Le texte au-dessus de la tete regarde toujours la camera, et monte en s'effacant
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Texte->SetWorldRotation((Camera->GetCameraLocation() - Texte->GetComponentLocation()).Rotation());
	}
	if (TempsTexte > 0.0f)
	{
		TempsTexte -= Secondes;
		Texte->SetRelativeLocation(FVector(0, 0, Taille + 70.0f + (1.0f - FMath::Max(0.0f, TempsTexte) / DureeTexte) * 90.0f));
		if (TempsTexte <= 0.0f)
		{
			Texte->SetText(FText::GetEmpty());
		}
	}
	DegatsRecus = FMath::Max(0.0f, DegatsRecus - Secondes * FMath::Max(8.0f, Stats.PvMax * 0.6f));

	// La lueur : l'eclat d'un coup, l'annonce d'une attaque (elle pulse), ou sa couleur normale
	if (Lueur->IsVisible())
	{
		if (TempsEclat > 0.0f)
		{
			TempsEclat -= Secondes;
			Lueur->SetIntensity(IntensiteLueur + 7000.0f);
			Lueur->SetLightColor(CouleurEclat);
		}
		else if (TempsAnnonce > 0.0f)
		{
			TempsAnnonce -= Secondes;
			Lueur->SetIntensity(IntensiteLueur + 2500.0f + FMath::Sin(TempsVie * 30.0f) * 1500.0f);
			Lueur->SetLightColor(CouleurAnnonce);
		}
		else
		{
			Lueur->SetIntensity(IntensiteLueur);
			Lueur->SetLightColor(CouleurLueur);
		}
	}

	// La vitesse reelle (ce qu'on l'a fait marcher depuis la derniere image, sans les poussees ni les sauts) :
	// la marche et la course suivent ce qu'il fait vraiment
	if (Secondes > 0.0f)
	{
		VitesseActuelle = FVector::Dist2D(GetActorLocation(), DernierePosition) / Secondes;
		if (VitesseActuelle > 2000.0f)
		{
			VitesseActuelle = 0.0f;		// un bond (une faille, un placement) : ce n'est pas une course
		}
	}
	if (UVespAnimInstance* A = Animateur())
	{
		A->SetVitesse(EstDebout() && !EstEnLAir() ? VitesseActuelle : 0.0f);
	}
	struct FALaFin
	{
		AVespUnite* U;
		~FALaFin() { U->DernierePosition = U->GetActorLocation(); }
	} ALaFin{this};

	if (!EstDebout())
	{
		TempsMort += Secondes;
		// Apres un moment, le corps s'enfonce dans le sol et disparait
		if (TempsMort > 5.0f && !bAylis)
		{
			AddActorWorldOffset(FVector(0, 0, -40.0f * Secondes));
		}
		Poussee = FMath::VInterpTo(Poussee, FVector::ZeroVector, Secondes, 6.0f);
		if (Monde && !Poussee.IsNearlyZero())
		{
			SetActorLocation(Monde->Contraindre(GetActorLocation(), GetActorLocation() + Poussee * Secondes));
		}
		return;
	}

	// Les etats
	Poison = FMath::Max(0.0f, Poison - Secondes);
	Brulure = FMath::Max(0.0f, Brulure - Secondes);
	Gel = FMath::Max(0.0f, Gel - Secondes);
	Fige = FMath::Max(0.0f, Fige - Secondes);
	Etourdi = FMath::Max(0.0f, Etourdi - Secondes);
	Invulnerable = FMath::Max(0.0f, Invulnerable - Secondes);
	TempsBrise = FMath::Max(0.0f, TempsBrise - Secondes);
	Modele->GlobalAnimRateScale = Fige > 0.0f ? 0.0f : (Gel > 0.0f ? 0.65f : 1.0f);
	TicEtats += Secondes;
	if (TicEtats >= 1.0f)
	{
		TicEtats = 0.0f;
		int32 Total = 0;
		if (Poison > 0.0f) Total += FMath::Max(1, FMath::RoundToInt(Stats.PvMax * 0.02f));
		if (Brulure > 0.0f) Total += FMath::Max(2, FMath::RoundToInt(Stats.PvMax * 0.03f));
		if (Total > 0)
		{
			Stats.Pv = FMath::Max(bAylis ? 1 : 0, Stats.Pv - Total);		// les etats seuls ne font pas tomber AYLIS
			DegatsRecus += Total;
			AfficherMessage(FString::Printf(TEXT("-%d"), Total), Brulure > 0.0f ? FColor(255, 150, 60) : FColor(170, 240, 90), 30.0f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, GetActorLocation(), FVector::UpVector,
			                  Brulure > 0.0f ? FLinearColor(1.0f, 0.5f, 0.15f) : FLinearColor(0.6f, 1.0f, 0.3f));
			if (!EstDebout())
			{
				Mourir();
				return;
			}
		}
	}

	// Le saut (un arc de cercle), puis l'atterrissage
	if (TempsSaut < DureeSaut)
	{
		TempsSaut += Secondes;
		const float T = FMath::Clamp(TempsSaut / DureeSaut, 0.0f, 1.0f);
		FVector P = FMath::Lerp(DepartSaut, ArriveeSaut, T);
		P.Z = DepartSaut.Z + FMath::Sin(T * PI) * HauteurSaut;
		SetActorLocation(P);
		if (T >= 1.0f)
		{
			SetActorLocation(Monde ? Monde->Contraindre(DepartSaut, ArriveeSaut) : ArriveeSaut);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.6f, 0.45f, 0.35f));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, GetActorLocation(), FVector::UpVector, FLinearColor(0.5f, 0.4f, 0.35f));
		}
	}

	// La poussee d'un coup : il glisse, puis s'arrete
	if (!Poussee.IsNearlyZero(1.0f))
	{
		const FVector Ici = GetActorLocation();
		SetActorLocation(Monde ? Monde->Contraindre(Ici, Ici + Poussee * Secondes) : Ici + Poussee * Secondes);
		Poussee = FMath::VInterpTo(Poussee, FVector::ZeroVector, Secondes, 7.0f);
	}

	// Le corps : il respire, s'ecrase sous un coup
	float Ecrase = 0.0f;
	if (TempsEcrase > 0.0f)
	{
		TempsEcrase -= Secondes;
		Ecrase = FMath::Sin(FMath::Clamp(1.0f - TempsEcrase / 0.22f, 0.0f, 1.0f) * PI) * 0.14f;
	}
	const float Souffle = FMath::Sin(TempsVie * 2.2f) * 0.012f;
	const float Haut = 1.0f - Ecrase + Souffle;
	const float Large = 1.0f + Ecrase * 0.7f - Souffle * 0.5f;
	if (Modele->GetSkeletalMeshAsset())
	{
		Modele->SetRelativeScale3D(FVector(Large, Large, Haut) * EchelleModele);
	}
	else
	{
		Corps->SetRelativeScale3D(FVector(0.45f * Large, 0.45f * Large, Haut));
	}
}

// ===================== Les coups =====================

void AVespUnite::Encaisser(int32 Degats, bool bCritique, const FVector& Direction, float Force, bool bSansFlechir)
{
	if (!EstDebout())
	{
		return;
	}
	Stats.Pv = FMath::Max(0, Stats.Pv - Degats);
	DegatsRecus += Degats;
	TempsEclat = bCritique ? 0.16f : 0.1f;
	CouleurEclat = bCritique ? FLinearColor(1.0f, 0.8f, 0.45f) : FLinearColor(1.0f, 0.92f, 0.85f);
	DirectionCoup = Direction.GetSafeNormal2D();
	Texte->SetText(FText::FromString(FString::Printf(TEXT("%s%d"), bCritique ? TEXT("CRIT ") : TEXT(""), Degats)));
	Texte->SetTextRenderColor(bAylis ? FColor(255, 110, 110) : (bCritique ? FColor(255, 170, 60) : FColor(255, 235, 150)));
	Texte->SetWorldSize(bCritique ? 60.0f : 42.0f);
	TempsTexte = DureeTexte = bCritique ? 1.0f : 0.8f;
	Pousser(DirectionCoup * Force);
	TempsEcrase = 0.22f;
	if (!EstDebout())
	{
		Mourir();
		return;
	}
	if (bSansFlechir)
	{
		return;
	}
	Equilibre += Degats;
	if (Equilibre >= SeuilEquilibre)
	{
		Equilibre = 0.0f;
		Jouer(EVespGeste::Touche, 1.25f);
	}
}

void AVespUnite::Mourir()
{
	Lueur->SetVisibility(false);
	Poison = Brulure = Gel = Fige = 0.0f;
	Modele->GlobalAnimRateScale = 1.0f;
	TenirGarde(false);
	if (bAnime)
	{
		Jouer(EVespGeste::Mort, 1.0f, false, true);
	}
	else
	{
		SetActorRotation(FRotator(0, GetActorRotation().Yaw, 90));	// la silhouette se couche
	}
	// L'ame s'echappe
	AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, GetActorLocation(), FVector::UpVector,
	                  bAylis ? FLinearColor(0.5f, 0.55f, 1.0f) : FLinearColor(0.75f, 0.3f, 1.0f), 0.3f);
	TempsMort = 0.0f;
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

void AVespUnite::AfficherMessage(const FString& Message, FColor Couleur, float TailleTexte)
{
	Texte->SetText(FText::FromString(Message));
	Texte->SetTextRenderColor(Couleur);
	Texte->SetWorldSize(TailleTexte);
	TempsTexte = 1.0f;
	DureeTexte = 1.0f;
}

void AVespUnite::Annoncer(float Duree, const FLinearColor& Couleur)
{
	TempsAnnonce = Duree;
	CouleurAnnonce = Couleur;
}

void AVespUnite::Surbrillance(float Duree, const FLinearColor& Couleur)
{
	TempsEclat = Duree;
	CouleurEclat = Couleur;
}
