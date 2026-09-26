#include "VespMonde.h"
#include "VespGrille.h"
#include "VespEffet.h"
#include "VespStyles.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

// Les types de clairieres (les memes valeurs que EVespSalle)
namespace
{
	constexpr int32 REPOS = 2, MARCHAND = 3, EVENEMENT = 4, BOSS = 5, DEPART = 6, TRESOR = 7;
}

// ===================== Les packs =====================

UStaticMesh* AVespMonde::Modele(const FString& Chemin)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Deja;
	if (TWeakObjectPtr<UStaticMesh>* M = Deja.Find(Chemin))
	{
		if (M->IsValid())
		{
			return M->Get();
		}
	}
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Chemin, nullptr, LOAD_NoWarn | LOAD_Quiet);
	Deja.Add(Chemin, M);
	return M;
}

// Un modele du pack StylizedProvencal (le monde "sain"), ou du pack Planet385CY (la corruption du Voile)
static UStaticMesh* Provencal(const TCHAR* Nom)
{
	return AVespMonde::Modele(FString::Printf(TEXT("/Game/StylizedProvencal/Meshes/%s.%s"), Nom, Nom));
}

static UStaticMesh* Planete(const TCHAR* Dossier, const TCHAR* Nom)
{
	return AVespMonde::Modele(FString::Printf(TEXT("/Game/Planet385CY/Meshes/%s/%s.%s"), Dossier, Nom, Nom));
}

// Un modele du Fantastic_Village_Pack (le village : lanternes, braseros, potences, coffres...)
static UStaticMesh* Village(const TCHAR* Dossier, const TCHAR* Nom)
{
	return AVespMonde::Modele(FString::Printf(TEXT("/Game/Fantastic_Village_Pack/meshes/%s/%s.%s"), Dossier, Nom, Nom));
}

// Un modele de la foret stylisee de StyleHex (bouleaux, feuillus, herbes, pierres)
static UStaticMesh* ForetStylisee(const TCHAR* Dossier, const TCHAR* Nom)
{
	return AVespMonde::Modele(FString::Printf(TEXT("/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes/%s/%s.%s"), Dossier, Nom, Nom));
}

// Un materiau de sol du Pack_Bonus (herbe, pierre, dallage, plancher)
static UMaterialInterface* MateriauDeSol(const TCHAR* Nom)
{
	if (!Nom)
	{
		return nullptr;
	}
	return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Pack_Bonus/Materials/%s.%s"), Nom, Nom), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

static TArray<UStaticMesh*> Liste(std::initializer_list<UStaticMesh*> Modeles)
{
	TArray<UStaticMesh*> L;
	for (UStaticMesh* M : Modeles)
	{
		if (M)
		{
			L.Add(M);		// un modele absent (pack pas importe) est simplement ignore
		}
	}
	return L;
}

// La palette d'un acte : ce qui pousse et ce qui traine le long de la route
struct FVespPalette
{
	TArray<UStaticMesh*> Arbres;		// la foret qui borde le couloir (elle sert de mur)
	TArray<UStaticMesh*> Buissons;
	TArray<UStaticMesh*> Rochers;
	TArray<UStaticMesh*> Falaises;		// les grands rochers, au loin
	TArray<UStaticMesh*> Herbes;
	TArray<UStaticMesh*> Fleurs;
	TArray<UStaticMesh*> Debris;		// au bord du sentier : tonneaux, caisses, planches, poutres
	TArray<UStaticMesh*> Batiments;		// au loin : maisons, tours, remparts, moulins
	TArray<UStaticMesh*> Corruption;	// le Voile : tentacules, cocons, yeux (actes VI et VII)
	TArray<UStaticMesh*> Tombes;
	FVector2D HauteurArbres = FVector2D(520.0f, 820.0f);
	float DensiteArbres = 1.0f, DensiteBuissons = 1.0f, DensiteHerbes = 1.0f, DensiteFleurs = 1.0f;
	float DensiteRochers = 1.0f, DensiteFalaises = 0.3f, DensiteDebris = 0.3f, DensiteBatiments = 0.0f, DensiteCorruption = 0.0f;
	int32 Mares = 0;					// des mares d'eau peinte
	bool bSolPeint = false;
	UMaterialInterface* Sol = nullptr;		// le sol du monde (en dalles de 10 m)
	UMaterialInterface* Pave = nullptr;		// le sentier, s'il est pave ou en planches (sinon : de la terre)
	UStaticMesh* Lumiere = nullptr;		// ce qui eclaire le sentier (lampadaire, bougies, lanterne, brasero)
	float HauteurLumiere = 300.0f;
	FLinearColor CouleurLumiere = FLinearColor(1.0f, 0.66f, 0.32f);
	bool bFlammes = false;				// un vrai feu au sommet (les braseros)
	int32 Maisons = 0;					// des maisons du village, en retrait
};

static FVespPalette PaletteDeLActe(int32 Acte)
{
	FVespPalette P;
	const TArray<UStaticMesh*> Foret = Liste({Provencal(TEXT("SMF_Forest_Tree01")), Provencal(TEXT("SMF_Forest_Tree02")), Provencal(TEXT("SMF_Forest_Tree03")),
	                                          Provencal(TEXT("SMF_Forest_Tree04")), Provencal(TEXT("SMF_Forest_Tree05")), Provencal(TEXT("SMF_Forest_Tree06"))});
	const TArray<UStaticMesh*> Buissons = Liste({Provencal(TEXT("SMF_Forest_Bush_2")), Provencal(TEXT("SMF_Forest_Bush_3")), Provencal(TEXT("SMF_Forest_Bush_4")),
	                                             Provencal(TEXT("SM_Bush_01")), Provencal(TEXT("SM_Bush_02")), Provencal(TEXT("SM_Bush_03")), Provencal(TEXT("SM_Bush_04"))});
	const TArray<UStaticMesh*> RochersForet = Liste({Provencal(TEXT("SMF_Forest_Rock_1")), Provencal(TEXT("SMF_Forest_Rock_2")), Provencal(TEXT("SMF_Forest_Rock_3")),
	                                                 Provencal(TEXT("SMF_Forest_Rock_4"))});
	const TArray<UStaticMesh*> PetitsRochers = Liste({Provencal(TEXT("SM_Rock_Small_01")), Provencal(TEXT("SM_Rock_Small_02")), Provencal(TEXT("SM_Rock_Small_03"))});
	const TArray<UStaticMesh*> GrandsRochers = Liste({Provencal(TEXT("SM_Rock_Large_01")), Provencal(TEXT("SM_Rock_Large_02"))});
	const TArray<UStaticMesh*> Falaises = Liste({Provencal(TEXT("SM_Rock_Cliff_01")), Provencal(TEXT("SM_Rock_Cliff_02")), Provencal(TEXT("SM_Rock_Cliff_03")),
	                                             Provencal(TEXT("SM_Rock_Cliff_04"))});
	const TArray<UStaticMesh*> Herbes = Liste({Provencal(TEXT("SMF_GrassField_01_a")), Provencal(TEXT("SMF_GrassField_01_b")), Provencal(TEXT("SMF_GrassField_02_a")),
	                                           Provencal(TEXT("SMF_GrassField_02_b")), Provencal(TEXT("SMF_Grass_Small_01_a")), Provencal(TEXT("SMF_Grass_Small_01_b")),
	                                           Provencal(TEXT("SMF_Grass_Small_02_a")), Provencal(TEXT("SMF_Grass_Small_02_b"))});
	const TArray<UStaticMesh*> Fleurs = Liste({Provencal(TEXT("SM_Flower_01_a")), Provencal(TEXT("SM_Flower_01_b")), Provencal(TEXT("SM_Flower_01_c")),
	                                           Provencal(TEXT("SM_Flower_02_a")), Provencal(TEXT("SM_Flower_02_b")), Provencal(TEXT("SM_Flower_02_c")),
	                                           Provencal(TEXT("SM_Flower_03_a")), Provencal(TEXT("SM_Flower_03_b"))});
	const TArray<UStaticMesh*> Bois = Liste({Provencal(TEXT("SM_WoodenPlank_01")), Provencal(TEXT("SM_WoodenPlank_02")), Provencal(TEXT("SM_WoodenPlank_03")),
	                                         Provencal(TEXT("SM_WoodenBeam_01")), Provencal(TEXT("SM_WoodenBeam_02")), Provencal(TEXT("SM_WoodenBeam_03"))});
	const TArray<UStaticMesh*> Objets = Liste({Provencal(TEXT("SM_Barrel")), Provencal(TEXT("SM_Barrel_Open")), Provencal(TEXT("SM_Crate_01")), Provencal(TEXT("SM_Crate_02")),
	                                            Provencal(TEXT("SM_Crate_03")), Provencal(TEXT("SM_Cart")), Provencal(TEXT("SM_Wheel")), Provencal(TEXT("SM_Jar_01")),
	                                            Provencal(TEXT("SM_Jar_02"))});
	const TArray<UStaticMesh*> Ruines = Liste({Provencal(TEXT("SM_RockBrick_Medium_01")), Provencal(TEXT("SM_RockBrick_Medium_02")), Provencal(TEXT("SM_RockBrick_Small_01")),
	                                           Provencal(TEXT("SM_RockBrick_Small_02")), Provencal(TEXT("SM_RockBrick_Small_03"))});
	const TArray<UStaticMesh*> Maisons = Liste({Provencal(TEXT("SM_House_01")), Provencal(TEXT("SM_House_02")), Provencal(TEXT("SM_House_Ivy")),
	                                            Provencal(TEXT("SM_House_Main")), Provencal(TEXT("SM_House_Tower")), Provencal(TEXT("SM_House_Passage"))});
	const TArray<UStaticMesh*> ArbreTordu = Liste({Planete(TEXT("Tree"), TEXT("SM_Tree01a"))});
	const TArray<UStaticMesh*> RochersPoreux = Liste({Planete(TEXT("Rocks"), TEXT("SM_PorousRock_01")), Planete(TEXT("Rocks"), TEXT("SM_Rock01a")),
	                                                  Planete(TEXT("Rocks"), TEXT("SM_RocksSmall01")), Planete(TEXT("Rocks"), TEXT("SM_RocksSmall02")),
	                                                  Planete(TEXT("Rocks"), TEXT("SM_RocksSmall03"))});
	const TArray<UStaticMesh*> Tentacules = Liste({Planete(TEXT("Tendrils"), TEXT("SM_Tendrils_01_Cluster_01")), Planete(TEXT("Tendrils"), TEXT("SM_Tendril_01a")),
	                                               Planete(TEXT("Tendrils"), TEXT("SM_Tendril_01b")), Planete(TEXT("Tendrils"), TEXT("SM_Tendril_01c")),
	                                               Planete(TEXT("Tendrils"), TEXT("SM_Tendril_01d")), Planete(TEXT("Tendrils"), TEXT("SM_Tendril_01e"))});
	auto Plus = [](TArray<UStaticMesh*> A, const TArray<UStaticMesh*>& B) { A.Append(B); return A; };
	const TArray<UStaticMesh*> Ferme = Liste({Village(TEXT("props/natural"), TEXT("SM_PROP_hay_01")), Village(TEXT("props/natural"), TEXT("SM_PROP_hay_03")),
	                                          Village(TEXT("props/construction"), TEXT("SM_PROP_fence_v01_01")), Village(TEXT("props/construction"), TEXT("SM_PROP_fence_v01_03")),
	                                          Village(TEXT("props/vehicles"), TEXT("SM_PROP_cart_01")), Village(TEXT("props/natural"), TEXT("SM_PROP_treetrunk_01")),
	                                          Village(TEXT("props/natural"), TEXT("SM_PROP_treetrunk_03")), Village(TEXT("props/natural"), TEXT("SM_PROP_stone_02")),
	                                          Village(TEXT("props/container"), TEXT("SM_PROP_basket_01")), Village(TEXT("props/container"), TEXT("SM_PROP_bucket_01"))});
	const TArray<UStaticMesh*> Potences = Liste({Village(TEXT("props/furniture"), TEXT("SM_PROP_gallows")), Village(TEXT("props/furniture"), TEXT("SM_PROP_gallows")),
	                                             Village(TEXT("props/container"), TEXT("SM_PROP_coffin")), Village(TEXT("props/construction"), TEXT("SM_PROP_fence_v03_02")),
	                                             Village(TEXT("props/vehicles"), TEXT("SM_PROP_cart_03"))});
	const TArray<UStaticMesh*> Garnison = Liste({Village(TEXT("props/deco"), TEXT("SM_PROP_flag_01")), Village(TEXT("props/deco"), TEXT("SM_PROP_flag_03")),
	                                             Village(TEXT("props/container"), TEXT("SM_PROP_barrel_02")), Village(TEXT("props/container"), TEXT("SM_PROP_box_02")),
	                                             Village(TEXT("props/furniture"), TEXT("SM_PROP_guillotine")), Village(TEXT("props/deco"), TEXT("SM_PROP_board_01"))});
	const TArray<UStaticMesh*> Ruelles = Liste({Village(TEXT("props/deco"), TEXT("SM_PROP_signpost_02")), Village(TEXT("props/deco"), TEXT("SM_PROP_clothesline_01")),
	                                            Village(TEXT("props/construction"), TEXT("SM_PROP_market_v02_01")), Village(TEXT("props/deco"), TEXT("SM_PROP_well")),
	                                            Village(TEXT("props/deco"), TEXT("SM_PROP_flag_05")), Village(TEXT("props/container"), TEXT("SM_PROP_barrel_05"))});
	UStaticMesh* ArbreVillage = Village(TEXT("environment"), TEXT("SM_ENV_TREE_village_LOD0"));

	// La foret stylisee (StyleHex) : des bouleaux et des feuillus, des touffes d'herbe et de fleurs, des pierres
	const TArray<UStaticMesh*> Bouleaux = Liste({ForetStylisee(TEXT("Trees"), TEXT("SM_Tree_Birch_1")), ForetStylisee(TEXT("Trees"), TEXT("SM_Tree_Birch_2"))});
	const TArray<UStaticMesh*> Feuillus = Liste({ForetStylisee(TEXT("Trees"), TEXT("SM_Tree_Broadleaf_1")), ForetStylisee(TEXT("Trees"), TEXT("SM_Tree_Broadleaf_2"))});
	const TArray<UStaticMesh*> Touffes = Liste({ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Grass_Clump_1")), ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Grass_Clump_2")),
	                                            ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Grass_Clump_3")), ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Grass_Clump_4")),
	                                            ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Grass_Clump_5"))});
	const TArray<UStaticMesh*> Bouquets = Liste({ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Flower_Clump_1")), ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Flower_Clump_2")),
	                                             ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Flower_Clump_3"))});
	const TArray<UStaticMesh*> Pierres = Liste({ForetStylisee(TEXT("Stones"), TEXT("SM_Stone_1")), ForetStylisee(TEXT("Stones"), TEXT("SM_Stone_2")),
	                                            ForetStylisee(TEXT("Stones"), TEXT("SM_Stone_3"))});
	const TArray<UStaticMesh*> Bloc = Liste({ForetStylisee(TEXT("Stones"), TEXT("SM_Stone_Boulder_1"))});
	const TArray<UStaticMesh*> BuissonsStylises = Liste({ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Bush_1")), ForetStylisee(TEXT("Foliage"), TEXT("SM_Fol_Bush_2"))});

	P.Buissons = Plus(Buissons, BuissonsStylises);
	P.Herbes = Plus(Herbes, Touffes);
	// Les sols (Pack_Bonus) : dalles d'herbe, de pierre, de carrelage ; passerelles de planches dans les marais
	static const TCHAR* SOLS[7] = {TEXT("M_Pack_Bonus_Grass_1"), TEXT("M_Pack_Bonus_Grass_3"), TEXT("M_Pack_Bonus_Grass_2"), TEXT("M_Pack_Bonus_Stone_1"),
	                               nullptr, TEXT("M_Pack_Bonus_Stone_3"), TEXT("M_Pack_Bonus_Tile_3")};
	static const TCHAR* PAVES[7] = {nullptr, nullptr, TEXT("M_Pack_Bonus_Wooden_Floor_1"), TEXT("M_Pack_Bonus_Stone_2"), nullptr, nullptr, TEXT("M_Pack_Bonus_Tile_1")};
	P.Sol = nullptr;		// (SOLS : essaye, mais une grande etendue de dalles se lit mal ; le sol reste uni)
	(void)SOLS;
	P.Pave = MateriauDeSol(PAVES[FMath::Clamp(Acte, 1, 7) - 1]);
	switch (Acte)
	{
		case 1:		// la Foret des Brumes : une vraie foret, des fleurs, un moulin au loin
			P.Arbres = Plus(Plus(Foret, Liste({ArbreVillage, ArbreVillage})), Plus(Plus(Bouleaux, Bouleaux), Feuillus));
			P.Rochers = Plus(RochersForet, Pierres);
			P.Falaises = Plus(GrandsRochers, Bloc);
			P.Fleurs = Plus(Fleurs, Plus(Bouquets, Bouquets));
			P.Debris = Plus(Bois, Ferme);
			P.DensiteDebris = 0.5f;
			P.Batiments = Liste({Provencal(TEXT("SM_WindMill_SM_WindMill")), Provencal(TEXT("SM_House_Barn"))});
			P.DensiteBatiments = 0.15f;
			P.Maisons = 5;				// un hameau, au bord de la foret
			P.Mares = 10;
			P.bSolPeint = true;
			P.Lumiere = Village(TEXT("props/light"), TEXT("SM_PROP_streetlamp_v01_01"));
			P.HauteurLumiere = 330.0f;
			break;
		case 2:		// le Bois des Pendus : la meme foret, plus sombre ; des gibets, des charrettes abandonnees, des tombes
			P.Arbres = Plus(Plus(Foret, Bouleaux), Feuillus);		// des bouleaux pales dans le bois hante
			P.Rochers = Plus(RochersForet, Pierres);
			P.Falaises = Plus(GrandsRochers, Bloc);
			P.Fleurs = Liste({Provencal(TEXT("SM_Flower_03_a")), Provencal(TEXT("SM_Flower_03_b"))});
			P.DensiteFleurs = 0.4f;
			P.Debris = Plus(Plus(Bois, Potences), Liste({Provencal(TEXT("SM_Cart")), Provencal(TEXT("SM_Fence_02")), Provencal(TEXT("SM_Barrel_Open"))}));
			P.DensiteDebris = 0.8f;
			P.Lumiere = Provencal(TEXT("SM_Shrine_Candle"));		// des bougies, rien de plus
			P.HauteurLumiere = 115.0f;
			P.CouleurLumiere = FLinearColor(1.0f, 0.75f, 0.45f);
			P.Tombes = Liste({Provencal(TEXT("SM_RockBrick_Small_01")), Provencal(TEXT("SM_RockBrick_Small_02")), Provencal(TEXT("SM_RockBrick_Small_03"))});
			P.bSolPeint = true;
			break;
		case 3:		// les Marais : des cypres, des roseaux, des mares, des passerelles de planches
			P.Arbres = Plus(Liste({Provencal(TEXT("SM_Tree_Cypress")), Provencal(TEXT("SM_Tree_Cypress"))}), Liste({Provencal(TEXT("SMF_Forest_Tree05")), Provencal(TEXT("SMF_Forest_Tree06"))}));
			P.DensiteArbres = 0.7f;
			P.Rochers = PetitsRochers;
			P.Fleurs = Liste({Provencal(TEXT("SM_Flower_02_a")), Provencal(TEXT("SM_Flower_02_b")), Provencal(TEXT("SM_Flower_02_c"))});
			P.DensiteFleurs = 0.5f;
			P.DensiteHerbes = 1.6f;
			P.Debris = Plus(Bois, Liste({Provencal(TEXT("SM_Barrel")), Village(TEXT("props/vehicles"), TEXT("SM_PROP_rowboat")),
			                             Village(TEXT("props/container"), TEXT("SM_PROP_bucket_02")), Village(TEXT("props/natural"), TEXT("SM_PROP_treetrunk_02"))}));
			P.DensiteDebris = 0.5f;
			P.Mares = 45;
			P.bSolPeint = true;
			P.Lumiere = Village(TEXT("props/light"), TEXT("SM_PROP_lantern_02"));
			P.HauteurLumiere = 90.0f;
			P.CouleurLumiere = FLinearColor(0.75f, 1.0f, 0.6f);		// une lueur de marais
			break;
		case 4:		// la forteresse d'Ashka : remparts, tours, maisons fortifiees, gravats, cypres
			P.Arbres = Liste({Provencal(TEXT("SM_Tree_Cypress")), Provencal(TEXT("SM_Tree_01")), Provencal(TEXT("SM_Tree_02")), Provencal(TEXT("SM_Tree_03"))});
			P.DensiteArbres = 0.45f;
			P.Rochers = Ruines;
			P.Falaises = Liste({Provencal(TEXT("SM_Rock_Wall_01")), Provencal(TEXT("SM_Rock_Wall_02")), Provencal(TEXT("SM_Rock_Cliff_01"))});
			P.DensiteFalaises = 0.5f;
			P.Fleurs = Liste({Provencal(TEXT("SM_Flower_01_a")), Provencal(TEXT("SM_Flower_01_b"))});
			P.DensiteFleurs = 0.3f;
			P.DensiteHerbes = 0.6f;
			P.Debris = Plus(Plus(Objets, Garnison), Plus(Ruines, Liste({Provencal(TEXT("SM_Wooden_Pillar")), Provencal(TEXT("SM_Fence_01"))})));
			P.DensiteDebris = 1.0f;
			P.Lumiere = Village(TEXT("props/light"), TEXT("SM_PROP_brazier_01"));
			P.HauteurLumiere = 140.0f;
			P.bFlammes = true;
			P.Batiments = Plus(Liste({Provencal(TEXT("SM_Castle_Wall")), Provencal(TEXT("SM_Castle_Wall")), Provencal(TEXT("SM_House_Tower"))}), Maisons);
			P.DensiteBatiments = 1.0f;
			P.bSolPeint = true;
			break;
		case 5:		// le col gele : des falaises partout, quelques arbres, des rochers
			P.Arbres = Liste({Provencal(TEXT("SMF_Forest_Tree02")), Provencal(TEXT("SMF_Forest_Tree04"))});
			P.DensiteArbres = 0.35f;
			P.Rochers = Plus(Plus(RochersForet, GrandsRochers), Plus(Pierres, Bloc));
			P.DensiteRochers = 1.6f;
			P.Falaises = Falaises;
			P.DensiteFalaises = 1.4f;
			P.DensiteHerbes = 0.3f;
			P.DensiteBuissons = 0.3f;
			P.Debris = Liste({Village(TEXT("props/natural"), TEXT("SM_PROP_treetrunk_04")), Village(TEXT("props/natural"), TEXT("SM_PROP_stone_04"))});
			P.DensiteDebris = 0.3f;
			P.Lumiere = Village(TEXT("props/light"), TEXT("SM_PROP_brazier_03"));
			P.HauteurLumiere = 140.0f;
			P.bFlammes = true;
			break;
		case 6:		// les Terres de Cendre : le Voile commence a deformer le monde (roches poreuses, arbres tordus)
			P.Arbres = Plus(ArbreTordu, ArbreTordu);
			P.DensiteArbres = 0.3f;
			P.HauteurArbres = FVector2D(450.0f, 700.0f);
			P.Rochers = RochersPoreux.Num() > 0 ? RochersPoreux : RochersForet;
			P.DensiteRochers = 1.6f;
			P.Falaises = Liste({Provencal(TEXT("SM_Rock_Cliff_03")), Provencal(TEXT("SM_Rock_Cliff_04"))});
			P.DensiteFalaises = 0.8f;
			P.Corruption = Tentacules;
			P.DensiteCorruption = 0.25f;
			P.DensiteHerbes = 0.15f;
			P.DensiteBuissons = 0.15f;
			P.Lumiere = Village(TEXT("props/light"), TEXT("SM_PROP_brazier_05"));
			P.HauteurLumiere = 140.0f;
			P.bFlammes = true;
			break;
		default:	// Karn, la cite voilee : une cite en ruine, envahie par la chair du Voile
			P.Arbres = Plus(Liste({Provencal(TEXT("SM_Tree_Cypress"))}), ArbreTordu);
			P.DensiteArbres = 0.3f;
			P.Rochers = Plus(Ruines, RochersPoreux);
			P.Falaises = Liste({Provencal(TEXT("SM_Castle_Wall")), Provencal(TEXT("SM_Rock_Wall_01"))});
			P.DensiteFalaises = 0.5f;
			P.Batiments = Maisons;
			P.DensiteBatiments = 1.0f;
			P.Corruption = Plus(Tentacules, Liste({Planete(TEXT("Cocoon_01"), TEXT("SM_Cocoon_01a")), Planete(TEXT("CreatureHead"), TEXT("SM_Creature_Eyeball_01_low"))}));
			P.DensiteCorruption = 1.0f;
			P.Debris = Plus(Ruines, Ruelles);
			P.DensiteDebris = 0.7f;
			P.Maisons = 18;				// la cite voilee
			P.Lumiere = Village(TEXT("props/light"), TEXT("SM_PROP_streetlamp_v02_01"));
			P.HauteurLumiere = 330.0f;
			P.CouleurLumiere = FLinearColor(0.7f, 0.45f, 1.0f);		// la lumiere du Voile
			P.DensiteHerbes = 0.4f;
			P.DensiteBuissons = 0.3f;
			break;
	}
	// Rien d'importe ? des formes simples, pour que le jeu tourne quand meme
	if (P.Arbres.Num() == 0) P.Arbres = Liste({LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"))});
	if (P.Rochers.Num() == 0) P.Rochers = Liste({LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"))});
	return P;
}

// ===================== La mise en place =====================

AVespMonde::AVespMonde()
{
	PrimaryActorTick.bCanEverTick = true;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> LePlan(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeCone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LaSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LeCylindre(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Plan = LePlan.Object;
	Cube = LeCube.Object;
	Cone = LeCone.Object;
	Sphere = LaSphere.Object;
	Cylindre = LeCylindre.Object;

	auto Creer = [this](const TCHAR* Nom, UStaticMesh* M, float Distance) {
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Nom);
		C->SetupAttachment(Racine);
		C->SetStaticMesh(M);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCastShadow(false);
		C->SetCullDistances(0, Distance);
		return C;
	};
	EnvTerre = Creer(TEXT("EnvTerre"), Plan, 0);
	EnvClairieres = Creer(TEXT("EnvClairieres"), Cylindre, 0);
	EnvChemin = Creer(TEXT("EnvChemin"), Cylindre, 12000);
	EnvTaches = Creer(TEXT("EnvTaches"), Cylindre, 12000);
	EnvChampignons = Creer(TEXT("EnvChampignons"), Sphere, 7000);
	EnvLueurs = Creer(TEXT("EnvLueurs"), Cone, 14000);
}

UMaterialInstanceDynamic* AVespMonde::Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Teinte);
	if (Composant)
	{
		Composant->SetMaterial(0, Materiau);
	}
	return Materiau;
}

UMaterialInstanceDynamic* AVespMonde::Lumineux(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte)
{
	UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Teinte);
	if (Composant)
	{
		Composant->SetMaterial(0, Materiau);
	}
	return Materiau;
}

UHierarchicalInstancedStaticMeshComponent* AVespMonde::Instances(UStaticMesh* M, bool bOmbre, float DistanceMax)
{
	if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Deja = Catalogue.Find(M))
	{
		return Deja->Get();
	}
	UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	C->SetupAttachment(Racine);
	C->SetStaticMesh(M);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(bOmbre);
	C->SetCullDistances(0, DistanceMax);
	C->RegisterComponent();
	Catalogue.Add(M, C);
	return C;
}

// Pose un modele, a la bonne taille (Hauteur en cm, sans depasser LargeurMax), tourne au hasard
void AVespMonde::Poser(UStaticMesh* M, const FVector& Pied, float Hauteur, float LargeurMax, bool bOmbre, float DistanceMax, float Inclinaison, float Yaw)
{
	if (!M)
	{
		return;
	}
	const float E = AVespGrille::EchelleSur(M, Hauteur, LargeurMax);
	const FRotator R(Hasard.FRandRange(-Inclinaison, Inclinaison), Yaw >= 0.0f ? Yaw : Hasard.FRandRange(0.0f, 360.0f), Hasard.FRandRange(-Inclinaison, Inclinaison));
	Instances(M, bOmbre, DistanceMax)->AddInstance(FTransform(R, Pied, FVector(E)), true);
}

// ===================== Ce qui est praticable =====================

static float DistanceAuSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
{
	const FVector2D AB = B - A;
	const float L2 = AB.SizeSquared();
	const float T = L2 > 0.0f ? FMath::Clamp(FVector2D::DotProduct(P - A, AB) / L2, 0.0f, 1.0f) : 0.0f;
	return FVector2D::Distance(P, A + AB * T);
}

float AVespMonde::DistanceAuxSentiers(const FVector& P) const
{
	const FVector2D Q(P.X, P.Y);
	float D = MAX_flt;
	for (const FVespCouloir& C : Couloirs)
	{
		const FVector& A = Zones[C.A].Centre;
		const FVector& B = Zones[C.B].Centre;
		D = FMath::Min(D, DistanceAuSegment(Q, FVector2D(A.X, A.Y), FVector2D(B.X, B.Y)));
	}
	return D;
}

float AVespMonde::DistanceAuPraticable(const FVector& P) const
{
	float D = DistanceAuxSentiers(P) - DemiLargeurSentier;
	for (const FVespZone& Z : Zones)
	{
		D = FMath::Min(D, FVector::Dist2D(P, Z.Centre) - Z.Rayon);
	}
	return D;
}

bool AVespMonde::EstPraticable(const FVector& P) const
{
	return DistanceAuPraticable(P) <= 0.0f;
}

FVector AVespMonde::Contraindre(const FVector& Depuis, const FVector& Vers) const
{
	if (EstPraticable(Vers))
	{
		return Vers;
	}
	// On glisse le long du bord : on essaie chaque axe separement
	const FVector SurX(Vers.X, Depuis.Y, Depuis.Z);
	if (EstPraticable(SurX))
	{
		return SurX;
	}
	const FVector SurY(Depuis.X, Vers.Y, Depuis.Z);
	if (EstPraticable(SurY))
	{
		return SurY;
	}
	return Depuis;
}

// ===================== La construction =====================

void AVespMonde::Vider()
{
	for (UInstancedStaticMeshComponent* C : {EnvTerre.Get(), EnvClairieres.Get(), EnvChemin.Get(), EnvTaches.Get(), EnvChampignons.Get(), EnvLueurs.Get()})
	{
		C->ClearInstances();
	}
	for (auto& Paire : Catalogue)
	{
		if (Paire.Value)
		{
			Paire.Value->DestroyComponent();
		}
	}
	Catalogue.Reset();
	auto Detruire = [](auto& Tableau) {
		for (auto& C : Tableau)
		{
			if (C)
			{
				C->DestroyComponent();
			}
		}
		Tableau.Reset();
	};
	Detruire(Orbes);
	Detruire(LumieresBalises);
	Detruire(Lettres);
	Detruire(Lumieres);
	Detruire(Feux);
	Detruire(Eaux);
	Detruire(Insectes);
	Detruire(Flammes);
	for (AActor* M : Maisons)
	{
		if (M)
		{
			M->Destroy();
		}
	}
	Maisons.Reset();
	CentresInsectes.Reset();
	IntensitesBalises.Reset();
}

// Un vrai feu (l'effet de flammes du Fantastic_Village_Pack)
void AVespMonde::AjouterFlamme(const FVector& Position, float Echelle)
{
	static TWeakObjectPtr<UParticleSystem> Feu;
	if (!Feu.IsValid())
	{
		Feu = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/Fantastic_Village_Pack/effects/PS_FX_fire_1.PS_FX_fire_1"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (Feu.IsValid())
	{
		if (UParticleSystemComponent* F = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Feu.Get(), Position, FRotator::ZeroRotator, FVector(Echelle), false))
		{
			Flammes.Add(F);
		}
	}
}

void AVespMonde::AjouterLumiere(const FVector& Position, const FLinearColor& Teinte, float Intensite, float Rayon)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetupAttachment(Racine);
	L->RegisterComponent();
	L->SetWorldLocation(Position);
	L->SetLightColor(Teinte);
	L->SetIntensity(Intensite);
	L->SetAttenuationRadius(Rayon);
	L->SetCastShadows(false);
	Lumieres.Add(L);
}

// La balise d'une clairiere : une orbe lumineuse qui flotte, sa lumiere, et sa lettre ; et ses accessoires
void AVespMonde::AjouterBalise(int32 Index)
{
	const FVespZone& Z = Zones[Index];
	const bool bBoss = Z.Type == BOSS;
	const FVector Haut = Z.Centre + FVector(0, 0, bBoss ? 420.0f : 300.0f);

	UStaticMeshComponent* Orbe = NewObject<UStaticMeshComponent>(this);
	Orbe->SetupAttachment(Racine);
	Orbe->SetStaticMesh(Sphere);
	Orbe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Orbe->SetCastShadow(false);
	Orbe->RegisterComponent();
	Orbe->SetWorldLocation(Haut);
	Orbe->SetWorldScale3D(FVector(bBoss ? 0.7f : 0.38f));
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	M->SetVectorParameterValue(TEXT("Color"), Z.Couleur * 3.5f);
	Orbe->SetMaterial(0, M);
	Orbes.Add(Orbe);

	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetupAttachment(Racine);
	L->RegisterComponent();
	L->SetWorldLocation(Haut);
	L->SetLightColor(Z.Couleur);
	const float Intensite = bBoss ? 9000.0f : 3500.0f;
	L->SetIntensity(Intensite);
	L->SetAttenuationRadius(bBoss ? 1800.0f : 1100.0f);
	L->SetCastShadows(false);
	LumieresBalises.Add(L);
	IntensitesBalises.Add(Intensite);

	UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
	T->SetupAttachment(Racine);
	T->RegisterComponent();
	T->SetWorldLocation(Haut + FVector(0, 0, bBoss ? 170.0f : 110.0f));
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetWorldSize(bBoss ? 150.0f : 90.0f);
	T->SetTextRenderColor(Z.Couleur.ToFColor(true));
	T->SetText(FText::FromString(Z.Lettre));
	Lettres.Add(T);

	// Les accessoires de la clairiere (hors du centre, ou une arene pourrait apparaitre)
	const FVector C = Z.Centre;
	switch (Z.Type)
	{
		case DEPART:		// l'autel de la prophetie, et ses bougies
			Poser(Provencal(TEXT("SM_Shrine")), C + FVector(380, 0, 0), 230.0f, 260.0f, true, 20000.0f, 0.0f, 180.0f);
			Poser(Provencal(TEXT("SM_Shrine_Candle")), C + FVector(380, 220, 0), 110.0f, 90.0f);
			Poser(Provencal(TEXT("SM_Shrine_Candle")), C + FVector(380, -220, 0), 110.0f, 90.0f);
			AjouterLumiere(C + FVector(380, 0, 180), FLinearColor(0.75f, 0.55f, 1.0f), 4000.0f, 900.0f);
			break;
		case REPOS:			// un feu de camp : des buches croisees, des bancs, et un feu qui vacille
		{
			if (UStaticMesh* Feu = Village(TEXT("props/light"), TEXT("SM_PROP_campfire")))
			{
				Poser(Feu, C, 70.0f, 170.0f, true, 20000.0f);
				Poser(Village(TEXT("props/deco"), TEXT("SM_PROP_firepit_woodpile")), C + FVector(-160, -240, 0), 90.0f, 160.0f);
			}
			else
			{
				UStaticMesh* Buche = Provencal(TEXT("SM_WoodenBeam_01"));
				for (int32 i = 0; i < 4; i++)
				{
					Poser(Buche, C + FVector(0, 0, 10), 30.0f, 170.0f, true, 20000.0f, 8.0f, i * 45.0f);
				}
			}
			AjouterFlamme(C + FVector(0, 0, 20), 1.0f);
			Poser(Provencal(TEXT("SM_Bench")), C + FVector(260, 0, 0), 60.0f, 220.0f, true, 20000.0f, 0.0f, 90.0f);
			Poser(Provencal(TEXT("SM_Stool")), C + FVector(-180, 200, 0), 50.0f, 80.0f);
			UPointLightComponent* Feu = NewObject<UPointLightComponent>(this);
			Feu->SetupAttachment(Racine);
			Feu->RegisterComponent();
			Feu->SetWorldLocation(C + FVector(0, 0, 60));
			Feu->SetLightColor(FLinearColor(1.0f, 0.55f, 0.2f));
			Feu->SetIntensity(6000.0f);
			Feu->SetAttenuationRadius(900.0f);
			Feu->SetCastShadows(false);
			Feux.Add(Feu);
			break;
		}
		case MARCHAND:		// la charrette du marchand, ses tonneaux, ses caisses, ses jarres
			Poser(Village(TEXT("props/construction"), TEXT("SM_PROP_market_v01_01")), C + FVector(420, 0, 0), 300.0f, 420.0f, true, 20000.0f, 0.0f, 180.0f);
			Poser(Village(TEXT("props/furniture"), TEXT("SM_PROP_market_shelf_01")), C + FVector(420, -330, 0), 190.0f, 200.0f, true, 20000.0f, 0.0f, 180.0f);
			Poser(Provencal(TEXT("SM_Cart")), C + FVector(300, 420, 0), 180.0f, 380.0f, true, 20000.0f, 0.0f, 60.0f);
			Poser(Provencal(TEXT("SM_Barrel")), C + FVector(330, 260, 0), 100.0f, 80.0f);
			Poser(Provencal(TEXT("SM_Barrel_Open")), C + FVector(420, 300, 0), 95.0f, 80.0f);
			Poser(Provencal(TEXT("SM_Crate_02")), C + FVector(300, -260, 0), 80.0f, 90.0f);
			Poser(Provencal(TEXT("SM_Jar_01")), C + FVector(460, -240, 0), 70.0f, 50.0f);
			Poser(Provencal(TEXT("SM_Flower_Pot")), C + FVector(250, 330, 0), 60.0f, 60.0f);
			AjouterLumiere(C + FVector(350, 0, 220), FLinearColor(1.0f, 0.75f, 0.45f), 3000.0f, 800.0f);
			break;
		case TRESOR:		// un coffre (une caisse cerclee) qui luit
			if (UStaticMesh* Coffre = Village(TEXT("props/container"), TEXT("SM_PROP_treasurechest")))
			{
				Poser(Coffre, C, 80.0f, 140.0f, true, 20000.0f, 0.0f, 200.0f);
				Poser(Village(TEXT("props/container"), TEXT("SM_PROP_treasurechest_top")), C + FVector(0, 0, 0), 80.0f, 140.0f, true, 20000.0f, 0.0f, 200.0f);
			}
			else
			{
				Poser(Provencal(TEXT("SM_Crate_03")), C, 80.0f, 110.0f, true, 20000.0f, 0.0f, 20.0f);
			}
			AjouterLumiere(C + FVector(0, 0, 90), FLinearColor(1.0f, 0.85f, 0.4f), 2500.0f, 500.0f);
			break;
		case EVENEMENT:		// une arche de bois, comme un passage vers autre chose
			Poser(Provencal(TEXT("SM_WoodenArch")), C + FVector(350, 0, 0), 330.0f, 420.0f, true, 20000.0f, 0.0f, 90.0f);
			Poser(Village(TEXT("props/deco"), TEXT("SM_PROP_well")), C + FVector(420, 380, 0), 220.0f, 220.0f);
			break;
		case BOSS:			// un cercle de grands rochers autour de l'arene du boss
			for (int32 i = 0; i < 10; i++)
			{
				const float A = i * 2.0f * PI / 10.0f + 0.2f;
				const FVector P = C + FVector(FMath::Cos(A), FMath::Sin(A), 0) * (Z.Rayon + 180.0f);
				if (DistanceAuxSentiers(P) > DemiLargeurSentier + 150.0f)
				{
					Poser(Provencal(TEXT("SM_Rock_Large_01")), P, Hasard.FRandRange(260.0f, 420.0f), 420.0f, true, 20000.0f, 6.0f);
				}
			}
			break;
		default: break;
	}
}

void AVespMonde::Construire(int32 LActe, const TArray<FVespZone>& LesZones, const TArray<FVespCouloir>& LesCouloirs)
{
	if (!bPret)
	{
		bPret = true;
		CouleurTerre = Couleur(EnvTerre, FLinearColor::Black);
		CouleurClairieres = Couleur(EnvClairieres, FLinearColor::Black);
		CouleurChemin = Couleur(EnvChemin, FLinearColor::Black);
		CouleurTaches = Couleur(EnvTaches, FLinearColor::Black);
		CouleurChampignons = Lumineux(EnvChampignons, FLinearColor::Black);
		CouleurLueurs = Lumineux(EnvLueurs, FLinearColor::Black);
		SolPeint = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StylizedProvencal/Materials/MI_Top_planar.MI_Top_planar"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		EauPeinte = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StylizedProvencal/Materials/MI_Water_01.MI_Water_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	Vider();
	Acte = FMath::Clamp(LActe, 1, 7);
	Zones = LesZones;
	Couloirs = LesCouloirs;
	Hasard.Initialize(900 + Acte * 77);
	const FVespStyleActe& S = StyleDeLActe(Acte);
	const FVespPalette Pal = PaletteDeLActe(Acte);
	auto Teindre = [](UMaterialInstanceDynamic* M, const FLinearColor& C) { if (M) { M->SetVectorParameterValue(TEXT("Color"), C); } };
	Teindre(CouleurChampignons, S.Champignons);
	Teindre(CouleurTaches, S.Taches);
	Teindre(CouleurTerre, S.Terre);
	Teindre(CouleurChemin, S.Chemin);
	Teindre(CouleurClairieres, FMath::Lerp(S.Terre, S.Sol, 0.55f));
	// Le sol : l'herbe peinte du pack dans les actes "sains", une couleur unie ailleurs (neige, cendre, Voile)
	EnvTerre->SetMaterial(0, Pal.Sol ? Pal.Sol : (Pal.bSolPeint && SolPeint ? SolPeint.Get() : static_cast<UMaterialInterface*>(CouleurTerre)));
	EnvChemin->SetStaticMesh(Pal.Pave ? Plan.Get() : Cylindre.Get());
	EnvChemin->SetMaterial(0, Pal.Pave ? Pal.Pave : static_cast<UMaterialInterface*>(CouleurChemin));

	// Les limites du monde
	FBox2D Limites(ForceInit);
	for (const FVespZone& Z : Zones)
	{
		Limites += FVector2D(Z.Centre.X, Z.Centre.Y);
	}
	Limites = Limites.ExpandBy(3200.0f);
	const float Z0 = GetActorLocation().Z;
	auto F = [this](float A, float B) { return Hasard.FRandRange(A, B); };
	auto Point = [&]() { return FVector(F(Limites.Min.X, Limites.Max.X), F(Limites.Min.Y, Limites.Max.Y), Z0); };
	auto Pose = [Z0](const FVector& P, float Z = 0.0f) { return FVector(P.X, P.Y, Z0 + Z); };
	auto AuHasard = [this](const TArray<UStaticMesh*>& L) -> UStaticMesh* { return L.Num() > 0 ? L[Hasard.RandRange(0, L.Num() - 1)] : nullptr; };
	// "Du cote de la camera" (en bas de l'ecran) : la camera regarde vers +X, depuis -X. Un point juste sous le
	// praticable pourrait cacher AYLIS : on n'y met que des choses basses.
	auto CoteCamera = [&](const FVector& P) { return DistanceAuPraticable(P + FVector(650.0f, 0, 0)) < 0.0f; };
	// Dans une arene (le rectangle 12 x 8 cases au centre d'une clairiere) : rien qui gene
	auto DansUneArene = [&](const FVector& P) {
		for (const FVespZone& Z : Zones)
		{
			if (FMath::Abs(P.X - Z.Centre.X) < 470.0f && FMath::Abs(P.Y - Z.Centre.Y) < 670.0f)
			{
				return true;
			}
		}
		return false;
	};
	auto PointAuBord = [&](float DMin, float DMax) {
		for (int32 k = 0; k < 80; k++)
		{
			const FVector P = Point();
			const float D = DistanceAuPraticable(P);
			if (D > DMin && D < DMax)
			{
				return P;
			}
		}
		return Point();
	};

	// ----- Le sol : la terre, les clairieres, les sentiers, des taches -----
	const FVector2D Taille = Limites.GetSize();
	const FVector2D Milieu = Limites.GetCenter();
	if (Pal.Sol)
	{
		// En dalles de 10 m : le materiau se repete au lieu de s'etirer sur tout le monde
		for (float X = Limites.Min.X; X < Limites.Max.X; X += 1000.0f)
		{
			for (float Y = Limites.Min.Y; Y < Limites.Max.Y; Y += 1000.0f)
			{
				EnvTerre->AddInstance(FTransform(FRotator::ZeroRotator, Pose(FVector(X + 500.0f, Y + 500.0f, 0), -3.7f), FVector(10.0f, 10.0f, 1.0f)), true);
			}
		}
	}
	else
	{
		EnvTerre->AddInstance(FTransform(FRotator::ZeroRotator, Pose(FVector(Milieu.X, Milieu.Y, 0), -3.7f), FVector(Taille.X / 100.0f, Taille.Y / 100.0f, 1.0f)), true);
	}
	for (const FVespZone& Z : Zones)
	{
		const float E = Z.Rayon * 2.1f / 100.0f;
		EnvClairieres->AddInstance(FTransform(FRotator::ZeroRotator, Pose(Z.Centre, -3.2f), FVector(E, E, 0.01f)), true);
	}
	UStaticMesh* Bougie = Pal.Lumiere ? Pal.Lumiere : Provencal(TEXT("SM_Shrine_Candle"));
	const float HauteurBougie = Pal.Lumiere ? Pal.HauteurLumiere : 115.0f;
	for (const FVespCouloir& C : Couloirs)
	{
		const FVector A = Zones[C.A].Centre, B = Zones[C.B].Centre;
		const float Longueur = FVector::Dist2D(A, B);
		if (Pal.Pave)
		{
			// Un sentier pave : des dalles carrees alignees (pas des disques qui se chevauchent)
			const float Cote = DemiLargeurSentier * 1.7f;
			const float Yaw = (B - A).Rotation().Yaw;
			int32 k = 0;
			for (float D = 0.0f; D < Longueur; D += Cote, k++)
			{
				const FVector P = FMath::Lerp(A, B, D / Longueur);
				EnvChemin->AddInstance(FTransform(FRotator(0, Yaw, 0), Pose(P, -2.6f + (k % 2) * 0.15f), FVector(Cote / 100.0f, Cote / 100.0f, 1.0f)), true);
			}
		}
		for (float D = 0.0f; D < Longueur && !Pal.Pave; D += 70.0f)
		{
			const FVector P = FMath::Lerp(A, B, D / Longueur) + FVector(F(-25.0f, 25.0f), F(-25.0f, 25.0f), 0);
			const float E = DemiLargeurSentier * 2.0f / 100.0f * F(0.75f, 0.95f);
			EnvChemin->AddInstance(FTransform(FRotator(0, F(0.0f, 360.0f), 0), Pose(P, -2.6f), FVector(E, E * F(0.8f, 1.0f), 0.012f)), true);
		}
		// Des autels a bougies le long du sentier, de chaque cote en alternance (la prophetie montre la route)
		const FVector Sens = (B - A).GetSafeNormal2D();
		const FVector Cote(-Sens.Y, Sens.X, 0);
		int32 n = 0;
		for (float D = 700.0f; D < Longueur - 700.0f; D += 1100.0f, n++)
		{
			const FVector P = Pose(FMath::Lerp(A, B, D / Longueur) + Cote * (n % 2 == 0 ? 1.0f : -1.0f) * (DemiLargeurSentier + 70.0f));
			if (Bougie)
			{
				Poser(Bougie, P, HauteurBougie, FMath::Max(90.0f, HauteurBougie * 0.6f), true, 14000.0f, 0.0f, (Sens.Rotation().Yaw + (n % 2 == 0 ? 90.0f : -90.0f)));
			}
			AjouterLumiere(P + FVector(0, 0, HauteurBougie * 0.9f), Pal.CouleurLumiere, 1300.0f, 700.0f);
			if (Pal.bFlammes)
			{
				AjouterFlamme(P + FVector(0, 0, HauteurBougie * 0.85f), 0.5f);
			}
		}
	}
	for (int32 i = 0; i < 450; i++)
	{
		const float E = F(1.5f, 5.0f);
		EnvTaches->AddInstance(FTransform(FRotator(0, F(0.0f, 360.0f), 0), Pose(Point(), -3.4f), FVector(E, E * F(0.6f, 1.0f), 0.01f)), true);
	}

	// ----- La foret qui borde le couloir : dense au bord, puis de plus en plus clairsemee -----
	TArray<FVector> PiedsDArbres;
	for (int32 i = 0; i < FMath::RoundToInt(14000 * Pal.DensiteArbres); i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D < 160.0f || D > 2800.0f || Hasard.FRand() > (D < 1000.0f ? 0.6f : 0.2f))
		{
			continue;
		}
		const bool bBas = CoteCamera(P);
		if (bBas && D < 750.0f)
		{
			continue;		// devant la camera : pas d'arbre juste au bord
		}
		PiedsDArbres.Add(P);
		const float Hauteur = F(Pal.HauteurArbres.X, Pal.HauteurArbres.Y) * (bBas ? 0.75f : 1.0f);
		Poser(AuHasard(Pal.Arbres), P, Hauteur, Hauteur * 0.9f, true, 18000.0f, 3.0f);
	}
	// Les buissons, au bord du couloir
	for (int32 i = 0; i < FMath::RoundToInt(9000 * Pal.DensiteBuissons); i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D > 20.0f && D < 1500.0f && Hasard.FRand() < 0.4f)
		{
			Poser(AuHasard(Pal.Buissons), P, F(70.0f, 160.0f), 260.0f, true, 11000.0f);
		}
	}
	// Les rochers : au bord, et quelques cailloux dans les clairieres (hors des arenes)
	for (int32 i = 0; i < FMath::RoundToInt(3500 * Pal.DensiteRochers); i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D > 0.0f && D < 1800.0f && Hasard.FRand() < 0.2f)
		{
			const float H = CoteCamera(P) ? F(40.0f, 110.0f) : F(60.0f, 230.0f);
			Poser(AuHasard(Pal.Rochers), P, H, H * 2.2f, true, 12000.0f, 8.0f);
		}
		else if (D < -80.0f && DistanceAuxSentiers(P) > DemiLargeurSentier * 0.7f && !DansUneArene(P) && Hasard.FRand() < 0.025f)
		{
			Poser(AuHasard(Pal.Rochers), P, F(25.0f, 50.0f), 90.0f, true, 9000.0f, 8.0f);
		}
	}
	// Les falaises et les grands rochers : en retrait, ils ferment l'horizon
	for (int32 i = 0; i < FMath::RoundToInt(900 * Pal.DensiteFalaises); i++)
	{
		const FVector P = PointAuBord(500.0f, 3000.0f);
		if (!CoteCamera(P))
		{
			Poser(AuHasard(Pal.Falaises), P, F(350.0f, 900.0f), 1400.0f, true, 22000.0f, 4.0f);
		}
	}
	// Les herbes et les fleurs : au bord et dans les clairieres (pas au milieu du sentier ni dans les arenes)
	for (int32 i = 0; i < FMath::RoundToInt(9000 * Pal.DensiteHerbes); i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D > 1400.0f || DansUneArene(P) || DistanceAuxSentiers(P) < DemiLargeurSentier * 0.6f || (D > 500.0f && Hasard.FRand() < 0.6f))
		{
			continue;
		}
		Poser(AuHasard(Pal.Herbes), P, F(35.0f, 80.0f), 260.0f, false, 7000.0f);
		if (Pal.Fleurs.Num() > 0 && Hasard.FRand() < 0.18f * Pal.DensiteFleurs)
		{
			Poser(AuHasard(Pal.Fleurs), P + FVector(F(-60.0f, 60.0f), F(-60.0f, 60.0f), 0), F(30.0f, 60.0f), 90.0f, false, 7000.0f);
		}
	}
	// Les champignons luminescents (la magie de la prophetie) : en cercles au pied des arbres
	for (int32 i = 0; i < FMath::RoundToInt(160 * S.DensiteChampignons) && PiedsDArbres.Num() > 0; i++)
	{
		const FVector Pied = PiedsDArbres[Hasard.RandRange(0, PiedsDArbres.Num() - 1)];
		const int32 Nombre = Hasard.RandRange(3, 6);
		const float Depart = F(0.0f, 2.0f * PI);
		for (int32 k = 0; k < Nombre; k++)
		{
			const float A = Depart + k * 0.55f;
			const FVector P = Pied + FVector(FMath::Cos(A), FMath::Sin(A), 0) * F(90.0f, 140.0f);
			const float E = F(0.08f, 0.15f);
			EnvChampignons->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, E * 60.0f), FVector(E, E, E * 0.45f)), true);
		}
	}
	// Au bord du sentier : ce que les voyageurs (et les Haschen) ont laisse
	for (int32 i = 0; i < FMath::RoundToInt(260 * Pal.DensiteDebris) && Pal.Debris.Num() > 0; i++)
	{
		const FVector P = PointAuBord(40.0f, 700.0f);
		Poser(AuHasard(Pal.Debris), P, F(60.0f, 180.0f), 320.0f, true, 11000.0f, 6.0f);
	}
	// Les tombes du Bois des Pendus, par petits groupes
	for (int32 g = 0; g < (Pal.Tombes.Num() > 0 ? 35 : 0); g++)
	{
		const FVector Centre = PointAuBord(100.0f, 1400.0f);
		for (int32 k = 0; k < Hasard.RandRange(3, 6); k++)
		{
			const FVector P = Centre + FVector(F(-180.0f, 180.0f), F(-180.0f, 180.0f), 0);
			if (DistanceAuPraticable(P) > 60.0f)
			{
				Poser(AuHasard(Pal.Tombes), P, F(55.0f, 85.0f), 80.0f, true, 10000.0f, 8.0f);
			}
		}
	}
	// Les batiments : en retrait du couloir, jamais devant la camera (maisons, tours, remparts, moulins)
	for (int32 i = 0; i < FMath::RoundToInt(160 * Pal.DensiteBatiments) && Pal.Batiments.Num() > 0; i++)
	{
		const FVector P = PointAuBord(650.0f, 2600.0f);
		if (CoteCamera(P))
		{
			continue;
		}
		// Orientes vers le sentier le plus proche (plus ou moins)
		Poser(AuHasard(Pal.Batiments), P, F(700.0f, 1200.0f), 1500.0f, true, 24000.0f, 0.0f, F(0.0f, 4.0f) * 90.0f);
	}
	// La corruption du Voile : des tentacules, des cocons, des yeux qui sortent du sol
	for (int32 i = 0; i < FMath::RoundToInt(260 * Pal.DensiteCorruption) && Pal.Corruption.Num() > 0; i++)
	{
		const FVector P = PointAuBord(120.0f, 2200.0f);
		const float H = CoteCamera(P) ? F(80.0f, 160.0f) : F(180.0f, 600.0f);
		Poser(AuHasard(Pal.Corruption), P, H, H * 1.4f, true, 16000.0f, 12.0f);
	}
	// Les mares d'eau peinte
	UMaterialInterface* Eau = S.Special == 3 ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WaterMaterials/Materials/MIC_PondScum_Cheaper.MIC_PondScum_Cheaper"), nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	if (!Eau)
	{
		Eau = EauPeinte;
	}
	for (int32 i = 0; i < Pal.Mares && Eau; i++)
	{
		const FVector P = PointAuBord(300.0f, 2200.0f);
		UStaticMeshComponent* Mare = NewObject<UStaticMeshComponent>(this);
		Mare->SetupAttachment(Racine);
		Mare->SetStaticMesh(Cylindre);
		Mare->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mare->SetCastShadow(false);
		Mare->RegisterComponent();
		const float E = F(2.5f, 7.0f);
		Mare->SetWorldTransform(FTransform(FRotator(0, F(0.0f, 360.0f), 0), Pose(P, -2.2f), FVector(E, E * F(0.5f, 0.9f), 0.01f)));
		Mare->SetMaterial(0, Eau);
		Eaux.Add(Mare);
		for (int32 k = 0; k < 10; k++)		// des roseaux tout autour
		{
			const float A = F(0.0f, 2.0f * PI);
			Poser(AuHasard(Pal.Herbes), P + FVector(FMath::Cos(A) * E * 48.0f, FMath::Sin(A) * E * 40.0f, 0), F(50.0f, 110.0f), 200.0f, false, 7000.0f);
		}
	}

	// ----- Ce qui brille : les pics de glace, les coulees de lave, les cristaux du Voile -----
	switch (S.Special)
	{
		case 5:
			EnvLueurs->SetStaticMesh(Cone);
			CouleurLueurs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.4f, 0.65f, 1.0f) * 0.8f);
			for (int32 g = 0; g < 90; g++)
			{
				const FVector Centre = PointAuBord(200.0f, 2200.0f);
				for (int32 k = 0; k < Hasard.RandRange(3, 6); k++)
				{
					const FVector P = Centre + FVector(F(-80.0f, 80.0f), F(-80.0f, 80.0f), 0);
					const float Haut = CoteCamera(P) ? F(0.4f, 0.9f) : F(0.8f, 2.8f);
					EnvLueurs->AddInstance(FTransform(FRotator(F(-20.0f, 20.0f), F(0.0f, 360.0f), F(-20.0f, 20.0f)), Pose(P, Haut * 40.0f),
					                                  FVector(F(0.2f, 0.45f), F(0.2f, 0.45f), Haut)), true);
				}
			}
			break;
		case 6:
			EnvLueurs->SetStaticMesh(Cylindre);
			CouleurLueurs->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.3f, 0.03f) * 3.0f);
			for (int32 i = 0; i < 170; i++)
			{
				const FVector P = PointAuBord(250.0f, 2400.0f);
				const float E = F(1.2f, 4.5f);
				EnvLueurs->AddInstance(FTransform(FRotator(0, F(0.0f, 360.0f), 0), Pose(P, -2.0f), FVector(E, E * F(0.4f, 0.9f), 0.01f)), true);
			}
			break;
		case 7:
			EnvLueurs->SetStaticMesh(Cube);
			CouleurLueurs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.3f, 1.0f) * 2.0f);
			for (int32 g = 0; g < 110; g++)
			{
				const FVector Centre = PointAuBord(150.0f, 2200.0f);
				for (int32 k = 0; k < Hasard.RandRange(3, 7); k++)
				{
					const FVector P = Centre + FVector(F(-80.0f, 80.0f), F(-80.0f, 80.0f), 0);
					const float Haut = CoteCamera(P) ? F(0.4f, 0.8f) : F(0.8f, 2.6f);
					EnvLueurs->AddInstance(FTransform(FRotator(F(-25.0f, 25.0f), F(0.0f, 360.0f), F(-25.0f, 25.0f)), Pose(P, Haut * 45.0f),
					                                  FVector(F(0.15f, 0.32f), F(0.15f, 0.32f), Haut)), true);
				}
			}
			break;
		default: break;
	}

	// ----- Les insectes du Voile (acte VII) : ils tournent au-dessus de la cite -----
	if (Acte == 7)
	{
		USkeletalMesh* Insecte = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Planet385CY/Characters/InsectFlying01/SK_Insect_01.SK_Insect_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UAnimSequence* Vol = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Planet385CY/Characters/InsectFlying01/Animations/AS_Insect_01_Rigged_Flap_Fast01.AS_Insect_01_Rigged_Flap_Fast01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		for (int32 i = 0; i < 10 && Insecte; i++)
		{
			USkeletalMeshComponent* I = NewObject<USkeletalMeshComponent>(this);
			I->SetupAttachment(Racine);
			I->SetSkeletalMesh(Insecte);
			I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			I->RegisterComponent();
			const float Hauteur = Insecte->GetBounds().GetBox().GetSize().GetMax();
			I->SetWorldScale3D(FVector(Hauteur > 1.0f ? 160.0f / Hauteur : 1.0f));
			if (Vol)
			{
				I->PlayAnimation(Vol, true);
				I->SetPlayRate(F(0.8f, 1.2f));
			}
			Insectes.Add(I);
			CentresInsectes.Add(PointAuBord(0.0f, 1500.0f) + FVector(0, 0, F(350.0f, 700.0f)));
		}
	}

	// ----- Les panneaux indicateurs, aux embranchements -----
	UStaticMesh* Panneau = Village(TEXT("props/deco"), TEXT("SM_PROP_signpost_01"));
	for (int32 i = 0; i < Zones.Num() && Panneau; i++)
	{
		int32 Sorties = 0;
		FVector Somme = FVector::ZeroVector;
		for (const FVespCouloir& C : Couloirs)
		{
			if (C.A == i)
			{
				Sorties++;
				Somme += (Zones[C.B].Centre - Zones[i].Centre).GetSafeNormal2D();
			}
		}
		if (Sorties >= 2)
		{
			const FVector Sens = Somme.GetSafeNormal2D();
			Poser(Panneau, Pose(Zones[i].Centre + Sens * (Zones[i].Rayon - 160.0f) + FVector(-Sens.Y, Sens.X, 0) * 120.0f), 240.0f, 200.0f, true, 16000.0f, 0.0f,
			      Sens.Rotation().Yaw);
		}
	}

	// ----- Les maisons du village (des blueprints du pack), en retrait du couloir -----
	for (int32 i = 0; i < Pal.Maisons; i++)
	{
		const FString Nom = FString::Printf(TEXT("BP_BLD_house_%d"), 1 + Hasard.RandRange(0, 13));
		UClass* Classe = LoadClass<AActor>(nullptr, *FString::Printf(TEXT("/Game/Fantastic_Village_Pack/blueprints/buildings/%s.%s_C"), *Nom, *Nom),
		                                   nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Classe)
		{
			continue;
		}
		FVector P = PointAuBord(900.0f, 2400.0f);
		for (int32 k = 0; k < 10 && CoteCamera(P); k++)
		{
			P = PointAuBord(900.0f, 2400.0f);
		}
		if (CoteCamera(P))
		{
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AActor* Maison = GetWorld()->SpawnActor<AActor>(Classe, FTransform(FRotator(0, Hasard.RandRange(0, 3) * 90.0f + F(-10.0f, 10.0f), 0), Pose(P)), Params))
		{
			Maisons.Add(Maison);
		}
	}

	// ----- Les balises des clairieres -----
	for (int32 i = 0; i < Zones.Num(); i++)
	{
		AjouterBalise(i);
	}
}

void AVespMonde::MarquerZoneFaite(int32 Zone)
{
	if (!Zones.IsValidIndex(Zone) || !Orbes.IsValidIndex(Zone))
	{
		return;
	}
	if (Zones[Zone].Type == MARCHAND)
	{
		return;		// le marchand reste la
	}
	Orbes[Zone]->SetVisibility(false);
	Lettres[Zone]->SetVisibility(false);
	LumieresBalises[Zone]->SetIntensity(IntensitesBalises[Zone] * 0.2f);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Orbes[Zone]->GetComponentLocation(), FVector::UpVector, Zones[Zone].Couleur);
}

void AVespMonde::CacherBalise(int32 Zone, bool bCachee)
{
	if (Orbes.IsValidIndex(Zone) && Lettres.IsValidIndex(Zone))
	{
		Orbes[Zone]->SetVisibility(!bCachee);
		Lettres[Zone]->SetVisibility(!bCachee);
	}
}

void AVespMonde::OrienterTextes(const FVector& Camera)
{
	for (UTextRenderComponent* T : Lettres)
	{
		if (T)
		{
			T->SetWorldRotation((Camera - T->GetComponentLocation()).Rotation());
		}
	}
}

void AVespMonde::Tick(float Secondes)
{
	Super::Tick(Secondes);
	Temps += Secondes;
	// Les orbes flottent doucement, les feux de camp vacillent, les insectes tournent
	for (int32 i = 0; i < Orbes.Num(); i++)
	{
		if (Orbes[i] && Zones.IsValidIndex(i))
		{
			const float Haut = (Zones[i].Type == BOSS ? 420.0f : 300.0f) + FMath::Sin(Temps * 1.6f + i) * 22.0f;
			Orbes[i]->SetWorldLocation(Zones[i].Centre + FVector(0, 0, Haut));
		}
	}
	for (int32 i = 0; i < Feux.Num(); i++)
	{
		if (Feux[i])
		{
			Feux[i]->SetIntensity(5000.0f + 1800.0f * FMath::Sin(Temps * 13.0f + i) * FMath::Sin(Temps * 7.3f + i * 2.0f));
		}
	}
	for (int32 i = 0; i < Insectes.Num(); i++)
	{
		if (Insectes[i] && CentresInsectes.IsValidIndex(i))
		{
			const float A = Temps * (0.35f + 0.05f * i) + i * 1.7f;
			const FVector P = CentresInsectes[i] + FVector(FMath::Cos(A) * 600.0f, FMath::Sin(A) * 600.0f, FMath::Sin(A * 2.0f) * 80.0f);
			Insectes[i]->SetWorldLocation(P);
			Insectes[i]->SetWorldRotation(FRotator(0, FMath::RadiansToDegrees(A) + 90.0f, 0));
		}
	}
}
