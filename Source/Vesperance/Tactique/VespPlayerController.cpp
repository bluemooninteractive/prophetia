#include "VespPlayerController.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "VespHUD.h"
#include "Kismet/GameplayStatics.h"

AVespPlayerController::AVespPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AVespPlayerController::Ecrire(const FString& Message)
{
	Journal.Add(Message);
	if (Journal.Num() > 3)
	{
		Journal.RemoveAt(0);
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
	Ecrire(TEXT("Acte 1, salle 1 : Combat - La Foret des Brumes"));
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

// ===================== Les actions (les memes que le prototype) =====================

FString AVespPlayerController::NomAction(EVespAction Action)
{
	switch (Action)
	{
		case EVespAction::Attaque: return TEXT("Attaque");
		case EVespAction::Lourde: return TEXT("Lourde");
		case EVespAction::Garde: return TEXT("Garde");
		case EVespAction::Potion: return TEXT("Potion");
		default: return TEXT("SPECIAL");
	}
}

FString AVespPlayerController::DetailAction(EVespAction Action, int32 NombreDePotions)
{
	switch (Action)
	{
		case EVespAction::Potion: return FString::Printf(TEXT("+15 pv  (x%d)"), NombreDePotions);
		case EVespAction::Speciale: return TEXT("rage pleine");
		default: return TEXT("au contact");
	}
}

FString AVespPlayerController::AideAction(EVespAction Action)
{
	switch (Action)
	{
		case EVespAction::Attaque: return TEXT("Un coup d'epee. Ne rate jamais.");
		case EVespAction::Lourde: return TEXT("Degats x1.8, mais rate 4 fois sur 10.");
		case EVespAction::Garde: return TEXT("Petits degats, et tu encaisses 2 fois moins jusqu'a ton prochain tour.");
		case EVespAction::Potion: return TEXT("Rend 15 pv tout de suite (ton tour est fini).");
		default: return TEXT("Quand la rage est pleine : degats x2.2, ne rate jamais.");
	}
}

bool AVespPlayerController::ActionDisponible(EVespAction Action) const
{
	if (Action == EVespAction::Potion)
	{
		return Potions > 0;
	}
	if (Action == EVespAction::Speciale)
	{
		return Rage >= 100;
	}
	return true;
}

// Une action choisie (touche ou carte). La potion se boit tout de suite ; les autres attendent une cible.
void AVespPlayerController::ChoisirAction(EVespAction Action)
{
	if (!ActionDisponible(Action))
	{
		Ecrire(NomAction(Action) + (Action == EVespAction::Potion ? TEXT(" : plus de potions.") : TEXT(" : la rage n'est pas pleine.")));
		return;
	}
	if (Action == EVespAction::Potion)
	{
		Potions--;
		const int32 Avant = Aylis->Stats.Pv;
		Aylis->Stats.Pv = FMath::Min(Aylis->Stats.PvMax, Aylis->Stats.Pv + 15);
		Ecrire(FString::Printf(TEXT("AYLIS boit une potion : +%d pv"), Aylis->Stats.Pv - Avant));
		FinDuTourDAylis();
		return;
	}
	ActionChoisie = Action;
}

void AVespPlayerController::AgirSur(AVespUnite* Cible)
{
	const EVespAction Action = ActionChoisie;
	int32 Puissance = 100;
	if (Action == EVespAction::Lourde)
	{
		if (FMath::RandRange(1, 100) > 60)
		{
			Aylis->Frapper(Cible, 0);		// l'elan, mais rien ne touche
			Ecrire(TEXT("Attaque lourde... ratee !"));
			FinDuTourDAylis();
			return;
		}
		Puissance = 180;
	}
	else if (Action == EVespAction::Garde)
	{
		Puissance = 60;
		bEnGarde = true;
	}
	else if (Action == EVespAction::Speciale)
	{
		Puissance = 220;
		Rage = 0;
	}
	const int32 Degats = Aylis->Frapper(Cible, Puissance);
	Ecrire(FString::Printf(TEXT("AYLIS frappe %s : -%d pv%s"), *Cible->Stats.Nom, Degats, Cible->EstDebout() ? TEXT("") : TEXT(" ... il tombe !")));
	ActionChoisie = EVespAction::Attaque;
	FinDuTourDAylis();
}

// ===================== Le tour d'AYLIS =====================

void AVespPlayerController::TourDAylis()
{
	Tour++;
	Phase = EVespPhase::TourAylis;
	bADejaBouge = false;
	bEnGarde = false;
	Aylis->ReductionDegats = 1.0f;
	MontrerCasesAtteignables();
}

void AVespPlayerController::MontrerCasesAtteignables()
{
	const bool bVisible = Phase == EVespPhase::TourAylis && !bADejaBouge;
	Grille->AfficherCasesAtteignables(bVisible ? Grille->CasesAtteignables(Aylis->GetCase(), DeplacementParTour) : TArray<int32>());
}

void AVespPlayerController::FinDuTourDAylis()
{
	Grille->AfficherCasesAtteignables({});
	Aylis->ReductionDegats = bEnGarde ? 0.5f : 1.0f;
	if (!ResteDesHaschen())
	{
		Phase = EVespPhase::Victoire;
		Ecrire(TEXT("Les Haschen sont tombes !"));
		return;
	}
	Phase = EVespPhase::TourHaschen;
	HaschenQuiJoue = 0;
	EtapeHaschen = 0;
	Minuteur = 0.0f;
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

	// Les touches 1 a 5 choisissent une action
	const FKey Touches[AVespHUD::NombreDeCartes] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
	for (int32 i = 0; i < AVespHUD::NombreDeCartes; i++)
	{
		if (WasInputKeyJustPressed(Touches[i]))
		{
			ChoisirAction((EVespAction)i);
			return;
		}
	}
	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		Ecrire(TEXT("AYLIS attend."));
		FinDuTourDAylis();
		return;
	}
	if (!WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		return;
	}
	// Un clic sur une carte d'action
	float SourisX = 0, SourisY = 0;
	int32 LargeurEcran = 0, HauteurEcran = 0;
	GetMousePosition(SourisX, SourisY);
	GetViewportSize(LargeurEcran, HauteurEcran);
	for (int32 i = 0; i < AVespHUD::NombreDeCartes; i++)
	{
		if (AVespHUD::RectangleCarte(i, FVector2D(LargeurEcran, HauteurEcran)).IsInside(FVector2D(SourisX, SourisY)))
		{
			ChoisirAction((EVespAction)i);
			return;
		}
	}
	if (!bSurLaGrille)
	{
		return;
	}
	// Un clic sur un Haschen au contact : l'action choisie
	AVespUnite* Cible = Grille->UniteSur(Case);
	if (Cible && !Cible->EstAylis())
	{
		const FIntPoint Ecart = Case - Aylis->GetCase();
		if (FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y) == 1)
		{
			AgirSur(Cible);
		}
		else
		{
			Ecrire(TEXT("Trop loin ! Il faut etre sur une case voisine."));
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
		const int32 Degats = H->Frapper(Aylis, 100);
		Rage = FMath::Min(100, Rage + Degats * 4);		// la rage monte avec les coups recus
		Ecrire(FString::Printf(TEXT("%s touche AYLIS : -%d pv"), *H->Stats.Nom, Degats));
		if (!Aylis->EstDebout())
		{
			Phase = EVespPhase::Defaite;
			Grille->AfficherCasesAtteignables({});
			return;
		}
	}
	HaschenQuiJoue++;
	EtapeHaschen = 0;
	Minuteur = 0.0f;
}
