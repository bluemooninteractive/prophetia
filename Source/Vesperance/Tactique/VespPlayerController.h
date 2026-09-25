// VespPlayerController : le deroulement d'un combat, tour par tour (comme regles.cpp dans le prototype).
//
//   Tour d'AYLIS : clic sur une case bleue pour se deplacer (une fois), puis une action (touches 1 a 5, ou clic
//                  sur une carte) et un clic sur un Haschen au contact. ESPACE : passer.
//   Tour des Haschen : chacun a son tour, il s'approche d'AYLIS (2 cases) et frappe s'il est au contact.
//   Fin : tous les Haschen tombent (victoire), ou AYLIS tombe (la vision se brise). R pour recommencer.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VespPlayerController.generated.h"

class AVespGrille;
class AVespUnite;

UENUM()
enum class EVespPhase : uint8
{
	TourAylis,
	TourHaschen,
	Victoire,
	Defaite,
};

// Les actions d'AYLIS (les cartes du bas de l'ecran), comme dans le prototype
UENUM()
enum class EVespAction : uint8
{
	Attaque,
	Lourde,		// degats x1.8, mais rate 4 fois sur 10
	Garde,		// petits degats, et AYLIS encaisse 2 fois moins jusqu'a son prochain tour
	Potion,		// +15 pv
	Speciale,	// quand la rage est pleine : degats x2.2
};

UCLASS()
class VESPERANCE_API AVespPlayerController : public APlayerController
{
	GENERATED_BODY()
	friend class AVespHUD;		// l'interface lit l'etat du combat

public:
	AVespPlayerController();
	virtual void PlayerTick(float Secondes) override;

	void Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, const TArray<AVespUnite*>& LesHaschen);

	static FString NomAction(EVespAction Action);
	static FString DetailAction(EVespAction Action, int32 Potions);
	static FString AideAction(EVespAction Action);
	bool ActionDisponible(EVespAction Action) const;

	UPROPERTY(EditAnywhere, Category = "Vesperance") int32 DeplacementParTour = 3;	// comme dans le prototype
	UPROPERTY(EditAnywhere, Category = "Vesperance") int32 DeplacementHaschen = 2;
	UPROPERTY(EditAnywhere, Category = "Vesperance") float PauseEntreHaschen = 0.6f;

private:
	void TourDAylis();
	void FinDuTourDAylis();
	void JouerTourHaschen(float Secondes);
	void MontrerCasesAtteignables();
	bool CaseSousLaSouris(FIntPoint& Case) const;
	bool ResteDesHaschen() const;
	void Ecrire(const FString& Message);			// le journal (les 3 derniers messages)
	void ChoisirAction(EVespAction Action);
	void AgirSur(AVespUnite* Cible);

	UPROPERTY() TObjectPtr<AVespGrille> Grille;
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Haschen;

	EVespPhase Phase = EVespPhase::TourAylis;
	EVespAction ActionChoisie = EVespAction::Attaque;
	TArray<FString> Journal;
	int32 Tour = 0;
	int32 Potions = 3;
	int32 Rage = 0;					// de 0 a 100 : elle monte quand AYLIS encaisse des coups
	bool bEnGarde = false;
	bool bADejaBouge = false;
	int32 HaschenQuiJoue = 0;		// pendant le tour des Haschen : lequel joue
	int32 EtapeHaschen = 0;			// 0 = il se deplace, 1 = il frappe (une fois arrive)
	float Minuteur = 0.0f;
};
