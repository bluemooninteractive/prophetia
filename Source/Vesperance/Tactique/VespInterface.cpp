#include "VespInterface.h"
#include "VespPlayerController.h"
#include "VespUnite.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"

#define LOCTEXT_NAMESPACE "Vesperance"

// ===================== La palette (la meme que les planches et le press kit) =====================
namespace
{
	const FLinearColor OR(0.91f, 0.75f, 0.33f, 1.0f);
	const FLinearColor OR_PALE(1.0f, 0.89f, 0.6f, 1.0f);
	const FLinearColor VIOLET(0.73f, 0.55f, 1.0f, 1.0f);
	const FLinearColor BRUME(0.56f, 0.89f, 0.85f, 1.0f);
	const FLinearColor TEXTE(0.94f, 0.91f, 0.98f, 1.0f);
	const FLinearColor DOUX(0.66f, 0.63f, 0.75f, 1.0f);
	const FLinearColor ROUGE(0.9f, 0.32f, 0.3f, 1.0f);
	const FLinearColor VERT(0.4f, 0.82f, 0.5f, 1.0f);
	const FLinearColor BLEU_AYLIS(0.45f, 0.62f, 1.0f, 1.0f);
	const FLinearColor GIVRE(0.6f, 0.82f, 1.0f, 1.0f);

	// Une police : Roboto (fournie avec Unreal), avec un style ("Regular", "Bold", "Italic", "Light"),
	// une taille, et un espacement des lettres (en milliemes de la taille : 200 = des lettres bien espacees)
	FSlateFontInfo Police(const char* Style, int32 Taille, int32 Espacement = 0)
	{
		FSlateFontInfo P = FCoreStyle::GetDefaultFontStyle(Style, Taille);
		P.LetterSpacing = Espacement;
		return P;
	}

	FLinearColor CouleurVie(float Part)
	{
		if (Part <= 0.25f) return ROUGE;
		if (Part <= 0.5f) return FLinearColor(0.92f, 0.76f, 0.3f);
		return VERT;
	}

	FText Texte(const FString& S) { return FText::FromString(S); }

	float Pulsation(float Vitesse = 3.0f) { return 0.5f + 0.5f * FMath::Sin(FPlatformTime::Seconds() * Vitesse); }
}

// ===================== La carte de la route =====================
// Un widget dessine a la main : les salles (des disques de couleur, avec leur lettre), les chemins entre elles
// (en pointilles orange pour les raccourcis), les salles ou l'on peut aller (qui scintillent), et un clic pour choisir.

class SVespCarteRoute : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVespCarteRoute) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AVespPlayerController>, Joueur)
	SLATE_END_ARGS()

	int32 Survol = -1;		// la salle sous la souris

	void Construct(const FArguments& Args)
	{
		Joueur = Args._Joueur;
		Rond = FSlateRoundedBoxBrush(FLinearColor::White, 200.0f);
		Anneau = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 200.0f, FLinearColor::White, 3.0f);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1400.0f, 470.0f); }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
	                      int32 Couche, const FWidgetStyle& Style, bool bParentActif) const override
	{
		const AVespPlayerController* J = Joueur.Get();
		if (!J)
		{
			return Couche;
		}
		const FVector2D Taille = Geo.GetLocalSize();
		const float Pulse = Pulsation(4.0f);
		const TSharedRef<FSlateFontMeasure> Mesure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		// Les numeros des etages, en bas
		for (int32 E = 0; E < AVespPlayerController::NombreDEtages; E++)
		{
			const FString N = FString::FromInt(E + 1);
			const FSlateFontInfo F = Police("Regular", 9);
			const FVector2D T(Mesure->Measure(N, F));
			const float X = PositionX(E, Taille) - T.X / 2.0f;
			FSlateDrawElement::MakeText(Elements, Couche, Geo.ToPaintGeometry(FVector2f(T), FSlateLayoutTransform(FVector2f(X, Taille.Y - 16.0f))),
			                            N, F, ESlateDrawEffect::None, FLinearColor(0.5f, 0.47f, 0.6f, 0.6f));
		}
		// Les chemins
		for (int32 i = 0; i < J->Noeuds.Num(); i++)
		{
			const FVespNoeud& N = J->Noeuds[i];
			for (int32 S : N.Suivants)
			{
				const FVespNoeud& M = J->Noeuds[S];
				const bool bRaccourci = M.Etage - N.Etage > 1;
				const bool bOuvert = i == J->NoeudActuel && J->NoeudsPossibles.Contains(S);
				const bool bPris = N.bVisite && M.bVisite;
				FLinearColor C = bPris ? FLinearColor(0.95f, 0.75f, 0.35f, 0.95f)
				               : (bOuvert ? FLinearColor(1.0f, 1.0f, 1.0f, 0.5f + 0.45f * Pulse) : FLinearColor(0.45f, 0.42f, 0.6f, 0.35f));
				if (bRaccourci && !bPris)
				{
					C = FLinearColor(1.0f, 0.62f, 0.3f, bOuvert ? 0.55f + 0.45f * Pulse : 0.5f);
				}
				const FVector2D A = Position(N, Taille), B = Position(M, Taille);
				const float Epaisseur = bPris || bOuvert ? 3.0f : 2.0f;
				if (!bRaccourci)
				{
					FSlateDrawElement::MakeLines(Elements, Couche, Geo.ToPaintGeometry(), TArray<FVector2f>{FVector2f(A), FVector2f(B)},
					                             ESlateDrawEffect::None, C, true, Epaisseur);
					continue;
				}
				// Un raccourci : en pointilles
				const float Longueur = FVector2D::Distance(A, B);
				const FVector2D Sens = (B - A) / FMath::Max(1.0f, Longueur);
				for (float D = 0.0f; D < Longueur; D += 16.0f)
				{
					const FVector2D P1 = A + Sens * D;
					const FVector2D P2 = A + Sens * FMath::Min(Longueur, D + 9.0f);
					FSlateDrawElement::MakeLines(Elements, Couche, Geo.ToPaintGeometry(), TArray<FVector2f>{FVector2f(P1), FVector2f(P2)},
					                             ESlateDrawEffect::None, C, true, Epaisseur);
				}
			}
		}
		// Les salles
		for (int32 i = 0; i < J->Noeuds.Num(); i++)
		{
			const FVespNoeud& N = J->Noeuds[i];
			const FVector2D P = Position(N, Taille);
			const bool bPossible = J->NoeudsPossibles.Contains(i);
			const bool bIci = i == J->NoeudActuel;
			const FLinearColor Couleur = AVespPlayerController::CouleurSalle(N.Type);
			float R = Rayon(N);
			if (bPossible)
			{
				R *= i == Survol ? 1.3f : 1.0f + 0.1f * Pulse;
			}
			if (bPossible || bIci)
			{
				Disque(Elements, Couche + 1, Geo, P, R * 1.8f, Rond, FLinearColor(Couleur.R, Couleur.G, Couleur.B, bIci ? 0.25f : 0.14f + 0.14f * Pulse));
			}
			const FLinearColor Fond = N.bVisite ? Couleur * FLinearColor(0.45f, 0.45f, 0.45f, 1.0f)
			                        : (bPossible ? FLinearColor(0.1f, 0.07f, 0.16f, 1.0f) : FLinearColor(0.04f, 0.035f, 0.07f, 0.9f));
			Disque(Elements, Couche + 2, Geo, P, R, Rond, Fond);
			const FLinearColor Bord = (bPossible || N.bVisite) ? Couleur : FLinearColor(Couleur.R * 0.6f, Couleur.G * 0.6f, Couleur.B * 0.6f, 0.7f);
			Disque(Elements, Couche + 3, Geo, P, R, Anneau, Bord);
			const FString Lettre = AVespPlayerController::LettreSalle(N.Type);
			const FSlateFontInfo F = Police("Bold", N.Type == EVespSalle::Boss ? 22 : 13);
			const FVector2D T(Mesure->Measure(Lettre, F));
			FSlateDrawElement::MakeText(Elements, Couche + 4, Geo.ToPaintGeometry(FVector2f(T), FSlateLayoutTransform(FVector2f(P - T / 2.0f))),
			                            Lettre, F, ESlateDrawEffect::None, bPossible || N.bVisite ? FLinearColor::White : Bord);
			if (bIci)
			{
				// AYLIS est ici : un petit medaillon bleu au-dessus
				Disque(Elements, Couche + 4, Geo, P - FVector2D(0, R + 13.0f), 7.0f, Rond, BLEU_AYLIS);
			}
		}
		return Couche + 5;
	}

	virtual FReply OnMouseMove(const FGeometry& Geo, const FPointerEvent& Evenement) override
	{
		Survol = SalleSous(Geo, Evenement);
		return FReply::Unhandled();
	}

	virtual void OnMouseLeave(const FPointerEvent& Evenement) override
	{
		SLeafWidget::OnMouseLeave(Evenement);
		Survol = -1;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Evenement) override
	{
		AVespPlayerController* J = Joueur.Get();
		const int32 N = SalleSous(Geo, Evenement);
		if (J && N >= 0 && J->NoeudsPossibles.Contains(N))
		{
			J->ChoisirNoeud(N);
			Survol = -1;
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual FCursorReply OnCursorQuery(const FGeometry& Geo, const FPointerEvent& Evenement) const override
	{
		const AVespPlayerController* J = Joueur.Get();
		return FCursorReply::Cursor(J && Survol >= 0 && J->NoeudsPossibles.Contains(Survol) ? EMouseCursor::Hand : EMouseCursor::Default);
	}

private:
	TWeakObjectPtr<AVespPlayerController> Joueur;
	FSlateBrush Rond;
	FSlateBrush Anneau;

	static float PositionX(int32 Etage, const FVector2D& Taille)
	{
		return 40.0f + Etage * (Taille.X - 80.0f) / (AVespPlayerController::NombreDEtages - 1);
	}
	static FVector2D Position(const FVespNoeud& N, const FVector2D& Taille)
	{
		return FVector2D(PositionX(N.Etage, Taille), 40.0f + N.Hauteur * (Taille.Y - 90.0f));
	}
	static float Rayon(const FVespNoeud& N) { return N.Type == EVespSalle::Boss ? 32.0f : 19.0f; }

	static void Disque(FSlateWindowElementList& Elements, int32 Couche, const FGeometry& Geo, const FVector2D& Centre, float R,
	                   const FSlateBrush& Pinceau, const FLinearColor& Couleur)
	{
		FSlateDrawElement::MakeBox(Elements, Couche, Geo.ToPaintGeometry(FVector2f(2.0f * R, 2.0f * R), FSlateLayoutTransform(FVector2f(Centre - FVector2D(R, R)))),
		                           &Pinceau, ESlateDrawEffect::None, Couleur);
	}

	int32 SalleSous(const FGeometry& Geo, const FPointerEvent& Evenement) const
	{
		const AVespPlayerController* J = Joueur.Get();
		if (!J)
		{
			return -1;
		}
		const FVector2D Ici = Geo.AbsoluteToLocal(Evenement.GetScreenSpacePosition());
		const FVector2D Taille = Geo.GetLocalSize();
		for (int32 i = 0; i < J->Noeuds.Num(); i++)
		{
			if (FVector2D::Distance(Ici, Position(J->Noeuds[i], Taille)) <= Rayon(J->Noeuds[i]) + 8.0f)
			{
				return i;
			}
		}
		return -1;
	}
};

// ===================== La construction =====================

void SVespInterface::Construct(const FArguments& Args)
{
	Joueur = Args._Joueur;

	Panneau = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.028f, 0.06f, 0.86f), 16.0f, FLinearColor(0.24f, 0.21f, 0.35f, 0.9f), 1.0f);
	Blanc = FSlateRoundedBoxBrush(FLinearColor::White, 6.0f);
	Rond = FSlateRoundedBoxBrush(FLinearColor::White, 64.0f);
	Carte = FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.04f, 0.085f, 0.95f), 14.0f, FLinearColor(0.27f, 0.24f, 0.4f, 1.0f), 1.0f);
	CarteSurvol = FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.075f, 0.15f, 0.97f), 14.0f, FLinearColor(0.55f, 0.47f, 0.8f, 1.0f), 1.5f);
	CarteChoisie = FSlateRoundedBoxBrush(FLinearColor(0.2f, 0.14f, 0.05f, 0.97f), 14.0f, OR, 2.0f);
	GrandeCarte = FSlateRoundedBoxBrush(FLinearColor(0.045f, 0.035f, 0.08f, 0.97f), 22.0f, FLinearColor(0.3f, 0.26f, 0.45f, 1.0f), 1.5f);
	GrandeCarteSurvol = FSlateRoundedBoxBrush(FLinearColor(0.08f, 0.06f, 0.14f, 0.98f), 22.0f, VIOLET, 2.5f);
	Parchemin = FSlateRoundedBoxBrush(FLinearColor(0.87f, 0.79f, 0.61f, 1.0f), 10.0f, FLinearColor(0.36f, 0.2f, 0.07f, 1.0f), 3.0f);
	CadreDore = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 6.0f, FLinearColor(0.69f, 0.49f, 0.16f, 1.0f), 1.5f);
	Voile = FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.012f, 0.04f, 0.9f), 0.0f);
	Rien = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 0.0f);
	StyleBouton = FButtonStyle().SetNormal(Rien).SetHovered(Rien).SetPressed(Rien).SetDisabled(Rien)
	                            .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0, 2, 0, 0));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[CoucheCombat()]
		+ SOverlay::Slot()[CoucheRoute()]
		+ SOverlay::Slot()[CoucheRunes()]
		+ SOverlay::Slot()[CoucheMarchand()]
		+ SOverlay::Slot()[CoucheEvenement()]
		+ SOverlay::Slot()[CoucheDialogue()]
		+ SOverlay::Slot()[CoucheNouvelActe()]
		+ SOverlay::Slot()[CoucheFin()]
		+ SOverlay::Slot()[CoucheTitre()]
	];
}

bool SVespInterface::EnPhase(uint8 LaPhase) const
{
	return Joueur.IsValid() && (uint8)Joueur->Phase == LaPhase;
}

bool SVespInterface::EnCombat() const
{
	return EnPhase((uint8)EVespPhase::TourAylis) || EnPhase((uint8)EVespPhase::TourHaschen);
}

// Une jauge : un fond sombre arrondi, la partie remplie (qui suit la valeur), et un texte au centre
TSharedRef<SWidget> SVespInterface::Jauge(TAttribute<float> Part, TAttribute<FSlateColor> Couleur, TAttribute<FText> Legende, float Largeur, float Hauteur)
{
	return SNew(SBox).WidthOverride(Largeur).HeightOverride(Hauteur)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(FLinearColor(0.06f, 0.05f, 0.1f, 1.0f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride_Lambda([Part, Largeur]() { return FOptionalSize(FMath::Clamp(Part.Get(), 0.0f, 1.0f) * Largeur); })
			[
				SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(Couleur)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(Legende).Font(Police("Bold", 10)).ColorAndOpacity(FLinearColor::White)
			.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.8f))
		]
	];
}

// Un grand bouton de texte (continuer, nouvelle vision...)
TSharedRef<SWidget> SVespInterface::GrandBouton(const FText& Libelle, FLinearColor Couleur, TFunction<void()> Action)
{
	TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
	TSharedRef<SButton> Bouton = SNew(SButton).ButtonStyle(&StyleBouton).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
	[
		SNew(SBorder).Padding(FMargin(34, 12))
		.BorderImage_Lambda([this, Lien]() { const TSharedPtr<SButton> B = Lien->Pin(); return B.IsValid() && B->IsHovered() ? &CarteChoisie : &Carte; })
		[
			SNew(STextBlock).Text(Libelle).Font(Police("Bold", 16, 120)).ColorAndOpacity(Couleur)
		]
	];
	*Lien = Bouton;
	return Bouton;
}

// Un bouton du menu titre : du texte seul, qui s'allume en or au survol
TSharedRef<SWidget> SVespInterface::BoutonMenu(const FText& Libelle, TFunction<void()> Action)
{
	TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
	auto Survole = [Lien]() { const TSharedPtr<SButton> B = Lien->Pin(); return B.IsValid() && B->IsHovered(); };
	TSharedRef<SButton> Bouton = SNew(SButton).ButtonStyle(&StyleBouton).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
		[
			SNew(SBox).WidthOverride(22).HeightOverride(3)
			[
				SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor_Lambda([Survole]() { return FSlateColor(Survole() ? OR : FLinearColor(0, 0, 0, 0)); })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock).Text(Libelle).Font(Police("Bold", 20, 220))
			.ColorAndOpacity_Lambda([Survole]() { return FSlateColor(Survole() ? OR_PALE : TEXTE); })
		]
	];
	*Lien = Bouton;
	return Bouton;
}

// Une grande carte cliquable : un medaillon de couleur avec une lettre, un titre, un texte, un pied
TSharedRef<SWidget> SVespInterface::CarteCliquable(TAttribute<FText> Titre, TAttribute<FText> Corps, TAttribute<FText> Pied,
                                                   TFunction<FLinearColor()> Couleur, TAttribute<bool> Actif, TAttribute<EVisibility> Visible,
                                                   TFunction<void()> Clic, int32 Numero, float Largeur, float Hauteur)
{
	TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
	TSharedRef<SButton> Bouton = SNew(SButton).ButtonStyle(&StyleBouton).Visibility(Visible).IsEnabled(Actif)
	.OnClicked_Lambda([Clic]() { Clic(); return FReply::Handled(); })
	[
		SNew(SBox).WidthOverride(Largeur).HeightOverride(Hauteur)
		[
			SNew(SBorder).Padding(FMargin(20, 18))
			.BorderImage_Lambda([this, Lien]() { const TSharedPtr<SButton> B = Lien->Pin(); return B.IsValid() && B->IsHovered() ? &GrandeCarteSurvol : &GrandeCarte; })
			.ColorAndOpacity_Lambda([Actif]() { return FLinearColor(1, 1, 1, Actif.Get() ? 1.0f : 0.45f); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Texte(FString::FromInt(Numero + 1))).Font(Police("Bold", 12)).ColorAndOpacity(DOUX)
				]
				// Le medaillon : un disque lumineux de la couleur de la carte, avec l'initiale
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 14)
				[
					SNew(SBox).WidthOverride(92).HeightOverride(92)
					[
						SNew(SBorder).BorderImage(&Rond).Padding(4)
						.BorderBackgroundColor_Lambda([Couleur]() { FLinearColor C = Couleur(); C.A = 0.25f; return FSlateColor(C); })
						[
							SNew(SBorder).BorderImage(&Rond).HAlign(HAlign_Center).VAlign(VAlign_Center)
							.BorderBackgroundColor_Lambda([Couleur]() { return FSlateColor(Couleur() * FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)); })
							[
								SNew(STextBlock).Font(Police("Bold", 36))
								.ColorAndOpacity_Lambda([Couleur]() { return FSlateColor(Couleur()); })
								.Text_Lambda([Titre]() { return Texte(Titre.Get().ToString().Left(1)); })
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Font(Police("Bold", 19)).Justification(ETextJustify::Center).AutoWrapText(true).Text(Titre)
					.ColorAndOpacity_Lambda([Couleur]() { return FSlateColor(Couleur()); })
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).Padding(0, 10, 0, 0)
				[
					SNew(STextBlock).Font(Police("Regular", 13)).ColorAndOpacity(TEXTE).AutoWrapText(true).Justification(ETextJustify::Center).Text(Corps)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Font(Police("Bold", 13, 100)).ColorAndOpacity(OR).Text(Pied)
				]
			]
		]
	];
	*Lien = Bouton;
	return Bouton;
}

// L'en-tete des ecrans de choix : un petit sur-titre, un grand titre, une ligne dessous
TSharedRef<SWidget> SVespInterface::EnTete(TAttribute<FText> Petit, TAttribute<FText> Grand, TAttribute<FText> Dessous)
{
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(STextBlock).Font(Police("Bold", 11, 400)).ColorAndOpacity(VIOLET).Text(Petit)
	]
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 8)
	[
		SNew(STextBlock).Font(Police("Bold", 36, 120)).ColorAndOpacity(TEXTE).Text(Grand)
	]
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(STextBlock).Font(Police("Regular", 14)).ColorAndOpacity(DOUX).Text(Dessous)
	];
}

// ===================== L'ecran titre =====================

TSharedRef<SWidget> SVespInterface::CoucheTitre()
{
	auto Commandes = [this]() -> TSharedRef<SWidget> {
		TSharedRef<SVerticalBox> Liste = SNew(SVerticalBox);
		const TCHAR* Lignes[] = {
			TEXT("Clic sur une case bleue : deplacer AYLIS (une fois par tour)"),
			TEXT("1 a 5 : choisir une action (attaque, lourde, garde, potion, special)"),
			TEXT("Clic sur un Haschen au contact : frapper"),
			TEXT("ESPACE : finir le tour sans agir"),
			TEXT("1 a 4 : choisir une salle, une rune, un objet"),
			TEXT("ENTREE : valider, continuer un dialogue"),
			TEXT("R : une nouvelle vision (apres la fin)"),
		};
		for (const TCHAR* L : Lignes)
		{
			Liste->AddSlot().AutoHeight().Padding(0, 4)[SNew(STextBlock).Text(Texte(L)).Font(Police("Regular", 13)).ColorAndOpacity(TEXTE)];
		}
		return SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(26, 20))
		.Visibility_Lambda([this]() { return VisibleSi(bCommandes); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(STextBlock).Text(LOCTEXT("Commandes", "COMMANDES")).Font(Police("Bold", 12, 300)).ColorAndOpacity(OR)
			]
			+ SVerticalBox::Slot().AutoHeight()[Liste]
		];
	};
	TSharedRef<SHorizontalBox> Actes = SNew(SHorizontalBox);
	for (int32 A = 1; A <= AVespPlayerController::NombreDActes; A++)
	{
		Actes->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
		[
			GrandBouton(Texte(AVespPlayerController::Romain(A)), OR_PALE, [this, A]() { if (Joueur.IsValid()) Joueur->NouvellePartie(A); })
		];
	}
	return SNew(SOverlay).Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Titre)); })
	// Un voile sombre a gauche, pour lire le titre sur la foret
	+ SOverlay::Slot().HAlign(HAlign_Left)
	[
		SNew(SBox).WidthOverride(760)
		[
			SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.55f))
		]
	]
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(90, 0, 0, 0))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("Studio", "BLUEMOON INTERACTIVE PRESENTE")).Font(Police("Regular", 11, 500)).ColorAndOpacity(DOUX)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("Titre", "VESPERANCE")).Font(Police("Bold", 78, 380))
			.ColorAndOpacity_Lambda([]() { const float P = Pulsation(1.5f); return FSlateColor(FLinearColor(0.86f + 0.1f * P, 0.8f + 0.1f * P, 1.0f)); })
			.ShadowOffset(FVector2D(0, 4)).ShadowColorAndOpacity(FLinearColor(0.45f, 0.25f, 0.9f, 0.8f))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4, 6, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("SousTitre", "Une vision. Une route. Sept terres a traverser.")).Font(Police("Italic", 18)).ColorAndOpacity(BRUME)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 54, 0, 0)
		[
			BoutonMenu(LOCTEXT("Nouvelle", "NOUVELLE PARTIE"), [this]() { if (Joueur.IsValid()) Joueur->NouvellePartie(1); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
		[
			BoutonMenu(LOCTEXT("ChoisirActe", "COMMENCER A UN AUTRE ACTE"), [this]() { bActes = !bActes; bCommandes = false; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(34, 10, 0, 0)
		[
			SNew(SVerticalBox).Visibility_Lambda([this]() { return VisibleSi(bActes); })
			+ SVerticalBox::Slot().AutoHeight()[Actes]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(STextBlock).Text(LOCTEXT("ActeTest", "Pour tester : AYLIS recoit des forces a la hauteur de l'acte choisi."))
				.Font(Police("Italic", 11)).ColorAndOpacity(DOUX)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
		[
			BoutonMenu(LOCTEXT("VoirCommandes", "COMMANDES"), [this]() { bCommandes = !bCommandes; bActes = false; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
		[
			BoutonMenu(LOCTEXT("Quitter", "QUITTER"), [this]() { if (Joueur.IsValid()) Joueur->Quitter(); })
		]
	]
	+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin(0, 0, 80, 0))[Commandes()]
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(FMargin(90, 0, 0, 40))
	[
		SNew(STextBlock).Text(LOCTEXT("Entree", "ENTREE pour commencer")).Font(Police("Bold", 12, 300))
		.ColorAndOpacity_Lambda([]() { return FSlateColor(FLinearColor(OR.R, OR.G, OR.B, 0.35f + 0.65f * Pulsation(2.5f))); })
	]
	+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 40, 30))
	[
		SNew(STextBlock).Text(LOCTEXT("Mention", "BlueMoon Interactive  -  2027  -  en developpement, susceptible de changer"))
		.Font(Police("Regular", 10)).ColorAndOpacity(FLinearColor(0.66f, 0.63f, 0.75f, 0.6f))
	];
}

// ===================== Le combat =====================

TSharedRef<SWidget> SVespInterface::CoucheCombat()
{
	return SNew(SOverlay).Visibility_Lambda([this]() { return VisibleSi(EnCombat()); })
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24)[CarteDuLieu()]
	+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(20)[PastilleDuTour()]
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24)[FicheAylis()]
	+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 0, 24))[BarreDActions()];
}

// En haut a gauche : l'acte, l'etage, le lieu, la vague, et le journal des dernieres actions
TSharedRef<SWidget> SVespInterface::CarteDuLieu()
{
	TSharedRef<SVerticalBox> Journal = SNew(SVerticalBox);
	for (int32 i = 0; i < 4; i++)
	{
		Journal->AddSlot().AutoHeight().Padding(0, 3)
		[
			SNew(STextBlock).Font(Police("Regular", 11)).AutoWrapText(true)
			.Text_Lambda([this, i]() {
				const AVespPlayerController* J = Joueur.Get();
				return J && J->Journal.IsValidIndex(i) ? Texte(J->Journal[i]) : FText::GetEmpty();
			})
			.ColorAndOpacity_Lambda([this, i]() {
				const AVespPlayerController* J = Joueur.Get();
				if (!J || !J->Journal.IsValidIndex(i)) return FSlateColor(DOUX);
				const FString& L = J->Journal[i];
				FLinearColor C = DOUX;
				if (L.Contains(TEXT("AYLIS perd")) || L.Contains(TEXT("touche AYLIS")) || L.Contains(TEXT("Poison")) || L.Contains(TEXT("ronge"))) C = FLinearColor(1.0f, 0.55f, 0.52f);
				else if (L.Contains(TEXT("tombe")) || L.Contains(TEXT("succombe")) || L.Contains(TEXT("BRISEE")) || L.Contains(TEXT("Esquive"))) C = FLinearColor(0.55f, 0.9f, 0.6f);
				else if (L.Contains(TEXT("!"))) C = OR_PALE;
				C.A = 0.45f + 0.55f * (i + 1) / FMath::Max(1, J->Journal.Num());
				return FSlateColor(C);
			})
		];
	}
	return SNew(SBox).WidthOverride(400)
	[
		SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(20, 16))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Bold", 10, 260)).ColorAndOpacity(OR)
				.Text_Lambda([this]() {
					const AVespPlayerController* J = Joueur.Get();
					return J ? Texte(FString::Printf(TEXT("ACTE %s  -  ETAGE %d / %d"), *AVespPlayerController::Romain(J->Acte), J->Etage + 1, AVespPlayerController::NombreDEtages))
					         : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
			[
				SNew(STextBlock).Font(Police("Bold", 20)).ColorAndOpacity(TEXTE)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->NomDuLieu()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(STextBlock).Font(Police("Bold", 11, 150)).ColorAndOpacity(FLinearColor(1.0f, 0.6f, 0.45f))
				.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->VagueActuelle + Joueur->VaguesRestantes > 1); })
				.Text_Lambda([this]() {
					const AVespPlayerController* J = Joueur.Get();
					return J ? Texte(FString::Printf(TEXT("VAGUE %d / %d"), J->VagueActuelle, J->VagueActuelle + J->VaguesRestantes)) : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(FLinearColor(0.3f, 0.27f, 0.42f, 0.8f))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[Journal]
		]
	];
}

// En haut au centre : a qui c'est de jouer, et l'etat du terrain (pieges leves, blizzard)
TSharedRef<SWidget> SVespInterface::PastilleDuTour()
{
	auto CouleurTour = [this]() { return EnPhase((uint8)EVespPhase::TourAylis) ? VERT : ROUGE; };
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(SBorder).BorderImage(&Rond).Padding(FMargin(26, 8))
		.BorderBackgroundColor_Lambda([CouleurTour]() { FLinearColor C = CouleurTour(); C.A = 0.22f; return FSlateColor(C); })
		[
			SNew(STextBlock).Font(Police("Bold", 14, 220))
			.ColorAndOpacity_Lambda([CouleurTour]() { return FSlateColor(CouleurTour()); })
			.Text_Lambda([this]() { return EnPhase((uint8)EVespPhase::TourAylis) ? LOCTEXT("TonTour", "TON TOUR") : LOCTEXT("TourHaschen", "TOUR DES HASCHEN"); })
		]
	]
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
	[
		SNew(STextBlock).Font(Police("Regular", 11, 120)).ColorAndOpacity(DOUX)
		.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("Tour %d"), Joueur->Tour)) : FText::GetEmpty(); })
	]
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
	[
		SNew(STextBlock).Font(Police("Bold", 11, 150))
		.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && (Joueur->bBlizzard || (Joueur->Acte == 4 && Joueur->Tour % 2 == 1))); })
		.ColorAndOpacity_Lambda([this]() { return FSlateColor(Joueur.IsValid() && Joueur->bBlizzard ? GIVRE : FLinearColor(1.0f, 0.5f, 0.4f)); })
		.Text_Lambda([this]() {
			if (!Joueur.IsValid()) return FText::GetEmpty();
			return Joueur->bBlizzard ? LOCTEXT("Blizzard", "BLIZZARD : -1 case de deplacement")
			                         : LOCTEXT("PiegesBientot", "Les dalles piegees se leveront au prochain tour");
		})
	];
}

// En bas a gauche : la fiche d'AYLIS
TSharedRef<SWidget> SVespInterface::FicheAylis()
{
	auto Pv = [this]() -> float {
		const AVespPlayerController* J = Joueur.Get();
		return J && J->Aylis ? (float)J->Aylis->Stats.Pv / FMath::Max(1, J->Aylis->Stats.PvMax) : 0.0f;
	};
	auto Pastille = [this](TAttribute<FText> Libelle, FLinearColor Couleur, TAttribute<EVisibility> Visible) -> TSharedRef<SWidget> {
		return SNew(SBorder).BorderImage(&Rond).Padding(FMargin(10, 3)).Visibility(Visible)
		.BorderBackgroundColor(FLinearColor(Couleur.R, Couleur.G, Couleur.B, 0.2f))
		[
			SNew(STextBlock).Text(Libelle).Font(Police("Bold", 9, 80)).ColorAndOpacity(Couleur)
		];
	};
	auto EtatAylis = [this](int32 Quoi) {
		const AVespPlayerController* J = Joueur.Get();
		if (!J || !J->Aylis) return false;
		switch (Quoi)
		{
			case 0: return J->Aylis->Poison > 0;
			case 1: return J->Aylis->Brulure > 0;
			case 2: return J->Aylis->Gel > 0;
			default: return J->bEnGarde;
		}
	};
	return SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(18, 16))
	[
		SNew(SHorizontalBox)
		// Le portrait : un medaillon bleu nuit, cercle dore
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 16, 0)
		[
			SNew(SBox).WidthOverride(84).HeightOverride(84)
			[
				SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(OR).Padding(3)
				[
					SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0.12f, 0.17f, 0.42f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("InitialeA", "A")).Font(Police("Bold", 34)).ColorAndOpacity(FLinearColor(0.8f, 0.86f, 1.0f))
					]
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock).Text(LOCTEXT("Aylis", "AYLIS")).Font(Police("Bold", 18, 180)).ColorAndOpacity(BLEU_AYLIS)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(10, 0, 0, 2)
				[
					SNew(STextBlock).Font(Police("Italic", 10)).ColorAndOpacity(DOUX)
					.Text_Lambda([this]() {
						const AVespPlayerController* J = Joueur.Get();
						return J && J->Aylis ? Texte(FString::Printf(TEXT("attaque %d  -  defense %d"), J->Aylis->Stats.Attaque, J->Aylis->Stats.Defense)) : FText::GetEmpty();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				Jauge(TAttribute<float>::CreateLambda(Pv),
				      TAttribute<FSlateColor>::CreateLambda([Pv]() { return FSlateColor(CouleurVie(Pv())); }),
				      TAttribute<FText>::CreateLambda([this]() {
					      const AVespPlayerController* J = Joueur.Get();
					      return J && J->Aylis ? Texte(FString::Printf(TEXT("%d / %d  PV"), FMath::Max(0, J->Aylis->Stats.Pv), J->Aylis->Stats.PvMax)) : FText::GetEmpty();
				      }), 300, 20)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				Jauge(TAttribute<float>::CreateLambda([this]() { return Joueur.IsValid() ? Joueur->Rage / 100.0f : 0.0f; }),
				      TAttribute<FSlateColor>::CreateLambda([this]() {
					      const bool bPleine = Joueur.IsValid() && Joueur->Rage >= 100;
					      const float P = Pulsation(8.0f);
					      return FSlateColor(bPleine ? FLinearColor(1.0f, 0.45f + 0.3f * P, 0.15f) : FLinearColor(0.75f, 0.25f, 0.2f));
				      }),
				      TAttribute<FText>::CreateLambda([this]() {
					      if (!Joueur.IsValid()) return FText::GetEmpty();
					      return Joueur->Rage >= 100 ? LOCTEXT("RagePleine", "RAGE PLEINE  -  touche 5") : Texte(FString::Printf(TEXT("RAGE  %d%%"), Joueur->Rage));
				      }), 300, 12)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(TAttribute<FText>::CreateLambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("POTIONS %d"), Joueur->Potions)) : FText::GetEmpty(); }),
					         FLinearColor(0.95f, 0.5f, 0.5f), EVisibility::Visible)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(TAttribute<FText>::CreateLambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("ECLATS %d"), Joueur->Eclats)) : FText::GetEmpty(); }),
					         OR, EVisibility::Visible)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(TAttribute<FText>::CreateLambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("RUNES %d"), Joueur->Runes.Num())) : FText::GetEmpty(); }),
					         VIOLET, EVisibility::Visible)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(LOCTEXT("Poison", "POISON"), FLinearColor(0.7f, 0.95f, 0.4f),
					         TAttribute<EVisibility>::CreateLambda([this, EtatAylis]() { return VisibleSi(EtatAylis(0)); }))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(LOCTEXT("Brulure", "BRULURE"), FLinearColor(1.0f, 0.6f, 0.25f),
					         TAttribute<EVisibility>::CreateLambda([this, EtatAylis]() { return VisibleSi(EtatAylis(1)); }))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(LOCTEXT("Gel", "GEL"), GIVRE,
					         TAttribute<EVisibility>::CreateLambda([this, EtatAylis]() { return VisibleSi(EtatAylis(2)); }))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					Pastille(LOCTEXT("Garde", "EN GARDE"), BRUME,
					         TAttribute<EVisibility>::CreateLambda([this, EtatAylis]() { return VisibleSi(EtatAylis(3)); }))
				]
			]
		]
	];
}

// Un bouton d'action : son numero, son nom, son effet. Il s'allume quand il est choisi.
TSharedRef<SWidget> SVespInterface::BoutonAction(int32 Numero)
{
	const EVespAction Action = (EVespAction)Numero;
	TSharedPtr<SButton> Bouton;
	SAssignNew(Bouton, SButton).ButtonStyle(&StyleBouton)
	.ToolTipText(Texte(AVespPlayerController::NomAction(Action) + TEXT(" : ") + AVespPlayerController::AideAction(Action)))
	.IsEnabled_Lambda([this, Action]() { return Joueur.IsValid() && Joueur->Phase == EVespPhase::TourAylis && Joueur->ActionDisponible(Action); })
	.OnClicked_Lambda([this, Action]() { if (Joueur.IsValid()) Joueur->ChoisirAction(Action); return FReply::Handled(); })
	[
		SNew(SBox).WidthOverride(128).HeightOverride(92)
		[
			SNew(SBorder).Padding(FMargin(12, 10))
			.BorderImage_Lambda([this, Action, Numero]() {
				const bool bChoisie = Joueur.IsValid() && Joueur->ActionChoisie == Action && Action != EVespAction::Potion;
				if (bChoisie) return &CarteChoisie;
				return BoutonsActions.IsValidIndex(Numero) && BoutonsActions[Numero]->IsHovered() ? &CarteSurvol : &Carte;
			})
			.ColorAndOpacity_Lambda([this, Action]() {
				return FLinearColor(1, 1, 1, Joueur.IsValid() && Joueur->ActionDisponible(Action) ? 1.0f : 0.35f);
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(20).HeightOverride(20)
						[
							SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.55f)).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0)
							[
								SNew(STextBlock).Text(Texte(FString::FromInt(Numero + 1))).Font(Police("Bold", 9)).ColorAndOpacity(OR)
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 2)
				[
					SNew(STextBlock).Text(Texte(AVespPlayerController::NomAction(Action))).Font(Police("Bold", 15)).ColorAndOpacity(TEXTE)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(Police("Regular", 9)).ColorAndOpacity(DOUX)
					.Text_Lambda([this, Action]() { return Texte(AVespPlayerController::DetailAction(Action, Joueur.IsValid() ? Joueur->Potions : 0)); })
				]
			]
		]
	];
	if (BoutonsActions.Num() <= Numero)
	{
		BoutonsActions.SetNum(Numero + 1);
	}
	BoutonsActions[Numero] = Bouton;
	return Bouton.ToSharedRef();
}

// En bas au centre : la consigne, puis les 5 actions et "passer"
TSharedRef<SWidget> SVespInterface::BarreDActions()
{
	TSharedRef<SHorizontalBox> Rangee = SNew(SHorizontalBox);
	for (int32 i = 0; i < 5; i++)
	{
		Rangee->AddSlot().AutoWidth().Padding(5, 0)[BoutonAction(i)];
	}
	Rangee->AddSlot().AutoWidth().Padding(12, 0, 0, 0).VAlign(VAlign_Center)
	[
		SNew(SButton).ButtonStyle(&StyleBouton).ToolTipText(LOCTEXT("PasserAide", "Finir le tour sans agir (ESPACE)"))
		.IsEnabled_Lambda([this]() { return EnPhase((uint8)EVespPhase::TourAylis); })
		.OnClicked_Lambda([this]() { if (Joueur.IsValid()) Joueur->PasserLeTour(); return FReply::Handled(); })
		[
			SNew(SBorder).BorderImage(&Carte).Padding(FMargin(16, 10))
			[
				SNew(STextBlock).Text(LOCTEXT("Passer", "PASSER")).Font(Police("Bold", 11, 160)).ColorAndOpacity(DOUX)
			]
		]
	];
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)
	[
		SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.55f)).Padding(FMargin(18, 6))
		.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::TourAylis)); })
		[
			SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(OR_PALE)
			.Text_Lambda([this]() {
				if (!Joueur.IsValid()) return FText::GetEmpty();
				return Joueur->bADejaBouge ? LOCTEXT("Consigne2", "Choisis une action, puis clique sur un Haschen au contact")
				                           : LOCTEXT("Consigne1", "Deplace AYLIS sur une case bleue, ou agis directement");
			})
		]
	]
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(12, 12))[Rangee]
	];
}

// ===================== La carte de la route =====================

TSharedRef<SWidget> SVespInterface::CoucheRoute()
{
	SAssignNew(CarteRoute, SVespCarteRoute).Joueur(Joueur);
	auto Legende = [this](EVespSalle S) -> TSharedRef<SWidget> {
		const FLinearColor C = AVespPlayerController::CouleurSalle(S);
		return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
		[
			SNew(SBox).WidthOverride(22).HeightOverride(22)
			[
				SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(C * FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0)
				[
					SNew(STextBlock).Text(Texte(AVespPlayerController::LettreSalle(S))).Font(Police("Bold", 10)).ColorAndOpacity(FLinearColor::White)
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18, 0)
		[
			SNew(STextBlock).Text(Texte(S == EVespSalle::Boss ? FString(TEXT("Boss")) : AVespPlayerController::NomSalle(S, 1))).Font(Police("Regular", 11)).ColorAndOpacity(DOUX)
		];
	};
	return SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.82f)).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::ChoixSalle)); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			EnTete(TAttribute<FText>::CreateLambda([this]() {
				       return Joueur.IsValid() ? Texte(FString::Printf(TEXT("ACTE %s  -  %s"), *AVespPlayerController::Romain(Joueur->Acte), *Joueur->NomDeLActe().ToUpper())) : FText::GetEmpty();
			       }),
			       LOCTEXT("TitreRoute", "LA ROUTE DE LA VISION"),
			       TAttribute<FText>::CreateLambda([this]() {
				       const AVespPlayerController* J = Joueur.Get();
				       return J && J->Aylis ? Texte(FString::Printf(TEXT("PV %d / %d   -   potions %d   -   eclats %d   -   runes %d"),
				                                                   J->Aylis->Stats.Pv, J->Aylis->Stats.PvMax, J->Potions, J->Eclats, J->Runes.Num()))
				                            : FText::GetEmpty();
			       }))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
		[
			SNew(STextBlock).Font(Police("Italic", 13)).ColorAndOpacity(FLinearColor(0.55f, 0.9f, 0.62f)).AutoWrapText(true)
			.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->MessageRoute) : FText::GetEmpty(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(50, 18, 50, 0))
		[
			SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(10, 12))
			[
				SNew(SBox).HeightOverride(470)[CarteRoute.ToSharedRef()]
			]
		]
		// Ce que la salle survolee annonce
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)
		[
			SNew(SBox).HeightOverride(48)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Font(Police("Bold", 16))
					.ColorAndOpacity_Lambda([this]() {
						const AVespPlayerController* J = Joueur.Get();
						const int32 S = CarteRoute.IsValid() ? CarteRoute->Survol : -1;
						return FSlateColor(J && J->Noeuds.IsValidIndex(S) ? AVespPlayerController::CouleurSalle(J->Noeuds[S].Type) : TEXTE);
					})
					.Text_Lambda([this]() {
						const AVespPlayerController* J = Joueur.Get();
						const int32 S = CarteRoute.IsValid() ? CarteRoute->Survol : -1;
						if (!J || !J->Noeuds.IsValidIndex(S))
						{
							return LOCTEXT("ChoisisSalle", "Choisis une salle qui scintille (clic, ou touches 1 a 4)");
						}
						const FVespNoeud& N = J->Noeuds[S];
						const bool bRaccourci = J->NoeudActuel >= 0 && N.Etage - J->Noeuds[J->NoeudActuel].Etage > 1;
						return Texte(FString::Printf(TEXT("%s  -  etage %d%s"), *AVespPlayerController::NomSalle(N.Type, J->Acte), N.Etage + 1,
						                             bRaccourci ? TEXT("  (raccourci)") : TEXT("")));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 0)
				[
					SNew(STextBlock).Font(Police("Regular", 13)).ColorAndOpacity(DOUX)
					.Text_Lambda([this]() {
						const AVespPlayerController* J = Joueur.Get();
						const int32 S = CarteRoute.IsValid() ? CarteRoute->Survol : -1;
						if (!J || !J->Noeuds.IsValidIndex(S))
						{
							return LOCTEXT("AideRoute", "Les pointilles orange sont des raccourcis : ils sautent des etages, mais menent au combat.");
						}
						return Texte(AVespPlayerController::AideSalle(J->Noeuds[S].Type));
					})
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 12, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Combat)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Elite)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Evenement)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Marchand)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Repos)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Boss)]
		]
	];
}

// ===================== Les runes =====================

TSharedRef<SWidget> SVespInterface::CoucheRunes()
{
	TSharedRef<SHorizontalBox> Cartes = SNew(SHorizontalBox);
	for (int32 i = 0; i < 3; i++)
	{
		auto Rune = [this, i]() { return Joueur.IsValid() && Joueur->RunesProposees.IsValidIndex(i) ? Joueur->RunesProposees[i] : -1; };
		Cartes->AddSlot().AutoWidth().Padding(14, 0)
		[
			CarteCliquable(
				TAttribute<FText>::CreateLambda([Rune]() { return Rune() >= 0 ? Texte(AVespPlayerController::NomRune(Rune())) : FText::GetEmpty(); }),
				TAttribute<FText>::CreateLambda([Rune]() { return Rune() >= 0 ? Texte(AVespPlayerController::AideRune(Rune())) : FText::GetEmpty(); }),
				FText::GetEmpty(),
				[Rune]() { return Rune() >= 4 ? FLinearColor(0.95f, 0.7f, 0.35f) : VIOLET; },
				true,
				TAttribute<EVisibility>::CreateLambda([this, Rune]() { return VisibleSi(Rune() >= 0); }),
				[this, i]() { if (Joueur.IsValid()) Joueur->ChoisirRune(i); },
				i, 260, 320)
		];
	}
	return SNew(SBorder).BorderImage(&Voile).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::ChoixRune)); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			EnTete(LOCTEXT("TitreRunes1", "LA PROPHETIE T'OFFRE"), LOCTEXT("TitreRunes2", "UNE RUNE"),
			       LOCTEXT("AideRunes", "AYLIS la garde jusqu'a la fin de la route. Les runes dorees sont uniques."))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 30)
		[
			SNew(STextBlock).Font(Police("Italic", 13)).ColorAndOpacity(FLinearColor(0.55f, 0.9f, 0.62f))
			.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->MessageRoute) : FText::GetEmpty(); })
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Cartes]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 28, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("AideChoix", "Clique sur une carte, ou tape son numero")).Font(Police("Regular", 12)).ColorAndOpacity(DOUX)
		]
	];
}

// ===================== Le marchand =====================

TSharedRef<SWidget> SVespInterface::CoucheMarchand()
{
	TSharedRef<SHorizontalBox> Cartes = SNew(SHorizontalBox);
	for (int32 i = 0; i < 4; i++)
	{
		auto Offre = [this, i]() -> const FVespOffre* { return Joueur.IsValid() && Joueur->Offres.IsValidIndex(i) ? &Joueur->Offres[i] : nullptr; };
		Cartes->AddSlot().AutoWidth().Padding(10, 0)
		[
			CarteCliquable(
				TAttribute<FText>::CreateLambda([Offre]() { return Offre() ? Texte(Offre()->Nom) : FText::GetEmpty(); }),
				TAttribute<FText>::CreateLambda([Offre]() { return Offre() ? Texte(Offre()->Aide) : FText::GetEmpty(); }),
				TAttribute<FText>::CreateLambda([Offre]() {
					if (!Offre()) return FText::GetEmpty();
					return Offre()->bVendue ? LOCTEXT("Vendu", "VENDU") : Texte(FString::Printf(TEXT("%d eclats"), Offre()->Prix));
				}),
				[Offre]() { return Offre() && Offre()->Genre == 1 ? VIOLET : BRUME; },
				TAttribute<bool>::CreateLambda([this, Offre]() { return Offre() && !Offre()->bVendue && Joueur->Eclats >= Offre()->Prix; }),
				TAttribute<EVisibility>::CreateLambda([this, Offre]() { return VisibleSi(Offre() != nullptr); }),
				[this, i]() { if (Joueur.IsValid()) Joueur->AcheterOffre(i); },
				i, 230, 300)
		];
	}
	return SNew(SBorder).BorderImage(&Voile).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Marchand)); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			EnTete(LOCTEXT("Marchand1", "SUR LE BORD DE LA ROUTE"), LOCTEXT("Marchand2", "LE MARCHAND AMBULANT"),
			       LOCTEXT("Marchand3", "\"Tout se paie. Surtout les visions.\""))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 28)
		[
			SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(OR.R, OR.G, OR.B, 0.18f)).Padding(FMargin(22, 6))
			[
				SNew(STextBlock).Font(Police("Bold", 16, 120)).ColorAndOpacity(OR)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("ECLATS : %d"), Joueur->Eclats)) : FText::GetEmpty(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Cartes]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 30, 0, 0)
		[
			GrandBouton(LOCTEXT("QuitterMarchand", "REPRENDRE LA ROUTE  (ENTREE)"), OR_PALE, [this]() { if (Joueur.IsValid()) Joueur->QuitterMarchand(); })
		]
	];
}

// ===================== Un evenement =====================

TSharedRef<SWidget> SVespInterface::CoucheEvenement()
{
	auto Choix = [this](int32 c) -> TSharedRef<SWidget> {
		TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
		TSharedRef<SButton> Bouton = SNew(SButton).ButtonStyle(&StyleBouton)
		.OnClicked_Lambda([this, c]() { if (Joueur.IsValid()) Joueur->ChoisirEvenement(c); return FReply::Handled(); })
		[
			SNew(SBox).WidthOverride(360)
			[
				SNew(SBorder).Padding(FMargin(20, 14))
				.BorderImage_Lambda([this, Lien]() { const TSharedPtr<SButton> B = Lien->Pin(); return B.IsValid() && B->IsHovered() ? &CarteChoisie : &Carte; })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(Police("Bold", 16)).ColorAndOpacity(OR_PALE)
						.Text_Lambda([this, c]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("%d.  %s"), c + 1, *Joueur->ChoixEvenement(c))) : FText::GetEmpty(); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
					[
						SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(DOUX).AutoWrapText(true)
						.Text_Lambda([this, c]() { return Joueur.IsValid() ? Texte(Joueur->AideEvenement(c)) : FText::GetEmpty(); })
					]
				]
			]
		];
		*Lien = Bouton;
		return Bouton;
	};
	return SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.75f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Evenement)); })
	[
		SNew(SBox).WidthOverride(860)
		[
			SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(40, 34))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("Rencontre", "LA VISION SE TROUBLE")).Font(Police("Bold", 11, 400)).ColorAndOpacity(VIOLET)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8, 0, 18)
				[
					SNew(STextBlock).Font(Police("Bold", 30, 80)).ColorAndOpacity(TEXTE)
					.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->TitreEvenement()) : FText::GetEmpty(); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 28)
				[
					SNew(STextBlock).Font(Police("Italic", 16)).ColorAndOpacity(FLinearColor(0.85f, 0.82f, 0.92f)).AutoWrapText(true).Justification(ETextJustify::Center)
					.LineHeightPercentage(1.15f)
					.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->TexteEvenement()) : FText::GetEmpty(); })
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0)[Choix(0)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0)[Choix(1)]
				]
			]
		]
	];
}

// ===================== Le dialogue : un parchemin =====================

TSharedRef<SWidget> SVespInterface::CoucheDialogue()
{
	auto Orateur = [this]() -> FString {
		const AVespPlayerController* J = Joueur.Get();
		return J && J->Orateurs.IsValidIndex(J->LigneDialogue) ? J->Orateurs[J->LigneDialogue] : FString();
	};
	auto CouleurOrateur = [Orateur]() {
		const FString O = Orateur();
		if (O == TEXT("AYLIS")) return BLEU_AYLIS;
		if (O == TEXT("La prophetie") || O == TEXT("L'Oracle")) return VIOLET;
		if (O == TEXT("La Matriarche") || O == TEXT("Le Roi Noye")) return FLinearColor(0.45f, 0.9f, 0.5f);
		if (O == TEXT("Ashka")) return FLinearColor(1.0f, 0.45f, 0.65f);
		return FLinearColor(1.0f, 0.45f, 0.3f);
	};
	return SNew(SButton).ButtonStyle(&StyleBouton)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Dialogue)); })
	.OnClicked_Lambda([this]() { if (Joueur.IsValid()) Joueur->AvancerDialogue(); return FReply::Handled(); })
	[
		SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.55f)).VAlign(VAlign_Bottom).HAlign(HAlign_Center).Padding(FMargin(0, 0, 0, 60))
		[
			SNew(SBox).WidthOverride(1000)
			[
				SNew(SOverlay)
				// Le parchemin
				+ SOverlay::Slot().Padding(FMargin(0, 26, 0, 0))
				[
					SNew(SBorder).BorderImage(&Parchemin).Padding(10)
					[
						SNew(SBorder).BorderImage(&CadreDore).Padding(FMargin(40, 42, 40, 26))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).MinDesiredHeight(96)
								[
									SNew(STextBlock).Font(Police("Regular", 19)).ColorAndOpacity(FLinearColor(0.2f, 0.11f, 0.04f)).AutoWrapText(true).LineHeightPercentage(1.15f)
									.Text_Lambda([this]() {
										const AVespPlayerController* J = Joueur.Get();
										if (!J || !J->Repliques.IsValidIndex(J->LigneDialogue)) return FText::GetEmpty();
										const FString& R = J->Repliques[J->LigneDialogue];
										return Texte(R.Left(FMath::Clamp((int32)J->Ecriture, 0, R.Len())));
									})
								]
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
							[
								SNew(STextBlock).Text(LOCTEXT("Suite", "cliquer ou ENTREE")).Font(Police("Italic", 10)).ColorAndOpacity(FLinearColor(0.42f, 0.29f, 0.16f))
							]
						]
					]
				]
				// Le ruban du nom, par-dessus le bord du parchemin
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(46, 0, 0, 0))
				[
					SNew(SBorder).BorderImage(&Blanc).Padding(FMargin(22, 8))
					.BorderBackgroundColor_Lambda([CouleurOrateur]() { return FSlateColor(CouleurOrateur() * FLinearColor(0.38f, 0.38f, 0.38f, 1.0f)); })
					[
						SNew(STextBlock).Font(Police("Bold", 18, 140)).ColorAndOpacity(FLinearColor(1.0f, 0.95f, 0.87f))
						.Text_Lambda([Orateur]() { return Texte(Orateur().ToUpper()); })
					]
				]
			]
		]
	];
}

// ===================== Un nouvel acte =====================

TSharedRef<SWidget> SVespInterface::CoucheNouvelActe()
{
	return SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.8f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::NouvelActe)); })
	.ColorAndOpacity_Lambda([this]() { return FLinearColor(1, 1, 1, Joueur.IsValid() ? FMath::Clamp(Joueur->TempsPhase, 0.0f, 1.0f) : 1.0f); })
	[
		SNew(SBox).WidthOverride(980)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(Police("Bold", 70, 300)).ColorAndOpacity(FLinearColor(0.88f, 0.8f, 1.0f))
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(TEXT("ACTE ") + AVespPlayerController::Romain(Joueur->Acte)) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 14)
			[
				SNew(STextBlock).Font(Police("Bold", 30)).ColorAndOpacity(OR)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->NomDeLActe()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(Police("Regular", 15)).ColorAndOpacity(DOUX)
				.Text_Lambda([this]() {
					return Joueur.IsValid() ? Texte(Joueur->NomDuLieu() + TEXT("   -   ") + Joueur->NomDuBoss() + TEXT(" attend au bout de la route")) : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 30, 0, 0)
			[
				SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(26, 16))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("LaRegle", "CE QUI T'ATTEND")).Font(Police("Bold", 10, 300)).ColorAndOpacity(VIOLET)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
					[
						SNew(STextBlock).Font(Police("Italic", 15)).ColorAndOpacity(TEXTE).AutoWrapText(true).Justification(ETextJustify::Center)
						.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->RegleDeLActe()) : FText::GetEmpty(); })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 22, 0, 0)
			[
				SNew(STextBlock).Font(Police("Italic", 14)).ColorAndOpacity(FLinearColor(0.55f, 0.9f, 0.62f))
				.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->Acte > 1 && Joueur->Runes.Num() + Joueur->Tour > 0); })
				.Text(LOCTEXT("Forces", "AYLIS reprend des forces : tous les pv, +6 pv max, +1 attaque, et une potion."))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 34, 0, 0)
			[
				GrandBouton(LOCTEXT("Continuer", "PRENDRE LA ROUTE"), OR_PALE, [this]() { if (Joueur.IsValid()) Joueur->ContinuerApresLActe(); })
			]
		]
	];
}

// ===================== La fin : victoire, ou la vision se brise =====================

TSharedRef<SWidget> SVespInterface::CoucheFin()
{
	auto bVictoire = [this]() { return EnPhase((uint8)EVespPhase::Victoire); };
	return SNew(SBorder).BorderImage(&Voile).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Victoire) || EnPhase((uint8)EVespPhase::Defaite)); })
	.BorderBackgroundColor_Lambda([bVictoire]() { return bVictoire() ? FLinearColor(0.4f, 0.4f, 0.4f, 0.7f) : FLinearColor(0.7f, 0.4f, 1.0f, 0.85f); })
	[
		SNew(SBox).WidthOverride(1000)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(Police("Bold", 52, 160))
				.ColorAndOpacity_Lambda([bVictoire]() { return FSlateColor(bVictoire() ? OR_PALE : FLinearColor(0.85f, 0.75f, 1.0f)); })
				.Text_Lambda([bVictoire]() { return bVictoire() ? LOCTEXT("Victoire", "LE VOILE SE DECHIRE") : LOCTEXT("Defaite", "LA VISION SE BRISE..."); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 40)
			[
				SNew(STextBlock).Font(Police("Italic", 17)).ColorAndOpacity(DOUX).AutoWrapText(true).Justification(ETextJustify::Center)
				.Text_Lambda([this, bVictoire]() {
					if (bVictoire())
					{
						return LOCTEXT("VictoireSuite", "L'Oracle est tombe. Karn s'eveille, et la route s'ouvre sur un monde que personne n'avait jamais vu.");
					}
					const AVespPlayerController* J = Joueur.Get();
					return J ? Texte(FString::Printf(TEXT("Acte %s, etage %d. Ce n'etait qu'un futur possible. La prophetie en montre d'autres."),
					                                 *AVespPlayerController::Romain(J->Acte), J->Etage + 1))
					         : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				GrandBouton(LOCTEXT("Recommencer", "UNE NOUVELLE VISION  (R)"), OR_PALE, [this]() { if (Joueur.IsValid()) Joueur->Recommencer(); })
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
