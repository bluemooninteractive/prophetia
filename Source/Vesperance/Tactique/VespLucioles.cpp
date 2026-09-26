#include "VespLucioles.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AVespLucioles::AVespLucioles()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
}

void AVespLucioles::BeginPlay()
{
	Super::BeginPlay();
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	// Les couleurs de la prophetie : cyan, violet, rose
	const FLinearColor Couleurs[3] = {FLinearColor(0.55f, 0.9f, 0.95f), FLinearColor(0.8f, 0.6f, 1.0f), FLinearColor(1.0f, 0.75f, 0.9f)};
	for (int32 i = 0; i < 26; i++)
	{
		FFeuFollet F;
		F.Centre = GetActorLocation() + FVector(FMath::FRandRange(-500.0f, 1600.0f), FMath::FRandRange(-1700.0f, 1700.0f), FMath::FRandRange(70.0f, 260.0f));
		F.Rayon = FMath::FRandRange(80.0f, 220.0f);
		F.Vitesse = FMath::FRandRange(0.25f, 0.6f) * (FMath::RandBool() ? 1 : -1);
		F.Phase = FMath::FRandRange(0.0f, 6.28f);
		Feux.Add(F);

		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetupAttachment(RootComponent);
		L->RegisterComponent();
		L->SetLightColor(Couleurs[i % 3]);
		L->SetIntensity(500.0f);
		L->SetAttenuationRadius(260.0f);
		L->SetCastShadows(false);
		Lumieres.Add(L);

		// Une petite boule claire au coeur de la lumiere (le feu follet lui-meme)
		UStaticMeshComponent* B = NewObject<UStaticMeshComponent>(this);
		B->SetupAttachment(RootComponent);
		B->SetStaticMesh(Sphere);
		B->SetWorldScale3D(FVector(0.06f));
		B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		B->SetCastShadow(false);
		B->RegisterComponent();
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
		M->SetVectorParameterValue(TEXT("Color"), Couleurs[i % 3]);
		B->SetMaterial(0, M);
		Boules.Add(B);
	}
}

void AVespLucioles::Tick(float Secondes)
{
	Super::Tick(Secondes);
	Temps += Secondes;
	for (int32 i = 0; i < Feux.Num(); i++)
	{
		const FFeuFollet& F = Feux[i];
		const float A = Temps * F.Vitesse + F.Phase;
		// Une boucle un peu ovale, qui monte et descend doucement
		const FVector Position = F.Centre + FVector(FMath::Cos(A) * F.Rayon, FMath::Sin(A * 1.3f) * F.Rayon, FMath::Sin(A * 2.1f) * 30.0f);
		Lumieres[i]->SetWorldLocation(Position);
		Boules[i]->SetWorldLocation(Position);
		Lumieres[i]->SetIntensity(380.0f + 220.0f * FMath::Sin(Temps * 3.0f + F.Phase));	// il palpite
	}
}
