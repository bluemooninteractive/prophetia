#include "VespGrille.h"
#include "VespUnite.h"
#include "VespEffet.h"
#include "VespStyles.h"
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

static const FVespStyleActe& Style(int32 Acte)
{
	return StyleDeLActe(Acte);
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
UStaticMesh* AVespGrille::ModeleDuDecor(std::initializer_list<const TCHAR*> Noms)
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
float AVespGrille::EchelleSur(UStaticMesh* Modele, float Hauteur, float LargeurMax)
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
	}
	if (UStaticMesh* Rocher = ModeleDuDecor({TEXT("rock"), TEXT("stone")}))
	{
		Rochers->SetStaticMesh(Rocher);
		bVraisRochers = true;
	}
	else
	{
		Couleur(Rochers, FLinearColor(0.25f, 0.24f, 0.30f));
	}
	CouleurSol = Couleur(Sol, FLinearColor(0.10f, 0.16f, 0.12f));
	Couleur(Accessibles, FLinearColor(0.35f, 0.75f, 1.0f));
	Couleur(Survol, FLinearColor(1.0f, 1.0f, 1.0f));
	Couleur(Danger, FLinearColor(1.0f, 0.12f, 0.08f));
	Couleur(TerrBoue, FLinearColor(0.09f, 0.06f, 0.03f));
	Couleur(TerrGlace, FLinearColor(0.55f, 0.72f, 0.9f));
	CouleurPoison = Lumineux(TerrPoison, FLinearColor::Black);
	Lumineux(TerrFailles, FLinearColor(0.7f, 0.3f, 1.0f) * 3.0f);
	PiegeBaisse = Couleur(TerrPieges, FLinearColor(0.2f, 0.18f, 0.17f));
	PiegeLeve = Lumineux(nullptr, FLinearColor(1.0f, 0.18f, 0.08f) * 2.5f);
	Carte.Init('.', Colonnes * Lignes);
	Occupants.Init(nullptr, Colonnes * Lignes);
}

// La fin du combat : l'arene disparait (le monde reste)
void AVespGrille::Effacer()
{
	for (UInstancedStaticMeshComponent* C : {Sol.Get(), Rochers.Get(), Arbres.Get(), Accessibles.Get(), Survol.Get(), Danger.Get(), TerrBoue.Get(),
	                                         TerrPoison.Get(), TerrPieges.Get(), TerrGlace.Get(), TerrFailles.Get()})
	{
		C->ClearInstances();
	}
	Carte.Init('.', Colonnes * Lignes);
	bPiegesLeves = false;
	ViderOccupants();
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
	AfficherDanger({});
}

// (appele par PreparerCarte : les obstacles de l'arene surgissent du sol)
static void FaireSurgir(UWorld* Monde, const TArray<FVector>& Positions)
{
	for (const FVector& P : Positions)
	{
		AVespEffet::Jouer(Monde, EVespEffet::Poussiere, P, FVector::UpVector, FLinearColor(0.45f, 0.4f, 0.4f));
	}
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
	AfficherDanger({});
	TArray<FVector> Obstacles;
	for (int32 i = 0; i < Carte.Num(); i++)
	{
		if (Carte[i] == '#' || Carte[i] == 'T')
		{
			Obstacles.Add(CentreDeCase(FIntPoint(i % Colonnes, i / Colonnes)));
		}
	}
	FaireSurgir(GetWorld(), Obstacles);
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

