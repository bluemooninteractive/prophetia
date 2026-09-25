#include "VespPlayerController.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "VespHUD.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

// Le dossier ou chaque personnage range son modele 3D
static const FString DOSSIER = TEXT("/Game/Characters/");

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

void AVespPlayerController::Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, ACameraActor* LaCamera)
{
	Grille = LaGrille;
	Aylis = LAylis;
	CameraArene = LaCamera;
	PositionCamera = CameraArene ? CameraArene->GetActorLocation() : FVector::ZeroVector;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	PreparerCombat(EVespSalle::Combat);		// la premiere salle est toujours un combat
}

// ===================== Les noms (actions, salles, runes) =====================

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

FString AVespPlayerController::NomSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Combat");
		case EVespSalle::Elite: return TEXT("Elite");
		case EVespSalle::Repos: return TEXT("Feu de camp");
		default: return TEXT("Skarn");
	}
}

FString AVespPlayerController::AideSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Quelques Haschen. Une rune a la fin.");
		case EVespSalle::Elite: return TEXT("Un Haschen d'elite. Dur... mais une rune de plus.");
		case EVespSalle::Repos: return TEXT("AYLIS se repose : +60% pv et une potion.");
		default: return TEXT("Le gardien de la foret.");
	}
}

// Les runes de prophetie (comme route.cpp)
static const TCHAR* NOMS_RUNES[] = {TEXT("Vigueur"), TEXT("Tranchant"), TEXT("Pierre"), TEXT("Vent"), TEXT("Flamme"), TEXT("Seve"), TEXT("Fureur")};
static const TCHAR* AIDES_RUNES[] = {TEXT("+8 pv max"), TEXT("+2 attaque"), TEXT("+2 defense"), TEXT("+1 case de deplacement"),
                                     TEXT("Tes coups ont 1 chance sur 3 de bruler"), TEXT("+5 pv a chaque Haschen abattu"),
                                     TEXT("La rage monte 2 fois plus vite")};
static constexpr int32 NOMBRE_RUNES = 7;

FString AVespPlayerController::NomRune(int32 Rune) { return NOMS_RUNES[Rune]; }
FString AVespPlayerController::AideRune(int32 Rune) { return AIDES_RUNES[Rune]; }

// ===================== La route =====================

AVespUnite* AVespPlayerController::CreerHaschen(const FString& Nom, const FString& Dossier, FIntPoint Case, int32 Pv, int32 Attaque,
                                                int32 Defense, FLinearColor Teinte, EVespStyle Style, bool bPoison, float Taille)
{
	// Plus AYLIS avance, plus les Haschen sont coriaces (comme dans le prototype)
	FVespStats S;
	S.Nom = Nom;
	S.PvMax = Pv + (Salle - 1) * 2;
	S.Pv = S.PvMax;
	S.Attaque = Attaque + (Salle - 1) / 3;
	S.Defense = Defense;
	S.Style = Style;
	S.bAttaquePoison = bPoison;
	AVespUnite* H = GetWorld()->SpawnActor<AVespUnite>(FVector::ZeroVector, FRotator(0, 180, 0));
	H->Taille = Taille;
	H->Preparer(Grille, Case, S, DOSSIER + Dossier, Teinte, false);
	Haschen.Add(H);
	return H;
}

void AVespPlayerController::PreparerCombat(EVespSalle Type)
{
	TypeSalle = Type;
	// On range le combat d'avant : les Haschen tombes disparaissent, et une autre carte de la foret
	for (AVespUnite* H : Haschen)
	{
		H->Destroy();
	}
	Haschen.Reset();
	Grille->ViderOccupants();
	Grille->ChangerCarte(Type == EVespSalle::Boss ? 2 : (Salle - 1) % 2);
	Aylis->Replacer(FIntPoint(1, 3));
	ZonesDanger.Reset();
	Journal.Reset();

	// Les cases de depart des Haschen, a droite, sur du sol libre
	TArray<FIntPoint> Places;
	for (int32 C = 8; C < AVespGrille::Colonnes; C++)
	{
		for (int32 L = 0; L < AVespGrille::Lignes; L++)
		{
			if (!Grille->EstBloquee(FIntPoint(C, L)))
			{
				Places.Add(FIntPoint(C, L));
			}
		}
	}
	auto Place = [&Places]() {
		const int32 i = FMath::RandRange(0, Places.Num() - 1);
		const FIntPoint P = Places[i];
		Places.RemoveAt(i);
		return P;
	};
	// Les Haschen de la foret : un melange au hasard
	auto HaschenAuHasard = [&]() {
		switch (FMath::RandRange(0, 3))
		{
			case 0: CreerHaschen(TEXT("Haschen guerrier"), TEXT("guerrier"), Place(), 22, 9, 2, FLinearColor(0.8f, 0.2f, 0.15f), EVespStyle::Melee, false); break;
			case 1: CreerHaschen(TEXT("Haschen traqueur"), TEXT("traqueur"), Place(), 20, 9, 1, FLinearColor(0.2f, 0.6f, 0.25f), EVespStyle::Lanceur, false); break;
			case 2: CreerHaschen(TEXT("Haschen chaman"), TEXT("chaman"), Place(), 20, 10, 1, FLinearColor(0.5f, 0.25f, 0.7f), EVespStyle::Melee, true); break;
			default: CreerHaschen(TEXT("Haschen eclaireur"), TEXT("sbire"), Place(), 18, 8, 1, FLinearColor(0.9f, 0.5f, 0.1f), EVespStyle::Melee, false); break;
		}
	};
	if (Type == EVespSalle::Boss)
	{
		// Skarn le Brise-Cranes : une brute plus grande que les autres, et deux Haschen avec lui
		AVespUnite* Skarn = CreerHaschen(TEXT("Skarn le Brise-Cranes"), TEXT("guerrier"), FIntPoint(9, 3), 80, 13, 3,
		                                 FLinearColor(0.5f, 0.5f, 0.6f), EVespStyle::Chargeur, false, 260.0f);
		Skarn->Stats.PvMax = 80;
		Skarn->Stats.Pv = 80;
		Skarn->Stats.Boss = 1;
		Places.Remove(FIntPoint(9, 3));
		HaschenAuHasard();
		HaschenAuHasard();
	}
	else if (Type == EVespSalle::Elite)
	{
		AVespUnite* Elite = CreerHaschen(TEXT("Haschen guerrier d'elite"), TEXT("guerrier"), Place(), 37, 11, 3,
		                                 FLinearColor(0.95f, 0.75f, 0.2f), EVespStyle::Melee, false, 210.0f);
		Elite->Stats.ChanceCritique = 15;
		HaschenAuHasard();
		HaschenAuHasard();
	}
	else
	{
		for (int32 i = 0; i < (Salle == 1 ? 2 : 3); i++)
		{
			HaschenAuHasard();
		}
	}
	Tour = 0;
	Ecrire(FString::Printf(TEXT("Acte I, salle %d/%d : %s - La Foret des Brumes"), Salle, NombreDeSalles, *NomSalle(Type)));
	TourDAylis();
}

void AVespPlayerController::ApresVictoire()
{
	Grille->AfficherCasesAtteignables({});
	Grille->AfficherDanger({});
	if (TypeSalle == EVespSalle::Boss)
	{
		Phase = EVespPhase::Victoire;		// Skarn est tombe : l'acte I est termine
		return;
	}
	// AYLIS reprend son souffle, puis choisit une rune
	const int32 Soin = Aylis->Stats.PvMax / 4;
	Aylis->Soigner(Soin);
	MessageRoute = FString::Printf(TEXT("Victoire ! AYLIS reprend son souffle : +%d pv."), Soin);
	ProposerRunes();
}

void AVespPlayerController::ProposerRunes()
{
	TArray<int32> Possibles;
	for (int32 R = 0; R < NOMBRE_RUNES; R++)
	{
		if (R < 4 || !Runes.Contains(R))		// Flamme, Seve et Fureur ne se prennent qu'une fois
		{
			Possibles.Add(R);
		}
	}
	RunesProposees.Reset();
	while (RunesProposees.Num() < 3 && Possibles.Num() > 0)
	{
		const int32 i = FMath::RandRange(0, Possibles.Num() - 1);
		RunesProposees.Add(Possibles[i]);
		Possibles.RemoveAt(i);
	}
	Phase = EVespPhase::ChoixRune;
}

void AVespPlayerController::ChoisirRune(int32 Numero)
{
	if (!RunesProposees.IsValidIndex(Numero))
	{
		return;
	}
	const int32 R = RunesProposees[Numero];
	Runes.Add(R);
	FVespStats& S = Aylis->Stats;
	switch (R)
	{
		case 0: S.PvMax += 8; Aylis->Soigner(8); break;
		case 1: S.Attaque += 2; break;
		case 2: S.Defense += 2; break;
		case 3: DeplacementParTour += 1; break;
		case 4: bFlamme = true; break;
		case 5: bSeve = true; break;
		default: bFureur = true; break;
	}
	MessageRoute = FString::Printf(TEXT("Rune %s : %s."), *NomRune(R), *AideRune(R));
	SalleSuivante();
}

void AVespPlayerController::SalleSuivante()
{
	Salle++;
	if (Salle >= NombreDeSalles)
	{
		DialogueDeSkarn();		// la derniere salle : Skarn
		return;
	}
	ProposerSalles();
}

// La vision montre 2 ou 3 salles possibles
void AVespPlayerController::ProposerSalles()
{
	Propositions = {EVespSalle::Combat};
	if (FMath::RandBool())
	{
		Propositions.Add(EVespSalle::Elite);
	}
	Propositions.Add(EVespSalle::Repos);
	Phase = EVespPhase::ChoixSalle;
}

void AVespPlayerController::ChoisirSalle(int32 Numero)
{
	if (!Propositions.IsValidIndex(Numero))
	{
		return;
	}
	const EVespSalle S = Propositions[Numero];
	if (S == EVespSalle::Repos)
	{
		const int32 Soin = Aylis->Stats.PvMax * 60 / 100;
		Aylis->Soigner(Soin);
		Potions++;
		MessageRoute = FString::Printf(TEXT("AYLIS se repose au coin du feu : +%d pv et une potion."), Soin);
		SalleSuivante();
		return;
	}
	MessageRoute.Reset();
	PreparerCombat(S);
}

void AVespPlayerController::DialogueDeSkarn()
{
	Orateurs = {TEXT("Skarn"), TEXT("AYLIS"), TEXT("Skarn")};
	Repliques = {
		TEXT("Encore une petite vision qui marche vers Karn ? Approche. Le sol se souviendra de toi, meme quand ton nom sera perdu."),
		TEXT("Le sol, peut-etre. Toi, tu vas oublier."),
		TEXT("Quand je leve ma masse, la terre se brise. Compte les pas, petite vision... si tu sais compter."),
	};
	LigneDialogue = 0;
	Ecriture = 0.0f;
	Phase = EVespPhase::Dialogue;
}

// ===================== Les actions d'AYLIS =====================

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
		Aylis->Soigner(15);
		Ecrire(TEXT("AYLIS boit une potion."));
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
	bool bCritique = false;
	const int32 Degats = Aylis->Frapper(Cible, Puissance, &bCritique);
	Trembler(bCritique ? 1.0f : 0.3f);
	Ecrire(FString::Printf(TEXT("AYLIS frappe %s : -%d pv%s"), *Cible->Stats.Nom, Degats, Cible->EstDebout() ? TEXT("") : TEXT(" ... il tombe !")));
	// La rune de Flamme : une chance sur 3 de mettre le feu
	if (bFlamme && Cible->EstDebout() && FMath::RandRange(1, 3) == 1)
	{
		Cible->Brulure = 2;
		Cible->AfficherMessage(TEXT("brule"), FColor(255, 150, 60));
	}
	// La rune de Seve : chaque Haschen abattu rend un peu de vie
	if (bSeve && !Cible->EstDebout())
	{
		Aylis->Soigner(5);
	}
	ActionChoisie = EVespAction::Attaque;
	FinDuTourDAylis();
}

// AYLIS encaisse un coup : la garde divise par 2 (dans Frapper), la rage se remplit
void AVespPlayerController::ToucherAylis(AVespUnite* Attaquant, int32 Degats, bool bCritique)
{
	Rage = FMath::Min(100, Rage + Degats * (bFureur ? 8 : 4));
	Trembler(bCritique ? 1.0f : 0.4f);
	Ecrire(FString::Printf(TEXT("%s touche AYLIS : -%d pv"), *Attaquant->Stats.Nom, Degats));
	// Les coups du chaman empoisonnent (une chance sur deux)
	if (Attaquant->Stats.bAttaquePoison && FMath::RandBool())
	{
		Aylis->Poison = 3;
		Ecrire(TEXT("Poison ! AYLIS perdra des pv a chaque tour."));
	}
	if (!Aylis->EstDebout())
	{
		Phase = EVespPhase::Defaite;
		Grille->AfficherCasesAtteignables({});
		Grille->AfficherDanger({});
	}
}

// ===================== Le tour d'AYLIS =====================

void AVespPlayerController::TourDAylis()
{
	Tour++;
	Phase = EVespPhase::TourAylis;
	bADejaBouge = false;
	bEnGarde = false;
	Aylis->ReductionDegats = 1.0f;
	// Le poison et la brulure font effet au debut du tour
	if (Aylis->SubirEtats() > 0)
	{
		Ecrire(TEXT("Le poison ronge AYLIS."));
		if (!Aylis->EstDebout())
		{
			Phase = EVespPhase::Defaite;
			return;
		}
	}
	MontrerCasesAtteignables();
}

void AVespPlayerController::MontrerCasesAtteignables()
{
	const bool bVisible = Phase == EVespPhase::TourAylis && !bADejaBouge;
	Grille->AfficherCasesAtteignables(bVisible ? Grille->CasesAtteignables(Aylis->GetCase(), DeplacementParTour) : TArray<int32>());
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

void AVespPlayerController::FinDuTourDAylis()
{
	Grille->AfficherCasesAtteignables({});
	Aylis->ReductionDegats = bEnGarde ? 0.5f : 1.0f;
	if (!ResteDesHaschen())
	{
		ApresVictoire();
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

int32 AVespPlayerController::CarteCliquee(int32 Nombre) const
{
	float SourisX = 0, SourisY = 0;
	int32 LargeurEcran = 0, HauteurEcran = 0;
	GetMousePosition(SourisX, SourisY);
	GetViewportSize(LargeurEcran, HauteurEcran);
	for (int32 i = 0; i < Nombre; i++)
	{
		if (AVespHUD::RectangleChoix(i, Nombre, FVector2D(LargeurEcran, HauteurEcran)).IsInside(FVector2D(SourisX, SourisY)))
		{
			return i;
		}
	}
	return -1;
}

// ===================== A chaque image : lire la souris et le clavier =====================

void AVespPlayerController::PlayerTick(float Secondes)
{
	Super::PlayerTick(Secondes);
	if (!Grille || !Aylis)
	{
		return;
	}
	// L'ecran tremble un instant apres un coup (plus fort sur un critique)
	if (CameraArene)
	{
		Secousse = FMath::Max(0.0f, Secousse - Secondes * 3.0f);
		CameraArene->SetActorLocation(PositionCamera + FMath::VRand() * Secousse * 10.0f);
	}
	if (WasInputKeyJustPressed(EKeys::R) && (Phase == EVespPhase::Victoire || Phase == EVespPhase::Defaite))
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));	// une nouvelle vision
		return;
	}
	const bool bClic = WasInputKeyJustPressed(EKeys::LeftMouseButton);
	const FKey Touches[5] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};

	// ----- Le dialogue : ENTREE (ou clic) pour continuer ; le premier appui ecrit toute la replique -----
	if (Phase == EVespPhase::Dialogue)
	{
		Ecriture += Secondes * 45.0f;
		if (bClic || WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::SpaceBar))
		{
			if (Ecriture < Repliques[LigneDialogue].Len())
			{
				Ecriture = 9999.0f;
			}
			else if (++LigneDialogue < Repliques.Num())
			{
				Ecriture = 0.0f;
			}
			else
			{
				PreparerCombat(EVespSalle::Boss);
			}
		}
		return;
	}
	// ----- Les choix de la route : touches 1 a 3, ou clic sur une carte -----
	if (Phase == EVespPhase::ChoixRune || Phase == EVespPhase::ChoixSalle)
	{
		const int32 Nombre = Phase == EVespPhase::ChoixRune ? RunesProposees.Num() : Propositions.Num();
		int32 Choix = bClic ? CarteCliquee(Nombre) : -1;
		for (int32 i = 0; i < Nombre; i++)
		{
			if (WasInputKeyJustPressed(Touches[i]))
			{
				Choix = i;
			}
		}
		if (Choix >= 0)
		{
			if (Phase == EVespPhase::ChoixRune)
			{
				ChoisirRune(Choix);
			}
			else
			{
				ChoisirSalle(Choix);
			}
		}
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

	// ----- Le tour d'AYLIS -----
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
	if (!bClic)
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

// Skarn : tous les 3 tours, il leve sa masse (les cases autour de lui deviennent rouges)... et au tour suivant,
// elles volent en eclats. Renvoie true s'il a utilise son tour pour ca.
bool AVespPlayerController::TourDeSkarn(AVespUnite* Skarn)
{
	Skarn->Compteur++;
	if (ZonesDanger.Num() > 0)
	{
		// Le fracas : la case d AYLIS est-elle encore rouge ?
		const bool bTouche = ZonesDanger.Contains(Aylis->GetCase());
		ZonesDanger.Reset();
		Grille->AfficherDanger({});
		Trembler(1.5f);
		if (bTouche)
		{
			const int32 Degats = FMath::RoundToInt(DegatsDanger * Aylis->ReductionDegats);
			Aylis->Encaisser(Degats, false);
			ToucherAylis(Skarn, Degats, false);
		}
		else
		{
			Ecrire(TEXT("Esquive ! AYLIS n'etait plus dans la zone."));
		}
		return true;		// il reprend son souffle
	}
	if (Skarn->Compteur % 3 == 0)
	{
		for (int32 C = -2; C <= 2; C++)
		{
			for (int32 L = -2; L <= 2; L++)
			{
				const FIntPoint P = Skarn->GetCase() + FIntPoint(C, L);
				if (FMath::Abs(C) + FMath::Abs(L) <= 2 && Grille->EstDansArene(P) && !Grille->EstBloquee(P))
				{
					ZonesDanger.Add(P);
				}
			}
		}
		DegatsDanger = Skarn->Stats.Attaque * 3 / 2;
		Grille->AfficherDanger(ZonesDanger);
		Skarn->AfficherMessage(TEXT("!"), FColor(255, 80, 60));
		Ecrire(TEXT("Skarn leve sa masse... ECARTE-TOI des cases rouges !"));
		return true;
	}
	return false;
}

// Un par un, avec une petite pause : les etats, puis il agit selon son style
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
	auto Suivant = [this]() {
		HaschenQuiJoue++;
		EtapeHaschen = 0;
		Minuteur = 0.0f;
	};
	const FIntPoint Ecart = H->GetCase() - Aylis->GetCase();
	const int32 Distance = FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y);

	if (EtapeHaschen == 0)
	{
		// Le poison et la brulure font effet
		if (H->SubirEtats() > 0 && !H->EstDebout())
		{
			Ecrire(H->Stats.Nom + TEXT(" succombe a ses blessures !"));
			if (!ResteDesHaschen())
			{
				ApresVictoire();
				return;
			}
			Suivant();
			return;
		}
		if (H->Stats.Boss == 1 && TourDeSkarn(H))
		{
			Suivant();
			return;
		}
		// Un lanceur tire de loin une fois sur deux
		if (H->Stats.Style == EVespStyle::Lanceur && Distance >= 2 && Distance <= 4 && FMath::RandBool())
		{
			bool bCritique = false;
			const int32 Degats = H->Frapper(Aylis, 80, &bCritique);
			ToucherAylis(H, Degats, bCritique);
			Suivant();
			return;
		}
		// Sinon il avance : un chargeur va plus loin
		H->Suivre(Grille->ApprocheVers(H->GetCase(), Aylis->GetCase(), H->Stats.Style == EVespStyle::Chargeur ? 4 : 2));
		EtapeHaschen = 1;
		return;
	}
	// Arrive : il frappe s'il est au contact
	if (Distance == 1)
	{
		bool bCritique = false;
		const int32 Degats = H->Frapper(Aylis, 100, &bCritique);
		ToucherAylis(H, Degats, bCritique);
		if (Phase == EVespPhase::Defaite)
		{
			return;
		}
	}
	Suivant();
}
