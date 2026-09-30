#include "VespMeteo.h"
#include "VespEffet.h"
#include "VespSons.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

AVespMeteo::AVespMeteo()
{
	PrimaryActorTick.bCanEverTick = true;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;
}

void AVespMeteo::Vider()
{
	for (UInstancedStaticMeshComponent* C : Composants)
	{
		if (C) C->DestroyComponent();
	}
	Composants.Reset();
	Couches.Reset();
}

void AVespMeteo::AjouterCouche(UStaticMesh* Modele, UMaterialInterface* Materiau, int32 Nombre, const FVector& Echelle, float Vitesse,
                               bool bMonte, bool bEtire, bool bTournoie, float Ondulation)
{
	if (!Modele)
	{
		return;
	}
	FCouche C;
	C.Grains = NewObject<UInstancedStaticMeshComponent>(this);
	C.Grains->SetupAttachment(Racine);
	C.Grains->SetStaticMesh(Modele);
	C.Grains->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C.Grains->SetCastShadow(false);
	C.Grains->SetMobility(EComponentMobility::Movable);
	C.Grains->RegisterComponent();
	C.Grains->SetAbsolute(true, true, true);
	if (Materiau)
	{
		C.Grains->SetMaterial(0, Materiau);
	}
	C.Echelle = Echelle;
	C.Vitesse = Vitesse;
	C.bMonte = bMonte;
	C.bEtire = bEtire;
	C.bTournoie = bTournoie;
	C.Ondulation = Ondulation;
	TArray<FTransform> Depart;
	for (int32 i = 0; i < Nombre; i++)
	{
		C.Positions.Add(FVector::ZeroVector);
		C.Vitesses.Add(FVector::ZeroVector);
		C.Phases.Add(FMath::FRandRange(0.0f, 6.28f));
		Depart.Add(FTransform(FVector::ZeroVector));
	}
	C.Grains->AddInstances(Depart, false, true);
	for (int32 i = 0; i < Nombre; i++)
	{
		Placer(C, i, true);
	}
	Composants.Add(C.Grains);
	Couches.Add(C);
}

void AVespMeteo::Placer(FCouche& C, int32 i, bool bAuHasard)
{
	const FVector Centre = Cible ? Cible->GetActorLocation() : GetActorLocation();
	const float Z = bAuHasard ? FMath::FRandRange(0.0f, Hauteur) : (C.bMonte ? FMath::FRandRange(0.0f, 60.0f) : Hauteur * FMath::FRandRange(0.85f, 1.0f));
	C.Positions[i] = Centre + FVector(FMath::FRandRange(-DemiBoite, DemiBoite), FMath::FRandRange(-DemiBoite, DemiBoite), Z);
	const float V = C.Vitesse * FMath::FRandRange(0.75f, 1.25f);
	C.Vitesses[i] = FVector(0.0f, 0.0f, C.bMonte ? V : -V);
}

void AVespMeteo::Configurer(int32 LActe)
{
	Vider();
	Acte = FMath::Clamp(LActe, 1, 7);
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		Lune = *It;
		IntensiteLune = It->GetLightComponent()->Intensity;
		break;
	}
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Feuille = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Fantastic_Village_Pack/meshes/environment/SM_ENV_PLANT_leaf_village.SM_ENV_PLANT_leaf_village"),
	                                               nullptr, LOAD_NoWarn | LOAD_Quiet);
	auto Lumineux = [this](const FLinearColor& C) {
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
		M->SetVectorParameterValue(TEXT("Color"), C);
		return M;
	};
	auto Mat = [this](const FLinearColor& C) {
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), this);
		M->SetVectorParameterValue(TEXT("Color"), C);
		return M;
	};
	bEclairs = false;
	bAverses = false;
	Averse = 0.0f;
	ForcePluie = 0.0f;
	ProchaineAverse = FMath::FRandRange(12.0f, 25.0f);
	ProchainEclair = FMath::FRandRange(8.0f, 14.0f);
	UMaterialInterface* Pluie = Lumineux(FLinearColor(0.35f, 0.45f, 0.6f) * 0.5f);
	switch (Acte)
	{
		case 1:
			AjouterCouche(Feuille, nullptr, 70, FVector(0.22f), 90.0f, false, false, true, 70.0f);
			break;
		case 2:
			AjouterCouche(Feuille, Mat(FLinearColor(0.35f, 0.22f, 0.1f)), 120, FVector(0.22f), 110.0f, false, false, true, 90.0f);
			AjouterCouche(Cube, Pluie, 1300, FVector(0.012f, 0.012f, 0.75f), 1500.0f, false, true, false, 0.0f);
			bAverses = true;
			break;
		case 3:
			AjouterCouche(Cube, Pluie, 700, FVector(0.01f, 0.01f, 0.5f), 1100.0f, false, true, false, 0.0f);
			AjouterCouche(Sphere, Lumineux(FLinearColor(0.5f, 1.0f, 0.6f) * 1.5f), 60, FVector(0.03f), 35.0f, true, false, false, 60.0f);
			ForcePluie = 0.45f;
			break;
		case 4:
			AjouterCouche(Sphere, Lumineux(FLinearColor(1.0f, 0.45f, 0.1f) * 4.0f), 140, FVector(0.03f), 70.0f, true, false, false, 80.0f);
			AjouterCouche(Cube, Mat(FLinearColor(0.45f, 0.4f, 0.35f)), 250, FVector(0.03f), 40.0f, false, false, true, 160.0f);
			break;
		case 5:
			AjouterCouche(Sphere, Lumineux(FLinearColor(0.85f, 0.9f, 1.0f) * 0.6f), 1100, FVector(0.06f), 150.0f, false, false, false, 60.0f);
			break;
		case 6:
			AjouterCouche(Cube, Mat(FLinearColor(0.3f, 0.29f, 0.3f)), 800, FVector(0.045f, 0.045f, 0.015f), 110.0f, false, false, true, 70.0f);
			AjouterCouche(Sphere, Lumineux(FLinearColor(1.0f, 0.35f, 0.05f) * 5.0f), 200, FVector(0.035f), 120.0f, true, false, false, 90.0f);
			break;
		default:
			AjouterCouche(Sphere, Lumineux(FLinearColor(0.65f, 0.35f, 1.0f) * 3.0f), 450, FVector(0.04f), 55.0f, true, false, false, 110.0f);
			bEclairs = true;
			break;
	}
	if (UVespSons* Sons = GetWorld()->GetSubsystem<UVespSons>())
	{
		Sons->Pluie(ForcePluie);
	}
}

void AVespMeteo::Tick(float Secondes)
{
	Super::Tick(Secondes);
	Temps += Secondes;
	ForceBlizzard = FMath::FInterpTo(ForceBlizzard, bBlizzard ? 1.0f : 0.0f, Secondes, 1.5f);
	// Les averses du Bois des Pendus : elles viennent, grossissent, s'arretent (avec des eclairs)
	if (bAverses)
	{
		if (Averse > 0.0f)
		{
			Averse -= Secondes;
		}
		else if ((ProchaineAverse -= Secondes) <= 0.0f)
		{
			Averse = FMath::FRandRange(20.0f, 35.0f);
			ProchaineAverse = FMath::FRandRange(25.0f, 45.0f);
		}
		const float Voulue = Averse > 0.0f ? 0.9f : 0.0f;
		const float Avant = ForcePluie;
		ForcePluie = FMath::FInterpTo(ForcePluie, Voulue, Secondes, 0.4f);
		bEclairs = Averse > 0.0f;
		if (FMath::Abs(ForcePluie - Avant) > 0.001f)
		{
			if (UVespSons* Sons = GetWorld()->GetSubsystem<UVespSons>())
			{
				Sons->Pluie(ForcePluie);
			}
		}
	}
	// L'orage : le ciel s'illumine, puis le tonnerre arrive
	if (bEclairs && Lune)
	{
		if ((ProchainEclair -= Secondes) <= 0.0f)
		{
			ProchainEclair = FMath::FRandRange(9.0f, 16.0f);
			Eclair = 0.35f;
			Tonnerre = FMath::FRandRange(0.6f, 1.6f);
		}
	}
	if (Lune)
	{
		if (Eclair > 0.0f)
		{
			Eclair -= Secondes;
			const float Flash = FMath::Sin(Eclair * 40.0f) > 0.0f ? 1.0f : 0.3f;
			Lune->GetLightComponent()->SetIntensity(IntensiteLune + 25.0f * Flash * FMath::Clamp(Eclair / 0.35f, 0.0f, 1.0f));
			if (Eclair <= 0.0f)
			{
				Lune->GetLightComponent()->SetIntensity(IntensiteLune);
			}
		}
	}
	if (Tonnerre > 0.0f && (Tonnerre -= Secondes) <= 0.0f)
	{
		UVespSons::Jouer2D(this, EVespSon::Tonnerre, 0.7f);
	}

	// Les grains
	const FVector Centre = Cible ? Cible->GetActorLocation() : GetActorLocation();
	for (int32 c = 0; c < Couches.Num(); c++)
	{
		FCouche& C = Couches[c];
		// La pluie des averses : on ne la voit que quand il pleut
		const bool bPluie = C.bEtire;
		if (bPluie)
		{
			const bool bVisible = ForcePluie > 0.05f;
			C.Grains->SetVisibility(bVisible);
			if (!bVisible)
			{
				continue;
			}
		}
		// Le vent : le blizzard couche la neige
		const FVector Vent = FVector(700.0f, 350.0f, 0.0f) * ForceBlizzard + FVector(60.0f, 30.0f, 0.0f);
		const float Acceleration = 1.0f + ForceBlizzard * 1.5f;
		TArray<FTransform> Transformations;
		Transformations.SetNum(C.Positions.Num());
		for (int32 i = 0; i < C.Positions.Num(); i++)
		{
			FVector V = C.Vitesses[i] * Acceleration + Vent * (bPluie ? 0.3f : 1.0f);
			if (C.Ondulation > 0.0f)
			{
				V += FVector(FMath::Sin(Temps * 1.3f + C.Phases[i]), FMath::Cos(Temps * 1.1f + C.Phases[i] * 1.7f), 0.0f) * C.Ondulation;
			}
			FVector& P = C.Positions[i];
			P += V * Secondes;
			// Sortie de la boite : il reapparait de l'autre cote (ou en haut, ou en bas)
			if ((!C.bMonte && P.Z < Centre.Z) || (C.bMonte && P.Z > Centre.Z + Hauteur))
			{
				Placer(C, i, false);
			}
			if (P.X - Centre.X > DemiBoite) P.X -= 2.0f * DemiBoite;
			if (P.X - Centre.X < -DemiBoite) P.X += 2.0f * DemiBoite;
			if (P.Y - Centre.Y > DemiBoite) P.Y -= 2.0f * DemiBoite;
			if (P.Y - Centre.Y < -DemiBoite) P.Y += 2.0f * DemiBoite;
			FRotator R = FRotator::ZeroRotator;
			if (C.bEtire)
			{
				R = FRotationMatrix::MakeFromZ(V.GetSafeNormal()).Rotator();
			}
			else if (C.bTournoie)
			{
				R = FRotator(Temps * 90.0f + C.Phases[i] * 50.0f, C.Phases[i] * 57.0f, Temps * 60.0f + C.Phases[i] * 30.0f);
			}
			// Les braises et les poussieres s'eteignent en montant ; tout pulse un peu
			float E = 1.0f;
			if (C.bMonte)
			{
				E = FMath::Clamp(1.2f - (P.Z - Centre.Z) / Hauteur, 0.1f, 1.0f) * (0.8f + 0.2f * FMath::Sin(Temps * 5.0f + C.Phases[i]));
			}
			Transformations[i] = FTransform(R, P, C.Echelle * E);
		}
		C.Grains->BatchUpdateInstancesTransforms(0, Transformations, true, true, true);
	}
}
