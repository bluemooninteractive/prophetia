#include "VespGameMode.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "VespPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

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
