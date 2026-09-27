#include "VespHUD.h"
#include "VespPlayerController.h"
#include "VespCombat.h"
#include "VespUnite.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

// Au-dessus de chaque Haschen reveille : une barre de vie fine (verte, jaune puis rouge), le morceau blanc des
// degats qui vient de partir (il s'efface doucement), ses plaques d'armure, et ses etats (poison, brulure, gel).
// Les elites ont une barre doree et plus large ; le boss, lui, a sa grande barre en haut de l'ecran (l'interface).
void AVespHUD::DrawHUD()
{
	Super::DrawHUD();
	AVespPlayerController* Joueur = Cast<AVespPlayerController>(GetOwningPlayerController());
	if (!Joueur || !Canvas || !Joueur->Combat || Joueur->Phase != EVespPhase::Exploration)
	{
		return;
	}
	const float Taille = FMath::Clamp(Canvas->SizeY / 900.0f, 0.7f, 2.0f);
	for (const FVespHaschen& H : Joueur->Combat->GetHaschen())
	{
		const AVespUnite* U = H.U.Get();
		if (!U || !U->EstDebout() || U->IsHidden() || H.bBoss || H.Etat == EVespIntention::Dormir)
		{
			continue;
		}
		if (FVector::Dist2D(U->GetActorLocation(), Joueur->Aylis->GetActorLocation()) > 2600.0f)
		{
			continue;
		}
		const FVector Ecran = Project(U->GetActorLocation() + FVector(0, 0, U->Taille + 40.0f), false);
		if (Ecran.Z <= 0)
		{
			continue;	// derriere la camera
		}
		const float PvMax = FMath::Max(1, U->Stats.PvMax);
		const float Part = FMath::Clamp(U->Stats.Pv / PvMax, 0.0f, 1.0f);
		const float Perdu = FMath::Clamp(U->DegatsRecus / PvMax, 0.0f, 1.0f - Part);
		const float L = (H.bElite ? 96.0f : 64.0f) * Taille;
		const float Ht = (H.bElite ? 7.0f : 5.0f) * Taille;
		const float X = Ecran.X - L / 2, Y = Ecran.Y;
		FLinearColor Couleur = Part <= 0.25f ? FLinearColor(0.9f, 0.32f, 0.3f) : (Part <= 0.5f ? FLinearColor(0.92f, 0.76f, 0.3f) : FLinearColor(0.4f, 0.82f, 0.5f));
		if (H.bElite) Couleur = FLinearColor(0.95f, 0.72f, 0.3f);
		DrawRect(FLinearColor(0, 0, 0, 0.7f), X - 2, Y - 2, L + 4, Ht + 4);			// un liseré sombre
		DrawRect(FLinearColor(0.1f, 0.08f, 0.14f, 1.0f), X, Y, L, Ht);
		DrawRect(FLinearColor(1.0f, 0.95f, 0.85f, 0.9f), X + L * Part, Y, L * Perdu, Ht);	// les degats qui s'effacent
		DrawRect(Couleur, X, Y, L * Part, Ht);
		DrawRect(FLinearColor(1, 1, 1, 0.25f), X, Y, L * Part, Ht * 0.35f);			// un reflet
		// Il va frapper : un petit losange rouge au-dessus de la barre
		if (H.Etat == EVespIntention::Preparer)
		{
			const float D = 9.0f * Taille;
			DrawRect(FLinearColor(1.0f, 0.25f, 0.15f, 0.95f), Ecran.X - D / 2, Y - D - 6.0f * Taille, D, D);
		}
		// Les plaques d'armure : des petits carres gris au-dessus de la barre (dores s'il est sonne)
		const float Carre = 7.0f * Taille;
		for (int32 i = 0; i < U->Stats.ArmureMax; i++)
		{
			const bool bPleine = i < U->Stats.Armure;
			DrawRect(FLinearColor(0, 0, 0, 0.7f), X + i * (Carre + 3) - 1, Y - Carre - 5, Carre + 2, Carre + 2);
			DrawRect(U->TempsBrise > 0.0f ? FLinearColor(1.0f, 0.8f, 0.3f) : (bPleine ? FLinearColor(0.8f, 0.8f, 0.88f) : FLinearColor(0.2f, 0.2f, 0.24f)),
			         X + i * (Carre + 3), Y - Carre - 4, Carre, Carre);
		}
		// Les etats, a droite de la barre
		float Decalage = L + 5;
		auto Etat = [&](bool bActif, const FLinearColor& C) {
			if (bActif)
			{
				DrawRect(C, X + Decalage, Y, Ht, Ht);
				Decalage += Ht + 3;
			}
		};
		Etat(U->Poison > 0.0f, FLinearColor(0.7f, 0.95f, 0.4f));
		Etat(U->Brulure > 0.0f, FLinearColor(1.0f, 0.55f, 0.2f));
		Etat(U->Gel > 0.0f, FLinearColor(0.6f, 0.82f, 1.0f));
		Etat(U->Etourdi > 0.0f, FLinearColor(1.0f, 0.9f, 0.5f));
	}
}
