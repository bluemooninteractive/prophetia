#include "VespPlayerController.h"
#include "VespGrille.h"
#include "VespMonde.h"
#include "VespUnite.h"
#include "VespEffet.h"
#include "VespInterface.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/SWeakWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/PostProcessVolume.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "AssetCompilingManager.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

// Le dossier ou chaque personnage range son modele 3D
static const FString DOSSIER = TEXT("/Game/Characters/");

// ===================== Les 7 actes =====================

struct FVespInfoActe
{
	const TCHAR* Titre;
	const TCHAR* Lieu;
	const TCHAR* LieuDuBoss;
	const TCHAR* Regle;			// ce qui rend cet acte different
	FLinearColor Brume;
	float Densite;
	FLinearColor Lune;
	float IntensiteLune;
	float Ciel;
};

static const FVespInfoActe ACTES[7] = {
	{TEXT("Les Terres Brumeuses"), TEXT("La Foret des Brumes"), TEXT("Le cercle des anciens"),
	 TEXT("Une foret calme... pour l'instant. Apprends a lire les Haschen : les tireurs frappent de loin, les chargeurs foncent."),
	 FLinearColor(0.05f, 0.12f, 0.16f), 0.006f, FLinearColor(0.55f, 0.65f, 1.0f), 3.0f, 0.35f},
	{TEXT("Les Terres Hantees"), TEXT("Le Bois des Pendus"), TEXT("La clairiere de la Matriarche"),
	 TEXT("Le poison ronge a chaque tour. La boue arrete la marche. Les loups chassent en meute."),
	 FLinearColor(0.07f, 0.13f, 0.05f), 0.012f, FLinearColor(0.62f, 0.78f, 0.55f), 2.4f, 0.3f},
	{TEXT("Les Marais Noyes"), TEXT("Les Marais de Sombreval"), TEXT("Le trone englouti"),
	 TEXT("La boue arrete la marche. Les eaux toxiques rongent qui finit son tour dedans... et tous les 4 tours, elles montent."),
	 FLinearColor(0.04f, 0.1f, 0.08f), 0.016f, FLinearColor(0.5f, 0.75f, 0.7f), 2.2f, 0.3f},
	{TEXT("La Marche d'Ashka"), TEXT("La forteresse d'Ashka"), TEXT("Le grand portail"),
	 TEXT("Les dalles piegees se levent un tour sur deux. Les armures ne cedent qu'aux coups lourds : une armure brisee laisse son porteur sonne."),
	 FLinearColor(0.1f, 0.07f, 0.05f), 0.008f, FLinearColor(0.9f, 0.7f, 0.5f), 2.6f, 0.35f},
	{TEXT("Le Col d'Ashka"), TEXT("Le col gele"), TEXT("Le sommet du col"),
	 TEXT("La glace fait glisser plus loin que prevu. Tous les 3 tours, le blizzard ralentit la marche. Le givre fige sur place."),
	 FLinearColor(0.12f, 0.14f, 0.2f), 0.01f, FLinearColor(0.75f, 0.85f, 1.0f), 2.2f, 0.4f},
	{TEXT("Les Terres de Cendre"), TEXT("La faille ardente"), TEXT("La forge de Vorgath"),
	 TEXT("La lave brule qui finit son tour dessus. A chaque tour, le sol se fissure : les cases marquees entrent en eruption au tour suivant."),
	 FLinearColor(0.18f, 0.06f, 0.03f), 0.01f, FLinearColor(1.0f, 0.55f, 0.35f), 2.4f, 0.3f},
	{TEXT("Karn"), TEXT("La cite voilee"), TEXT("Le coeur du Voile"),
	 TEXT("Les failles du Voile emportent qui s'y arrete. Tous les 4 tours, le Voile se dechire et un echo surgit. Tout ce que tu as affronte revient."),
	 FLinearColor(0.08f, 0.04f, 0.14f), 0.012f, FLinearColor(0.7f, 0.5f, 1.0f), 2.6f, 0.35f},
};

static const FVespInfoActe& InfoActe(int32 Acte)
{
	return ACTES[FMath::Clamp(Acte, 1, 7) - 1];
}

// ===================== Les Haschen de chaque acte =====================
// nom, dossier du modele 3D, pv, attaque, defense, couleur, style, effet de ses coups, talents,
// armure, cases par tour (0 = selon le style), portee de tir, chances de critique, taille (cm)

struct FVespModeleHaschen
{
	const TCHAR* Nom;
	const TCHAR* Dossier;
	int32 Pv, Attaque, Defense;
	FLinearColor Teinte;
	EVespStyle Style;
	int32 Effet;
	int32 Capacites;
	int32 Armure;
	int32 Pas;
	int32 Portee;
	int32 Critique;
	float Taille;
};

namespace
{
	constexpr EVespStyle MELEE = EVespStyle::Melee, LANCEUR = EVespStyle::Lanceur, CHARGEUR = EVespStyle::Chargeur;
	constexpr int32 POISON = VespEffetCoup::Poison, GEL = VespEffetCoup::Gel, BRULURE = VespEffetCoup::Brulure;
	constexpr int32 SOIGNEUR = VespCapacite::Soigneur, EXPLOSIF = VespCapacite::Explosif, INVOCATEUR = VespCapacite::Invocateur;
	constexpr int32 SAUTEUR = VespCapacite::Sauteur, VAMPIRE = VespCapacite::Vampire, ATTIRE = VespCapacite::Attire;
	const FLinearColor OR(0.95f, 0.75f, 0.2f);
}

static const FVespModeleHaschen HASCHEN[7][5] = {
	{	// I. La Foret des Brumes : les eclaireurs d'Ashka
		{TEXT("Haschen guerrier"), TEXT("guerrier"), 22, 9, 2, FLinearColor(0.8f, 0.2f, 0.15f), MELEE, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen traqueur"), TEXT("traqueur"), 20, 9, 1, FLinearColor(0.2f, 0.6f, 0.25f), LANCEUR, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen chaman"), TEXT("chaman"), 20, 10, 1, FLinearColor(0.5f, 0.25f, 0.7f), MELEE, POISON, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen porte-bouclier"), TEXT("guerrier"), 28, 8, 5, FLinearColor(0.45f, 0.45f, 0.55f), MELEE, 0, 0, 0, 0, 4, 5, 185.0f},
		{TEXT("Haschen eclaireur"), TEXT("sbire"), 18, 8, 1, FLinearColor(0.9f, 0.5f, 0.1f), CHARGEUR, 0, 0, 0, 0, 4, 12, 165.0f},
	},
	{	// II. Le Bois des Pendus
		{TEXT("Haschen louvetier"), TEXT("sbire"), 24, 11, 2, FLinearColor(0.45f, 0.3f, 0.2f), CHARGEUR, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen chaman"), TEXT("chaman"), 24, 11, 1, FLinearColor(0.5f, 0.25f, 0.7f), MELEE, POISON, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen traqueur"), TEXT("traqueur"), 24, 10, 1, FLinearColor(0.2f, 0.6f, 0.25f), LANCEUR, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen des cordes"), TEXT("traqueur"), 22, 12, 1, FLinearColor(0.35f, 0.55f, 0.35f), LANCEUR, POISON, ATTIRE, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen brute"), TEXT("guerrier"), 36, 12, 4, FLinearColor(0.5f, 0.15f, 0.15f), CHARGEUR, 0, 0, 0, 0, 4, 10, 205.0f},
	},
	{	// III. Les Marais de Sombreval
		{TEXT("Noye"), TEXT("guerrier"), 36, 13, 3, FLinearColor(0.2f, 0.45f, 0.4f), MELEE, 0, 0, 0, 1, 4, 10, 180.0f},
		{TEXT("Crapaud cracheur"), TEXT("sbire"), 26, 12, 1, FLinearColor(0.4f, 0.55f, 0.2f), LANCEUR, POISON, 0, 0, 0, 4, 10, 130.0f},
		{TEXT("Sangsue des vases"), TEXT("sbire"), 28, 12, 2, FLinearColor(0.35f, 0.15f, 0.2f), MELEE, 0, VAMPIRE, 0, 3, 4, 10, 160.0f},
		{TEXT("Pecheur d'ames"), TEXT("traqueur"), 28, 13, 2, FLinearColor(0.25f, 0.35f, 0.5f), LANCEUR, 0, ATTIRE, 0, 0, 5, 10, 175.0f},
		{TEXT("Sorciere des vases"), TEXT("chaman"), 28, 12, 1, FLinearColor(0.3f, 0.5f, 0.3f), LANCEUR, POISON, SOIGNEUR, 0, 0, 4, 10, 175.0f},
	},
	{	// IV. La forteresse d'Ashka
		{TEXT("Sentinelle"), TEXT("guerrier"), 38, 14, 5, FLinearColor(0.5f, 0.45f, 0.35f), MELEE, 0, 0, 1, 0, 4, 10, 190.0f},
		{TEXT("Arbaletrier"), TEXT("traqueur"), 32, 16, 2, FLinearColor(0.55f, 0.35f, 0.2f), LANCEUR, 0, 0, 0, 0, 5, 15, 175.0f},
		{TEXT("Rodeur des ruines"), TEXT("sbire"), 30, 15, 2, FLinearColor(0.6f, 0.5f, 0.3f), MELEE, 0, SAUTEUR, 0, 0, 4, 15, 170.0f},
		{TEXT("Gardien d'autel"), TEXT("chaman"), 32, 13, 2, FLinearColor(0.8f, 0.7f, 0.4f), MELEE, 0, SOIGNEUR, 0, 0, 4, 10, 175.0f},
		{TEXT("Golem mineur"), TEXT("guerrier"), 44, 15, 4, FLinearColor(0.45f, 0.45f, 0.45f), MELEE, 0, 0, 2, 1, 4, 5, 200.0f},
	},
	{	// V. Le col gele
		{TEXT("Loup des neiges"), TEXT("sbire"), 36, 16, 2, FLinearColor(0.85f, 0.88f, 0.95f), CHARGEUR, 0, 0, 0, 5, 4, 12, 165.0f},
		{TEXT("Chaman du givre"), TEXT("chaman"), 36, 15, 2, FLinearColor(0.4f, 0.6f, 0.9f), LANCEUR, GEL, 0, 0, 0, 4, 10, 175.0f},
		{TEXT("Yeti Haschen"), TEXT("guerrier"), 55, 18, 4, FLinearColor(0.9f, 0.9f, 0.95f), MELEE, GEL, 0, 0, 1, 4, 10, 225.0f},
		{TEXT("Archer d'Ashka"), TEXT("traqueur"), 36, 17, 2, FLinearColor(0.6f, 0.2f, 0.3f), LANCEUR, 0, 0, 0, 0, 5, 15, 175.0f},
		{TEXT("Givrin"), TEXT("sbire"), 28, 14, 1, FLinearColor(0.5f, 0.8f, 1.0f), CHARGEUR, GEL, EXPLOSIF, 0, 0, 4, 10, 130.0f},
	},
	{	// VI. Les Terres de Cendre
		{TEXT("Incendiaire"), TEXT("sbire"), 36, 17, 2, FLinearColor(1.0f, 0.4f, 0.1f), CHARGEUR, BRULURE, EXPLOSIF, 0, 0, 4, 10, 160.0f},
		{TEXT("Forgeron Haschen"), TEXT("guerrier"), 50, 18, 5, FLinearColor(0.4f, 0.25f, 0.2f), MELEE, BRULURE, 0, 1, 0, 4, 10, 195.0f},
		{TEXT("Salamandre"), TEXT("sbire"), 40, 18, 3, FLinearColor(0.9f, 0.3f, 0.1f), CHARGEUR, BRULURE, 0, 0, 5, 4, 12, 150.0f},
		{TEXT("Pyromancien"), TEXT("chaman"), 42, 18, 2, FLinearColor(0.9f, 0.5f, 0.2f), LANCEUR, BRULURE, 0, 0, 0, 4, 10, 175.0f},
		{TEXT("Colosse de braise"), TEXT("guerrier"), 65, 20, 5, FLinearColor(0.35f, 0.15f, 0.1f), MELEE, BRULURE, 0, 2, 1, 4, 5, 235.0f},
	},
	{	// VII. Karn, la cite voilee
		{TEXT("Echo de la vision"), TEXT("Aylis"), 46, 20, 4, FLinearColor(0.3f, 0.25f, 0.45f), MELEE, 0, SAUTEUR, 0, 0, 4, 15, 175.0f},
		{TEXT("Garde de Karn"), TEXT("guerrier"), 60, 20, 6, FLinearColor(0.55f, 0.4f, 0.7f), MELEE, 0, 0, 1, 0, 4, 10, 205.0f},
		{TEXT("Lame du Voile"), TEXT("sbire"), 44, 22, 3, FLinearColor(0.6f, 0.3f, 0.9f), MELEE, 0, SAUTEUR, 0, 3, 4, 25, 165.0f},
		{TEXT("Archer du Voile"), TEXT("traqueur"), 44, 20, 3, FLinearColor(0.5f, 0.35f, 0.8f), LANCEUR, GEL, 0, 0, 0, 5, 12, 175.0f},
		{TEXT("Prophete Haschen"), TEXT("chaman"), 48, 19, 3, FLinearColor(0.8f, 0.6f, 1.0f), LANCEUR, POISON, SOIGNEUR | INVOCATEUR, 0, 0, 4, 10, 180.0f},
	},
};

static const FVespModeleHaschen ELITES[7][2] = {
	{{TEXT("Haschen guerrier d'elite"), TEXT("guerrier"), 40, 11, 3, OR, MELEE, 0, 0, 0, 0, 4, 15, 210.0f},
	 {TEXT("Haschen hurleur"), TEXT("sbire"), 42, 12, 2, OR, CHARGEUR, 0, INVOCATEUR, 0, 0, 4, 15, 205.0f}},
	{{TEXT("Louvetier d'elite"), TEXT("sbire"), 46, 13, 3, OR, CHARGEUR, 0, INVOCATEUR, 0, 0, 4, 15, 210.0f},
	 {TEXT("Brute des tombes"), TEXT("guerrier"), 58, 14, 4, OR, MELEE, 0, VAMPIRE, 0, 0, 4, 15, 228.0f}},
	{{TEXT("Noye colossal"), TEXT("guerrier"), 72, 16, 4, OR, MELEE, 0, VAMPIRE, 0, 1, 4, 15, 240.0f},
	 {TEXT("Mere des crapauds"), TEXT("sbire"), 62, 15, 2, OR, LANCEUR, POISON, INVOCATEUR, 0, 0, 5, 15, 190.0f}},
	{{TEXT("Capitaine de la forteresse"), TEXT("guerrier"), 78, 17, 5, OR, MELEE, 0, INVOCATEUR, 2, 0, 4, 15, 215.0f},
	 {TEXT("Golem ancien"), TEXT("guerrier"), 88, 17, 4, OR, MELEE, 0, 0, 3, 1, 4, 10, 240.0f}},
	{{TEXT("Alpha blanc"), TEXT("sbire"), 84, 19, 3, OR, CHARGEUR, GEL, INVOCATEUR, 0, 5, 4, 15, 215.0f},
	 {TEXT("Garde pourpre d'Ashka"), TEXT("guerrier"), 92, 19, 5, OR, MELEE, 0, 0, 2, 0, 4, 15, 215.0f}},
	{{TEXT("Heraut des cendres"), TEXT("chaman"), 96, 21, 4, OR, LANCEUR, BRULURE, INVOCATEUR | SOIGNEUR, 0, 0, 5, 15, 210.0f},
	 {TEXT("Colosse ardent"), TEXT("guerrier"), 115, 22, 6, OR, MELEE, BRULURE, 0, 3, 1, 4, 10, 250.0f}},
	{{TEXT("Grand Echo"), TEXT("Aylis"), 112, 24, 5, OR, MELEE, 0, SAUTEUR | VAMPIRE, 0, 0, 4, 20, 200.0f},
	 {TEXT("Champion de Karn"), TEXT("guerrier"), 135, 24, 7, OR, MELEE, 0, 0, 3, 0, 4, 15, 245.0f}},
};

static const FVespModeleHaschen BOSS[7] = {
	{TEXT("Skarn le Brise-Cranes"), TEXT("guerrier"), 90, 13, 3, FLinearColor(0.5f, 0.5f, 0.6f), CHARGEUR, 0, 0, 0, 0, 4, 10, 260.0f},
	{TEXT("La Matriarche"), TEXT("chaman"), 110, 14, 3, FLinearColor(0.3f, 0.65f, 0.35f), LANCEUR, POISON, 0, 0, 0, 5, 10, 250.0f},
	{TEXT("Le Roi Noye"), TEXT("guerrier"), 160, 17, 4, FLinearColor(0.2f, 0.5f, 0.45f), MELEE, POISON, VAMPIRE, 0, 2, 4, 10, 265.0f},
	{TEXT("Le Gardien de Pierre"), TEXT("guerrier"), 180, 19, 5, FLinearColor(0.55f, 0.55f, 0.55f), MELEE, 0, 0, 3, 1, 4, 5, 290.0f},
	{TEXT("Ashka"), TEXT("traqueur"), 190, 21, 5, FLinearColor(0.6f, 0.15f, 0.35f), LANCEUR, GEL, 0, 0, 2, 6, 15, 240.0f},
	{TEXT("Vorgath le Destructeur"), TEXT("guerrier"), 230, 23, 6, FLinearColor(0.3f, 0.1f, 0.05f), CHARGEUR, BRULURE, 0, 0, 0, 4, 10, 300.0f},
	{TEXT("L'Oracle de Karn"), TEXT("chaman"), 270, 25, 6, FLinearColor(0.75f, 0.55f, 1.0f), LANCEUR, POISON, 0, 0, 0, 6, 12, 260.0f},
};

// Les armes des personnages (plus bas)
static FString Arme(const TCHAR* Dossier, const TCHAR* Nom);
static void Armer(AVespUnite* H, const FVespModeleHaschen& M, int32 BossActe);

// ===================== La mise en route =====================

AVespPlayerController::AVespPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AVespPlayerController::Ecrire(const FString& Message)
{
	Journal.Add(Message);
	while (Journal.Num() > 4)
	{
		Journal.RemoveAt(0);
	}
}

void AVespPlayerController::Commencer(AVespGrille* LaGrille, AVespMonde* LeMonde, AVespUnite* LAylis, ACameraActor* LaCamera)
{
	Grille = LaGrille;
	Monde = LeMonde;
	Aylis = LAylis;
	CameraArene = LaCamera;
	PositionCamera = CameraArene ? CameraArene->GetActorLocation() : FVector::ZeroVector;
	RotationCamera = CameraArene ? CameraArene->GetActorRotation() : FRotator::ZeroRotator;
	DecalageCamera = PositionCamera - Grille->GetActorLocation();
	CameraActuelle = PositionCamera;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	// L'interface (Slate) : posee par-dessus l'ecran de jeu
	if (GEngine && GEngine->GameViewport)
	{
		Interface = SNew(SVespInterface).Joueur(this);
		GEngine->GameViewport->AddViewportWidgetContent(Interface.ToSharedRef());
	}
	// AYLIS, voie de l'epee
	Aylis->Equiper(Arme(TEXT("Sword"), TEXT("SK_Sword_1H_Newbie_02")), 0.5f);
	// Le monde du premier acte ; l'ecran titre le montre, la camera tourne lentement autour d'AYLIS
	GenererMonde();
	Phase = EVespPhase::Titre;
	TempsPhase = 0.0f;
	bModePhoto = FParse::Param(FCommandLine::Get(), TEXT("VespPhotos"));
	PhotoAttente = 8.0f;
}

void AVespPlayerController::EndPlay(const EEndPlayReason::Type Raison)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	if (Interface.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Interface.ToSharedRef());
	}
	Interface.Reset();
	Super::EndPlay(Raison);
}

void AVespPlayerController::NouvellePartie(int32 ActeDeDepart)
{
	if (Phase != EVespPhase::Titre)
	{
		return;
	}
	Acte = FMath::Clamp(ActeDeDepart, 1, NombreDActes);
	if (Acte > 1)
	{
		// Commencer plus loin (pour tester) : AYLIS recoit des forces a la hauteur de l'acte
		const int32 Avance = Acte - 1;
		Aylis->Stats.PvMax += 14 * Avance;
		Aylis->Stats.Pv = Aylis->Stats.PvMax;
		Aylis->Stats.Attaque += 3 * Avance;
		Aylis->Stats.Defense += Avance;
		Potions += Avance;
		Eclats += 40 * Avance;
	}
	AmbianceDeLActe();
	Phase = EVespPhase::NouvelActe;
	TempsPhase = 0.0f;
}

void AVespPlayerController::Quitter()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
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
		case EVespAction::Lourde: return TEXT("Degats x1.8, mais rate 4 fois sur 10. Fissure une plaque d'armure.");
		case EVespAction::Garde: return TEXT("Petits degats, et tu encaisses 2 fois moins jusqu'a ton prochain tour.");
		case EVespAction::Potion: return TEXT("Rend 15 pv tout de suite (ton tour est fini).");
		default: return TEXT("Quand la rage est pleine : degats x2.2, ne rate jamais, fissure les armures.");
	}
}

FString AVespPlayerController::NomSalle(EVespSalle S, int32 LActe)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Combat");
		case EVespSalle::Elite: return TEXT("Elite");
		case EVespSalle::Repos: return TEXT("Feu de camp");
		case EVespSalle::Marchand: return TEXT("Marchand");
		case EVespSalle::Evenement: return TEXT("Inconnu");
		case EVespSalle::Depart: return TEXT("Depart");
		case EVespSalle::Tresor: return TEXT("Tresor");
		default: return BOSS[FMath::Clamp(LActe, 1, 7) - 1].Nom;
	}
}

FString AVespPlayerController::AideSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Des Haschen, parfois en plusieurs vagues. Des eclats et une rune a la fin.");
		case EVespSalle::Elite: return TEXT("Un Haschen d'elite et son escorte. Dur... mais beaucoup d'eclats.");
		case EVespSalle::Repos: return TEXT("Un feu de camp : +60% pv et une potion.");
		case EVespSalle::Marchand: return TEXT("Un marchand ambulant. Des potions, des runes... contre des eclats.");
		case EVespSalle::Evenement: return TEXT("La vision est trouble. Une rencontre, un tresor... ou un piege.");
		case EVespSalle::Depart: return TEXT("Le debut de la route.");
		case EVespSalle::Tresor: return TEXT("Un coffre, au bout du chemin : des eclats et une rune.");
		default: return TEXT("Le gardien de cette terre.");
	}
}

FString AVespPlayerController::LettreSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("X");
		case EVespSalle::Elite: return TEXT("E");
		case EVespSalle::Repos: return TEXT("R");
		case EVespSalle::Marchand: return TEXT("$");
		case EVespSalle::Evenement: return TEXT("?");
		case EVespSalle::Depart: return TEXT("");
		case EVespSalle::Tresor: return TEXT("T");
		default: return TEXT("B");
	}
}

FLinearColor AVespPlayerController::CouleurSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return FLinearColor(0.92f, 0.42f, 0.36f);
		case EVespSalle::Elite: return FLinearColor(0.95f, 0.7f, 0.25f);
		case EVespSalle::Repos: return FLinearColor(0.48f, 0.85f, 0.52f);
		case EVespSalle::Marchand: return FLinearColor(0.45f, 0.8f, 0.95f);
		case EVespSalle::Evenement: return FLinearColor(0.75f, 0.58f, 1.0f);
		case EVespSalle::Depart: return FLinearColor(0.73f, 0.55f, 1.0f);
		case EVespSalle::Tresor: return FLinearColor(1.0f, 0.85f, 0.4f);
		default: return FLinearColor(1.0f, 0.3f, 0.3f);
	}
}

FString AVespPlayerController::Romain(int32 Nombre)
{
	static const TCHAR* R[] = {TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI"), TEXT("VII")};
	return R[FMath::Clamp(Nombre, 1, 7) - 1];
}

// Les runes de prophetie : Vigueur, Tranchant et Pierre se cumulent ; Vent deux fois au plus ; les autres une seule fois
static const TCHAR* NOMS_RUNES[] = {TEXT("Vigueur"), TEXT("Tranchant"), TEXT("Pierre"), TEXT("Vent"), TEXT("Flamme"), TEXT("Seve"),
                                    TEXT("Fureur"), TEXT("Epines"), TEXT("Sangsue"), TEXT("Precision"), TEXT("Fortune"), TEXT("Rempart"),
                                    TEXT("Pied sur"), TEXT("Givre")};
static const TCHAR* AIDES_RUNES[] = {TEXT("+8 pv max"), TEXT("+2 attaque"), TEXT("+1 defense"), TEXT("+1 case de deplacement"),
                                     TEXT("Tes coups ont 1 chance sur 3 de bruler"), TEXT("+5 pv a chaque Haschen abattu"),
                                     TEXT("La rage monte 2 fois plus vite"), TEXT("Qui touche AYLIS au contact perd 3 pv"),
                                     TEXT("+2 pv a chaque coup porte"), TEXT("+12% de chances de critique"),
                                     TEXT("+50% d'eclats apres chaque combat"), TEXT("La garde divise les degats par 3"),
                                     TEXT("La lave, les eaux toxiques et les pieges n'atteignent plus AYLIS"),
                                     TEXT("Tes coups ont 1 chance sur 4 de geler")};
static constexpr int32 NOMBRE_RUNES = 14;

FString AVespPlayerController::NomRune(int32 Rune) { return NOMS_RUNES[FMath::Clamp(Rune, 0, NOMBRE_RUNES - 1)]; }
FString AVespPlayerController::AideRune(int32 Rune) { return AIDES_RUNES[FMath::Clamp(Rune, 0, NOMBRE_RUNES - 1)]; }

FString AVespPlayerController::NomDuLieu() const
{
	return TypeSalle == EVespSalle::Boss ? InfoActe(Acte).LieuDuBoss : InfoActe(Acte).Lieu;
}

FString AVespPlayerController::NomDeLActe() const { return InfoActe(Acte).Titre; }
FString AVespPlayerController::RegleDeLActe() const { return InfoActe(Acte).Regle; }
FString AVespPlayerController::NomDuBoss() const { return BOSS[FMath::Clamp(Acte, 1, 7) - 1].Nom; }

// ===================== Les evenements (les salles "?") =====================
// Toujours sans genre pour AYLIS.

struct FVespTexteEvenement
{
	const TCHAR* Titre;
	const TCHAR* Texte;
	const TCHAR* Choix[2];
	const TCHAR* Aide[2];
	int32 ActeMin;
	int32 ActeMax;
};

static const FVespTexteEvenement EVENEMENTS[] = {
	{TEXT("L'autel oublie"),
	 TEXT("Une pierre couverte de runes, a moitie avalee par la mousse. Une voix murmure : du sang contre une vision."),
	 {TEXT("Offrir son sang"), TEXT("Passer son chemin")}, {TEXT("-8 pv, une rune au hasard"), TEXT("Rien ne se passe")}, 1, 7},
	{TEXT("La source claire"),
	 TEXT("Une eau si pure qu'elle brille dans la nuit. Les feux follets tournent autour sans oser la toucher."),
	 {TEXT("Boire"), TEXT("Remplir une fiole")}, {TEXT("+35% pv"), TEXT("+1 potion")}, 1, 7},
	{TEXT("Le Haschen blesse"),
	 TEXT("Un eclaireur Haschen, adosse a un arbre. Il ne peut plus se battre. Il te regarde sans rien dire."),
	 {TEXT("L'achever"), TEXT("L'epargner")}, {TEXT("+25 eclats"), TEXT("+6 pv max : la prophetie s'en souviendra")}, 1, 7},
	{TEXT("Le coffre sous la mousse"),
	 TEXT("Un coffre a moitie enterre. La serrure est rouillee... ou piegee ? Quelque chose bouge dans les fourres."),
	 {TEXT("L'ouvrir"), TEXT("Le laisser")}, {TEXT("Une chance sur deux : 45 eclats... ou une embuscade"), TEXT("Rien ne se passe")}, 1, 7},
	{TEXT("Le colporteur des brumes"),
	 TEXT("Une silhouette encapuchonnee sort du brouillard. Elle tend une main pleine de runes et reclame tes potions."),
	 {TEXT("Echanger 2 potions"), TEXT("Refuser")}, {TEXT("-2 potions, une rune au hasard"), TEXT("La silhouette disparait")}, 1, 7},
	{TEXT("Les pendus"),
	 TEXT("Des cordes grincent au-dessus du sentier. L'un des pendus ouvre les yeux et murmure le nom d'AYLIS."),
	 {TEXT("Ecouter"), TEXT("Couper la corde")}, {TEXT("-10 pv, +2 attaque"), TEXT("+30 eclats")}, 2, 3},
	{TEXT("Le feu des voyageurs"),
	 TEXT("Un feu encore tiede, abandonne en hate. Des provisions, et des sacs a moitie ouverts."),
	 {TEXT("Manger"), TEXT("Fouiller les sacs")}, {TEXT("+20% pv"), TEXT("+20 eclats")}, 1, 7},
	{TEXT("Le deserteur"),
	 TEXT("Un jeune Haschen sans arme tremble derriere un rocher. Il a fui le camp d'Ashka."),
	 {TEXT("L'aider"), TEXT("Le chasser")}, {TEXT("-1 potion, +35 eclats"), TEXT("Il s'enfuit dans la nuit")}, 4, 6},
	{TEXT("La cloche engloutie"),
	 TEXT("Une cloche rouillee depasse de la vase. Si on la sonne, quelque chose repondra."),
	 {TEXT("Sonner la cloche"), TEXT("La laisser dormir")}, {TEXT("Un combat d'elite (et sa recompense)"), TEXT("Rien ne se passe")}, 3, 3},
	{TEXT("L'abri de pierre"),
	 TEXT("Le blizzard se leve. Un abri de pierre, a peine assez grand, et des traces de pas qui continuent dans la neige."),
	 {TEXT("S'abriter"), TEXT("Suivre les traces")}, {TEXT("+30% pv"), TEXT("-8 pv, +30 eclats")}, 5, 5},
	{TEXT("L'autel de braise"),
	 TEXT("Un autel de pierre noire, brulant. C'est ici que Vorgath trempe ses lames."),
	 {TEXT("Tremper l'epee"), TEXT("Refroidir l'autel")}, {TEXT("-12 pv, +3 attaque"), TEXT("+1 defense")}, 6, 6},
	{TEXT("Le miroir du Voile"),
	 TEXT("Une flaque d'argent reflete AYLIS... mais le reflet sourit, et tend la main."),
	 {TEXT("Toucher le reflet"), TEXT("Briser le miroir")}, {TEXT("Une chance sur deux : une rune... ou des echos"), TEXT("+40 eclats")}, 7, 7},
	{TEXT("La statue d'Ashka"),
	 TEXT("Une statue de la cheffe de guerre, couronnee d'epines. A ses pieds, des offrandes de ses guerriers."),
	 {TEXT("Prendre les offrandes"), TEXT("Briser la statue")}, {TEXT("+35 eclats, -6 pv"), TEXT("+1 attaque")}, 4, 5},
};
static constexpr int32 NOMBRE_EVENEMENTS = UE_ARRAY_COUNT(EVENEMENTS);

FString AVespPlayerController::TitreEvenement() const { return EVENEMENTS[EvenementActuel].Titre; }
FString AVespPlayerController::TexteEvenement() const { return EVENEMENTS[EvenementActuel].Texte; }
FString AVespPlayerController::ChoixEvenement(int32 Choix) const { return EVENEMENTS[EvenementActuel].Choix[Choix]; }
FString AVespPlayerController::AideEvenement(int32 Choix) const { return EVENEMENTS[EvenementActuel].Aide[Choix]; }

// ===================== Le monde de l'acte =====================
// Un sentier principal d'ouest en est : 9 clairieres, du depart jusqu'au boss. Et des embranchements de 1 a 3
// clairieres vers le nord ou le sud, qui finissent sur un tresor ou une elite.
// En ligne droite : 4 ou 5 combats et le boss (15 a 20 minutes). En explorant tout : 40 minutes et plus.

static EVespSalle SalleDEmbranchement(bool bDerniere)
{
	const int32 D = FMath::RandRange(0, 99);
	if (bDerniere)
	{
		return D < 55 ? EVespSalle::Tresor : EVespSalle::Elite;
	}
	if (D < 45) return EVespSalle::Combat;
	if (D < 60) return EVespSalle::Elite;
	if (D < 82) return EVespSalle::Evenement;
	if (D < 94) return EVespSalle::Marchand;
	return EVespSalle::Repos;
}

void AVespPlayerController::GenererMonde()
{
	Noeuds.Reset();
	ZoneActuelle = -1;
	MarchandZone = -1;
	Offres.Reset();
	const FVector Base = Monde->GetActorLocation();
	const float Pas = 2900.0f;
	// Le sentier principal
	const EVespSalle Principal[9] = {
		EVespSalle::Depart, EVespSalle::Combat, FMath::RandBool() ? EVespSalle::Combat : EVespSalle::Evenement, EVespSalle::Elite,
		FMath::RandBool() ? EVespSalle::Marchand : EVespSalle::Evenement, EVespSalle::Combat, EVespSalle::Combat, EVespSalle::Repos, EVespSalle::Boss};
	for (int32 i = 0; i < 9; i++)
	{
		FVespNoeud N;
		N.Type = Principal[i];
		N.Etage = i;
		N.Centre = Base + FVector(i > 0 && i < 8 ? FMath::FRandRange(-450.0f, 450.0f) : 0.0f, i * Pas, 0);
		if (i > 0)
		{
			Noeuds[i - 1].Suivants.Add(i);
		}
		Noeuds.Add(N);
	}
	// Les embranchements (au moins 3)
	int32 Branches = 0;
	for (int32 i = 1; i <= 6; i++)
	{
		if (FMath::RandRange(0, 99) >= 70 && !(i >= 5 && Branches < 3))
		{
			continue;
		}
		const float Cote = FMath::RandBool() ? 1.0f : -1.0f;
		const int32 Longueur = FMath::RandRange(1, 3);
		const FVector Depuis = Noeuds[i].Centre;
		const FVector Positions[3] = {
			FVector(Base.X + Cote * (2700.0f + FMath::FRandRange(-250.0f, 250.0f)), Depuis.Y + 1000.0f, Base.Z),
			FVector(Base.X + Cote * 5200.0f, Depuis.Y + 1800.0f, Base.Z),
			FVector(Base.X + Cote * 7700.0f, Depuis.Y + 1000.0f, Base.Z),
		};
		int32 Precedent = i;
		for (int32 k = 0; k < Longueur; k++)
		{
			FVespNoeud N;
			N.Type = SalleDEmbranchement(k == Longueur - 1);
			N.Etage = i + k + 1;
			N.Centre = Positions[k];
			const int32 Nouveau = Noeuds.Add(N);
			Noeuds[Precedent].Suivants.Add(Nouveau);
			Precedent = Nouveau;
		}
		Branches++;
	}
	Noeuds[0].bVisite = true;

	// On construit le monde (c'est le chargement de l'acte)
	TArray<FVespZone> Zones;
	TArray<FVespCouloir> Couloirs;
	for (int32 i = 0; i < Noeuds.Num(); i++)
	{
		const FVespNoeud& N = Noeuds[i];
		FVespZone Z;
		Z.Centre = N.Centre;
		Z.Type = (int32)N.Type;
		Z.Lettre = LettreSalle(N.Type);
		Z.Couleur = CouleurSalle(N.Type);
		Z.Rayon = N.Type == EVespSalle::Boss ? 1300.0f : ((N.Type == EVespSalle::Combat || N.Type == EVespSalle::Elite) ? 1050.0f : 850.0f);
		Zones.Add(Z);
		for (int32 S : N.Suivants)
		{
			FVespCouloir C;
			C.A = i;
			C.B = S;
			Couloirs.Add(C);
		}
	}
	Monde->Construire(Acte, Zones, Couloirs);

	// AYLIS au depart, tournee vers l'est
	Aylis->PasserEnModeLibre(true);
	Aylis->SetActorLocation(Noeuds[0].Centre + FVector(0, -250.0f, 0));
	Aylis->SetActorRotation(FRotator(0, 90.0f, 0));
	bCaleCamera = true;
	bCarteOuverte = false;
}

// AYLIS entre dans une clairiere
void AVespPlayerController::Declencher(int32 Zone)
{
	ZoneActuelle = Zone;
	FVespNoeud& N = Noeuds[Zone];
	Etage = N.Etage;
	MessageRoute.Reset();
	switch (N.Type)
	{
		case EVespSalle::Combat:
		case EVespSalle::Elite:
			PreparerCombat(N.Type);
			return;
		case EVespSalle::Boss:
			DialogueDuBoss();
			return;
		case EVespSalle::Evenement:
			N.bVisite = true;
			Monde->MarquerZoneFaite(Zone);
			OuvrirEvenement();
			return;
		case EVespSalle::Marchand:
			N.bVisite = true;
			OuvrirMarchand();
			return;
		case EVespSalle::Repos:
		{
			N.bVisite = true;
			Monde->MarquerZoneFaite(Zone);
			const int32 Soin = Aylis->Stats.PvMax * 60 / 100;
			Aylis->Soigner(Soin);
			Potions++;
			MessageRoute = FString::Printf(TEXT("AYLIS se repose au coin du feu : +%d pv et une potion."), Soin);
			TempsMessage = 5.0f;
			return;
		}
		case EVespSalle::Tresor:
		{
			N.bVisite = true;
			Monde->MarquerZoneFaite(Zone);
			const int32 Gain = 30 + Acte * 8 + FMath::RandRange(0, 10);
			Eclats += Gain;
			AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, N.Centre + FVector(0, 0, 80), FVector::UpVector, FLinearColor(1.0f, 0.85f, 0.4f));
			MessageRoute = FString::Printf(TEXT("Le coffre s'ouvre : +%d eclats... et une rune."), Gain);
			ProposerRunes();
			return;
		}
		default:
			return;
	}
}

// Retour a l'exploration : l'arene disparait, les Haschen tombes s'effacent, AYLIS reprend la route
void AVespPlayerController::RetourExploration()
{
	for (AVespUnite* H : Haschen)
	{
		AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, H->GetActorLocation(), FVector::UpVector, FLinearColor(0.5f, 0.35f, 0.6f));
		H->Destroy();
	}
	Haschen.Reset();
	ZonesDanger.Reset();
	Eruptions.Reset();
	bBlizzard = false;
	Grille->Effacer();
	if (!Aylis->EstLibre())
	{
		Aylis->PasserEnModeLibre(true);
	}
	Phase = EVespPhase::Exploration;
	TempsPhase = 0.0f;
	TempsMessage = MessageRoute.IsEmpty() ? 0.0f : 5.0f;
}

FString AVespPlayerController::Invite() const
{
	if (Phase == EVespPhase::Exploration && MarchandProche >= 0)
	{
		return TEXT("E  /  (A)  :  parler au marchand");
	}
	return FString();
}

// La case libre (et sans danger) de l'arene la plus proche d'une position du monde
FIntPoint AVespPlayerController::CaseLaPlusProche(const FVector& Position) const
{
	FIntPoint Meilleure(1, 3);
	float MeilleureDistance = MAX_flt;
	for (const FIntPoint& C : Grille->CasesLibres(0))
	{
		const float D = FVector::Dist2D(Grille->CentreDeCase(C), Position);
		if (D < MeilleureDistance)
		{
			MeilleureDistance = D;
			Meilleure = C;
		}
	}
	return Meilleure;
}

// Les cases libres loin d'une case (pour placer les Haschen de l'autre cote de l'arene)
TArray<FIntPoint> AVespPlayerController::PlacesLoinDe(FIntPoint Depart, int32 DistanceMin) const
{
	TArray<FIntPoint> Places;
	for (int32 D = DistanceMin; D >= 2 && Places.Num() < 6; D -= 2)
	{
		Places.Reset();
		for (const FIntPoint& C : Grille->CasesLibres(0))
		{
			const FIntPoint E = C - Depart;
			if (FMath::Abs(E.X) + FMath::Abs(E.Y) >= D)
			{
				Places.Add(C);
			}
		}
	}
	return Places;
}

// ===================== Le marchand =====================

bool AVespPlayerController::RunePossible(int32 R) const
{
	if (R < 3)
	{
		return true;
	}
	if (R == 3)
	{
		int32 Deja = 0;
		for (int32 X : Runes)
		{
			Deja += X == 3 ? 1 : 0;
		}
		return Deja < 2;
	}
	return !Runes.Contains(R);
}

int32 AVespPlayerController::RuneAuHasard() const
{
	TArray<int32> Possibles;
	for (int32 R = 0; R < NOMBRE_RUNES; R++)
	{
		if (RunePossible(R))
		{
			Possibles.Add(R);
		}
	}
	return Possibles[FMath::RandRange(0, Possibles.Num() - 1)];
}

void AVespPlayerController::OuvrirMarchand()
{
	Offres.Reset();
	auto Ajouter = [this](const FString& Nom, const FString& Aide, int32 Prix, int32 Genre, int32 R = 0) {
		FVespOffre O;
		O.Nom = Nom;
		O.Aide = Aide;
		O.Prix = Prix + Acte * 4 + FMath::RandRange(-3, 3);
		O.Genre = Genre;
		O.Rune = R;
		Offres.Add(O);
	};
	const int32 R = RuneAuHasard();
	Ajouter(TEXT("Potion"), TEXT("+1 potion (15 pv)"), 16, 0);
	Ajouter(TEXT("Rune de ") + NomRune(R), AideRune(R), 42, 1, R);
	TArray<int32> Autres = {2, 3, 4, 5};
	for (int32 k = 0; k < 2; k++)
	{
		const int32 Autre = Autres[FMath::RandRange(0, Autres.Num() - 1)];
		Autres.Remove(Autre);
		switch (Autre)
		{
			case 2: Ajouter(TEXT("Onguent"), TEXT("Rend 50% des pv"), 20, 2); break;
			case 3: Ajouter(TEXT("Pierre a aiguiser"), TEXT("+1 attaque"), 28, 3); break;
			case 4: Ajouter(TEXT("Amulette de sureau"), TEXT("+6 pv max"), 26, 4); break;
			default: Ajouter(TEXT("Bouclier cloute"), TEXT("+1 defense"), 30, 5); break;
		}
	}
	MarchandZone = ZoneActuelle;
	SelectionMenu = 0;
	Phase = EVespPhase::Marchand;
	TempsPhase = 0.0f;
}

void AVespPlayerController::RouvrirMarchand(int32 Zone)
{
	ZoneActuelle = Zone;
	if (Zone != MarchandZone || Offres.Num() == 0)
	{
		OuvrirMarchand();
		return;
	}
	SelectionMenu = 0;
	Phase = EVespPhase::Marchand;
	TempsPhase = 0.0f;
}

void AVespPlayerController::AcheterOffre(int32 Numero)
{
	if (Phase != EVespPhase::Marchand || !Offres.IsValidIndex(Numero))
	{
		return;
	}
	FVespOffre& O = Offres[Numero];
	if (O.bVendue || Eclats < O.Prix)
	{
		return;
	}
	Eclats -= O.Prix;
	O.bVendue = true;
	switch (O.Genre)
	{
		case 0: Potions++; break;
		case 1: AppliquerRune(O.Rune); break;
		case 2: Aylis->Soigner(Aylis->Stats.PvMax / 2); break;
		case 3: Aylis->Stats.Attaque += 1; break;
		case 4: Aylis->Stats.PvMax += 6; Aylis->Soigner(6); break;
		default: Aylis->Stats.Defense += 1; break;
	}
	AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Aylis->GetActorLocation() + FVector(0, 0, 100), FVector::UpVector, FLinearColor(1.0f, 0.8f, 0.35f));
}

void AVespPlayerController::QuitterMarchand()
{
	if (Phase == EVespPhase::Marchand)
	{
		MessageRoute = TEXT("\"Reviens quand tu veux. Je ne bouge pas d'ici.\"");
		RetourExploration();
	}
}

// ===================== Les evenements =====================

void AVespPlayerController::OuvrirEvenement()
{
	TArray<int32> Possibles;
	for (int32 i = 0; i < NOMBRE_EVENEMENTS; i++)
	{
		if (Acte >= EVENEMENTS[i].ActeMin && Acte <= EVENEMENTS[i].ActeMax)
		{
			Possibles.Add(i);
			if (EVENEMENTS[i].ActeMin == EVENEMENTS[i].ActeMax)
			{
				Possibles.Add(i);		// les evenements propres a un acte reviennent plus souvent
			}
		}
	}
	EvenementActuel = Possibles[FMath::RandRange(0, Possibles.Num() - 1)];
	SelectionMenu = 0;
	Phase = EVespPhase::Evenement;
	TempsPhase = 0.0f;
}

void AVespPlayerController::ChoisirEvenement(int32 Choix)
{
	if (Phase != EVespPhase::Evenement || TempsPhase < 0.4f)
	{
		return;
	}
	const FVector Ici = Aylis->GetActorLocation() + FVector(0, 0, 100);
	auto PerdrePv = [this](int32 N) { Aylis->Stats.Pv = FMath::Max(1, Aylis->Stats.Pv - N); };
	auto RuneOfferte = [this](const TCHAR* Debut) {
		const int32 R = RuneAuHasard();
		AppliquerRune(R);
		MessageRoute = FString::Printf(TEXT("%s la rune %s : %s."), Debut, *NomRune(R), *AideRune(R));
	};
	MessageRoute = TEXT("AYLIS reprend la route.");
	switch (EvenementActuel * 2 + Choix)
	{
		case 0:
			PerdrePv(8);
			RuneOfferte(TEXT("L'autel boit le sang... et offre"));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, Ici, FVector::UpVector, FLinearColor(0.9f, 0.2f, 0.3f));
			break;
		case 2:
		{
			const int32 Soin = Aylis->Stats.PvMax * 35 / 100;
			Aylis->Soigner(Soin);
			MessageRoute = FString::Printf(TEXT("L'eau est glacee et douce. +%d pv."), Soin);
			break;
		}
		case 3: Potions++; MessageRoute = TEXT("Une fiole de lumiere liquide : +1 potion."); break;
		case 4: Eclats += 25; MessageRoute = TEXT("Il ne dit rien, jusqu'au bout. +25 eclats."); break;
		case 5:
			Aylis->Stats.PvMax += 6;
			Aylis->Soigner(6);
			MessageRoute = TEXT("Le Haschen disparait dans la brume. +6 pv max : la prophetie s'en souviendra.");
			break;
		case 6:
			if (FMath::RandBool())
			{
				Eclats += 45;
				MessageRoute = TEXT("Le coffre cede : +45 eclats !");
				AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Ici, FVector::UpVector, FLinearColor(1.0f, 0.8f, 0.35f));
				break;
			}
			MessageRoute = TEXT("Une embuscade ! Les fourres s'ouvrent...");
			PreparerCombat(EVespSalle::Combat);
			return;
		case 8:
			if (Potions >= 2)
			{
				Potions -= 2;
				RuneOfferte(TEXT("Marche conclu :"));
			}
			else
			{
				MessageRoute = TEXT("Pas assez de potions... la silhouette s'efface en riant.");
			}
			break;
		case 10:
			PerdrePv(10);
			Aylis->Stats.Attaque += 2;
			MessageRoute = TEXT("Le murmure brule... mais l'epee semble plus lourde de colere. +2 attaque.");
			AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, Ici, FVector::UpVector, FLinearColor(0.5f, 1.0f, 0.4f));
			break;
		case 11: Eclats += 30; MessageRoute = TEXT("Le corps tombe dans la mousse. Dans sa poche : +30 eclats."); break;
		case 12:
		{
			const int32 Soin = Aylis->Stats.PvMax / 5;
			Aylis->Soigner(Soin);
			MessageRoute = FString::Printf(TEXT("Un vrai repas, enfin. +%d pv."), Soin);
			break;
		}
		case 13: Eclats += 20; MessageRoute = TEXT("Au fond d'un sac : +20 eclats."); break;
		case 14:
			if (Potions > 0)
			{
				Potions--;
				Eclats += 35;
				MessageRoute = TEXT("Il boit, et glisse 35 eclats dans ta main : \"Ashka a peur de la prophetie... elle a peur de toi.\"");
			}
			else
			{
				Eclats += 10;
				MessageRoute = TEXT("Tu n'as rien a lui donner. Il file quand meme, en laissant tomber 10 eclats.");
			}
			break;
		case 16:
			MessageRoute = TEXT("La cloche sonne sous la vase... et quelque chose repond.");
			PreparerCombat(EVespSalle::Elite);
			return;
		case 18:
		{
			const int32 Soin = Aylis->Stats.PvMax * 30 / 100;
			Aylis->Soigner(Soin);
			MessageRoute = FString::Printf(TEXT("Le vent hurle dehors. A l'abri, AYLIS reprend des forces : +%d pv."), Soin);
			break;
		}
		case 19:
			PerdrePv(8);
			Eclats += 30;
			MessageRoute = TEXT("Les traces menent a un campement gele. -8 pv, +30 eclats.");
			break;
		case 20:
			PerdrePv(12);
			Aylis->Stats.Attaque += 3;
			MessageRoute = TEXT("La lame rougit, puis noircit. -12 pv, +3 attaque.");
			AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, Ici, FVector::UpVector, FLinearColor(1.0f, 0.45f, 0.1f));
			break;
		case 21: Aylis->Stats.Defense += 1; MessageRoute = TEXT("La pierre refroidit en sifflant. +1 defense."); break;
		case 22:
			if (FMath::RandBool())
			{
				RuneOfferte(TEXT("Le reflet se fond dans AYLIS et laisse"));
				break;
			}
			MessageRoute = TEXT("Le reflet sort du miroir... et il n'est pas seul.");
			PreparerCombat(EVespSalle::Combat);
			return;
		case 23: Eclats += 40; MessageRoute = TEXT("Le miroir vole en eclats : +40 eclats."); break;
		case 24: PerdrePv(6); Eclats += 35; MessageRoute = TEXT("Les epines de la statue griffent AYLIS. -6 pv, +35 eclats."); break;
		case 25: Aylis->Stats.Attaque += 1; MessageRoute = TEXT("La couronne d'epines roule dans la poussiere. +1 attaque."); break;
		default: break;
	}
	RetourExploration();
}

// ===================== Les Haschen =====================

AVespUnite* AVespPlayerController::CreerHaschen(const FVespModeleHaschen& M, FIntPoint Case, bool bSelonEtage)
{
	FVespStats S;
	S.Nom = M.Nom;
	// Plus AYLIS avance dans l'acte, plus les Haschen sont coriaces
	S.PvMax = M.Pv + (bSelonEtage ? Etage * 2 : 0);
	S.Pv = S.PvMax;
	S.Attaque = M.Attaque + (bSelonEtage ? Etage / 4 : 0);
	S.Defense = M.Defense;
	S.Style = M.Style;
	S.Effet = M.Effet;
	S.Capacites = M.Capacites;
	S.Armure = M.Armure;
	S.ArmureMax = M.Armure;
	S.Pas = M.Pas;
	S.Portee = M.Portee;
	S.ChanceCritique = M.Critique;
	AVespUnite* H = GetWorld()->SpawnActor<AVespUnite>(FVector::ZeroVector, FRotator(0, 180, 0));
	H->Taille = M.Taille;
	H->Preparer(Grille, Case, S, DOSSIER + M.Dossier, M.Teinte, false);
	const bool bBoss = &M >= &BOSS[0] && &M <= &BOSS[6];
	Armer(H, M, bBoss ? int32(&M - &BOSS[0]) + 1 : 0);
	Haschen.Add(H);
	return H;
}

// ===================== Les armes (pack StylizedCharacter) =====================

static FString Arme(const TCHAR* Dossier, const TCHAR* Nom)
{
	return FString::Printf(TEXT("/Game/StylizedCharacter/Meshes/Item/Weapons/%s/%s.%s"), Dossier, Nom, Nom);
}

// Chaque Haschen a l'arme de son metier : hache (guerrier), arc (traqueur), baton (chaman), dague (sbire)...
// Les grands (elites) ont des armes plus lourdes ; chaque boss a la sienne.
static void Armer(AVespUnite* H, const FVespModeleHaschen& M, int32 BossActe)
{
	switch (BossActe)
	{
		case 1: H->Equiper(Arme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01")), 0.7f); return;					// Skarn et sa masse
		case 2: H->Equiper(Arme(TEXT("Staff"), TEXT("SK_Staff_Newbie_03")), 0.95f); return;				// la Matriarche
		case 3: H->Equiper(Arme(TEXT("Sword"), TEXT("SK_Sword_2H_Newbie_02")), 0.65f); return;			// le Roi Noye
		case 4: return;																						// le Gardien frappe de ses poings de pierre
		case 5: H->Equiper(Arme(TEXT("Bow"), TEXT("SK_Bow_Newbie_03")), 0.75f, true); return;				// Ashka et son arc
		case 6: H->Equiper(Arme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01")), 0.75f, false, Arme(TEXT("Shield"), TEXT("SK_Shield_Newbie_03"))); return;
		case 7: H->Equiper(Arme(TEXT("Staff"), TEXT("SK_Staff_Newbie_04")), 1.0f); return;				// l'Oracle
		default: break;
	}
	const FString Dossier = M.Dossier;
	const bool bGrand = M.Taille >= 205.0f;
	if (Dossier == TEXT("guerrier"))
	{
		H->Equiper(bGrand ? Arme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01")) : Arme(TEXT("Axe"), TEXT("SK_Axe_1H_Newbie_02")), bGrand ? 0.65f : 0.45f, false,
		           M.Defense >= 5 ? Arme(TEXT("Shield"), TEXT("SK_Shield_Newbie_01")) : FString());
	}
	else if (Dossier == TEXT("traqueur"))
	{
		H->Equiper(Arme(TEXT("Bow"), bGrand ? TEXT("SK_Bow_Newbie_02") : TEXT("SK_Bow_Newbie_01")), 0.65f, true);
	}
	else if (Dossier == TEXT("chaman"))
	{
		H->Equiper(Arme(TEXT("Staff"), bGrand ? TEXT("SK_Staff_Newbie_02") : TEXT("SK_Staff_Newbie_01")), 0.9f);
	}
	else if (Dossier == TEXT("Aylis"))
	{
		H->Equiper(Arme(TEXT("Sword"), TEXT("SK_Sword_1H_Newbie_01")), 0.5f);			// les echos d'AYLIS
	}
	else
	{
		H->Equiper(Arme(TEXT("Dagger"), bGrand ? TEXT("SK_Dagger_1H_Newbie_03") : TEXT("SK_Dagger_1H_Newbie_01")), 0.32f);
	}
}

static FIntPoint TirerPlace(TArray<FIntPoint>& Places)
{
	const int32 i = FMath::RandRange(0, Places.Num() - 1);
	const FIntPoint P = Places[i];
	Places.RemoveAt(i);
	return P;
}

AVespUnite* AVespPlayerController::HaschenAuHasard(TArray<FIntPoint>& Places)
{
	if (Places.Num() == 0)
	{
		return nullptr;
	}
	return CreerHaschen(HASCHEN[Acte - 1][FMath::RandRange(0, 4)], TirerPlace(Places));
}

AVespUnite* AVespPlayerController::EliteAuHasard(TArray<FIntPoint>& Places)
{
	if (Places.Num() == 0)
	{
		return nullptr;
	}
	return CreerHaschen(ELITES[Acte - 1][FMath::RandRange(0, 1)], TirerPlace(Places));
}

int32 AVespPlayerController::HaschenDebout() const
{
	int32 N = 0;
	for (const AVespUnite* H : Haschen)
	{
		N += H->EstDebout() ? 1 : 0;
	}
	return N;
}

// Des renforts appeles par un Haschen (ou un boss) : ils apparaissent pres de lui, et ne jouent qu'au tour suivant
int32 AVespPlayerController::AppelerRenforts(AVespUnite* Source, const FVespModeleHaschen& Modele, int32 Nombre)
{
	int32 Venus = 0;
	for (int32 i = 0; i < Nombre && HaschenDebout() < HaschenMax; i++)
	{
		FIntPoint Case;
		if (!CaseLibrePres(Source->GetCase(), Case))
		{
			break;
		}
		AVespUnite* R = CreerHaschen(Modele, Case);
		R->bVientDArriver = true;
		AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, R->GetActorLocation(), FVector::UpVector, FLinearColor(0.8f, 0.35f, 0.3f));
		Venus++;
	}
	return Venus;
}

// Une case libre et sans danger pres d'une autre, de plus en plus loin (1, 2, 3 cases...)
bool AVespPlayerController::CaseLibrePres(FIntPoint Centre, FIntPoint& Trouvee, int32 RayonMax) const
{
	for (int32 Rayon = 1; Rayon <= RayonMax; Rayon++)
	{
		TArray<FIntPoint> Candidates;
		for (int32 C = -Rayon; C <= Rayon; C++)
		{
			for (int32 L = -Rayon; L <= Rayon; L++)
			{
				const FIntPoint P = Centre + FIntPoint(C, L);
				if (FMath::Abs(C) + FMath::Abs(L) == Rayon && !Grille->EstBloquee(P) && !Grille->UniteSur(P) && !Grille->EstDangereuse(P)
				    && Grille->TerrainSur(P) != '@')
				{
					Candidates.Add(P);
				}
			}
		}
		if (Candidates.Num() > 0)
		{
			Trouvee = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			return true;
		}
	}
	return false;
}

// Une nouvelle vague : des Haschen surgissent a droite de l'arene
void AVespPlayerController::LancerVague()
{
	VaguesRestantes--;
	VagueActuelle++;
	TArray<FIntPoint> Places = PlacesLoinDe(Aylis->GetCase(), 5);
	const int32 Nombre = 2 + (Acte >= 3 ? 1 : 0) + (Etage >= 8 ? 1 : 0);
	for (int32 i = 0; i < Nombre; i++)
	{
		if (AVespUnite* H = HaschenAuHasard(Places))
		{
			H->bVientDArriver = true;
			AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, H->GetActorLocation(), FVector::UpVector, FLinearColor(0.8f, 0.35f, 0.3f));
		}
	}
	Trembler(0.6f);
	Ecrire(FString::Printf(TEXT("Vague %d : des renforts surgissent !"), VagueActuelle));
}

bool AVespPlayerController::VerifierFinDeVague()
{
	if (ResteDesHaschen() || Phase == EVespPhase::Defaite)
	{
		return false;
	}
	if (VaguesRestantes > 0)
	{
		LancerVague();
		return false;
	}
	ApresVictoire();
	return true;
}

// ===================== Les combats =====================

void AVespPlayerController::PreparerCombat(EVespSalle Type)
{
	TypeSalle = Type;
	// On range le combat d'avant : les Haschen tombes disparaissent, et une autre carte
	for (AVespUnite* H : Haschen)
	{
		H->Destroy();
	}
	Haschen.Reset();
	// L'arene apparait dans la clairiere, et AYLIS y entre, sur la case la plus proche de l'endroit ou AYLIS se tient
	const FVector Centre = Noeuds.IsValidIndex(ZoneActuelle) ? Noeuds[ZoneActuelle].Centre : Aylis->GetActorLocation();
	Aylis->PasserEnModeLibre(false);
	Grille->ViderOccupants();
	Grille->SetActorLocation(FVector(Centre.X, Centre.Y, Grille->GetActorLocation().Z));
	Grille->PreparerCarte(Acte, Type == EVespSalle::Boss);
	AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Circle2"), Grille->GetActorLocation() + FVector(0, 0, 5), FRotator::ZeroRotator, 3.0f);
	const FIntPoint Depart = CaseLaPlusProche(Aylis->GetActorLocation());
	Aylis->Replacer(Depart);
	Aylis->SetActorRotation(FRotator(0, 90.0f, 0));
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.6f, 0.5f, 1.0f));
	Curseur = Depart;
	bCarteOuverte = false;
	ZonesDanger.Reset();
	Eruptions.Reset();
	bBlizzard = false;
	Journal.Reset();
	VagueActuelle = 1;
	VaguesRestantes = 0;

	// Les Haschen : de l'autre cote de l'arene
	TArray<FIntPoint> Places = PlacesLoinDe(Depart, 6);
	if (Type == EVespSalle::Boss)
	{
		// Le boss : la case libre la plus eloignee d'AYLIS
		FIntPoint CaseDuBoss = Places.Num() > 0 ? Places[0] : FIntPoint(9, 3);
		int32 Loin = -1;
		for (const FIntPoint& C : Places)
		{
			const FIntPoint E = C - Depart;
			if (FMath::Abs(E.X) + FMath::Abs(E.Y) > Loin)
			{
				Loin = FMath::Abs(E.X) + FMath::Abs(E.Y);
				CaseDuBoss = C;
			}
		}
		Places.Remove(CaseDuBoss);
		AVespUnite* Boss = CreerHaschen(BOSS[Acte - 1], CaseDuBoss, false);
		Boss->Stats.Boss = Acte;
		PotionsDuBoss = 2;
		HaschenAuHasard(Places);
		HaschenAuHasard(Places);
	}
	else if (Type == EVespSalle::Elite)
	{
		EliteAuHasard(Places);
		HaschenAuHasard(Places);
		if (Acte >= 4 || Etage >= 6)
		{
			HaschenAuHasard(Places);
		}
		VaguesRestantes = Acte >= 3 ? 1 : 0;
	}
	else
	{
		int32 Nombre = (Acte == 1 && Etage <= 1) ? 2 : 3;
		Nombre += Etage >= 6 ? 1 : 0;
		Nombre += (Acte >= 5 && Etage >= 3) ? 1 : 0;
		for (int32 i = 0; i < FMath::Min(Nombre, 5); i++)
		{
			HaschenAuHasard(Places);
		}
		VaguesRestantes = (Etage >= 4 ? 1 : 0) + (Acte >= 4 && Etage >= 6 ? 1 : 0);
	}
	// Chaque Haschen surgit dans une bouffee de brume
	for (AVespUnite* H : Haschen)
	{
		AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, H->GetActorLocation(), FVector::UpVector, FLinearColor(0.5f, 0.3f, 0.3f));
	}
	Tour = 0;
	Ecrire(FString::Printf(TEXT("Acte %s : %s"), *Romain(Acte), *NomDuLieu()));
	if (VaguesRestantes > 0)
	{
		Ecrire(FString::Printf(TEXT("D'autres Haschen attendent dans l'ombre (%d vague%s de plus)."), VaguesRestantes, VaguesRestantes > 1 ? TEXT("s") : TEXT("")));
	}
	TourDAylis();
}

void AVespPlayerController::ApresVictoire()
{
	Grille->AfficherCasesAtteignables({});
	ZonesDanger.Reset();
	Eruptions.Reset();
	Grille->AfficherDanger({});
	Grille->LeverPieges(false);
	if (Noeuds.IsValidIndex(ZoneActuelle))
	{
		Noeuds[ZoneActuelle].bVisite = true;
		Monde->MarquerZoneFaite(ZoneActuelle);
	}
	if (TypeSalle == EVespSalle::Boss)
	{
		if (Acte >= NombreDActes)
		{
			Phase = EVespPhase::Victoire;		// le dernier boss est tombe
			TempsPhase = 0.0f;
			return;
		}
		// Un nouvel acte : AYLIS reprend toutes ses forces, et la prophetie la renforce
		Acte++;
		Etage = 0;
		Eclats += 60 + Acte * 10;
		Aylis->Stats.PvMax += 6;
		Aylis->Stats.Attaque += 1;
		Aylis->Soigner(Aylis->Stats.PvMax);
		Aylis->Poison = 0;
		Aylis->Brulure = 0;
		Aylis->Gel = 0;
		Potions++;
		AmbianceDeLActe();
		Phase = EVespPhase::NouvelActe;
		TempsPhase = 0.0f;
		return;
	}
	// Des eclats, AYLIS reprend son souffle, puis choisit une rune
	int32 Gain = TypeSalle == EVespSalle::Elite ? FMath::RandRange(32, 42) + Acte * 6 : FMath::RandRange(12, 18) + Acte * 3;
	if (bFortune)
	{
		Gain = Gain * 3 / 2;
	}
	Eclats += Gain;
	const int32 Soin = Aylis->Stats.PvMax / 5;
	Aylis->Soigner(Soin);
	MessageRoute = FString::Printf(TEXT("Victoire ! +%d eclats, et AYLIS reprend son souffle : +%d pv."), Gain, Soin);
	ProposerRunes();
}

// L'ambiance de l'acte : la brume, la lune, le ciel, et le monde autour de l'arene
void AVespPlayerController::AmbianceDeLActe()
{
	const FVespInfoActe& A = InfoActe(Acte);
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		It->GetComponent()->SetFogInscatteringColor(A.Brume);
		It->GetComponent()->SetFogDensity(A.Densite);
	}
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		It->GetLightComponent()->SetLightColor(A.Lune);
		It->GetLightComponent()->SetIntensity(A.IntensiteLune);
	}
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		It->GetLightComponent()->SetIntensity(A.Ciel);
	}
	for (AVespUnite* H : Haschen)
	{
		H->Destroy();
	}
	Haschen.Reset();
	Grille->ViderOccupants();
	TypeSalle = EVespSalle::Combat;
	ZonesDanger.Reset();
	Eruptions.Reset();
	Grille->Effacer();
	GenererMonde();
}

void AVespPlayerController::ContinuerApresLActe()
{
	if (Phase == EVespPhase::NouvelActe && TempsPhase > 1.0f)
	{
		MessageRoute = FString::Printf(TEXT("%s. Suis le sentier vers l'est : %s attend au bout."), InfoActe(Acte).Lieu, *NomDuBoss());
		RetourExploration();
	}
}

void AVespPlayerController::ProposerRunes()
{
	TArray<int32> Possibles;
	for (int32 R = 0; R < NOMBRE_RUNES; R++)
	{
		if (RunePossible(R))
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
	SelectionMenu = 0;
	Phase = EVespPhase::ChoixRune;
	TempsPhase = 0.0f;
}

void AVespPlayerController::AppliquerRune(int32 R)
{
	Runes.Add(R);
	FVespStats& S = Aylis->Stats;
	switch (R)
	{
		case 0: S.PvMax += 8; Aylis->Soigner(8); break;
		case 1: S.Attaque += 2; break;
		case 2: S.Defense += 1; break;
		case 3: DeplacementParTour += 1; break;
		case 4: bFlamme = true; break;
		case 5: bSeve = true; break;
		case 6: bFureur = true; break;
		case 7: bEpines = true; break;
		case 8: bSangsue = true; break;
		case 9: S.ChanceCritique += 12; break;
		case 10: bFortune = true; break;
		case 11: bRempart = true; break;
		case 12: bPiedSur = true; break;
		default: bGivre = true; break;
	}
	AVespEffet::Jouer(GetWorld(), EVespEffet::Soin, Aylis->GetActorLocation(), FVector::UpVector, FLinearColor(0.7f, 0.5f, 1.0f));
}

void AVespPlayerController::ChoisirRune(int32 Numero)
{
	if (Phase != EVespPhase::ChoixRune || !RunesProposees.IsValidIndex(Numero))
	{
		return;
	}
	const int32 R = RunesProposees[Numero];
	AppliquerRune(R);
	MessageRoute = FString::Printf(TEXT("Rune %s : %s."), *NomRune(R), *AideRune(R));
	RetourExploration();
}

// Le boss parle avant le combat (toujours sans genre pour AYLIS)
void AVespPlayerController::DialogueDuBoss()
{
	switch (Acte)
	{
		case 1:
			Orateurs = {TEXT("Skarn"), TEXT("AYLIS"), TEXT("Skarn")};
			Repliques = {
				TEXT("Encore une petite vision qui marche vers Karn ? Approche. Le sol se souviendra de toi, meme quand ton nom sera perdu."),
				TEXT("Le sol, peut-etre. Toi, tu vas oublier."),
				TEXT("Quand je leve ma masse, la terre se brise. Compte les pas, petite vision... si tu sais compter."),
			};
			break;
		case 2:
			Orateurs = {TEXT("La Matriarche"), TEXT("AYLIS"), TEXT("La Matriarche")};
			Repliques = {
				TEXT("Mes loups ont senti ta peur bien avant ton odeur. Ils ont faim, et moi, j'ai le temps."),
				TEXT("Tes loups auront faim longtemps. Ce n'est pas pour eux que je marche."),
				TEXT("Tous viennent pour moi, a la fin. Approche, que je te couvre de mon malefice."),
			};
			break;
		case 3:
			Orateurs = {TEXT("Le Roi Noye"), TEXT("AYLIS"), TEXT("Le Roi Noye")};
			Repliques = {
				TEXT("Tout finit dans l'eau, petite vision. Les rois, les armees, les prophetes. Moi, j'ai simplement commence plus tot."),
				TEXT("Alors tu as eu le temps de t'y habituer. Moi, je ne fais que passer."),
				TEXT("Personne ne passe. Les marais gardent tout ce qu'ils touchent. Regarde : la maree monte deja."),
			};
			break;
		case 4:
			Orateurs = {TEXT("La prophetie"), TEXT("Le Gardien de Pierre"), TEXT("AYLIS")};
			Repliques = {
				TEXT("Le grand portail s'ouvre sur un geant de pierre. Des runes s'allument une a une sur son torse."),
				TEXT("INTRUS. LA MARCHE D'ASHKA EST FERMEE. RETOURNE A LA POUSSIERE."),
				TEXT("Tu as ete taille pour garder une porte. Moi, pour la traverser. Voyons qui a ete le mieux fait."),
			};
			break;
		case 5:
			Orateurs = {TEXT("Ashka"), TEXT("AYLIS"), TEXT("Ashka")};
			Repliques = {
				TEXT("Alors voila la vision qui fait trembler mes guerriers. Mes fleches ont deja vu pire."),
				TEXT("Tes guerriers ont raison de trembler. Pas a cause de moi : a cause de ce qui vient apres toi."),
				TEXT("La prophetie dit que tu tomberas sur ce col. Je suis la pour qu'elle ne mente pas."),
			};
			break;
		case 6:
			Orateurs = {TEXT("Vorgath"), TEXT("AYLIS"), TEXT("Vorgath")};
			Repliques = {
				TEXT("Tout ce qui brule finit en cendre. Les forets, les villages, les prophetes. Et toi aussi."),
				TEXT("Personne ne brulera ce soir. Sauf ta forge."),
				TEXT("Approche. Je vais te faire une place dans ma collection de cendres."),
			};
			break;
		default:
			Orateurs = {TEXT("L'Oracle"), TEXT("AYLIS"), TEXT("L'Oracle")};
			Repliques = {
				TEXT("Mille fois, j'ai vu ta route finir ici, AYLIS. Dans chaque vision, tu tombes au coeur du Voile."),
				TEXT("Alors regarde bien celle-ci. Elle est differente."),
				TEXT("Il n'y a pas de visions differentes. Il n'y a que moi... et la fin de la route."),
			};
			break;
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
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
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
		Aylis->Poison = 0;			// la potion guerit aussi du poison
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

void AVespPlayerController::Ralenti(float Echelle, float Duree)
{
	UGameplayStatics::SetGlobalTimeDilation(this, Echelle);
	FinDuRalenti = FPlatformTime::Seconds() + Duree;
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
			Cible->AfficherMessage(TEXT("rate"), FColor(200, 200, 220));
			Ecrire(TEXT("Attaque lourde... ratee !"));
			ActionChoisie = EVespAction::Attaque;
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
		AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.6f, 0.5f, 1.0f));
	}
	const int32 ArmureAvant = Cible->Stats.Armure;
	bool bCritique = false;
	const int32 Degats = Aylis->Frapper(Cible, Puissance, &bCritique);
	Trembler(bCritique || Action == EVespAction::Speciale ? 1.0f : (Action == EVespAction::Lourde ? 0.6f : 0.3f));
	if (bCritique || Action == EVespAction::Speciale)
	{
		Zoom = 1.0f;
		Ralenti(0.25f, 0.14f);
	}
	if (!Cible->EstDebout())
	{
		Ralenti(0.15f, 0.22f);			// le coup fatal : le temps s'arrete un instant
		Zoom = FMath::Max(Zoom, 0.6f);
	}
	Ecrire(FString::Printf(TEXT("AYLIS frappe %s : -%d pv%s"), *Cible->Stats.Nom, Degats, Cible->EstDebout() ? TEXT("") : TEXT(" ... il tombe !")));
	if (ArmureAvant > 0 && Cible->Stats.Armure < ArmureAvant)
	{
		Ecrire(Cible->Stats.Armure == 0 ? Cible->Stats.Nom + TEXT(" : ARMURE BRISEE ! Il est sonne.") : TEXT("L'armure se fissure."));
	}
	else if (ArmureAvant > 0 && Cible->EstDebout())
	{
		Ecrire(TEXT("Le coup glisse sur l'armure... il faut un coup lourd."));
	}
	if (bFlamme && Cible->EstDebout() && FMath::RandRange(1, 3) == 1)
	{
		InfligerEffet(Cible, VespEffetCoup::Brulure);
	}
	if (bGivre && Cible->EstDebout() && FMath::RandRange(1, 4) == 1)
	{
		InfligerEffet(Cible, VespEffetCoup::Gel);
	}
	if (bSangsue)
	{
		Aylis->Soigner(2);
	}
	if (bSeve && !Cible->EstDebout())
	{
		Aylis->Soigner(5);
	}
	ActionChoisie = EVespAction::Attaque;
	Explosions();
	FinDuTourDAylis();
}

void AVespPlayerController::InfligerEffet(AVespUnite* Cible, int32 Effet)
{
	if (!Cible || !Cible->EstDebout())
	{
		return;
	}
	const FString Nom = Cible->EstAylis() ? FString(TEXT("AYLIS")) : Cible->Stats.Nom;
	const FVector Pied = Cible->GetActorLocation();
	switch (Effet)
	{
		case VespEffetCoup::Poison:
			Cible->Poison = FMath::Max(Cible->Poison, 3);
			Ecrire(TEXT("Poison ! ") + Nom + TEXT(" perdra des pv a chaque tour."));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, Pied, FVector::UpVector, FLinearColor(0.6f, 1.0f, 0.3f));
			break;
		case VespEffetCoup::Gel:
			Cible->Gel = 1;
			Cible->AfficherMessage(TEXT("GEL"), FColor(150, 210, 255));
			Ecrire(TEXT("Le givre fige ") + Nom + TEXT(" sur place."));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Pied + FVector(0, 0, 60), FVector::UpVector, FLinearColor(0.5f, 0.8f, 1.0f));
			break;
		case VespEffetCoup::Brulure:
			Cible->Brulure = FMath::Max(Cible->Brulure, 2);
			Cible->AfficherMessage(TEXT("brule"), FColor(255, 150, 60));
			Ecrire(Nom + TEXT(" prend feu !"));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, Pied, FVector::UpVector, FLinearColor(1.0f, 0.5f, 0.15f));
			break;
		default: break;
	}
}

// Des degats qui ne viennent pas d'un coup : le terrain, une explosion, une attaque annoncee
void AVespPlayerController::Blesser(AVespUnite* Cible, int32 Degats, const FString& Cause)
{
	if (!Cible || !Cible->EstDebout() || Degats <= 0)
	{
		return;
	}
	Cible->Encaisser(Degats, false);
	const bool bAylis = Cible->EstAylis();
	Ecrire(FString::Printf(TEXT("%s : %s perd %d pv"), *Cause, bAylis ? TEXT("AYLIS") : *Cible->Stats.Nom, Degats));
	if (bAylis)
	{
		Rage = FMath::Min(100, Rage + Degats * (bFureur ? 8 : 4));
		Trembler(0.5f);
		VerifierDefaite();
	}
	else if (!Cible->EstDebout())
	{
		Ecrire(Cible->Stats.Nom + TEXT(" tombe !"));
	}
}

void AVespPlayerController::VerifierDefaite()
{
	if (!Aylis->EstDebout() && Phase != EVespPhase::Defaite)
	{
		Phase = EVespPhase::Defaite;
		TempsPhase = 0.0f;
		Ralenti(0.2f, 0.6f);
		Grille->AfficherCasesAtteignables({});
		Grille->AfficherDanger({});
	}
}

// Les Haschen explosifs qui viennent de tomber explosent (et peuvent en faire exploser d'autres)
void AVespPlayerController::Explosions()
{
	bool bEncore = true;
	while (bEncore)
	{
		bEncore = false;
		for (int32 i = 0; i < Haschen.Num(); i++)
		{
			AVespUnite* H = Haschen[i];
			if (H->EstDebout() || H->bAExplose || !(H->Stats.Capacites & VespCapacite::Explosif))
			{
				continue;
			}
			H->bAExplose = true;
			bEncore = true;
			const FLinearColor Teinte = H->Stats.Effet == VespEffetCoup::Gel ? FLinearColor(0.5f, 0.8f, 1.0f) : FLinearColor(1.0f, 0.45f, 0.1f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, H->GetActorLocation() + FVector(0, 0, 60), FVector::UpVector, Teinte);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, H->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, Teinte);
			Trembler(0.8f);
			Ecrire(H->Stats.Nom + TEXT(" explose !"));
			TArray<AVespUnite*> Voisins;
			if (Aylis->EstDebout())
			{
				Voisins.Add(Aylis);
			}
			for (AVespUnite* Autre : Haschen)
			{
				Voisins.Add(Autre);
			}
			for (AVespUnite* V : Voisins)
			{
				const FIntPoint E = V->GetCase() - H->GetCase();
				if (V != H && V->EstDebout() && FMath::Abs(E.X) <= 1 && FMath::Abs(E.Y) <= 1)
				{
					Blesser(V, 8 + Acte, TEXT("L'explosion"));
					InfligerEffet(V, H->Stats.Effet);
				}
			}
		}
	}
}

// AYLIS encaisse un coup : la rage se remplit, les effets, les talents de l'attaquant, les epines
void AVespPlayerController::ToucherAylis(AVespUnite* Attaquant, int32 Degats, bool bCritique)
{
	Rage = FMath::Min(100, Rage + Degats * (bFureur ? 8 : 4));
	Trembler(bCritique ? 1.0f : 0.4f);
	if (bCritique)
	{
		Ralenti(0.3f, 0.12f);
	}
	Ecrire(FString::Printf(TEXT("%s touche AYLIS : -%d pv%s"), *Attaquant->Stats.Nom, Degats, bCritique ? TEXT(" (critique)") : TEXT("")));
	if (!Aylis->EstDebout())
	{
		VerifierDefaite();
		return;
	}
	if (Attaquant->Stats.Effet != 0 && FMath::RandBool())
	{
		InfligerEffet(Aylis, Attaquant->Stats.Effet);
	}
	const FIntPoint Ecart = Attaquant->GetCase() - Aylis->GetCase();
	const int32 Distance = FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y);
	// Le talent "attire" : le tir tire AYLIS d'une case vers le tireur
	if ((Attaquant->Stats.Capacites & VespCapacite::Attire) && Distance >= 2)
	{
		const FIntPoint Pas = FMath::Abs(Ecart.X) >= FMath::Abs(Ecart.Y) ? FIntPoint(FMath::Sign(Ecart.X), 0) : FIntPoint(0, FMath::Sign(Ecart.Y));
		const FIntPoint Vers = Aylis->GetCase() + Pas;
		if (!Grille->EstBloquee(Vers) && !Grille->UniteSur(Vers))
		{
			Aylis->Suivre({Vers});
			Ecrire(Attaquant->Stats.Nom + TEXT(" attire AYLIS vers lui !"));
		}
	}
	// La rune d'Epines : l'attaquant se blesse (s'il est au contact)
	if (bEpines && Attaquant->EstDebout() && Distance == 1)
	{
		Blesser(Attaquant, 3, TEXT("Les epines"));
		Explosions();
	}
}

// ===================== Le tour d'AYLIS =====================

int32 AVespPlayerController::DeplacementCeTour() const
{
	return FMath::Max(1, DeplacementParTour - (bBlizzard ? 1 : 0));
}

void AVespPlayerController::TourDAylis()
{
	Tour++;
	Phase = EVespPhase::TourAylis;
	bADejaBouge = false;
	bEnGarde = false;
	Aylis->ReductionDegats = 1.0f;
	ReglesDuTour();
	Explosions();
	if (Phase == EVespPhase::Defaite || VerifierFinDeVague())
	{
		return;
	}
	// Le poison et la brulure font effet au debut du tour
	if (Aylis->SubirEtats() > 0)
	{
		Ecrire(Aylis->Brulure > 0 ? TEXT("Les flammes rongent AYLIS.") : TEXT("Le poison ronge AYLIS."));
		VerifierDefaite();
		if (Phase == EVespPhase::Defaite)
		{
			return;
		}
	}
	if (Aylis->Gel > 0)
	{
		Aylis->Gel--;
		bADejaBouge = true;
		Ecrire(TEXT("Le givre retient AYLIS : pas de deplacement ce tour."));
	}
	MontrerCasesAtteignables();
}

// La regle de l'acte, au debut de chaque tour
void AVespPlayerController::ReglesDuTour()
{
	switch (Acte)
	{
		case 3:		// les eaux montent
			if (Tour > 1 && Tour % 4 == 0)
			{
				TArray<FIntPoint> Libres = Grille->CasesLibres(2);
				for (int32 i = 0; i < 2 && Libres.Num() > 0; i++)
				{
					const FIntPoint P = TirerPlace(Libres);
					Grille->ChangerTerrain(P, 'x');
					AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, Grille->CentreDeCase(P), FVector::UpVector, FLinearColor(0.4f, 1.0f, 0.3f));
				}
				Ecrire(TEXT("Les eaux toxiques montent !"));
			}
			break;
		case 4:		// les dalles piegees : levees un tour sur deux
		{
			const bool bLeves = Tour % 2 == 0;
			Grille->LeverPieges(bLeves);
			if (bLeves)
			{
				Ecrire(TEXT("Les dalles piegees se levent !"));
				TArray<AVespUnite*> Tous = {Aylis.Get()};
				for (AVespUnite* H : Haschen)
				{
					Tous.Add(H);
				}
				for (AVespUnite* U : Tous)
				{
					if (U->EstDebout() && Grille->TerrainSur(U->GetCase()) == '^' && !(U->EstAylis() && bPiedSur))
					{
						AVespEffet::Jouer(GetWorld(), EVespEffet::Impact, U->GetActorLocation() + FVector(0, 0, 20), FVector::UpVector, FLinearColor(1.0f, 0.3f, 0.2f));
						Blesser(U, 8, TEXT("Les pointes"));
					}
				}
			}
			break;
		}
		case 5:		// le blizzard, un tour sur trois
			bBlizzard = Tour % 3 == 0;
			if (bBlizzard)
			{
				Ecrire(TEXT("Le blizzard se leve : la marche est plus lente ce tour-ci."));
				for (int32 i = 0; i < 8; i++)
				{
					const FIntPoint P(FMath::RandRange(0, AVespGrille::Colonnes - 1), FMath::RandRange(0, AVespGrille::Lignes - 1));
					AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, Grille->CentreDeCase(P), FVector::UpVector, FLinearColor(0.8f, 0.9f, 1.0f));
				}
			}
			break;
		case 6:		// les eruptions : les cases marquees au tour d'avant explosent, puis d'autres se fissurent
		{
			for (const FIntPoint& P : Eruptions)
			{
				AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, Grille->CentreDeCase(P) + FVector(0, 0, 30), FVector::UpVector, FLinearColor(1.0f, 0.4f, 0.05f));
				AVespUnite* U = Grille->UniteSur(P);
				if (U && !(U->EstAylis() && bPiedSur))
				{
					Blesser(U, 10 + Etage / 2, TEXT("L'eruption"));
					InfligerEffet(U, VespEffetCoup::Brulure);
				}
			}
			if (Eruptions.Num() > 0)
			{
				Trembler(0.8f);
			}
			Eruptions.Reset();
			TArray<FIntPoint> Libres = Grille->CasesLibres(0);
			for (int32 i = 0; i < 3 && Libres.Num() > 0; i++)
			{
				Eruptions.Add(TirerPlace(Libres));
			}
			Ecrire(TEXT("Le sol se fissure : les cases rouges vont entrer en eruption."));
			break;
		}
		case 7:		// le Voile se dechire : un echo surgit
			if (Tour > 1 && Tour % 4 == 0 && HaschenDebout() < HaschenMax)
			{
				if (AppelerRenforts(Aylis, HASCHEN[6][0], 1) > 0)
				{
					Ecrire(TEXT("Le Voile se dechire : un echo surgit pres d'AYLIS !"));
				}
			}
			break;
		default: break;
	}
	MontrerDangers();
}

void AVespPlayerController::MontrerDangers()
{
	TArray<FIntPoint> Toutes = ZonesDanger;
	Toutes.Append(Eruptions);
	Grille->AfficherDanger(Toutes);
}

// La fin du tour sur un terrain dangereux
void AVespPlayerController::EffetDuTerrain(AVespUnite* U)
{
	if (!U || !U->EstDebout() || (U->EstAylis() && bPiedSur))
	{
		return;
	}
	const TCHAR T = Grille->TerrainSur(U->GetCase());
	if (T == 'x')
	{
		if (Acte == 6)
		{
			Blesser(U, 7, TEXT("La lave"));
			InfligerEffet(U, VespEffetCoup::Brulure);
		}
		else if (Acte == 7)
		{
			Blesser(U, 7, TEXT("La dechirure du Voile"));
		}
		else
		{
			Blesser(U, 4, TEXT("Les eaux toxiques"));
			U->Poison = FMath::Max(U->Poison, 2);
		}
	}
	else if (T == '^' && Grille->PiegesLeves())
	{
		Blesser(U, 8, TEXT("Les pointes"));
	}
}

void AVespPlayerController::MontrerCasesAtteignables()
{
	const bool bVisible = Phase == EVespPhase::TourAylis && !bADejaBouge;
	Grille->AfficherCasesAtteignables(bVisible ? Grille->CasesAtteignables(Aylis->GetCase(), DeplacementCeTour()) : TArray<int32>());
}

bool AVespPlayerController::ResteDesHaschen() const
{
	return HaschenDebout() > 0;
}

void AVespPlayerController::FinDuTourDAylis()
{
	Grille->AfficherCasesAtteignables({});
	Aylis->ReductionDegats = bEnGarde ? (bRempart ? 0.33f : 0.5f) : 1.0f;
	EffetDuTerrain(Aylis);
	Explosions();
	if (Phase == EVespPhase::Defaite || VerifierFinDeVague())
	{
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

// ===================== La camera =====================
// Sur l'ecran titre, elle tourne lentement autour de la clairiere. En combat : fixe, mais elle tremble
// apres un coup et se resserre un instant sur un critique.

void AVespPlayerController::PlacerCamera(float Secondes)
{
	if (!CameraArene)
	{
		return;
	}
	// Sur l'ecran titre : elle tourne lentement autour d'AYLIS, au depart de la route
	if (Phase == EVespPhase::Titre)
	{
		const FVector Centre = Aylis->GetActorLocation();
		const float Angle = FMath::Sin(TempsPhase * 0.1f) * 0.5f;
		const FVector Ecart = DecalageCamera.RotateAngleAxis(FMath::RadiansToDegrees(Angle), FVector::UpVector) * 0.7f;
		const FVector Position = Centre + FVector(Ecart.X, Ecart.Y, Ecart.Z * 0.5f);
		CameraArene->SetActorLocation(Position);
		CameraArene->SetActorRotation((Centre + FVector(0, 0, 180) - Position).Rotation());
		bCaleCamera = true;
		return;
	}
	// En combat (tant que l'arene est la) : elle cadre l'arene ; sinon, elle suit AYLIS, toujours sous le meme angle
	const bool bArene = Haschen.Num() > 0 && Phase != EVespPhase::Exploration;
	const FVector Cible = bArene ? Grille->GetActorLocation() : Aylis->GetActorLocation() + FVector(0, 0, 60);
	const FVector Voulue = Cible + DecalageCamera * (bArene ? 1.0f : 0.78f);
	CameraActuelle = bCaleCamera ? Voulue : FMath::VInterpTo(CameraActuelle, Voulue, Secondes, bArene ? 3.0f : 5.0f);
	bCaleCamera = false;
	Secousse = FMath::Max(0.0f, Secousse - Secondes * 3.0f);
	Zoom = FMath::Max(0.0f, Zoom - Secondes * 3.0f);
	CameraArene->SetActorLocation(CameraActuelle + FMath::VRand() * Secousse * 10.0f);
	CameraArene->SetActorRotation(RotationCamera);
	CameraArene->GetCameraComponent()->SetFieldOfView((bArene ? 42.0f : 46.0f) - Zoom * 4.0f);
	Monde->OrienterTextes(CameraActuelle);
}

// ===================== A chaque image : le clavier, et les clics sur l'arene =====================
// (les clics sur les boutons sont geres par l'interface elle-meme)

void AVespPlayerController::PlayerTick(float Secondes)
{
	Super::PlayerTick(Secondes);
	if (!Grille || !Aylis || !Monde)
	{
		return;
	}
	// Le ralenti se compte en vraies secondes (le temps du jeu, lui, est ralenti)
	if (FinDuRalenti > 0.0 && FPlatformTime::Seconds() >= FinDuRalenti)
	{
		FinDuRalenti = 0.0;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	}
	TempsPhase += Secondes;
	TempsMessage = FMath::Max(0.0f, TempsMessage - Secondes);
	if (bModePhoto)
	{
		ModePhoto(Secondes);
		if (!bCameraPhoto)
		{
			PlacerCamera(Secondes);
		}
		return;
	}
	// La rage pleine : une aura magique enveloppe AYLIS
	const bool bRagePleine = Rage >= 100 && (Phase == EVespPhase::TourAylis || Phase == EVespPhase::TourHaschen);
	if (bRagePleine && !AuraRage)
	{
		if (UNiagaraSystem* Aura = AVespEffet::Magie(TEXT("NS_Free_Magic_Aura")))
		{
			AuraRage = UNiagaraFunctionLibrary::SpawnSystemAttached(Aura, Aylis->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			                                                        EAttachLocation::KeepRelativeOffset, false);
		}
	}
	else if (!bRagePleine && AuraRage)
	{
		AuraRage->DestroyComponent();
		AuraRage = nullptr;
	}
	// F4 : les contours "toon" (le filtre du pack StylizedProvencal), en marche ou pas
	if (WasInputKeyJustPressed(EKeys::F4))
	{
		bContours = !bContours;
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			for (FWeightedBlendable& B : It->Settings.WeightedBlendables.Array)
			{
				B.Weight = bContours ? 1.0f : 0.0f;
			}
		}
	}
	Repetition = FMath::Max(0.0f, Repetition - Secondes);
	PlacerCamera(Secondes);
	// La souris a bouge : on vise de nouveau a la souris (et plus au curseur)
	float SourisX = 0.0f, SourisY = 0.0f;
	GetMousePosition(SourisX, SourisY);
	if (FVector2D(SourisX, SourisY) != DerniereSouris)
	{
		if (!DerniereSouris.IsZero())
		{
			bCurseur = false;
			bSelectionVisible = false;
		}
		DerniereSouris = FVector2D(SourisX, SourisY);
	}
	const bool bValider = WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);

	switch (Phase)
	{
		case EVespPhase::Titre:
			if (bValider || WasInputKeyJustPressed(EKeys::SpaceBar))
			{
				NouvellePartie(1);
			}
			return;
		case EVespPhase::Victoire:
		case EVespPhase::Defaite:
			if (WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right) || (bValider && TempsPhase > 1.5f))
			{
				Recommencer();
			}
			return;
		case EVespPhase::Dialogue:
			Ecriture += Secondes * 45.0f;
			if (bValider || WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::LeftMouseButton))
			{
				AvancerDialogue();
			}
			return;
		case EVespPhase::NouvelActe:
			if (bValider || WasInputKeyJustPressed(EKeys::SpaceBar))
			{
				ContinuerApresLActe();
			}
			return;
		case EVespPhase::ChoixRune:
		case EVespPhase::Marchand:
		case EVespPhase::Evenement:
			CommandesDeMenu(Secondes);
			return;
		case EVespPhase::Exploration:
			Explorer(Secondes);
			return;
		case EVespPhase::TourHaschen:
			Grille->AfficherSurvol(FIntPoint(-1, -1));
			JouerTourHaschen(Secondes);
			return;
		case EVespPhase::TourAylis:
			CommandesDeCombat(Secondes);
			return;
		default:
			return;
	}
}

// Une direction pressee : les fleches (et ZQSD / WASD), la croix, ou le stick gauche (qui se repete si on le tient)
bool AVespPlayerController::DirectionPressee(FIntPoint& Direction, float Secondes)
{
	Direction = FIntPoint(0, 0);
	auto Presse = [this](std::initializer_list<FKey> Touches) {
		for (const FKey& K : Touches)
		{
			if (WasInputKeyJustPressed(K))
			{
				return true;
			}
		}
		return false;
	};
	if (Presse({EKeys::Up, EKeys::Gamepad_DPad_Up})) Direction.Y = -1;
	else if (Presse({EKeys::Down, EKeys::Gamepad_DPad_Down})) Direction.Y = 1;
	else if (Presse({EKeys::Left, EKeys::Gamepad_DPad_Left})) Direction.X = -1;
	else if (Presse({EKeys::Right, EKeys::Gamepad_DPad_Right})) Direction.X = 1;
	if (Direction != FIntPoint(0, 0))
	{
		return true;
	}
	const float SX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float SY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Max(FMath::Abs(SX), FMath::Abs(SY)) < 0.55f)
	{
		Repetition = 0.0f;
		return false;
	}
	if (Repetition > 0.0f)
	{
		return false;
	}
	Repetition = 0.2f;
	if (FMath::Abs(SX) > FMath::Abs(SY))
	{
		Direction.X = SX > 0 ? 1 : -1;
	}
	else
	{
		Direction.Y = SY > 0 ? -1 : 1;		// le stick vers le haut = vers le haut de l'ecran
	}
	return true;
}

// ===================== Le mode photo (pour la promo) =====================
// Lance le jeu avec -VespPhotos : il attend que tout soit pret (shaders, modeles), puis, acte par acte, il prend
// 4 photos (l'exploration, un panorama, un combat au moment d'un coup, le boss) dans Saved/Photos, et quitte.

static TArray<FIntPoint> Carre(const AVespGrille* G, FIntPoint Centre);

void AVespPlayerController::Photographier(const FString& Nom, bool bAvecInterface)
{
	FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Photos") / (Nom + TEXT(".png")), bAvecInterface, false);
}

void AVespPlayerController::ModePhoto(float Secondes)
{
	PhotoAttente -= Secondes;
	if (PhotoAttente > 0.0f)
	{
		return;
	}
	// On attend que les shaders et les modeles soient prets (la premiere fois, ca peut etre long)
#if WITH_EDITOR
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		PhotoAttente = 1.0f;
		return;
	}
#endif
	if (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0)
	{
		PhotoAttente = 1.0f;
		return;
	}
	const FString Prefixe = FString::Printf(TEXT("Acte%d_%s"), PhotoActe, *Romain(FMath::Max(1, PhotoActe)));
	auto PremiereZone = [this](std::initializer_list<EVespSalle> Types) {
		for (int32 i = 0; i < Noeuds.Num(); i++)
		{
			for (EVespSalle T : Types)
			{
				if (Noeuds[i].Type == T)
				{
					return i;
				}
			}
		}
		return 1;
	};
	switch (PhotoEtape)
	{
		case 0:		// l'ecran titre
			if (PhotoActe == 0)
			{
				Photographier(TEXT("00_Ecran_titre"), true);
				PhotoActe = 1;
				PhotoAttente = 1.5f;
				return;
			}
			// Un acte : son monde, son ambiance
			Acte = PhotoActe;
			Aylis->Stats.Pv = Aylis->Stats.PvMax;
			AmbianceDeLActe();
			Phase = EVespPhase::Exploration;
			bCameraPhoto = false;
			PhotoEtape = 1;
			PhotoAttente = 8.0f;
			return;
		case 1:		// AYLIS sur le sentier, entre la 1re et la 2e clairiere
		{
			const FVector P = FMath::Lerp(Noeuds[1].Centre, Noeuds[2].Centre, 0.45f);
			Aylis->SetActorLocation(Monde->Contraindre(P, P));
			Aylis->SetActorRotation(FRotator(0, 80.0f, 0));
			bCaleCamera = true;
			PhotoEtape = 2;
			PhotoAttente = 3.0f;
			return;
		}
		case 2:
			Photographier(Prefixe + TEXT("_1_Exploration"), false);
			PhotoEtape = 3;
			PhotoAttente = 1.0f;
			return;
		case 3:		// un panorama : plus bas, plus loin, le long du sentier vers l'est
		{
			const FVector Cible = FMath::Lerp(Noeuds[2].Centre, Noeuds[3].Centre, 0.35f) + FVector(0, 0, 180.0f);
			const FVector Position = Cible + FVector(-2600.0f, -2100.0f, 1100.0f);
			bCameraPhoto = true;
			CameraArene->SetActorLocation(Position);
			CameraArene->SetActorRotation((Cible - Position).Rotation());
			CameraArene->GetCameraComponent()->SetFieldOfView(55.0f);
			Aylis->SetActorLocation(Monde->Contraindre(Cible, FVector(Cible.X, Cible.Y, Aylis->GetActorLocation().Z)));
			PhotoEtape = 4;
			PhotoAttente = 3.0f;
			return;
		}
		case 4:
			Photographier(Prefixe + TEXT("_2_Panorama"), false);
			PhotoEtape = 5;
			PhotoAttente = 1.0f;
			return;
		case 5:		// un combat, dans la premiere clairiere gardee
		{
			bCameraPhoto = false;
			const int32 Zone = PremiereZone({EVespSalle::Elite, EVespSalle::Combat});
			Aylis->SetActorLocation(Noeuds[Zone].Centre + FVector(0, -700.0f, 0));
			Declencher(Zone);
			Rage = 100;
			PhotoEtape = 6;
			PhotoAttente = 3.5f;
			return;
		}
		case 6:		// AYLIS frappe (le coup special) : la photo au moment de l'impact
		{
			AVespUnite* Cible = nullptr;
			for (AVespUnite* H : Haschen)
			{
				if (H->EstDebout() && (!Cible || H->Stats.PvMax > Cible->Stats.PvMax))
				{
					Cible = H;
				}
			}
			FIntPoint Case;
			if (Cible && CaseLibrePres(Cible->GetCase(), Case, 1))
			{
				Aylis->Replacer(Case);
				Aylis->Frapper(Cible, 220);
				AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.6f, 0.5f, 1.0f));
				MontrerCasesAtteignables();
			}
			PhotoEtape = 7;
			PhotoAttente = 0.25f;
			return;
		}
		case 7:
			Photographier(Prefixe + TEXT("_3_Combat"), true);
			PhotoEtape = 8;
			PhotoAttente = 1.5f;
			return;
		case 8:		// le boss, dans sa clairiere
		{
			RetourExploration();
			const int32 Zone = PremiereZone({EVespSalle::Boss});
			Aylis->SetActorLocation(Noeuds[Zone].Centre + FVector(0, -1000.0f, 0));
			ZoneActuelle = Zone;
			Etage = Noeuds[Zone].Etage;
			PreparerCombat(EVespSalle::Boss);
			PhotoEtape = 9;
			PhotoAttente = 3.5f;
			return;
		}
		case 9:		// son attaque annoncee : les cases rouges autour d'AYLIS
			Annoncer(Carre(Grille, Aylis->GetCase()), 10, 0, '.');
			PhotoEtape = 10;
			PhotoAttente = 0.6f;
			return;
		case 10:
			Photographier(Prefixe + TEXT("_4_Boss"), true);
			PhotoEtape = 11;
			PhotoAttente = 1.5f;
			return;
		default:	// l'acte suivant, ou la fin
			RetourExploration();
			PhotoActe++;
			PhotoEtape = 0;
			PhotoAttente = 1.0f;
			if (PhotoActe > NombreDActes)
			{
				bModePhoto = false;
				Quitter();
			}
			return;
	}
}

// ===================== L'exploration =====================

void AVespPlayerController::Explorer(float Secondes)
{
	// La carte du monde
	if (WasInputKeyJustPressed(EKeys::Tab) || WasInputKeyJustPressed(EKeys::M) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Left))
	{
		bCarteOuverte = !bCarteOuverte;
	}
	// La direction : ZQSD (AZERTY), WASD (QWERTY), les fleches, ou le stick gauche
	FVector2D Entree(0.0f, 0.0f);
	auto Tenue = [this](std::initializer_list<FKey> Touches) {
		for (const FKey& K : Touches)
		{
			if (IsInputKeyDown(K))
			{
				return true;
			}
		}
		return false;
	};
	if (Tenue({EKeys::Z, EKeys::W, EKeys::Up, EKeys::Gamepad_DPad_Up})) Entree.Y += 1.0f;
	if (Tenue({EKeys::S, EKeys::Down, EKeys::Gamepad_DPad_Down})) Entree.Y -= 1.0f;
	if (Tenue({EKeys::Q, EKeys::A, EKeys::Left, EKeys::Gamepad_DPad_Left})) Entree.X -= 1.0f;
	if (Tenue({EKeys::D, EKeys::Right, EKeys::Gamepad_DPad_Right})) Entree.X += 1.0f;
	const FVector2D Stick(GetInputAnalogKeyState(EKeys::Gamepad_LeftX), GetInputAnalogKeyState(EKeys::Gamepad_LeftY));
	if (Stick.Size() > 0.2f)
	{
		Entree += Stick;
	}
	const float Force = FMath::Min(1.0f, Entree.Size());
	if (Force > 0.05f)
	{
		Entree /= Entree.Size();
	}
	const bool bCourir = Tenue({EKeys::LeftShift, EKeys::RightShift}) || GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) > 0.3f;
	const float Vitesse = 430.0f * (bCourir ? 1.75f : 1.0f) * Force;
	// Le haut de l'ecran, et sa droite, sur le sol
	const float Yaw = FMath::DegreesToRadians(RotationCamera.Yaw);
	const FVector Avant(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f);
	const FVector Droite(-FMath::Sin(Yaw), FMath::Cos(Yaw), 0.0f);
	const FVector Ici = Aylis->GetActorLocation();
	const FVector Vers = Monde->Contraindre(Ici, Ici + (Avant * Entree.Y + Droite * Entree.X) * Vitesse * Secondes);
	Aylis->DeplacerLibrement(Vers - Ici, (bCourir ? 1.6f : 1.0f) * FMath::Max(Force, 0.4f), Secondes);

	// Une clairiere pas encore faite : on y entre
	for (int32 i = 0; i < Noeuds.Num(); i++)
	{
		const FVespNoeud& N = Noeuds[i];
		if (N.bVisite)
		{
			continue;
		}
		const bool bGardee = N.Type == EVespSalle::Combat || N.Type == EVespSalle::Elite || N.Type == EVespSalle::Boss;
		if (FVector::Dist2D(Vers, N.Centre) < (bGardee ? 760.0f : 480.0f))
		{
			bCarteOuverte = false;
			Declencher(i);
			return;
		}
	}
	// Un marchand deja rencontre : on peut lui reparler
	MarchandProche = -1;
	for (int32 i = 0; i < Noeuds.Num(); i++)
	{
		if (Noeuds[i].Type == EVespSalle::Marchand && FVector::Dist2D(Vers, Noeuds[i].Centre) < 700.0f)
		{
			MarchandProche = i;
		}
	}
	if (MarchandProche >= 0 && (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)))
	{
		RouvrirMarchand(MarchandProche);
	}
}

// ===================== Le combat : la souris, ou le curseur (clavier, manette) =====================

bool AVespPlayerController::CaseVisee(FIntPoint& Case) const
{
	if (bCurseur)
	{
		Case = Curseur;
		return Grille->EstDansArene(Case);
	}
	return CaseSousLaSouris(Case);
}

void AVespPlayerController::CommandesDeCombat(float Secondes)
{
	// Le curseur : les fleches, la croix ou le stick
	FIntPoint Direction;
	if (DirectionPressee(Direction, Secondes))
	{
		if (!bCurseur)
		{
			bCurseur = true;
			Curseur = Aylis->GetCase();
		}
		// (l'arene : les colonnes vont vers la droite de l'ecran, les lignes vers le bas)
		Curseur.X = FMath::Clamp(Curseur.X + Direction.X, 0, AVespGrille::Colonnes - 1);
		Curseur.Y = FMath::Clamp(Curseur.Y + Direction.Y, 0, AVespGrille::Lignes - 1);
	}
	FIntPoint Case(-1, -1);
	const bool bSurLaGrille = CaseVisee(Case);
	Grille->AfficherSurvol(bSurLaGrille ? Case : FIntPoint(-1, -1));
	if (Aylis->EstOccupe())
	{
		return;
	}
	// Les actions : 1 a 5 ; a la manette, les gachettes hautes changent d'action, X boit une potion, Y passe
	const FKey Touches[5] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five};
	for (int32 i = 0; i < 5; i++)
	{
		if (WasInputKeyJustPressed(Touches[i]))
		{
			ChoisirAction((EVespAction)i);
			return;
		}
	}
	const bool bSuivante = WasInputKeyJustPressed(EKeys::Gamepad_RightShoulder);
	if (bSuivante || WasInputKeyJustPressed(EKeys::Gamepad_LeftShoulder))
	{
		const EVespAction Ordre[4] = {EVespAction::Attaque, EVespAction::Lourde, EVespAction::Garde, EVespAction::Speciale};
		int32 Ici = 0;
		for (int32 i = 0; i < 4; i++)
		{
			Ici = Ordre[i] == ActionChoisie ? i : Ici;
		}
		for (int32 k = 1; k <= 4; k++)
		{
			const EVespAction A = Ordre[(Ici + (bSuivante ? k : 4 - k)) % 4];
			if (ActionDisponible(A))
			{
				ChoisirAction(A);
				break;
			}
		}
		return;
	}
	if (WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left))
	{
		ChoisirAction(EVespAction::Potion);
		return;
	}
	if (WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
	{
		PasserLeTour();
		return;
	}
	const bool bClic = WasInputKeyJustPressed(EKeys::LeftMouseButton) && !bCurseur;
	const bool bConfirme = WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);
	if (!(bClic || bConfirme) || !bSurLaGrille)
	{
		return;
	}
	// Sur un Haschen au contact : l'action choisie
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
		const TArray<FIntPoint> Chemin = Grille->Chemin(Aylis->GetCase(), Case, DeplacementCeTour());
		if (!Chemin.IsEmpty())
		{
			Aylis->Suivre(Chemin);
			bADejaBouge = true;
			MontrerCasesAtteignables();
		}
	}
}

// ===================== Les menus (runes, marchand, evenement) au clavier ou a la manette =====================

void AVespPlayerController::CommandesDeMenu(float Secondes)
{
	const int32 Nombre = Phase == EVespPhase::ChoixRune ? RunesProposees.Num() : (Phase == EVespPhase::Marchand ? Offres.Num() : 2);
	const FKey Touches[4] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four};
	auto Choisir = [this](int32 i) {
		if (Phase == EVespPhase::ChoixRune) ChoisirRune(i);
		else if (Phase == EVespPhase::Marchand) AcheterOffre(i);
		else if (i < 2) ChoisirEvenement(i);
	};
	for (int32 i = 0; i < 4; i++)
	{
		if (WasInputKeyJustPressed(Touches[i]))
		{
			Choisir(i);
			return;
		}
	}
	FIntPoint Direction;
	if (DirectionPressee(Direction, Secondes) && Direction.X != 0 && Nombre > 0)
	{
		SelectionMenu = bSelectionVisible ? (SelectionMenu + Direction.X + Nombre) % Nombre : 0;
		bSelectionVisible = true;
	}
	if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		if (bSelectionVisible)
		{
			Choisir(FMath::Clamp(SelectionMenu, 0, FMath::Max(0, Nombre - 1)));
		}
		else
		{
			SelectionMenu = 0;
			bSelectionVisible = true;
		}
		return;
	}
	if (Phase == EVespPhase::Marchand && (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)))
	{
		QuitterMarchand();
	}
}

// ===================== Les attaques annoncees (tous les boss) =====================
// Le principe : le boss marque des cases (en rouge). Elles restent rouges pendant le tour d'AYLIS,
// qui peut s'en ecarter. Au tour suivant du boss, elles explosent (et parfois le terrain change).

void AVespPlayerController::Annoncer(const TArray<FIntPoint>& Cases, int32 Degats, int32 Effet, TCHAR Terrain)
{
	ZonesDanger = Cases;
	DegatsDanger = Degats;
	EffetDanger = Effet;
	TerrainDanger = Terrain;
	MontrerDangers();
	Trembler(0.3f);
}

void AVespPlayerController::FrapperZones(AVespUnite* Source)
{
	const FLinearColor Teinte = EffetDanger == VespEffetCoup::Gel ? FLinearColor(0.5f, 0.8f, 1.0f)
	                          : (EffetDanger == VespEffetCoup::Poison ? FLinearColor(0.4f, 1.0f, 0.3f) : FLinearColor(1.0f, 0.45f, 0.15f));
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Source->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, Teinte);
	for (const FIntPoint& P : ZonesDanger)
	{
		AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, Grille->CentreDeCase(P), FVector::UpVector, Teinte);
	}
	Trembler(1.5f);
	Zoom = 1.0f;
	bool bToucheAylis = false;
	for (const FIntPoint& P : ZonesDanger)
	{
		AVespUnite* U = Grille->UniteSur(P);
		if (!U || U == Source || !U->EstDebout())
		{
			continue;
		}
		if (U->EstAylis())
		{
			bToucheAylis = true;
			Blesser(U, FMath::RoundToInt(DegatsDanger * Aylis->ReductionDegats), Source->Stats.Nom);
			InfligerEffet(U, EffetDanger);
		}
		else if (U->Stats.Boss == 0)
		{
			Blesser(U, DegatsDanger / 2, Source->Stats.Nom);		// ses propres Haschen n'y echappent pas
		}
	}
	if (!bToucheAylis)
	{
		Ecrire(TEXT("Esquive ! AYLIS n'etait plus dans la zone."));
	}
	if (TerrainDanger != '.')
	{
		for (const FIntPoint& P : ZonesDanger)
		{
			if (Grille->TerrainSur(P) != '@')
			{
				Grille->ChangerTerrain(P, TerrainDanger);
			}
		}
	}
	ZonesDanger.Reset();
	MontrerDangers();
	Explosions();
}

static TArray<FIntPoint> Losange(const AVespGrille* G, FIntPoint Centre, int32 Rayon)
{
	TArray<FIntPoint> Cases;
	for (int32 C = -Rayon; C <= Rayon; C++)
	{
		for (int32 L = -Rayon; L <= Rayon; L++)
		{
			const FIntPoint P = Centre + FIntPoint(C, L);
			if (FMath::Abs(C) + FMath::Abs(L) <= Rayon && (C != 0 || L != 0) && !G->EstBloquee(P))
			{
				Cases.Add(P);
			}
		}
	}
	return Cases;
}

static TArray<FIntPoint> Carre(const AVespGrille* G, FIntPoint Centre)
{
	TArray<FIntPoint> Cases;
	for (int32 C = -1; C <= 1; C++)
	{
		for (int32 L = -1; L <= 1; L++)
		{
			const FIntPoint P = Centre + FIntPoint(C, L);
			if (!G->EstBloquee(P))
			{
				Cases.Add(P);
			}
		}
	}
	return Cases;
}

// ===================== Les boss =====================

bool AVespPlayerController::TourDuBoss(AVespUnite* B)
{
	switch (B->Stats.Boss)
	{
		case 1: return TourDeSkarn(B);
		case 2: return TourDeLaMatriarche(B);
		case 3: return TourDuRoiNoye(B);
		case 4: return TourDuGardien(B);
		case 5: return TourDAshka(B);
		case 6: return TourDeVorgath(B);
		default: return TourDeLOracle(B);
	}
}

// Skarn : tous les 3 tours, il leve sa masse (les cases autour de lui deviennent rouges)... et au tour suivant,
// elles volent en eclats.
bool AVespPlayerController::TourDeSkarn(AVespUnite* Skarn)
{
	if (ZonesDanger.Num() > 0)
	{
		FrapperZones(Skarn);
		return true;		// il reprend son souffle
	}
	if (Skarn->Compteur % 3 == 0)
	{
		Annoncer(Losange(Grille, Skarn->GetCase(), 2), Skarn->Stats.Attaque * 3 / 2, 0, '.');
		Skarn->AfficherMessage(TEXT("!"), FColor(255, 80, 60));
		AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Skarn->GetActorLocation() + FVector(0, 0, 250), FVector::UpVector, FLinearColor(1.0f, 0.35f, 0.15f));
		Ecrire(TEXT("Skarn leve sa masse... ECARTE-TOI des cases rouges !"));
		return true;
	}
	return false;
}

// La Matriarche : elle se soigne quand elle faiblit, elle appelle ses loups tous les 3 tours,
// et sinon son malefice empoisonne de loin.
bool AVespPlayerController::TourDeLaMatriarche(AVespUnite* M)
{
	if (M->Stats.Pv < M->Stats.PvMax / 2 && PotionsDuBoss > 0 && M->Compteur % 2 == 0)
	{
		PotionsDuBoss--;
		M->Soigner(25);
		Ecrire(TEXT("La Matriarche boit une decoction : +25 pv."));
		return true;
	}
	if (M->Compteur % 3 == 0 && AppelerRenforts(M, HASCHEN[1][0], 2) > 0)
	{
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
			InfligerEffet(Aylis, VespEffetCoup::Poison);
			Ecrire(TEXT("Malefice !"));
		}
		return true;
	}
	return false;
}

// Le Roi Noye : il se regenere dans l'eau, leve la maree autour d'AYLIS (les cases deviennent toxiques),
// et appelle les noyes.
bool AVespPlayerController::TourDuRoiNoye(AVespUnite* Roi)
{
	if (ZonesDanger.Num() > 0)
	{
		FrapperZones(Roi);
		Ecrire(TEXT("La maree toxique retombe... et reste."));
		return true;
	}
	const TCHAR Sous = Grille->TerrainSur(Roi->GetCase());
	if (Roi->Stats.Pv < Roi->Stats.PvMax && (Sous == 'x' || Sous == 'o'))
	{
		Roi->Soigner(8);
		Ecrire(TEXT("Le Roi Noye se regenere dans les eaux."));
	}
	if (Roi->Compteur % 3 == 0)
	{
		Annoncer(Carre(Grille, Aylis->GetCase()), Roi->Stats.Attaque, VespEffetCoup::Poison, 'x');
		Roi->AfficherMessage(TEXT("!"), FColor(120, 255, 150));
		Ecrire(TEXT("Le Roi Noye leve la maree autour d'AYLIS... ECARTE-TOI !"));
		return true;
	}
	if (Roi->Compteur % 4 == 0 && AppelerRenforts(Roi, HASCHEN[2][0], 2) > 0)
	{
		Ecrire(TEXT("Les noyes se levent de la vase !"));
		return true;
	}
	return false;
}

// Le Gardien de Pierre : son armure (3 plaques) ne cede qu'aux coups lourds ; brise, il est sonne 2 tours,
// puis se reforme plus furieux. Tous les 3 tours, ses lignes de fracas.
bool AVespPlayerController::TourDuGardien(AVespUnite* G)
{
	if (ZonesDanger.Num() > 0)
	{
		FrapperZones(G);
		return true;
	}
	if (G->Compteur % 3 == 0)
	{
		TArray<FIntPoint> Lignes;
		const FIntPoint Directions[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (const FIntPoint& D : Directions)
		{
			for (int32 k = 1; k <= 5; k++)
			{
				const FIntPoint P = G->GetCase() + D * k;
				if (Grille->EstBloquee(P))
				{
					break;
				}
				Lignes.Add(P);
			}
		}
		Annoncer(Lignes, G->Stats.Attaque * 3 / 2, 0, '.');
		G->AfficherMessage(TEXT("!"), FColor(255, 220, 150));
		Ecrire(TEXT("Le Gardien leve les poings : ses lignes de fracas s'allument !"));
		return true;
	}
	if (G->Compteur % 5 == 0)
	{
		Grille->LeverPieges(true);
		Ecrire(TEXT("Le Gardien frappe le sol : toutes les dalles se levent !"));
		Trembler(1.0f);
		for (AVespUnite* U : TArray<AVespUnite*>{Aylis.Get()})
		{
			if (Grille->TerrainSur(U->GetCase()) == '^' && !bPiedSur)
			{
				Blesser(U, 8, TEXT("Les pointes"));
			}
		}
		return true;
	}
	return false;
}

// Ashka : sa pluie de fleches annoncee (glacee), et ses archers quand elle est blessee.
bool AVespPlayerController::TourDAshka(AVespUnite* A)
{
	if (ZonesDanger.Num() > 0)
	{
		FrapperZones(A);
		return true;
	}
	if ((A->Appels == 0 && A->Stats.Pv <= A->Stats.PvMax * 6 / 10) || (A->Appels == 1 && A->Stats.Pv <= A->Stats.PvMax * 3 / 10))
	{
		A->Appels++;
		AppelerRenforts(A, HASCHEN[4][3], 2);
		Ecrire(TEXT("Ashka siffle : ses archers sortent de l'ombre !"));
		return true;
	}
	if (A->Compteur % 3 == 0)
	{
		TArray<FIntPoint> Pluie = Carre(Grille, Aylis->GetCase());
		for (const FIntPoint& P : Losange(Grille, Aylis->GetCase(), 3))
		{
			if (!Pluie.Contains(P) && FMath::RandRange(0, 99) < 30)
			{
				Pluie.Add(P);
			}
		}
		Annoncer(Pluie, A->Stats.Attaque + 2, VespEffetCoup::Gel, '.');
		A->AfficherMessage(TEXT("!"), FColor(255, 120, 180));
		Ecrire(TEXT("Ashka bande son arc vers le ciel : une pluie de fleches va tomber !"));
		return true;
	}
	return false;
}

// Vorgath : le sol entre en eruption la ou AYLIS se tient (et devient de la lave)... et a moitie de ses pv,
// il se dechaine.
bool AVespPlayerController::TourDeVorgath(AVespUnite* V)
{
	if (!V->bPhaseDeux && V->Stats.Pv <= V->Stats.PvMax / 2)
	{
		V->bPhaseDeux = true;
		V->Stats.Attaque += 4;
		AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, V->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(1.0f, 0.4f, 0.1f));
		Trembler(1.5f);
		Ecrire(TEXT("Vorgath se dechaine ! Ses coups redoublent."));
		return true;
	}
	if (ZonesDanger.Num() > 0)
	{
		FrapperZones(V);
		return true;
	}
	if (V->Compteur % (V->bPhaseDeux ? 2 : 3) == 0)
	{
		TArray<FIntPoint> Zone = V->bPhaseDeux ? Carre(Grille, Aylis->GetCase()) : Losange(Grille, Aylis->GetCase(), 1);
		Zone.AddUnique(Aylis->GetCase());
		Annoncer(Zone, V->Stats.Attaque + 3, VespEffetCoup::Brulure, 'x');
		V->AfficherMessage(TEXT("!"), FColor(255, 140, 60));
		Ecrire(TEXT("Le sol gronde sous AYLIS : il va entrer en eruption !"));
		return true;
	}
	return false;
}

// L'Oracle de Karn : il se deplace dans le Voile et ouvre des passages aux echos... puis, a moitie de ses pv,
// la prophetie se brise : il reprend les armes de tous les gardiens (fracas, malefice, eruption).
bool AVespPlayerController::TourDeLOracle(AVespUnite* O)
{
	if (ZonesDanger.Num() > 0)
	{
		FrapperZones(O);
		return true;
	}
	if (!O->bPhaseDeux && O->Stats.Pv <= O->Stats.PvMax / 2)
	{
		O->bPhaseDeux = true;
		O->Stats.Attaque += 3;
		AppelerRenforts(O, HASCHEN[6][0], 2);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, O->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.7f, 0.4f, 1.0f));
		Trembler(1.5f);
		Ecrire(TEXT("L'Oracle hurle : la prophetie se brise ! Tout ce qu'AYLIS a affronte revient."));
		return true;
	}
	const FIntPoint Ecart = O->GetCase() - Aylis->GetCase();
	const int32 Distance = FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y);
	if (!O->bPhaseDeux)
	{
		if (O->Compteur % 3 == 0 && AppelerRenforts(O, HASCHEN[6][0], 2) > 0)
		{
			Ecrire(TEXT("L'Oracle ouvre le Voile : des echos en sortent."));
			return true;
		}
		if (O->Compteur % 2 == 0)
		{
			TArray<FIntPoint> Loin;
			for (const FIntPoint& P : Grille->CasesLibres(0))
			{
				const FIntPoint E = P - Aylis->GetCase();
				if (FMath::Abs(E.X) + FMath::Abs(E.Y) >= 5)
				{
					Loin.Add(P);
				}
			}
			if (Loin.Num() > 0)
			{
				O->Bondir(Loin[FMath::RandRange(0, Loin.Num() - 1)]);
				Ecrire(TEXT("L'Oracle disparait dans le Voile... et reapparait plus loin."));
				return true;
			}
		}
		return false;
	}
	switch (O->Compteur % 3)
	{
		case 0:
			Annoncer(Losange(Grille, O->GetCase(), 2), O->Stats.Attaque * 3 / 2, 0, '.');
			Ecrire(TEXT("L'Oracle leve une masse de lumiere, comme Skarn autrefois !"));
			return true;
		case 1:
			if (Distance >= 2 && Distance <= 6)
			{
				bool bCritique = false;
				const int32 Degats = O->Frapper(Aylis, 90, &bCritique);
				ToucherAylis(O, Degats, bCritique);
				InfligerEffet(Aylis, VespEffetCoup::Poison);
				Ecrire(TEXT("Le malefice de la Matriarche, dans la voix de l'Oracle !"));
				return true;
			}
			return false;
		default:
		{
			TArray<FIntPoint> Zone = Carre(Grille, Aylis->GetCase());
			Annoncer(Zone, O->Stats.Attaque + 3, VespEffetCoup::Brulure, 'x');
			Ecrire(TEXT("Le sol s'embrase sous AYLIS, comme dans la forge de Vorgath !"));
			return true;
		}
	}
}

// ===================== Le tour des Haschen =====================
// Un par un, avec une petite pause : les etats, les talents, puis il agit selon son style

void AVespPlayerController::Fuir(AVespUnite* H)
{
	const TArray<int32> Pas = Grille->CasesAtteignables(H->GetCase(), 2);
	FIntPoint Meilleure = H->GetCase();
	int32 MeilleureDistance = -1;
	for (int32 i = 0; i < Pas.Num(); i++)
	{
		const FIntPoint P(i % AVespGrille::Colonnes, i / AVespGrille::Colonnes);
		if (Pas[i] <= 0 || Grille->EstDangereuse(P))
		{
			continue;
		}
		const FIntPoint E = P - Aylis->GetCase();
		const int32 D = FMath::Abs(E.X) + FMath::Abs(E.Y);
		if (D > MeilleureDistance)
		{
			MeilleureDistance = D;
			Meilleure = P;
		}
	}
	if (Meilleure != H->GetCase())
	{
		H->Suivre(Grille->Chemin(H->GetCase(), Meilleure, 2));
	}
}

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
	auto Suivant = [this, H]() {
		EffetDuTerrain(H);
		Explosions();
		HaschenQuiJoue++;
		EtapeHaschen = 0;
		Minuteur = 0.0f;
	};
	const FIntPoint Ecart = H->GetCase() - Aylis->GetCase();
	const int32 Distance = FMath::Abs(Ecart.X) + FMath::Abs(Ecart.Y);
	const bool bLanceur = H->Stats.Style == EVespStyle::Lanceur;

	if (EtapeHaschen == 0)
	{
		// Un renfort ne joue pas le tour de son arrivee
		if (H->bVientDArriver)
		{
			H->bVientDArriver = false;
			HaschenQuiJoue++;
			Minuteur = PauseEntreHaschen;
			return;
		}
		H->Compteur++;
		// Le poison et la brulure font effet
		if (H->SubirEtats() > 0 && !H->EstDebout())
		{
			Ecrire(H->Stats.Nom + TEXT(" succombe a ses blessures !"));
			Explosions();
			if (VerifierFinDeVague())
			{
				return;
			}
			HaschenQuiJoue++;
			Minuteur = 0.0f;
			return;
		}
		// Une armure brisee : sonne, puis elle se reforme
		if (H->TempsBrise > 0)
		{
			H->TempsBrise--;
			if (H->TempsBrise > 0)
			{
				H->AfficherMessage(TEXT("sonne"), FColor(255, 230, 150));
				Ecrire(H->Stats.Nom + TEXT(" est sonne."));
				Suivant();
				return;
			}
			H->Stats.Armure = H->Stats.ArmureMax;
			H->AfficherMessage(TEXT("armure reformee"), FColor(220, 220, 235));
			Ecrire(H->Stats.Nom + TEXT(" reforme son armure."));
			if (H->Stats.Boss == 4)
			{
				H->Stats.Attaque += 1;
				Ecrire(TEXT("Le Gardien se reforme, plus furieux."));
			}
		}
		const bool bFige = H->Gel > 0;
		if (bFige)
		{
			H->Gel--;
			H->AfficherMessage(TEXT("fige"), FColor(150, 210, 255));
		}
		if (H->Stats.Boss > 0 && TourDuBoss(H))
		{
			Suivant();
			return;
		}
		if (Phase == EVespPhase::Defaite)
		{
			return;
		}
		const int32 Talents = H->Stats.Capacites;
		// Un soigneur soigne l'allie le plus blesse (un tour sur deux)
		if ((Talents & VespCapacite::Soigneur) && H->Compteur % 2 == 0)
		{
			AVespUnite* Blesse = nullptr;
			for (AVespUnite* Autre : Haschen)
			{
				const FIntPoint E = Autre->GetCase() - H->GetCase();
				if (Autre != H && Autre->EstDebout() && Autre->Stats.Pv < Autre->Stats.PvMax * 6 / 10 && FMath::Abs(E.X) + FMath::Abs(E.Y) <= 5
				    && (!Blesse || Autre->Stats.Pv * Blesse->Stats.PvMax < Blesse->Stats.Pv * Autre->Stats.PvMax))
				{
					Blesse = Autre;
				}
			}
			if (Blesse)
			{
				Blesse->Soigner(10 + Acte * 2);
				Ecrire(H->Stats.Nom + TEXT(" soigne ") + Blesse->Stats.Nom + TEXT("."));
				Suivant();
				return;
			}
		}
		// Un invocateur appelle des renforts (tous les 3 tours)
		if ((Talents & VespCapacite::Invocateur) && H->Compteur % 3 == 0 && AppelerRenforts(H, HASCHEN[Acte - 1][0], H->Stats.Boss > 0 ? 2 : 1) > 0)
		{
			Ecrire(H->Stats.Nom + TEXT(" appelle des renforts !"));
			Suivant();
			return;
		}
		// Un sauteur bondit a cote d'AYLIS (un tour sur deux)
		if ((Talents & VespCapacite::Sauteur) && !bFige && H->Compteur % 2 == 0 && Distance >= 3)
		{
			FIntPoint Arrivee;
			if (CaseLibrePres(Aylis->GetCase(), Arrivee, 1))
			{
				H->Bondir(Arrivee);
				Ecrire(H->Stats.Nom + TEXT(" bondit sur AYLIS !"));
				EtapeHaschen = 1;
				Minuteur = PauseEntreHaschen * 0.5f;
				return;
			}
		}
		// Un tireur recule s'il est au contact, et tire de loin une fois sur deux
		if (bLanceur)
		{
			if (Distance == 1 && !bFige && FMath::RandRange(0, 2) > 0)
			{
				Fuir(H);
				EtapeHaschen = 1;
				return;
			}
			if (Distance >= 2 && Distance <= H->Stats.Portee && (bFige || FMath::RandBool()))
			{
				bool bCritique = false;
				const int32 Degats = H->Frapper(Aylis, 80, &bCritique);
				ToucherAylis(H, Degats, bCritique);
				Suivant();
				return;
			}
		}
		if (bFige)
		{
			EtapeHaschen = 1;		// fige : il ne bouge pas, mais frappe s'il est au contact
			return;
		}
		// Sinon il avance
		int32 Pas = H->Stats.Pas > 0 ? H->Stats.Pas : (H->Stats.Style == EVespStyle::Chargeur ? 4 : 2);
		if (bBlizzard)
		{
			Pas = FMath::Max(1, Pas - 1);
		}
		H->Suivre(Grille->ApprocheVers(H->GetCase(), Aylis->GetCase(), Pas));
		EtapeHaschen = 1;
		return;
	}
	// Arrive : il frappe au contact, ou tire s'il est a portee
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
	else if (bLanceur && Distance >= 2 && Distance <= H->Stats.Portee)
	{
		bool bCritique = false;
		const int32 Degats = H->Frapper(Aylis, 80, &bCritique);
		ToucherAylis(H, Degats, bCritique);
		if (Phase == EVespPhase::Defaite)
		{
			return;
		}
	}
	Suivant();
}
