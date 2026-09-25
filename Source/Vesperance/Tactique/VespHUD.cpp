#include "VespHUD.h"
#include "VespPlayerController.h"
#include "VespUnite.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"

// Les couleurs de l'interface (les memes que le prototype 2D)
static const FLinearColor PANNEAU(0.016f, 0.012f, 0.025f, 0.92f);
static const FLinearColor CADRE(0.03f, 0.027f, 0.05f, 0.95f);
static const FLinearColor CADRE_CLAIR(0.07f, 0.06f, 0.11f, 0.95f);
static const FLinearColor BORD(0.34f, 0.31f, 0.44f, 1.0f);
static const FLinearColor OR(0.91f, 0.75f, 0.33f, 1.0f);
static const FLinearColor GRIS(0.59f, 0.57f, 0.65f, 1.0f);
static const FLinearColor BLANC(0.96f, 0.95f, 0.98f, 1.0f);

static FLinearColor CouleurVie(float Part)
{
	if (Part <= 0.25f) return FLinearColor(0.82f, 0.24f, 0.24f);
	if (Part <= 0.5f) return FLinearColor(0.86f, 0.75f, 0.24f);
	return FLinearColor(0.31f, 0.75f, 0.35f);
}

// ===================== Les briques =====================

void AVespHUD::Cadre(FBox2D R, FLinearColor Fond, FLinearColor Bord, float Epaisseur)
{
	DrawRect(Fond, R.Min.X, R.Min.Y, R.GetSize().X, R.GetSize().Y);
	DrawLine(R.Min.X, R.Min.Y, R.Max.X, R.Min.Y, Bord, Epaisseur);
	DrawLine(R.Max.X, R.Min.Y, R.Max.X, R.Max.Y, Bord, Epaisseur);
	DrawLine(R.Max.X, R.Max.Y, R.Min.X, R.Max.Y, Bord, Epaisseur);
	DrawLine(R.Min.X, R.Max.Y, R.Min.X, R.Min.Y, Bord, Epaisseur);
}

void AVespHUD::Jauge(FBox2D R, float Part, FLinearColor Couleur, const FString& T)
{
	Part = FMath::Clamp(Part, 0.0f, 1.0f);
	DrawRect(FLinearColor(0.04f, 0.035f, 0.06f), R.Min.X, R.Min.Y, R.GetSize().X, R.GetSize().Y);
	DrawRect(Couleur, R.Min.X, R.Min.Y, R.GetSize().X * Part, R.GetSize().Y);
	DrawRect(FLinearColor(1, 1, 1, 0.15f), R.Min.X, R.Min.Y, R.GetSize().X * Part, R.GetSize().Y / 3);		// un reflet
	Cadre(R, FLinearColor(0, 0, 0, 0), BORD, 1.0f);
	if (!T.IsEmpty())
	{
		Texte(T, R.GetCenter().X, R.Min.Y + (R.GetSize().Y - 14 * Taille) / 2, BLANC, 0.9f, true);
	}
}

void AVespHUD::Texte(const FString& T, float X, float Y, FLinearColor Couleur, float Echelle, bool bCentre)
{
	UFont* Police = GEngine->GetMediumFont();
	float Largeur = 0, Hauteur = 0;
	GetTextSize(T, Largeur, Hauteur, Police, Echelle * Taille);
	const float XFinal = bCentre ? X - Largeur / 2 : X;
	DrawText(T, FLinearColor(0, 0, 0, 0.7f), XFinal + 2, Y + 2, Police, Echelle * Taille);		// une ombre, pour la lisibilite
	DrawText(T, Couleur, XFinal, Y, Police, Echelle * Taille);
}

// Les cartes d'actions, en bas a droite
FBox2D AVespHUD::RectangleCarte(int32 Numero, FVector2D Ecran)
{
	const float T = FMath::Clamp(Ecran.Y / 900.0f, 0.7f, 2.0f);
	const float Largeur = 150 * T, Hauteur = 96 * T, Ecart = 10 * T;
	const float X0 = Ecran.X - NombreDeCartes * (Largeur + Ecart) - 10 * T;
	const float Y0 = Ecran.Y - Hauteur - 18 * T;
	const FVector2D Min(X0 + Numero * (Largeur + Ecart), Y0);
	return FBox2D(Min, Min + FVector2D(Largeur, Hauteur));
}

// ===================== Les barres de vie au-dessus des personnages =====================

void AVespHUD::BarresDeVie()
{
	for (TActorIterator<AVespUnite> It(GetWorld()); It; ++It)
	{
		AVespUnite* U = *It;
		if (!U->EstDebout())
		{
			continue;
		}
		const FVector Ecran = Project(U->GetActorLocation() + FVector(0, 0, 205), false);
		if (Ecran.Z <= 0)
		{
			continue;	// derriere la camera
		}
		const float Part = (float)U->Stats.Pv / U->Stats.PvMax;
		const float L = 80 * Taille, H = 10 * Taille;
		const FBox2D R(FVector2D(Ecran.X - L / 2, Ecran.Y), FVector2D(Ecran.X + L / 2, Ecran.Y + H));
		Jauge(R, Part, U->EstAylis() ? FLinearColor(0.31f, 0.55f, 0.95f) : CouleurVie(Part), TEXT(""));
		Texte(FString::Printf(TEXT("%d"), U->Stats.Pv), Ecran.X, Ecran.Y - 20 * Taille, U->EstAylis() ? FLinearColor(0.6f, 0.75f, 1.0f) : BLANC, 0.8f, true);
	}
}

// ===================== L'interface complete =====================

void AVespHUD::DrawHUD()
{
	Super::DrawHUD();
	AVespPlayerController* Joueur = Cast<AVespPlayerController>(GetOwningPlayerController());
	if (!Joueur || !Joueur->Aylis || !Canvas)
	{
		return;
	}
	const FVector2D Ecran(Canvas->SizeX, Canvas->SizeY);
	Taille = FMath::Clamp(Ecran.Y / 900.0f, 0.7f, 2.0f);
	const AVespUnite* Aylis = Joueur->Aylis;

	BarresDeVie();

	// ----- En haut : le lieu, a qui c'est de jouer, le tour -----
	DrawRect(FLinearColor(0, 0, 0, 0.55f), 0, 0, Ecran.X, 46 * Taille);
	Texte(TEXT("ACTE I"), 16 * Taille, 12 * Taille, OR, 1.2f);
	Texte(TEXT("La Foret des Brumes"), 110 * Taille, 12 * Taille, BLANC, 1.2f);
	const bool bTourAylis = Joueur->Phase == EVespPhase::TourAylis;
	const FString Qui = bTourAylis ? TEXT("TON TOUR") : (Joueur->Phase == EVespPhase::TourHaschen ? TEXT("TOUR DES HASCHEN") : TEXT("FIN DU COMBAT"));
	const FLinearColor CouleurQui = bTourAylis ? FLinearColor(0.35f, 0.78f, 0.43f) : FLinearColor(0.86f, 0.31f, 0.31f);
	const float LargeurPilule = 230 * Taille;
	const FBox2D Pilule(FVector2D(Ecran.X - LargeurPilule - 14 * Taille, 8 * Taille), FVector2D(Ecran.X - 14 * Taille, 38 * Taille));
	Cadre(Pilule, CouleurQui * FLinearColor(1, 1, 1, 0.25f), CouleurQui, 2.0f);
	Texte(Qui, Pilule.GetCenter().X, 13 * Taille, CouleurQui, 1.1f, true);
	Texte(FString::Printf(TEXT("Tour %d"), Joueur->Tour), Pilule.Min.X - 90 * Taille, 13 * Taille, GRIS, 1.1f);

	// ----- Le journal : les 3 derniers messages, les plus anciens plus pales -----
	for (int32 i = 0; i < Joueur->Journal.Num(); i++)
	{
		const float Transparence = 0.45f + 0.55f * (i + 1) / Joueur->Journal.Num();
		const FString& Ligne = Joueur->Journal[i];
		FLinearColor Couleur = FLinearColor(0.8f, 0.8f, 0.85f);
		if (Ligne.Contains(TEXT("touche AYLIS"))) Couleur = FLinearColor(1.0f, 0.5f, 0.5f);
		else if (Ligne.Contains(TEXT("tombe"))) Couleur = FLinearColor(0.55f, 0.9f, 0.55f);
		else if (Ligne.Contains(TEXT("!"))) Couleur = OR;
		Couleur.A = Transparence;
		Texte(Ligne, 16 * Taille, (58 + i * 26) * Taille, Couleur, 1.0f);
	}

	// ----- Le panneau du bas -----
	const float HauteurPanneau = 132 * Taille;
	const float Y0 = Ecran.Y - HauteurPanneau;
	DrawRect(PANNEAU, 0, Y0, Ecran.X, HauteurPanneau);
	DrawRect(BORD, 0, Y0, Ecran.X, 2);

	// AYLIS : son nom, ses jauges, ses potions
	const float X = 20 * Taille;
	Texte(TEXT("AYLIS"), X, Y0 + 12 * Taille, FLinearColor(0.47f, 0.7f, 1.0f), 1.4f);
	Texte(TEXT("voie de l'epee"), X + 110 * Taille, Y0 + 20 * Taille, GRIS, 0.9f);
	const float LargeurJauge = 300 * Taille;
	Texte(TEXT("PV"), X, Y0 + 52 * Taille, GRIS, 0.9f);
	Jauge(FBox2D(FVector2D(X + 50 * Taille, Y0 + 50 * Taille), FVector2D(X + 50 * Taille + LargeurJauge, Y0 + 72 * Taille)),
	      (float)Aylis->Stats.Pv / Aylis->Stats.PvMax, CouleurVie((float)Aylis->Stats.Pv / Aylis->Stats.PvMax),
	      FString::Printf(TEXT("%d / %d"), Aylis->Stats.Pv, Aylis->Stats.PvMax));
	Texte(TEXT("RAGE"), X, Y0 + 84 * Taille, GRIS, 0.9f);
	const bool bRagePleine = Joueur->Rage >= 100;
	const float Pulsation = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 8);
	Jauge(FBox2D(FVector2D(X + 50 * Taille, Y0 + 82 * Taille), FVector2D(X + 50 * Taille + LargeurJauge, Y0 + 104 * Taille)),
	      Joueur->Rage / 100.0f, bRagePleine ? FLinearColor(1.0f, 0.47f + 0.3f * Pulsation, 0.16f) : FLinearColor(0.78f, 0.27f, 0.2f),
	      bRagePleine ? TEXT("PLEINE ! (touche 5)") : FString::Printf(TEXT("%d%%"), Joueur->Rage));
	FString Effets = FString::Printf(TEXT("Potions : %d"), Joueur->Potions);
	if (Joueur->bEnGarde)
	{
		Effets += TEXT("     EN GARDE");
	}
	Texte(Effets, X + 50 * Taille + LargeurJauge + 24 * Taille, Y0 + 52 * Taille, FLinearColor(0.9f, 0.78f, 0.43f), 1.0f);

	// Les 5 cartes d'actions
	const FVector2D Souris = [&]() { float SX = 0, SY = 0; Joueur->GetMousePosition(SX, SY); return FVector2D(SX, SY); }();
	int32 Survolee = -1;
	for (int32 i = 0; i < NombreDeCartes; i++)
	{
		const EVespAction Action = (EVespAction)i;
		const FBox2D Carte = RectangleCarte(i, Ecran);
		const bool bDispo = Joueur->ActionDisponible(Action);
		const bool bChoisie = Joueur->ActionChoisie == Action;
		const bool bSurvol = Carte.IsInside(Souris);
		if (bSurvol)
		{
			Survolee = i;
		}
		Cadre(Carte, bChoisie ? FLinearColor(0.28f, 0.21f, 0.09f, 0.95f) : (bSurvol ? CADRE_CLAIR : CADRE), bChoisie ? OR : BORD, bChoisie ? 3.0f : 1.0f);
		Texte(FString::Printf(TEXT("%d"), i + 1), Carte.Min.X + 8 * Taille, Carte.Min.Y + 6 * Taille, OR, 0.9f);
		Texte(AVespPlayerController::NomAction(Action), Carte.GetCenter().X, Carte.Min.Y + 26 * Taille, bDispo ? BLANC : FLinearColor(0.37f, 0.36f, 0.41f), 1.2f, true);
		Texte(AVespPlayerController::DetailAction(Action, Joueur->Potions), Carte.GetCenter().X, Carte.Min.Y + 62 * Taille,
		      bDispo ? GRIS : FLinearColor(0.31f, 0.3f, 0.35f), 0.85f, true);
	}
	// La bulle d'aide de la carte survolee
	if (Survolee >= 0)
	{
		const FString Aide = AVespPlayerController::NomAction((EVespAction)Survolee) + TEXT(" : ") + AVespPlayerController::AideAction((EVespAction)Survolee);
		float L = 0, H = 0;
		GetTextSize(Aide, L, H, GEngine->GetMediumFont(), Taille);
		const FBox2D Bulle(FVector2D(Ecran.X - L - 40 * Taille, Y0 - 44 * Taille), FVector2D(Ecran.X - 12 * Taille, Y0 - 10 * Taille));
		Cadre(Bulle, PANNEAU, OR, 2.0f);
		Texte(Aide, Bulle.Min.X + 14 * Taille, Bulle.Min.Y + 8 * Taille, BLANC, 1.0f);
	}

	// La consigne, au-dessus du panneau
	if (bTourAylis)
	{
		const FString Consigne = Joueur->bADejaBouge
			? TEXT("Clique sur un Haschen au contact   -   ESPACE pour passer")
			: TEXT("Deplace AYLIS (cases bleues), puis choisis une action et une cible");
		Texte(Consigne, Ecran.X / 2, Y0 - 34 * Taille, OR, 1.1f, true);
	}

	// ----- La fin du combat -----
	if (Joueur->Phase == EVespPhase::Victoire || Joueur->Phase == EVespPhase::Defaite)
	{
		const bool bVictoire = Joueur->Phase == EVespPhase::Victoire;
		DrawRect(bVictoire ? FLinearColor(0, 0, 0, 0.5f) : FLinearColor(0.08f, 0.03f, 0.14f, 0.75f), 0, 0, Ecran.X, Ecran.Y);
		const FBox2D Boite(FVector2D(Ecran.X / 2 - 330 * Taille, Ecran.Y / 2 - 90 * Taille), FVector2D(Ecran.X / 2 + 330 * Taille, Ecran.Y / 2 + 90 * Taille));
		const FLinearColor Couleur = bVictoire ? FLinearColor(0.43f, 0.86f, 0.47f) : FLinearColor(0.78f, 0.63f, 1.0f);
		Cadre(Boite, PANNEAU, Couleur, 3.0f);
		Texte(bVictoire ? TEXT("VICTOIRE !") : TEXT("La vision se brise..."), Ecran.X / 2, Boite.Min.Y + 34 * Taille, Couleur, 2.4f, true);
		Texte(TEXT("R : recommencer"), Ecran.X / 2, Boite.Min.Y + 118 * Taille, BLANC, 1.1f, true);
	}
}
