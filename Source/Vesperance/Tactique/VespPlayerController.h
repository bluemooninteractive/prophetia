// VespPlayerController : le deroulement d'un combat, tour par tour (comme regles.cpp dans le prototype).
//
//   Tour d'AYLIS : clic sur une case bleue pour se deplacer (une fois), puis clic sur un Haschen au contact
//                  pour le frapper (ce qui termine le tour). ESPACE : passer.
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

UCLASS()
class VESPERANCE_API AVespPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVespPlayerController();
	virtual void PlayerTick(float Secondes) override;

	void Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, const TArray<AVespUnite*>& LesHaschen);

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
	void Annoncer(const FString& Texte, FColor Couleur, float Duree = 3.0f);

	UPROPERTY() TObjectPtr<AVespGrille> Grille;
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Haschen;

	EVespPhase Phase = EVespPhase::TourAylis;
	int32 Tour = 0;
	bool bADejaBouge = false;
	int32 HaschenQuiJoue = 0;		// pendant le tour des Haschen : lequel joue
	int32 EtapeHaschen = 0;			// 0 = il se deplace, 1 = il frappe (une fois arrive)
	float Minuteur = 0.0f;
};
