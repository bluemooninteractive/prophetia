#include "VespGameMode.h"
#include "VespCombat.h"
#include "VespUnite.h"
#include "VespPlayerController.h"
#include "VespHUD.h"
#include "VespLucioles.h"
#include "VespMonde.h"
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
// On transforme le niveau "Basic" : la lune a la place du soleil, un ciel sombre, une brume bleu-vert.
// (le sol, la foret, les lanternes : c'est le monde de l'acte, VespMonde, qui les construit)
static void AmbianceDeNuit(UWorld* Monde, float ExpositionImage)
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
	// Des contours "toon" (le filtre du pack StylizedProvencal) : ils unifient le rendu des differents packs
	// (personnages KayKit, decors peints, armes, et la corruption du Voile). F4 les coupe ou les remet.
	if (UMaterialInterface* Contours = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StylizedProvencal/Materials/MI_PP_ToonOutlines.MI_PP_ToonOutlines"),
	                                                                  nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		R.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Contours));
	}

}

// Le dossier ou chaque personnage range son modele 3D
static const FString DOSSIER = TEXT("/Game/Characters/");

AVespGameMode::AVespGameMode()
{
	PlayerControllerClass = AVespPlayerController::StaticClass();
	HUDClass = AVespHUD::StaticClass();		// l'interface du combat
	DefaultPawnClass = nullptr;		// AYLIS est un acteur a part, dirige par le PlayerController
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

	// 1. Le monde de l'acte (construit par le PlayerController), et le combat (les Haschen, les attaques annoncees)
	AVespMonde* LeMonde = Monde->SpawnActor<AVespMonde>(FVector(0, 0, 5), FRotator::ZeroRotator);
	AVespCombat* LeCombat = Monde->SpawnActor<AVespCombat>(FVector::ZeroVector, FRotator::ZeroRotator);

	AmbianceDeNuit(Monde, Exposition);

	// 2. AYLIS, voie de l'epee
	AVespUnite* Aylis = Monde->SpawnActor<AVespUnite>(FVector(0, 0, 5), FRotator::ZeroRotator);
	Aylis->Vitesse = 470.0f;
	Aylis->Preparer(LeMonde, Stats(TEXT("AYLIS"), 70, 13, 3), DOSSIER + TEXT("Aylis"), FLinearColor(0.2f, 0.35f, 1.0f), true);

	// 3. La magie : des feux follets qui flottent autour d'AYLIS
	Monde->SpawnActor<AVespLucioles>(FVector(0, 0, 5), FRotator::ZeroRotator);

	// 4. La camera : vue de haut et un peu de cote, presque sans perspective (un angle de vue etroit)
	const float Elevation = FMath::DegreesToRadians(ElevationCamera);
	const float Azimut = FMath::DegreesToRadians(AzimutCamera);
	const FVector Cible = LeMonde->GetActorLocation();
	const FVector Position = Cible + FVector(-FMath::Cos(Elevation) * FMath::Cos(Azimut), FMath::Cos(Elevation) * FMath::Sin(Azimut),
	                                        FMath::Sin(Elevation)) * DistanceCamera;
	ACameraActor* Camera = Monde->SpawnActor<ACameraActor>(Position, (Cible - Position).Rotation());
	Camera->GetCameraComponent()->SetFieldOfView(42.0f);

	// 5. Le joueur regarde par cette camera, et le combat commence
	if (AVespPlayerController* Joueur = Cast<AVespPlayerController>(Monde->GetFirstPlayerController()))
	{
		Joueur->SetViewTarget(Camera);
		Joueur->Commencer(LeMonde, LeCombat, Aylis, Camera);
	}
}
