#include "VespGrille.h"
#include "VespUnite.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"

// La carte de la clairiere de la Foret des Brumes : '#' = rocher, 'T' = arbre, '.' = sol libre
// (AYLIS commence a gauche, en colonne 1, ligne 3)
static const TCHAR* CARTE_FORET[AVespGrille::Lignes] = {
	TEXT("..T....T...."),
	TEXT(".....#......"),
	TEXT("..#.....T..."),
	TEXT("............"),
	TEXT("....T...#..."),
	TEXT(".......#...."),
	TEXT("..#......T.."),
	TEXT("T.....T....."),
};

AVespGrille::AVespGrille()
{
	PrimaryActorTick.bCanEverTick = false;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;

	// Les modeles de base d'Unreal (un carre plat, un cube, un cone) : on les remplacera par les vrais decors plus tard
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plan(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));

	auto Creer = [this](const TCHAR* Nom, UStaticMesh* Modele) {
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Nom);
		C->SetupAttachment(Racine);
		C->SetStaticMesh(Modele);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};
	Sol = Creer(TEXT("Sol"), Plan.Object);
	Rochers = Creer(TEXT("Rochers"), Cube.Object);
	Arbres = Creer(TEXT("Arbres"), Cone.Object);
	Accessibles = Creer(TEXT("Accessibles"), Plan.Object);
	Survol = Creer(TEXT("Survol"), Plan.Object);
	Accessibles->SetCastShadow(false);
	Survol->SetCastShadow(false);
}

UMaterialInstanceDynamic* AVespGrille::Couleur(UInstancedStaticMeshComponent* Composant, FLinearColor Teinte)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, this);
	Materiau->SetVectorParameterValue(TEXT("Color"), Teinte);
	Composant->SetMaterial(0, Materiau);
	return Materiau;
}

// Un decor KayKit importe dans /Game/Decor : le premier dont le nom contient un des mots demandes
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

// L'echelle pour qu'un modele fasse "Hauteur" cm de haut
float AVespGrille::EchelleSur(UStaticMesh* Modele, float Hauteur) const
{
	const float H = Modele->GetBounds().BoxExtent.Z * 2.0f;
	return H > 1.0f ? Hauteur / H : 1.0f;
}

void AVespGrille::BeginPlay()
{
	Super::BeginPlay();
	// Les vrais decors (s'ils sont importes) : les sapins et les rochers KayKit, avec leurs couleurs d'origine
	if (UStaticMesh* Sapin = ModeleDuDecor({TEXT("tree_pine"), TEXT("tree")}))
	{
		Arbres->SetStaticMesh(Sapin);
		Arbres->EmptyOverrideMaterials();
		bVraisArbres = true;
	}
	else
	{
		Couleur(Arbres, FLinearColor(0.08f, 0.30f, 0.26f));
	}
	if (UStaticMesh* Rocher = ModeleDuDecor({TEXT("rock"), TEXT("gravestone"), TEXT("stone")}))
	{
		Rochers->SetStaticMesh(Rocher);
		Rochers->EmptyOverrideMaterials();
		bVraisRochers = true;
	}
	else
	{
		Couleur(Rochers, FLinearColor(0.25f, 0.24f, 0.30f));
	}
	Couleur(Sol, FLinearColor(0.10f, 0.16f, 0.12f));
	Couleur(Accessibles, FLinearColor(0.35f, 0.75f, 1.0f));
	Couleur(Survol, FLinearColor(1.0f, 1.0f, 1.0f));
	ConstruireCarte();
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
	return EstDansArene(Case) ? Occupants[Index(Case)].Get() : nullptr;
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
	// 2. Pas a pas : a chaque fois, la case voisine libre la plus proche de la cible
	TArray<FIntPoint> Chemin;
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
			if (!EstBloquee(Voisine) && !UniteSur(Voisine) && Distance[Index(Voisine)] >= 0 && Distance[Index(Voisine)] < MeilleureDistance)
			{
				MeilleureDistance = Distance[Index(Voisine)];
				Meilleure = Voisine;
			}
		}
		if (Meilleure == Ici)
		{
			break;		// bloque
		}
		Chemin.Add(Meilleure);
		Ici = Meilleure;
	}
	return Chemin;
}

void AVespGrille::ConstruireCarte()
{
	Carte.SetNum(Colonnes * Lignes);
	Occupants.Init(nullptr, Colonnes * Lignes);
	for (int32 L = 0; L < Lignes; L++)
	{
		for (int32 C = 0; C < Colonnes; C++)
		{
			const FIntPoint Case(C, L);
			Carte[Index(Case)] = CARTE_FORET[L][C];
			const FVector Centre = CentreDeCase(Case);
			// Le carre de sol, un peu plus petit que la case : on voit la grille entre les cases
			Sol->AddInstance(FTransform(FRotator::ZeroRotator, Centre, FVector(0.96f, 0.96f, 1.0f)), true);
			if (Carte[Index(Case)] == '#')
			{
				if (bVraisRochers)
				{
					const float E = EchelleSur(Rochers->GetStaticMesh(), 70.0f);
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
					const float E = EchelleSur(Arbres->GetStaticMesh(), 260.0f + 40.0f * ((C + L) % 3));
					Arbres->AddInstance(FTransform(FRotator(0, 53.0f * C, 0), Centre, FVector(E)), true);
				}
				else
				{
					Arbres->AddInstance(FTransform(FRotator::ZeroRotator, Centre + FVector(0, 0, 90), FVector(0.8f, 0.8f, 1.8f)), true);
				}
			}
		}
	}
}

FVector AVespGrille::CentreDeCase(FIntPoint Case) const
{
	// L'arene est centree sur la position de la grille ; la colonne va vers +Y (la droite a l'ecran), la ligne vers +X... vers le bas
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
	return !EstDansArene(Case) || Carte[Index(Case)] != '.';
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
		if (Pas[Index(Ici)] == PasMax)
		{
			continue;	// on ne peut pas aller plus loin
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
			if (EstDansArene(Voisine) && Pas[Index(Voisine)] == Pas[Index(Ici)] - 1)
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

void AVespGrille::AfficherSurvol(FIntPoint Case)
{
	Survol->ClearInstances();
	if (EstDansArene(Case))
	{
		Survol->AddInstance(FTransform(FRotator::ZeroRotator, CentreDeCase(Case) + FVector(0, 0, 2.5f), FVector(0.3f, 0.3f, 1.0f)), true);
	}
}
