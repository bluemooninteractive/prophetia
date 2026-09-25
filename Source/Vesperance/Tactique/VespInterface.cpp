#include "VespInterface.h"
#include "VespPlayerController.h"
#include "VespUnite.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Vesperance"

// ===================== La palette (la meme que les planches et le press kit) =====================
namespace
{
	const FLinearColor NUIT(0.012f, 0.009f, 0.02f, 1.0f);
	const FLinearColor OR(0.91f, 0.75f, 0.33f, 1.0f);
	const FLinearColor OR_PALE(1.0f, 0.89f, 0.6f, 1.0f);
	const FLinearColor VIOLET(0.73f, 0.55f, 1.0f, 1.0f);
	const FLinearColor BRUME(0.56f, 0.89f, 0.85f, 1.0f);
	const FLinearColor TEXTE(0.94f, 0.91f, 0.98f, 1.0f);
	const FLinearColor DOUX(0.66f, 0.63f, 0.75f, 1.0f);
	const FLinearColor ROUGE(0.9f, 0.32f, 0.3f, 1.0f);
	const FLinearColor VERT(0.4f, 0.82f, 0.5f, 1.0f);
	const FLinearColor BLEU_AYLIS(0.45f, 0.62f, 1.0f, 1.0f);

	// Une police : Roboto (fournie avec Unreal), avec un style ("Regular", "Bold", "Black", "Light"),
	// une taille, et un espacement des lettres (en millièmes de la taille : 200 = des lettres bien espacees)
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
}

// ===================== La construction =====================

void SVespInterface::Construct(const FArguments& Args)
{
	Joueur = Args._Joueur;

	Panneau = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.028f, 0.06f, 0.86f), 16.0f, FLinearColor(0.24f, 0.21f, 0.35f, 0.9f), 1.0f);
	PanneauFonce = FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.016f, 0.035f, 0.92f), 12.0f, FLinearColor(0.2f, 0.18f, 0.3f, 1.0f), 1.0f);
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
		+ SOverlay::Slot()[CoucheChoix()]
		+ SOverlay::Slot()[CoucheDialogue()]
		+ SOverlay::Slot()[CoucheNouvelActe()]
		+ SOverlay::Slot()[CoucheFin()]
	];
}

bool SVespInterface::EnCombat() const
{
	const AVespPlayerController* J = Joueur.Get();
	return J && (J->Phase == EVespPhase::TourAylis || J->Phase == EVespPhase::TourHaschen);
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
			SNew(SBox).WidthOverride_Lambda([Part, Largeur]() { return FMath::Clamp(Part.Get(), 0.0f, 1.0f) * Largeur; })
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
	TSharedPtr<SButton> Bouton;
	SAssignNew(Bouton, SButton).ButtonStyle(&StyleBouton).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
	[
		SNew(SBorder).Padding(FMargin(34, 12))
		.BorderImage_Lambda([this, Bouton]() { return Bouton.IsValid() && Bouton->IsHovered() ? &CarteChoisie : &Carte; })
		[
			SNew(STextBlock).Text(Libelle).Font(Police("Bold", 16, 120)).ColorAndOpacity(Couleur)
		]
	];
	return Bouton.ToSharedRef();
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

// En haut a gauche : l'acte, le lieu, et le journal des dernieres actions
TSharedRef<SWidget> SVespInterface::CarteDuLieu()
{
	TSharedRef<SVerticalBox> Journal = SNew(SVerticalBox);
	for (int32 i = 0; i < 3; i++)
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
				if (L.Contains(TEXT("touche AYLIS")) || L.Contains(TEXT("Poison")) || L.Contains(TEXT("Malefice"))) C = FLinearColor(1.0f, 0.55f, 0.52f);
				else if (L.Contains(TEXT("tombe")) || L.Contains(TEXT("succombe"))) C = FLinearColor(0.55f, 0.9f, 0.6f);
				else if (L.Contains(TEXT("!"))) C = OR_PALE;
				C.A = 0.45f + 0.55f * (i + 1) / FMath::Max(1, J->Journal.Num());
				return FSlateColor(C);
			})
		];
	}
	return SNew(SBox).WidthOverride(380)
	[
		SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(20, 16))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Bold", 10, 260)).ColorAndOpacity(OR)
				.Text_Lambda([this]() {
					const AVespPlayerController* J = Joueur.Get();
					return J ? Texte(FString::Printf(TEXT("ACTE %s  -  SALLE %d / %d"), J->Acte == 1 ? TEXT("I") : TEXT("II"), J->Salle, AVespPlayerController::NombreDeSalles))
					         : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 10)
			[
				SNew(STextBlock).Font(Police("Bold", 20)).ColorAndOpacity(TEXTE)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->NomDuLieu()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(FLinearColor(0.3f, 0.27f, 0.42f, 0.8f))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[Journal]
		]
	];
}

// En haut au centre : a qui c'est de jouer
TSharedRef<SWidget> SVespInterface::PastilleDuTour()
{
	auto CouleurTour = [this]() {
		const AVespPlayerController* J = Joueur.Get();
		return J && J->Phase == EVespPhase::TourAylis ? VERT : ROUGE;
	};
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(SBorder).BorderImage(&Rond).Padding(FMargin(26, 8))
		.BorderBackgroundColor_Lambda([CouleurTour]() { FLinearColor C = CouleurTour(); C.A = 0.22f; return FSlateColor(C); })
		[
			SNew(STextBlock).Font(Police("Black", 14, 220))
			.ColorAndOpacity_Lambda([CouleurTour]() { return FSlateColor(CouleurTour()); })
			.Text_Lambda([this]() {
				const AVespPlayerController* J = Joueur.Get();
				return J && J->Phase == EVespPhase::TourAylis ? LOCTEXT("TonTour", "TON TOUR") : LOCTEXT("TourHaschen", "TOUR DES HASCHEN");
			})
		]
	]
	+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
	[
		SNew(STextBlock).Font(Police("Regular", 11, 120)).ColorAndOpacity(DOUX)
		.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("Tour %d"), Joueur->Tour)) : FText::GetEmpty(); })
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
						SNew(STextBlock).Text(LOCTEXT("InitialeA", "A")).Font(Police("Black", 34)).ColorAndOpacity(FLinearColor(0.8f, 0.86f, 1.0f))
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
					SNew(STextBlock).Text(LOCTEXT("Aylis", "AYLIS")).Font(Police("Black", 18, 180)).ColorAndOpacity(BLEU_AYLIS)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(10, 0, 0, 2)
				[
					SNew(STextBlock).Text(LOCTEXT("Voie", "voie de l'epee")).Font(Police("Italic", 10)).ColorAndOpacity(DOUX)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				Jauge(TAttribute<float>::CreateLambda(Pv),
				      TAttribute<FSlateColor>::CreateLambda([Pv]() { return FSlateColor(CouleurVie(Pv())); }),
				      TAttribute<FText>::CreateLambda([this]() {
					      const AVespPlayerController* J = Joueur.Get();
					      return J && J->Aylis ? Texte(FString::Printf(TEXT("%d / %d  PV"), FMath::Max(0, J->Aylis->Stats.Pv), J->Aylis->Stats.PvMax)) : FText::GetEmpty();
				      }), 290, 20)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				Jauge(TAttribute<float>::CreateLambda([this]() { return Joueur.IsValid() ? Joueur->Rage / 100.0f : 0.0f; }),
				      TAttribute<FSlateColor>::CreateLambda([this]() {
					      const bool bPleine = Joueur.IsValid() && Joueur->Rage >= 100;
					      const float P = 0.5f + 0.5f * FMath::Sin(FPlatformTime::Seconds() * 8.0);
					      return FSlateColor(bPleine ? FLinearColor(1.0f, 0.45f + 0.3f * P, 0.15f) : FLinearColor(0.75f, 0.25f, 0.2f));
				      }),
				      TAttribute<FText>::CreateLambda([this]() {
					      if (!Joueur.IsValid()) return FText::GetEmpty();
					      return Joueur->Rage >= 100 ? LOCTEXT("RagePleine", "RAGE PLEINE  -  touche 5") : Texte(FString::Printf(TEXT("RAGE  %d%%"), Joueur->Rage));
				      }), 290, 12)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(TAttribute<FText>::CreateLambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("POTIONS  %d"), Joueur->Potions)) : FText::GetEmpty(); }),
					         FLinearColor(0.95f, 0.5f, 0.5f), EVisibility::Visible)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(TAttribute<FText>::CreateLambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("RUNES  %d"), Joueur->Runes.Num())) : FText::GetEmpty(); }),
					         VIOLET, EVisibility::Visible)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					Pastille(LOCTEXT("Poison", "POISON"), FLinearColor(0.7f, 0.95f, 0.4f),
					         TAttribute<EVisibility>::CreateLambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->Aylis && Joueur->Aylis->Poison > 0); }))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					Pastille(LOCTEXT("Garde", "EN GARDE"), BRUME,
					         TAttribute<EVisibility>::CreateLambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->bEnGarde); }))
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
		.IsEnabled_Lambda([this]() { return Joueur.IsValid() && Joueur->Phase == EVespPhase::TourAylis; })
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
		.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->Phase == EVespPhase::TourAylis); })
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

// ===================== Les choix de la route : les runes, ou les salles de la vision =====================

TSharedRef<SWidget> SVespInterface::CarteDeChoix(int32 Numero)
{
	auto Nombre = [this]() {
		const AVespPlayerController* J = Joueur.Get();
		if (!J) return 0;
		return J->Phase == EVespPhase::ChoixRune ? J->RunesProposees.Num() : J->Propositions.Num();
	};
	auto Couleur = [this, Numero]() -> FLinearColor {
		const AVespPlayerController* J = Joueur.Get();
		if (!J || J->Phase == EVespPhase::ChoixRune || !J->Propositions.IsValidIndex(Numero)) return VIOLET;
		switch (J->Propositions[Numero])
		{
			case EVespSalle::Combat: return FLinearColor(0.92f, 0.42f, 0.36f);
			case EVespSalle::Elite: return OR;
			default: return FLinearColor(0.48f, 0.85f, 0.52f);
		}
	};
	TSharedPtr<SButton> Bouton;
	SAssignNew(Bouton, SButton).ButtonStyle(&StyleBouton)
	.Visibility_Lambda([this, Nombre, Numero]() { return VisibleSi(Numero < Nombre()); })
	.OnClicked_Lambda([this, Numero]() {
		if (Joueur.IsValid())
		{
			Joueur->Phase == EVespPhase::ChoixRune ? Joueur->ChoisirRune(Numero) : Joueur->ChoisirSalle(Numero);
		}
		return FReply::Handled();
	})
	[
		SNew(SBox).WidthOverride(270).HeightOverride(340)
		[
			SNew(SBorder).Padding(FMargin(22, 20))
			.BorderImage_Lambda([this, Numero]() { return BoutonsChoix.IsValidIndex(Numero) && BoutonsChoix[Numero]->IsHovered() ? &GrandeCarteSurvol : &GrandeCarte; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Texte(FString::FromInt(Numero + 1))).Font(Police("Bold", 13)).ColorAndOpacity(DOUX)
				]
				// Le medaillon : un disque lumineux de la couleur de la carte, avec l'initiale
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 18)
				[
					SNew(SBox).WidthOverride(112).HeightOverride(112)
					[
						SNew(SBorder).BorderImage(&Rond).Padding(4)
						.BorderBackgroundColor_Lambda([Couleur]() { FLinearColor C = Couleur(); C.A = 0.25f; return FSlateColor(C); })
						[
							SNew(SBorder).BorderImage(&Rond).HAlign(HAlign_Center).VAlign(VAlign_Center)
							.BorderBackgroundColor_Lambda([Couleur]() { return FSlateColor(Couleur() * FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)); })
							[
								SNew(STextBlock).Font(Police("Black", 44))
								.ColorAndOpacity_Lambda([Couleur]() { return FSlateColor(Couleur()); })
								.Text_Lambda([this, Numero]() {
									const AVespPlayerController* J = Joueur.Get();
									if (!J) return FText::GetEmpty();
									const FString Nom = J->Phase == EVespPhase::ChoixRune
										? (J->RunesProposees.IsValidIndex(Numero) ? AVespPlayerController::NomRune(J->RunesProposees[Numero]) : FString())
										: (J->Propositions.IsValidIndex(Numero) ? AVespPlayerController::NomSalle(J->Propositions[Numero], J->Acte) : FString());
									return Texte(Nom.Left(1));
								})
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Font(Police("Bold", 22)).Justification(ETextJustify::Center)
					.ColorAndOpacity_Lambda([Couleur]() { return FSlateColor(Couleur()); })
					.Text_Lambda([this, Numero]() {
						const AVespPlayerController* J = Joueur.Get();
						if (!J) return FText::GetEmpty();
						if (J->Phase == EVespPhase::ChoixRune)
							return J->RunesProposees.IsValidIndex(Numero) ? Texte(AVespPlayerController::NomRune(J->RunesProposees[Numero])) : FText::GetEmpty();
						return J->Propositions.IsValidIndex(Numero) ? Texte(AVespPlayerController::NomSalle(J->Propositions[Numero], J->Acte)) : FText::GetEmpty();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 12, 0, 0)
				[
					SNew(STextBlock).Font(Police("Regular", 13)).ColorAndOpacity(TEXTE).AutoWrapText(true).Justification(ETextJustify::Center)
					.Text_Lambda([this, Numero]() {
						const AVespPlayerController* J = Joueur.Get();
						if (!J) return FText::GetEmpty();
						if (J->Phase == EVespPhase::ChoixRune)
							return J->RunesProposees.IsValidIndex(Numero) ? Texte(AVespPlayerController::AideRune(J->RunesProposees[Numero])) : FText::GetEmpty();
						return J->Propositions.IsValidIndex(Numero) ? Texte(AVespPlayerController::AideSalle(J->Propositions[Numero])) : FText::GetEmpty();
					})
				]
			]
		]
	];
	if (BoutonsChoix.Num() <= Numero)
	{
		BoutonsChoix.SetNum(Numero + 1);
	}
	BoutonsChoix[Numero] = Bouton;
	return Bouton.ToSharedRef();
}

TSharedRef<SWidget> SVespInterface::CoucheChoix()
{
	auto bRunes = [this]() { return Joueur.IsValid() && Joueur->Phase == EVespPhase::ChoixRune; };
	TSharedRef<SHorizontalBox> Cartes = SNew(SHorizontalBox);
	for (int32 i = 0; i < 3; i++)
	{
		Cartes->AddSlot().AutoWidth().Padding(14, 0)[CarteDeChoix(i)];
	}
	return SNew(SBorder).BorderImage(&Voile).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && (Joueur->Phase == EVespPhase::ChoixRune || Joueur->Phase == EVespPhase::ChoixSalle)); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(Police("Bold", 11, 400)).ColorAndOpacity(VIOLET)
			.Text_Lambda([bRunes]() { return bRunes() ? LOCTEXT("TitreRunes1", "LA PROPHETIE T'OFFRE") : LOCTEXT("TitreSalles1", "LA VISION MONTRE"); })
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 8)
		[
			SNew(STextBlock).Font(Police("Black", 40, 120)).ColorAndOpacity(TEXTE)
			.Text_Lambda([bRunes]() { return bRunes() ? LOCTEXT("TitreRunes2", "UNE RUNE") : LOCTEXT("TitreSalles2", "LA SUITE DE LA ROUTE"); })
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 6)
		[
			SNew(STextBlock).Font(Police("Regular", 14)).ColorAndOpacity(DOUX)
			.Text_Lambda([this, bRunes]() {
				if (!Joueur.IsValid()) return FText::GetEmpty();
				return bRunes() ? LOCTEXT("AideRunes", "AYLIS la garde jusqu'a la fin de la route")
				                : Texte(FString::Printf(TEXT("%s  -  salle %d / %d"), *Joueur->NomDeLActe(), Joueur->Salle, AVespPlayerController::NombreDeSalles));
			})
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 34)
		[
			SNew(STextBlock).Font(Police("Italic", 13)).ColorAndOpacity(FLinearColor(0.55f, 0.9f, 0.62f))
			.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(Joueur->MessageRoute) : FText::GetEmpty(); })
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Cartes]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 30, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("AideChoix", "Clique sur une carte, ou tape son numero")).Font(Police("Regular", 12)).ColorAndOpacity(DOUX)
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
	auto CouleurOrateur = [Orateur]() { return Orateur() == TEXT("AYLIS") ? BLEU_AYLIS : (Orateur() == TEXT("Skarn") ? FLinearColor(1.0f, 0.45f, 0.3f) : FLinearColor(0.45f, 0.9f, 0.5f)); };
	return SNew(SButton).ButtonStyle(&StyleBouton)
	.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->Phase == EVespPhase::Dialogue); })
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
						SNew(STextBlock).Font(Police("Black", 18, 140)).ColorAndOpacity(FLinearColor(1.0f, 0.95f, 0.87f))
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
	return SNew(SBorder).BorderImage(&Voile).BorderBackgroundColor(FLinearColor(1, 1, 1, 1)).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && Joueur->Phase == EVespPhase::NouvelActe); })
	.ColorAndOpacity_Lambda([this]() { return FLinearColor(1, 1, 1, Joueur.IsValid() ? FMath::Clamp(Joueur->TempsPhase, 0.0f, 1.0f) : 1.0f); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("ActeII", "ACTE II")).Font(Police("Black", 72, 300)).ColorAndOpacity(FLinearColor(0.88f, 0.8f, 1.0f))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 18)
		[
			SNew(STextBlock).Text(LOCTEXT("TerresHantees", "Les Terres Hantees")).Font(Police("Bold", 30)).ColorAndOpacity(OR)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("LieuxII", "Le Bois des Pendus   -   la clairiere de la Matriarche")).Font(Police("Regular", 15)).ColorAndOpacity(DOUX)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 34, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("ForcesII", "AYLIS reprend des forces : tous les pv, et une potion.")).Font(Police("Italic", 15)).ColorAndOpacity(FLinearColor(0.55f, 0.9f, 0.62f))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 40, 0, 0)
		[
			GrandBouton(LOCTEXT("Continuer", "REPRENDRE LA ROUTE"), OR_PALE, [this]() { if (Joueur.IsValid()) Joueur->ContinuerApresLActe(); })
		]
	];
}

// ===================== La fin : victoire, ou la vision se brise =====================

TSharedRef<SWidget> SVespInterface::CoucheFin()
{
	auto bVictoire = [this]() { return Joueur.IsValid() && Joueur->Phase == EVespPhase::Victoire; };
	return SNew(SBorder).BorderImage(&Voile).HAlign(HAlign_Center).VAlign(VAlign_Center)
	.Visibility_Lambda([this]() { return VisibleSi(Joueur.IsValid() && (Joueur->Phase == EVespPhase::Victoire || Joueur->Phase == EVespPhase::Defaite)); })
	.BorderBackgroundColor_Lambda([bVictoire]() { return bVictoire() ? FLinearColor(0.4f, 0.4f, 0.4f, 0.7f) : FLinearColor(0.7f, 0.4f, 1.0f, 0.85f); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(Police("Black", 56, 160))
			.ColorAndOpacity_Lambda([bVictoire]() { return FSlateColor(bVictoire() ? OR_PALE : FLinearColor(0.85f, 0.75f, 1.0f)); })
			.Text_Lambda([bVictoire]() { return bVictoire() ? LOCTEXT("Victoire", "LA MATRIARCHE EST TOMBEE") : LOCTEXT("Defaite", "LA VISION SE BRISE..."); })
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 40)
		[
			SNew(STextBlock).Font(Police("Italic", 17)).ColorAndOpacity(DOUX)
			.Text_Lambda([bVictoire]() {
				return bVictoire() ? LOCTEXT("VictoireSuite", "Les Terres Hantees sont libres. La route continue vers la Marche d'Ashka...")
				                   : LOCTEXT("DefaiteSuite", "Ce n'etait qu'un futur possible. La prophetie en montre d'autres.");
			})
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			GrandBouton(LOCTEXT("Recommencer", "UNE NOUVELLE VISION  (R)"), OR_PALE, [this]() { if (Joueur.IsValid()) Joueur->Recommencer(); })
		]
	];
}

#undef LOCTEXT_NAMESPACE
