#include "VespInterface.h"
#include "VespPlayerController.h"
#include "VespUnite.h"
#include "VespCombat.h"
#include "VespSons.h"
#include "VespMenu.h"
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

// ===================== La carte du monde (TAB) =====================
// Un widget dessine a la main : les clairieres de l'acte (des disques de couleur, avec leur lettre), les sentiers
// entre elles, celles deja faites (pleines), et AYLIS (le point bleu). La vision montre toute la route.

class SVespCarteRoute : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVespCarteRoute) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AVespPlayerController>, Joueur)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Joueur = Args._Joueur;
		Rond = FSlateRoundedBoxBrush(FLinearColor::White, 200.0f);
		Anneau = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 200.0f, FLinearColor::White, 3.0f);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1300.0f, 560.0f); }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
	                      int32 Couche, const FWidgetStyle& Style, bool bParentActif) const override
	{
		const AVespPlayerController* J = Joueur.Get();
		if (!J || J->Noeuds.Num() == 0)
		{
			return Couche;
		}
		const FVector2D Taille = Geo.GetLocalSize();
		const float Pulse = Pulsation(4.0f);
		const TSharedRef<FSlateFontMeasure> Mesure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		FBox2D Limites(ForceInit);
		for (const FVespNoeud& N : J->Noeuds)
		{
			Limites += FVector2D(N.Centre.X, N.Centre.Y);
		}
		Limites = Limites.ExpandBy(1200.0f);
		auto Ecran = [&](const FVector& P) { return Projeter(P, Limites, Taille); };

		// Les sentiers
		for (const FVespNoeud& N : J->Noeuds)
		{
			for (int32 S : N.Suivants)
			{
				const FVespNoeud& M = J->Noeuds[S];
				const FLinearColor C = N.bVisite && M.bVisite ? FLinearColor(0.95f, 0.75f, 0.35f, 0.9f) : FLinearColor(0.55f, 0.5f, 0.7f, 0.55f);
				FSlateDrawElement::MakeLines(Elements, Couche, Geo.ToPaintGeometry(), TArray<FVector2f>{FVector2f(Ecran(N.Centre)), FVector2f(Ecran(M.Centre))},
				                             ESlateDrawEffect::None, C, true, 4.0f);
			}
		}
		// Les clairieres
		for (int32 i = 0; i < J->Noeuds.Num(); i++)
		{
			const FVespNoeud& N = J->Noeuds[i];
			const FVector2D P = Ecran(N.Centre);
			const FLinearColor Couleur = AVespPlayerController::CouleurSalle(N.Type);
			const float R = N.Type == EVespSalle::Boss ? 28.0f : 18.0f;
			if (!N.bVisite)
			{
				Disque(Elements, Couche + 1, Geo, P, R * 1.7f, Rond, FLinearColor(Couleur.R, Couleur.G, Couleur.B, 0.12f + 0.1f * Pulse));
			}
			Disque(Elements, Couche + 2, Geo, P, R, Rond, N.bVisite ? Couleur * FLinearColor(0.45f, 0.45f, 0.45f, 1.0f) : FLinearColor(0.07f, 0.05f, 0.12f, 1.0f));
			Disque(Elements, Couche + 3, Geo, P, R, Anneau, Couleur);
			const FString Lettre = AVespPlayerController::LettreSalle(N.Type);
			const FSlateFontInfo F = Police("Bold", N.Type == EVespSalle::Boss ? 20 : 12);
			const FVector2D T(Mesure->Measure(Lettre, F));
			FSlateDrawElement::MakeText(Elements, Couche + 4, Geo.ToPaintGeometry(FVector2f(T), FSlateLayoutTransform(FVector2f(P - T / 2.0f))),
			                            Lettre, F, ESlateDrawEffect::None, FLinearColor::White);
		}
		// AYLIS
		if (J->Aylis)
		{
			const FVector2D P = Ecran(J->Aylis->GetActorLocation());
			Disque(Elements, Couche + 5, Geo, P, 14.0f + 4.0f * Pulse, Rond, FLinearColor(0.45f, 0.62f, 1.0f, 0.35f));
			Disque(Elements, Couche + 6, Geo, P, 8.0f, Rond, BLEU_AYLIS);
		}
		return Couche + 7;
	}

private:
	TWeakObjectPtr<AVespPlayerController> Joueur;
	FSlateBrush Rond;
	FSlateBrush Anneau;

	// Le monde vu de haut, a l'echelle du widget : l'est a droite, le nord en haut (comme a l'ecran)
	static FVector2D Projeter(const FVector& P, const FBox2D& Limites, const FVector2D& Taille)
	{
		const FVector2D T = Limites.GetSize();
		const float Echelle = FMath::Min((Taille.X - 60.0f) / FMath::Max(1.0f, T.Y), (Taille.Y - 60.0f) / FMath::Max(1.0f, T.X));
		const FVector2D Milieu = Limites.GetCenter();
		return Taille / 2.0f + FVector2D((P.Y - Milieu.Y) * Echelle, -(P.X - Milieu.X) * Echelle);
	}

	static void Disque(FSlateWindowElementList& Elements, int32 Couche, const FGeometry& Geo, const FVector2D& Centre, float R,
	                   const FSlateBrush& Pinceau, const FLinearColor& Couleur)
	{
		FSlateDrawElement::MakeBox(Elements, Couche, Geo.ToPaintGeometry(FVector2f(2.0f * R, 2.0f * R), FSlateLayoutTransform(FVector2f(Centre - FVector2D(R, R)))),
		                           &Pinceau, ESlateDrawEffect::None, Couleur);
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
		+ SOverlay::Slot()[CoucheExploration()]
		+ SOverlay::Slot()[CoucheRoute()]
		+ SOverlay::Slot()[CoucheRunes()]
		+ SOverlay::Slot()[CoucheMarchand()]
		+ SOverlay::Slot()[CoucheEvenement()]
		+ SOverlay::Slot()[CoucheDialogue()]
		+ SOverlay::Slot()[CoucheNouvelActe()]
		+ SOverlay::Slot()[SNew(SVespMenu).Joueur(Joueur)]
		+ SOverlay::Slot()[CoucheFin()]
		+ SOverlay::Slot()[CoucheTitre()]
	];
}

bool SVespInterface::EnPhase(uint8 LaPhase) const
{
	return Joueur.IsValid() && (uint8)Joueur->Phase == LaPhase;
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
	TSharedRef<SButton> Bouton = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
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
	TSharedRef<SButton> Bouton = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
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
	TSharedRef<SButton> Bouton = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton).Visibility(Visible).IsEnabled(Actif)
	.OnClicked_Lambda([Clic]() { Clic(); return FReply::Handled(); })
	[
		SNew(SBox).WidthOverride(Largeur).HeightOverride(Hauteur)
		[
			SNew(SBorder).Padding(FMargin(20, 18))
			.BorderImage_Lambda([this, Lien, Numero]() {
				const TSharedPtr<SButton> B = Lien->Pin();
				const bool bChoisie = Joueur.IsValid() && Joueur->bSelectionVisible && Joueur->SelectionMenu == Numero;
				return (B.IsValid() && B->IsHovered()) || bChoisie ? &GrandeCarteSurvol : &GrandeCarte;
			})
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
			TEXT("SE DEPLACER"),
			TEXT("ZQSD / WASD / fleches / stick gauche : marcher      souris / stick droit : viser"),
			TEXT("TAB / Select : la carte du monde      E / B : parler, ouvrir"),
			TEXT("COMBATTRE (en temps reel)"),
			TEXT("Clic gauche, J / X : attaquer (trois coups s'enchainent)"),
			TEXT("Clic droit, K / Y : attaque lourde (brise les armures)"),
			TEXT("ESPACE / A : esquiver (AYLIS traverse les coups)"),
			TEXT("MAJ, F / gachette gauche (maintenue) : la garde. Au dernier moment : PARADE"),
			TEXT("R / croix haut : potion      V / RB : attaque speciale (rage pleine)"),
			TEXT("Les taches rouges au sol annoncent un coup : sors-en, ou esquive au travers."),
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
		SNew(STextBlock).Text(LOCTEXT("Entree", "ENTREE  /  (A)  pour commencer")).Font(Police("Bold", 12, 300))
		.ColorAndOpacity_Lambda([]() { return FSlateColor(FLinearColor(OR.R, OR.G, OR.B, 0.35f + 0.65f * Pulsation(2.5f))); })
	]
	+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 40, 30))
	[
		SNew(STextBlock).Text(LOCTEXT("Mention", "BlueMoon Interactive  -  2027  -  en developpement, susceptible de changer"))
		.Font(Police("Regular", 10)).ColorAndOpacity(FLinearColor(0.66f, 0.63f, 0.75f, 0.6f))
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
			default: return J->bGardeLevee;
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
					      return Joueur->Rage >= 100 ? LOCTEXT("RagePleine", "RAGE PLEINE  -  V / RB") : Texte(FString::Printf(TEXT("RAGE  %d%%"), Joueur->Rage));
				      }), 300, 12)
			]
			// L'experience : le niveau, et la barre vers le suivant
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				Jauge(TAttribute<float>::CreateLambda([this]() {
					      const AVespPlayerController* J = Joueur.Get();
					      return J ? (float)J->Xp / FMath::Max(1, J->XpPourNiveau(J->Niveau)) : 0.0f;
				      }),
				      FSlateColor(FLinearColor(0.55f, 0.45f, 1.0f)),
				      TAttribute<FText>::CreateLambda([this]() {
					      const AVespPlayerController* J = Joueur.Get();
					      return J ? Texte(FString::Printf(TEXT("NIVEAU %d  -  %d / %d XP"), J->Niveau, J->Xp, J->XpPourNiveau(J->Niveau))) : FText::GetEmpty();
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

// ===================== Les pouvoirs du Seuil =====================

TSharedRef<SWidget> SVespInterface::CasePouvoir(int32 N)
{
	auto Etoile = [this, N]() { return Joueur.IsValid() ? Joueur->EtoileDuPouvoir(N) : -1; };
	auto Appris = [this, Etoile]() { const int32 E = Etoile(); return E >= 0 && Joueur.IsValid() && Joueur->EtoileAcquise(E); };
	static const TCHAR* TOUCHES[4] = {TEXT(""), TEXT("1  /  LB"), TEXT("2  /  RT"), TEXT("3  /  >")};
	return SNew(SBox).WidthOverride(76)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(58).HeightOverride(58)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(&Rond).Padding(2)
					.BorderBackgroundColor_Lambda([Etoile, Appris]() {
						const int32 E = Etoile();
						return FSlateColor(Appris() && E >= 0 ? VespSeuil::CouleurVoie(VespSeuil::Etoile(E).Voie) : FLinearColor(0.25f, 0.22f, 0.35f, 0.6f));
					})
					[
						SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0.05f, 0.04f, 0.09f, 0.95f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Font(Police("Bold", 20))
							.ColorAndOpacity_Lambda([Appris]() { return FSlateColor(Appris() ? TEXTE : FLinearColor(0.35f, 0.32f, 0.45f)); })
							.Text_Lambda([Etoile]() { const int32 E = Etoile(); return E >= 0 ? Texte(FString(VespSeuil::Etoile(E).Nom).Left(1)) : FText::GetEmpty(); })
						]
					]
				]
				// La recharge : un voile sombre qui descend
				+ SOverlay::Slot().VAlign(VAlign_Bottom)
				[
					SNew(SBox).HeightOverride_Lambda([this, N, Appris]() {
						return FOptionalSize(Appris() && Joueur.IsValid() ? 58.0f * Joueur->RechargeDuPouvoir(N) : 0.0f);
					})
					[
						SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f))
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 0)
		[
			SNew(STextBlock).Text(Texte(TOUCHES[FMath::Clamp(N, 0, 3)])).Font(Police("Bold", 9, 100)).ColorAndOpacity(DOUX)
		]
	];
}

// ===================== Le boss, et la clairiere scellee =====================

TSharedRef<SWidget> SVespInterface::BarreDuBoss()
{
	auto Boss = [this]() -> AVespUnite* { return Joueur.IsValid() && Joueur->Combat ? Joueur->Combat->BossActif() : nullptr; };
	auto Part = [Boss]() { const AVespUnite* B = Boss(); return B ? (float)B->Stats.Pv / FMath::Max(1, B->Stats.PvMax) : 0.0f; };
	return SNew(SVerticalBox).Visibility_Lambda([this, Boss]() { return VisibleSi(Boss() != nullptr); })
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 6)
	[
		SNew(STextBlock).Font(Police("Bold", 16, 300)).ColorAndOpacity(OR_PALE)
		.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.8f))
		.Text_Lambda([Boss]() {
			const AVespUnite* B = Boss();
			return B ? Texte(B->Stats.Nom.ToUpper() + (B->bPhaseDeux ? TEXT("  -  EN RAGE") : TEXT(""))) : FText::GetEmpty();
		})
	]
	+ SVerticalBox::Slot().AutoHeight()
	[
		Jauge(TAttribute<float>::CreateLambda(Part),
		      TAttribute<FSlateColor>::CreateLambda([Boss]() {
			      const AVespUnite* B = Boss();
			      return FSlateColor(B && B->bPhaseDeux ? FLinearColor(0.95f, 0.35f, 0.2f) : FLinearColor(0.9f, 0.62f, 0.25f));
		      }),
		      TAttribute<FText>::CreateLambda([Boss]() {
			      const AVespUnite* B = Boss();
			      if (!B) return FText::GetEmpty();
			      FString T = FString::Printf(TEXT("%d / %d"), FMath::Max(0, B->Stats.Pv), B->Stats.PvMax);
			      if (B->Stats.ArmureMax > 0) T += FString::Printf(TEXT("   -   armure %d / %d"), B->Stats.Armure, B->Stats.ArmureMax);
			      return Texte(T);
		      }), 640, 18)
	];
}

TSharedRef<SWidget> SVespInterface::BandeauClairiere()
{
	auto Scellee = [this]() {
		return Joueur.IsValid() && Joueur->Combat && Joueur->Combat->GroupeFerme() && !Joueur->Combat->BossActif();
	};
	return SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0.55f, 0.35f, 1.0f, 0.22f)).Padding(FMargin(22, 7))
	.Visibility_Lambda([this, Scellee]() { return VisibleSi(Scellee()); })
	[
		SNew(STextBlock).Font(Police("Bold", 12, 250)).ColorAndOpacity(VIOLET)
		.Text_Lambda([this]() {
			if (!Joueur.IsValid() || !Joueur->Combat) return FText::GetEmpty();
			const FVespGroupe* G = Joueur->Combat->GroupeFerme();
			const int32 Restants = Joueur->Combat->HaschenEngages();
			FString T = FString::Printf(TEXT("CLAIRIERE SCELLEE  -  %d HASCHEN"), Restants);
			if (G && G->VaguesRestantes > 0) T += FString::Printf(TEXT("  -  %d VAGUE%s A VENIR"), G->VaguesRestantes, G->VaguesRestantes > 1 ? TEXT("S") : TEXT(""));
			return Texte(T);
		})
	];
}

// ===================== L'exploration =====================

TSharedRef<SWidget> SVespInterface::CoucheExploration()
{
	auto Faites = [this]() {
		const AVespPlayerController* J = Joueur.Get();
		int32 Nombre = 0, Total = 0;
		if (J)
		{
			for (const FVespNoeud& N : J->Noeuds)
			{
				if (N.Type != EVespSalle::Depart)
				{
					Total++;
					Nombre += N.bVisite ? 1 : 0;
				}
			}
		}
		return FString::Printf(TEXT("Clairieres visitees : %d / %d"), Nombre, Total);
	};
	return SNew(SOverlay).Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Exploration)); })
	// En haut a gauche : ou l'on est
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24)
	[
		SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(20, 14))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Bold", 10, 260)).ColorAndOpacity(OR)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(TEXT("ACTE ") + AVespPlayerController::Romain(Joueur->Acte) + TEXT("  -  ") + Joueur->NomDeLActe().ToUpper()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
			[
				SNew(STextBlock).Font(Police("Bold", 20)).ColorAndOpacity(TEXTE)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->NomDuLieu()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Italic", 12)).ColorAndOpacity(DOUX)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(TEXT("Vers l'est : ") + Joueur->NomDuBoss()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
			[
				SNew(STextBlock).Font(Police("Regular", 11)).ColorAndOpacity(DOUX).Text_Lambda([Faites]() { return Texte(Faites()); })
			]
		]
	]
	// En bas a gauche : AYLIS
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24)[FicheAylis()]
	// Au centre, en bas : ce qu'on peut faire ici, puis les commandes
	+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 0, 28))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 12)
		[
			SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0.45f, 0.8f, 0.95f, 0.22f)).Padding(FMargin(22, 8))
			.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && !Joueur->Invite().IsEmpty()); })
			[
				SNew(STextBlock).Font(Police("Bold", 14, 120)).ColorAndOpacity(BRUME)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->Invite()) : FText::GetEmpty(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(Police("Regular", 11)).ColorAndOpacity(FLinearColor(0.66f, 0.63f, 0.75f, 0.75f))
			.Text(LOCTEXT("CommandesExplo", "clic gauche : attaquer     clic droit : lourde     ESPACE : esquiver     MAJ : garde     R : potion     V : special     TAB : carte"))
		]
	]
	// En bas a droite : les trois pouvoirs du Seuil (et leur recharge)
	+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 30, 30))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)[CasePouvoir(1)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)[CasePouvoir(2)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)[CasePouvoir(3)]
	]
	// A droite : l'objet qu'on vient de ramasser
	+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(FMargin(0, 0, 30, 120))
	[
		SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(18, 12))
		.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->TempsButin > 0.0f); })
		.ColorAndOpacity_Lambda([this]() { return FLinearColor(1, 1, 1, Joueur.IsValid() ? FMath::Clamp(Joueur->TempsButin, 0.0f, 1.0f) : 0.0f); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("ObjetTrouve", "OBJET TROUVE  -  I : inventaire")).Font(Police("Bold", 10, 250)).ColorAndOpacity(DOUX)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
			[
				SNew(STextBlock).Font(Police("Bold", 16))
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(Joueur.IsValid() ? Joueur->CouleurDernierButin : TEXTE); })
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->DernierButin) : FText::GetEmpty(); })
			]
		]
	]
	// En haut au centre : la grande barre du boss, ou l'etat de la clairiere scellee
	+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0, 24, 0, 0))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[BarreDuBoss()]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[BandeauClairiere()]
	]
	// Un niveau de plus : un bandeau dore au milieu de l'ecran
	+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(0, 0, 0, 260))
	[
		SNew(SBorder).BorderImage(&Rien).Padding(0)
		.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->TempsNiveau > 0.0f); })
		.ColorAndOpacity_Lambda([this]() { return FLinearColor(1, 1, 1, Joueur.IsValid() ? FMath::Clamp(Joueur->TempsNiveau, 0.0f, 1.0f) : 0.0f); })
		[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(Police("Bold", 13, 500)).ColorAndOpacity(VIOLET).Text(LOCTEXT("LaVisionGrandit", "LA VISION GRANDIT"))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(Police("Bold", 46, 300)).ColorAndOpacity(OR_PALE)
			.ShadowOffset(FVector2D(0, 3)).ShadowColorAndOpacity(FLinearColor(0.5f, 0.3f, 0.0f, 0.8f))
			.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("NIVEAU %d"), Joueur->Niveau)) : FText::GetEmpty(); })
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(Police("Regular", 13)).ColorAndOpacity(TEXTE)
			.Text(LOCTEXT("GainsNiveau", "+5 pv max   +1 attaque   +1 point de competence"))
		]
		]
	]
	// Au milieu : le message qui vient de tomber (repos, marchand, rune...), qui s'efface
	+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0, 110, 0, 0))
	[
		SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0, 0, 0, 0.6f)).Padding(FMargin(24, 10))
		.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->TempsMessage > 0.0f && !Joueur->MessageRoute.IsEmpty()); })
		.ColorAndOpacity_Lambda([this]() { return FLinearColor(1, 1, 1, Joueur.IsValid() ? FMath::Clamp(Joueur->TempsMessage, 0.0f, 1.0f) : 0.0f); })
		[
			SNew(STextBlock).Font(Police("Italic", 15)).ColorAndOpacity(FLinearColor(0.6f, 0.95f, 0.66f))
			.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->MessageRoute) : FText::GetEmpty(); })
		]
	];
}

// ===================== La carte du monde (TAB) =====================

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
	return SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 0.85f)).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(EnPhase((uint8)EVespPhase::Exploration) && Joueur->bCarteOuverte); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			EnTete(TAttribute<FText>::CreateLambda([this]() {
				       return Joueur.IsValid() ? Texte(FString::Printf(TEXT("ACTE %s  -  %s"), *AVespPlayerController::Romain(Joueur->Acte), *Joueur->NomDeLActe().ToUpper())) : FText::GetEmpty();
			       }),
			       LOCTEXT("TitreCarte", "LA ROUTE DE LA VISION"),
			       LOCTEXT("SousCarte", "Le sentier principal mene au boss, a l'est. Les embranchements cachent des tresors... et des elites."))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(60, 18, 60, 0))
		[
			SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(10, 12))
			[
				SNew(SBox).HeightOverride(540)[CarteRoute.ToSharedRef()]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 16, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Combat)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Elite)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Evenement)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Marchand)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Repos)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Tresor)]
			+ SHorizontalBox::Slot().AutoWidth()[Legende(EVespSalle::Boss)]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("FermerCarte", "TAB / Select : fermer la carte")).Font(Police("Regular", 12)).ColorAndOpacity(DOUX)
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
			SNew(STextBlock).Text(LOCTEXT("AideChoix", "Clique sur une carte, tape son numero, ou choisis avec les fleches / la manette et valide (ENTREE / A)")).Font(Police("Regular", 12)).ColorAndOpacity(DOUX)
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
			GrandBouton(LOCTEXT("QuitterMarchand", "REPRENDRE LA ROUTE  (ECHAP / B)"), OR_PALE, [this]() { if (Joueur.IsValid()) Joueur->QuitterMarchand(); })
		]
	];
}

// ===================== Un evenement =====================

TSharedRef<SWidget> SVespInterface::CoucheEvenement()
{
	auto Choix = [this](int32 c) -> TSharedRef<SWidget> {
		TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
		TSharedRef<SButton> Bouton = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
		.OnClicked_Lambda([this, c]() { if (Joueur.IsValid()) Joueur->ChoisirEvenement(c); return FReply::Handled(); })
		[
			SNew(SBox).WidthOverride(360)
			[
				SNew(SBorder).Padding(FMargin(20, 14))
				.BorderImage_Lambda([this, Lien, c]() {
					const TSharedPtr<SButton> B = Lien->Pin();
					const bool bChoisie = Joueur.IsValid() && Joueur->bSelectionVisible && Joueur->SelectionMenu == c;
					return (B.IsValid() && B->IsHovered()) || bChoisie ? &CarteChoisie : &Carte;
				})
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
	return SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
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
				.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->Acte > 1 && Joueur->Runes.Num() + Joueur->HaschenVaincus > 0); })
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
					return J ? Texte(FString::Printf(TEXT("Acte %s, niveau %d, %d Haschen vaincus. Ce n'etait qu'un futur possible. La prophetie en montre d'autres."),
					                                 *AVespPlayerController::Romain(J->Acte), J->Niveau, J->HaschenVaincus))
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
