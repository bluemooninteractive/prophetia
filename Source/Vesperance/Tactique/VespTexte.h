// VespTexte : les petits outils de texte du jeu.
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"

// Les majuscules, accents compris (FString::ToUpper ne touche pas aux lettres accentuees : "Crânes" donnait "CRâNES")
inline FString VespMajuscules(const FString& Texte)
{
	FString M = Texte.ToUpper();
	static const TCHAR* MINUSCULES = TEXT("àâäéèêëîïôöùûüçœæ");
	static const TCHAR* MAJUSCULES = TEXT("ÀÂÄÉÈÊËÎÏÔÖÙÛÜÇŒÆ");
	for (TCHAR& C : M.GetCharArray())
	{
		for (int32 i = 0; MINUSCULES[i]; i++)
		{
			if (C == MINUSCULES[i])
			{
				C = MAJUSCULES[i];
				break;
			}
		}
	}
	return M;
}

// Un pinceau Slate pour une texture du jeu (icones de sorts, de potions), garde en vie tant que le jeu tourne
inline const FSlateBrush* VespPinceau(const FString& CheminTexture)
{
	static TMap<FString, TSharedPtr<FSlateBrush>> Pinceaux;
	if (TSharedPtr<FSlateBrush>* Deja = Pinceaux.Find(CheminTexture))
	{
		return Deja->Get();
	}
	TSharedPtr<FSlateBrush> B = MakeShared<FSlateBrush>();
	B->DrawAs = ESlateBrushDrawType::NoDrawType;
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *CheminTexture, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		T->AddToRoot();
		B->SetResourceObject(T);
		B->ImageSize = FVector2D(T->GetSizeX(), T->GetSizeY());
		B->DrawAs = ESlateBrushDrawType::Image;
	}
	Pinceaux.Add(CheminTexture, B);
	return B.Get();
}

// L'icone d'une etoile du Seuil (Spell_Mix), et celle de la potion (Vol01_Potions)
inline const FSlateBrush* VespIconeEtoile(int32 Etoile)
{
	static const int32 NUMEROS[12] = {28, 40, 10, 8, 17, 9, 15, 27, 20, 1, 6, 37};
	const int32 N = NUMEROS[FMath::Clamp(Etoile, 0, 11)];
	return VespPinceau(FString::Printf(TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_%02d.T_spells_mix_frame_%02d"), N, N));
}

inline const FSlateBrush* VespIconePotion()
{
	return VespPinceau(TEXT("/Game/Vol01_Potions/ruby_red_healing_potion.ruby_red_healing_potion"));
}
