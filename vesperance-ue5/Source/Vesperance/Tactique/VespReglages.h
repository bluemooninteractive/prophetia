// VespReglages : les options du joueur (le son, le confort). Une sauvegarde a part de la memoire de la boucle :
// elles ne dependent pas de la partie, et survivent meme si on efface la progression.
// (Le plein ecran, lui, est garde par Unreal dans GameUserSettings.)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VespReglages.generated.h"

UCLASS()
class VESPERANCE_API UVespReglages : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() float VolumeMusique = 1.0f;		// de 0 a 1 (la musique et les tambours du combat)
	UPROPERTY() float VolumeEffets = 1.0f;		// de 0 a 1 (les coups, l'interface, l'ambiance de l'acte)
	UPROPERTY() bool bSecousses = true;			// la camera tremble aux coups forts
};
