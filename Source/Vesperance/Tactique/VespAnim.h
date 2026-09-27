// VespAnim : les animations d'un personnage, melangees en douceur (sans Animation Blueprint).
//
// Trois "couches" se superposent a chaque image :
//   1. la locomotion : repos, marche et course, melangees selon la vitesse (les pas restent synchronises)
//   2. la posture : une boucle tenue sur le haut du corps (la garde au bouclier)
//   3. l'action : une animation jouee une fois (une attaque, un coup recu, une esquive, une potion...),
//      sur tout le corps ou seulement le haut (AYLIS peut frapper en courant : les jambes continuent de courir)
// Chaque changement se fait en fondu (un dixieme de seconde), au lieu de sauter d'une pose a l'autre.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "VespAnim.generated.h"

class UAnimSequence;

// Une animation en train de jouer
struct FVespCouche
{
	const UAnimSequence* Anim = nullptr;
	float Temps = 0.0f;
	float Poids = 0.0f;			// de 0 a 1
	bool bBoucle = true;
};

// Ce que le calcul des poses a besoin de savoir (une copie, prise a chaque image)
struct FVespEtatAnim
{
	FVespCouche Loco[3];		// repos, marche, course
	FVespCouche Posture;
	FVespCouche ActionAvant;	// l'action d'avant, qui s'efface pendant que la nouvelle arrive (un enchainement)
	FVespCouche Action;
	bool bAvantHautDuCorps = false;
	bool bActionHautDuCorps = false;
};

USTRUCT()
struct FVespAnimProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FVespAnimProxy() {}
	FVespAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual bool Evaluate(FPoseContext& Output) override;

	FVespEtatAnim Etat;			// recopie par l'instance a chaque image

private:
	void PreparerMasque(const FBoneContainer& Os);
	void Superposer(FPoseContext& Sortie, const FVespCouche& Couche, bool bHautDuCorps);
	TArray<float> Masque;			// pour chaque os : 1 s'il fait partie du haut du corps (la colonne et au-dessus)
	uint16 SerieMasque = 0;
	bool bMasquePret = false;
};

UCLASS(Transient, NotBlueprintable)
class VESPERANCE_API UVespAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	friend struct FVespAnimProxy;

public:
	// Les animations de base, et les vitesses (cm/s) auxquelles la marche et la course sont "naturelles"
	void Locomotion(const UAnimSequence* Repos, const UAnimSequence* Marche, const UAnimSequence* Course,
	                float VitesseDeMarche = 260.0f, float VitesseDeCourse = 560.0f);
	void ChangerRepos(const UAnimSequence* Repos);		// la garde de combat, ou le repos tranquille
	void SetVitesse(float CmParSeconde) { VitesseVoulue = CmParSeconde; }

	// Une action jouee une fois. Renvoie sa duree (en secondes). Tenir : elle reste sur sa derniere pose (la chute).
	float JouerAction(const UAnimSequence* Anim, float Vitesse = 1.0f, bool bHautDuCorps = false, bool bTenir = false,
	                  float Entree = 0.08f, float Sortie = 0.16f);
	void ArreterAction(float Sortie = 0.12f);
	void SetVitesseAction(float Vitesse) { VitesseAction = FMath::Max(0.05f, Vitesse); }
	bool ActionEnCours() const;
	float ProgressionAction() const;		// de 0 a 1
	const UAnimSequence* ActionActuelle() const { return Etat.Action.Anim; }

	// Une posture tenue sur le haut du corps (nullptr pour la quitter)
	void SetPosture(const UAnimSequence* Boucle);

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	FVespEtatAnim Etat;
	float VitesseVoulue = 0.0f;
	float VitesseLissee = 0.0f;
	float VitesseMarche = 260.0f;
	float VitesseCourse = 560.0f;
	float Phase = 0.0f;					// la phase commune de la marche et de la course (0 a 1)
	float VitesseAction = 1.0f;
	float EntreeAction = 0.08f;
	float SortieAction = 0.16f;
	bool bTenirAction = false;
	bool bSortieDemandee = false;
	const UAnimSequence* PostureVoulue = nullptr;
};
