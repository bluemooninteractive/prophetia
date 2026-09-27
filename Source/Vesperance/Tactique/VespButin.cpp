#include "VespButin.h"
#include "VespEffet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AVespButin::AVespButin()
{
	PrimaryActorTick.bCanEverTick = true;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeCylindre(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LaSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	auto Creer = [this](const TCHAR* Nom, UStaticMesh* M) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Nom);
		C->SetupAttachment(Racine);
		C->SetStaticMesh(M);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		return C;
	};
	Faisceau = Creer(TEXT("Faisceau"), LeCylindre.Object);
	Faisceau->SetRelativeLocation(FVector(0, 0, 260));
	Faisceau->SetRelativeScale3D(FVector(0.05f, 0.05f, 5.2f));
	Socle = Creer(TEXT("Socle"), LeCylindre.Object);
	Socle->SetRelativeLocation(FVector(0, 0, 2));
	Socle->SetRelativeScale3D(FVector(1.1f, 1.1f, 0.01f));
	Gemme = Creer(TEXT("Gemme"), LaSphere.Object);
	Gemme->SetRelativeLocation(FVector(0, 0, 70));
	Gemme->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.26f));
	Arme = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arme"));
	Arme->SetupAttachment(Racine);
	Arme->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Arme->SetRelativeLocation(FVector(0, 0, 60));
	Lumiere = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lumiere"));
	Lumiere->SetupAttachment(Racine);
	Lumiere->SetRelativeLocation(FVector(0, 0, 90));
	Lumiere->SetAttenuationRadius(420.0f);
	Lumiere->SetCastShadows(false);
}

void AVespButin::Preparer(const FVespObjet& LObjet)
{
	Objet = LObjet;
	const FLinearColor C = VespButin::CouleurRarete(Objet.Rarete);
	Couleur = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	Couleur->SetVectorParameterValue(TEXT("Color"), C * 1.5f);
	Faisceau->SetMaterial(0, Couleur);
	Gemme->SetMaterial(0, Couleur);
	UMaterialInstanceDynamic* Sol = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	Sol->SetVectorParameterValue(TEXT("Color"), C * 0.35f);
	Socle->SetMaterial(0, Sol);
	// Le faisceau : plus haut et plus vif pour les objets rares
	const float Rang = (float)Objet.Rarete;
	Faisceau->SetRelativeScale3D(FVector(0.04f + Rang * 0.015f, 0.04f + Rang * 0.015f, 3.0f + Rang * 1.5f));
	Faisceau->SetRelativeLocation(FVector(0, 0, (3.0f + Rang * 1.5f) * 50.0f));
	Lumiere->SetLightColor(C);
	Lumiere->SetIntensity(1500.0f + Rang * 1500.0f);
	USkeletalMesh* M = Objet.Modele.IsEmpty() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *Objet.Modele, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (M)
	{
		Arme->SetSkeletalMesh(M);
		const float Mesure = M->GetBounds().GetBox().GetSize().GetMax();
		Arme->SetRelativeScale3D(FVector(Mesure > 1.0f ? 95.0f / Mesure : 1.0f));
		Gemme->SetVisibility(false);
	}
	else
	{
		Arme->SetVisibility(false);
	}
	Depart = GetActorLocation();
	Vol = FVector(FMath::FRandRange(-160.0f, 160.0f), FMath::FRandRange(-160.0f, 160.0f), 0.0f);
}

void AVespButin::Tick(float Secondes)
{
	Super::Tick(Secondes);
	Age += Secondes;
	// Il jaillit en arc, puis se pose
	const float T = FMath::Clamp(Age / 0.55f, 0.0f, 1.0f);
	SetActorLocation(Depart + Vol * T);
	const float Saut = FMath::Sin(T * PI) * 120.0f;
	const float Flotte = 10.0f * FMath::Sin(Age * 2.2f);
	Arme->SetRelativeLocation(FVector(0, 0, 60.0f + Saut + Flotte));
	Gemme->SetRelativeLocation(FVector(0, 0, 70.0f + Saut + Flotte));
	Arme->SetRelativeRotation(FRotator(20.0f, Age * 60.0f, 0.0f));
	Gemme->SetRelativeRotation(FRotator(0.0f, Age * 90.0f, 0.0f));
	if (Couleur)
	{
		Couleur->SetVectorParameterValue(TEXT("Color"), VespButin::CouleurRarete(Objet.Rarete) * (1.2f + 0.5f * FMath::Sin(Age * 3.0f)));
	}
}
