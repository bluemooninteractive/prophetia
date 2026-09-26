#include "VespGrille.h"
#include "VespUnite.h"
#include "VespEffet.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"

// Les cartes dessinees a la main : '#' = rocher, 'T' = arbre, '.' = sol libre
// (AYLIS commence a gauche, en colonne 1, ligne 3 : cette case reste toujours libre)
static const TCHAR* CARTES[10][AVespGrille::Lignes] = {
	{	// la clairiere
		TEXT("..T....T...."),
		TEXT(".....#......"),
		TEXT("..#.....T..."),
		TEXT("............"),
		TEXT("....T...#..."),
		TEXT(".......#...."),
		TEXT("..#......T.."),
		TEXT("T.....T....."),
	},
	{	// le sentier entre les rochers
		TEXT("T...#...T..T"),
		TEXT("....#......."),
		TEXT(".T......#..."),
		TEXT("......T....."),
		TEXT("...#......T."),
		TEXT("T.....#....."),
		TEXT("...T....#..."),
		TEXT("........T..T"),
	},
	{	// le gue des brumes
		TEXT("....T.....T."),
		TEXT("T.#......#.."),
		TEXT("....#..T...."),
		TEXT("............"),
		TEXT("..T...#....."),
		TEXT("......#..T.."),
		TEXT(".#.......#.."),
		TEXT("...T..T....T"),
	},
	{	// les ronces
		TEXT("T..TT...T..T"),
		TEXT("...#....#..."),
		TEXT(".T....T....."),
		TEXT("......T...T."),
		TEXT("..##......#."),
		TEXT("T.....T....."),
		TEXT("..T.#....T.."),
		TEXT("T.....TT...T"),
	},
	{	// le cercle des anciens (la ou Skarn attend)
		TEXT("T.T......T.T"),
		TEXT("...#....#..."),
		TEXT("............"),
		TEXT("..T.......T."),
		TEXT("............"),
		TEXT("...#....#..."),
		TEXT("T..........T"),
		TEXT(".T.T....T.T."),
	},
	// ----- Acte II : le Bois des Pendus (des arbres morts, serres, et des tombes) -----
	{	// l'entree du bois
		TEXT("TT..T...T.TT"),
		TEXT("T....#.....T"),
		TEXT("..T.....T..."),
		TEXT("......T....."),
		TEXT("..T#.....T.."),
		TEXT("T......T...."),
		TEXT("...T.#....T."),
		TEXT("TT....T..TTT"),
	},
	{	// le cimetiere des pendus
		TEXT("T.#.T.#.T.#T"),
		TEXT("..........T."),
		TEXT("T..#..T..#.."),
		TEXT("......#....."),
		TEXT("..T......T.."),
		TEXT("T..#.T...#.."),
		TEXT("..........#."),
		TEXT("T.T.#..T.T.T"),
	},
	{	// le gibet
		TEXT("TT.T..#..TTT"),
		TEXT("T.....#....T"),
		TEXT("..T......T.."),
		TEXT("....T......."),
		TEXT("..#...T..#.."),
		TEXT("T.....T....."),
		TEXT("..T.#....T.T"),
		TEXT("TT....T...TT"),
	},
	{	// la fosse
		TEXT("T.T.TT.T.T.T"),
		TEXT("............"),
		TEXT("..#.T..T.#.."),
		TEXT("...T....T..."),
		TEXT("..#......#.."),
		TEXT("....T..T...."),
		TEXT("T.........T."),
		TEXT("TT.T.TT.T.TT"),
	},
	{	// la clairiere de la Matriarche
		TEXT("TT.T....T.TT"),
		TEXT("T..........T"),
		TEXT("....#..#...."),
		TEXT("..T........."),
		TEXT("..........T."),
		TEXT("....#..#...."),
		TEXT("T..........T"),
		TEXT("TT.T....T.TT"),
	},
};

// ===================== Le style de chaque acte =====================
// Les couleurs du monde, la densite de la foret, et son "decor special" :
//   3 = des mares d'eau noire, 4 = des colonnes et des blocs tombes, 5 = des pics de glace et des congeres,
//   6 = des coulees de lave et de l'obsidienne, 7 = des cristaux du Voile et des colonnes brisees.
struct FVespStyleActe
{
	FLinearColor Terre, Sol, Herbes, Buissons, Taches, Chemin;
	FLinearColor Fleurs, Champignons, Poison;		// ce qui brille
	int32 NombreArbres;
	bool bArbresMorts;
	int32 NombreHerbes;
	float PartFleurs;
	int32 NombreBuissons;
	int32 NombreChampignons;
	int32 Special;
	bool bTombes;
};

static const FVespStyleActe STYLES[7] = {
	// I. La Foret des Brumes
	{FLinearColor(0.02f, 0.05f, 0.035f), FLinearColor(0.10f, 0.16f, 0.12f), FLinearColor(0.06f, 0.2f, 0.09f), FLinearColor(0.03f, 0.09f, 0.05f),
	 FLinearColor(0.035f, 0.075f, 0.05f), FLinearColor(0.13f, 0.1f, 0.07f), FLinearColor(0.7f, 0.55f, 1.0f) * 3.0f, FLinearColor(0.3f, 0.9f, 1.0f) * 4.0f,
	 FLinearColor(0.4f, 1.0f, 0.2f) * 1.2f, 300, false, 1100, 0.14f, 190, 45, 0, false},
	// II. Le Bois des Pendus
	{FLinearColor(0.03f, 0.035f, 0.022f), FLinearColor(0.12f, 0.13f, 0.09f), FLinearColor(0.12f, 0.13f, 0.06f), FLinearColor(0.07f, 0.06f, 0.04f),
	 FLinearColor(0.06f, 0.06f, 0.035f), FLinearColor(0.12f, 0.1f, 0.07f), FLinearColor(0.9f, 0.9f, 0.6f) * 3.0f, FLinearColor(0.5f, 1.0f, 0.3f) * 4.0f,
	 FLinearColor(0.4f, 1.0f, 0.2f) * 1.2f, 260, true, 900, 0.06f, 160, 45, 0, true},
	// III. Les Marais de Sombreval
	{FLinearColor(0.015f, 0.035f, 0.03f), FLinearColor(0.07f, 0.11f, 0.09f), FLinearColor(0.05f, 0.15f, 0.08f), FLinearColor(0.03f, 0.08f, 0.05f),
	 FLinearColor(0.02f, 0.05f, 0.045f), FLinearColor(0.08f, 0.07f, 0.05f), FLinearColor(0.6f, 1.0f, 0.8f) * 2.5f, FLinearColor(0.6f, 1.0f, 0.4f) * 4.0f,
	 FLinearColor(0.35f, 1.0f, 0.25f) * 1.4f, 160, true, 1400, 0.05f, 120, 60, 3, false},
	// IV. La forteresse d'Ashka
	{FLinearColor(0.045f, 0.04f, 0.035f), FLinearColor(0.17f, 0.16f, 0.14f), FLinearColor(0.1f, 0.14f, 0.06f), FLinearColor(0.05f, 0.08f, 0.04f),
	 FLinearColor(0.07f, 0.065f, 0.055f), FLinearColor(0.2f, 0.18f, 0.15f), FLinearColor(1.0f, 0.8f, 0.5f) * 2.0f, FLinearColor(1.0f, 0.7f, 0.3f) * 3.0f,
	 FLinearColor(1.0f, 0.3f, 0.1f) * 1.5f, 110, false, 700, 0.05f, 90, 20, 4, false},
	// V. Le col gele
	{FLinearColor(0.3f, 0.33f, 0.4f), FLinearColor(0.42f, 0.47f, 0.55f), FLinearColor(0.3f, 0.36f, 0.33f), FLinearColor(0.25f, 0.3f, 0.3f),
	 FLinearColor(0.38f, 0.42f, 0.5f), FLinearColor(0.2f, 0.2f, 0.24f), FLinearColor(0.6f, 0.8f, 1.0f) * 2.0f, FLinearColor(0.5f, 0.8f, 1.0f) * 3.0f,
	 FLinearColor(0.5f, 0.8f, 1.0f) * 1.5f, 230, false, 400, 0.03f, 80, 20, 5, false},
	// VI. Les Terres de Cendre
	{FLinearColor(0.025f, 0.015f, 0.015f), FLinearColor(0.11f, 0.07f, 0.06f), FLinearColor(0.1f, 0.04f, 0.03f), FLinearColor(0.06f, 0.03f, 0.02f),
	 FLinearColor(0.05f, 0.02f, 0.015f), FLinearColor(0.05f, 0.035f, 0.03f), FLinearColor(1.0f, 0.5f, 0.2f) * 2.0f, FLinearColor(1.0f, 0.4f, 0.1f) * 3.0f,
	 FLinearColor(1.0f, 0.35f, 0.05f) * 3.0f, 70, true, 250, 0.03f, 40, 15, 6, false},
	// VII. Karn, la cite voilee
	{FLinearColor(0.025f, 0.015f, 0.04f), FLinearColor(0.12f, 0.1f, 0.16f), FLinearColor(0.1f, 0.06f, 0.16f), FLinearColor(0.06f, 0.03f, 0.08f),
	 FLinearColor(0.05f, 0.03f, 0.08f), FLinearColor(0.14f, 0.12f, 0.18f), FLinearColor(0.8f, 0.5f, 1.0f) * 3.0f, FLinearColor(0.6f, 0.3f, 1.0f) * 4.0f,
	 FLinearColor(0.7f, 0.25f, 1.0f) * 2.0f, 90, true, 600, 0.12f, 70, 50, 7, false},
};

static const FVespStyleActe& Style(int32 Acte)
{
	return STYLES[FMath::Clamp(Acte, 1, 7) - 1];
}

AVespGrille::AVespGrille()
{
	PrimaryActorTick.bCanEverTick = false;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;

	// Les modeles de base d'Unreal (remplaces par les vrais decors quand ils sont importes)
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

	auto Creer = [this](const TCHAR* Nom, UStaticMesh* Modele, bool bOmbre = true) {
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Nom);
		C->SetupAttachment(Racine);
		C->SetStaticMesh(Modele);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);	// modifiable pendant la partie (pour y mettre les decors importes)
		C->SetCastShadow(bOmbre);
		return C;
	};
	Sol = Creer(TEXT("Sol"), Plan);
	Rochers = Creer(TEXT("Rochers"), Cube);
	Arbres = Creer(TEXT("Arbres"), Cone);
	Accessibles = Creer(TEXT("Accessibles"), Plan, false);
	Survol = Creer(TEXT("Survol"), Plan, false);
	Danger = Creer(TEXT("Danger"), Plan, false);
	TerrBoue = Creer(TEXT("TerrBoue"), Plan, false);
	TerrPoison = Creer(TEXT("TerrPoison"), Plan, false);
	TerrPieges = Creer(TEXT("TerrPieges"), Plan, false);
	TerrGlace = Creer(TEXT("TerrGlace"), Plan, false);
	TerrFailles = Creer(TEXT("TerrFailles"), Cylindre, false);

	EnvTerre = Creer(TEXT("EnvTerre"), Plan, false);
	EnvArbres = Creer(TEXT("EnvArbres"), Cone);
	EnvRochers = Creer(TEXT("EnvRochers"), Cube);
	EnvBuissons = Creer(TEXT("EnvBuissons"), Sphere);
	EnvHerbes = Creer(TEXT("EnvHerbes"), Cone, false);		// les petites choses n'ont pas besoin d'ombre
	EnvFleurs = Creer(TEXT("EnvFleurs"), Sphere, false);
	EnvChampignons = Creer(TEXT("EnvChampignons"), Sphere, false);
	EnvTroncs = Creer(TEXT("EnvTroncs"), Cylindre);
	EnvTombes = Creer(TEXT("EnvTombes"), Cube);
	EnvTaches = Creer(TEXT("EnvTaches"), Cylindre, false);
	EnvChemin = Creer(TEXT("EnvChemin"), Cylindre, false);
	EnvSpecial = Creer(TEXT("EnvSpecial"), Cylindre);
	EnvBlocs = Creer(TEXT("EnvBlocs"), Cube);
}

UMaterialInstanceDynamic* AVespGrille::Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte)
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

// Un materiau qui brille (fleurs, champignons, lave, cristaux, failles)
UMaterialInstanceDynamic* AVespGrille::Lumineux(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte)
{
	UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Teinte);
	if (Composant)
	{
		Composant->SetMaterial(0, Materiau);
	}
	return Materiau;
}

// Un decor importe dans /Game/Decor : le premier dont le nom contient un des mots demandes
UStaticMesh* AVespGrille::ModeleDuDecor(std::initializer_list<const TCHAR*> Noms) const
{
	IAssetRegistry& Registre = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registre.GetAssetsByPath(FName(TEXT("/Game/Decor")), Assets, true);
	for (const TCHAR* Nom : Noms)
	{
		for (const FAssetData& A : Assets)
		{
			if (A.IsInstanceOf(UStaticMesh::StaticClass()) && A.AssetName.ToString().Contains(Nom))
			{
				return Cast<UStaticMesh>(A.GetAsset());
			}
		}
	}
	return nullptr;
}

// L'echelle pour qu'un modele fasse "Hauteur" cm de haut, sans depasser "LargeurMax" cm de large
float AVespGrille::EchelleSur(UStaticMesh* Modele, float Hauteur, float LargeurMax) const
{
	const FVector Taille = Modele->GetBounds().BoxExtent * 2.0f;
	float E = Taille.Z > 1.0f ? Hauteur / Taille.Z : 1.0f;
	const float Largeur = FMath::Max(Taille.X, Taille.Y);
	if (Largeur * E > LargeurMax)
	{
		E = LargeurMax / Largeur;
	}
	return E;
}

void AVespGrille::BeginPlay()
{
	Super::BeginPlay();
	// Les vrais decors (s'ils sont importes), sinon des formes simples et colorees
	Sapin = ModeleDuDecor({TEXT("tree_pine"), TEXT("tree")});
	ArbreMort = ModeleDuDecor({TEXT("tree_dead")});
	if (!ArbreMort)
	{
		ArbreMort = Sapin;		// pas encore d'arbres morts importes : les sapins feront l'affaire
	}
	bVraisArbres = Sapin != nullptr;
	if (!bVraisArbres)
	{
		CouleurArbres = Couleur(Arbres, FLinearColor(0.08f, 0.30f, 0.26f));
		EnvArbres->SetMaterial(0, CouleurArbres);
	}
	if (UStaticMesh* Rocher = ModeleDuDecor({TEXT("rock"), TEXT("stone")}))
	{
		Rochers->SetStaticMesh(Rocher);
		EnvRochers->SetStaticMesh(Rocher);
		bVraisRochers = true;
	}
	else
	{
		UMaterialInstanceDynamic* Gris = Couleur(Rochers, FLinearColor(0.25f, 0.24f, 0.30f));
		EnvRochers->SetMaterial(0, Gris);
	}
	auto Modele = [this](UInstancedStaticMeshComponent* C, std::initializer_list<const TCHAR*> Noms) {
		if (UStaticMesh* M = ModeleDuDecor(Noms))
		{
			C->SetStaticMesh(M);
			return true;
		}
		return false;
	};
	bVraisBuissons = Modele(EnvBuissons, {TEXT("bush")});
	bVraiesHerbes = Modele(EnvHerbes, {TEXT("grass")});
	bVraiesFleurs = Modele(EnvFleurs, {TEXT("flower")});
	bVraisChampignons = Modele(EnvChampignons, {TEXT("mushroom")});
	bVraisTroncs = Modele(EnvTroncs, {TEXT("log"), TEXT("stump"), TEXT("trunk")});
	bVraiesTombes = Modele(EnvTombes, {TEXT("grave"), TEXT("tomb")});
	if (!bVraisBuissons) CouleurBuissons = Couleur(EnvBuissons, FLinearColor::Black);
	if (!bVraiesHerbes) CouleurHerbes = Couleur(EnvHerbes, FLinearColor::Black);
	if (!bVraiesFleurs) CouleurFleurs = Lumineux(EnvFleurs, FLinearColor::Black);
	if (!bVraisChampignons) CouleurChampignons = Lumineux(EnvChampignons, FLinearColor::Black);
	if (!bVraisTroncs) Couleur(EnvTroncs, FLinearColor(0.12f, 0.07f, 0.04f));
	if (!bVraiesTombes) Couleur(EnvTombes, FLinearColor(0.22f, 0.22f, 0.25f));
	CouleurTerre = Couleur(EnvTerre, FLinearColor::Black);
	CouleurTaches = Couleur(EnvTaches, FLinearColor::Black);
	CouleurChemin = Couleur(EnvChemin, FLinearColor::Black);
	CouleurSol = Couleur(Sol, FLinearColor(0.10f, 0.16f, 0.12f));
	CouleurBlocs = Couleur(EnvBlocs, FLinearColor::Black);
	SpecialMat = Couleur(nullptr, FLinearColor::Black);
	SpecialLumineux = Lumineux(nullptr, FLinearColor::Black);
	Couleur(Accessibles, FLinearColor(0.35f, 0.75f, 1.0f));
	Couleur(Survol, FLinearColor(1.0f, 1.0f, 1.0f));
	Couleur(Danger, FLinearColor(1.0f, 0.12f, 0.08f));
	Couleur(TerrBoue, FLinearColor(0.09f, 0.06f, 0.03f));
	Couleur(TerrGlace, FLinearColor(0.55f, 0.72f, 0.9f));
	CouleurPoison = Lumineux(TerrPoison, FLinearColor::Black);
	Lumineux(TerrFailles, FLinearColor(0.7f, 0.3f, 1.0f) * 3.0f);
	PiegeBaisse = Couleur(TerrPieges, FLinearColor(0.2f, 0.18f, 0.17f));
	PiegeLeve = Lumineux(nullptr, FLinearColor(1.0f, 0.18f, 0.08f) * 2.5f);
	ChangerCarte(0);
}

// ===================== Les cartes =====================

void AVespGrille::RemplirDepuis(int32 Numero)
{
	Numero = FMath::Clamp(Numero, 0, 9);
	Carte.SetNum(Colonnes * Lignes);
	for (int32 L = 0; L < Lignes; L++)
	{
		for (int32 C = 0; C < Colonnes; C++)
		{
			Carte[Index(FIntPoint(C, L))] = CARTES[Numero][L][C];
		}
	}
}

void AVespGrille::ChangerCarte(int32 Numero)
{
	Acte = Numero >= 5 ? 2 : 1;
	RemplirDepuis(Numero);
	bPiegesLeves = false;
	ConstruireCarte();
	ConstruireEnvironnement();
	AfficherDanger({});
}

void AVespGrille::PreparerCarte(int32 LActe, bool bBoss)
{
	Acte = FMath::Clamp(LActe, 1, 7);
	bool bFaite = false;
	// Les actes I et II ont leurs cartes dessinees a la main... et des cartes inventees, une fois sur deux
	if (Acte <= 2 && FMath::RandBool())
	{
		const int32 Base = Acte == 1 ? 0 : 5;
		RemplirDepuis(Base + (bBoss ? 4 : FMath::RandRange(0, 3)));
		bFaite = true;
	}
	if (!bFaite && !GenererCarte(Acte, bBoss))
	{
		RemplirDepuis(bBoss ? 4 : 0);
	}
	bPiegesLeves = false;
	ConstruireCarte();
	ConstruireEnvironnement();
	AfficherDanger({});
}

// Une carte inventee : des obstacles, puis les terrains de l'acte (en taches), et on verifie
// qu'on peut aller partout (sinon on recommence)
bool AVespGrille::GenererCarte(int32 LActe, bool bBoss)
{
	FRandomStream R(FMath::Rand());
	const FIntPoint Depart(1, 3);
	const FIntPoint Directions[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (int32 Essai = 0; Essai < 40; Essai++)
	{
		Carte.Init('.', Colonnes * Lignes);
		auto Protegee = [&](FIntPoint P) {
			const FIntPoint E = P - Depart;
			if (FMath::Abs(E.X) + FMath::Abs(E.Y) <= 1 || P == FIntPoint(0, 3))
			{
				return true;		// la case de depart d'AYLIS et ses voisines
			}
			const FIntPoint B = P - FIntPoint(9, 3);
			return bBoss && FMath::Abs(B.X) + FMath::Abs(B.Y) <= 1;		// la place du boss
		};
		auto Poser = [&](FIntPoint P, TCHAR T) {
			if (EstDansArene(P) && !Protegee(P) && Carte[Index(P)] == '.')
			{
				Carte[Index(P)] = T;
			}
		};
		auto AuHasard = [&]() { return FIntPoint(R.RandRange(0, Colonnes - 1), R.RandRange(0, Lignes - 1)); };
		auto Tache = [&](TCHAR T, int32 Nombre, int32 Taille) {
			for (int32 n = 0; n < Nombre; n++)
			{
				FIntPoint P = AuHasard();
				for (int32 k = 0; k < Taille; k++)
				{
					Poser(P, T);
					P += Directions[R.RandRange(0, 3)];
				}
			}
		};
		// Les obstacles : des arbres dans les bois, des rochers et des murs ailleurs
		const int32 Obstacles = bBoss ? R.RandRange(5, 8) : R.RandRange(9, 15);
		const TCHAR Principal = (LActe == 4 || LActe >= 6) ? '#' : 'T';
		const TCHAR Second = Principal == '#' ? 'T' : '#';
		const float PartSecond = LActe >= 6 ? 0.12f : 0.35f;
		for (int32 i = 0; i < Obstacles; i++)
		{
			Poser(AuHasard(), R.FRand() < PartSecond ? Second : Principal);
		}
		if (LActe == 4 || LActe == 7)
		{
			// Des pans de murs (deux rochers cote a cote)
			for (int32 i = 0; i < 2; i++)
			{
				const FIntPoint P = AuHasard();
				Poser(P, '#');
				Poser(P + (R.RandBool() ? FIntPoint(1, 0) : FIntPoint(0, 1)), '#');
			}
		}
		// Les terrains de l'acte
		switch (LActe)
		{
			case 2: Tache('o', 1, 3); break;
			case 3: Tache('o', R.RandRange(3, 4), 4); Tache('x', 2, 3); break;
			case 4: for (int32 i = 0; i < R.RandRange(8, 11); i++) { Poser(AuHasard(), '^'); } break;
			case 5: Tache('*', R.RandRange(3, 5), 5); break;
			case 6: Tache('x', R.RandRange(3, 4), 3); break;
			case 7:
				// Deux failles a gauche, deux a droite ; et un peu de tout ce qu'AYLIS a traverse
				for (int32 i = 0; i < 2; i++)
				{
					Poser(FIntPoint(R.RandRange(2, 5), R.RandRange(0, Lignes - 1)), '@');
					Poser(FIntPoint(R.RandRange(6, 10), R.RandRange(0, Lignes - 1)), '@');
				}
				Tache('x', 1, 3);
				Tache('*', 1, 4);
				for (int32 i = 0; i < 3; i++) { Poser(AuHasard(), '^'); }
				break;
			default: break;
		}
		// On verifie : depuis la case d'AYLIS, on doit pouvoir atteindre (presque) toutes les cases libres
		TArray<bool> Vu;
		Vu.Init(false, Colonnes * Lignes);
		TArray<FIntPoint> AVisiter = {Depart};
		Vu[Index(Depart)] = true;
		for (int32 i = 0; i < AVisiter.Num(); i++)
		{
			for (const FIntPoint& Dir : Directions)
			{
				const FIntPoint V = AVisiter[i] + Dir;
				if (!EstBloquee(V) && !Vu[Index(V)])
				{
					Vu[Index(V)] = true;
					AVisiter.Add(V);
				}
			}
		}
		int32 Libres = 0, ADroite = 0;
		for (int32 i = 0; i < Carte.Num(); i++)
		{
			if (Carte[i] != '#' && Carte[i] != 'T')
			{
				Libres++;
				if (Vu[i] && i % Colonnes >= 8 && (Carte[i] == '.' || Carte[i] == 'o' || Carte[i] == '*'))
				{
					ADroite++;
				}
			}
		}
		if (AVisiter.Num() >= Libres - 1 && ADroite >= 14)
		{
			return true;
		}
	}
	return false;
}

// ===================== L'arene =====================

void AVespGrille::ConstruireCarte()
{
	if (Occupants.Num() != Colonnes * Lignes)
	{
		Occupants.Init(nullptr, Colonnes * Lignes);
	}
	Sol->ClearInstances();
	Rochers->ClearInstances();
	Arbres->ClearInstances();
	const bool bMorts = Style(Acte).bArbresMorts;
	if (bVraisArbres)
	{
		Arbres->SetStaticMesh(bMorts ? ArbreMort.Get() : Sapin.Get());
	}
	for (int32 L = 0; L < Lignes; L++)
	{
		for (int32 C = 0; C < Colonnes; C++)
		{
			const FIntPoint Case(C, L);
			const FVector Centre = CentreDeCase(Case);
			// Le carre de sol, un peu plus petit que la case : on voit la grille entre les cases
			Sol->AddInstance(FTransform(FRotator::ZeroRotator, Centre, FVector(0.96f, 0.96f, 1.0f)), true);
			if (Carte[Index(Case)] == '#')
			{
				if (bVraisRochers)
				{
					const float E = EchelleSur(Rochers->GetStaticMesh(), 70.0f, 85.0f);
					Rochers->AddInstance(FTransform(FRotator(0, 37.0f * C + 11.0f * L, 0), Centre, FVector(E)), true);
				}
				else
				{
					Rochers->AddInstance(FTransform(FRotator(0, 20.0f * C, 0), Centre + FVector(0, 0, 35), FVector(0.7f, 0.7f, 0.7f)), true);
				}
			}
			else if (Carte[Index(Case)] == 'T')
			{
				if (bVraisArbres)
				{
					const float E = EchelleSur(Arbres->GetStaticMesh(), 260.0f + 40.0f * ((C + L) % 3), 150.0f);
					Arbres->AddInstance(FTransform(FRotator(0, 53.0f * C, 0), Centre, FVector(E)), true);
				}
				else
				{
					Arbres->AddInstance(FTransform(FRotator::ZeroRotator, Centre + FVector(0, 0, 90), FVector(0.8f, 0.8f, 1.8f)), true);
				}
			}
		}
	}
	if (CouleurSol)
	{
		CouleurSol->SetVectorParameterValue(TEXT("Color"), Style(Acte).Sol);
	}
	if (CouleurPoison)
	{
		CouleurPoison->SetVectorParameterValue(TEXT("Color"), Style(Acte).Poison);
	}
	ConstruireTerrain();
}

// Les terrains speciaux, poses juste au-dessus des dalles
void AVespGrille::ConstruireTerrain()
{
	for (UInstancedStaticMeshComponent* C : {TerrBoue.Get(), TerrPoison.Get(), TerrPieges.Get(), TerrGlace.Get(), TerrFailles.Get()})
	{
		C->ClearInstances();
	}
	for (int32 i = 0; i < Carte.Num(); i++)
	{
		const FIntPoint Case(i % Colonnes, i / Colonnes);
		const FVector Centre = CentreDeCase(Case);
		switch (Carte[i])
		{
			case 'o': TerrBoue->AddInstance(FTransform(FRotator(0, 90.0f * (i % 4), 0), Centre + FVector(0, 0, 0.7f), FVector(0.97f, 0.97f, 1.0f)), true); break;
			case 'x': TerrPoison->AddInstance(FTransform(FRotator::ZeroRotator, Centre + FVector(0, 0, 0.9f), FVector(0.93f, 0.93f, 1.0f)), true); break;
			case '^': TerrPieges->AddInstance(FTransform(FRotator(0, 45.0f, 0), Centre + FVector(0, 0, 1.0f), FVector(0.55f, 0.55f, 1.0f)), true); break;
			case '*': TerrGlace->AddInstance(FTransform(FRotator::ZeroRotator, Centre + FVector(0, 0, 0.6f), FVector(0.98f, 0.98f, 1.0f)), true); break;
			case '@': TerrFailles->AddInstance(FTransform(FRotator::ZeroRotator, Centre + FVector(0, 0, 1.2f), FVector(0.75f, 0.75f, 0.012f)), true); break;
			default: break;
		}
	}
	TerrPieges->SetMaterial(0, bPiegesLeves ? PiegeLeve.Get() : PiegeBaisse.Get());
}

TCHAR AVespGrille::TerrainSur(FIntPoint Case) const
{
	return EstDansArene(Case) ? Carte[Index(Case)] : TCHAR('#');
}

bool AVespGrille::EstDangereuse(FIntPoint Case) const
{
	const TCHAR T = TerrainSur(Case);
	return T == 'x' || (T == '^' && bPiegesLeves);
}

void AVespGrille::ChangerTerrain(FIntPoint Case, TCHAR Terrain)
{
	if (EstDansArene(Case) && !EstBloquee(Case))
	{
		Carte[Index(Case)] = Terrain;
		ConstruireTerrain();
	}
}

void AVespGrille::LeverPieges(bool bLeves)
{
	bPiegesLeves = bLeves;
	TerrPieges->SetMaterial(0, bPiegesLeves ? PiegeLeve.Get() : PiegeBaisse.Get());
}

// La glace : tant que le chemin finit sur la glace, on glisse d'une case de plus dans le meme sens
void AVespGrille::Glisser(FIntPoint Depart, TArray<FIntPoint>& LeChemin) const
{
	for (int32 Glissades = 0; Glissades < 4 && LeChemin.Num() > 0 && TerrainSur(LeChemin.Last()) == '*'; Glissades++)
	{
		const FIntPoint Avant = LeChemin.Num() > 1 ? LeChemin[LeChemin.Num() - 2] : Depart;
		const FIntPoint Sens = LeChemin.Last() - Avant;
		if (FMath::Abs(Sens.X) + FMath::Abs(Sens.Y) != 1)
		{
			break;
		}
		const FIntPoint Suite = LeChemin.Last() + Sens;
		if (EstBloquee(Suite) || UniteSur(Suite))
		{
			break;
		}
		LeChemin.Add(Suite);
	}
}

bool AVespGrille::AutreFaille(FIntPoint Case, FIntPoint& Sortie) const
{
	TArray<FIntPoint> Failles;
	for (int32 i = 0; i < Carte.Num(); i++)
	{
		const FIntPoint P(i % Colonnes, i / Colonnes);
		if (Carte[i] == '@' && P != Case && !UniteSur(P))
		{
			Failles.Add(P);
		}
	}
	if (Failles.Num() == 0)
	{
		return false;
	}
	Sortie = Failles[FMath::RandRange(0, Failles.Num() - 1)];
	return true;
}

TArray<FIntPoint> AVespGrille::CasesLibres(int32 ColonneMin) const
{
	TArray<FIntPoint> Libres;
	for (int32 i = 0; i < Carte.Num(); i++)
	{
		const FIntPoint P(i % Colonnes, i / Colonnes);
		if (P.X >= ColonneMin && (Carte[i] == '.' || Carte[i] == 'o' || Carte[i] == '*') && !UniteSur(P))
		{
			Libres.Add(P);
		}
	}
	return Libres;
}

void AVespGrille::Occuper(AVespUnite* Unite)
{
	if (EstDansArene(Unite->GetCase()))
	{
		Occupants[Index(Unite->GetCase())] = Unite;
	}
}

void AVespGrille::Liberer(AVespUnite* Unite)
{
	for (TObjectPtr<AVespUnite>& Occupant : Occupants)
	{
		if (Occupant == Unite)
		{
			Occupant = nullptr;
		}
	}
}

AVespUnite* AVespGrille::UniteSur(FIntPoint Case) const
{
	return EstDansArene(Case) && Occupants.IsValidIndex(Index(Case)) ? Occupants[Index(Case)].Get() : nullptr;
}

TArray<FIntPoint> AVespGrille::ApprocheVers(FIntPoint Depart, FIntPoint Cible, int32 PasMax) const
{
	// 1. La carte des distances jusqu'a la cible, en contournant les obstacles (les unites ne comptent pas :
	//    elles finiront par se pousser, comme dans le prototype)
	TArray<int32> Distance;
	Distance.Init(-1, Colonnes * Lignes);
	TArray<FIntPoint> AVisiter = {Cible};
	Distance[Index(Cible)] = 0;
	const FIntPoint Directions[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (int32 i = 0; i < AVisiter.Num(); i++)
	{
		for (const FIntPoint& Dir : Directions)
		{
			const FIntPoint Voisine = AVisiter[i] + Dir;
			if (!EstBloquee(Voisine) && Distance[Index(Voisine)] == -1)
			{
				Distance[Index(Voisine)] = Distance[Index(AVisiter[i])] + 1;
				AVisiter.Add(Voisine);
			}
		}
	}
	// 2. Pas a pas : a chaque fois, la case voisine libre (et sans danger) la plus proche de la cible
	TArray<FIntPoint> LeChemin;
	FIntPoint Ici = Depart;
	for (int32 Pas = 0; Pas < PasMax; Pas++)
	{
		if (FMath::Abs(Ici.X - Cible.X) + FMath::Abs(Ici.Y - Cible.Y) <= 1)
		{
			break;		// au contact
		}
		FIntPoint Meilleure = Ici;
		int32 MeilleureDistance = Distance[Index(Ici)] >= 0 ? Distance[Index(Ici)] : MAX_int32;
		for (const FIntPoint& Dir : Directions)
		{
			const FIntPoint Voisine = Ici + Dir;
			if (!EstBloquee(Voisine) && !UniteSur(Voisine) && !EstDangereuse(Voisine) && Distance[Index(Voisine)] >= 0
			    && Distance[Index(Voisine)] < MeilleureDistance)
			{
				MeilleureDistance = Distance[Index(Voisine)];
				Meilleure = Voisine;
			}
		}
		if (Meilleure == Ici)
		{
			break;		// bloque
		}
		LeChemin.Add(Meilleure);
		Ici = Meilleure;
		if (Carte[Index(Ici)] == 'o')
		{
			break;		// la boue arrete la marche (pour les Haschen aussi)
		}
	}
	return LeChemin;
}

FVector AVespGrille::CentreDeCase(FIntPoint Case) const
{
	// L'arene est centree sur la position de la grille ; la colonne va vers +Y (la droite a l'ecran), la ligne vers -X (le bas)
	const FVector Origine = GetActorLocation();
	return Origine + FVector((Case.Y - Lignes / 2.0f + 0.5f) * -TailleCase, (Case.X - Colonnes / 2.0f + 0.5f) * TailleCase, 0.0f);
}

bool AVespGrille::CaseSousPoint(const FVector& Point, FIntPoint& Case) const
{
	const FVector Local = Point - GetActorLocation();
	Case.X = FMath::FloorToInt(Local.Y / TailleCase + Colonnes / 2.0f);
	Case.Y = FMath::FloorToInt(-Local.X / TailleCase + Lignes / 2.0f);
	return EstDansArene(Case);
}

bool AVespGrille::EstDansArene(FIntPoint Case) const
{
	return Case.X >= 0 && Case.X < Colonnes && Case.Y >= 0 && Case.Y < Lignes;
}

bool AVespGrille::EstBloquee(FIntPoint Case) const
{
	if (!EstDansArene(Case))
	{
		return true;
	}
	const TCHAR T = Carte[Index(Case)];
	return T == '#' || T == 'T';
}

// Le "parcours en largeur" : on part de la case d'AYLIS et on s'etend case par case, comme une tache d'encre
TArray<int32> AVespGrille::CasesAtteignables(FIntPoint Depart, int32 PasMax) const
{
	TArray<int32> Pas;
	Pas.Init(-1, Colonnes * Lignes);
	TArray<FIntPoint> AVisiter = {Depart};
	Pas[Index(Depart)] = 0;
	const FIntPoint Directions[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (int32 i = 0; i < AVisiter.Num(); i++)
	{
		const FIntPoint Ici = AVisiter[i];
		if (Pas[Index(Ici)] == PasMax || (Ici != Depart && Carte[Index(Ici)] == 'o'))
		{
			continue;	// on ne peut pas aller plus loin (la boue arrete la marche)
		}
		for (const FIntPoint& Dir : Directions)
		{
			const FIntPoint Voisine = Ici + Dir;
			if (!EstBloquee(Voisine) && !UniteSur(Voisine) && Pas[Index(Voisine)] == -1)
			{
				Pas[Index(Voisine)] = Pas[Index(Ici)] + 1;
				AVisiter.Add(Voisine);
			}
		}
	}
	return Pas;
}

// Le chemin : on remonte depuis l'arrivee, en prenant a chaque fois une voisine qui a un pas de moins
TArray<FIntPoint> AVespGrille::Chemin(FIntPoint Depart, FIntPoint Arrivee, int32 PasMax) const
{
	TArray<FIntPoint> Resultat;
	const TArray<int32> Pas = CasesAtteignables(Depart, PasMax);
	if (!EstDansArene(Arrivee) || Pas[Index(Arrivee)] <= 0)
	{
		return Resultat;
	}
	const FIntPoint Directions[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	FIntPoint Ici = Arrivee;
	Resultat.Add(Ici);
	while (Pas[Index(Ici)] > 1)
	{
		for (const FIntPoint& Dir : Directions)
		{
			const FIntPoint Voisine = Ici + Dir;
			// (on ne repasse pas par la boue : elle aurait arrete la marche)
			if (EstDansArene(Voisine) && Pas[Index(Voisine)] == Pas[Index(Ici)] - 1 && (Carte[Index(Voisine)] != 'o' || Voisine == Depart))
			{
				Ici = Voisine;
				break;
			}
		}
		Resultat.Insert(Ici, 0);
	}
	return Resultat;
}

void AVespGrille::AfficherCasesAtteignables(const TArray<int32>& Pas)
{
	Accessibles->ClearInstances();
	for (int32 i = 0; i < Pas.Num(); i++)
	{
		if (Pas[i] > 0)
		{
			const FIntPoint Case(i % Colonnes, i / Colonnes);
			Accessibles->AddInstance(FTransform(FRotator::ZeroRotator, CentreDeCase(Case) + FVector(0, 0, 1.5f), FVector(0.86f, 0.86f, 1.0f)), true);
		}
	}
}

void AVespGrille::ViderOccupants()
{
	Occupants.Init(nullptr, Colonnes * Lignes);
}

void AVespGrille::AfficherDanger(const TArray<FIntPoint>& Cases)
{
	Danger->ClearInstances();
	for (const FIntPoint& Case : Cases)
	{
		Danger->AddInstance(FTransform(FRotator::ZeroRotator, CentreDeCase(Case) + FVector(0, 0, 3.5f), FVector(0.9f, 0.9f, 1.0f)), true);
	}
}

void AVespGrille::AfficherSurvol(FIntPoint Case)
{
	Survol->ClearInstances();
	if (EstDansArene(Case))
	{
		Survol->AddInstance(FTransform(FRotator::ZeroRotator, CentreDeCase(Case) + FVector(0, 0, 2.5f), FVector(0.3f, 0.3f, 1.0f)), true);
	}
}

// ===================== Le monde autour de l'arene =====================
// Tout est pose "au hasard", mais avec une graine fixe : le meme monde a chaque partie, pour chaque acte.
// Un sentier traverse la clairiere de gauche a droite. Devant la camera, rien de haut (pour ne jamais cacher
// l'arene) ; derriere et sur les cotes, le monde est dense.

void AVespGrille::ConstruireEnvironnement()
{
	const FVespStyleActe& S = Style(Acte);
	FRandomStream Hasard(4242 + Acte * 131 + Carte.Num() + (Carte.Num() > 0 ? Carte[5] + Carte[40] * 7 + Carte[77] * 13 : 0));
	const FVector Centre = GetActorLocation();
	const float DemiX = Lignes * TailleCase / 2.0f, DemiY = Colonnes * TailleCase / 2.0f;
	auto H = [&Hasard](float A, float B) { return Hasard.FRandRange(A, B); };
	// Le sentier : une courbe douce, de la gauche du monde jusqu'a l'arene, puis de l'arene vers la droite
	auto CheminX = [](float Y) { return FMath::Sin(Y / 520.0f) * 170.0f + FMath::Sin(Y / 190.0f) * 35.0f; };
	auto PresDuChemin = [&](const FVector& P, float Marge) { return FMath::Abs(P.Y) > DemiY - 40.0f && FMath::Abs(P.X - CheminX(P.Y)) < Marge; };
	auto DansLArene = [&](const FVector& P, float Marge) { return FMath::Abs(P.X) < DemiX + Marge && FMath::Abs(P.Y) < DemiY + Marge; };
	auto DevantLaCamera = [&](const FVector& P) { return P.X < -DemiX; };		// en bas de l'ecran : seulement des choses basses
	auto Pose = [&](const FVector& P, float Z = 0.0f) { return Centre + FVector(P.X, P.Y, Z); };

	// Les couleurs de l'acte
	auto Teindre = [](UMaterialInstanceDynamic* M, const FLinearColor& C) { if (M) { M->SetVectorParameterValue(TEXT("Color"), C); } };
	Teindre(CouleurFleurs, S.Fleurs);
	Teindre(CouleurChampignons, S.Champignons);
	Teindre(CouleurHerbes, S.Herbes);
	Teindre(CouleurBuissons, S.Buissons);
	Teindre(CouleurTaches, S.Taches);
	Teindre(CouleurTerre, S.Terre);
	Teindre(CouleurChemin, S.Chemin);
	Teindre(CouleurSol, S.Sol);

	// ----- Les herbes et les fleurs (dans l'arene : purement decoratives, elles ne genent pas) -----
	auto Touffe = [&](FRandomStream& R, const FVector& P) {
		if (bVraiesHerbes)
		{
			EnvHerbes->AddInstance(FTransform(FRotator(0, R.FRandRange(0.0f, 360.0f), 0), Pose(P),
			                                  FVector(EchelleSur(EnvHerbes->GetStaticMesh(), R.FRandRange(18.0f, 38.0f), 70.0f))), true);
			return;
		}
		// Sans modele : 3 brins (des cones tres fins), un peu penches ; dans les marais, des roseaux plus hauts
		const float Grand = Acte == 3 ? 2.0f : 1.0f;
		for (int32 i = 0; i < 3; i++)
		{
			const float Haut = R.FRandRange(0.14f, 0.34f) * Grand;
			const FRotator Rot(R.FRandRange(-22.0f, 22.0f), R.FRandRange(0.0f, 360.0f), R.FRandRange(-22.0f, 22.0f));
			EnvHerbes->AddInstance(FTransform(Rot, Pose(P + FVector(R.FRandRange(-8.0f, 8.0f), R.FRandRange(-8.0f, 8.0f), 0), Haut * 50.0f),
			                                  FVector(0.035f, 0.035f, Haut)), true);
		}
	};
	auto Fleur = [&](FRandomStream& R, const FVector& P) {
		if (bVraiesFleurs)
		{
			EnvFleurs->AddInstance(FTransform(FRotator(0, R.FRandRange(0.0f, 360.0f), 0), Pose(P),
			                                  FVector(EchelleSur(EnvFleurs->GetStaticMesh(), R.FRandRange(15.0f, 30.0f), 40.0f))), true);
			return;
		}
		EnvFleurs->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, R.FRandRange(8.0f, 22.0f)), FVector(R.FRandRange(0.04f, 0.075f))), true);
	};
	EnvHerbes->ClearInstances();
	EnvFleurs->ClearInstances();
	const float DensiteArene = S.NombreHerbes / 1100.0f;
	for (int32 L = 0; L < Lignes; L++)
	{
		for (int32 C = 0; C < Colonnes; C++)
		{
			const FVector Milieu = CentreDeCase(FIntPoint(C, L)) - Centre;
			const TCHAR T = Carte[Index(FIntPoint(C, L))];
			if (T == 'x' || T == '^' || T == '@' || T == '*')
			{
				continue;		// rien ne pousse sur la lave, les dalles ou la glace
			}
			const bool bLibre = T == '.' || T == 'o';
			// Sur une case libre : quelques touffes pres des bords (le centre reste degage pour les pions)
			const int32 Nombre = FMath::RoundToInt((bLibre ? Hasard.RandRange(1, 3) : Hasard.RandRange(4, 7)) * DensiteArene);
			for (int32 i = 0; i < Nombre; i++)
			{
				FVector P = Milieu + FVector(H(-46.0f, 46.0f), H(-46.0f, 46.0f), 0);
				if (bLibre && FVector::Dist2D(P, Milieu) < 30.0f)
				{
					P = Milieu + (P - Milieu).GetSafeNormal2D() * 38.0f;
				}
				Touffe(Hasard, P);
			}
			if (Hasard.FRand() < (bLibre ? 0.8f : 2.5f) * S.PartFleurs)
			{
				Fleur(Hasard, Milieu + FVector(H(-42.0f, 42.0f), H(-42.0f, 42.0f), 0));
			}
		}
	}

	// ----- Le monde du dehors (avec sa propre graine : le meme pour tout l'acte) -----
	FRandomStream Monde(900 + Acte * 77);
	auto F = [&Monde](float A, float B) { return Monde.FRandRange(A, B); };
	auto Point = [&]() { return FVector(F(-1150.0f, 2900.0f), F(-3000.0f, 3000.0f), 0); };
	auto RotF = [&Monde]() { return FRotator(0, Monde.FRandRange(0.0f, 360.0f), 0); };

	for (int32 i = 0; i < S.NombreHerbes; i++)
	{
		const FVector P = Point();
		if (DansLArene(P, 5.0f) || PresDuChemin(P, 70.0f))
		{
			continue;
		}
		Touffe(Monde, P);
		if (Monde.FRand() < S.PartFleurs)
		{
			Fleur(Monde, P + FVector(F(-30.0f, 30.0f), F(-30.0f, 30.0f), 0));
		}
	}
	if (EnvironnementConstruit == Acte)
	{
		return;		// le reste du monde est deja la
	}
	EnvironnementConstruit = Acte;
	for (UInstancedStaticMeshComponent* C : {EnvTerre.Get(), EnvArbres.Get(), EnvRochers.Get(), EnvBuissons.Get(), EnvChampignons.Get(),
	                                         EnvTroncs.Get(), EnvTombes.Get(), EnvTaches.Get(), EnvChemin.Get(), EnvSpecial.Get(), EnvBlocs.Get()})
	{
		C->ClearInstances();
	}
	if (bVraisArbres)
	{
		EnvArbres->SetStaticMesh(S.bArbresMorts ? ArbreMort.Get() : Sapin.Get());
	}

	// Le grand sol, sous tout le reste
	EnvTerre->AddInstance(FTransform(FRotator::ZeroRotator, Pose(FVector(800.0f, 0, 0), -3.7f), FVector(70.0f, 80.0f, 1.0f)), true);
	// Des taches de mousse, de neige ou de cendre, pour casser l'uniformite
	for (int32 i = 0; i < 110; i++)
	{
		const FVector P = Point();
		const float E = F(1.5f, 5.0f);
		EnvTaches->AddInstance(FTransform(RotF(), Pose(P, -3.3f), FVector(E, E * F(0.6f, 1.0f), 0.01f)), true);
	}
	// Le sentier : des plaques qui se chevauchent, bordees de petits cailloux
	TArray<FVector> Cailloux;
	TArray<FVector> Sentier;
	for (float Y = -3000.0f; Y <= 3000.0f; Y += 55.0f)
	{
		if (FMath::Abs(Y) < DemiY - 20.0f)
		{
			continue;
		}
		const FVector P(CheminX(Y) + F(-15.0f, 15.0f), Y, 0);
		Sentier.Add(P);
		const float E = F(1.1f, 1.5f);
		EnvChemin->AddInstance(FTransform(RotF(), Pose(P, -2.5f), FVector(E, E * F(0.75f, 1.0f), 0.012f)), true);
		if (Monde.FRand() < 0.35f)
		{
			Cailloux.Add(P + FVector((Monde.FRand() < 0.5f ? -1.0f : 1.0f) * F(70.0f, 95.0f), F(-20.0f, 20.0f), 0));
		}
	}

	// Les arbres : denses derriere l'arene et sur les cotes, jamais sur le sentier ni devant la camera
	TArray<FVector> PiedsDArbres;
	for (int32 i = 0, Essais = 0; i < S.NombreArbres && Essais < 8000; Essais++)
	{
		const FVector P = Point();
		if (DansLArene(P, 90.0f) || PresDuChemin(P, 150.0f) || DevantLaCamera(P))
		{
			continue;
		}
		i++;
		PiedsDArbres.Add(P);
		// Plus loin de l'arene, plus grands (la foret qui monte)
		const float Loin = FMath::Clamp(FMath::Max(P.X - DemiX, FMath::Abs(P.Y) - DemiY) / 1500.0f, 0.0f, 1.0f);
		const float Hauteur = F(260.0f, 380.0f) + Loin * F(80.0f, 240.0f);
		if (bVraisArbres)
		{
			EnvArbres->AddInstance(FTransform(RotF(), Pose(P), FVector(EchelleSur(EnvArbres->GetStaticMesh(), Hauteur, 400.0f))), true);
		}
		else
		{
			EnvArbres->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, Hauteur / 2.0f), FVector(0.9f, 0.9f, Hauteur / 100.0f)), true);
		}
	}
	// Les buissons : partout, y compris devant la camera (ils sont bas)
	for (int32 i = 0; i < S.NombreBuissons; i++)
	{
		const FVector P = Point();
		if (DansLArene(P, 60.0f) || PresDuChemin(P, 110.0f))
		{
			continue;
		}
		if (bVraisBuissons)
		{
			EnvBuissons->AddInstance(FTransform(RotF(), Pose(P), FVector(EchelleSur(EnvBuissons->GetStaticMesh(), F(45.0f, 90.0f), 160.0f))), true);
		}
		else
		{
			const float L = F(0.7f, 1.4f);
			EnvBuissons->AddInstance(FTransform(RotF(), Pose(P, 20.0f), FVector(L, L * F(0.7f, 1.0f), F(0.45f, 0.8f))), true);
		}
	}
	// Les rochers : le long du sentier, en bordure de l'arene, et semes partout
	auto Rocher = [&](const FVector& P, float Hauteur) {
		if (bVraisRochers)
		{
			EnvRochers->AddInstance(FTransform(RotF(), Pose(P), FVector(EchelleSur(EnvRochers->GetStaticMesh(), Hauteur, Hauteur * 1.8f))), true);
		}
		else
		{
			const float E = Hauteur / 100.0f;
			EnvRochers->AddInstance(FTransform(FRotator(F(-15.0f, 15.0f), F(0.0f, 360.0f), F(-15.0f, 15.0f)), Pose(P, Hauteur * 0.35f),
			                                   FVector(E * 1.3f, E, E)), true);
		}
	};
	for (const FVector& P : Cailloux)
	{
		Rocher(P, F(12.0f, 26.0f));
	}
	for (int32 i = 0; i < 26; i++)
	{
		const int32 Bord = Monde.RandRange(0, 3);
		const FVector P = Bord < 2 ? FVector((Bord == 0 ? 1 : -1) * (DemiX + F(40.0f, 110.0f)), F(-DemiY, DemiY), 0)
		                           : FVector(F(-DemiX, DemiX), (Bord == 2 ? 1 : -1) * (DemiY + F(40.0f, 110.0f)), 0);
		if (!PresDuChemin(P, 120.0f))
		{
			Rocher(P, F(25.0f, 60.0f));
		}
	}
	for (int32 i = 0; i < (Acte >= 5 ? 110 : 70); i++)
	{
		const FVector P = Point();
		if (!DansLArene(P, 60.0f) && !PresDuChemin(P, 110.0f))
		{
			Rocher(P, F(30.0f, DevantLaCamera(P) ? 60.0f : 160.0f));
		}
	}
	// Les champignons luminescents : en cercles au pied des arbres (ou des rochers, s'il n'y a pas d'arbres)
	for (int32 i = 0; i < S.NombreChampignons && PiedsDArbres.Num() > 0; i++)
	{
		const FVector Pied = PiedsDArbres[Monde.RandRange(0, PiedsDArbres.Num() - 1)];
		const int32 Nombre = Monde.RandRange(3, 6);
		const float Depart = F(0.0f, 2.0f * PI);
		for (int32 k = 0; k < Nombre; k++)
		{
			const float A = Depart + k * 0.55f;
			const FVector P = Pied + FVector(FMath::Cos(A), FMath::Sin(A), 0) * F(55.0f, 85.0f);
			if (bVraisChampignons)
			{
				EnvChampignons->AddInstance(FTransform(RotF(), Pose(P), FVector(EchelleSur(EnvChampignons->GetStaticMesh(), F(12.0f, 26.0f), 30.0f))), true);
			}
			else
			{
				const float E = F(0.08f, 0.15f);
				EnvChampignons->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, E * 60.0f), FVector(E, E, E * 0.45f)), true);
			}
		}
	}
	// Les troncs couches
	for (int32 i = 0; i < (Acte == 6 || Acte == 7 ? 6 : 20); i++)
	{
		const FVector P = Point();
		if (DansLArene(P, 150.0f) || PresDuChemin(P, 160.0f))
		{
			continue;
		}
		if (bVraisTroncs)
		{
			EnvTroncs->AddInstance(FTransform(RotF(), Pose(P), FVector(EchelleSur(EnvTroncs->GetStaticMesh(), F(30.0f, 60.0f), 260.0f))), true);
		}
		else
		{
			EnvTroncs->AddInstance(FTransform(FRotator(90.0f, F(0.0f, 360.0f), 0), Pose(P, 17.0f), FVector(0.35f, 0.35f, F(1.6f, 2.6f))), true);
		}
	}
	// Les tombes du Bois des Pendus, par petits groupes
	if (S.bTombes)
	{
		for (int32 g = 0; g < 10; g++)
		{
			const FVector Milieu = Point();
			const int32 Nombre = Monde.RandRange(3, 6);
			for (int32 k = 0; k < Nombre; k++)
			{
				const FVector P = Milieu + FVector(F(-160.0f, 160.0f), F(-160.0f, 160.0f), 0);
				if (DansLArene(P, 70.0f) || PresDuChemin(P, 100.0f))
				{
					continue;
				}
				if (bVraiesTombes)
				{
					EnvTombes->AddInstance(FTransform(RotF(), Pose(P), FVector(EchelleSur(EnvTombes->GetStaticMesh(), F(50.0f, 80.0f), 80.0f))), true);
				}
				else
				{
					EnvTombes->AddInstance(FTransform(FRotator(F(-8.0f, 8.0f), F(-12.0f, 12.0f), F(-8.0f, 8.0f)), Pose(P, 32.0f), FVector(0.12f, 0.45f, 0.65f)), true);
				}
			}
		}
	}

	// ----- Le decor special de l'acte -----
	auto PointLibre = [&](float Marge) {
		for (int32 k = 0; k < 30; k++)
		{
			const FVector P = Point();
			if (!DansLArene(P, Marge) && !PresDuChemin(P, Marge + 40.0f))
			{
				return P;
			}
		}
		return FVector(2500.0f, 0, 0);
	};
	switch (S.Special)
	{
		case 3:		// les marais : des mares d'eau noire, et des roseaux autour
		{
			EnvSpecial->SetStaticMesh(Cylindre);
			SpecialMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.012f, 0.04f, 0.05f));
			EnvSpecial->SetMaterial(0, SpecialMat);
			for (int32 i = 0; i < 45; i++)
			{
				const FVector P = PointLibre(120.0f);
				const float E = F(2.0f, 6.0f);
				EnvSpecial->AddInstance(FTransform(RotF(), Pose(P, -2.9f), FVector(E, E * F(0.5f, 0.9f), 0.01f)), true);
				for (int32 k = 0; k < 8; k++)
				{
					const float A = F(0.0f, 2.0f * PI);
					Touffe(Monde, P + FVector(FMath::Cos(A) * E * 45.0f, FMath::Sin(A) * E * 40.0f, 0));
				}
			}
			break;
		}
		case 4:		// la forteresse : des colonnes (certaines brisees) le long du sentier, et des blocs tombes
		{
			EnvSpecial->SetStaticMesh(Cylindre);
			SpecialMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.3f, 0.28f, 0.25f));
			EnvSpecial->SetMaterial(0, SpecialMat);
			for (int32 i = 0; i < Sentier.Num(); i += 6)
			{
				for (int32 Cote = -1; Cote <= 1; Cote += 2)
				{
					const FVector P = Sentier[i] + FVector(Cote * 180.0f, 0, 0);
					if (DansLArene(P, 100.0f))
					{
						continue;
					}
					const float Haut = DevantLaCamera(P) ? F(40.0f, 110.0f) : (Monde.FRand() < 0.3f ? F(60.0f, 160.0f) : F(300.0f, 480.0f));
					EnvSpecial->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, Haut / 2.0f), FVector(0.5f, 0.5f, Haut / 100.0f)), true);
				}
			}
			for (int32 i = 0; i < 30; i++)
			{
				const FVector P = PointLibre(120.0f);
				const float Haut = DevantLaCamera(P) ? F(40.0f, 100.0f) : F(150.0f, 420.0f);
				EnvSpecial->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, Haut / 2.0f), FVector(0.45f, 0.45f, Haut / 100.0f)), true);
			}
			EnvBlocs->SetStaticMesh(Cube);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.26f, 0.24f, 0.21f));
			for (int32 i = 0; i < 70; i++)
			{
				const FVector P = PointLibre(80.0f);
				const FVector E(F(0.4f, 1.1f), F(0.4f, 0.9f), F(0.3f, 0.6f));
				EnvBlocs->AddInstance(FTransform(FRotator(F(-12.0f, 12.0f), F(0.0f, 360.0f), F(-12.0f, 12.0f)), Pose(P, E.Z * 40.0f), E), true);
			}
			break;
		}
		case 5:		// le col : des pics de glace qui brillent, et des congeres
		{
			EnvSpecial->SetStaticMesh(Cone);
			SpecialLumineux->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.4f, 0.65f, 1.0f) * 0.8f);
			EnvSpecial->SetMaterial(0, SpecialLumineux);
			for (int32 g = 0; g < 22; g++)
			{
				const FVector Milieu = PointLibre(120.0f);
				for (int32 k = 0; k < Monde.RandRange(3, 6); k++)
				{
					const FVector P = Milieu + FVector(F(-70.0f, 70.0f), F(-70.0f, 70.0f), 0);
					const float Haut = DevantLaCamera(P) ? F(0.4f, 0.9f) : F(0.8f, 2.6f);
					EnvSpecial->AddInstance(FTransform(FRotator(F(-20.0f, 20.0f), F(0.0f, 360.0f), F(-20.0f, 20.0f)), Pose(P, Haut * 40.0f),
					                                   FVector(F(0.2f, 0.45f), F(0.2f, 0.45f), Haut)), true);
				}
			}
			EnvBlocs->SetStaticMesh(Sphere);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.5f, 0.55f, 0.63f));
			for (int32 i = 0; i < 60; i++)
			{
				const FVector P = PointLibre(60.0f);
				const float L = F(1.5f, 4.0f);
				EnvBlocs->AddInstance(FTransform(RotF(), Pose(P, 0.0f), FVector(L, L * F(0.5f, 0.9f), F(0.3f, 0.6f))), true);
			}
			break;
		}
		case 6:		// les terres de cendre : des coulees de lave (le long du sentier aussi) et de l'obsidienne
		{
			EnvSpecial->SetStaticMesh(Cylindre);
			SpecialLumineux->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.3f, 0.03f) * 3.0f);
			EnvSpecial->SetMaterial(0, SpecialLumineux);
			for (int32 i = 0; i < Sentier.Num(); i += 2)
			{
				const float E = F(0.4f, 0.7f);
				EnvSpecial->AddInstance(FTransform(RotF(), Pose(Sentier[i], -2.0f), FVector(E, E * F(0.6f, 1.0f), 0.01f)), true);
			}
			for (int32 i = 0; i < 40; i++)
			{
				const FVector P = PointLibre(120.0f);
				const float E = F(1.2f, 4.0f);
				EnvSpecial->AddInstance(FTransform(RotF(), Pose(P, -2.0f), FVector(E, E * F(0.4f, 0.9f), 0.01f)), true);
			}
			EnvBlocs->SetStaticMesh(Cube);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.025f, 0.03f));
			for (int32 i = 0; i < 90; i++)
			{
				const FVector P = PointLibre(70.0f);
				const float Haut = DevantLaCamera(P) ? F(0.3f, 0.7f) : F(0.6f, 2.8f);
				EnvBlocs->AddInstance(FTransform(FRotator(F(-25.0f, 25.0f), F(0.0f, 360.0f), F(-25.0f, 25.0f)), Pose(P, Haut * 30.0f),
				                                 FVector(F(0.3f, 0.8f), F(0.3f, 0.8f), Haut)), true);
			}
			break;
		}
		case 7:		// Karn : des cristaux du Voile qui brillent, et des colonnes brisees
		{
			EnvSpecial->SetStaticMesh(Cube);
			SpecialLumineux->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.3f, 1.0f) * 2.0f);
			EnvSpecial->SetMaterial(0, SpecialLumineux);
			for (int32 g = 0; g < 26; g++)
			{
				const FVector Milieu = PointLibre(120.0f);
				for (int32 k = 0; k < Monde.RandRange(3, 7); k++)
				{
					const FVector P = Milieu + FVector(F(-80.0f, 80.0f), F(-80.0f, 80.0f), 0);
					const float Haut = DevantLaCamera(P) ? F(0.4f, 0.8f) : F(0.8f, 2.6f);
					EnvSpecial->AddInstance(FTransform(FRotator(F(-25.0f, 25.0f), F(0.0f, 360.0f), F(-25.0f, 25.0f)), Pose(P, Haut * 45.0f),
					                                   FVector(F(0.15f, 0.32f), F(0.15f, 0.32f), Haut)), true);
				}
			}
			EnvBlocs->SetStaticMesh(Cylindre);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.17f, 0.26f));
			for (int32 i = 0; i < 40; i++)
			{
				const FVector P = PointLibre(120.0f);
				const float Haut = DevantLaCamera(P) ? F(40.0f, 100.0f) : F(120.0f, 500.0f);
				EnvBlocs->AddInstance(FTransform(FRotator(F(-6.0f, 6.0f), 0, F(-6.0f, 6.0f)), Pose(P, Haut / 2.0f), FVector(0.5f, 0.5f, Haut / 100.0f)), true);
			}
			break;
		}
		default: break;
	}
}
