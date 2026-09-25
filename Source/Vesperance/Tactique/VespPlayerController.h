// VespPlayerController : le deroulement de l'acte I, salle apres salle (comme regles.cpp et route.cpp du prototype).
//
//   La route : salle 1 = un combat, salles 2 et 3 = au choix (combat, elite, feu de camp), salle 4 = Skarn.
//   Apres chaque victoire : une rune a choisir parmi 3.
//   Tour d'AYLIS : clic sur une case bleue pour se deplacer (une fois), puis une action (touches 1 a 5, ou clic
//                  sur une carte) et un clic sur un Haschen au contact. ESPACE : passer.
//   Tour des Haschen : le poison et la brulure font effet, puis chacun agit selon son style
//                      (au contact, a distance, en chargeant). Skarn annonce ses coups de masse (cases rouges).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VespUnite.h"
#include "VespPlayerController.generated.h"

class AVespGrille;
class ACameraActor;

UENUM()
enum class EVespPhase : uint8
{
	TourAylis,
	TourHaschen,
	Dialogue,		// quelqu'un parle (le parchemin)
	ChoixRune,		// apres une victoire : une rune parmi 3
	ChoixSalle,		// la vision : ou aller ensuite
	Victoire,		// Skarn est tombe : l'acte I est termine
	Defaite,		// AYLIS tombe : la vision se brise
};

// Les actions d'AYLIS (les cartes du bas de l'ecran), comme dans le prototype
UENUM()
enum class EVespAction : uint8
{
	Attaque,
	Lourde,		// degats x1.8, mais rate 4 fois sur 10
	Garde,		// petits degats, et les coups recus font 2 fois moins mal jusqu'au prochain tour
	Potion,		// +15 pv
	Speciale,	// quand la rage est pleine : degats x2.2
};

UENUM()
enum class EVespSalle : uint8
{
	Combat,
	Elite,
	Repos,
	Boss,
};

UCLASS()
class VESPERANCE_API AVespPlayerController : public APlayerController
{
	GENERATED_BODY()
	friend class AVespHUD;		// l'interface lit l'etat du combat

public:
	AVespPlayerController();
	virtual void PlayerTick(float Secondes) override;

	void Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, ACameraActor* LaCamera);

	static FString NomAction(EVespAction Action);
	static FString DetailAction(EVespAction Action, int32 Potions);
	static FString AideAction(EVespAction Action);
	bool ActionDisponible(EVespAction Action) const;
	static FString NomSalle(EVespSalle Salle);
	static FString AideSalle(EVespSalle Salle);
	static FString NomRune(int32 Rune);
	static FString AideRune(int32 Rune);

	UPROPERTY(EditAnywhere, Category = "Vesperance") int32 DeplacementParTour = 3;	// comme dans le prototype
	UPROPERTY(EditAnywhere, Category = "Vesperance") float PauseEntreHaschen = 0.5f;
	static constexpr int32 NombreDeSalles = 4;		// l'acte I : 3 salles, puis Skarn

private:
	// La route
	void PreparerCombat(EVespSalle Type);
	void ApresVictoire();
	void ProposerRunes();
	void ChoisirRune(int32 Numero);
	void SalleSuivante();
	void ProposerSalles();
	void ChoisirSalle(int32 Numero);
	void DialogueDeSkarn();
	AVespUnite* CreerHaschen(const FString& Nom, const FString& Dossier, FIntPoint Case, int32 Pv, int32 Attaque, int32 Defense,
	                         FLinearColor Teinte, EVespStyle Style, bool bPoison, float Taille = 170.0f);

	// Le combat
	void TourDAylis();
	void FinDuTourDAylis();
	void JouerTourHaschen(float Secondes);
	bool TourDeSkarn(AVespUnite* Skarn);		// true : il a utilise son tour (annonce ou fracas)
	void MontrerCasesAtteignables();
	bool CaseSousLaSouris(FIntPoint& Case) const;
	bool ResteDesHaschen() const;
	void Ecrire(const FString& Message);			// le journal (les 3 derniers messages)
	void ChoisirAction(EVespAction Action);
	void AgirSur(AVespUnite* Cible);
	void ToucherAylis(AVespUnite* Attaquant, int32 Degats, bool bCritique);
	void Trembler(float Force) { Secousse = FMath::Min(1.5f, Secousse + Force); }
	int32 CarteCliquee(int32 Nombre) const;			// la carte de choix (runes, salles) sous la souris, ou -1

	UPROPERTY() TObjectPtr<AVespGrille> Grille;
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Haschen;
	UPROPERTY() TObjectPtr<ACameraActor> CameraArene;
	FVector PositionCamera;
	float Secousse = 0.0f;

	// L'etat du combat
	EVespPhase Phase = EVespPhase::TourAylis;
	EVespAction ActionChoisie = EVespAction::Attaque;
	TArray<FString> Journal;
	int32 Tour = 0;
	int32 Potions = 3;
	int32 Rage = 0;					// de 0 a 100 : elle monte quand AYLIS encaisse des coups
	bool bEnGarde = false;
	bool bADejaBouge = false;
	int32 HaschenQuiJoue = 0;		// pendant le tour des Haschen : lequel joue
	int32 EtapeHaschen = 0;			// 0 = il se prepare, 1 = il frappe (une fois arrive)
	float Minuteur = 0.0f;
	TArray<FIntPoint> ZonesDanger;	// les cases annoncees par Skarn
	int32 DegatsDanger = 0;

	// La route et les runes
	int32 Salle = 1;
	EVespSalle TypeSalle = EVespSalle::Combat;
	TArray<EVespSalle> Propositions;
	TArray<int32> RunesProposees;
	TArray<int32> Runes;
	bool bFlamme = false;			// les coups d'AYLIS peuvent bruler
	bool bSeve = false;				// +5 pv a chaque Haschen abattu
	bool bFureur = false;			// la rage monte 2 fois plus vite
	FString MessageRoute;			// ce qui vient de se passer (affiche sur les ecrans de choix)

	// Le dialogue
	TArray<FString> Orateurs;
	TArray<FString> Repliques;
	int32 LigneDialogue = 0;
	float Ecriture = 0.0f;			// le nombre de lettres deja ecrites
};
