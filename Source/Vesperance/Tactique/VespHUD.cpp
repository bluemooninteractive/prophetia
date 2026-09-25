#include "VespHUD.h"
#include "VespPlayerController.h"
#include "VespUnite.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

// Une barre de vie fine au-dessus de chaque personnage debout (bleue pour AYLIS ; verte, jaune puis rouge
// pour les Haschen ; doree et plus large pour un boss)
void AVespHUD::DrawHUD()
{
	Super::DrawHUD();
	const AVespPlayerController* Joueur = Cast<AVespPlayerController>(GetOwningPlayerController());
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
		const float Hauteur = U->Stats.Boss > 0 ? 300.0f : 210.0f;
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
		if (U->Poison > 0)
		{
			DrawRect(FLinearColor(0.7f, 0.95f, 0.4f), X + L + 5, Y, H, H);			// un petit carre vert : empoisonne
		}
	}
}
