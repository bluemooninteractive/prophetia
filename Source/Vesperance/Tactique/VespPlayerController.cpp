#include "VespPlayerController.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

AVespPlayerController::AVespPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AVespPlayerController::Annoncer(const FString& Texte, FColor Couleur, float Duree)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, Duree, Couleur, Texte, true, FVector2D(1.6f, 1.6f));
	}
}

void AVespPlayerController::Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, const TArray<AVespUnite*>& LesHaschen)
{
	Grille = LaGrille;
	Aylis = LAylis;
	Haschen.Reset();
	for (AVespUnite* H : LesHaschen)
	{
		Haschen.Add(H);
	}
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	TourDAylis();
}

bool AVespPlayerController::ResteDesHaschen() const
{
	for (const AVespUnite* H : Haschen)
	{
		if (H->EstDebout())
		{
			return true;
		}
	}
	return false;
}

// ===================== Le tour d'AYLIS =====================

void AVespPlayerController::TourDAylis()
{
	Tour++;
	Phase = EVespPhase::TourAylis;
	bADejaBouge = false;
	MontrerCasesAtteignables();
	Annoncer(FString::Printf(TEXT("TOUR %d : deplace AYLIS, puis clique sur un Haschen au contact (ESPACE : passer)"), Tour),
	         FColor(232, 192, 84));
}

void AVespPlayerController::MontrerCasesAtteignables()
{
	const bool bVisible = Phase == EVespPhase::TourAylis && !bADejaBouge;
	Grille->AfficherCasesAtteignables(bVisible ? Grille->CasesAtteignables(Aylis->GetCase(), DeplacementParTour) : TArray<int32>());
}

void AVespPlayerController::FinDuTourDAylis()
{
	Grille->AfficherCasesAtteignables({});
	if (!ResteDesHaschen())
	{
		Phase = EVespPhase::Victoire;
		Annoncer(TEXT("VICTOIRE ! Les Haschen sont tombes. (R : recommencer)"), FColor(110, 220, 120), 30.0f);
		return;
	}
	Phase = EVespPhase::TourHaschen;
	HaschenQuiJoue = 0;
	EtapeHaschen = 0;
	Minuteur = 0.0f;
	Annoncer(TEXT("TOUR DES HASCHEN"), FColor(220, 80, 80), 1.5f);
}

// La case sous la souris : on lance un rayon depuis la camera, et on regarde ou il coupe le sol
bool AVespPlayerController::CaseSousLaSouris(FIntPoint& Case) const
{
	FVector Depart, Direction;
	if (!DeprojectMousePositionToWorld(Depart, Direction) || FMath::IsNearlyZero(Direction.Z))
	{
		return false;
	}
	const float Distance = (Grille->GetActorLocation().Z - Depart.Z) / Direction.Z;
	return Distance >= 0 && Grille->CaseSousPoint(Depart + Direction * Distance, Case);
}

void AVespPlayerController::PlayerTick(float Secondes)
{
	Super::PlayerTick(Secondes);
	if (!Grille || !Aylis)
	{
		return;
	}
	if (WasInputKeyJustPressed(EKeys::R))
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));	// on recommence
		return;
	}

	FIntPoint Case(-1, -1);
	const bool bSurLaGrille = CaseSousLaSouris(Case);
	Grille->AfficherSurvol(bSurLaGrille && Phase == EVespPhase::TourAylis ? Case : FIntPoint(-1, -1));

	if (Phase == EVespPhase::TourHaschen)
	{
		JouerTourHaschen(Secondes);
		return;
	}
	if (Phase != EVespPhase::TourAylis || Aylis->EstOccupe())
	{
		return;
	}

	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		FinDuTourDAylis();
		return;
	}
	if (!WasInputKeyJustPressed(EKeys::LeftMouseButton) || !bSurLaGrille)
	{
		return;
	}
	// Un clic sur un Haschen au contact : AYLIS frappe, et son tour est fini
	AVespUnite* Cible = Grille->UniteSur(Case);
	if (Cible && !Cible->EstAylis())
	{
		const FIntPoint Ecart = Case - Aylis->GetCase();
		if (FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y) == 1)
		{
			const int32 Degats = Aylis->Frapper(Cible);
			Annoncer(FString::Printf(TEXT("AYLIS frappe %s : -%d pv%s"), *Cible->Stats.Nom, Degats, Cible->EstDebout() ? TEXT("") : TEXT(" ... il tombe !")),
			         FColor(240, 240, 240), 2.0f);
			FinDuTourDAylis();
		}
		else
		{
			Annoncer(TEXT("Trop loin : approche-toi d'abord (il faut etre sur une case voisine)"), FColor(255, 160, 120), 2.0f);
		}
		return;
	}
	// Sinon : un deplacement (une fois par tour)
	if (!bADejaBouge)
	{
		const TArray<FIntPoint> Chemin = Grille->Chemin(Aylis->GetCase(), Case, DeplacementParTour);
		if (!Chemin.IsEmpty())
		{
			Aylis->Suivre(Chemin);
			bADejaBouge = true;
			MontrerCasesAtteignables();
		}
	}
}

// ===================== Le tour des Haschen =====================
// Un par un, avec une petite pause : il se deplace, puis (une fois arrive) il frappe s'il est au contact

void AVespPlayerController::JouerTourHaschen(float Secondes)
{
	Minuteur += Secondes;
	if (Minuteur < PauseEntreHaschen || Aylis->EstOccupe())
	{
		return;
	}
	while (HaschenQuiJoue < Haschen.Num() && !Haschen[HaschenQuiJoue]->EstDebout())
	{
		HaschenQuiJoue++;
	}
	if (HaschenQuiJoue >= Haschen.Num())
	{
		TourDAylis();		// tout le monde a joue
		return;
	}
	AVespUnite* H = Haschen[HaschenQuiJoue];
	if (H->EstOccupe())
	{
		return;				// il marche encore
	}
	if (EtapeHaschen == 0)
	{
		H->Suivre(Grille->ApprocheVers(H->GetCase(), Aylis->GetCase(), DeplacementHaschen));
		EtapeHaschen = 1;
		return;
	}
	// Arrive : il frappe s'il est au contact
	const FIntPoint Ecart = H->GetCase() - Aylis->GetCase();
	if (FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y) == 1)
	{
		const int32 Degats = H->Frapper(Aylis);
		Annoncer(FString::Printf(TEXT("%s touche AYLIS : -%d pv"), *H->Stats.Nom, Degats), FColor(255, 125, 125), 2.0f);
		if (!Aylis->EstDebout())
		{
			Phase = EVespPhase::Defaite;
			Grille->AfficherCasesAtteignables({});
			Annoncer(TEXT("La vision se brise... (R : recommencer)"), FColor(200, 160, 255), 30.0f);
			return;
		}
	}
	HaschenQuiJoue++;
	EtapeHaschen = 0;
	Minuteur = 0.0f;
}
