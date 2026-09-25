#include "VespPlayerController.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "VespInterface.h"
#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWeakWidget.h"

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
	// L'interface (Slate) : posee par-dessus l'ecran de jeu
	if (GEngine && GEngine->GameViewport)
	{
		Interface = SNew(SVespInterface).Joueur(this);
		GEngine->GameViewport->AddViewportWidgetContent(Interface.ToSharedRef());
	}
	PreparerCombat(EVespSalle::Combat);		// la premiere salle est toujours un combat
}

void AVespPlayerController::EndPlay(const EEndPlayReason::Type Raison)
{
	if (Interface.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Interface.ToSharedRef());
	}
	Interface.Reset();
	Super::EndPlay(Raison);
}

// ===================== Les noms (actions, salles, runes, lieux) =====================

FString AVespPlayerController::NomAction(EVespAction Action)
{
	switch (Action)
	{
		case EVespAction::Attaque: return TEXT("Attaque");
		case EVespAction::Lourde: return TEXT("Lourde");
		case EVespAction::Garde: return TEXT("Garde");
		case EVespAction::Potion: return TEXT("Potion");
		default: return TEXT("Special");
	}
}

FString AVespPlayerController::DetailAction(EVespAction Action, int32 NombreDePotions)
{
	switch (Action)
	{
		case EVespAction::Attaque: return TEXT("x1");
		case EVespAction::Lourde: return TEXT("x1.8  -  60%");
		case EVespAction::Garde: return TEXT("x0.6  -  degats /2");
		case EVespAction::Potion: return FString::Printf(TEXT("+15 pv  -  reste %d"), NombreDePotions);
		default: return TEXT("x2.2  -  rage pleine");
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

FString AVespPlayerController::NomSalle(EVespSalle S, int32 LActe)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Combat");
		case EVespSalle::Elite: return TEXT("Elite");
		case EVespSalle::Repos: return TEXT("Feu de camp");
		default: return LActe == 1 ? TEXT("Skarn") : TEXT("La Matriarche");
	}
}

FString AVespPlayerController::AideSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Quelques Haschen. Une rune a la fin.");
		case EVespSalle::Elite: return TEXT("Un Haschen d'elite. Dur... mais une rune de plus.");
		case EVespSalle::Repos: return TEXT("AYLIS se repose : +60% pv et une potion.");
		default: return TEXT("Le gardien de cette terre.");
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

FString AVespPlayerController::NomDuLieu() const
{
	if (TypeSalle == EVespSalle::Boss)
	{
		return Acte == 1 ? TEXT("Le cercle des anciens") : TEXT("La clairiere de la Matriarche");
	}
	return Acte == 1 ? TEXT("La Foret des Brumes") : TEXT("Le Bois des Pendus");
}

FString AVespPlayerController::NomDeLActe() const
{
	return Acte == 1 ? TEXT("Les Terres Brumeuses") : TEXT("Les Terres Hantees");
}

// ===================== La route =====================

AVespUnite* AVespPlayerController::CreerHaschen(const FString& Nom, const FString& Dossier, FIntPoint Case, int32 Pv, int32 Attaque,
                                                int32 Defense, FLinearColor Teinte, EVespStyle Style, bool bPoison, float Taille)
{
	// Plus AYLIS avance sur la route, plus les Haschen sont coriaces (comme dans le prototype)
	const int32 Avancee = (Acte - 1) * NombreDeSalles + Salle - 1;
	FVespStats S;
	S.Nom = Nom;
	S.PvMax = Pv + Avancee * 2;
	S.Pv = S.PvMax;
	S.Attaque = Attaque + Avancee / 3;
	S.Defense = Defense;
	S.Style = Style;
	S.bAttaquePoison = bPoison;
	AVespUnite* H = GetWorld()->SpawnActor<AVespUnite>(FVector::ZeroVector, FRotator(0, 180, 0));
	H->Taille = Taille;
	H->Preparer(Grille, Case, S, DOSSIER + Dossier, Teinte, false);
	Haschen.Add(H);
	return H;
}

// Un Haschen au hasard, parmi ceux du lieu
void AVespPlayerController::HaschenAuHasard(TArray<FIntPoint>& Places)
{
	if (Places.Num() == 0)
	{
		return;
	}
	const int32 i = FMath::RandRange(0, Places.Num() - 1);
	const FIntPoint P = Places[i];
	Places.RemoveAt(i);
	if (Acte == 1)
	{
		// La foret : les eclaireurs d'Ashka
		switch (FMath::RandRange(0, 3))
		{
			case 0: CreerHaschen(TEXT("Haschen guerrier"), TEXT("guerrier"), P, 22, 9, 2, FLinearColor(0.8f, 0.2f, 0.15f), EVespStyle::Melee, false); break;
			case 1: CreerHaschen(TEXT("Haschen traqueur"), TEXT("traqueur"), P, 20, 9, 1, FLinearColor(0.2f, 0.6f, 0.25f), EVespStyle::Lanceur, false); break;
			case 2: CreerHaschen(TEXT("Haschen chaman"), TEXT("chaman"), P, 20, 10, 1, FLinearColor(0.5f, 0.25f, 0.7f), EVespStyle::Melee, true); break;
			default: CreerHaschen(TEXT("Haschen eclaireur"), TEXT("sbire"), P, 18, 8, 1, FLinearColor(0.9f, 0.5f, 0.1f), EVespStyle::Melee, false); break;
		}
		return;
	}
	// Le Bois des Pendus : chamans, louvetiers qui chargent, et les premieres brutes
	switch (FMath::RandRange(0, 3))
	{
		case 0: CreerHaschen(TEXT("Haschen louvetier"), TEXT("sbire"), P, 22, 10, 2, FLinearColor(0.45f, 0.3f, 0.2f), EVespStyle::Chargeur, false); break;
		case 1: CreerHaschen(TEXT("Haschen chaman"), TEXT("chaman"), P, 20, 10, 1, FLinearColor(0.5f, 0.25f, 0.7f), EVespStyle::Melee, true); break;
		case 2: CreerHaschen(TEXT("Haschen traqueur"), TEXT("traqueur"), P, 20, 9, 1, FLinearColor(0.2f, 0.6f, 0.25f), EVespStyle::Lanceur, false); break;
		default: CreerHaschen(TEXT("Haschen brute"), TEXT("guerrier"), P, 34, 12, 4, FLinearColor(0.5f, 0.15f, 0.15f), EVespStyle::Chargeur, false, 205.0f); break;
	}
}

// Une case libre pres d'une autre (pour les loups appeles par la Matriarche)
bool AVespPlayerController::CaseLibrePres(FIntPoint Centre, FIntPoint& Trouvee) const
{
	for (int32 Rayon = 1; Rayon <= 3; Rayon++)
	{
		for (int32 C = -Rayon; C <= Rayon; C++)
		{
			for (int32 L = -Rayon; L <= Rayon; L++)
			{
				const FIntPoint P = Centre + FIntPoint(C, L);
				if (!Grille->EstBloquee(P) && !Grille->UniteSur(P))
				{
					Trouvee = P;
					return true;
				}
			}
		}
	}
	return false;
}

void AVespPlayerController::PreparerCombat(EVespSalle Type)
{
	TypeSalle = Type;
	// On range le combat d'avant : les Haschen tombes disparaissent, et une autre carte
	for (AVespUnite* H : Haschen)
	{
		H->Destroy();
	}
	Haschen.Reset();
	Grille->ViderOccupants();
	const int32 Base = Acte == 1 ? 0 : 3;
	Grille->ChangerCarte(Base + (Type == EVespSalle::Boss ? 2 : (Salle - 1) % 2));
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
	if (Type == EVespSalle::Boss)
	{
		FIntPoint CaseDuBoss(9, 3);
		Places.Remove(CaseDuBoss);
		AVespUnite* Boss = nullptr;
		if (Acte == 1)
		{
			// Skarn le Brise-Cranes : une brute plus grande que les autres
			Boss = CreerHaschen(TEXT("Skarn le Brise-Cranes"), TEXT("guerrier"), CaseDuBoss, 80, 13, 3,
			                    FLinearColor(0.5f, 0.5f, 0.6f), EVespStyle::Chargeur, false, 260.0f);
			Boss->Stats.PvMax = 80;
		}
		else
		{
			// La Matriarche : une chamane, son malefice empoisonne de loin
			Boss = CreerHaschen(TEXT("La Matriarche"), TEXT("chaman"), CaseDuBoss, 95, 14, 3,
			                    FLinearColor(0.3f, 0.65f, 0.35f), EVespStyle::Lanceur, true, 250.0f);
			Boss->Stats.PvMax = 95;
			PotionsDuBoss = 2;
		}
		Boss->Stats.Pv = Boss->Stats.PvMax;
		Boss->Stats.Boss = Acte;
		HaschenAuHasard(Places);
		HaschenAuHasard(Places);
	}
	else if (Type == EVespSalle::Elite)
	{
		const int32 i = FMath::RandRange(0, Places.Num() - 1);
		const FIntPoint P = Places[i];
		Places.RemoveAt(i);
		AVespUnite* Elite = Acte == 1
			? CreerHaschen(TEXT("Haschen guerrier d'elite"), TEXT("guerrier"), P, 37, 11, 3, FLinearColor(0.95f, 0.75f, 0.2f), EVespStyle::Melee, false, 210.0f)
			: CreerHaschen(TEXT("Haschen louvetier d'elite"), TEXT("sbire"), P, 38, 12, 3, FLinearColor(0.95f, 0.75f, 0.2f), EVespStyle::Chargeur, false, 210.0f);
		Elite->Stats.ChanceCritique = 15;
		HaschenAuHasard(Places);
		HaschenAuHasard(Places);
	}
	else
	{
		const int32 Nombre = (Acte == 1 && Salle == 1) ? 2 : (Acte == 2 ? FMath::RandRange(3, 4) : 3);
		for (int32 i = 0; i < Nombre; i++)
		{
			HaschenAuHasard(Places);
		}
	}
	Tour = 0;
	Ecrire(FString::Printf(TEXT("Acte %s, salle %d/%d : %s"), Acte == 1 ? TEXT("I") : TEXT("II"), Salle, NombreDeSalles, *NomDuLieu()));
	TourDAylis();
}

void AVespPlayerController::ApresVictoire()
{
	Grille->AfficherCasesAtteignables({});
	Grille->AfficherDanger({});
	ZonesDanger.Reset();
	if (TypeSalle == EVespSalle::Boss)
	{
		if (Acte >= NombreDActes)
		{
			Phase = EVespPhase::Victoire;		// le dernier boss est tombe
			TempsPhase = 0.0f;
			return;
		}
		// Un nouvel acte : AYLIS reprend toutes ses forces, et une potion
		Acte++;
		Salle = 1;
		Aylis->Soigner(Aylis->Stats.PvMax);
		Aylis->Poison = 0;
		Aylis->Brulure = 0;
		Potions++;
		AmbianceDeLActe();
		Phase = EVespPhase::NouvelActe;
		TempsPhase = 0.0f;
		return;
	}
	// AYLIS reprend son souffle, puis choisit une rune
	const int32 Soin = Aylis->Stats.PvMax / 4;
	Aylis->Soigner(Soin);
	MessageRoute = FString::Printf(TEXT("Victoire ! AYLIS reprend son souffle : +%d pv."), Soin);
	ProposerRunes();
}

// L'acte II : une brume verte et malade, une lune plus pale
void AVespPlayerController::AmbianceDeLActe()
{
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		It->GetComponent()->SetFogInscatteringColor(FLinearColor(0.07f, 0.13f, 0.05f));
		It->GetComponent()->SetFogDensity(0.012f);
	}
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		It->GetLightComponent()->SetLightColor(FLinearColor(0.62f, 0.78f, 0.55f));
		It->GetLightComponent()->SetIntensity(2.4f);
	}
}

void AVespPlayerController::ContinuerApresLActe()
{
	if (Phase == EVespPhase::NouvelActe && TempsPhase > 1.0f)
	{
		MessageRoute.Reset();
		PreparerCombat(EVespSalle::Combat);		// la premiere salle d'un acte est toujours un combat
	}
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
	TempsPhase = 0.0f;
}

void AVespPlayerController::ChoisirRune(int32 Numero)
{
	if (Phase != EVespPhase::ChoixRune || !RunesProposees.IsValidIndex(Numero))
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
		DialogueDuBoss();		// la derniere salle de l'acte : le boss
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
	TempsPhase = 0.0f;
}

void AVespPlayerController::ChoisirSalle(int32 Numero)
{
	if (Phase != EVespPhase::ChoixSalle || !Propositions.IsValidIndex(Numero))
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

// Le boss parle avant le combat (toujours sans genre pour AYLIS)
void AVespPlayerController::DialogueDuBoss()
{
	if (Acte == 1)
	{
		Orateurs = {TEXT("Skarn"), TEXT("AYLIS"), TEXT("Skarn")};
		Repliques = {
			TEXT("Encore une petite vision qui marche vers Karn ? Approche. Le sol se souviendra de toi, meme quand ton nom sera perdu."),
			TEXT("Le sol, peut-etre. Toi, tu vas oublier."),
			TEXT("Quand je leve ma masse, la terre se brise. Compte les pas, petite vision... si tu sais compter."),
		};
	}
	else
	{
		Orateurs = {TEXT("La Matriarche"), TEXT("AYLIS"), TEXT("La Matriarche")};
		Repliques = {
			TEXT("Mes loups ont senti ta peur bien avant ton odeur. Ils ont faim, et moi, j'ai le temps."),
			TEXT("Tes loups auront faim longtemps. Ce n'est pas pour eux que je marche."),
			TEXT("Tous viennent pour moi, a la fin. Approche, que je te couvre de mon malefice."),
		};
	}
	LigneDialogue = 0;
	Ecriture = 0.0f;
	Phase = EVespPhase::Dialogue;
	TempsPhase = 0.0f;
}

void AVespPlayerController::AvancerDialogue()
{
	if (Phase != EVespPhase::Dialogue)
	{
		return;
	}
	if (Ecriture < Repliques[LigneDialogue].Len())
	{
		Ecriture = 9999.0f;			// un premier appui ecrit toute la replique
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

void AVespPlayerController::Recommencer()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));	// une nouvelle vision
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
	if (Phase != EVespPhase::TourAylis || Aylis->EstOccupe())
	{
		return;
	}
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

void AVespPlayerController::PasserLeTour()
{
	if (Phase == EVespPhase::TourAylis && !Aylis->EstOccupe())
	{
		Ecrire(TEXT("AYLIS attend."));
		FinDuTourDAylis();
	}
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

// AYLIS encaisse un coup : la rage se remplit, le chaman peut empoisonner
void AVespPlayerController::ToucherAylis(AVespUnite* Attaquant, int32 Degats, bool bCritique)
{
	Rage = FMath::Min(100, Rage + Degats * (bFureur ? 8 : 4));
	Trembler(bCritique ? 1.0f : 0.4f);
	Ecrire(FString::Printf(TEXT("%s touche AYLIS : -%d pv"), *Attaquant->Stats.Nom, Degats));
	if (Attaquant->Stats.bAttaquePoison && FMath::RandBool())
	{
		Aylis->Poison = 3;
		Ecrire(TEXT("Poison ! AYLIS perdra des pv a chaque tour."));
	}
	if (!Aylis->EstDebout())
	{
		Phase = EVespPhase::Defaite;
		TempsPhase = 0.0f;
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
			TempsPhase = 0.0f;
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

// ===================== A chaque image : le clavier, et les clics sur l'arene =====================
// (les clics sur les boutons sont geres par l'interface elle-meme)

void AVespPlayerController::PlayerTick(float Secondes)
{
	Super::PlayerTick(Secondes);
	if (!Grille || !Aylis)
	{
		return;
	}
	TempsPhase += Secondes;
	// L'ecran tremble un instant apres un coup (plus fort sur un critique)
	if (CameraArene)
	{
		Secousse = FMath::Max(0.0f, Secousse - Secondes * 3.0f);
		CameraArene->SetActorLocation(PositionCamera + FMath::VRand() * Secousse * 10.0f);
	}
	const FKey Touches[5] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
	const bool bValider = WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::SpaceBar);

	switch (Phase)
	{
		case EVespPhase::Victoire:
		case EVespPhase::Defaite:
			if (WasInputKeyJustPressed(EKeys::R))
			{
				Recommencer();
			}
			return;
		case EVespPhase::Dialogue:
			Ecriture += Secondes * 45.0f;
			if (bValider || WasInputKeyJustPressed(EKeys::LeftMouseButton))
			{
				AvancerDialogue();
			}
			return;
		case EVespPhase::NouvelActe:
			if (bValider)
			{
				ContinuerApresLActe();
			}
			return;
		case EVespPhase::ChoixRune:
		case EVespPhase::ChoixSalle:
			for (int32 i = 0; i < 3; i++)
			{
				if (WasInputKeyJustPressed(Touches[i]))
				{
					Phase == EVespPhase::ChoixRune ? ChoisirRune(i) : ChoisirSalle(i);
					return;
				}
			}
			return;
		default:
			break;
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
	for (int32 i = 0; i < 5; i++)
	{
		if (WasInputKeyJustPressed(Touches[i]))
		{
			ChoisirAction((EVespAction)i);
			return;
		}
	}
	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		PasserLeTour();
		return;
	}
	if (!WasInputKeyJustPressed(EKeys::LeftMouseButton) || !bSurLaGrille)
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

// ===================== Les boss =====================

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

// La Matriarche (comme boss.cpp) : elle se soigne quand elle faiblit, elle appelle ses loups tous les 3 tours,
// et sinon son malefice empoisonne de loin.
bool AVespPlayerController::TourDeLaMatriarche(AVespUnite* M)
{
	M->Compteur++;
	if (M->Stats.Pv < M->Stats.PvMax / 2 && PotionsDuBoss > 0 && M->Compteur % 2 == 0)
	{
		PotionsDuBoss--;
		M->Soigner(20);
		Ecrire(TEXT("La Matriarche boit une decoction : +20 pv."));
		return true;
	}
	int32 Debout = 0;
	for (const AVespUnite* H : Haschen)
	{
		Debout += H->EstDebout() ? 1 : 0;
	}
	if (M->Compteur % 3 == 0 && Debout < 5)
	{
		for (int32 i = 0; i < 2; i++)
		{
			FIntPoint Case;
			if (CaseLibrePres(M->GetCase(), Case))
			{
				CreerHaschen(TEXT("Haschen louvetier"), TEXT("sbire"), Case, 22, 10, 2, FLinearColor(0.45f, 0.3f, 0.2f), EVespStyle::Chargeur, false);
			}
		}
		M->AfficherMessage(TEXT("AOUUUU"), FColor(150, 255, 150));
		Ecrire(TEXT("La Matriarche hurle... les loups repondent !"));
		return true;
	}
	const FIntPoint Ecart = M->GetCase() - Aylis->GetCase();
	const int32 Distance = FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y);
	if (Distance >= 2 && Distance <= 5)
	{
		bool bCritique = false;
		const int32 Degats = M->Frapper(Aylis, 90, &bCritique);
		ToucherAylis(M, Degats, bCritique);
		if (Phase != EVespPhase::Defaite)
		{
			Aylis->Poison = 3;
			Ecrire(TEXT("Malefice ! Le poison ronge AYLIS."));
		}
		return true;
	}
	return false;
}

// ===================== Le tour des Haschen =====================
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
		if ((H->Stats.Boss == 1 && TourDeSkarn(H)) || (H->Stats.Boss == 2 && TourDeLaMatriarche(H)))
		{
			Suivant();
			return;
		}
		if (Phase == EVespPhase::Defaite)
		{
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
