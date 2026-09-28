// VespPartie : la vision en cours, mise de cote quand on quitte le jeu, pour la reprendre plus tard.
//
// Elle est ecrite a chaque retour au calme (debut d'un acte, apres une clairiere et sa rune, un evenement, le marchand,
// un dialogue) et quand on quitte hors combat ; effacee a la chute, a la victoire, quand on abandonne ou qu'on lance
// une nouvelle vision. A la reprise, l'acte est reconstruit a l'identique (meme carte, meme decor, grace aux graines),
// les clairieres deja faites restent faites, et AYLIS retrouve tout ce qu'elle portait.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VespUnite.h"
#include "VespProgression.h"
#include "VespPartie.generated.h"

// Un objet du sac ou de l'equipement, tel qu'on le range dans la sauvegarde
USTRUCT()
struct FVespObjetSauve
{
	GENERATED_BODY()

	UPROPERTY() FString Nom;
	UPROPERTY() FString Recit;
	UPROPERTY() uint8 Emplacement = 0;
	UPROPERTY() uint8 Rarete = 0;
	UPROPERTY() int32 Niveau = 1;
	UPROPERTY() int32 Attaque = 0;
	UPROPERTY() int32 Defense = 0;
	UPROPERTY() int32 PvMax = 0;
	UPROPERTY() int32 Critique = 0;
	UPROPERTY() float Vitesse = 0.0f;
	UPROPERTY() int32 Effet = 0;
	UPROPERTY() float ChanceEffet = 0.0f;
	UPROPERTY() int32 VolDeVie = 0;
	UPROPERTY() FString Modele;
	UPROPERTY() FString SecondModele;
	UPROPERTY() float Longueur = 0.5f;
	UPROPERTY() uint8 TypeArme = 0;
	UPROPERTY() FLinearColor Teinte = FLinearColor::White;
	UPROPERTY() int32 Icone = 0;
	UPROPERTY() uint32 Graine = 0;

	static FVespObjetSauve De(const FVespObjet& O)
	{
		FVespObjetSauve S;
		S.Nom = O.Nom; S.Recit = O.Recit; S.Emplacement = (uint8)O.Emplacement; S.Rarete = (uint8)O.Rarete; S.Niveau = O.Niveau;
		S.Attaque = O.Attaque; S.Defense = O.Defense; S.PvMax = O.PvMax; S.Critique = O.Critique; S.Vitesse = O.Vitesse;
		S.Effet = O.Effet; S.ChanceEffet = O.ChanceEffet; S.VolDeVie = O.VolDeVie; S.Modele = O.Modele; S.SecondModele = O.SecondModele;
		S.Longueur = O.Longueur; S.TypeArme = (uint8)O.TypeArme; S.Teinte = O.Teinte; S.Icone = O.Icone; S.Graine = O.Graine;
		return S;
	}

	FVespObjet Objet() const
	{
		FVespObjet O;
		O.Nom = Nom; O.Recit = Recit; O.Emplacement = (EVespEmplacement)Emplacement; O.Rarete = (EVespRarete)Rarete; O.Niveau = Niveau;
		O.Attaque = Attaque; O.Defense = Defense; O.PvMax = PvMax; O.Critique = Critique; O.Vitesse = Vitesse;
		O.Effet = Effet; O.ChanceEffet = ChanceEffet; O.VolDeVie = VolDeVie; O.Modele = Modele; O.SecondModele = SecondModele;
		O.Longueur = Longueur; O.TypeArme = (EVespArme)TypeArme; O.Teinte = Teinte; O.Icone = Icone; O.Graine = Graine;
		return O;
	}
};

UCLASS()
class VESPERANCE_API UVespPartie : public USaveGame
{
	GENERATED_BODY()

public:
	// Ou en est la vision
	UPROPERTY() int32 NumeroVision = 1;
	UPROPERTY() int32 Acte = 1;
	UPROPERTY() int32 GraineVision = 0;			// le decor
	UPROPERTY() int32 GraineCarte = 0;			// les clairieres et les sentiers de l'acte
	UPROPERTY() TArray<bool> Visites;			// les clairieres deja faites
	UPROPERTY() FVector Position = FVector::ZeroVector;

	// AYLIS
	UPROPERTY() FVespStats Stats;
	UPROPERTY() int32 Potions = 0;
	UPROPERTY() int32 Rage = 0;
	UPROPERTY() int32 Eclats = 0;
	UPROPERTY() int32 Niveau = 1;
	UPROPERTY() int32 Xp = 0;
	UPROPERTY() int32 PointsDeCompetence = 0;
	UPROPERTY() int32 HaschenVaincus = 0;
	UPROPERTY() float BonusVitesse = 0.0f;
	UPROPERTY() TArray<int32> Runes;
	UPROPERTY() int32 EffetsDesRunes = 0;		// flamme, seve, fureur, sangsue, fortune, givre, pied sur, epines (un bit chacun)
	UPROPERTY() float BonusParade = 0.0f;
	UPROPERTY() TArray<bool> Seuil;
	UPROPERTY() TArray<FVespObjetSauve> Sac;
	UPROPERTY() TArray<FVespObjetSauve> Equipement;
	UPROPERTY() TArray<bool> Equipe;

	// Le chronometre, et ce que la vision a deja gagne ou dit
	UPROPERTY() float ChronoPartie = 0.0f;
	UPROPERTY() float ChronoActe = 0.0f;
	UPROPERTY() TArray<float> TempsDesActes;
	UPROPERTY() int32 SouvenirsDeLaVision = 0;
	UPROPERTY() bool bSouvenirsPossibles = true;
	UPROPERTY() bool bSecondSouffle = false;
	UPROPERTY() TArray<FString> DejaDit;
};
