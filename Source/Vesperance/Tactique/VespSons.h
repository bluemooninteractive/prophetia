// VespSons : tous les sons du jeu, fabriques par le code (aucun fichier son n'est necessaire).
//
// Au premier besoin, chaque son est "synthetise" : du bruit filtre (un souffle d'epee, un pas dans l'herbe),
// des oscillateurs (une cloche, un accord), des enveloppes (l'attaque et l'extinction), un peu d'echo.
// Chaque son a plusieurs variantes (deux coups d'epee ne sonnent jamais tout a fait pareil).
// Les ambiances (le vent, la pluie, les grillons, les marais) et la musique de chaque acte tournent en boucle.
//
// L'oreille du joueur est posee sur AYLIS (pas sur la camera, qui est loin) : les sons autour d'elle sont
// nets, ceux du bout de la clairiere plus doux, et a gauche ou a droite selon l'ecran.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VespSons.generated.h"

class USoundWaveProcedural;
class UAudioComponent;
class USoundAttenuation;

UENUM()
enum class EVespSon : uint8
{
	Pas,
	Frappe,			// le souffle d'une lame
	FrappeLourde,
	Impact,			// un coup qui touche
	ImpactArmure,	// un coup qui glisse sur une armure
	Critique,
	Parade,			// un coup arrete par le bouclier
	Esquive,
	Blessure,		// AYLIS est touchee
	Os,				// un Haschen (un squelette) s'effondre
	Tir,			// une fleche part
	Sort,			// un sort part
	Explosion,
	Soin,
	Potion,
	Ramasser,
	Eclats,			// la monnaie
	Niveau,			// un niveau de plus
	Rune,
	Clic,			// l'interface
	Survol,
	Barriere,		// la clairiere se ferme
	Rugissement,	// un boss
	Annonce,		// une attaque se prepare
	Tonnerre,
	Victoire,		// la clairiere est liberee
	Coffre,
	Glace,
	Feu,
	Pouvoir,		// un pouvoir du Seuil
	Nombre UMETA(Hidden)
};

UENUM()
enum class EVespAmbiance : uint8
{
	Aucune,
	Foret,			// le vent dans les arbres, les grillons, une chouette
	Marais,			// les grenouilles, les bulles
	Vent,			// le col, la forteresse
	Braises,		// les Terres de Cendre
	Voile,			// Karn : un bourdonnement etrange
};

UCLASS()
class VESPERANCE_API UVespSons : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// Un son a un endroit du monde (Volume : 1 = normal ; Hauteur : 1 = normal, 1.2 = plus aigu)
	static void Jouer(const UObject* Contexte, EVespSon Son, const FVector& Position, float Volume = 1.0f, float Hauteur = 1.0f);
	// Un son "dans la tete" (l'interface)
	static void Jouer2D(const UObject* Contexte, EVespSon Son, float Volume = 1.0f, float Hauteur = 1.0f);

	// L'oreille : ou elle est, et vers ou elle regarde (celle de la camera)
	void PlacerOreille(APlayerController* Joueur, const FVector& Position, const FRotator& Regard);

	// L'ambiance et la musique d'un acte (elles tournent en boucle et se croisent en fondu)
	void AmbianceDeLActe(int32 Acte);
	// En combat, la musique se fait plus intense (des tambours) ; Boss : encore plus
	void Tension(float Niveau);
	void Pluie(float Force);		// 0 : pas de pluie
	void CouperTout();

	virtual void Deinitialize() override;
	virtual void Tick(float Secondes) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVespSons, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type Type) const override { return Type == EWorldType::Game || Type == EWorldType::PIE; }

private:
	struct FSonJoue
	{
		TObjectPtr<USoundWaveProcedural> Onde;
		double Fin = 0.0;
	};
	USoundWaveProcedural* Creer(const TSharedPtr<TArray<uint8>>& Donnees, bool bBoucle);
	UAudioComponent* Boucle(const TSharedPtr<TArray<uint8>>& Donnees, float Volume);
	void Lancer(EVespSon Son, const FVector* Position, float Volume, float Hauteur);

	UPROPERTY() TArray<TObjectPtr<USoundWaveProcedural>> Vivants;
	TArray<double> FinsDesVivants;
	UPROPERTY() TObjectPtr<USoundAttenuation> Attenuation;
	UPROPERTY() TObjectPtr<UAudioComponent> Ambiance;
	UPROPERTY() TObjectPtr<UAudioComponent> Musique;
	UPROPERTY() TObjectPtr<UAudioComponent> Tambours;
	UPROPERTY() TObjectPtr<UAudioComponent> BouclePluie;
	int32 ActeJoue = 0;
	float TensionVoulue = 0.0f;
	float TensionActuelle = 0.0f;
	float PluieVoulue = 0.0f;
	float PluieActuelle = 0.0f;
	double DernierTick = 0.0;
	TMap<uint8, double> DerniereFois;		// pour ne pas empiler dix fois le meme son dans la meme image
};
