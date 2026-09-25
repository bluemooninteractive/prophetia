// VespHUD : l'interface du combat, dessinee a chaque image (comme dessinerPanneau dans le prototype 2D).
//   En haut : l'acte, le lieu, "TON TOUR" / "TOUR DES HASCHEN", le tour, et le journal des dernieres actions.
//   En bas : les jauges d'AYLIS (pv, rage), ses potions, et les 5 cartes d'actions.
//   Au-dessus de chaque personnage : sa barre de vie.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VespHUD.generated.h"

UCLASS()
class VESPERANCE_API AVespHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// Le rectangle d'une carte d'action (numero 0 a 4) : utilise aussi par le PlayerController pour les clics
	static FBox2D RectangleCarte(int32 Numero, FVector2D Ecran);
	static constexpr int32 NombreDeCartes = 5;
	// Les grandes cartes des choix de la route (runes, salles), centrees sur l'ecran
	static FBox2D RectangleChoix(int32 Numero, int32 Nombre, FVector2D Ecran);

private:
	void Cadre(FBox2D R, FLinearColor Fond, FLinearColor Bord, float Epaisseur);
	void Jauge(FBox2D R, float Part, FLinearColor Couleur, const FString& Texte);
	void Texte(const FString& T, float X, float Y, FLinearColor Couleur, float Echelle, bool bCentre = false);
	void BarresDeVie();
	void Parchemin(FBox2D R, FLinearColor Lueur);
	void EcranDeChoix(class AVespPlayerController* Joueur, FVector2D Ecran);
	void EcranDeDialogue(class AVespPlayerController* Joueur, FVector2D Ecran);
	float Taille = 1.0f;		// l'interface grandit avec l'ecran
};
