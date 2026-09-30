// VespMenu : le menu d'AYLIS (touche I / Start), d'apres les maquettes "epurees et dynamiques".
//
//   L'INVENTAIRE : AYLIS au centre d'un anneau qui tourne lentement, ses six emplacements tout autour
//   (tete, amulette, arme, anneau, main gauche, corps) ; a droite la sacoche (des tuiles avec une barre de la
//   couleur de la rarete) et la fiche de l'objet choisi, qui glisse a chaque nouvel objet, avec la comparaison.
//   LE SEUIL : la constellation des trois voies (la Lame, le Rempart, la Prophetie). Les etoiles allumees
//   brillent, celles qu'on peut allumer pulsent, les liens entre elles coulent comme de la lumiere.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateTypes.h"

class AVespPlayerController;

// Une icone dessinee au trait (les memes que les maquettes) : epee, dague, bouclier, anneau, capuche...
class SVespIcone : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVespIcone) : _Icone(0), _Couleur(FLinearColor::White), _Epaisseur(2.0f) {}
		SLATE_ATTRIBUTE(int32, Icone)
		SLATE_ATTRIBUTE(FLinearColor, Couleur)
		SLATE_ARGUMENT(float, Epaisseur)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(40, 40); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
	                      int32 Couche, const FWidgetStyle& Style, bool bParentActif) const override;

private:
	TAttribute<int32> Icone;
	TAttribute<FLinearColor> Couleur;
	float Epaisseur = 2.0f;
};

// Le Seuil : la constellation (dessinee a la main, cliquable)
class SVespConstellation : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVespConstellation) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AVespPlayerController>, Joueur)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1440, 900); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
	                      int32 Couche, const FWidgetStyle& Style, bool bParentActif) const override;
	virtual FReply OnMouseMove(const FGeometry& Geo, const FPointerEvent& Souris) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Souris) override;

	static FVector2D PositionEtoile(int32 Index);		// dans le repere 1440 x 900

private:
	int32 EtoileSous(const FGeometry& Geo, const FVector2D& Ecran) const;
	TWeakObjectPtr<AVespPlayerController> Joueur;
	FSlateBrush Rond;
	FSlateBrush Anneau;
	TArray<FVector4f> Ciel;			// x, y, taille, phase
};

class SVespMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVespMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AVespPlayerController>, Joueur)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<SWidget> Inventaire();
	TSharedRef<SWidget> LeSeuil();
	TSharedRef<SWidget> Onglet(const FText& Nom, int32 Numero);
	TSharedRef<SWidget> Emplacement(int32 Numero);
	TSharedRef<SWidget> Tuile(int32 Index);
	TSharedRef<SWidget> Fiche();
	TSharedRef<SWidget> Bouton(const FText& Texte, bool bPrincipal, TFunction<void()> Action, TAttribute<bool> Actif);
	const struct FVespObjet* ObjetChoisi() const;

	TWeakObjectPtr<AVespPlayerController> Joueur;
	int32 EmplacementChoisi = -1;	// -1 : on regarde la sacoche ; sinon un emplacement porte
	FSlateBrush Rond, Blanc, Tuiles, TuileChoisie, Cadre, Rien, Voile;
	FButtonStyle StyleBouton;
};
