// VespHUD : ce qui s'affiche dans l'arene elle-meme, au-dessus des personnages : leur barre de vie.
// (Tout le reste de l'interface est dans VespInterface, en Slate.)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VespHUD.generated.h"

UCLASS()
class VESPERANCE_API AVespHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
