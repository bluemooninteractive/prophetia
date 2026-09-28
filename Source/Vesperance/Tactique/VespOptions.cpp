#include "VespOptions.h"
#include "VespPlayerController.h"
#include "VespSons.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Vesperance"

namespace
{
	const FLinearColor OR(0.91f, 0.75f, 0.33f, 1.0f);
	const FLinearColor TEXTE(0.94f, 0.91f, 0.98f, 1.0f);
	const FLinearColor DOUX(0.66f, 0.63f, 0.75f, 1.0f);
	const FLinearColor ROUGE(0.9f, 0.45f, 0.4f, 1.0f);

	FSlateFontInfo Police(const char* Style, int32 Taille, int32 Espacement = 0)
	{
		FSlateFontInfo P = FCoreStyle::GetDefaultFontStyle(Style, Taille);
		P.LetterSpacing = Espacement;
		return P;
	}
}

void SVespOptions::Construct(const FArguments& Args)
{
	Joueur = Args._Joueur;
	Panneau = FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.045f, 0.1f, 0.94f), 14.0f, FLinearColor(0.35f, 0.28f, 0.5f, 0.6f), 1.0f);
	Fond = FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.03f), 10.0f, FLinearColor(1, 1, 1, 0.08f), 1.0f);
	FondChoisi = FSlateRoundedBoxBrush(FLinearColor(0.91f, 0.75f, 0.33f, 0.14f), 10.0f, OR, 1.0f);
	Rond = FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.08f), 6.0f, FLinearColor(1, 1, 1, 0.2f), 1.0f);
	Rien = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 0.0f);
	StyleBouton = FButtonStyle().SetNormal(Rien).SetHovered(Rien).SetPressed(Rien).SetDisabled(Rien).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0, 1, 0, 0));

	const int32 Nombre = Args._bEnJeu ? AVespPlayerController::LignesReglages : AVespPlayerController::LignesReglagesTitre;
	TSharedRef<SVerticalBox> Lignes = SNew(SVerticalBox);
	for (int32 i = 0; i < Nombre; i++)
	{
		Lignes->AddSlot().AutoHeight().Padding(0, 3)[Ligne(i)];
	}
	ChildSlot
	[
		SNew(SBorder).BorderImage(&Panneau).Padding(FMargin(26, 20))
		[
			SNew(SBox).WidthOverride(540)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
				[
					SNew(STextBlock).Text(LOCTEXT("Options", "OPTIONS")).Font(Police("Bold", 12, 300)).ColorAndOpacity(OR)
				]
				+ SVerticalBox::Slot().AutoHeight()[Lignes]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
				[
					SNew(STextBlock).Font(Police("Regular", 10)).ColorAndOpacity(DOUX).AutoWrapText(true)
					.Text(Args._bEnJeu ? LOCTEXT("OptionsAideJeu", "Haut / bas : choisir  ·  gauche / droite : régler  ·  ENTRÉE / (A) : valider  ·  TAB : onglet  ·  I / (B) : reprendre")
					                   : LOCTEXT("OptionsAideTitre", "Haut / bas : choisir  ·  gauche / droite : régler  ·  ENTRÉE / (A) : valider  ·  ÉCHAP / (B) : fermer"))
				]
			]
		]
	];
}

TSharedRef<SWidget> SVespOptions::PetitBouton(const FText& Libelle, int32 Numero, int32 Sens)
{
	return SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
	.OnClicked_Lambda([this, Numero, Sens]() {
		if (AVespPlayerController* J = Joueur.Get())
		{
			J->SelectionReglage = Numero;
			J->ChangerReglage(Numero, Sens);
		}
		return FReply::Handled();
	})
	[
		SNew(SBox).WidthOverride(30).HeightOverride(30).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBorder).BorderImage(&Rond).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(8, 2))
			[
				SNew(STextBlock).Text(Libelle).Font(Police("Bold", 14)).ColorAndOpacity(TEXTE)
			]
		]
	];
}

TSharedRef<SWidget> SVespOptions::Ligne(int32 Numero)
{
	const bool bVolume = Numero <= 1;
	const bool bAction = Numero >= 4;
	TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
	TSharedRef<SHorizontalBox> Contenu = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(AVespPlayerController::NomReglage(Numero))).Font(Police("Bold", 13, 80))
			.ColorAndOpacity(Numero == 4 ? ROUGE : (bAction ? DOUX : TEXTE))
		];
	if (bVolume)
	{
		Contenu->AddSlot().AutoWidth().VAlign(VAlign_Center)[PetitBouton(FText::FromString(TEXT("-")), Numero, -1)];
	}
	Contenu->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
	[
		SNew(SBox).MinDesiredWidth(bAction ? 0.0f : 84.0f)
		[
			SNew(STextBlock).Justification(ETextJustify::Center).Font(Police("Bold", bAction ? 10 : 14)).ColorAndOpacity(bAction ? ROUGE : OR)
			.Text_Lambda([this, Numero]() { const AVespPlayerController* J = Joueur.Get(); return J ? FText::FromString(J->ValeurReglage(Numero)) : FText::GetEmpty(); })
		]
	];
	if (bVolume)
	{
		Contenu->AddSlot().AutoWidth().VAlign(VAlign_Center)[PetitBouton(FText::FromString(TEXT("+")), Numero, 1)];
	}
	TSharedRef<SButton> Bouton = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
	.OnClicked_Lambda([this, Numero]() {
		if (AVespPlayerController* J = Joueur.Get())
		{
			J->SelectionReglage = Numero;
			J->ChangerReglage(Numero, 0);
		}
		return FReply::Handled();
	})
	.OnHovered_Lambda([this, Numero]() {
		if (AVespPlayerController* J = Joueur.Get())
		{
			if (J->SelectionReglage != Numero)
			{
				J->SelectionReglage = Numero;
				UVespSons::Jouer2D(J, EVespSon::Survol);
			}
		}
	})
	[
		SNew(SBorder).Padding(FMargin(16, 9))
		.BorderImage_Lambda([this, Numero]() { const AVespPlayerController* J = Joueur.Get(); return J && J->SelectionReglage == Numero ? &FondChoisi : &Fond; })
		[
			Contenu
		]
	];
	*Lien = Bouton;
	return Bouton;
}

#undef LOCTEXT_NAMESPACE
