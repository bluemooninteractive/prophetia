#include "VespMenu.h"
#include "VespTexte.h"
#include "VespPlayerController.h"
#include "VespProgression.h"
#include "VespUnite.h"
#include "VespSons.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"

#define LOCTEXT_NAMESPACE "VesperanceMenu"

// La palette des maquettes
namespace
{
	const FLinearColor FOND(0.027f, 0.02f, 0.05f, 0.98f);
	const FLinearColor OR(0.886f, 0.725f, 0.36f, 1.0f);
	const FLinearColor TEXTE(0.94f, 0.91f, 0.97f, 1.0f);
	const FLinearColor DOUX(0.56f, 0.53f, 0.65f, 1.0f);
	const FLinearColor TRES_DOUX(0.43f, 0.4f, 0.52f, 1.0f);
	const FLinearColor VIOLET(0.69f, 0.55f, 1.0f, 1.0f);
	const FLinearColor LIGNE(0.23f, 0.18f, 0.34f, 1.0f);

	FSlateFontInfo Police(const char* Style, int32 Taille, int32 Espacement = 0)
	{
		FSlateFontInfo P = FCoreStyle::GetDefaultFontStyle(Style, Taille);
		P.LetterSpacing = Espacement;
		return P;
	}
	FText Texte(const FString& S) { return FText::FromString(S); }
	float Horloge() { return (float)FMath::Fmod(FPlatformTime::Seconds(), 100000.0); }

	// L'anneau d'equipement : ou est chaque emplacement (angles des maquettes), autour de (380, 430), rayon 230
	const float ANGLES[6] = {-90.0f, -30.0f, 30.0f, 90.0f, 150.0f, 210.0f};
	FVector2D PositionEmplacement(int32 i)
	{
		const float A = FMath::DegreesToRadians(ANGLES[i]);
		return FVector2D(380.0f + FMath::Cos(A) * 230.0f, 430.0f + FMath::Sin(A) * 230.0f);
	}
	// L'icone d'un emplacement vide
	const int32 ICONES_VIDES[6] = {8, 10, 0, 7, 3, 9};
}

// ===================== Les icones au trait =====================

void SVespIcone::Construct(const FArguments& Args)
{
	Icone = Args._Icone;
	Couleur = Args._Couleur;
	Epaisseur = Args._Epaisseur;
}

static void Cercle(TArray<TArray<FVector2f>>& Traits, float Cx, float Cy, float R, int32 Segments = 16)
{
	TArray<FVector2f> C;
	for (int32 i = 0; i <= Segments; i++)
	{
		const float A = i * 2.0f * PI / Segments;
		C.Add(FVector2f(Cx + FMath::Cos(A) * R, Cy + FMath::Sin(A) * R));
	}
	Traits.Add(C);
}

int32 SVespIcone::OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
                          int32 Couche, const FWidgetStyle& Style, bool bParentActif) const
{
	// Les traits dans une boite de 24 x 24 (comme les icones SVG des maquettes)
	TArray<TArray<FVector2f>> T;
	auto L = [&T](std::initializer_list<FVector2f> P) { T.Add(TArray<FVector2f>(P)); };
	switch (Icone.Get())
	{
		case 0:		// l'epee
			L({{19, 5}, {9, 15}}); L({{7, 13}, {11, 17}}); L({{8, 16}, {5, 19}}); L({{15, 5}, {19, 5}, {19, 9}});
			break;
		case 1:		// la dague
			L({{17, 7}, {10, 14}}); L({{8, 12}, {12, 16}}); L({{9, 15}, {6, 18}}); L({{14, 7}, {17, 7}, {17, 10}});
			break;
		case 3:		// le bouclier
			L({{12, 3}, {19, 6}, {19, 11}, {17, 16}, {12, 21}, {7, 16}, {5, 11}, {5, 6}, {12, 3}}); L({{12, 7}, {12, 17}});
			break;
		case 7:		// l'anneau
			Cercle(T, 12, 14, 6); L({{9, 5}, {12, 3}, {15, 5}, {12, 8}, {9, 5}});
			break;
		case 8:		// la capuche
			L({{4, 20}, {4, 13}, {6, 7}, {12, 3}, {18, 7}, {20, 13}, {20, 20}, {4, 20}}); L({{8, 20}, {8, 14}, {12, 10}, {16, 14}, {16, 20}});
			break;
		case 9:		// la cape
			L({{8, 3}, {16, 3}, {19, 21}, {5, 21}, {8, 3}}); L({{12, 6}, {12, 21}});
			break;
		case 10:	// l'amulette
			L({{6, 3}, {9, 7}, {12, 8}, {15, 7}, {18, 3}}); L({{12, 8}, {12, 11}}); L({{12, 11}, {16, 16}, {12, 21}, {8, 16}, {12, 11}});
			break;
		case 12:	// le baton
			L({{6, 21}, {15, 7}}); Cercle(T, 17, 5, 3, 10);
			break;
		default:	// une gemme
			L({{6, 4}, {18, 4}, {21, 9}, {12, 20}, {3, 9}, {6, 4}}); L({{3, 9}, {21, 9}});
			break;
	}
	const FVector2D Taille = Geo.GetLocalSize();
	const float E = FMath::Min(Taille.X, Taille.Y) / 24.0f;
	const FVector2f Decalage((Taille.X - 24.0f * E) * 0.5f, (Taille.Y - 24.0f * E) * 0.5f);
	for (TArray<FVector2f>& Trait : T)
	{
		for (FVector2f& P : Trait)
		{
			P = Decalage + P * E;
		}
		FSlateDrawElement::MakeLines(Elements, Couche, Geo.ToPaintGeometry(), Trait, ESlateDrawEffect::None, Couleur.Get() * Style.GetColorAndOpacityTint(), true, Epaisseur);
	}
	return Couche + 1;
}

// ===================== L'anneau d'AYLIS (dessine) =====================

class SVespAnneau : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVespAnneau) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		Rond = FSlateRoundedBoxBrush(FLinearColor::White, 400.0f);
		Rond.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1440, 900); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
	                      int32 Couche, const FWidgetStyle& Style, bool bParentActif) const override
	{
		const float T = Horloge();
		const FVector2f C(380.0f, 430.0f);
		// Le halo violet de la prophetie, qui respire
		const float Souffle = 0.5f + 0.5f * FMath::Sin(T * 1.05f);
		for (int32 k = 0; k < 6; k++)
		{
			const float R = 150.0f + k * 45.0f;
			FSlateDrawElement::MakeBox(Elements, Couche, Geo.ToPaintGeometry(FVector2f(2 * R, 2 * R), FSlateLayoutTransform(C - FVector2f(R, R))), &Rond,
			                           ESlateDrawEffect::None, FLinearColor(0.48f, 0.36f, 0.84f, (0.05f + 0.02f * Souffle) * (1.0f - k / 6.0f)));
		}
		// L'anneau pointille, qui tourne lentement ; et un anneau plus fin a l'interieur
		auto Pointilles = [&](float R, float Rotation, int32 Segments, const FLinearColor& Couleur, float Epaisseur) {
			for (int32 i = 0; i < Segments; i++)
			{
				const float A0 = Rotation + i * 2.0f * PI / Segments;
				const float A1 = A0 + PI / Segments;
				FSlateDrawElement::MakeLines(Elements, Couche + 1, Geo.ToPaintGeometry(),
				                             {C + FVector2f(FMath::Cos(A0), FMath::Sin(A0)) * R, C + FVector2f(FMath::Cos(A1), FMath::Sin(A1)) * R},
				                             ESlateDrawEffect::None, Couleur, true, Epaisseur);
			}
		};
		Pointilles(230.0f, T * 2.0f * PI / 90.0f, 72, FLinearColor(0.69f, 0.55f, 1.0f, 0.3f), 1.2f);
		Pointilles(168.0f, -T * 2.0f * PI / 140.0f, 120, FLinearColor(0.69f, 0.55f, 1.0f, 0.12f), 1.0f);
		// La silhouette d'AYLIS (le trait des maquettes), qui flotte
		const float Flotte = FMath::Sin(T * 2.0f * PI / 5.0f) * 8.0f;
		const float E = 1.35f;
		auto P = [&](float X, float Y) { return FVector2f(C.X + (X - 60.0f) * E, C.Y - 20.0f + (Y - 130.0f) * E + Flotte); };
		const FLinearColor Bleu(0.62f, 0.71f, 1.0f, 0.95f);
		auto Trait = [&](std::initializer_list<FVector2f> Points, const FLinearColor& Couleur, float Ep) {
			FSlateDrawElement::MakeLines(Elements, Couche + 2, Geo.ToPaintGeometry(), TArray<FVector2f>(Points), ESlateDrawEffect::None, Couleur, true, Ep);
		};
		Trait({P(60, 18), P(44, 22), P(34, 40), P(32, 62), P(38, 78), P(60, 86), P(82, 78), P(88, 62), P(86, 40), P(76, 22), P(60, 18)}, Bleu, 2.0f);		// la capuche
		Trait({P(36, 88), P(30, 120), P(28, 160), P(30, 200), P(90, 200), P(92, 160), P(90, 120), P(84, 88)}, Bleu, 2.0f);								// la cape
		Trait({P(30, 200), P(24, 236), P(50, 236), P(54, 200)}, Bleu, 1.6f);
		Trait({P(90, 200), P(96, 236), P(70, 236), P(66, 200)}, Bleu, 1.6f);
		Trait({P(84, 100), P(102, 128), P(98, 132)}, OR, 2.2f);																							// le bras, l'epee
		Trait({P(100, 130), P(112, 52)}, OR, 2.4f);
		return Couche + 3;
	}

private:
	FSlateBrush Rond;
};

// ===================== La constellation du Seuil =====================

static const float RAYONS[5] = {0.0f, 125.0f, 230.0f, 335.0f, 440.0f};
static const FVector2D CENTRE_SEUIL(720.0f, 560.0f);

FVector2D SVespConstellation::PositionEtoile(int32 Index)
{
	const FVespEtoile& E = VespSeuil::Etoile(Index);
	const float Base = E.Voie == EVespVoie::Lame ? -158.0f : (E.Voie == EVespVoie::Rempart ? -90.0f : -22.0f);
	const float Courbe = E.Voie == EVespVoie::Lame ? -4.0f : (E.Voie == EVespVoie::Rempart ? 0.0f : 4.0f);
	const float A = FMath::DegreesToRadians(Base + Courbe * (E.Rang - 1));
	const float R = RAYONS[FMath::Clamp(E.Rang, 1, 4)] * 0.92f;
	return CENTRE_SEUIL + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
}

void SVespConstellation::Construct(const FArguments& Args)
{
	Joueur = Args._Joueur;
	Rond = FSlateRoundedBoxBrush(FLinearColor::White, 200.0f);
	Rond.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	Anneau = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 200.0f, FLinearColor::White, 2.0f);
	Anneau.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	FRandomStream H(11);
	for (int32 i = 0; i < 110; i++)
	{
		Ciel.Add(FVector4f(H.FRandRange(0, 1440), H.FRandRange(0, 900), H.FRandRange(1.0f, 3.0f), H.FRandRange(0.0f, 6.28f)));
	}
}

int32 SVespConstellation::EtoileSous(const FGeometry& Geo, const FVector2D& Ecran) const
{
	const FVector2D Local = Geo.AbsoluteToLocal(Ecran);
	for (int32 i = 0; i < VespSeuil::Nombre; i++)
	{
		if (FVector2D::Distance(Local, PositionEtoile(i)) < 30.0f)
		{
			return i;
		}
	}
	return -1;
}

FReply SVespConstellation::OnMouseMove(const FGeometry& Geo, const FPointerEvent& Souris)
{
	const int32 i = EtoileSous(Geo, Souris.GetScreenSpacePosition());
	if (i >= 0 && Joueur.IsValid() && Joueur->SelectionEtoile != i)
	{
		Joueur->SelectionEtoile = i;
		UVespSons::Jouer2D(Joueur.Get(), EVespSon::Survol);
	}
	return FReply::Unhandled();
}

FReply SVespConstellation::OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Souris)
{
	const int32 i = EtoileSous(Geo, Souris.GetScreenSpacePosition());
	if (i >= 0 && Joueur.IsValid())
	{
		if (Joueur->SelectionEtoile == i)
		{
			Joueur->DebloquerEtoile(i);
		}
		Joueur->SelectionEtoile = i;
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

int32 SVespConstellation::OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect, FSlateWindowElementList& Elements,
                                  int32 Couche, const FWidgetStyle& Style, bool bParentActif) const
{
	const AVespPlayerController* J = Joueur.Get();
	if (!J)
	{
		return Couche;
	}
	const float T = Horloge();
	auto Disque = [&](int32 C, const FVector2D& P, float R, const FSlateBrush& B, const FLinearColor& Couleur) {
		FSlateDrawElement::MakeBox(Elements, C, Geo.ToPaintGeometry(FVector2f(2 * R, 2 * R), FSlateLayoutTransform(FVector2f(P - FVector2D(R, R)))), &B,
		                           ESlateDrawEffect::None, Couleur);
	};
	// Le ciel : des etoiles qui scintillent
	for (const FVector4f& E : Ciel)
	{
		const float S = 0.15f + 0.6f * (0.5f + 0.5f * FMath::Sin(T * (0.8f + E.W * 0.2f) + E.W));
		Disque(Couche, FVector2D(E.X, E.Y), E.Z * 0.5f, Rond, FLinearColor(0.81f, 0.77f, 1.0f, S));
	}
	// Les orbites
	for (int32 r = 1; r <= 4; r++)
	{
		const float R = RAYONS[r] * 0.92f;
		const int32 Segments = 60 + r * 20;
		const float Rot = T * (r % 2 == 0 ? 1.0f : -1.0f) * 0.03f;
		for (int32 i = 0; i < Segments; i += 2)
		{
			const float A0 = Rot + i * 2.0f * PI / Segments, A1 = Rot + (i + 1) * 2.0f * PI / Segments;
			FSlateDrawElement::MakeLines(Elements, Couche + 1, Geo.ToPaintGeometry(),
			                             {FVector2f(CENTRE_SEUIL + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R), FVector2f(CENTRE_SEUIL + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R)},
			                             ESlateDrawEffect::None, FLinearColor(0.69f, 0.55f, 1.0f, 0.09f), true, 1.0f);
		}
	}
	// Le coeur : AYLIS
	Disque(Couche + 2, CENTRE_SEUIL, 34.0f + 4.0f * FMath::Sin(T * 2.0f), Rond, FLinearColor(0.69f, 0.55f, 1.0f, 0.18f));
	Disque(Couche + 3, CENTRE_SEUIL, 16.0f, Rond, FLinearColor(0.93f, 0.9f, 1.0f, 1.0f));
	// Les liens : du coeur au premier rang, puis de rang en rang. Allumes : dores ; possibles : la lumiere coule ; sinon : a peine visibles
	for (int32 i = 0; i < VespSeuil::Nombre; i++)
	{
		const FVespEtoile& E = VespSeuil::Etoile(i);
		FVector2D De = CENTRE_SEUIL;
		bool bAvantAcquis = true;
		for (int32 k = 0; k < VespSeuil::Nombre; k++)
		{
			if (VespSeuil::Etoile(k).Voie == E.Voie && VespSeuil::Etoile(k).Rang == E.Rang - 1)
			{
				De = PositionEtoile(k);
				bAvantAcquis = J->EtoileAcquise(k);
			}
		}
		const FVector2D A = PositionEtoile(i);
		const FLinearColor CV = VespSeuil::CouleurVoie(E.Voie);
		if (J->EtoileAcquise(i))
		{
			FSlateDrawElement::MakeLines(Elements, Couche + 2, Geo.ToPaintGeometry(), {FVector2f(De), FVector2f(A)}, ESlateDrawEffect::None, FLinearColor(CV.R, CV.G, CV.B, 0.85f), true, 2.5f);
		}
		else if (bAvantAcquis)
		{
			// Des tirets qui avancent vers l'etoile
			const float L = FVector2D::Distance(De, A);
			const FVector2D Dir = (A - De) / FMath::Max(1.0f, L);
			for (float d = FMath::Fmod(T * 40.0f, 18.0f); d < L; d += 18.0f)
			{
				FSlateDrawElement::MakeLines(Elements, Couche + 2, Geo.ToPaintGeometry(), {FVector2f(De + Dir * d), FVector2f(De + Dir * FMath::Min(L, d + 8.0f))},
				                             ESlateDrawEffect::None, FLinearColor(CV.R, CV.G, CV.B, 0.55f), true, 1.6f);
			}
		}
		else
		{
			FSlateDrawElement::MakeLines(Elements, Couche + 2, Geo.ToPaintGeometry(), {FVector2f(De), FVector2f(A)}, ESlateDrawEffect::None, FLinearColor(0.4f, 0.36f, 0.55f, 0.25f), true, 1.0f);
		}
	}
	// Les etoiles
	const TSharedRef<FSlateFontMeasure> Mesure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	for (int32 i = 0; i < VespSeuil::Nombre; i++)
	{
		const FVespEtoile& E = VespSeuil::Etoile(i);
		const FVector2D P = PositionEtoile(i);
		const FLinearColor CV = VespSeuil::CouleurVoie(E.Voie);
		const bool bAcquise = J->EtoileAcquise(i);
		const bool bPossible = J->EtoilePossible(i);
		const float Taille = E.Pouvoir > 0 ? 17.0f : 13.0f;
		if (bAcquise)
		{
			Disque(Couche + 3, P, Taille * 2.3f, Rond, FLinearColor(CV.R, CV.G, CV.B, 0.22f));
			Disque(Couche + 4, P, Taille, Rond, CV);
		}
		else if (bPossible)
		{
			const float Pulse = 0.5f + 0.5f * FMath::Sin(T * 4.0f);
			Disque(Couche + 3, P, Taille * (1.6f + 0.4f * Pulse), Rond, FLinearColor(CV.R, CV.G, CV.B, 0.12f + 0.12f * Pulse));
			Disque(Couche + 4, P, Taille, Rond, FLinearColor(0.08f, 0.06f, 0.14f, 1.0f));
			Disque(Couche + 5, P, Taille, Anneau, CV);
		}
		else
		{
			Disque(Couche + 4, P, Taille * 0.8f, Rond, FLinearColor(0.1f, 0.08f, 0.16f, 1.0f));
			Disque(Couche + 5, P, Taille * 0.8f, Anneau, FLinearColor(0.4f, 0.36f, 0.55f, 0.6f));
		}
		// L'etoile choisie : un anneau qui tourne autour
		if (J->SelectionEtoile == i)
		{
			const float R = Taille + 12.0f;
			const float Rot = T * 1.6f;
			for (int32 k = 0; k < 12; k += 2)
			{
				const float A0 = Rot + k * 2.0f * PI / 12.0f, A1 = Rot + (k + 1) * 2.0f * PI / 12.0f;
				FSlateDrawElement::MakeLines(Elements, Couche + 6, Geo.ToPaintGeometry(),
				                             {FVector2f(P + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R), FVector2f(P + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R)},
				                             ESlateDrawEffect::None, OR, true, 2.0f);
			}
		}
		// Son nom, dessous
		const FString Nom = VespMajuscules(FString(E.Nom));
		const FSlateFontInfo F = Police("Bold", 10, 200);
		const FVector2D TT(Mesure->Measure(Nom, F));
		FSlateDrawElement::MakeText(Elements, Couche + 7, Geo.ToPaintGeometry(FVector2f(TT), FSlateLayoutTransform(FVector2f(P + FVector2D(-TT.X / 2, Taille + 10.0f)))),
		                            Nom, F, ESlateDrawEffect::None, bAcquise ? TEXTE : (bPossible ? CV : DOUX));
	}
	// Le nom des voies, au bout de chaque branche
	for (int32 v = 0; v < 3; v++)
	{
		const EVespVoie Voie = (EVespVoie)v;
		const float Base = v == 0 ? -158.0f : (v == 1 ? -90.0f : -22.0f);
		const float Courbe = v == 0 ? -12.0f : (v == 1 ? 0.0f : 12.0f);
		const float A = FMath::DegreesToRadians(Base + Courbe);
		const FString Nom = VespSeuil::NomVoie(Voie);
		const FSlateFontInfo F = Police("Bold", 12, 500);
		const FVector2D TT(Mesure->Measure(Nom, F));
		// assez loin pour que le texte (large sur les cotes) ne touche pas l'etoile
		const float Ecart = 36.0f + FMath::Abs(FMath::Cos(A)) * TT.X * 0.5f + FMath::Abs(FMath::Sin(A)) * TT.Y * 0.5f;
		const FVector2D P = CENTRE_SEUIL + FVector2D(FMath::Cos(A), FMath::Sin(A)) * (RAYONS[4] * 0.92f + Ecart);
		FSlateDrawElement::MakeText(Elements, Couche + 7, Geo.ToPaintGeometry(FVector2f(TT), FSlateLayoutTransform(FVector2f(P - TT / 2))), Nom, F,
		                            ESlateDrawEffect::None, VespSeuil::CouleurVoie(Voie));
	}
	return Couche + 8;
}

// ===================== Le menu =====================

void SVespMenu::Construct(const FArguments& Args)
{
	Joueur = Args._Joueur;
	Rond = FSlateRoundedBoxBrush(FLinearColor::White, 64.0f);
	Rond.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	Blanc = FSlateRoundedBoxBrush(FLinearColor::White, 3.0f);
	Tuiles = FSlateRoundedBoxBrush(FLinearColor(0.067f, 0.047f, 0.114f, 1.0f), 16.0f, FLinearColor(0.118f, 0.094f, 0.188f, 1.0f), 1.0f);
	TuileChoisie = FSlateRoundedBoxBrush(FLinearColor(0.114f, 0.082f, 0.208f, 1.0f), 16.0f, OR, 1.5f);
	Cadre = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 24.0f, LIGNE, 1.0f);
	Rien = FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 0.0f);
	Voile = FSlateRoundedBoxBrush(FOND, 0.0f);
	StyleBouton = FButtonStyle().SetNormal(Rien).SetHovered(Rien).SetPressed(Rien).SetDisabled(Rien).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0, 1, 0, 0));

	ChildSlot
	[
		SNew(SBorder).BorderImage(&Voile).Padding(0)
		.Visibility_Lambda([this]() { return Joueur.IsValid() && Joueur->bMenuOuvert && Joueur->Phase == EVespPhase::Exploration ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(1440).HeightOverride(900)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[Inventaire()]
					+ SOverlay::Slot()[LeSeuil()]
					// L'en-tete : les onglets, et ce qu'AYLIS possede
					+ SOverlay::Slot().VAlign(VAlign_Top).Padding(FMargin(60, 36, 60, 0))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 40, 0)[Onglet(LOCTEXT("OngletInventaire", "INVENTAIRE"), 0)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 40, 0)[Onglet(LOCTEXT("OngletSeuil", "LE SEUIL"), 1)]
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28, 0, 0, 0)
						[
							SNew(STextBlock).Font(Police("Bold", 14)).ColorAndOpacity(OR)
							.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("%d éclats"), Joueur->Eclats)) : FText::GetEmpty(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28, 0, 0, 0)
						[
							SNew(STextBlock).Font(Police("Bold", 14)).ColorAndOpacity(FLinearColor(0.94f, 0.55f, 0.55f))
							.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("%d potions"), Joueur->Potions)) : FText::GetEmpty(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28, 0, 0, 0)
						[
							SNew(STextBlock).Font(Police("Bold", 14)).ColorAndOpacity(VIOLET)
							.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("niveau %d  -  %d point%s"), Joueur->Niveau, Joueur->PointsDeCompetence,
							                                                                        Joueur->PointsDeCompetence > 1 ? TEXT("s") : TEXT(""))) : FText::GetEmpty(); })
						]
					]
					// Les commandes
					// (le sac : sous l'en-tete, la fiche d'objet et ses boutons occupent le bas ; le Seuil : en bas, le haut est a la constellation)
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(FMargin(0, 88, 60, 0))
					[
						SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(TRES_DOUX)
						.Text_Lambda([this]() {
							return Joueur.IsValid() && Joueur->OngletMenu == 0
							           ? LOCTEXT("CommandesSac", "flèches : choisir     ENTRÉE / A : équiper     SUPPR / X : jeter     TAB / LB RB : onglet     I / B : fermer")
							           : FText::GetEmpty();
						})
					]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 60, 30))
					[
						SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(TRES_DOUX)
						.Text_Lambda([this]() {
							return Joueur.IsValid() && Joueur->OngletMenu == 1
							           ? LOCTEXT("CommandesSeuil", "flèches : choisir     ENTRÉE / A : allumer l'étoile     TAB / LB RB : onglet     I / B : fermer")
							           : FText::GetEmpty();
						})
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SVespMenu::Onglet(const FText& Nom, int32 Numero)
{
	return SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
	.OnClicked_Lambda([this, Numero]() { if (Joueur.IsValid()) { Joueur->OngletMenu = Numero; UVespSons::Jouer2D(Joueur.Get(), EVespSon::Clic); } return FReply::Handled(); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
		[
			SNew(STextBlock).Text(Nom).Font(Police("Bold", 15, 400))
			.ColorAndOpacity_Lambda([this, Numero]() { return FSlateColor(Joueur.IsValid() && Joueur->OngletMenu == Numero ? OR : DOUX); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(2)
			[
				SNew(SBorder).BorderImage(&Blanc)
				.BorderBackgroundColor_Lambda([this, Numero]() { return FSlateColor(Joueur.IsValid() && Joueur->OngletMenu == Numero ? OR : FLinearColor(0, 0, 0, 0)); })
			]
		]
	];
}

const FVespObjet* SVespMenu::ObjetChoisi() const
{
	const AVespPlayerController* J = Joueur.Get();
	if (!J)
	{
		return nullptr;
	}
	if (EmplacementChoisi >= 0)
	{
		return J->Equipe((EVespEmplacement)EmplacementChoisi);
	}
	return J->Sac.IsValidIndex(J->SelectionSac) ? &J->Sac[J->SelectionSac] : nullptr;
}

TSharedRef<SWidget> SVespMenu::Bouton(const FText& Libelle, bool bPrincipal, TFunction<void()> Action, TAttribute<bool> Actif)
{
	TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
	TSharedRef<SButton> B = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton).IsEnabled(Actif)
	.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
	[
		SNew(SBorder).Padding(FMargin(26, 12)).BorderImage(&Rond)
		.BorderBackgroundColor_Lambda([Lien, bPrincipal]() {
			const TSharedPtr<SButton> P = Lien->Pin();
			const bool bSurvol = P.IsValid() && P->IsHovered();
			return FSlateColor(bPrincipal ? (bSurvol ? FLinearColor(0.95f, 0.83f, 0.56f) : OR) : (bSurvol ? FLinearColor(0.23f, 0.18f, 0.34f) : FLinearColor(0.12f, 0.1f, 0.19f)));
		})
		[
			SNew(STextBlock).Text(Libelle).Font(Police("Bold", 13, 60)).ColorAndOpacity(bPrincipal ? FLinearColor(0.1f, 0.07f, 0.02f) : TEXTE)
		]
	];
	*Lien = B;
	return B;
}

// ----- L'inventaire -----

TSharedRef<SWidget> SVespMenu::Emplacement(int32 Numero)
{
	auto Objet = [this, Numero]() -> const FVespObjet* { return Joueur.IsValid() ? Joueur->Equipe((EVespEmplacement)Numero) : nullptr; };
	TSharedRef<TWeakPtr<SButton>> Lien = MakeShared<TWeakPtr<SButton>>();
	TSharedRef<SButton> B = SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
	.OnClicked_Lambda([this, Numero]() {
		EmplacementChoisi = Numero;
		if (Joueur.IsValid()) { Joueur->VueFiche++; UVespSons::Jouer2D(Joueur.Get(), EVespSon::Clic); }
		return FReply::Handled();
	})
	[
		SNew(SBox).WidthOverride(68).HeightOverride(68)
		[
			SNew(SBorder).BorderImage(&Rond).Padding(2)
			.BorderBackgroundColor_Lambda([this, Numero, Objet, Lien]() {
				const FVespObjet* O = Objet();
				const TSharedPtr<SButton> Bt = Lien->Pin();
				if (EmplacementChoisi == Numero) return FSlateColor(OR);
				if (!O) return FSlateColor(FLinearColor(0.23f, 0.18f, 0.34f, Bt.IsValid() && Bt->IsHovered() ? 1.0f : 0.6f));
				return FSlateColor(VespButin::CouleurRarete(O->Rarete));
			})
			[
				SNew(SBorder).BorderImage(&Rond).BorderBackgroundColor(FLinearColor(0.06f, 0.043f, 0.1f, 1.0f)).Padding(14)
				[
					SNew(SVespIcone).Epaisseur(1.8f)
					.Icone_Lambda([Objet, Numero]() { const FVespObjet* O = Objet(); return O ? O->Icone : ICONES_VIDES[Numero]; })
					.Couleur_Lambda([Objet]() { const FVespObjet* O = Objet(); return O ? VespButin::CouleurRarete(O->Rarete) : FLinearColor(0.31f, 0.28f, 0.4f); })
				]
			]
		]
	];
	*Lien = B;
	return SNew(SBox).WidthOverride(120)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[B]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
		[
			SNew(STextBlock).Text(Texte(VespButin::NomEmplacement((EVespEmplacement)Numero))).Font(Police("Regular", 10, 200)).ColorAndOpacity(TRES_DOUX)
		]
	];
}

TSharedRef<SWidget> SVespMenu::Tuile(int32 Index)
{
	auto Objet = [this, Index]() -> const FVespObjet* { return Joueur.IsValid() && Joueur->Sac.IsValidIndex(Index) ? &Joueur->Sac[Index] : nullptr; };
	auto Choisie = [this, Index]() { return Joueur.IsValid() && EmplacementChoisi < 0 && Joueur->SelectionSac == Index && Joueur->Sac.IsValidIndex(Index); };
	return SNew(SButton).IsFocusable(false).ButtonStyle(&StyleBouton)
	.OnClicked_Lambda([this, Index]() {
		if (Joueur.IsValid() && Joueur->Sac.IsValidIndex(Index))
		{
			if (EmplacementChoisi < 0 && Joueur->SelectionSac == Index)
			{
				Joueur->EquiperDuSac(Index);		// un deuxieme clic : AYLIS s'en equipe
			}
			else
			{
				Joueur->SelectionSac = Index;
				Joueur->VueFiche++;
				UVespSons::Jouer2D(Joueur.Get(), EVespSon::Clic);
			}
			EmplacementChoisi = -1;
		}
		return FReply::Handled();
	})
	[
		SNew(SBox).WidthOverride(88).HeightOverride(84)
		.RenderTransform_Lambda([Choisie]() { return FSlateRenderTransform(FVector2f(0.0f, Choisie() ? -4.0f : 0.0f)); })
		[
			SNew(SBorder).Padding(0).BorderImage_Lambda([this, Choisie]() { return Choisie() ? &TuileChoisie : &Tuiles; })
			.ColorAndOpacity_Lambda([Objet]() { return FLinearColor(1, 1, 1, Objet() ? 1.0f : 0.35f); })
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(24)
				[
					SNew(SVespIcone).Epaisseur(1.6f)
					.Icone_Lambda([Objet]() { const FVespObjet* O = Objet(); return O ? O->Icone : -1; })
					.Couleur_Lambda([Objet]() { const FVespObjet* O = Objet(); return O ? VespButin::CouleurRarete(O->Rarete) : FLinearColor(0, 0, 0, 0); })
				]
				+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(FMargin(20, 0, 20, 0))
				[
					SNew(SBox).HeightOverride(3)
					[
						SNew(SBorder).BorderImage(&Blanc)
						.BorderBackgroundColor_Lambda([Objet]() { const FVespObjet* O = Objet(); return FSlateColor(O ? VespButin::CouleurRarete(O->Rarete) : FLinearColor(0, 0, 0, 0)); })
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SVespMenu::Fiche()
{
	// La fiche glisse de la droite a chaque nouvel objet regarde
	TSharedRef<int32> DerniereVue = MakeShared<int32>(-1);
	TSharedRef<double> Depuis = MakeShared<double>(0.0);
	auto Avance = [this, DerniereVue, Depuis]() {
		const int32 Vue = Joueur.IsValid() ? Joueur->VueFiche + EmplacementChoisi * 1000 : 0;
		if (Vue != *DerniereVue)
		{
			*DerniereVue = Vue;
			*Depuis = FPlatformTime::Seconds();
		}
		return FMath::Clamp((float)(FPlatformTime::Seconds() - *Depuis) / 0.35f, 0.0f, 1.0f);
	};
	auto Couleur = [this]() { const FVespObjet* O = ObjetChoisi(); return O ? VespButin::CouleurRarete(O->Rarete) : DOUX; };
	return SNew(SBox).WidthOverride(600)
	.RenderTransform_Lambda([Avance]() { const float T = Avance(); return FSlateRenderTransform(FVector2f(24.0f * FMath::Square(1.0f - T), 0.0f)); })
	[
		SNew(SBorder).BorderImage(&Rien).Padding(0)
		.ColorAndOpacity_Lambda([Avance]() { return FLinearColor(1, 1, 1, Avance()); })
		.Visibility_Lambda([this]() { return ObjetChoisi() ? EVisibility::Visible : EVisibility::Hidden; })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(SBox).WidthOverride(28).HeightOverride(2)[SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor_Lambda([Couleur]() { return FSlateColor(Couleur()); })]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(STextBlock).Font(Police("Bold", 12, 400)).ColorAndOpacity_Lambda([Couleur]() { return FSlateColor(Couleur()); })
					.Text_Lambda([this]() { const FVespObjet* O = ObjetChoisi(); return O ? Texte(VespButin::NomRarete(O->Rarete)) : FText::GetEmpty(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(TRES_DOUX)
					.Text_Lambda([this]() {
						const FVespObjet* O = ObjetChoisi();
						return O ? Texte(FString(VespButin::NomEmplacement(O->Emplacement)) + (EmplacementChoisi >= 0 ? TEXT("  -  porte par AYLIS") : TEXT("")))
						         : FText::GetEmpty();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 10)
			[
				SNew(STextBlock).Font(Police("Bold", 30)).ColorAndOpacity(TEXTE).AutoWrapText(true)
				.Text_Lambda([this]() { const FVespObjet* O = ObjetChoisi(); return O ? Texte(O->Nom) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Regular", 16)).ColorAndOpacity(TEXTE).LineHeightPercentage(1.2f)
				.Text_Lambda([this]() { const FVespObjet* O = ObjetChoisi(); return O ? Texte(O->Lignes()) : FText::GetEmpty(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
			[
				SNew(STextBlock).Font(Police("Italic", 14)).ColorAndOpacity(DOUX).AutoWrapText(true)
				.Text_Lambda([this]() { const FVespObjet* O = ObjetChoisi(); return O ? Texte(O->Recit) : FText::GetEmpty(); })
			]
			// La comparaison avec ce qu'AYLIS porte deja a cet emplacement
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
			[
				SNew(STextBlock).Font(Police("Bold", 13))
				.Visibility_Lambda([this]() { return EmplacementChoisi < 0 && ObjetChoisi() ? EVisibility::Visible : EVisibility::Collapsed; })
				.ColorAndOpacity_Lambda([this]() {
					const FVespObjet* O = ObjetChoisi();
					const FVespObjet* Porte = O && Joueur.IsValid() ? Joueur->Equipe(O->Emplacement) : nullptr;
					return FSlateColor(!Porte || O->Valeur() >= Porte->Valeur() ? FLinearColor(0.5f, 0.84f, 0.6f) : FLinearColor(0.95f, 0.6f, 0.45f));
				})
				.Text_Lambda([this]() {
					const FVespObjet* O = ObjetChoisi();
					if (!O || !Joueur.IsValid()) return FText::GetEmpty();
					const FVespObjet* Porte = Joueur->Equipe(O->Emplacement);
					if (!Porte) return Texte(TEXT("Rien n'est porté à cet emplacement."));
					TArray<FString> D;
					auto Ecart = [&D](int32 A, int32 B, const TCHAR* Nom) { if (A != B) D.Add(FString::Printf(TEXT("%+d %s"), A - B, Nom)); };
					Ecart(O->Attaque, Porte->Attaque, TEXT("attaque"));
					Ecart(O->Defense, Porte->Defense, TEXT("défense"));
					Ecart(O->PvMax, Porte->PvMax, TEXT("pv"));
					Ecart(O->Critique, Porte->Critique, TEXT("% critique"));
					return Texte(TEXT("Par rapport à ") + Porte->Nom + TEXT(" :  ") + (D.Num() ? FString::Join(D, TEXT("   ")) : FString(TEXT("pareil"))));
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
				[
					Bouton(LOCTEXT("Equiper", "Équiper"), true, [this]() { if (Joueur.IsValid() && EmplacementChoisi < 0) Joueur->EquiperDuSac(Joueur->SelectionSac); },
					       TAttribute<bool>::CreateLambda([this]() { return EmplacementChoisi < 0 && ObjetChoisi() != nullptr; }))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
				[
					Bouton(LOCTEXT("Retirer", "Retirer"), false, [this]() {
						if (Joueur.IsValid() && EmplacementChoisi >= 0) { Joueur->Retirer((EVespEmplacement)EmplacementChoisi); EmplacementChoisi = -1; }
					}, TAttribute<bool>::CreateLambda([this]() { return EmplacementChoisi >= 0 && ObjetChoisi() != nullptr; }))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					Bouton(LOCTEXT("Jeter", "Jeter (+éclats)"), false, [this]() { if (Joueur.IsValid() && EmplacementChoisi < 0) Joueur->JeterDuSac(Joueur->SelectionSac); },
					       TAttribute<bool>::CreateLambda([this]() { return EmplacementChoisi < 0 && ObjetChoisi() != nullptr; }))
				]
			]
		]
	];
}

TSharedRef<SWidget> SVespMenu::Inventaire()
{
	TSharedRef<SOverlay> O = SNew(SOverlay).Visibility_Lambda([this]() { return Joueur.IsValid() && Joueur->OngletMenu == 0 ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; });
	O->AddSlot()[SNew(SVespAnneau)];
	// Les six emplacements, autour d'AYLIS
	for (int32 i = 0; i < 6; i++)
	{
		const FVector2D P = PositionEmplacement(i);
		O->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(P.X - 60.0f, P.Y - 34.0f, 0, 0))[Emplacement(i)];
	}
	// Le nom, sous AYLIS
	O->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(290, 566, 0, 0))
	[
		SNew(SBox).WidthOverride(180)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(LOCTEXT("AYLIS", "AYLIS")).Font(Police("Bold", 24, 500)).ColorAndOpacity(TEXTE)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(Police("Italic", 12)).ColorAndOpacity(DOUX)
				.Text_Lambda([this]() {
					if (!Joueur.IsValid()) return FText::GetEmpty();
					switch (Joueur->ArmeEnMain())
					{
						case EVespArme::Dagues: return LOCTEXT("VoieDagues", "voie des dagues");
						case EVespArme::DeuxMains: return LOCTEXT("VoieDeuxMains", "voie de la grande lame");
						case EVespArme::Baton: return LOCTEXT("VoieBaton", "voie des sorts");
						default: return LOCTEXT("VoieEpee", "voie de l'épée");
					}
				})
			]
		]
	];
	// Les stats : quatre chiffres, et la vie
	auto Chiffre = [this](TFunction<FString()> Valeur, const FText& Nom) -> TSharedRef<SWidget> {
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(Police("Bold", 22)).ColorAndOpacity(TEXTE).Text_Lambda([Valeur]() { return Texte(Valeur()); })]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Nom).Font(Police("Regular", 10, 200)).ColorAndOpacity(DOUX)];
	};
	O->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(110, 752, 0, 0))
	[
		SNew(SBox).WidthOverride(540)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[Chiffre([this]() { return Joueur.IsValid() ? FString::FromInt(Joueur->Aylis->Stats.Attaque) : FString(); }, LOCTEXT("Attaque", "ATTAQUE"))]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[Chiffre([this]() { return Joueur.IsValid() ? FString::FromInt(Joueur->Aylis->Stats.Defense) : FString(); }, LOCTEXT("Defense", "DÉFENSE"))]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[Chiffre([this]() { return Joueur.IsValid() ? FString::Printf(TEXT("%d%%"), Joueur->Aylis->Stats.ChanceCritique) : FString(); }, LOCTEXT("Critique", "CRITIQUE"))]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[Chiffre([this]() { return Joueur.IsValid() ? FString::Printf(TEXT("+%d%%"), FMath::RoundToInt(Joueur->BonusVitesse * 100.0f)) : FString(); }, LOCTEXT("Vitesse", "VITESSE"))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(DOUX)
					.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("VIE  %d / %d"), Joueur->Aylis->Stats.Pv, Joueur->Aylis->Stats.PvMax)) : FText::GetEmpty(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SBox).HeightOverride(4)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(FLinearColor(0.12f, 0.094f, 0.19f))]
						+ SOverlay::Slot().HAlign(HAlign_Left)
						[
							SNew(SBox).WidthOverride_Lambda([this]() { return Joueur.IsValid() ? FOptionalSize(380.0f * Joueur->Aylis->Stats.Pv / FMath::Max(1, Joueur->Aylis->Stats.PvMax)) : FOptionalSize(0.0f); })
							[
								SNew(SBorder).BorderImage(&Blanc).BorderBackgroundColor(FLinearColor(0.5f, 0.84f, 0.6f))
							]
						]
					]
				]
			]
		]
	];
	// La sacoche
	TSharedRef<SUniformGridPanel> Grille = SNew(SUniformGridPanel).SlotPadding(FMargin(6));
	for (int32 i = 0; i < AVespPlayerController::TailleDuSac; i++)
	{
		Grille->AddSlot(i % 6, i / 6)[Tuile(i)];
	}
	O->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(780, 118, 0, 0))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 0, 0, 8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(LOCTEXT("Sacoche", "LA SACOCHE")).Font(Police("Bold", 12, 300)).ColorAndOpacity(DOUX)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 0, 0)
			[
				SNew(STextBlock).Font(Police("Regular", 12)).ColorAndOpacity(TRES_DOUX)
				.Text_Lambda([this]() { return Joueur.IsValid() ? Texte(FString::Printf(TEXT("%d / %d"), Joueur->Sac.Num(), AVespPlayerController::TailleDuSac)) : FText::GetEmpty(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[Grille]
	];
	O->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(786, 540, 0, 0))[Fiche()];
	return O;
}

// ----- Le Seuil -----

TSharedRef<SWidget> SVespMenu::LeSeuil()
{
	auto Etoile = [this]() { return Joueur.IsValid() ? Joueur->SelectionEtoile : 0; };
	return SNew(SOverlay).Visibility_Lambda([this]() { return Joueur.IsValid() && Joueur->OngletMenu == 1 ? EVisibility::Visible : EVisibility::Collapsed; })
	+ SOverlay::Slot()[SNew(SVespConstellation).Joueur(Joueur)]
	// La fiche de l'etoile choisie, en bas a droite
	+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 60, 70))
	[
		SNew(SBox).WidthOverride(380)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Bold", 11, 400))
				.ColorAndOpacity_Lambda([Etoile]() { return FSlateColor(VespSeuil::CouleurVoie(VespSeuil::Etoile(Etoile()).Voie)); })
				.Text_Lambda([Etoile]() {
					const FVespEtoile& E = VespSeuil::Etoile(Etoile());
					return Texte(FString::Printf(TEXT("%s  -  RANG %d"), VespSeuil::NomVoie(E.Voie), E.Rang));
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
				[
					SNew(SBox).WidthOverride(64).HeightOverride(64)
					[
						SNew(SImage).Image_Lambda([Etoile]() { return VespIconeEtoile(Etoile()); })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(Police("Bold", 28)).ColorAndOpacity(TEXTE)
					.Text_Lambda([Etoile]() { return Texte(VespSeuil::Etoile(Etoile()).Nom); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Police("Regular", 15)).ColorAndOpacity(TEXTE).AutoWrapText(true)
				.Text_Lambda([Etoile]() { return Texte(VespSeuil::Etoile(Etoile()).Aide); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					Bouton(LOCTEXT("Allumer", "Allumer l'étoile  (1 point)"), true, [this, Etoile]() { if (Joueur.IsValid()) Joueur->DebloquerEtoile(Etoile()); },
					       TAttribute<bool>::CreateLambda([this, Etoile]() { return Joueur.IsValid() && Joueur->EtoilePossible(Etoile()); }))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 0, 0)
				[
					SNew(STextBlock).Font(Police("Italic", 13)).ColorAndOpacity(DOUX)
					.Text_Lambda([this, Etoile]() {
						if (!Joueur.IsValid()) return FText::GetEmpty();
						if (Joueur->EtoileAcquise(Etoile())) return LOCTEXT("Allumee", "déjà allumée");
						if (Joueur->PointsDeCompetence <= 0) return LOCTEXT("PasDePoint", "un point par niveau gagne");
						if (!Joueur->EtoilePossible(Etoile())) return LOCTEXT("Avant", "allume d'abord l'étoile d'avant");
						return FText::GetEmpty();
					})
				]
			]
		]
	]
	// Le titre, en haut a gauche
	+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(60, 110, 0, 0))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("LeSeuil", "LE SEUIL")).Font(Police("Bold", 30, 600)).ColorAndOpacity(TEXTE)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("SeuilAide", "Chaque niveau allume une étoile. Trois voies, trois pouvoirs."))
			.Font(Police("Italic", 14)).ColorAndOpacity(DOUX)
		]
	];
}

#undef LOCTEXT_NAMESPACE
