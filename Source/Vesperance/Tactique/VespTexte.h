// VespTexte : les petits outils de texte du jeu.
#pragma once

#include "CoreMinimal.h"

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
