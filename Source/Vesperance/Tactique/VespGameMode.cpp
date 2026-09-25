#include "VespGameMode.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "VespPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"

// ===================== La nuit de la Foret des Brumes =====================
// On transforme le niveau "Basic" : la lune a la place du soleil, un ciel sombre, une brume bleu-vert,
// un sol de mousse sous l'arene, et des lumieres magiques (lanternes, autels) autour.
static void AmbianceDeNuit(UWorld* Monde, const FVector& Centre, float ExpositionImage)
{
	for (TActorIterator<ADirectionalLight> It(Monde); It; ++It)
	{
		UDirectionalLightComponent* Lune = Cast<UDirectionalLightComponent>(It->GetLightComponent());
		Lune->SetIntensity(3.0f);
		Lune->SetLightColor(FLinearColor(0.55f, 0.65f, 1.0f));
		It->SetActorRotation(FRotator(-35.0f, 140.0f, 0.0f));	// une lune basse, qui allonge les ombres
	}
	for (TActorIterator<ASkyLight> It(Monde); It; ++It)
	{
		It->GetLightComponent()->SetIntensity(0.35f);
		It->GetLightComponent()->SetLightColor(FLinearColor(0.35f, 0.4f, 0.8f));
	}
	for (TActorIterator<AExponentialHeightFog> It(Monde); It; ++It)
	{
		UExponentialHeightFogComponent* Brume = It->GetComponent();
		Brume->SetFogDensity(0.006f);
		Brume->SetFogInscatteringColor(FLinearColor(0.05f, 0.12f, 0.16f));
		Brume->SetVolumetricFog(true);
	}
	// Un grand tapis de mousse sombre sous l'arene (il cache le sol a damier du niveau)
	UStaticMesh* Plan = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	AStaticMeshActor* Mousse = Monde->SpawnActor<AStaticMeshActor>(Centre - FVector(0, 0, 4), FRotator::ZeroRotator);	// sous la grille
	Mousse->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	Mousse->GetStaticMeshComponent()->SetStaticMesh(Plan);
	Mousse->SetActorScale3D(FVector(60.0f, 60.0f, 1.0f));
	UMaterialInstanceDynamic* Couleur = UMaterialInstanceDynamic::Create(Base, Mousse);
	Couleur->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.05f, 0.04f));
	Mousse->GetStaticMeshComponent()->SetMaterial(0, Couleur);
	// Les lumieres magiques : des lanternes dorees le long de l'arene, et deux autels (violet et cyan)
	struct FLumiere { FVector Position; FLinearColor Couleur; float Intensite; float Rayon; };
	const FLumiere Lumieres[] = {
		{FVector(-450, -700, 80), FLinearColor(1.0f, 0.65f, 0.3f), 900.0f, 450.0f},
		{FVector(450, -300, 80), FLinearColor(1.0f, 0.65f, 0.3f), 900.0f, 450.0f},
		{FVector(-450, 450, 80), FLinearColor(1.0f, 0.65f, 0.3f), 900.0f, 450.0f},
		{FVector(300, 700, 80), FLinearColor(1.0f, 0.65f, 0.3f), 900.0f, 450.0f},
		{FVector(-300, -150, 160), FLinearColor(0.7f, 0.45f, 1.0f), 1800.0f, 650.0f},
		{FVector(200, 350, 160), FLinearColor(0.4f, 0.9f, 1.0f), 1800.0f, 650.0f},
	};
	// L'image : une exposition fixe (sinon Unreal eclaircit la nuit tout seul, comme un appareil photo),
	// des couleurs un peu plus froides, et les bords de l'ecran assombris
	APostProcessVolume* Image = Monde->SpawnActor<APostProcessVolume>();
	Image->bUnbound = true;
	FPostProcessSettings& R = Image->Settings;
	R.bOverride_AutoExposureMethod = true;
	R.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	R.bOverride_AutoExposureBias = true;
	R.AutoExposureBias = ExpositionImage;
	R.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	R.AutoExposureApplyPhysicalCameraExposure = false;	// pas de "reglages d appareil photo" : la lumiere de la scene telle quelle
	R.bOverride_VignetteIntensity = true;
	R.VignetteIntensity = 0.7f;
	R.bOverride_ColorSaturation = true;
	R.ColorSaturation = FVector4(0.9f, 0.95f, 1.1f, 1.0f);
	R.bOverride_BloomIntensity = true;
	R.BloomIntensity = 1.2f;

	for (const FLumiere& L : Lumieres)
	{
		APointLight* Lampe = Monde->SpawnActor<APointLight>(Centre + L.Position, FRotator::ZeroRotator);
		Lampe->SetMobility(EComponentMobility::Movable);
		Lampe->PointLightComponent->SetLightColor(L.Couleur);
		Lampe->PointLightComponent->SetIntensity(L.Intensite);
		Lampe->PointLightComponent->SetAttenuationRadius(L.Rayon);
	}
}

// Le dossier ou chaque personnage range son modele 3D (voir l'import des modeles KayKit)
static const FString DOSSIER = TEXT("/Game/Characters/");

AVespGameMode::AVespGameMode()
{
	PlayerControllerClass = AVespPlayerController::StaticClass();
	DefaultPawnClass = nullptr;		// pas de personnage a diriger au clavier : on joue a la souris, sur la grille
}

// Les stats, comme dans le prototype (route.cpp)
static FVespStats Stats(const TCHAR* Nom, int32 Pv, int32 Attaque, int32 Defense)
{
	FVespStats S;
	S.Nom = Nom;
	S.Pv = Pv;
	S.PvMax = Pv;
	S.Attaque = Attaque;
	S.Defense = Defense;
	return S;
}

void AVespGameMode::BeginPlay()
{
	Super::BeginPlay();
	UWorld* Monde = GetWorld();

	// 1. L'arene, au centre du monde (un peu au-dessus du sol du niveau)
	AVespGrille* Grille = Monde->SpawnActor<AVespGrille>(FVector(0, 0, 5), FRotator::ZeroRotator);

	AmbianceDeNuit(Monde, Grille->GetActorLocation(), Exposition);

	// 2. AYLIS, a gauche de l'arene (colonne 1, ligne 3), voie de l'epee
	AVespUnite* Aylis = Monde->SpawnActor<AVespUnite>(FVector::ZeroVector, FRotator::ZeroRotator);
	Aylis->Preparer(Grille, FIntPoint(1, 3), Stats(TEXT("AYLIS"), 44, 13, 4), DOSSIER + TEXT("Aylis"), FLinearColor(0.2f, 0.35f, 1.0f), true);

	// 3. Les Haschen, a droite : un guerrier, un traqueur, un eclaireur (les stats du prototype)
	struct FHaschen { const TCHAR* Nom; const TCHAR* Dossier; FIntPoint Case; int32 Pv, Attaque, Defense; FLinearColor Teinte; };
	const FHaschen Groupe[] = {
		{TEXT("Haschen guerrier"), TEXT("guerrier"), FIntPoint(9, 2), 22, 9, 2, FLinearColor(0.8f, 0.2f, 0.15f)},
		{TEXT("Haschen traqueur"), TEXT("traqueur"), FIntPoint(10, 5), 20, 9, 1, FLinearColor(0.2f, 0.6f, 0.25f)},
		{TEXT("Haschen eclaireur"), TEXT("sbire"), FIntPoint(8, 3), 18, 8, 1, FLinearColor(0.9f, 0.5f, 0.1f)},
	};
	TArray<AVespUnite*> Haschen;
	for (const FHaschen& H : Groupe)
	{
		AVespUnite* Unite = Monde->SpawnActor<AVespUnite>(FVector::ZeroVector, FRotator(0, 180, 0));
		Unite->Preparer(Grille, H.Case, Stats(H.Nom, H.Pv, H.Attaque, H.Defense), DOSSIER + H.Dossier, H.Teinte, false);
		Haschen.Add(Unite);
	}

	// 4. La camera : vue de haut et un peu de cote, presque sans perspective (un angle de vue etroit)
	const float Elevation = FMath::DegreesToRadians(ElevationCamera);
	const float Azimut = FMath::DegreesToRadians(AzimutCamera);
	const FVector Cible = Grille->GetActorLocation();
	const FVector Position = Cible + FVector(-FMath::Cos(Elevation) * FMath::Cos(Azimut), FMath::Cos(Elevation) * FMath::Sin(Azimut),
	                                        FMath::Sin(Elevation)) * DistanceCamera;
	ACameraActor* Camera = Monde->SpawnActor<ACameraActor>(Position, (Cible - Position).Rotation());
	Camera->GetCameraComponent()->SetFieldOfView(42.0f);

	// 5. Le joueur regarde par cette camera, et le combat commence
	if (AVespPlayerController* Joueur = Cast<AVespPlayerController>(Monde->GetFirstPlayerController()))
	{
		Joueur->SetViewTarget(Camera);
		Joueur->Commencer(Grille, Aylis, Haschen);
	}
}
