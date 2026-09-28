// VespOptions : le panneau des options (le son, le plein ecran, les secousses), le meme sur l'ecran titre
// et dans le menu d'AYLIS (onglet OPTIONS, qui ajoute "abandonner la vision" et "quitter le jeu").
// Chaque ligne se choisit a la souris, au clavier ou a la manette ; c'est le PlayerController qui change les reglages.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class AVespPlayerController;

class SVespOptions : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVespOptions) : _bEnJeu(false) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AVespPlayerController>, Joueur)
		SLATE_ARGUMENT(bool, bEnJeu)		// dans le menu d'AYLIS : les lignes "abandonner" et "quitter" en plus
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<SWidget> Ligne(int32 Numero);
	TSharedRef<SWidget> PetitBouton(const FText& Libelle, int32 Numero, int32 Sens);

	TWeakObjectPtr<AVespPlayerController> Joueur;
	FSlateBrush Panneau, Fond, FondChoisi, Rond, Rien;
	FButtonStyle StyleBouton;
};
