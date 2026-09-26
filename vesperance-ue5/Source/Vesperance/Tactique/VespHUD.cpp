#include "VespHUD.h"
#include "VespPlayerController.h"
#include "VespGrille.h"
#include "VespUnite.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

// Au-dessus de chaque personnage debout : une barre de vie fine (bleue pour AYLIS ; verte, jaune puis rouge pour
// les Haschen ; doree et plus large pour un boss), ses plaques d'armure, et ses etats (poison, brulure, gel).
// Au survol d'un Haschen : son nom, ses stats et ses talents.
void AVespHUD::DrawHUD()
{
	Super::DrawHUD();
	AVespPlayerController* Joueur = Cast<AVespPlayerController>(GetOwningPlayerController());
	if (!Joueur || !Canvas || !(Joueur->Phase == EVespPhase::TourAylis || Joueur->Phase == EVespPhase::TourHaschen))
	{
		return;
	}
	const float Taille = FMath::Clamp(Canvas->SizeY / 900.0f, 0.7f, 2.0f);
	for (TActorIterator<AVespUnite> It(GetWorld()); It; ++It)
	{
		const AVespUnite* U = *It;
		if (!U->EstDebout())
		{
			continue;
		}
		const float Hauteur = U->Taille + 45.0f;
		const FVector Ecran = Project(U->GetActorLocation() + FVector(0, 0, Hauteur), false);
		if (Ecran.Z <= 0)
		{
			continue;	// derriere la camera
		}
		const float Part = FMath::Clamp((float)U->Stats.Pv / FMath::Max(1, U->Stats.PvMax), 0.0f, 1.0f);
		const float L = (U->Stats.Boss > 0 ? 130.0f : 70.0f) * Taille;
		const float H = (U->Stats.Boss > 0 ? 9.0f : 6.0f) * Taille;
		const float X = Ecran.X - L / 2, Y = Ecran.Y;
		FLinearColor Couleur = Part <= 0.25f ? FLinearColor(0.9f, 0.32f, 0.3f) : (Part <= 0.5f ? FLinearColor(0.92f, 0.76f, 0.3f) : FLinearColor(0.4f, 0.82f, 0.5f));
		if (U->EstAylis()) Couleur = FLinearColor(0.45f, 0.62f, 1.0f);
		if (U->Stats.Boss > 0) Couleur = FLinearColor(0.95f, 0.72f, 0.3f);
		DrawRect(FLinearColor(0, 0, 0, 0.75f), X - 2, Y - 2, L + 4, H + 4);			// un liseré sombre
		DrawRect(FLinearColor(0.1f, 0.08f, 0.14f, 1.0f), X, Y, L, H);
		DrawRect(Couleur, X, Y, L * Part, H);
		DrawRect(FLinearColor(1, 1, 1, 0.25f), X, Y, L * Part, H * 0.35f);			// un reflet
		// Les plaques d'armure : des petits carres gris au-dessus de la barre (dores s'il est sonne)
		const float Carre = 7.0f * Taille;
		for (int32 i = 0; i < U->Stats.ArmureMax; i++)
		{
			const bool bPleine = i < U->Stats.Armure;
			DrawRect(FLinearColor(0, 0, 0, 0.7f), X + i * (Carre + 3) - 1, Y - Carre - 5, Carre + 2, Carre + 2);
			DrawRect(U->TempsBrise > 0 ? FLinearColor(1.0f, 0.8f, 0.3f) : (bPleine ? FLinearColor(0.8f, 0.8f, 0.88f) : FLinearColor(0.2f, 0.2f, 0.24f)),
			         X + i * (Carre + 3), Y - Carre - 4, Carre, Carre);
		}
		// Les etats, a droite de la barre
		float Decalage = L + 5;
		auto Etat = [&](bool bActif, const FLinearColor& C) {
			if (bActif)
			{
				DrawRect(C, X + Decalage, Y, H, H);
				Decalage += H + 3;
			}
		};
		Etat(U->Poison > 0, FLinearColor(0.7f, 0.95f, 0.4f));
		Etat(U->Brulure > 0, FLinearColor(1.0f, 0.55f, 0.2f));
		Etat(U->Gel > 0, FLinearColor(0.6f, 0.82f, 1.0f));
	}

	// La fiche du Haschen sous la souris
	FIntPoint Case;
	if (!Joueur->CaseVisee(Case) || !Joueur->Grille)
	{
		return;
	}
	const AVespUnite* U = Joueur->Grille->UniteSur(Case);
	if (!U || U->EstAylis() || !U->EstDebout())
	{
		return;
	}
	TArray<FString> Lignes;
	Lignes.Add(FString::Printf(TEXT("%s   %d / %d pv"), *U->Stats.Nom, U->Stats.Pv, U->Stats.PvMax));
	const TCHAR* Styles[] = {TEXT("corps a corps"), TEXT("tireur"), TEXT("chargeur")};
	Lignes.Add(FString::Printf(TEXT("attaque %d  -  defense %d  -  %s"), U->Stats.Attaque, U->Stats.Defense, Styles[(int32)U->Stats.Style]));
	FString Talents;
	auto Ajouter = [&Talents](const TCHAR* T) { Talents += Talents.IsEmpty() ? FString(T) : FString(TEXT(", ")) + T; };
	if (U->Stats.Effet == VespEffetCoup::Poison) Ajouter(TEXT("empoisonne"));
	if (U->Stats.Effet == VespEffetCoup::Gel) Ajouter(TEXT("gele"));
	if (U->Stats.Effet == VespEffetCoup::Brulure) Ajouter(TEXT("brule"));
	if (U->Stats.Capacites & VespCapacite::Soigneur) Ajouter(TEXT("soigne ses allies"));
	if (U->Stats.Capacites & VespCapacite::Explosif) Ajouter(TEXT("explose en tombant"));
	if (U->Stats.Capacites & VespCapacite::Invocateur) Ajouter(TEXT("appelle des renforts"));
	if (U->Stats.Capacites & VespCapacite::Sauteur) Ajouter(TEXT("bondit"));
	if (U->Stats.Capacites & VespCapacite::Vampire) Ajouter(TEXT("vole la vie"));
	if (U->Stats.Capacites & VespCapacite::Attire) Ajouter(TEXT("attire vers lui"));
	if (U->Stats.ArmureMax > 0) Ajouter(TEXT("armure : seuls les coups lourds la fissurent"));
	if (!Talents.IsEmpty())
	{
		Lignes.Add(Talents);
	}
	float SourisX = 0, SourisY = 0;
	Joueur->GetMousePosition(SourisX, SourisY);
	UFont* Police = GEngine->GetSmallFont();
	const float Echelle = 1.25f * Taille;
	float Largeur = 0;
	for (const FString& Ligne : Lignes)
	{
		float W = 0, Ht = 0;
		GetTextSize(Ligne, W, Ht, Police, Echelle);
		Largeur = FMath::Max(Largeur, W);
	}
	const float Interligne = 18.0f * Taille;
	const float BX = FMath::Min(SourisX + 22, Canvas->SizeX - Largeur - 30), BY = SourisY + 18;
	DrawRect(FLinearColor(0.02f, 0.015f, 0.04f, 0.9f), BX - 10, BY - 8, Largeur + 20, Lignes.Num() * Interligne + 14);
	DrawRect(FLinearColor(0.73f, 0.55f, 1.0f, 0.8f), BX - 10, BY - 8, 3, Lignes.Num() * Interligne + 14);
	for (int32 i = 0; i < Lignes.Num(); i++)
	{
		DrawText(Lignes[i], i == 0 ? FLinearColor(1.0f, 0.89f, 0.6f) : FLinearColor(0.85f, 0.82f, 0.92f), BX, BY + i * Interligne, Police, Echelle);
	}
}
