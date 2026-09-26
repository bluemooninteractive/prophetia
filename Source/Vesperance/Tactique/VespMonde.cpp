#include "VespMonde.h"
#include "VespGrille.h"
#include "VespEffet.h"
#include "VespStyles.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// Les types de clairieres (les memes valeurs que EVespSalle)
namespace
{
	constexpr int32 REPOS = 2, MARCHAND = 3, BOSS = 5, DEPART = 6, TRESOR = 7;
}

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

	// Beaucoup d'instances : les petites choses sans ombre, et tout disparait au loin (plus rapide)
	auto Creer = [this](const TCHAR* Nom, UStaticMesh* Modele, bool bOmbre, float Distance) {
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Nom);
		C->SetupAttachment(Racine);
		C->SetStaticMesh(Modele);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCastShadow(bOmbre);
		C->SetCullDistances(0, Distance);
		return C;
	};
	EnvTerre = Creer(TEXT("EnvTerre"), Plan, false, 0);
	EnvClairieres = Creer(TEXT("EnvClairieres"), Cylindre, false, 0);
	EnvChemin = Creer(TEXT("EnvChemin"), Cylindre, false, 12000);
	EnvTaches = Creer(TEXT("EnvTaches"), Cylindre, false, 12000);
	EnvArbres = Creer(TEXT("EnvArbres"), Cone, true, 14000);
	EnvRochers = Creer(TEXT("EnvRochers"), Cube, true, 10000);
	EnvBuissons = Creer(TEXT("EnvBuissons"), Sphere, true, 9000);
	EnvHerbes = Creer(TEXT("EnvHerbes"), Cone, false, 5500);
	EnvFleurs = Creer(TEXT("EnvFleurs"), Sphere, false, 6000);
	EnvChampignons = Creer(TEXT("EnvChampignons"), Sphere, false, 7000);
	EnvTroncs = Creer(TEXT("EnvTroncs"), Cylindre, true, 9000);
	EnvTombes = Creer(TEXT("EnvTombes"), Cube, true, 9000);
	EnvSpecial = Creer(TEXT("EnvSpecial"), Cylindre, true, 14000);
	EnvBlocs = Creer(TEXT("EnvBlocs"), Cube, true, 11000);
	EnvLanternes = Creer(TEXT("EnvLanternes"), Cylindre, true, 12000);
	EnvAccessoires = Creer(TEXT("EnvAccessoires"), Cube, true, 12000);
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
	for (UInstancedStaticMeshComponent* C : {EnvTerre.Get(), EnvClairieres.Get(), EnvChemin.Get(), EnvTaches.Get(), EnvArbres.Get(), EnvRochers.Get(),
	                                         EnvBuissons.Get(), EnvHerbes.Get(), EnvFleurs.Get(), EnvChampignons.Get(), EnvTroncs.Get(), EnvTombes.Get(),
	                                         EnvSpecial.Get(), EnvBlocs.Get(), EnvLanternes.Get(), EnvAccessoires.Get()})
	{
		C->ClearInstances();
	}
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
	IntensitesBalises.Reset();
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

	// Les accessoires : un autel au depart, un feu de camp, la tente du marchand, un coffre
	const FVector C = Z.Centre;
	switch (Z.Type)
	{
		case DEPART:
			if (Autel)
			{
				EnvAccessoires->AddInstance(FTransform(FRotator(0, 90, 0), C + FVector(350, 0, 0), FVector(AVespGrille::EchelleSur(Autel, 180.0f, 160.0f))), true);
			}
			break;
		case REPOS:
		{
			// Des buches croisees, et un feu qui vacille
			for (int32 i = 0; i < 4; i++)
			{
				EnvAccessoires->AddInstance(FTransform(FRotator(90.0f, i * 45.0f, 0), C + FVector(0, 0, 12), FVector(0.18f, 0.18f, 1.1f)), true);
			}
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
		case MARCHAND:
			// Une tente : un socle et un toit en pente
			EnvAccessoires->AddInstance(FTransform(FRotator::ZeroRotator, C + FVector(420, 0, 60), FVector(2.4f, 2.4f, 1.2f)), true);
			EnvAccessoires->AddInstance(FTransform(FRotator(0, 0, 45.0f), C + FVector(420, 0, 175), FVector(2.8f, 1.75f, 1.75f)), true);	// le toit
			break;
		case TRESOR:
			EnvAccessoires->AddInstance(FTransform(FRotator(0, 20, 0), C + FVector(0, 0, 30), FVector(0.9f, 0.6f, 0.6f)), true);
			break;
		default: break;
	}
}

void AVespMonde::Construire(int32 LActe, const TArray<FVespZone>& LesZones, const TArray<FVespCouloir>& LesCouloirs)
{
	if (!bPret)
	{
		// La premiere fois : on cherche les decors importes (dans /Game/Decor), et on prepare les couleurs
		bPret = true;
		Sapin = AVespGrille::ModeleDuDecor({TEXT("tree_pine"), TEXT("tree")});
		ArbreMort = AVespGrille::ModeleDuDecor({TEXT("tree_dead")});
		if (!ArbreMort)
		{
			ArbreMort = Sapin;
		}
		bVraisArbres = Sapin != nullptr;
		if (!bVraisArbres)
		{
			CouleurArbres = Couleur(EnvArbres, FLinearColor(0.06f, 0.22f, 0.18f));
		}
		Lanterne = AVespGrille::ModeleDuDecor({TEXT("lantern")});
		Autel = AVespGrille::ModeleDuDecor({TEXT("shrine")});
		if (Lanterne)
		{
			EnvLanternes->SetStaticMesh(Lanterne);
		}
		else
		{
			Couleur(EnvLanternes, FLinearColor(0.15f, 0.1f, 0.06f));
		}
		if (UStaticMesh* Rocher = AVespGrille::ModeleDuDecor({TEXT("rock"), TEXT("stone")}))
		{
			EnvRochers->SetStaticMesh(Rocher);
			bVraisRochers = true;
		}
		else
		{
			Couleur(EnvRochers, FLinearColor(0.25f, 0.24f, 0.30f));
		}
		auto Modele = [](UInstancedStaticMeshComponent* C, std::initializer_list<const TCHAR*> Noms) {
			if (UStaticMesh* M = AVespGrille::ModeleDuDecor(Noms))
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
		CouleurClairieres = Couleur(EnvClairieres, FLinearColor::Black);
		CouleurChemin = Couleur(EnvChemin, FLinearColor::Black);
		CouleurTaches = Couleur(EnvTaches, FLinearColor::Black);
		CouleurBlocs = Couleur(EnvBlocs, FLinearColor::Black);
		Couleur(EnvAccessoires, FLinearColor(0.35f, 0.22f, 0.1f));
		SpecialMat = Couleur(nullptr, FLinearColor::Black);
		SpecialLumineux = Lumineux(nullptr, FLinearColor::Black);
	}

	Vider();
	Acte = FMath::Clamp(LActe, 1, 7);
	Zones = LesZones;
	Couloirs = LesCouloirs;
	const FVespStyleActe& S = StyleDeLActe(Acte);
	auto Teindre = [](UMaterialInstanceDynamic* M, const FLinearColor& C) { if (M) { M->SetVectorParameterValue(TEXT("Color"), C); } };
	Teindre(CouleurFleurs, S.Fleurs);
	Teindre(CouleurChampignons, S.Champignons);
	Teindre(CouleurHerbes, S.Herbes);
	Teindre(CouleurBuissons, S.Buissons);
	Teindre(CouleurTaches, S.Taches);
	Teindre(CouleurTerre, S.Terre);
	Teindre(CouleurChemin, S.Chemin);
	Teindre(CouleurClairieres, FMath::Lerp(S.Terre, S.Sol, 0.55f));
	if (bVraisArbres)
	{
		EnvArbres->SetStaticMesh(S.bArbresMorts ? ArbreMort.Get() : Sapin.Get());
	}

	// Les limites du monde
	FBox2D Limites(ForceInit);
	for (const FVespZone& Z : Zones)
	{
		Limites += FVector2D(Z.Centre.X, Z.Centre.Y);
	}
	Limites = Limites.ExpandBy(3200.0f);
	const float Z0 = GetActorLocation().Z;
	FRandomStream R(900 + Acte * 77);
	auto F = [&R](float A, float B) { return R.FRandRange(A, B); };
	auto Point = [&]() { return FVector(F(Limites.Min.X, Limites.Max.X), F(Limites.Min.Y, Limites.Max.Y), Z0); };
	auto RotF = [&R]() { return FRotator(0, R.FRandRange(0.0f, 360.0f), 0); };
	auto Pose = [Z0](const FVector& P, float Z = 0.0f) { return FVector(P.X, P.Y, Z0 + Z); };
	// "Du cote de la camera" (en bas de l'ecran) : la camera regarde vers +X, depuis -X. Un point juste sous le
	// praticable pourrait cacher AYLIS : on n'y met que des choses basses.
	auto CoteCamera = [&](const FVector& P) { return DistanceAuPraticable(P + FVector(520.0f, 0, 0)) < 0.0f; };
	// Dans une arene (le rectangle 12 x 8 cases au centre d'une clairiere de combat) : rien qui gene
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

	// ----- Le sol : la terre, les clairieres, les sentiers, des taches -----
	const FVector2D Taille = Limites.GetSize();
	const FVector2D Milieu = Limites.GetCenter();
	EnvTerre->AddInstance(FTransform(FRotator::ZeroRotator, Pose(FVector(Milieu.X, Milieu.Y, 0), -3.7f), FVector(Taille.X / 100.0f, Taille.Y / 100.0f, 1.0f)), true);
	for (const FVespZone& Z : Zones)
	{
		const float E = Z.Rayon * 2.1f / 100.0f;
		EnvClairieres->AddInstance(FTransform(FRotator::ZeroRotator, Pose(Z.Centre, -3.2f), FVector(E, E, 0.01f)), true);
	}
	for (const FVespCouloir& C : Couloirs)
	{
		const FVector A = Zones[C.A].Centre, B = Zones[C.B].Centre;
		const float Longueur = FVector::Dist2D(A, B);
		for (float D = 0.0f; D < Longueur; D += 70.0f)
		{
			const FVector P = FMath::Lerp(A, B, D / Longueur) + FVector(F(-25.0f, 25.0f), F(-25.0f, 25.0f), 0);
			const float E = DemiLargeurSentier * 2.0f / 100.0f * F(0.75f, 0.95f);
			EnvChemin->AddInstance(FTransform(RotF(), Pose(P, -2.6f), FVector(E, E * F(0.8f, 1.0f), 0.012f)), true);
		}
		// Des lanternes le long du sentier, de chaque cote en alternance
		const FVector Sens = (B - A).GetSafeNormal2D();
		const FVector Cote(-Sens.Y, Sens.X, 0);
		int32 n = 0;
		for (float D = 700.0f; D < Longueur - 700.0f; D += 1100.0f, n++)
		{
			const FVector P = FMath::Lerp(A, B, D / Longueur) + Cote * (n % 2 == 0 ? 1.0f : -1.0f) * (DemiLargeurSentier + 60.0f);
			if (Lanterne)
			{
				EnvLanternes->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(Lanterne, 115.0f, 70.0f))), true);
			}
			else
			{
				EnvLanternes->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, 55.0f), FVector(0.08f, 0.08f, 1.1f)), true);
			}
			AjouterLumiere(Pose(P, 110.0f), FLinearColor(1.0f, 0.65f, 0.3f), 1100.0f, 650.0f);
		}
	}
	for (int32 i = 0; i < 450; i++)
	{
		const float E = F(1.5f, 5.0f);
		EnvTaches->AddInstance(FTransform(RotF(), Pose(Point(), -3.4f), FVector(E, E * F(0.6f, 1.0f), 0.01f)), true);
	}

	// ----- La foret qui borde le couloir : dense au bord, puis de plus en plus clairsemee -----
	TArray<FVector> PiedsDArbres;
	const int32 EssaisArbres = FMath::RoundToInt(22000 * S.DensiteArbres);
	for (int32 i = 0; i < EssaisArbres; i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D < 110.0f || D > 2600.0f || R.FRand() > (D < 900.0f ? 0.55f : 0.18f))
		{
			continue;
		}
		const bool bBas = CoteCamera(P);
		if (bBas && D < 600.0f)
		{
			continue;		// devant la camera : pas d'arbre juste au bord
		}
		PiedsDArbres.Add(P);
		const float Hauteur = bBas ? F(200.0f, 300.0f) : F(280.0f, 420.0f) + FMath::Clamp(D / 2600.0f, 0.0f, 1.0f) * F(60.0f, 260.0f);
		if (bVraisArbres)
		{
			EnvArbres->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvArbres->GetStaticMesh(), Hauteur, 420.0f))), true);
		}
		else
		{
			EnvArbres->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, Hauteur / 2.0f), FVector(0.9f, 0.9f, Hauteur / 100.0f)), true);
		}
	}
	// Les buissons : au bord du couloir (ils sont bas, ils peuvent etre partout)
	for (int32 i = 0; i < FMath::RoundToInt(9000 * S.DensiteBuissons); i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D < 40.0f || D > 1400.0f || R.FRand() > 0.35f)
		{
			continue;
		}
		if (bVraisBuissons)
		{
			EnvBuissons->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvBuissons->GetStaticMesh(), F(45.0f, 95.0f), 170.0f))), true);
		}
		else
		{
			const float L = F(0.7f, 1.5f);
			EnvBuissons->AddInstance(FTransform(RotF(), Pose(P, 20.0f), FVector(L, L * F(0.7f, 1.0f), F(0.45f, 0.8f))), true);
		}
	}
	// Les rochers : semes au bord, et quelques-uns dans les clairieres (hors des arenes)
	auto Rocher = [&](const FVector& P, float Hauteur) {
		if (bVraisRochers)
		{
			EnvRochers->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvRochers->GetStaticMesh(), Hauteur, Hauteur * 1.8f))), true);
		}
		else
		{
			const float E = Hauteur / 100.0f;
			EnvRochers->AddInstance(FTransform(FRotator(F(-15.0f, 15.0f), F(0.0f, 360.0f), F(-15.0f, 15.0f)), Pose(P, Hauteur * 0.35f), FVector(E * 1.3f, E, E)), true);
		}
	};
	for (int32 i = 0; i < (Acte >= 5 ? 5000 : 3000); i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D > 0.0f && D < 1600.0f && R.FRand() < 0.25f)
		{
			Rocher(P, CoteCamera(P) ? F(25.0f, 70.0f) : F(30.0f, 170.0f));
		}
		else if (D < -80.0f && DistanceAuxSentiers(P) > DemiLargeurSentier * 0.7f && !DansUneArene(P) && R.FRand() < 0.03f)
		{
			Rocher(P, F(15.0f, 35.0f));		// un caillou dans une clairiere
		}
	}
	// Les herbes et les fleurs : au bord et dans les clairieres (pas au milieu du sentier ni dans les arenes)
	const int32 NombreHerbes = FMath::RoundToInt(16000 * S.DensiteHerbes);
	for (int32 i = 0; i < NombreHerbes; i++)
	{
		const FVector P = Point();
		const float D = DistanceAuPraticable(P);
		if (D > 1200.0f || DansUneArene(P) || DistanceAuxSentiers(P) < DemiLargeurSentier * 0.55f || (D > 500.0f && R.FRand() < 0.6f))
		{
			continue;
		}
		if (bVraiesHerbes)
		{
			EnvHerbes->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvHerbes->GetStaticMesh(), F(18.0f, 40.0f), 70.0f))), true);
		}
		else
		{
			const float Grand = Acte == 3 ? 2.0f : 1.0f;		// dans les marais, des roseaux
			for (int32 b = 0; b < 3; b++)
			{
				const float Haut = F(0.14f, 0.34f) * Grand;
				EnvHerbes->AddInstance(FTransform(FRotator(F(-22.0f, 22.0f), F(0.0f, 360.0f), F(-22.0f, 22.0f)),
				                                  Pose(P + FVector(F(-8.0f, 8.0f), F(-8.0f, 8.0f), 0), Haut * 50.0f), FVector(0.035f, 0.035f, Haut)), true);
			}
		}
		if (R.FRand() < S.PartFleurs)
		{
			const FVector Q = P + FVector(F(-30.0f, 30.0f), F(-30.0f, 30.0f), 0);
			if (bVraiesFleurs)
			{
				EnvFleurs->AddInstance(FTransform(RotF(), Pose(Q), FVector(AVespGrille::EchelleSur(EnvFleurs->GetStaticMesh(), F(15.0f, 30.0f), 40.0f))), true);
			}
			else
			{
				EnvFleurs->AddInstance(FTransform(FRotator::ZeroRotator, Pose(Q, F(8.0f, 22.0f)), FVector(F(0.04f, 0.075f))), true);
			}
		}
	}
	// Les champignons luminescents : en cercles au pied des arbres
	for (int32 i = 0; i < FMath::RoundToInt(160 * S.DensiteChampignons) && PiedsDArbres.Num() > 0; i++)
	{
		const FVector Pied = PiedsDArbres[R.RandRange(0, PiedsDArbres.Num() - 1)];
		const int32 Nombre = R.RandRange(3, 6);
		const float Depart = F(0.0f, 2.0f * PI);
		for (int32 k = 0; k < Nombre; k++)
		{
			const float A = Depart + k * 0.55f;
			const FVector P = Pied + FVector(FMath::Cos(A), FMath::Sin(A), 0) * F(55.0f, 85.0f);
			if (bVraisChampignons)
			{
				EnvChampignons->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvChampignons->GetStaticMesh(), F(12.0f, 26.0f), 30.0f))), true);
			}
			else
			{
				const float E = F(0.08f, 0.15f);
				EnvChampignons->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, E * 60.0f), FVector(E, E, E * 0.45f)), true);
			}
		}
	}
	// Des points au bord du couloir (pas dedans), pour le reste du decor
	auto PointAuBord = [&](float DMin, float DMax) {
		for (int32 k = 0; k < 60; k++)
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
	// Les troncs couches
	for (int32 i = 0; i < (Acte >= 6 ? 15 : 60); i++)
	{
		const FVector P = PointAuBord(150.0f, 1500.0f);
		if (bVraisTroncs)
		{
			EnvTroncs->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvTroncs->GetStaticMesh(), F(30.0f, 60.0f), 260.0f))), true);
		}
		else
		{
			EnvTroncs->AddInstance(FTransform(FRotator(90.0f, F(0.0f, 360.0f), 0), Pose(P, 17.0f), FVector(0.35f, 0.35f, F(1.6f, 2.6f))), true);
		}
	}
	// Les tombes du Bois des Pendus, par petits groupes
	if (S.bTombes)
	{
		for (int32 g = 0; g < 30; g++)
		{
			const FVector Milieu3 = PointAuBord(100.0f, 1300.0f);
			const int32 Nombre = R.RandRange(3, 6);
			for (int32 k = 0; k < Nombre; k++)
			{
				const FVector P = Milieu3 + FVector(F(-160.0f, 160.0f), F(-160.0f, 160.0f), 0);
				if (DistanceAuPraticable(P) < 60.0f)
				{
					continue;
				}
				if (bVraiesTombes)
				{
					EnvTombes->AddInstance(FTransform(RotF(), Pose(P), FVector(AVespGrille::EchelleSur(EnvTombes->GetStaticMesh(), F(50.0f, 80.0f), 80.0f))), true);
				}
				else
				{
					EnvTombes->AddInstance(FTransform(FRotator(F(-8.0f, 8.0f), F(-12.0f, 12.0f), F(-8.0f, 8.0f)), Pose(P, 32.0f), FVector(0.12f, 0.45f, 0.65f)), true);
				}
			}
		}
	}

	// ----- Le decor special de l'acte -----
	switch (S.Special)
	{
		case 3:		// les marais : des mares d'eau noire
			EnvSpecial->SetStaticMesh(Cylindre);
			SpecialMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.012f, 0.04f, 0.05f));
			EnvSpecial->SetMaterial(0, SpecialMat);
			for (int32 i = 0; i < 140; i++)
			{
				const FVector P = PointAuBord(250.0f, 2200.0f);
				const float E = F(2.0f, 6.0f);
				EnvSpecial->AddInstance(FTransform(RotF(), Pose(P, -2.9f), FVector(E, E * F(0.5f, 0.9f), 0.01f)), true);
			}
			break;
		case 4:		// la forteresse : des colonnes le long des sentiers, et des blocs tombes
			EnvSpecial->SetStaticMesh(Cylindre);
			SpecialMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.3f, 0.28f, 0.25f));
			EnvSpecial->SetMaterial(0, SpecialMat);
			for (const FVespCouloir& C : Couloirs)
			{
				const FVector A = Zones[C.A].Centre, B = Zones[C.B].Centre;
				const float Longueur = FVector::Dist2D(A, B);
				const FVector Sens = (B - A).GetSafeNormal2D();
				const FVector Cote(-Sens.Y, Sens.X, 0);
				for (float D = 400.0f; D < Longueur - 400.0f; D += 420.0f)
				{
					for (int32 s = -1; s <= 1; s += 2)
					{
						const FVector P = FMath::Lerp(A, B, D / Longueur) + Cote * s * (DemiLargeurSentier + 110.0f);
						if (DistanceAuPraticable(P) < 40.0f)
						{
							continue;
						}
						const float Haut = CoteCamera(P) ? F(40.0f, 120.0f) : (R.FRand() < 0.3f ? F(60.0f, 160.0f) : F(320.0f, 480.0f));
						EnvSpecial->AddInstance(FTransform(FRotator::ZeroRotator, Pose(P, Haut / 2.0f), FVector(0.5f, 0.5f, Haut / 100.0f)), true);
					}
				}
			}
			EnvBlocs->SetStaticMesh(Cube);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.26f, 0.24f, 0.21f));
			for (int32 i = 0; i < 260; i++)
			{
				const FVector P = PointAuBord(80.0f, 1800.0f);
				const FVector E(F(0.4f, 1.1f), F(0.4f, 0.9f), F(0.3f, 0.6f));
				EnvBlocs->AddInstance(FTransform(FRotator(F(-12.0f, 12.0f), F(0.0f, 360.0f), F(-12.0f, 12.0f)), Pose(P, E.Z * 40.0f), E), true);
			}
			break;
		case 5:		// le col : des pics de glace qui brillent, et des congeres
			EnvSpecial->SetStaticMesh(Cone);
			SpecialLumineux->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.4f, 0.65f, 1.0f) * 0.8f);
			EnvSpecial->SetMaterial(0, SpecialLumineux);
			for (int32 g = 0; g < 90; g++)
			{
				const FVector Milieu4 = PointAuBord(200.0f, 2200.0f);
				for (int32 k = 0; k < R.RandRange(3, 6); k++)
				{
					const FVector P = Milieu4 + FVector(F(-70.0f, 70.0f), F(-70.0f, 70.0f), 0);
					const float Haut = CoteCamera(P) ? F(0.4f, 0.9f) : F(0.8f, 2.6f);
					EnvSpecial->AddInstance(FTransform(FRotator(F(-20.0f, 20.0f), F(0.0f, 360.0f), F(-20.0f, 20.0f)), Pose(P, Haut * 40.0f),
					                                   FVector(F(0.2f, 0.45f), F(0.2f, 0.45f), Haut)), true);
				}
			}
			EnvBlocs->SetStaticMesh(Sphere);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.5f, 0.55f, 0.63f));
			for (int32 i = 0; i < 220; i++)
			{
				const FVector P = PointAuBord(60.0f, 2000.0f);
				const float L = F(1.5f, 4.0f);
				EnvBlocs->AddInstance(FTransform(RotF(), Pose(P), FVector(L, L * F(0.5f, 0.9f), F(0.3f, 0.6f))), true);
			}
			break;
		case 6:		// les terres de cendre : des coulees de lave et de l'obsidienne
			EnvSpecial->SetStaticMesh(Cylindre);
			SpecialLumineux->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.3f, 0.03f) * 3.0f);
			EnvSpecial->SetMaterial(0, SpecialLumineux);
			for (int32 i = 0; i < 160; i++)
			{
				const FVector P = PointAuBord(250.0f, 2400.0f);
				const float E = F(1.2f, 4.0f);
				EnvSpecial->AddInstance(FTransform(RotF(), Pose(P, -2.0f), FVector(E, E * F(0.4f, 0.9f), 0.01f)), true);
			}
			EnvBlocs->SetStaticMesh(Cube);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.025f, 0.03f));
			for (int32 i = 0; i < 380; i++)
			{
				const FVector P = PointAuBord(70.0f, 2000.0f);
				const float Haut = CoteCamera(P) ? F(0.3f, 0.7f) : F(0.6f, 2.8f);
				EnvBlocs->AddInstance(FTransform(FRotator(F(-25.0f, 25.0f), F(0.0f, 360.0f), F(-25.0f, 25.0f)), Pose(P, Haut * 30.0f),
				                                 FVector(F(0.3f, 0.8f), F(0.3f, 0.8f), Haut)), true);
			}
			break;
		case 7:		// Karn : des cristaux du Voile et des colonnes brisees
			EnvSpecial->SetStaticMesh(Cube);
			SpecialLumineux->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.3f, 1.0f) * 2.0f);
			EnvSpecial->SetMaterial(0, SpecialLumineux);
			for (int32 g = 0; g < 110; g++)
			{
				const FVector Milieu5 = PointAuBord(150.0f, 2200.0f);
				for (int32 k = 0; k < R.RandRange(3, 7); k++)
				{
					const FVector P = Milieu5 + FVector(F(-80.0f, 80.0f), F(-80.0f, 80.0f), 0);
					const float Haut = CoteCamera(P) ? F(0.4f, 0.8f) : F(0.8f, 2.6f);
					EnvSpecial->AddInstance(FTransform(FRotator(F(-25.0f, 25.0f), F(0.0f, 360.0f), F(-25.0f, 25.0f)), Pose(P, Haut * 45.0f),
					                                   FVector(F(0.15f, 0.32f), F(0.15f, 0.32f), Haut)), true);
				}
			}
			EnvBlocs->SetStaticMesh(Cylindre);
			CouleurBlocs->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.17f, 0.26f));
			for (int32 i = 0; i < 160; i++)
			{
				const FVector P = PointAuBord(120.0f, 2000.0f);
				const float Haut = CoteCamera(P) ? F(40.0f, 100.0f) : F(120.0f, 500.0f);
				EnvBlocs->AddInstance(FTransform(FRotator(F(-6.0f, 6.0f), 0, F(-6.0f, 6.0f)), Pose(P, Haut / 2.0f), FVector(0.5f, 0.5f, Haut / 100.0f)), true);
			}
			break;
		default:
			// La tente du marchand utilise EnvSpecial : un cone de toile
			EnvSpecial->SetStaticMesh(Cone);
			SpecialMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.12f, 0.08f));
			EnvSpecial->SetMaterial(0, SpecialMat);
			break;
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
	// Les orbes flottent doucement, les feux de camp vacillent
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
}
