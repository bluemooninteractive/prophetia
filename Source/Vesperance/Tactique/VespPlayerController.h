// VespPlayerController : le deroulement de la route, acte apres acte, salle apres salle
// (comme regles.cpp, route.cpp et boss.cpp dans le prototype).
//
//   Chaque acte : salle 1 = un combat, salles 2 et 3 = au choix (combat, elite, feu de camp), salle 4 = le boss.
//     Acte I  : la Foret des Brumes, puis Skarn le Brise-Cranes (coups de masse annonces).
//     Acte II : le Bois des Pendus, puis la Matriarche (malefice empoisonne, loups, decoctions).
//   Apres chaque victoire : une rune a choisir parmi 3.
//   L'interface (VespInterface) appelle les fonctions publiques : choisir une action, une rune, une salle...
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
	NouvelActe,		// le titre d'un nouvel acte
	Victoire,		// le dernier boss est tombe
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
	friend class AVespHUD;
	friend class SVespInterface;		// l'interface lit l'etat du combat

public:
	AVespPlayerController();
	virtual void PlayerTick(float Secondes) override;
	virtual void EndPlay(const EEndPlayReason::Type Raison) override;

	void Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, ACameraActor* LaCamera);

	// Ce que l'interface peut demander
	void ChoisirAction(EVespAction Action);
	void ChoisirRune(int32 Numero);
	void ChoisirSalle(int32 Numero);
	void AvancerDialogue();
	void ContinuerApresLActe();
	void PasserLeTour();
	void Recommencer();

	static FString NomAction(EVespAction Action);
	static FString DetailAction(EVespAction Action, int32 Potions);
	static FString AideAction(EVespAction Action);
	bool ActionDisponible(EVespAction Action) const;
	static FString NomSalle(EVespSalle Salle, int32 Acte);
	static FString AideSalle(EVespSalle Salle);
	static FString NomRune(int32 Rune);
	static FString AideRune(int32 Rune);
	FString NomDuLieu() const;
	FString NomDeLActe() const;

	UPROPERTY(EditAnywhere, Category = "Vesperance") int32 DeplacementParTour = 3;	// comme dans le prototype
	UPROPERTY(EditAnywhere, Category = "Vesperance") float PauseEntreHaschen = 0.5f;
	static constexpr int32 NombreDeSalles = 4;		// par acte : 3 salles, puis le boss
	static constexpr int32 NombreDActes = 2;

private:
	// La route
	void PreparerCombat(EVespSalle Type);
	void ApresVictoire();
	void ProposerRunes();
	void SalleSuivante();
	void ProposerSalles();
	void DialogueDuBoss();
	void AmbianceDeLActe();
	AVespUnite* CreerHaschen(const FString& Nom, const FString& Dossier, FIntPoint Case, int32 Pv, int32 Attaque, int32 Defense,
	                         FLinearColor Teinte, EVespStyle Style, bool bPoison, float Taille = 170.0f);
	void HaschenAuHasard(TArray<FIntPoint>& Places);
	bool CaseLibrePres(FIntPoint Centre, FIntPoint& Trouvee) const;

	// Le combat
	void TourDAylis();
	void FinDuTourDAylis();
	void JouerTourHaschen(float Secondes);
	bool TourDeSkarn(AVespUnite* Skarn);			// true : il a utilise son tour (annonce ou fracas)
	bool TourDeLaMatriarche(AVespUnite* Matriarche);
	void MontrerCasesAtteignables();
	bool CaseSousLaSouris(FIntPoint& Case) const;
	bool ResteDesHaschen() const;
	void Ecrire(const FString& Message);			// le journal (les 3 derniers messages)
	void AgirSur(AVespUnite* Cible);
	void ToucherAylis(AVespUnite* Attaquant, int32 Degats, bool bCritique);
	void Trembler(float Force) { Secousse = FMath::Min(1.5f, Secousse + Force); }

	UPROPERTY() TObjectPtr<AVespGrille> Grille;
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Haschen;
	UPROPERTY() TObjectPtr<ACameraActor> CameraArene;
	TSharedPtr<class SVespInterface> Interface;
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
	int32 PotionsDuBoss = 2;		// les decoctions de la Matriarche

	// La route et les runes
	int32 Acte = 1;
	int32 Salle = 1;
	EVespSalle TypeSalle = EVespSalle::Combat;
	TArray<EVespSalle> Propositions;
	TArray<int32> RunesProposees;
	TArray<int32> Runes;
	bool bFlamme = false;			// les coups d'AYLIS peuvent bruler
	bool bSeve = false;				// +5 pv a chaque Haschen abattu
	bool bFureur = false;			// la rage monte 2 fois plus vite
	FString MessageRoute;			// ce qui vient de se passer (affiche sur les ecrans de choix)
	float TempsPhase = 0.0f;		// depuis combien de temps l'ecran en cours est affiche (pour les fondus)

	// Le dialogue
	TArray<FString> Orateurs;
	TArray<FString> Repliques;
	int32 LigneDialogue = 0;
	float Ecriture = 0.0f;			// le nombre de lettres deja ecrites
};
