// VespInterface : toute l'interface du jeu, construite en Slate (le systeme d'interface d'Unreal), en C++.
//
//   Pendant le combat : en haut a gauche l'acte, le lieu et le journal ; en haut au centre "TON TOUR" ;
//   en bas a gauche la fiche d'AYLIS (jauges de pv et de rage, potions, runes, etats) ; en bas au centre
//   la barre des 5 actions (de vrais boutons, avec une bulle d'aide au survol).
//   Par-dessus, selon le moment : les choix de la route (runes, salles), le dialogue (un parchemin),
//   le titre d'un nouvel acte, et la fin (victoire ou vision brisee).
//
// Slate se "branche" sur le jeu avec des fonctions lambda : a chaque image, un texte ou une couleur
// va lire l'etat du combat dans le PlayerController. Rien a mettre a jour a la main.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"

class AVespPlayerController;

class SVespInterface : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVespInterface) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AVespPlayerController>, Joueur)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TWeakObjectPtr<AVespPlayerController> Joueur;

	// Les morceaux de l'interface
	TSharedRef<SWidget> CoucheCombat();
	TSharedRef<SWidget> CarteDuLieu();
	TSharedRef<SWidget> PastilleDuTour();
	TSharedRef<SWidget> FicheAylis();
	TSharedRef<SWidget> BarreDActions();
	TSharedRef<SWidget> BoutonAction(int32 Numero);
	TSharedRef<SWidget> CoucheChoix();
	TSharedRef<SWidget> CarteDeChoix(int32 Numero);
	TSharedRef<SWidget> CoucheDialogue();
	TSharedRef<SWidget> CoucheNouvelActe();
	TSharedRef<SWidget> CoucheFin();
	TSharedRef<SWidget> Jauge(TAttribute<float> Part, TAttribute<FSlateColor> Couleur, TAttribute<FText> Texte, float Largeur, float Hauteur);
	TSharedRef<SWidget> GrandBouton(const FText& Texte, FLinearColor Couleur, TFunction<void()> Action);

	// Les petites questions sur l'etat du jeu
	bool EnCombat() const;
	EVisibility VisibleSi(bool bCondition) const { return bCondition ? EVisibility::Visible : EVisibility::Collapsed; }

	// Les "pinceaux" : des rectangles aux coins arrondis, avec ou sans bordure (ils doivent vivre aussi longtemps que l'interface)
	FSlateBrush Panneau;			// les cadres sombres et translucides
	FSlateBrush PanneauFonce;
	FSlateBrush Blanc;			// un rectangle blanc, recolore a la demande (jauges, pastilles)
	FSlateBrush Rond;				// un disque blanc (le portrait, les numeros)
	FSlateBrush Carte;			// une carte d'action (normale)
	FSlateBrush CarteSurvol;
	FSlateBrush CarteChoisie;
	FSlateBrush GrandeCarte;		// les cartes des choix de la route
	FSlateBrush GrandeCarteSurvol;
	FSlateBrush Parchemin;
	FSlateBrush CadreDore;
	FSlateBrush Voile;			// le fond sombre derriere les ecrans de choix
	FSlateBrush Rien;				// transparent
	FButtonStyle StyleBouton;				// des boutons sans decor (leur contenu fait le dessin)

	TArray<TSharedPtr<class SButton>> BoutonsActions;
	TArray<TSharedPtr<class SButton>> BoutonsChoix;
};
