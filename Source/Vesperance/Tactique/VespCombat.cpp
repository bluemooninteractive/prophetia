#include "VespCombat.h"
#include "VespMonde.h"
#include "VespEffet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// Le dossier ou chaque personnage range son modele 3D
static const FString DOSSIER = TEXT("/Game/Characters/");

// Les types de clairieres (les memes valeurs que EVespSalle)
namespace
{
	constexpr int32 COMBAT = 0, ELITE = 1, BOSS_SALLE = 5;
}

// ===================== Les Haschen de chaque acte =====================
// nom, dossier du modele 3D, pv, attaque, defense, couleur, style, effet de ses coups, talents,
// armure, bonus de vitesse, portee de tir (x 1,7 m), chances de critique, taille (cm)

struct FVespModeleHaschen
{
	const TCHAR* Nom;
	const TCHAR* Dossier;
	int32 Pv, Attaque, Defense;
	FLinearColor Teinte;
	EVespStyle Style;
	int32 Effet;
	int32 Capacites;
	int32 Armure;
	int32 Pas;
	int32 Portee;
	int32 Critique;
	float Taille;
};

namespace
{
	constexpr EVespStyle MELEE = EVespStyle::Melee, LANCEUR = EVespStyle::Lanceur, CHARGEUR = EVespStyle::Chargeur;
	constexpr int32 POISON = VespEffetCoup::Poison, GEL = VespEffetCoup::Gel, BRULURE = VespEffetCoup::Brulure;
	constexpr int32 SOIGNEUR = VespCapacite::Soigneur, EXPLOSIF = VespCapacite::Explosif, INVOCATEUR = VespCapacite::Invocateur;
	constexpr int32 SAUTEUR = VespCapacite::Sauteur, VAMPIRE = VespCapacite::Vampire, ATTIRE = VespCapacite::Attire;
	const FLinearColor OR(0.95f, 0.75f, 0.2f);
}

static const FVespModeleHaschen HASCHEN[7][5] = {
	{	// I. La Foret des Brumes : les eclaireurs d'Ashka
		{TEXT("Haschen guerrier"), TEXT("guerrier"), 22, 9, 2, FLinearColor(0.8f, 0.2f, 0.15f), MELEE, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen traqueur"), TEXT("traqueur"), 20, 9, 1, FLinearColor(0.2f, 0.6f, 0.25f), LANCEUR, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen chaman"), TEXT("chaman"), 20, 10, 1, FLinearColor(0.5f, 0.25f, 0.7f), MELEE, POISON, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen porte-bouclier"), TEXT("guerrier"), 28, 8, 5, FLinearColor(0.45f, 0.45f, 0.55f), MELEE, 0, 0, 0, 0, 4, 5, 185.0f},
		{TEXT("Haschen eclaireur"), TEXT("sbire"), 18, 8, 1, FLinearColor(0.9f, 0.5f, 0.1f), CHARGEUR, 0, 0, 0, 1, 4, 12, 165.0f},
	},
	{	// II. Le Bois des Pendus
		{TEXT("Haschen louvetier"), TEXT("sbire"), 24, 11, 2, FLinearColor(0.45f, 0.3f, 0.2f), CHARGEUR, 0, 0, 0, 1, 4, 10, 170.0f},
		{TEXT("Haschen chaman"), TEXT("chaman"), 24, 11, 1, FLinearColor(0.5f, 0.25f, 0.7f), MELEE, POISON, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen traqueur"), TEXT("traqueur"), 24, 10, 1, FLinearColor(0.2f, 0.6f, 0.25f), LANCEUR, 0, 0, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen des cordes"), TEXT("traqueur"), 22, 12, 1, FLinearColor(0.35f, 0.55f, 0.35f), LANCEUR, POISON, ATTIRE, 0, 0, 4, 10, 170.0f},
		{TEXT("Haschen brute"), TEXT("guerrier"), 36, 12, 4, FLinearColor(0.5f, 0.15f, 0.15f), CHARGEUR, 0, 0, 0, 0, 4, 10, 205.0f},
	},
	{	// III. Les Marais de Sombreval
		{TEXT("Noyé"), TEXT("guerrier"), 36, 13, 3, FLinearColor(0.2f, 0.45f, 0.4f), MELEE, 0, 0, 0, 0, 4, 10, 180.0f},
		{TEXT("Crapaud cracheur"), TEXT("sbire"), 26, 12, 1, FLinearColor(0.4f, 0.55f, 0.2f), LANCEUR, POISON, 0, 0, 0, 4, 10, 130.0f},
		{TEXT("Sangsue des vases"), TEXT("sbire"), 28, 12, 2, FLinearColor(0.35f, 0.15f, 0.2f), MELEE, 0, VAMPIRE, 0, 2, 4, 10, 160.0f},
		{TEXT("Pêcheur d'âmes"), TEXT("traqueur"), 28, 13, 2, FLinearColor(0.25f, 0.35f, 0.5f), LANCEUR, 0, ATTIRE, 0, 0, 5, 10, 175.0f},
		{TEXT("Sorciere des vases"), TEXT("chaman"), 28, 12, 1, FLinearColor(0.3f, 0.5f, 0.3f), LANCEUR, POISON, SOIGNEUR, 0, 0, 4, 10, 175.0f},
	},
	{	// IV. La forteresse d'Ashka
		{TEXT("Sentinelle"), TEXT("guerrier"), 38, 14, 5, FLinearColor(0.5f, 0.45f, 0.35f), MELEE, 0, 0, 1, 0, 4, 10, 190.0f},
		{TEXT("Arbaletrier"), TEXT("traqueur"), 32, 16, 2, FLinearColor(0.55f, 0.35f, 0.2f), LANCEUR, 0, 0, 0, 0, 5, 15, 175.0f},
		{TEXT("Rodeur des ruines"), TEXT("sbire"), 30, 15, 2, FLinearColor(0.6f, 0.5f, 0.3f), MELEE, 0, SAUTEUR, 0, 1, 4, 15, 170.0f},
		{TEXT("Gardien d'autel"), TEXT("chaman"), 32, 13, 2, FLinearColor(0.8f, 0.7f, 0.4f), MELEE, 0, SOIGNEUR, 0, 0, 4, 10, 175.0f},
		{TEXT("Golem mineur"), TEXT("guerrier"), 44, 15, 4, FLinearColor(0.45f, 0.45f, 0.45f), MELEE, 0, 0, 2, 0, 4, 5, 205.0f},
	},
	{	// V. Le col gele
		{TEXT("Loup des neiges"), TEXT("sbire"), 36, 16, 2, FLinearColor(0.85f, 0.88f, 0.95f), CHARGEUR, 0, 0, 0, 3, 4, 12, 165.0f},
		{TEXT("Chaman du givre"), TEXT("chaman"), 36, 15, 2, FLinearColor(0.4f, 0.6f, 0.9f), LANCEUR, GEL, 0, 0, 0, 4, 10, 175.0f},
		{TEXT("Yeti Haschen"), TEXT("guerrier"), 55, 18, 4, FLinearColor(0.9f, 0.9f, 0.95f), MELEE, GEL, 0, 0, 0, 4, 10, 225.0f},
		{TEXT("Archer d'Ashka"), TEXT("traqueur"), 36, 17, 2, FLinearColor(0.6f, 0.2f, 0.3f), LANCEUR, 0, 0, 0, 0, 5, 15, 175.0f},
		{TEXT("Givrin"), TEXT("sbire"), 28, 14, 1, FLinearColor(0.5f, 0.8f, 1.0f), CHARGEUR, GEL, EXPLOSIF, 0, 1, 4, 10, 130.0f},
	},
	{	// VI. Les Terres de Cendre
		{TEXT("Incendiaire"), TEXT("sbire"), 36, 17, 2, FLinearColor(1.0f, 0.4f, 0.1f), CHARGEUR, BRULURE, EXPLOSIF, 0, 1, 4, 10, 160.0f},
		{TEXT("Forgeron Haschen"), TEXT("guerrier"), 50, 18, 5, FLinearColor(0.4f, 0.25f, 0.2f), MELEE, BRULURE, 0, 1, 0, 4, 10, 195.0f},
		{TEXT("Salamandre"), TEXT("sbire"), 40, 18, 3, FLinearColor(0.9f, 0.3f, 0.1f), CHARGEUR, BRULURE, 0, 0, 3, 4, 12, 150.0f},
		{TEXT("Pyromancien"), TEXT("chaman"), 42, 18, 2, FLinearColor(0.9f, 0.5f, 0.2f), LANCEUR, BRULURE, 0, 0, 0, 4, 10, 175.0f},
		{TEXT("Colosse de braise"), TEXT("guerrier"), 65, 20, 5, FLinearColor(0.35f, 0.15f, 0.1f), MELEE, BRULURE, 0, 2, 0, 4, 5, 235.0f},
	},
	{	// VII. Karn, la cite voilee
		{TEXT("Écho de la vision"), TEXT("Aylis"), 46, 20, 4, FLinearColor(0.3f, 0.25f, 0.45f), MELEE, 0, SAUTEUR, 0, 1, 4, 15, 175.0f},
		{TEXT("Garde de Karn"), TEXT("guerrier"), 60, 20, 6, FLinearColor(0.55f, 0.4f, 0.7f), MELEE, 0, 0, 1, 0, 4, 10, 205.0f},
		{TEXT("Lame du Voile"), TEXT("sbire"), 44, 22, 3, FLinearColor(0.6f, 0.3f, 0.9f), MELEE, 0, SAUTEUR, 0, 2, 4, 25, 165.0f},
		{TEXT("Archer du Voile"), TEXT("traqueur"), 44, 20, 3, FLinearColor(0.5f, 0.35f, 0.8f), LANCEUR, GEL, 0, 0, 0, 5, 12, 175.0f},
		{TEXT("Prophète Haschen"), TEXT("chaman"), 48, 19, 3, FLinearColor(0.8f, 0.6f, 1.0f), LANCEUR, POISON, SOIGNEUR | INVOCATEUR, 0, 0, 4, 10, 180.0f},
	},
};

static const FVespModeleHaschen ELITES[7][2] = {
	{{TEXT("Haschen guerrier d'élite"), TEXT("guerrier"), 44, 11, 3, OR, MELEE, 0, 0, 0, 0, 4, 15, 210.0f},
	 {TEXT("Haschen hurleur"), TEXT("sbire"), 46, 12, 2, OR, CHARGEUR, 0, INVOCATEUR, 0, 1, 4, 15, 205.0f}},
	{{TEXT("Louvetier d'élite"), TEXT("sbire"), 50, 13, 3, OR, CHARGEUR, 0, INVOCATEUR, 0, 1, 4, 15, 210.0f},
	 {TEXT("Brute des tombes"), TEXT("guerrier"), 64, 14, 4, OR, MELEE, 0, VAMPIRE, 0, 0, 4, 15, 228.0f}},
	{{TEXT("Noyé colossal"), TEXT("guerrier"), 78, 16, 4, OR, MELEE, 0, VAMPIRE, 0, 0, 4, 15, 240.0f},
	 {TEXT("Mère des crapauds"), TEXT("sbire"), 68, 15, 2, OR, LANCEUR, POISON, INVOCATEUR, 0, 0, 5, 15, 190.0f}},
	{{TEXT("Capitaine de la forteresse"), TEXT("guerrier"), 84, 17, 5, OR, MELEE, 0, INVOCATEUR, 2, 0, 4, 15, 215.0f},
	 {TEXT("Golem ancien"), TEXT("guerrier"), 96, 17, 4, OR, MELEE, 0, 0, 3, 0, 4, 10, 240.0f}},
	{{TEXT("Alpha blanc"), TEXT("sbire"), 90, 19, 3, OR, CHARGEUR, GEL, INVOCATEUR, 0, 2, 4, 15, 215.0f},
	 {TEXT("Garde pourpre d'Ashka"), TEXT("guerrier"), 100, 19, 5, OR, MELEE, 0, 0, 2, 0, 4, 15, 215.0f}},
	{{TEXT("Heraut des cendres"), TEXT("chaman"), 104, 21, 4, OR, LANCEUR, BRULURE, INVOCATEUR | SOIGNEUR, 0, 0, 5, 15, 210.0f},
	 {TEXT("Colosse ardent"), TEXT("guerrier"), 125, 22, 6, OR, MELEE, BRULURE, 0, 3, 0, 4, 10, 250.0f}},
	{{TEXT("Grand Écho"), TEXT("Aylis"), 120, 24, 5, OR, MELEE, 0, SAUTEUR | VAMPIRE, 0, 1, 4, 20, 200.0f},
	 {TEXT("Champion de Karn"), TEXT("guerrier"), 145, 24, 7, OR, MELEE, 0, 0, 3, 0, 4, 15, 245.0f}},
};

// Les boss : bien plus de pv qu'au tour par tour (le combat dure, et chaque motif se lit et s'evite)
static const FVespModeleHaschen BOSS[7] = {
	{TEXT("Skarn le Brise-Crânes"), TEXT("guerrier"), 240, 12, 3, FLinearColor(0.5f, 0.5f, 0.6f), CHARGEUR, 0, 0, 0, 0, 4, 10, 260.0f},
	{TEXT("La Matriarche"), TEXT("chaman"), 290, 13, 3, FLinearColor(0.3f, 0.65f, 0.35f), LANCEUR, POISON, 0, 0, 0, 5, 10, 250.0f},
	{TEXT("Le Roi Noyé"), TEXT("guerrier"), 380, 16, 4, FLinearColor(0.2f, 0.5f, 0.45f), MELEE, POISON, VAMPIRE, 0, 0, 4, 10, 265.0f},
	{TEXT("Le Gardien de Pierre"), TEXT("guerrier"), 440, 18, 5, FLinearColor(0.55f, 0.55f, 0.55f), MELEE, 0, 0, 4, 0, 4, 5, 290.0f},
	{TEXT("Ashka"), TEXT("traqueur"), 470, 19, 5, FLinearColor(0.6f, 0.15f, 0.35f), LANCEUR, GEL, 0, 0, 1, 6, 15, 240.0f},
	{TEXT("Vorgath le Destructeur"), TEXT("guerrier"), 560, 21, 6, FLinearColor(0.3f, 0.1f, 0.05f), CHARGEUR, BRULURE, 0, 0, 0, 4, 10, 300.0f},
	{TEXT("L'Oracle de Karn"), TEXT("chaman"), 650, 23, 6, FLinearColor(0.75f, 0.55f, 1.0f), LANCEUR, POISON, 0, 0, 0, 6, 12, 260.0f},
};

FString AVespCombat::NomDuBoss(int32 LActe)
{
	return BOSS[FMath::Clamp(LActe, 1, 7) - 1].Nom;
}

// ===================== Les armes (pack StylizedCharacter) =====================

static FString Arme(const TCHAR* Dossier, const TCHAR* Nom)
{
	return FString::Printf(TEXT("/Game/StylizedCharacter/Meshes/Item/Weapons/%s/%s.%s"), Dossier, Nom, Nom);
}

// Chaque Haschen a l'arme de son metier : hache (guerrier), arc (traqueur), baton (chaman), dague (sbire)...
static void Armer(AVespUnite* H, const FVespModeleHaschen& M, int32 BossActe)
{
	switch (BossActe)
	{
		case 1: H->Equiper(Arme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01")), 0.7f, false, FString(), EVespArme::DeuxMains); return;		// Skarn et sa masse
		case 2: H->Equiper(Arme(TEXT("Staff"), TEXT("SK_Staff_Newbie_03")), 0.95f, false, FString(), EVespArme::Baton); return;		// la Matriarche
		case 3: H->Equiper(Arme(TEXT("Sword"), TEXT("SK_Sword_2H_Newbie_02")), 0.65f, false, FString(), EVespArme::DeuxMains); return;	// le Roi Noye
		case 4: H->Equiper(FString(), 0.0f, false, FString(), EVespArme::Poings); return;												// le Gardien : ses poings de pierre
		case 5: H->Equiper(Arme(TEXT("Bow"), TEXT("SK_Bow_Newbie_03")), 0.75f, true, FString(), EVespArme::Arc); return;				// Ashka et son arc
		case 6: H->Equiper(Arme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01")), 0.75f, false, Arme(TEXT("Shield"), TEXT("SK_Shield_Newbie_03")), EVespArme::DeuxMains); return;
		case 7: H->Equiper(Arme(TEXT("Staff"), TEXT("SK_Staff_Newbie_04")), 1.0f, false, FString(), EVespArme::Baton); return;			// l'Oracle
		default: break;
	}
	const FString Dossier = M.Dossier;
	const bool bGrand = M.Taille >= 205.0f;
	if (Dossier == TEXT("guerrier"))
	{
		H->Equiper(bGrand ? Arme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01")) : Arme(TEXT("Axe"), TEXT("SK_Axe_1H_Newbie_02")), bGrand ? 0.65f : 0.45f, false,
		           M.Defense >= 5 ? Arme(TEXT("Shield"), TEXT("SK_Shield_Newbie_01")) : FString(), bGrand ? EVespArme::DeuxMains : EVespArme::Epee);
	}
	else if (Dossier == TEXT("traqueur"))
	{
		H->Equiper(Arme(TEXT("Bow"), bGrand ? TEXT("SK_Bow_Newbie_02") : TEXT("SK_Bow_Newbie_01")), 0.65f, true, FString(), EVespArme::Arc);
	}
	else if (Dossier == TEXT("chaman"))
	{
		H->Equiper(Arme(TEXT("Staff"), bGrand ? TEXT("SK_Staff_Newbie_02") : TEXT("SK_Staff_Newbie_01")), 0.9f, false, FString(),
		           M.Style == LANCEUR ? EVespArme::Baton : EVespArme::Epee);
	}
	else if (Dossier == TEXT("Aylis"))
	{
		H->Equiper(Arme(TEXT("Sword"), TEXT("SK_Sword_1H_Newbie_01")), 0.5f, false, FString(), EVespArme::Epee);		// les echos d'AYLIS
	}
	else
	{
		H->Equiper(Arme(TEXT("Dagger"), bGrand ? TEXT("SK_Dagger_1H_Newbie_03") : TEXT("SK_Dagger_1H_Newbie_01")), 0.32f, false, FString(), EVespArme::Epee);
	}
}

// ===================== La mise en place =====================

AVespCombat::AVespCombat()
{
	PrimaryActorTick.bCanEverTick = false;		// c'est le PlayerController qui le fait avancer (rien ne bouge pendant les menus)
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeCylindre(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LaSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Cylindre = LeCylindre.Object;
	Cube = LeCube.Object;
	Sphere = LaSphere.Object;
}

void AVespCombat::Preparer(AVespMonde* LeMonde, AVespUnite* LAylis)
{
	Monde = LeMonde;
	Aylis = LAylis;
}

void AVespCombat::Vider()
{
	for (FVespHaschen& H : Haschen)
	{
		if (AVespUnite* U = H.U.Get())
		{
			U->Destroy();
		}
	}
	Haschen.Reset();
	Groupes.Reset();
	for (FVespDanger& Z : Dangers)
	{
		EffacerDanger(Z);
	}
	Dangers.Reset();
	for (FVespProjectile& P : Projectiles)
	{
		if (P.Visuel) P.Visuel->DestroyComponent();
		if (P.Lumiere) P.Lumiere->DestroyComponent();
	}
	Projectiles.Reset();
	Appels.Reset();
	EffacerBarriere();
	RayonEnclos = 0.0f;
	TempsBlizzard = 0.0f;
}

AVespUnite* AVespCombat::Creer(int32 IndexModele, int32 Categorie, const FVector& Position, int32 Groupe, int32 Etage)
{
	const int32 A = FMath::Clamp(Acte, 1, 7) - 1;
	const FVespModeleHaschen& M = Categorie == 2 ? BOSS[A] : (Categorie == 1 ? ELITES[A][IndexModele % 2] : HASCHEN[A][IndexModele % 5]);
	FVespStats S;
	S.Nom = M.Nom;
	// Plus AYLIS avance dans l'acte, plus les Haschen sont coriaces
	S.PvMax = M.Pv + (Categorie == 2 ? 0 : Etage * 2);
	S.Pv = S.PvMax;
	S.Attaque = M.Attaque + (Categorie == 2 ? 0 : Etage / 4);
	S.Defense = M.Defense;
	S.Style = M.Style;
	S.Effet = M.Effet;
	S.Capacites = M.Capacites;
	S.Armure = M.Armure;
	S.ArmureMax = M.Armure;
	S.Pas = M.Pas;
	S.Portee = M.Portee;
	S.ChanceCritique = M.Critique;
	S.Boss = Categorie == 2 ? Acte : 0;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AVespUnite* U = GetWorld()->SpawnActor<AVespUnite>(FVector(Position.X, Position.Y, SolZ), FRotator(0, FMath::FRandRange(0.0f, 360.0f), 0), Params);
	if (!U)
	{
		return nullptr;
	}
	U->Taille = M.Taille;
	float V = M.Style == LANCEUR ? 280.0f : (M.Style == CHARGEUR ? 320.0f : 300.0f);
	V += M.Pas * 25.0f;
	if (M.Taille >= 205.0f) V *= 0.88f;
	if (Categorie == 2) V = M.Style == LANCEUR ? 330.0f : 290.0f;
	U->Vitesse = V;
	U->Preparer(Monde, S, DOSSIER + M.Dossier, M.Teinte, false);
	Armer(U, M, Categorie == 2 ? Acte : 0);
	FVespHaschen H;
	H.U = U;
	H.Groupe = Groupe;
	H.bBoss = Categorie == 2;
	H.bElite = Categorie == 1;
	H.Recharge = FMath::FRandRange(0.4f, 1.4f);
	H.Recharge2 = FMath::FRandRange(2.5f, 5.0f);
	H.Contournement = FMath::RandBool() ? 1.0f : -1.0f;
	Haschen.Add(H);
	return U;
}

// Une place au hasard dans une clairiere (loin du centre, loin d'AYLIS si possible)
static FVector PlaceDans(const FVector& Centre, float Rayon, float MinPart, float MaxPart)
{
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float D = Rayon * FMath::FRandRange(MinPart, MaxPart);
	return Centre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * D;
}

void AVespCombat::Peupler(int32 LActe, const TArray<FVespLieu>& Lieux)
{
	Vider();
	Acte = FMath::Clamp(LActe, 1, 7);
	SolZ = Lieux.Num() > 0 ? Lieux[0].Centre.Z : 0.0f;
	for (int32 i = 0; i < Lieux.Num(); i++)
	{
		const FVespLieu& L = Lieux[i];
		if (L.Type != COMBAT && L.Type != ELITE && L.Type != BOSS_SALLE)
		{
			continue;
		}
		FVespGroupe G;
		G.Zone = i;
		G.Type = L.Type;
		G.Centre = L.Centre;
		G.Rayon = L.Rayon;
		G.Etage = L.Etage;
		G.bBoss = L.Type == BOSS_SALLE;
		if (L.Type == COMBAT)
		{
			G.VaguesRestantes = (L.Etage >= 4 ? 1 : 0) + (Acte >= 4 && L.Etage >= 6 ? 1 : 0);
		}
		else if (L.Type == ELITE)
		{
			G.VaguesRestantes = Acte >= 3 ? 1 : 0;
		}
		if (Acte == 4)
		{
			for (int32 k = 0; k < 4; k++)
			{
				G.Pieges.Add(PlaceDans(L.Centre, L.Rayon, 0.3f, 0.75f));
			}
		}
		const int32 g = Groupes.Add(G);
		if (G.bBoss)
		{
			// Le boss attend au fond de sa clairiere, face au sentier
			if (AVespUnite* B = Creer(0, 2, L.Centre + FVector(0.0f, 350.0f, 0.0f), g, L.Etage))
			{
				B->TournerDUnCoup(FVector(0.0f, -1.0f, 0.0f));
			}
			continue;
		}
		TArray<int32> AAppeler;
		if (L.Type == ELITE)
		{
			Creer(FMath::RandRange(0, 1), 1, PlaceDans(L.Centre, L.Rayon, 0.2f, 0.5f), g, L.Etage);
			AAppeler.Add(FMath::RandRange(0, 4));
			if (Acte >= 4 || L.Etage >= 6)
			{
				AAppeler.Add(FMath::RandRange(0, 4));
			}
		}
		else
		{
			int32 Nombre = (Acte == 1 && L.Etage <= 1) ? 2 : 3;
			Nombre += L.Etage >= 6 ? 1 : 0;
			Nombre += (Acte >= 5 && L.Etage >= 3) ? 1 : 0;
			for (int32 k = 0; k < FMath::Min(Nombre, 5); k++)
			{
				AAppeler.Add(FMath::RandRange(0, 4));
			}
		}
		for (int32 Modele : AAppeler)
		{
			Creer(Modele, 0, PlaceDans(L.Centre, L.Rayon, 0.25f, 0.7f), g, L.Etage);
		}
	}
	// Ils dorment sous terre, jusqu'a l'arrivee d'AYLIS (sauf le boss, qui attend debout)
	for (FVespHaschen& H : Haschen)
	{
		if (!H.bBoss && H.U.IsValid())
		{
			H.U->SetActorHiddenInGame(true);
			H.Etat = EVespIntention::Dormir;
		}
		else if (H.bBoss)
		{
			H.Etat = EVespIntention::Dormir;
		}
	}
	// Les patrouilles : sur les longs sentiers, deux ou trois Haschen font les cent pas
	for (int32 i = 0; i < Lieux.Num(); i++)
	{
		for (int32 S : Lieux[i].Suivants)
		{
			const FVector A = Lieux[i].Centre, B = Lieux[S].Centre;
			if (i == 0 || FVector::Dist2D(A, B) < 2200.0f || FMath::FRand() > 0.6f || Lieux[S].Type == BOSS_SALLE)
			{
				continue;
			}
			FVespGroupe G;
			G.Zone = -1;
			G.Centre = FMath::Lerp(A, B, 0.5f);
			G.Rayon = 600.0f;
			G.Etage = Lieux[i].Etage;
			const int32 g = Groupes.Add(G);
			const FVector Sens = (B - A).GetSafeNormal2D();
			const FVector Cote(-Sens.Y, Sens.X, 0.0f);
			const int32 Nombre = FMath::RandRange(2, 3);
			for (int32 k = 0; k < Nombre; k++)
			{
				const FVector P = FMath::Lerp(A, B, 0.38f) + Cote * FMath::FRandRange(-150.0f, 150.0f) + Sens * k * 90.0f;
				if (Creer(FMath::RandRange(0, 4), 0, P, g, Lieux[i].Etage))
				{
					FVespHaschen& H = Haschen.Last();
					H.Etat = EVespIntention::Errer;
					H.Errance = P;
					H.ErranceB = P + Sens * FVector::Dist2D(A, B) * 0.24f;
				}
			}
		}
	}
}

void AVespCombat::Embuscade(const FVector& Centre, bool bElite)
{
	FVespGroupe G;
	G.Zone = -1;
	G.Centre = Centre;
	G.Rayon = 700.0f;
	G.bEngage = true;
	const int32 g = Groupes.Add(G);
	const int32 Nombre = bElite ? 2 : 3;
	for (int32 i = 0; i < Nombre + (bElite ? 1 : 0); i++)
	{
		const float A = i * 2.0f * PI / (Nombre + 1) + FMath::FRandRange(-0.3f, 0.3f);
		FVespAppel Appel;
		Appel.Position = Centre + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * FMath::FRandRange(380.0f, 520.0f);
		if (Monde && !Monde->EstPraticable(Appel.Position))
		{
			Appel.Position = Monde->Contraindre(Centre, Appel.Position);
		}
		Appel.Modele = FMath::RandRange(0, 4);
		Appel.Categorie = (bElite && i == 0) ? 1 : 0;
		Appel.Groupe = g;
		Appels.Add(Appel);
	}
	CreerAppels();
	UVespSons::Jouer(this, EVespSon::Os, Centre, 1.0f, 0.7f);
}

void AVespCombat::LeverLaBarriere()
{
	for (int32 g = 0; g < Groupes.Num(); g++)
	{
		if (!Groupes[g].bFerme)
		{
			continue;
		}
		Groupes[g].bFerme = false;
		Groupes[g].bLibere = true;
		for (FVespHaschen& H : Haschen)
		{
			if (H.Groupe == g && H.U.IsValid())
			{
				H.U->Destroy();
			}
		}
	}
	for (FVespDanger& Z : Dangers)
	{
		EffacerDanger(Z);
	}
	Dangers.Reset();
	RayonEnclos = 0.0f;
	EffacerBarriere();
}

// ===================== Les aides =====================

FVespHaschen* AVespCombat::Trouver(const AVespUnite* U)
{
	for (FVespHaschen& H : Haschen)
	{
		if (H.U.Get() == U)
		{
			return &H;
		}
	}
	return nullptr;
}

int32 AVespCombat::DefenseAylis() const
{
	return Aylis ? Aylis->Stats.Defense : 0;
}

int32 AVespCombat::DegatsDe(const AVespUnite* U, float Multiplicateur) const
{
	return FMath::Max(1, FMath::RoundToInt(U->Stats.Attaque * Multiplicateur + FMath::FRandRange(-1.0f, 1.0f)));
}

int32 AVespCombat::AttaquantsAuContact() const
{
	int32 N = 0;
	for (const FVespHaschen& H : Haschen)
	{
		if ((H.Etat == EVespIntention::Preparer || H.Etat == EVespIntention::Frapper) && H.Geste == 0 && !H.bBoss)
		{
			N++;
		}
	}
	return N;
}

bool AVespCombat::EnCombat() const
{
	if (RayonEnclos > 0.0f)
	{
		return true;
	}
	for (const FVespHaschen& H : Haschen)
	{
		const AVespUnite* U = H.U.Get();
		if (U && U->EstDebout() && Groupes.IsValidIndex(H.Groupe) && Groupes[H.Groupe].bEngage && Aylis
		    && FVector::Dist2D(U->GetActorLocation(), Aylis->GetActorLocation()) < 2200.0f)
		{
			return true;
		}
	}
	return false;
}

const FVespGroupe* AVespCombat::GroupeFerme() const
{
	for (const FVespGroupe& G : Groupes)
	{
		if (G.bFerme)
		{
			return &G;
		}
	}
	return nullptr;
}

AVespUnite* AVespCombat::BossActif() const
{
	for (const FVespHaschen& H : Haschen)
	{
		AVespUnite* U = H.U.Get();
		if (H.bBoss && U && U->EstDebout() && Groupes.IsValidIndex(H.Groupe) && Groupes[H.Groupe].bEngage)
		{
			return U;
		}
	}
	return nullptr;
}

int32 AVespCombat::HaschenEngages() const
{
	int32 N = 0;
	for (const FVespHaschen& H : Haschen)
	{
		const AVespUnite* U = H.U.Get();
		N += (U && U->EstDebout() && Groupes.IsValidIndex(H.Groupe) && Groupes[H.Groupe].bEngage) ? 1 : 0;
	}
	return N;
}

AVespUnite* AVespCombat::CibleProche(const FVector& Depuis, const FVector& Direction, float Portee, float DemiAngle) const
{
	AVespUnite* Meilleure = nullptr;
	float MeilleurScore = MAX_flt;
	const float CosMax = FMath::Cos(FMath::DegreesToRadians(DemiAngle));
	for (const FVespHaschen& H : Haschen)
	{
		AVespUnite* U = H.U.Get();
		if (!U || !U->EstDebout() || U->IsHidden())
		{
			continue;
		}
		FVector V = U->GetActorLocation() - Depuis;
		V.Z = 0.0f;
		const float D = V.Size();
		if (D > Portee || (D > 1.0f && FVector::DotProduct(V / D, Direction) < CosMax))
		{
			continue;
		}
		const float Score = D - FVector::DotProduct(V.GetSafeNormal(), Direction) * 150.0f;		// devant, et proche
		if (Score < MeilleurScore)
		{
			MeilleurScore = Score;
			Meilleure = U;
		}
	}
	return Meilleure;
}

void AVespCombat::Changer(FVespHaschen& H, EVespIntention Etat)
{
	H.Etat = Etat;
	H.Temps = 0.0f;
}

void AVespCombat::Bouger(FVespHaschen& H, const FVector& Vers, float VitesseVoulue, float Secondes, bool bTourner)
{
	AVespUnite* U = H.U.Get();
	FVector Dir = Vers - U->GetActorLocation();
	Dir.Z = 0.0f;
	if (Dir.SizeSquared() < 25.0f)
	{
		return;
	}
	const float Blizzard = TempsBlizzard > 0.0f ? 0.7f : 1.0f;
	U->Deplacer(Dir.GetSafeNormal() * VitesseVoulue * Blizzard, Secondes, bTourner);
}

// ===================== Les coups =====================

void AVespCombat::AppliquerEffet(AVespUnite* Cible, int32 Effet)
{
	if (!Cible || !Cible->EstDebout())
	{
		return;
	}
	const FVector Pied = Cible->GetActorLocation();
	switch (Effet)
	{
		case VespEffetCoup::Poison:
			Cible->Empoisonner(4.0f);
			Cible->AfficherMessage(TEXT("poison"), FColor(170, 240, 90), 32.0f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, Pied, FVector::UpVector, FLinearColor(0.6f, 1.0f, 0.3f));
			break;
		case VespEffetCoup::Gel:
			Cible->Geler(2.5f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Pied + FVector(0, 0, 60), FVector::UpVector, FLinearColor(0.5f, 0.8f, 1.0f));
			UVespSons::Jouer(this, EVespSon::Glace, Pied, 0.7f);
			break;
		case VespEffetCoup::Brulure:
			Cible->Bruler(3.0f);
			Cible->AfficherMessage(TEXT("brule"), FColor(255, 150, 60), 32.0f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poison, Pied, FVector::UpVector, FLinearColor(1.0f, 0.5f, 0.15f));
			UVespSons::Jouer(this, EVespSon::Feu, Pied, 0.7f);
			break;
		default: break;
	}
}

void AVespCombat::Blesser(AVespUnite* Cible, int32 Degats, bool bCritique, const FVector& Direction, float Poussee, bool bLourd, int32 Effet, const FLinearColor& Couleur)
{
	if (!Cible || !Cible->EstDebout())
	{
		return;
	}
	int32 D = Degats;
	bool bArmure = false;
	// L'armure : un coup ordinaire glisse dessus ; un coup lourd fissure une plaque
	if (Cible->Stats.Armure > 0)
	{
		bArmure = true;
		if (bLourd)
		{
			Cible->Stats.Armure--;
			if (Cible->Stats.Armure == 0)
			{
				Cible->TempsBrise = 4.0f;
				Cible->Etourdi = Cible->Stats.Boss > 0 ? 3.0f : 2.0f;
				Cible->AfficherMessage(TEXT("ARMURE BRISÉE"), FColor(255, 210, 120), 46.0f);
				AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, Cible->GetActorLocation() + FVector(0, 0, Cible->Taille * 0.6f), FVector::UpVector, FLinearColor(0.8f, 0.8f, 0.9f), 0.05f);
				if (SurMessage) SurMessage(Cible->Stats.Nom + TEXT(" : armure brisée ! Il est sonné."));
			}
		}
		else
		{
			D = FMath::Max(1, D / 4);
		}
	}
	if (Cible->TempsBrise > 0.0f)
	{
		D = D * 3 / 2;
	}
	FVespHaschen* H = Trouver(Cible);
	const bool bBoss = Cible->Stats.Boss > 0;
	const float Avant = Cible->Equilibre;
	Cible->Encaisser(D, bCritique, Direction, bBoss ? Poussee * 0.15f : (Cible->SeuilEquilibre > 0.0f ? Poussee * 0.5f : Poussee), bBoss && !bLourd);
	// Il flechit : s'il preparait une attaque, elle n'a pas lieu
	const bool bAFlechi = Cible->EstDebout() && !bBoss && (Cible->SeuilEquilibre <= 0.0f || Cible->Equilibre < Avant || bLourd);
	if (H && bAFlechi && (H->Etat == EVespIntention::Preparer || H->Etat == EVespIntention::Incanter))
	{
		Changer(*H, EVespIntention::Recuperer);
		H->Recharge = FMath::Max(H->Recharge, 0.7f);
	}
	// Un boss qui boit une decoction : un coup lourd la lui fait lacher
	if (H && bBoss && H->Motif == 9 && bLourd)
	{
		H->PvDebut = 99999;
	}
	AVespEffet::Jouer(GetWorld(), bCritique ? EVespEffet::Critique : EVespEffet::Impact, Cible->GetActorLocation() + FVector(0, 0, Cible->Taille * 0.55f),
	                  Direction, bArmure && !bLourd ? FLinearColor(0.85f, 0.85f, 0.95f) : Couleur);
	UVespSons::Jouer(this, bArmure && !bLourd ? EVespSon::ImpactArmure : (bCritique ? EVespSon::Critique : EVespSon::Impact), Cible->GetActorLocation(),
	                 bLourd ? 1.0f : 0.8f, bLourd ? 0.85f : 1.0f);
	if (Effet != 0)
	{
		AppliquerEffet(Cible, Effet);
	}
	if (SurImpact)
	{
		SurImpact(bCritique ? 1.0f : (bLourd ? 0.65f : 0.3f), bCritique || !Cible->EstDebout());
	}
}

int32 AVespCombat::FrappeDAylis(const FVespCoup& Coup)
{
	if (!Aylis)
	{
		return 0;
	}
	int32 Touches = 0;
	const float CosMax = FMath::Cos(FMath::DegreesToRadians(FMath::Min(Coup.DemiAngle, 180.0f)));
	for (int32 i = 0; i < Haschen.Num(); i++)
	{
		AVespUnite* U = Haschen[i].U.Get();
		if (!U || !U->EstDebout() || U->IsHidden() || Haschen[i].Etat == EVespIntention::Surgir)
		{
			continue;
		}
		FVector V = U->GetActorLocation() - Coup.Origine;
		V.Z = 0.0f;
		const float D = V.Size() - U->Rayon();
		if (D > Coup.Portee)
		{
			continue;
		}
		const FVector N = V.GetSafeNormal();
		if (Coup.DemiAngle < 180.0f && D > 40.0f && FVector::DotProduct(N, Coup.Direction) < CosMax)
		{
			continue;
		}
		int32 Degats = FMath::RoundToInt(Aylis->Stats.Attaque * Coup.Puissance - U->Stats.Defense * 0.5f + FMath::FRandRange(-1.0f, 1.0f));
		const bool bCritique = Coup.bPeutCritiquer && FMath::RandRange(1, 100) <= Aylis->Stats.ChanceCritique;
		if (bCritique)
		{
			Degats = FMath::RoundToInt(Degats * MultCritique);
		}
		if (bExecution && U->Stats.Pv * 3 < U->Stats.PvMax)
		{
			Degats = FMath::RoundToInt(Degats * 1.6f);
		}
		const int32 Effet = (Coup.Effet != 0 && FMath::FRand() < Coup.ChanceEffet) ? Coup.Effet : 0;
		Blesser(U, FMath::Max(1, Degats), bCritique, N.IsNearlyZero() ? Coup.Direction : N, Coup.Poussee, Coup.bLourd, Effet, Coup.Couleur);
		Touches++;
	}
	return Touches;
}

void AVespCombat::ToucherAylis(int32 Degats, int32 Effet, AVespUnite* Source, const FVector& Direction, float Poussee, bool bParable, EVespSon Son)
{
	if (!Aylis || !Aylis->EstDebout())
	{
		return;
	}
	const FVector Ici = Aylis->GetActorLocation();
	// L'esquive : AYLIS passe a travers
	if (Aylis->Invulnerable > 0.0f)
	{
		Aylis->AfficherMessage(TEXT("esquive"), FColor(180, 200, 255), 30.0f);
		if (SurEsquive) SurEsquive();
		return;
	}
	FVector VersSource = Source ? (Source->GetActorLocation() - Ici).GetSafeNormal2D() : -Direction.GetSafeNormal2D();
	const bool bFace = FVector::DotProduct(Aylis->Avant(), VersSource) > 0.2f;
	if (bGarde && bParable && bFace)
	{
		if (TempsGarde < FenetreParade)
		{
			// La parade parfaite : rien ne passe, et l'attaquant est desequilibre
			Aylis->AfficherMessage(TEXT("PARADE !"), FColor(255, 220, 130), 44.0f);
			Aylis->Jouer(EVespGeste::Riposte, 1.3f, true);
			UVespSons::Jouer(this, EVespSon::Parade, Ici, 1.0f, 1.2f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Ici + FVector(0, 0, 110) + VersSource * 60.0f, VersSource, FLinearColor(1.0f, 0.85f, 0.5f));
			if (Source && Source->EstDebout() && FVector::Dist2D(Source->GetActorLocation(), Ici) < 450.0f)
			{
				Source->Etourdi = FMath::Max(Source->Etourdi, Source->Stats.Boss > 0 ? 0.8f : 1.6f);
				Source->Pousser(VersSource * 450.0f);
				Source->Jouer(EVespGeste::Touche, 1.0f);
				Source->AfficherMessage(TEXT("sonne"), FColor(255, 220, 130), 32.0f);
			}
			if (SurParade) SurParade();
			if (SurImpact) SurImpact(0.5f, true);
			return;
		}
		Degats = FMath::Max(1, FMath::RoundToInt(Degats * (0.35f - BonusParade * 0.2f)));
		Aylis->Jouer(EVespGeste::GardeTouchee, 1.2f, true);
		UVespSons::Jouer(this, EVespSon::Parade, Ici, 0.7f, 0.85f);
		const int32 Final = FMath::Max(1, Degats - Aylis->Stats.Defense / 2);
		Aylis->Encaisser(Final, false, Direction, Poussee * 0.4f, true);
		if (SurBlessure) SurBlessure(Final, false, Source);
		return;
	}
	const bool bCritique = Source && FMath::RandRange(1, 100) <= Source->Stats.ChanceCritique / 2;
	int32 Final = FMath::Max(1, Degats - Aylis->Stats.Defense);
	if (bCritique)
	{
		Final = Final * 3 / 2;
	}
	Final = FMath::Max(1, FMath::RoundToInt(Final * Aylis->ReductionDegats));
	Aylis->Encaisser(Final, bCritique, Direction, Poussee, false);
	UVespSons::Jouer(this, Son, Ici, 0.8f);
	UVespSons::Jouer(this, EVespSon::Blessure, Ici, 0.7f);
	if (Effet != 0 && FMath::RandBool())
	{
		AppliquerEffet(Aylis, Effet);
	}
	if (SurBlessure) SurBlessure(Final, bCritique, Source);
	if (Source && Source->EstDebout())
	{
		if (Source->Stats.Capacites & VespCapacite::Vampire)
		{
			Source->Soigner(FMath::Max(1, Final / 2));
		}
		if (bEpines && FVector::Dist2D(Source->GetActorLocation(), Ici) < 320.0f)
		{
			Blesser(Source, 3 + Acte, false, VersSource, 100.0f, false, 0, FLinearColor(0.5f, 1.0f, 0.4f));
		}
	}
}

// ===================== Les attaques annoncees =====================

UStaticMeshComponent* AVespCombat::Disque(const FLinearColor& Couleur, UMaterialInstanceDynamic*& Materiau, bool bCube)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetupAttachment(Racine);
	C->SetStaticMesh(bCube ? Cube.Get() : Cylindre.Get());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->RegisterComponent();
	Materiau = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Couleur);
	C->SetMaterial(0, Materiau);
	return C;
}

FVespDanger& AVespCombat::Annoncer(const FVector& Centre, float Rayon, float Delai, int32 Degats, AVespUnite* Source)
{
	FVespDanger Z;
	Z.Centre = FVector(Centre.X, Centre.Y, SolZ);
	Z.Rayon = Rayon;
	Z.Delai = Z.DelaiTotal = FMath::Max(0.05f, Delai);
	Z.Degats = Degats;
	Z.Source = Source;
	Z.Fond = Disque(Z.Couleur * 0.25f, Z.MatFond, false);
	Z.Remplissage = Disque(Z.Couleur * 0.6f, Z.MatRemplissage, false);
	Z.Fond->SetWorldTransform(FTransform(FRotator::ZeroRotator, Z.Centre + FVector(0, 0, 2.0f), FVector(Rayon / 50.0f, Rayon / 50.0f, 0.01f)));
	Z.Remplissage->SetWorldTransform(FTransform(FRotator::ZeroRotator, Z.Centre + FVector(0, 0, 3.0f), FVector(0.01f, 0.01f, 0.01f)));
	return Dangers[Dangers.Add(Z)];
}

FVespDanger& AVespCombat::AnnoncerLigne(const FVector& Depart, const FVector& Direction, float Longueur, float Largeur, float Delai, AVespUnite* Source)
{
	FVespDanger Z;
	Z.Centre = FVector(Depart.X, Depart.Y, SolZ);
	Z.Direction = Direction.GetSafeNormal2D();
	Z.Longueur = Longueur;
	Z.Rayon = Largeur * 0.5f;
	Z.Delai = Z.DelaiTotal = FMath::Max(0.05f, Delai);
	Z.Source = Source;
	Z.Fond = Disque(Z.Couleur * 0.25f, Z.MatFond, true);
	Z.Remplissage = Disque(Z.Couleur * 0.6f, Z.MatRemplissage, true);
	const FRotator R(0, Z.Direction.Rotation().Yaw, 0);
	Z.Fond->SetWorldTransform(FTransform(R, Z.Centre + Z.Direction * Longueur * 0.5f + FVector(0, 0, 2.0f), FVector(Longueur / 100.0f, Largeur / 100.0f, 0.01f)));
	Z.Remplissage->SetWorldTransform(FTransform(R, Z.Centre + FVector(0, 0, 3.0f), FVector(0.01f, Largeur / 100.0f, 0.01f)));
	return Dangers[Dangers.Add(Z)];
}

void AVespCombat::EffacerDanger(FVespDanger& Z)
{
	if (Z.Fond) Z.Fond->DestroyComponent();
	if (Z.Remplissage) Z.Remplissage->DestroyComponent();
	Z.Fond = Z.Remplissage = nullptr;
}

bool AVespCombat::DansDanger(const FVespDanger& Z, const FVector& P, float Marge) const
{
	if (Z.Longueur <= 0.0f)
	{
		return FVector::Dist2D(P, Z.Centre) <= Z.Rayon + Marge;
	}
	const FVector V = P - Z.Centre;
	const float T = FVector::DotProduct(FVector(V.X, V.Y, 0.0f), Z.Direction);
	const float Cote = FMath::Abs(V.X * -Z.Direction.Y + V.Y * Z.Direction.X);
	return T >= -Marge && T <= Z.Longueur + Marge && Cote <= Z.Rayon + Marge;
}

void AVespCombat::AvancerDangers(float Secondes)
{
	for (int32 i = Dangers.Num() - 1; i >= 0; i--)
	{
		FVespDanger& Z = Dangers[i];
		AVespUnite* Source = Z.Source.Get();
		// L'attaquant est tombe ou sonne avant de frapper : l'attaque n'a pas lieu
		if (!Z.bFrappee && Z.bAnnuleeSiSourceTombe && Z.Source.IsValid()
		    && (!Source->EstDebout() || Source->Etourdi > 0.0f || Source->Fige > 0.0f))
		{
			EffacerDanger(Z);
			Dangers.RemoveAt(i);
			continue;
		}
		if (!Z.bFrappee)
		{
			Z.Delai -= Secondes;
			const float P = FMath::Clamp(1.0f - Z.Delai / Z.DelaiTotal, 0.0f, 1.0f);
			// La tache se remplit ; elle clignote a la fin
			if (Z.Longueur <= 0.0f)
			{
				Z.Remplissage->SetWorldScale3D(FVector(Z.Rayon * P / 50.0f, Z.Rayon * P / 50.0f, 0.01f));
			}
			else
			{
				Z.Remplissage->SetWorldScale3D(FVector(FMath::Max(0.01f, Z.Longueur * P / 100.0f), Z.Rayon * 2.0f / 100.0f, 0.01f));
				Z.Remplissage->SetWorldLocation(Z.Centre + Z.Direction * Z.Longueur * P * 0.5f + FVector(0, 0, 3.0f));
			}
			const float Clignote = P > 0.75f ? 0.6f + 0.4f * FMath::Sin(Horloge * 40.0f) : 0.6f;
			Z.MatRemplissage->SetVectorParameterValue(TEXT("Color"), Z.Couleur * Clignote);
			if (Z.Delai > 0.0f)
			{
				continue;
			}
			// L'impact
			Z.bFrappee = true;
			if (Z.bDegats)
			{
				if (Z.bContreAylis && Aylis && DansDanger(Z, Aylis->GetActorLocation(), Aylis->Rayon()) && !(bPiedSur && !Z.Source.IsValid()))
				{
					FVector Dir = (Aylis->GetActorLocation() - Z.Centre).GetSafeNormal2D();
					if (Dir.IsNearlyZero() || Z.Longueur > 0.0f) Dir = Z.Longueur > 0.0f ? Z.Direction : FVector(1, 0, 0);
					ToucherAylis(Z.Degats, Z.Effet, Source, Dir, Z.Poussee, Z.bParable, Z.Son);
				}
				if (Z.bContreHaschen)
				{
					for (FVespHaschen& H : Haschen)
					{
						AVespUnite* U = H.U.Get();
						if (U && U != Source && U->EstDebout() && !U->IsHidden() && DansDanger(Z, U->GetActorLocation(), U->Rayon()))
						{
							Blesser(U, Z.Degats, false, (U->GetActorLocation() - Z.Centre).GetSafeNormal2D(), Z.Poussee, false, Z.Effet, Z.Couleur);
						}
					}
				}
				// Ce qu'on voit et entend : une onde, un eclat ; et une gerbe de flammes (RPG-FlameAttackVFX) quand ca brule
				if (Z.Effet == VespEffetCoup::Brulure && Z.Longueur <= 0.0f)
				{
					static TWeakObjectPtr<UNiagaraSystem> Flammes;
					if (!Flammes.IsValid())
					{
						Flammes = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/RPG-FlameAttackVFX/VFX/Niagara/N_FlameAttack.N_FlameAttack"), nullptr, LOAD_NoWarn | LOAD_Quiet);
					}
					if (Flammes.IsValid())
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Flammes.Get(), Z.Centre + FVector(0, 0, 5), FRotator::ZeroRotator, FVector(Z.Rayon / 200.0f));
					}
				}
				if (Z.Longueur <= 0.0f && (!Source || !Z.bParable))
				{
					AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Z.Centre + FVector(0, 0, 10), FVector::UpVector, Z.Couleur);
					if (Z.Rayon >= 180.0f)
					{
						AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, Z.Centre, FVector::UpVector, FLinearColor(0.5f, 0.35f, 0.3f));
						UVespSons::Jouer(this, Z.Son == EVespSon::Explosion ? EVespSon::Explosion : EVespSon::Impact, Z.Centre, 0.7f, 0.8f);
					}
				}
			}
			if (Z.Persistance <= 0.0f)
			{
				// Un eclair de la tache, puis elle disparait
				EffacerDanger(Z);
				Dangers.RemoveAt(i);
				continue;
			}
			Z.MatFond->SetVectorParameterValue(TEXT("Color"), Z.Couleur * 0.45f);
			Z.Remplissage->SetVisibility(false);
			Z.Tic = 1.0f;
			continue;
		}
		// La zone reste (poison, lave, eaux toxiques) : elle mord chaque seconde
		Z.Persistance -= Secondes;
		Z.Tic -= Secondes;
		Z.MatFond->SetVectorParameterValue(TEXT("Color"), Z.Couleur * (0.35f + 0.1f * FMath::Sin(Horloge * 5.0f)) * FMath::Clamp(Z.Persistance, 0.0f, 1.0f));
		if (Z.Tic <= 0.0f)
		{
			Z.Tic = 1.0f;
			if (Z.bContreAylis && Aylis && Aylis->EstDebout() && DansDanger(Z, Aylis->GetActorLocation(), 0.0f) && Aylis->Invulnerable <= 0.0f && !bPiedSur)
			{
				const int32 D = FMath::Max(1, Z.Degats / 2);
				Aylis->Encaisser(D, false, FVector::ZeroVector, 0.0f, true);
				AppliquerEffet(Aylis, Z.Effet);
				if (SurBlessure) SurBlessure(D, false, nullptr);
			}
			if (Z.bContreHaschen)
			{
				for (FVespHaschen& H : Haschen)
				{
					AVespUnite* U = H.U.Get();
					if (U && U->EstDebout() && !U->IsHidden() && DansDanger(Z, U->GetActorLocation(), 0.0f))
					{
						U->Encaisser(FMath::Max(1, Z.Degats / 2), false, FVector::ZeroVector, 0.0f, true);
					}
				}
			}
		}
		if (Z.Persistance <= 0.0f)
		{
			EffacerDanger(Z);
			Dangers.RemoveAt(i);
		}
	}
}

// ===================== Les projectiles =====================

void AVespCombat::Tirer(AVespUnite* Source, const FVector& Depart, const FVector& Direction, float VitesseTir, int32 Degats, int32 Effet, bool bAttire, const FLinearColor& Couleur)
{
	FVespProjectile P;
	P.Position = Depart;
	P.Vitesse = Direction.GetSafeNormal2D() * VitesseTir;
	P.Vie = 2.2f;
	P.Degats = Degats;
	P.Effet = Effet;
	P.bAttire = bAttire;
	P.Couleur = Couleur;
	P.Source = Source;
	UMaterialInstanceDynamic* M = nullptr;
	P.Visuel = NewObject<UStaticMeshComponent>(this);
	P.Visuel->SetupAttachment(Racine);
	P.Visuel->SetStaticMesh(Sphere);
	P.Visuel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P.Visuel->SetCastShadow(false);
	P.Visuel->RegisterComponent();
	M = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	M->SetVectorParameterValue(TEXT("Color"), Couleur * 5.0f);
	P.Visuel->SetMaterial(0, M);
	// Une fleche : un trait allonge ; un sort : une boule
	const bool bFleche = Source && Source->GetTypeArme() == EVespArme::Arc;
	P.Visuel->SetWorldTransform(FTransform(P.Vitesse.Rotation(), Depart, bFleche ? FVector(0.9f, 0.07f, 0.07f) : FVector(0.28f)));
	P.Lumiere = NewObject<UPointLightComponent>(this);
	P.Lumiere->SetupAttachment(Racine);
	P.Lumiere->RegisterComponent();
	P.Lumiere->SetLightColor(Couleur);
	P.Lumiere->SetIntensity(1800.0f);
	P.Lumiere->SetAttenuationRadius(300.0f);
	P.Lumiere->SetCastShadows(false);
	P.Lumiere->SetWorldLocation(Depart);
	Projectiles.Add(P);
}

void AVespCombat::TirDAylis(const FVespCoup& Coup, float VitesseTir, float RayonTir, bool bTraversant, float Taille)
{
	Tirer(nullptr, Coup.Origine, Coup.Direction, VitesseTir, 0, Coup.Effet, false, Coup.Couleur);
	FVespProjectile& P = Projectiles.Last();
	P.bContreAylis = false;
	P.Coup = Coup;
	P.Rayon = RayonTir;
	P.bTraversant = bTraversant;
	P.Vie = Coup.Portee / FMath::Max(100.0f, VitesseTir);
	// Une lame de lumiere : large et plate, couchee dans le sens du vol
	P.Visuel->SetWorldScale3D(bTraversant ? FVector(0.35f, 1.6f, 0.08f) * Taille : FVector(0.3f) * Taille);
	P.Lumiere->SetIntensity(3500.0f * Taille);
}

void AVespCombat::AvancerProjectiles(float Secondes)
{
	for (int32 i = Projectiles.Num() - 1; i >= 0; i--)
	{
		FVespProjectile& P = Projectiles[i];
		P.Position += P.Vitesse * Secondes;
		P.Vie -= Secondes;
		P.Visuel->SetWorldLocation(P.Position);
		P.Lumiere->SetWorldLocation(P.Position);
		bool bFini = P.Vie <= 0.0f || (Monde && Monde->DistanceAuPraticable(P.Position) > 350.0f);
		if (!bFini && P.bContreAylis && Aylis && Aylis->EstDebout())
		{
			const FVector Coeur = Aylis->GetActorLocation() + FVector(0, 0, 100);
			if (FVector::Dist2D(P.Position, Coeur) < P.Rayon + Aylis->Rayon())
			{
				const bool bParfaite = bGarde && TempsGarde < FenetreParade &&FVector::DotProduct(Aylis->Avant(), -P.Vitesse.GetSafeNormal2D()) > 0.2f;
				if (bParfaite)
				{
					// Une parade parfaite renvoie le projectile vers son tireur
					P.bContreAylis = false;
					P.bRenvoye = true;
					P.Vitesse = -P.Vitesse * 1.3f;
					P.Vie = 1.5f;
					P.Coup.Puissance = 1.6f;
					P.Coup.Couleur = FLinearColor(1.0f, 0.85f, 0.5f);
					P.Coup.Effet = P.Effet;
					P.Coup.ChanceEffet = 1.0f;
					ToucherAylis(P.Degats, 0, P.Source.Get(), P.Vitesse.GetSafeNormal2D(), 0.0f, true, EVespSon::Impact);		// (le message de parade)
					continue;
				}
				ToucherAylis(P.Degats, P.Effet, P.Source.Get(), P.Vitesse.GetSafeNormal2D(), 200.0f, true, EVespSon::Impact);
				// Le tir qui attire : AYLIS est tiree vers le tireur
				if (P.bAttire && P.Source.IsValid() && Aylis->Invulnerable <= 0.0f && !bGarde)
				{
					const FVector Vers = (P.Source->GetActorLocation() - Aylis->GetActorLocation()).GetSafeNormal2D();
					Aylis->Pousser(Vers * 900.0f);
					Aylis->AfficherMessage(TEXT("attiree"), FColor(200, 170, 255), 30.0f);
				}
				bFini = true;
			}
		}
		else if (!bFini && !P.bContreAylis)
		{
			for (FVespHaschen& H : Haschen)
			{
				AVespUnite* U = H.U.Get();
				if (!U || !U->EstDebout() || U->IsHidden() || P.DejaTouches.Contains(U))
				{
					continue;
				}
				if (FVector::Dist2D(P.Position, U->GetActorLocation()) < P.Rayon + U->Rayon())
				{
					const int32 Attaque = Aylis ? Aylis->Stats.Attaque : 10;
					int32 D = FMath::RoundToInt(Attaque * P.Coup.Puissance - U->Stats.Defense * 0.5f);
					const bool bCritique = P.Coup.bPeutCritiquer && Aylis && FMath::RandRange(1, 100) <= Aylis->Stats.ChanceCritique;
					if (bCritique) D = FMath::RoundToInt(D * MultCritique);
					if (bExecution && U->Stats.Pv * 3 < U->Stats.PvMax) D = FMath::RoundToInt(D * 1.6f);
					const int32 Effet = (P.Coup.Effet != 0 && FMath::FRand() < P.Coup.ChanceEffet) ? P.Coup.Effet : 0;
					Blesser(U, FMath::Max(1, D), bCritique, P.Vitesse.GetSafeNormal2D(), P.Coup.Poussee, P.Coup.bLourd, Effet, P.Coup.Couleur);
					P.DejaTouches.Add(U);
					if (!P.bTraversant)
					{
						bFini = true;
						break;
					}
				}
			}
		}
		if (bFini)
		{
			AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, P.Position, -P.Vitesse.GetSafeNormal(), P.Couleur);
			P.Visuel->DestroyComponent();
			P.Lumiere->DestroyComponent();
			Projectiles.RemoveAt(i);
		}
	}
}

// ===================== La barriere =====================

void AVespCombat::ConstruireBarriere(const FVector& Centre, float Rayon)
{
	EffacerBarriere();
	if (!MatBarriere)
	{
		MatBarriere = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	}
	MatBarriere->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.35f, 1.0f) * 1.6f);
	const int32 Nombre = FMath::Clamp(FMath::RoundToInt(Rayon * 2.0f * PI / 160.0f), 24, 64);
	for (int32 i = 0; i < Nombre; i++)
	{
		const float A = i * 2.0f * PI / Nombre;
		const FVector P(Centre.X + FMath::Cos(A) * Rayon, Centre.Y + FMath::Sin(A) * Rayon, SolZ);
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetupAttachment(Racine);
		C->SetStaticMesh(Cylindre);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->RegisterComponent();
		C->SetMaterial(0, MatBarriere);
		C->SetWorldTransform(FTransform(FRotator::ZeroRotator, P, FVector(0.06f, 0.06f, 0.01f)));
		Barriere.Add(C);
	}
	for (int32 i = 0; i < 6; i++)
	{
		const float A = i * 2.0f * PI / 6.0f;
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetupAttachment(Racine);
		L->RegisterComponent();
		L->SetWorldLocation(FVector(Centre.X + FMath::Cos(A) * Rayon, Centre.Y + FMath::Sin(A) * Rayon, SolZ + 150.0f));
		L->SetLightColor(FLinearColor(0.6f, 0.4f, 1.0f));
		L->SetIntensity(3500.0f);
		L->SetAttenuationRadius(900.0f);
		L->SetCastShadows(false);
		LumieresBarriere.Add(L);
	}
	// (plus de grand cercle runique au sol : il ressemblait a une zone de danger et cachait les vraies, les rouges)
	TempsBarriere = 0.0f;
}

void AVespCombat::EffacerBarriere()
{
	for (int32 i = 0; i < Barriere.Num(); i++)
	{
		if (Barriere[i])
		{
			if (i % 4 == 0)
			{
				AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Barriere[i]->GetComponentLocation() + FVector(0, 0, 100), FVector::UpVector, FLinearColor(0.6f, 0.45f, 1.0f));
			}
			Barriere[i]->DestroyComponent();
		}
	}
	Barriere.Reset();
	for (UPointLightComponent* L : LumieresBarriere)
	{
		if (L) L->DestroyComponent();
	}
	LumieresBarriere.Reset();
}

// ===================== Les groupes =====================

void AVespCombat::Engager(int32 g)
{
	FVespGroupe& G = Groupes[g];
	if (G.bEngage)
	{
		return;
	}
	G.bEngage = true;
	for (FVespHaschen& H : Haschen)
	{
		AVespUnite* U = H.U.Get();
		if (H.Groupe != g || !U || !U->EstDebout())
		{
			continue;
		}
		if (H.Etat == EVespIntention::Dormir && !H.bBoss)
		{
			// Il sort de terre
			U->SetActorHiddenInGame(false);
			U->Jouer(EVespGeste::Resurrection, FMath::FRandRange(1.1f, 1.4f));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, U->GetActorLocation(), FVector::UpVector, FLinearColor(0.5f, 0.3f, 0.35f));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, U->GetActorLocation(), FVector::UpVector, FLinearColor(0.8f, 0.35f, 0.3f));
			Changer(H, EVespIntention::Surgir);
			H.Temps = -FMath::FRandRange(0.0f, 0.5f);
		}
		else
		{
			Changer(H, EVespIntention::Poursuivre);
		}
		if (Aylis)
		{
			U->TournerDUnCoup(Aylis->GetActorLocation() - U->GetActorLocation());
		}
	}
	UVespSons::Jouer(this, G.bBoss ? EVespSon::Rugissement : EVespSon::Os, G.Centre, 0.8f, G.bBoss ? 1.0f : 0.7f);
}

void AVespCombat::Fermer(int32 g)
{
	FVespGroupe& G = Groupes[g];
	if (G.bFerme || G.bLibere)
	{
		return;
	}
	G.bFerme = true;
	CentreEnclos = G.Centre;
	RayonEnclos = G.Rayon * 0.97f;
	ConstruireBarriere(G.Centre, RayonEnclos);
	UVespSons::Jouer(this, EVespSon::Barriere, Aylis ? Aylis->GetActorLocation() : G.Centre, 1.0f);
	Engager(g);
	if (SurFermeture) SurFermeture(G.Zone);
	if (G.VaguesRestantes > 0 && SurMessage)
	{
		SurMessage(FString::Printf(TEXT("D'autres Haschen attendent dans l'ombre (%d vague%s de plus)."), G.VaguesRestantes, G.VaguesRestantes > 1 ? TEXT("s") : TEXT("")));
	}
}

void AVespCombat::Ouvrir(int32 g)
{
	FVespGroupe& G = Groupes[g];
	G.bFerme = false;
	G.bLibere = true;
	RayonEnclos = 0.0f;
	EffacerBarriere();
	UVespSons::Jouer(this, EVespSon::Victoire, Aylis ? Aylis->GetActorLocation() : G.Centre, 1.0f);
	// Les dangers qui restent s'effacent
	for (FVespDanger& Z : Dangers)
	{
		EffacerDanger(Z);
	}
	Dangers.Reset();
	if (SurLiberation) SurLiberation(G.Zone);
}

void AVespCombat::VagueSuivante(int32 g)
{
	FVespGroupe& G = Groupes[g];
	G.VaguesRestantes--;
	const int32 Nombre = 2 + (Acte >= 3 ? 1 : 0) + (G.Etage >= 8 ? 1 : 0);
	for (int32 i = 0; i < Nombre; i++)
	{
		FVector P = PlaceDans(G.Centre, G.Rayon, 0.4f, 0.8f);
		for (int32 k = 0; k < 6 && Aylis && FVector::Dist2D(P, Aylis->GetActorLocation()) < 450.0f; k++)
		{
			P = PlaceDans(G.Centre, G.Rayon, 0.4f, 0.8f);
		}
		FVespAppel A;
		A.Position = P;
		A.Modele = FMath::RandRange(0, 4);
		A.Groupe = g;
		Appels.Add(A);
	}
	if (SurMessage) SurMessage(TEXT("Des renforts surgissent de terre !"));
	if (SurImpact) SurImpact(0.6f, false);
}

void AVespCombat::Invoquer(FVespHaschen& Source, int32 Nombre, int32 IndexModele, int32 Categorie)
{
	AVespUnite* S = Source.U.Get();
	if (!S)
	{
		return;
	}
	for (int32 i = 0; i < Nombre; i++)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		FVespAppel A;
		A.Position = S->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FMath::FRandRange(180.0f, 320.0f);
		if (Monde && !Monde->EstPraticable(A.Position))
		{
			A.Position = S->GetActorLocation();
		}
		if (RayonEnclos > 0.0f && FVector::Dist2D(A.Position, CentreEnclos) > RayonEnclos - 80.0f)
		{
			A.Position = CentreEnclos + (A.Position - CentreEnclos).GetSafeNormal2D() * (RayonEnclos - 120.0f);
		}
		A.Modele = IndexModele;
		A.Categorie = Categorie;
		A.Groupe = Source.Groupe;
		A.Invocateur = S;
		Appels.Add(A);
		Source.Invoques++;
	}
}

void AVespCombat::CreerAppels()
{
	TArray<FVespAppel> AFaire = MoveTemp(Appels);
	Appels.Reset();
	for (const FVespAppel& A : AFaire)
	{
		const FVespGroupe* G = Groupes.IsValidIndex(A.Groupe) ? &Groupes[A.Groupe] : nullptr;
		AVespUnite* U = Creer(A.Modele, A.Categorie, A.Position, A.Groupe, G ? G->Etage : 0);
		if (!U)
		{
			continue;
		}
		FVespHaschen& H = Haschen.Last();
		H.Invocateur = A.Invocateur;
		U->Jouer(EVespGeste::Resurrection, 1.3f);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, U->GetActorLocation(), FVector::UpVector, FLinearColor(0.5f, 0.3f, 0.35f));
		AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, U->GetActorLocation(), FVector::UpVector, FLinearColor(0.8f, 0.35f, 0.3f));
		Changer(H, EVespIntention::Surgir);
		if (Aylis)
		{
			U->TournerDUnCoup(Aylis->GetActorLocation() - U->GetActorLocation());
		}
	}
}

void AVespCombat::Mort(FVespHaschen& H)
{
	H.bTombe = true;
	AVespUnite* U = H.U.Get();
	if (!U)
	{
		return;
	}
	UVespSons::Jouer(this, EVespSon::Os, U->GetActorLocation(), 0.9f);
	// Ses attaques annoncees s'effacent
	for (int32 i = Dangers.Num() - 1; i >= 0; i--)
	{
		if (Dangers[i].Source.Get() == U && !Dangers[i].bFrappee && Dangers[i].bAnnuleeSiSourceTombe)
		{
			EffacerDanger(Dangers[i]);
			Dangers.RemoveAt(i);
		}
	}
	// L'explosif explose (et blesse tout le monde autour, Haschen compris)
	if ((U->Stats.Capacites & VespCapacite::Explosif) && !U->bAExplose)
	{
		U->bAExplose = true;
		FVespDanger& Z = Annoncer(U->GetActorLocation(), 280.0f, 0.9f, 8 + Acte * 2, U);
		Z.bContreHaschen = true;
		Z.bAnnuleeSiSourceTombe = false;
		Z.Effet = U->Stats.Effet;
		Z.Son = EVespSon::Explosion;
		Z.Poussee = 700.0f;
		Z.Couleur = U->Stats.Effet == VespEffetCoup::Gel ? FLinearColor(0.35f, 0.65f, 1.0f) : FLinearColor(1.0f, 0.4f, 0.05f);
		if (SurMessage) SurMessage(U->Stats.Nom + TEXT(" va exploser !"));
	}
	if (AVespUnite* Inv = H.Invocateur.Get())
	{
		if (FVespHaschen* I = Trouver(Inv))
		{
			I->Invoques = FMath::Max(0, I->Invoques - 1);
		}
	}
	if (SurChute) SurChute(U, H.bBoss ? 2 : (H.bElite ? 1 : 0));
}

// ===================== Les regles de chaque acte (pendant les combats des clairieres) =====================

void AVespCombat::ReglesDeLActe(FVespGroupe& G, float Secondes)
{
	if (!Aylis || !Aylis->EstDebout())
	{
		return;
	}
	G.Chrono += Secondes;
	const FVector Ici = Aylis->GetActorLocation();
	auto Pres = [&](float Ecart) {
		const float A = FMath::FRandRange(0.0f, 2.0f * PI);
		FVector P = Ici + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * FMath::FRandRange(0.0f, Ecart);
		if (RayonEnclos > 0.0f && FVector::Dist2D(P, CentreEnclos) > RayonEnclos - 150.0f)
		{
			P = CentreEnclos + (P - CentreEnclos).GetSafeNormal2D() * (RayonEnclos - 200.0f);
		}
		return P;
	};
	auto Flaque = [&](float Rayon, float Duree, int32 Degats, int32 Effet, const FLinearColor& Couleur, bool bHaschen) {
		FVespDanger& Z = Annoncer(Pres(260.0f), Rayon, 1.1f, Degats, nullptr);
		Z.Persistance = Duree;
		Z.Effet = Effet;
		Z.Couleur = Couleur;
		Z.bContreHaschen = bHaschen;
		Z.bAnnuleeSiSourceTombe = false;
		Z.Poussee = 150.0f;
	};
	switch (Acte)
	{
		case 2:		// le poison ronge le bois : des nappes empoisonnees
			if (G.Chrono > 8.0f)
			{
				G.Chrono = 0.0f;
				Flaque(190.0f, 5.0f, 4, VespEffetCoup::Poison, FLinearColor(0.35f, 1.0f, 0.15f), false);
			}
			break;
		case 3:		// les eaux toxiques montent
			if (G.Chrono > 9.0f)
			{
				G.Chrono = 0.0f;
				Flaque(220.0f, 6.0f, 5, VespEffetCoup::Poison, FLinearColor(0.2f, 0.9f, 0.5f), true);
				Flaque(220.0f, 6.0f, 5, VespEffetCoup::Poison, FLinearColor(0.2f, 0.9f, 0.5f), true);
				if (SurMessage) SurMessage(TEXT("Les eaux toxiques montent..."));
			}
			break;
		case 4:		// les dalles piegees se levent
			if (G.Chrono > 4.5f)
			{
				G.Chrono = 0.0f;
				for (const FVector& P : G.Pieges)
				{
					FVespDanger& Z = Annoncer(P, 150.0f, 1.0f, 8 + Acte, nullptr);
					Z.bContreHaschen = true;
					Z.bAnnuleeSiSourceTombe = false;
					Z.Couleur = FLinearColor(1.0f, 0.55f, 0.1f);
				}
			}
			break;
		case 5:		// le blizzard
			if (G.Chrono > 16.0f)
			{
				G.Chrono = 0.0f;
				TempsBlizzard = 4.5f;
				if (SurMessage) SurMessage(TEXT("Le blizzard se lève : tout le monde ralentit !"));
			}
			break;
		case 6:		// le sol se fissure et entre en eruption
			if (G.Chrono > 5.0f)
			{
				G.Chrono = 0.0f;
				for (int32 k = 0; k < 2; k++)
				{
					FVespDanger& Z = Annoncer(Pres(420.0f), 200.0f, 1.2f, 6 + Acte, nullptr);
					Z.Persistance = 1.5f;
					Z.Effet = VespEffetCoup::Brulure;
					Z.bContreHaschen = true;
					Z.bAnnuleeSiSourceTombe = false;
					Z.Couleur = FLinearColor(1.0f, 0.35f, 0.02f);
					Z.Son = EVespSon::Explosion;
				}
			}
			break;
		case 7:		// le Voile se dechire : un echo surgit
			if (G.Chrono > 12.0f && HaschenEngages() < 8)
			{
				G.Chrono = 0.0f;
				FVespAppel A;
				A.Position = Pres(450.0f);
				A.Modele = 0;
				A.Groupe = int32(&G - Groupes.GetData());
				Appels.Add(A);
				AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Circle1"), A.Position, FRotator::ZeroRotator, 1.0f);
				if (SurMessage) SurMessage(TEXT("Le Voile se déchire : un écho en sort !"));
			}
			break;
		default: break;
	}
}

// ===================== A chaque image =====================

void AVespCombat::Avancer(float Secondes)
{
	Horloge += Secondes;
	TempsBlizzard = FMath::Max(0.0f, TempsBlizzard - Secondes);
	const FVector Ici = Aylis ? Aylis->GetActorLocation() : FVector::ZeroVector;

	// La barriere monte du sol et pulse
	if (Barriere.Num() > 0)
	{
		TempsBarriere += Secondes;
		const float Hauteur = FMath::Min(1.0f, TempsBarriere / 0.6f) * 1.7f;
		for (UStaticMeshComponent* C : Barriere)
		{
			C->SetWorldScale3D(FVector(0.1f, 0.1f, FMath::Max(0.01f, Hauteur)));
			C->SetWorldLocation(FVector(C->GetComponentLocation().X, C->GetComponentLocation().Y, SolZ + Hauteur * 50.0f));
		}
		MatBarriere->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.35f, 1.0f) * (1.3f + 0.4f * FMath::Sin(Horloge * 3.0f)));
	}

	// Les groupes : on les reveille, on les libere
	for (int32 g = 0; g < Groupes.Num(); g++)
	{
		FVespGroupe& G = Groupes[g];
		if (G.bLibere)
		{
			continue;
		}
		const float D = FVector::Dist2D(Ici, G.Centre);
		if (Aylis && Aylis->EstDebout())
		{
			if (G.Zone >= 0 && !G.bFerme && D < G.Rayon * 0.72f)
			{
				Fermer(g);
			}
			else if (G.Zone < 0 && !G.bEngage)
			{
				for (const FVespHaschen& H : Haschen)
				{
					if (H.Groupe == g && H.U.IsValid() && H.U->EstDebout() && FVector::Dist2D(H.U->GetActorLocation(), Ici) < 950.0f)
					{
						Engager(g);
						break;
					}
				}
			}
		}
		if (!G.bEngage)
		{
			continue;
		}
		int32 Debout = 0;
		float PlusProche = MAX_flt;
		for (const FVespHaschen& H : Haschen)
		{
			if (H.Groupe == g && H.U.IsValid() && H.U->EstDebout())
			{
				Debout++;
				PlusProche = FMath::Min(PlusProche, FVector::Dist2D(H.U->GetActorLocation(), Ici));
			}
		}
		bool bAppelEnCours = false;
		for (const FVespAppel& A : Appels)
		{
			bAppelEnCours |= A.Groupe == g;
		}
		if (Debout == 0 && !bAppelEnCours)
		{
			if (G.VaguesRestantes > 0 && G.bFerme)
			{
				VagueSuivante(g);
			}
			else if (G.Zone >= 0)
			{
				if (G.bFerme)
				{
					Ouvrir(g);
				}
			}
			else
			{
				G.bLibere = true;
			}
			continue;
		}
		// Une patrouille distancee abandonne la poursuite
		if (G.Zone < 0 && PlusProche > 2800.0f)
		{
			G.bEngage = false;
			for (FVespHaschen& H : Haschen)
			{
				if (H.Groupe == g)
				{
					Changer(H, EVespIntention::Errer);
				}
			}
		}
		if (G.bFerme)
		{
			ReglesDeLActe(G, Secondes);
		}
	}

	// Chaque Haschen reflechit (les lointains dorment : ca ne coute rien)
	for (int32 i = 0; i < Haschen.Num(); i++)
	{
		FVespHaschen& H = Haschen[i];
		AVespUnite* U = H.U.Get();
		if (!U)
		{
			continue;
		}
		if (!U->EstDebout())
		{
			if (!H.bTombe)
			{
				Mort(H);
			}
			continue;
		}
		if (FVector::Dist2D(U->GetActorLocation(), Ici) > 5000.0f && H.Etat != EVespIntention::Errer)
		{
			continue;
		}
		if (H.bBoss)
		{
			PenserBoss(Haschen[i], Secondes);
		}
		else
		{
			Penser(Haschen[i], Secondes);
		}
	}
	// Les corps finissent par disparaitre
	for (int32 i = Haschen.Num() - 1; i >= 0; i--)
	{
		AVespUnite* U = Haschen[i].U.Get();
		if (!U || (Haschen[i].bTombe && U->TempsDepuisMort() > 7.0f))
		{
			if (U) U->Destroy();
			Haschen.RemoveAt(i);
		}
	}
	CreerAppels();
	Eloigner(Secondes);
	AvancerDangers(Secondes);
	AvancerProjectiles(Secondes);

	// La barriere retient AYLIS (et les Haschen) dans la clairiere
	if (RayonEnclos > 0.0f)
	{
		auto Retenir = [this](AVespUnite* U) {
			const FVector P = U->GetActorLocation();
			const FVector V = P - CentreEnclos;
			const float D = FVector(V.X, V.Y, 0.0f).Size();
			const float Max = RayonEnclos - U->Rayon() - 15.0f;
			if (D > Max && D > 1.0f)
			{
				const FVector Dans = CentreEnclos + FVector(V.X, V.Y, 0.0f) / D * Max;
				U->SetActorLocation(FVector(Dans.X, Dans.Y, P.Z));
			}
		};
		if (Aylis) Retenir(Aylis);
		for (FVespHaschen& H : Haschen)
		{
			if (H.U.IsValid() && H.U->EstDebout() && Groupes.IsValidIndex(H.Groupe) && Groupes[H.Groupe].bFerme)
			{
				Retenir(H.U.Get());
			}
		}
	}
}

void AVespCombat::Eloigner(float Secondes)
{
	if (!Aylis)
	{
		return;
	}
	const FVector Ici = Aylis->GetActorLocation();
	TArray<AVespUnite*> Proches;
	for (FVespHaschen& H : Haschen)
	{
		AVespUnite* U = H.U.Get();
		if (U && U->EstDebout() && !U->IsHidden() && !U->EstEnLAir() && FVector::Dist2D(U->GetActorLocation(), Ici) < 3000.0f)
		{
			Proches.Add(U);
		}
	}
	auto Pousser = [this](AVespUnite* U, const FVector& Decalage) {
		const FVector P = U->GetActorLocation();
		U->SetActorLocation(Monde ? Monde->Contraindre(P, P + Decalage) : P + Decalage);
	};
	for (int32 a = 0; a < Proches.Num(); a++)
	{
		for (int32 b = a + 1; b < Proches.Num(); b++)
		{
			FVector V = Proches[b]->GetActorLocation() - Proches[a]->GetActorLocation();
			V.Z = 0.0f;
			const float D = V.Size();
			const float Min = Proches[a]->Rayon() + Proches[b]->Rayon() + 20.0f;
			if (D < Min)
			{
				const FVector N = D > 1.0f ? V / D : FVector(1, 0, 0);
				const float Ecart = (Min - D) * 0.5f;
				const bool bBossA = Proches[a]->Stats.Boss > 0, bBossB = Proches[b]->Stats.Boss > 0;
				Pousser(Proches[a], -N * Ecart * (bBossA ? 0.2f : (bBossB ? 1.8f : 1.0f)));
				Pousser(Proches[b], N * Ecart * (bBossB ? 0.2f : (bBossA ? 1.8f : 1.0f)));
			}
		}
		// Et personne ne passe a travers AYLIS
		if (Aylis->EstDebout())
		{
			FVector V = Proches[a]->GetActorLocation() - Ici;
			V.Z = 0.0f;
			const float D = V.Size();
			const float Min = Proches[a]->Rayon() + Aylis->Rayon() + 10.0f;
			if (D < Min && Aylis->Invulnerable <= 0.0f)
			{
				const FVector N = D > 1.0f ? V / D : FVector(1, 0, 0);
				Pousser(Proches[a], N * (Min - D) * 0.7f);
				Pousser(Aylis, -N * (Min - D) * 0.3f);
			}
		}
	}
}

// ===================== L'esprit d'un Haschen =====================

static float TempsDePreparation(int32 Acte, bool bElite)
{
	return (0.8f - 0.04f * (Acte - 1)) * (bElite ? 0.9f : 1.0f);
}

void AVespCombat::PreparerAttaque(FVespHaschen& H, int32 Geste)
{
	AVespUnite* U = H.U.Get();
	const FVector Ici = U->GetActorLocation();
	const FVector Cible = Aylis->GetActorLocation();
	const FVector Dir = (Cible - Ici).GetSafeNormal2D();
	H.Geste = Geste;
	H.bImpactFait = false;
	H.Etape = 0;
	Changer(H, EVespIntention::Preparer);
	const float Base = TempsDePreparation(Acte, H.bElite);
	const FVespStats& S = U->Stats;
	const FLinearColor CouleurEffet = S.Effet == VespEffetCoup::Poison ? FLinearColor(0.4f, 1.0f, 0.2f)
	                                : (S.Effet == VespEffetCoup::Gel ? FLinearColor(0.3f, 0.6f, 1.0f)
	                                : (S.Effet == VespEffetCoup::Brulure ? FLinearColor(1.0f, 0.45f, 0.05f) : FLinearColor(1.0f, 0.15f, 0.05f)));
	U->TournerDUnCoup(Dir);
	switch (Geste)
	{
		case 0:		// un coup au contact : la tache rouge devant lui
		{
			const float Portee = 110.0f + U->Taille * 0.35f;
			const float T = Base;
			FVespDanger& Z = Annoncer(Ici + Dir * Portee * 0.55f, Portee * 0.62f, T, DegatsDe(U, 1.0f), U);
			Z.bParable = true;
			Z.Effet = S.Effet;
			Z.Poussee = U->Taille >= 205.0f ? 550.0f : 320.0f;
			const EVespGeste G = FMath::RandBool() ? EVespGeste::Attaque1 : (FMath::RandBool() ? EVespGeste::Attaque2 : EVespGeste::Attaque3);
			const float Duree = U->DureeGeste(G);
			U->Jouer(G, FMath::Clamp(Duree * 0.4f / T, 0.3f, 1.5f));
			H.Vise = Z.Centre;
			U->Annoncer(T, CouleurEffet);
			break;
		}
		case 1:		// un tir
		{
			const float T = Base * 0.9f;
			const float Duree = U->DureeGeste(EVespGeste::Tir);
			U->Jouer(EVespGeste::Tir, FMath::Clamp(Duree * 0.55f / T, 0.3f, 1.5f));
			U->Annoncer(T, CouleurEffet);
			break;
		}
		case 2:		// une charge : la ligne, puis il fonce
		{
			const float T = Base + 0.1f;
			FVespDanger& Z = AnnoncerLigne(Ici, Dir, 950.0f, 170.0f, T, U);
			Z.bDegats = false;
			H.Vise = Dir;
			U->Jouer(EVespGeste::Lourde, 0.5f);
			U->Annoncer(T, CouleurEffet);
			UVespSons::Jouer(this, EVespSon::Annonce, Ici, 0.5f);
			break;
		}
		case 3:		// un saut : l'endroit ou il va retomber
		{
			const float T = Base;
			const FVector Arrivee = Cible;
			FVespDanger& Z = Annoncer(Arrivee, 230.0f, T + 0.8f, DegatsDe(U, 1.2f), U);
			Z.Effet = S.Effet;
			Z.Poussee = 600.0f;
			H.Vise = Arrivee;
			U->Jouer(EVespGeste::GardeLevee, 0.8f);
			U->Annoncer(T, CouleurEffet);
			break;
		}
		case 4:		// un soin
			U->Jouer(EVespGeste::Sort, 0.7f);
			U->Annoncer(0.9f, FLinearColor(0.3f, 1.0f, 0.4f));
			Changer(H, EVespIntention::Incanter);
			break;
		case 5:		// une invocation
			U->Jouer(EVespGeste::Invocation, 0.8f);
			U->Annoncer(1.0f, FLinearColor(0.7f, 0.3f, 1.0f));
			Changer(H, EVespIntention::Incanter);
			break;
		default: break;
	}
}

void AVespCombat::Frapper(FVespHaschen& H)
{
	AVespUnite* U = H.U.Get();
	const FVector Ici = U->GetActorLocation();
	const FVespStats& S = U->Stats;
	switch (H.Geste)
	{
		case 0:
			U->AccelererGeste(1.2f);
			UVespSons::Jouer(this, U->Taille >= 205.0f ? EVespSon::FrappeLourde : EVespSon::Frappe, Ici, 0.7f, 0.9f);
			Changer(H, EVespIntention::Frapper);
			break;
		case 1:
		{
			// Il vise un peu devant AYLIS (la ou elle sera)
			const FVector Prevu = Aylis->GetActorLocation() + (Aylis->GetVitesseActuelle() > 50.0f ? Aylis->Avant() * Aylis->GetVitesseActuelle() * 0.25f : FVector::ZeroVector);
			const FVector Dir = (Prevu - Ici).GetSafeNormal2D();
			const bool bMagie = U->GetTypeArme() != EVespArme::Arc;
			const FLinearColor Couleur = S.Effet == VespEffetCoup::Poison ? FLinearColor(0.4f, 1.0f, 0.2f)
			                           : (S.Effet == VespEffetCoup::Gel ? FLinearColor(0.35f, 0.7f, 1.0f)
			                           : (S.Effet == VespEffetCoup::Brulure ? FLinearColor(1.0f, 0.45f, 0.05f)
			                           : (bMagie ? FLinearColor(0.8f, 0.35f, 1.0f) : FLinearColor(1.0f, 0.75f, 0.45f))));
			Tirer(U, Ici + FVector(0, 0, U->Taille * 0.6f) + Dir * 50.0f, Dir, bMagie ? 950.0f : 1250.0f, DegatsDe(U, 0.9f), S.Effet,
			      (S.Capacites & VespCapacite::Attire) != 0, Couleur);
			U->AccelererGeste(1.2f);
			UVespSons::Jouer(this, bMagie ? EVespSon::Sort : EVespSon::Tir, Ici, 0.7f);
			Changer(H, EVespIntention::Frapper);
			break;
		}
		case 2:
			UVespSons::Jouer(this, EVespSon::FrappeLourde, Ici, 0.8f, 0.8f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, Ici, FVector::UpVector, FLinearColor(0.5f, 0.4f, 0.35f));
			U->ArreterGeste();
			Changer(H, EVespIntention::Charger);
			break;
		case 3:
			U->Bondir(H.Vise, 0.8f, 260.0f);
			Changer(H, EVespIntention::Sauter);
			break;
		default: break;
	}
}

void AVespCombat::Penser(FVespHaschen& H, float Secondes)
{
	AVespUnite* U = H.U.Get();
	H.Temps += Secondes;
	H.Recharge -= Secondes;
	H.Recharge2 -= Secondes;
	const FVector Ici = U->GetActorLocation();
	switch (H.Etat)
	{
		case EVespIntention::Dormir:
			return;
		case EVespIntention::Errer:
		{
			const FVector But = H.bVersB ? H.ErranceB : H.Errance;
			if (FVector::Dist2D(Ici, But) < 60.0f)
			{
				H.bVersB = !H.bVersB;
			}
			Bouger(H, But, U->Vitesse * 0.4f, Secondes);
			return;
		}
		case EVespIntention::Surgir:
			if (H.Temps > 1.3f)
			{
				Changer(H, EVespIntention::Poursuivre);
			}
			return;
		default: break;
	}
	if (!Aylis || !Aylis->EstDebout())
	{
		return;
	}
	if (U->Etourdi > 0.0f || U->Fige > 0.0f)
	{
		return;
	}
	const FVector Cible = Aylis->GetActorLocation();
	FVector Vers = Cible - Ici;
	Vers.Z = 0.0f;
	const float Dist = Vers.Size();
	const FVector Dir = Vers.GetSafeNormal();
	const FVespStats& S = U->Stats;
	const float PorteeMelee = 110.0f + U->Taille * 0.35f;
	const float PorteeTir = S.Portee * 170.0f;
	const int32 MaxAttaquants = 2 + (Acte >= 4 ? 1 : 0);
	auto Contourner = [&](float Rayon) {
		// Il attend son tour en tournant autour d'AYLIS
		const FVector Autour = (-Dir).RotateAngleAxis(30.0f * H.Contournement, FVector::UpVector);
		Bouger(H, Cible + Autour * Rayon, U->Vitesse * 0.55f, Secondes, false);
		U->Tourner(Dir, Secondes, 8.0f);
	};
	switch (H.Etat)
	{
		case EVespIntention::Poursuivre:
		{
			// Ses talents d'abord
			if ((S.Capacites & VespCapacite::Soigneur) && H.Recharge2 <= 0.0f)
			{
				for (const FVespHaschen& Autre : Haschen)
				{
					const AVespUnite* A = Autre.U.Get();
					if (A && A != U && A->EstDebout() && !A->IsHidden() && A->Stats.Pv < A->Stats.PvMax * 0.6f && FVector::Dist2D(A->GetActorLocation(), Ici) < 1100.0f)
					{
						H.Vise = A->GetActorLocation();
						PreparerAttaque(H, 4);
						return;
					}
				}
			}
			if ((S.Capacites & VespCapacite::Invocateur) && H.Recharge2 <= 0.0f && H.Invoques < 2 && HaschenEngages() < 9)
			{
				PreparerAttaque(H, 5);
				return;
			}
			if ((S.Capacites & VespCapacite::Sauteur) && H.Recharge2 <= 0.0f && Dist > 350.0f && Dist < 1100.0f)
			{
				H.Recharge2 = FMath::FRandRange(6.0f, 9.0f);
				PreparerAttaque(H, 3);
				return;
			}
			switch (S.Style)
			{
				case EVespStyle::Lanceur:
					if (Dist < 380.0f)
					{
						Bouger(H, Ici - Dir * 300.0f, U->Vitesse * 0.9f, Secondes, false);		// il recule
						U->Tourner(Dir, Secondes);
					}
					else if (Dist > PorteeTir * 0.9f)
					{
						Bouger(H, Cible, U->Vitesse, Secondes);
					}
					else
					{
						Bouger(H, Ici + FVector(-Dir.Y, Dir.X, 0.0f) * 200.0f * H.Contournement, U->Vitesse * 0.35f, Secondes, false);
						U->Tourner(Dir, Secondes);
					}
					if (H.Recharge <= 0.0f && Dist < PorteeTir)
					{
						PreparerAttaque(H, 1);
					}
					break;
				case EVespStyle::Chargeur:
					if (H.Recharge <= 0.0f && Dist > 320.0f && Dist < 1000.0f)
					{
						PreparerAttaque(H, 2);
					}
					else if (Dist > PorteeMelee * 0.85f)
					{
						Bouger(H, Cible, U->Vitesse, Secondes);
					}
					else if (H.Recharge <= 0.0f && AttaquantsAuContact() < MaxAttaquants)
					{
						PreparerAttaque(H, 0);
					}
					else
					{
						Contourner(PorteeMelee * 1.4f);
					}
					break;
				default:
					if (Dist > PorteeMelee * 0.85f)
					{
						if (AttaquantsAuContact() >= MaxAttaquants && Dist < 520.0f)
						{
							Contourner(380.0f);
						}
						else
						{
							Bouger(H, Cible, U->Vitesse, Secondes);
						}
					}
					else if (H.Recharge <= 0.0f && AttaquantsAuContact() < MaxAttaquants)
					{
						PreparerAttaque(H, 0);
					}
					else
					{
						U->Tourner(Dir, Secondes);
					}
					break;
			}
			break;
		}
		case EVespIntention::Preparer:
		{
			const float T = H.Geste == 2 ? TempsDePreparation(Acte, H.bElite) + 0.1f : (H.Geste == 1 ? TempsDePreparation(Acte, H.bElite) * 0.9f : TempsDePreparation(Acte, H.bElite));
			if (H.Geste == 1)
			{
				U->Tourner(Dir, Secondes, 10.0f);		// un tireur suit sa cible jusqu'au bout
			}
			if (H.Temps >= T)
			{
				Frapper(H);
			}
			break;
		}
		case EVespIntention::Frapper:
			if (H.Temps > 0.5f)
			{
				Changer(H, EVespIntention::Recuperer);
				H.Recharge = FMath::FRandRange(1.3f, 2.3f) * (S.Style == EVespStyle::Lanceur ? 1.3f : 1.0f);
			}
			break;
		case EVespIntention::Charger:
		{
			const FVector Avant = Ici;
			U->Deplacer(H.Vise * 1500.0f, Secondes, false);
			const float Fait = FVector::Dist2D(Avant, U->GetActorLocation());
			if (!H.bImpactFait && FVector::Dist2D(U->GetActorLocation(), Cible) < U->Rayon() + Aylis->Rayon() + 40.0f)
			{
				H.bImpactFait = true;
				ToucherAylis(DegatsDe(U, 1.3f), S.Effet, U, H.Vise, 750.0f, true, EVespSon::Impact);
			}
			if (H.Temps > 0.65f || (H.Temps > 0.1f && Fait < 1500.0f * Secondes * 0.3f))
			{
				Changer(H, EVespIntention::Recuperer);
				H.Recharge = FMath::FRandRange(1.8f, 2.6f);
				AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, U->GetActorLocation(), FVector::UpVector, FLinearColor(0.5f, 0.4f, 0.35f));
			}
			break;
		}
		case EVespIntention::Sauter:
			if (H.Temps > 0.85f && !U->EstEnLAir())
			{
				Changer(H, EVespIntention::Recuperer);
				H.Recharge = 1.0f;
			}
			break;
		case EVespIntention::Incanter:
			if (H.Temps > 1.0f)
			{
				if (H.Geste == 4)
				{
					// Il soigne l'allie le plus blesse autour de lui
					AVespUnite* Blesse = nullptr;
					for (const FVespHaschen& Autre : Haschen)
					{
						AVespUnite* A = Autre.U.Get();
						if (A && A->EstDebout() && !A->IsHidden() && FVector::Dist2D(A->GetActorLocation(), Ici) < 1200.0f
						    && (!Blesse || (float)A->Stats.Pv / A->Stats.PvMax < (float)Blesse->Stats.Pv / Blesse->Stats.PvMax))
						{
							Blesse = A;
						}
					}
					if (Blesse)
					{
						Blesse->Soigner(FMath::Max(4, Blesse->Stats.PvMax * 30 / 100));
						UVespSons::Jouer(this, EVespSon::Soin, Blesse->GetActorLocation(), 0.8f);
					}
					H.Recharge2 = FMath::FRandRange(6.0f, 8.0f);
				}
				else
				{
					Invoquer(H, FMath::RandRange(1, 2), FMath::RandRange(0, 4));
					UVespSons::Jouer(this, EVespSon::Sort, Ici, 0.8f, 0.7f);
					H.Recharge2 = FMath::FRandRange(9.0f, 12.0f);
				}
				Changer(H, EVespIntention::Recuperer);
			}
			break;
		case EVespIntention::Recuperer:
			if (H.Temps > 0.45f)
			{
				Changer(H, EVespIntention::Poursuivre);
			}
			break;
		default: break;
	}
}

// ===================== Les boss =====================
// Chaque boss a ses motifs (ses attaques). A mi-vie, il entre en rage : tout va plus vite, et il en gagne un.
//   0 frappe   1 ecrasement   2 serie d'ecrasements   3 charge   4 cercle autour de lui   5 pluie de fleches
//   6 salve    7 invocation   8 flaques   9 decoction (soin)   10 eruptions   11 bond   12 teleportation
//   13 nova    14 recul (un saut en arriere)

static const TArray<int32>& MotifsDuBoss(int32 Acte, bool bRage)
{
	static const TArray<int32> M[7][2] = {
		{{0, 1, 2, 3}, {0, 1, 2, 3, 4}},							// Skarn : sa masse brise la terre
		{{6, 8, 7, 9}, {6, 8, 7, 9, 5}},							// la Matriarche : poison, loups, decoctions
		{{0, 13, 8, 7}, {0, 13, 8, 7, 11}},							// le Roi Noye
		{{0, 4, 1, 3}, {0, 4, 1, 3, 11}},							// le Gardien de Pierre
		{{6, 5, 14, 7}, {6, 5, 14, 7, 5}},							// Ashka : fleches et pluies de fleches
		{{0, 3, 10, 4}, {0, 3, 10, 4, 10}},							// Vorgath : charges et eruptions
		{{12, 6, 5, 7, 13, 8}, {12, 6, 5, 7, 13, 8, 10, 2}},		// l'Oracle : tout ce qu'AYLIS a affronte
	};
	return M[FMath::Clamp(Acte, 1, 7) - 1][bRage ? 1 : 0];
}

// Les renforts de chaque boss (l'index du modele de l'acte)
static int32 RenfortDuBoss(int32 Acte)
{
	static const int32 R[7] = {4, 0, 0, 0, 3, 0, 0};
	return R[FMath::Clamp(Acte, 1, 7) - 1];
}

void AVespCombat::LancerMotif(FVespHaschen& H, int32 Motif)
{
	AVespUnite* U = H.U.Get();
	const FVector Ici = U->GetActorLocation();
	const FVector Cible = Aylis->GetActorLocation();
	const FVector Dir = (Cible - Ici).GetSafeNormal2D();
	const float Rage = U->bPhaseDeux ? 0.75f : 1.0f;
	const FVespStats& S = U->Stats;
	H.Motif = Motif;
	H.DernierMotif = Motif;
	H.Etape = 0;
	H.bImpactFait = false;
	H.PvDebut = S.Pv;
	H.Vise = Dir;
	Changer(H, EVespIntention::Preparer);
	U->TournerDUnCoup(Dir);
	const FLinearColor Rouge(1.0f, 0.12f, 0.05f);
	switch (Motif)
	{
		case 0:		// une grande frappe devant lui
		{
			const float Portee = 150.0f + U->Taille * 0.4f;
			FVespDanger& Z = Annoncer(Ici + Dir * Portee * 0.55f, Portee * 0.7f, 0.85f * Rage, DegatsDe(U, 1.3f), U);
			Z.bParable = true;
			Z.Effet = S.Effet;
			Z.Poussee = 650.0f;
			U->Jouer(EVespGeste::Lourde, FMath::Clamp(U->DureeGeste(EVespGeste::Lourde) * 0.45f / (0.85f * Rage), 0.3f, 1.4f));
			U->Annoncer(0.85f * Rage);
			break;
		}
		case 1:		// il ecrase le sol la ou se tient AYLIS
		{
			FVespDanger& Z = Annoncer(Cible, 300.0f, 1.1f * Rage, DegatsDe(U, 1.5f), U);
			Z.Poussee = 700.0f;
			Z.Son = EVespSon::Explosion;
			U->Jouer(EVespGeste::Lourde, 0.6f);
			U->Annoncer(1.1f * Rage);
			break;
		}
		case 2:		// une serie d'ecrasements, en ligne, vers AYLIS
			for (int32 k = 0; k < 5; k++)
			{
				FVespDanger& Z = Annoncer(Ici + Dir * (250.0f + k * 260.0f), 200.0f, (0.9f + k * 0.18f) * Rage, DegatsDe(U, 1.1f), U);
				Z.bAnnuleeSiSourceTombe = false;
				Z.Poussee = 450.0f;
			}
			U->Jouer(EVespGeste::Lourde, 0.7f);
			U->Annoncer(0.9f * Rage);
			break;
		case 3:		// la charge
		{
			FVespDanger& Z = AnnoncerLigne(Ici, Dir, 1300.0f, 260.0f, 0.95f * Rage, U);
			Z.bDegats = false;
			U->Jouer(EVespGeste::Lourde, 0.45f);
			U->Annoncer(0.95f * Rage);
			UVespSons::Jouer(this, EVespSon::Annonce, Ici, 0.8f, 0.8f);
			break;
		}
		case 4:		// un cercle tout autour de lui
		case 13:	// la nova : un tres grand cercle (il faut s'eloigner, ou esquiver au bon moment)
		{
			const bool bNova = Motif == 13;
			FVespDanger& Z = Annoncer(Ici, bNova ? 650.0f : 430.0f, (bNova ? 1.5f : 1.1f) * Rage, DegatsDe(U, bNova ? 1.4f : 1.2f), U);
			Z.Poussee = 800.0f;
			Z.Effet = S.Effet;
			Z.Son = EVespSon::Explosion;
			Z.Couleur = bNova ? FLinearColor(0.7f, 0.2f, 1.0f) : Rouge;
			U->Jouer(bNova ? EVespGeste::Invocation : EVespGeste::Tourbillon, 0.8f);
			U->Annoncer((bNova ? 1.5f : 1.1f) * Rage);
			break;
		}
		case 5:		// la pluie de fleches (ou de pierres, ou d'ombres) autour d'AYLIS
		{
			const int32 Nombre = U->bPhaseDeux ? 14 : 10;
			for (int32 k = 0; k < Nombre; k++)
			{
				const float A = FMath::FRandRange(0.0f, 2.0f * PI);
				const FVector P = Cible + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * FMath::FRandRange(0.0f, 520.0f);
				FVespDanger& Z = Annoncer(k == 0 ? Cible : P, 150.0f, FMath::FRandRange(1.0f, 1.7f) * Rage, DegatsDe(U, 0.8f), U);
				Z.bAnnuleeSiSourceTombe = false;
				Z.Effet = S.Effet;
				Z.Poussee = 200.0f;
				Z.Son = EVespSon::Impact;
			}
			U->Jouer(EVespGeste::Tir, 0.7f);
			UVespSons::Jouer(this, EVespSon::Tir, Ici, 0.9f, 0.8f);
			break;
		}
		case 6:		// une salve en eventail
			U->Jouer(EVespGeste::Tir, 0.8f);
			U->Annoncer(0.7f * Rage, S.Effet == VespEffetCoup::Poison ? FLinearColor(0.4f, 1.0f, 0.2f) : FLinearColor(0.4f, 0.7f, 1.0f));
			break;
		case 7:		// il appelle des renforts
			U->Jouer(EVespGeste::Invocation, 0.8f);
			U->Annoncer(1.0f, FLinearColor(0.7f, 0.3f, 1.0f));
			UVespSons::Jouer(this, EVespSon::Rugissement, Ici, 0.8f, 1.2f);
			break;
		case 8:		// des flaques (poison, eaux toxiques)
			for (int32 k = 0; k < 3; k++)
			{
				const float A = FMath::FRandRange(0.0f, 2.0f * PI);
				FVespDanger& Z = Annoncer(Cible + FVector(FMath::Cos(A), FMath::Sin(A), 0.0f) * (k == 0 ? 0.0f : FMath::FRandRange(250.0f, 450.0f)), 230.0f, 1.2f * Rage, DegatsDe(U, 0.5f), U);
				Z.bAnnuleeSiSourceTombe = false;
				Z.Persistance = 6.0f;
				Z.Effet = VespEffetCoup::Poison;
				Z.Couleur = FLinearColor(0.3f, 1.0f, 0.2f);
				Z.Poussee = 100.0f;
			}
			U->Jouer(EVespGeste::Lancer, 0.8f);
			break;
		case 9:		// une decoction : elle se soigne (un coup lourd la lui fait lacher)
			U->Jouer(EVespGeste::Potion, 0.5f);
			U->Annoncer(2.0f, FLinearColor(0.3f, 1.0f, 0.4f));
			if (SurMessage) SurMessage(S.Nom + TEXT(" boit une decoction... un coup lourd peut l'arreter !"));
			break;
		case 10:	// les eruptions : partout dans l'arene
		{
			const int32 Nombre = U->bPhaseDeux ? 9 : 6;
			for (int32 k = 0; k < Nombre; k++)
			{
				const FVector P = k < 2 ? Cible + FMath::VRand().GetSafeNormal2D() * FMath::FRandRange(0.0f, 200.0f)
				                        : (RayonEnclos > 0.0f ? PlaceDans(CentreEnclos, RayonEnclos, 0.0f, 0.85f) : PlaceDans(Cible, 700.0f, 0.0f, 1.0f));
				FVespDanger& Z = Annoncer(P, 210.0f, FMath::FRandRange(1.0f, 1.6f) * Rage, DegatsDe(U, 1.0f), U);
				Z.bAnnuleeSiSourceTombe = false;
				Z.Persistance = 2.0f;
				Z.Effet = VespEffetCoup::Brulure;
				Z.bContreHaschen = true;
				Z.Couleur = FLinearColor(1.0f, 0.35f, 0.02f);
				Z.Son = EVespSon::Explosion;
			}
			U->Jouer(EVespGeste::Lourde, 0.6f);
			break;
		}
		case 11:	// un bond sur AYLIS
		{
			FVespDanger& Z = Annoncer(Cible, 320.0f, 1.0f * Rage + 0.8f, DegatsDe(U, 1.5f), U);
			Z.Poussee = 800.0f;
			Z.Son = EVespSon::Explosion;
			Z.bAnnuleeSiSourceTombe = false;
			H.Vise = Cible;
			U->Jouer(EVespGeste::GardeLevee, 0.7f);
			U->Annoncer(1.0f * Rage);
			break;
		}
		case 12:	// il disparait dans le Voile, et reapparait ailleurs
			U->Jouer(EVespGeste::Invocation, 1.0f);
			AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Circle1"), Ici, FRotator::ZeroRotator, 1.2f);
			break;
		case 14:	// un saut en arriere (Ashka garde ses distances)
			H.Vise = Ici - Dir * 650.0f;
			if (Monde && !Monde->EstPraticable(H.Vise)) H.Vise = Ici + FVector(-Dir.Y, Dir.X, 0.0f) * 600.0f;
			if (RayonEnclos > 0.0f && FVector::Dist2D(H.Vise, CentreEnclos) > RayonEnclos - 150.0f)
			{
				H.Vise = CentreEnclos + (H.Vise - CentreEnclos).GetSafeNormal2D() * (RayonEnclos - 200.0f);
			}
			U->Bondir(H.Vise, 0.6f, 220.0f);
			break;
		default: break;
	}
}

void AVespCombat::AvancerMotif(FVespHaschen& H, float Secondes)
{
	AVespUnite* U = H.U.Get();
	const FVector Ici = U->GetActorLocation();
	const FVector Cible = Aylis->GetActorLocation();
	const FVector Dir = (Cible - Ici).GetSafeNormal2D();
	const float Rage = U->bPhaseDeux ? 0.75f : 1.0f;
	const float T = H.Temps;
	auto Fin = [&](float Repos) {
		H.Motif = -1;
		Changer(H, EVespIntention::Recuperer);
		H.Recharge = Repos * Rage;
	};
	switch (H.Motif)
	{
		case 0: case 1: case 2: case 4: case 13:
			if (!H.bImpactFait && T > (H.Motif == 13 ? 1.5f : (H.Motif == 4 || H.Motif == 1 ? 1.1f : 0.85f)) * Rage)
			{
				H.bImpactFait = true;
				U->AccelererGeste(1.2f);
				UVespSons::Jouer(this, EVespSon::FrappeLourde, Ici, 1.0f, 0.7f);
				if (SurImpact) SurImpact(0.6f, false);
			}
			if (T > (H.Motif == 2 ? 2.0f : 1.6f) * Rage)
			{
				Fin(1.3f);
			}
			break;
		case 3:
			if (H.Etape == 0 && T > 0.95f * Rage)
			{
				H.Etape = 1;
				H.bImpactFait = false;
				U->ArreterGeste();
				UVespSons::Jouer(this, EVespSon::Rugissement, Ici, 0.6f, 1.3f);
			}
			if (H.Etape == 1)
			{
				const FVector Avant = U->GetActorLocation();
				U->Deplacer(H.Vise * 1700.0f, Secondes, false);
				if (!H.bImpactFait && FVector::Dist2D(U->GetActorLocation(), Cible) < U->Rayon() + Aylis->Rayon() + 50.0f)
				{
					H.bImpactFait = true;
					ToucherAylis(DegatsDe(U, 1.6f), U->Stats.Effet, U, H.Vise, 900.0f, true, EVespSon::Impact);
				}
				if (T > 0.95f * Rage + 0.8f || FVector::Dist2D(Avant, U->GetActorLocation()) < 1700.0f * Secondes * 0.3f)
				{
					AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, U->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.6f, 0.45f, 0.35f));
					if (SurImpact) SurImpact(0.7f, false);
					Fin(1.6f);		// essouffle : c'est le moment de frapper
				}
			}
			break;
		case 5:
			if (T > 1.8f * Rage) Fin(1.2f);
			break;
		case 6:
			if (H.Etape < (U->bPhaseDeux ? 2 : 1) && T > 0.7f * Rage + H.Etape * 0.45f)
			{
				const int32 Nombre = U->bPhaseDeux ? 7 : 5;
				const FLinearColor Couleur = U->Stats.Effet == VespEffetCoup::Poison ? FLinearColor(0.4f, 1.0f, 0.2f)
				                           : (U->Stats.Effet == VespEffetCoup::Gel ? FLinearColor(0.35f, 0.7f, 1.0f) : FLinearColor(0.8f, 0.35f, 1.0f));
				for (int32 k = 0; k < Nombre; k++)
				{
					const float Angle = (k - (Nombre - 1) * 0.5f) * 11.0f;
					Tirer(U, Ici + FVector(0, 0, U->Taille * 0.55f), Dir.RotateAngleAxis(Angle, FVector::UpVector), 1000.0f, DegatsDe(U, 0.8f),
					      U->Stats.Effet, false, Couleur);
				}
				UVespSons::Jouer(this, U->GetTypeArme() == EVespArme::Arc ? EVespSon::Tir : EVespSon::Sort, Ici, 1.0f);
				H.Etape++;
			}
			if (T > 1.6f * Rage) Fin(1.2f);
			break;
		case 7:
			if (H.Etape == 0 && T > 1.0f)
			{
				H.Etape = 1;
				if (HaschenEngages() < 8)
				{
					Invoquer(H, Acte == 2 ? 2 : (U->bPhaseDeux ? 3 : 2), RenfortDuBoss(Acte));
				}
			}
			if (T > 1.6f) Fin(2.0f);
			break;
		case 8: case 10:
			if (T > 1.4f * Rage) Fin(1.3f);
			break;
		case 9:
			if (U->Stats.Pv < H.PvDebut - U->Stats.PvMax / 12 || H.PvDebut > 90000)
			{
				U->ArreterGeste();
				U->AfficherMessage(TEXT("la fiole se brise !"), FColor(255, 220, 130), 36.0f);
				UVespSons::Jouer(this, EVespSon::ImpactArmure, Ici, 1.0f, 1.4f);
				U->Etourdi = 1.2f;
				Fin(1.5f);
			}
			else if (T > 2.0f)
			{
				U->Soigner(U->Stats.PvMax * 18 / 100);
				UVespSons::Jouer(this, EVespSon::Potion, Ici, 1.0f, 0.8f);
				Fin(1.5f);
			}
			break;
		case 11:
			if (H.Etape == 0 && T > 1.0f * Rage)
			{
				H.Etape = 1;
				U->Bondir(H.Vise, 0.8f, 380.0f);
			}
			if (H.Etape == 1 && T > 1.0f * Rage + 0.85f && !U->EstEnLAir())
			{
				Fin(1.6f);
			}
			break;
		case 12:
			if (H.Etape == 0 && T > 0.6f)
			{
				H.Etape = 1;
				// Il reapparait loin d'AYLIS
				FVector P = RayonEnclos > 0.0f ? PlaceDans(CentreEnclos, RayonEnclos, 0.4f, 0.8f) : Ici;
				for (int32 k = 0; k < 8 && FVector::Dist2D(P, Cible) < 600.0f; k++)
				{
					P = PlaceDans(CentreEnclos, RayonEnclos, 0.4f, 0.8f);
				}
				U->Teleporter(FVector(P.X, P.Y, Ici.Z));
				AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Circle1"), U->GetActorLocation(), FRotator::ZeroRotator, 1.2f);
				UVespSons::Jouer(this, EVespSon::Sort, U->GetActorLocation(), 1.0f, 0.6f);
			}
			if (T > 1.0f) Fin(0.4f);
			break;
		case 14:
			if (T > 0.7f && !U->EstEnLAir()) Fin(0.3f);
			break;
		default:
			Fin(1.0f);
			break;
	}
}

void AVespCombat::PenserBoss(FVespHaschen& H, float Secondes)
{
	AVespUnite* U = H.U.Get();
	H.Temps += Secondes;
	H.Recharge -= Secondes;
	if (!Groupes.IsValidIndex(H.Groupe) || !Groupes[H.Groupe].bEngage || !Aylis || !Aylis->EstDebout())
	{
		return;
	}
	// La rage, a mi-vie
	if (!U->bPhaseDeux && U->Stats.Pv <= U->Stats.PvMax / 2)
	{
		U->bPhaseDeux = true;
		U->Surbrillance(0.6f, FLinearColor(1.0f, 0.3f, 0.1f));
		U->Vitesse *= 1.15f;
		UVespSons::Jouer(this, EVespSon::Rugissement, U->GetActorLocation(), 1.0f, 0.85f);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, U->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(1.0f, 0.3f, 0.1f));
		if (SurMessage) SurMessage(U->Stats.Nom + TEXT(" entre dans une rage folle !"));
		if (SurImpact) SurImpact(1.0f, false);
	}
	if (U->Etourdi > 0.0f || U->Fige > 0.0f)
	{
		if (H.Motif >= 0)
		{
			H.Motif = -1;
			Changer(H, EVespIntention::Recuperer);
		}
		return;
	}
	const FVector Ici = U->GetActorLocation();
	const FVector Cible = Aylis->GetActorLocation();
	FVector Vers = Cible - Ici;
	Vers.Z = 0.0f;
	const float Dist = Vers.Size();
	const FVector Dir = Vers.GetSafeNormal();
	if (H.Motif >= 0)
	{
		AvancerMotif(H, Secondes);
		return;
	}
	switch (H.Etat)
	{
		case EVespIntention::Dormir:
		case EVespIntention::Surgir:
			Changer(H, EVespIntention::Poursuivre);
			H.Recharge = 1.5f;
			break;
		case EVespIntention::Recuperer:
			if (H.Temps > 0.5f)
			{
				Changer(H, EVespIntention::Poursuivre);
			}
			break;
		default:
		{
			// Les tireurs gardent leurs distances ; les autres viennent au contact
			const bool bDistance = U->Stats.Style == EVespStyle::Lanceur;
			const float Voulue = bDistance ? 700.0f : 220.0f + U->Rayon();
			if (Dist > Voulue + 80.0f)
			{
				Bouger(H, Cible, U->Vitesse, Secondes);
			}
			else if (bDistance && Dist < Voulue - 250.0f)
			{
				Bouger(H, Ici - Dir * 300.0f, U->Vitesse * 0.8f, Secondes, false);
				U->Tourner(Dir, Secondes);
			}
			else
			{
				U->Tourner(Dir, Secondes, 6.0f);
			}
			if (H.Recharge <= 0.0f)
			{
				const TArray<int32>& Motifs = MotifsDuBoss(Acte, U->bPhaseDeux);
				int32 Motif = Motifs[FMath::RandRange(0, Motifs.Num() - 1)];
				for (int32 k = 0; k < 4 && (Motif == H.DernierMotif || (Motif == 0 && Dist > 450.0f) || (Motif == 9 && U->Stats.Pv > U->Stats.PvMax * 0.7f)
				                             || (Motif == 7 && HaschenEngages() >= 6)); k++)
				{
					Motif = Motifs[FMath::RandRange(0, Motifs.Num() - 1)];
				}
				LancerMotif(H, Motif);
			}
			break;
		}
	}
}
