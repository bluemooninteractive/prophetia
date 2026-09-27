#include "VespEffet.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AVespEffet::AVespEffet()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
}

// Le materiau lumineux : on le fabrique une seule fois, par le code (dans l'editeur).
// Il est "additif" : sa couleur s'ajoute a l'image, comme une lumiere. Noir = invisible.
UMaterialInterface* AVespEffet::MateriauLumineux()
{
	static TWeakObjectPtr<UMaterialInterface> Deja;
	if (Deja.IsValid())
	{
		return Deja.Get();
	}
	UMaterialInterface* Resultat = nullptr;
#if WITH_EDITORONLY_DATA
	UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), TEXT("VespLumineux"), RF_Transient);
	M->SetShadingModel(MSM_Unlit);
	M->BlendMode = BLEND_Additive;
	M->bUsedWithInstancedStaticMeshes = true;
	UMaterialExpressionVectorParameter* Parametre = NewObject<UMaterialExpressionVectorParameter>(M);
	Parametre->ParameterName = TEXT("Color");
	Parametre->DefaultValue = FLinearColor::White;
	M->GetExpressionCollection().AddExpression(Parametre);
	M->GetEditorOnlyData()->EmissiveColor.Expression = Parametre;
	M->PostEditChange();			// compile le materiau
	M->AddToRoot();					// le garder tant que le jeu tourne
	Resultat = M;
#else
	Resultat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
#endif
	Deja = Resultat;
	return Resultat;
}

// ===================== Les effets magiques (Free_Magic, Niagara) =====================

UNiagaraSystem* AVespEffet::Magie(const TCHAR* Nom)
{
	static TMap<FString, TWeakObjectPtr<UNiagaraSystem>> Deja;
	const FString Cle(Nom);
	if (TWeakObjectPtr<UNiagaraSystem>* S = Deja.Find(Cle))
	{
		if (S->IsValid())
		{
			return S->Get();
		}
	}
	UNiagaraSystem* S = LoadObject<UNiagaraSystem>(nullptr, *FString::Printf(TEXT("/Game/Free_Magic/VFX_Niagara/%s.%s"), Nom, Nom), nullptr, LOAD_NoWarn | LOAD_Quiet);
	Deja.Add(Cle, S);
	return S;
}

void AVespEffet::JouerMagie(UWorld* Monde, const TCHAR* Nom, const FVector& Position, const FRotator& Rotation, float Echelle)
{
	if (UNiagaraSystem* S = Magie(Nom))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Monde, S, Position, Rotation, FVector(Echelle), true);
	}
}

void AVespEffet::Ajouter(const FVector& Position, const FVector& Vitesse, float VieMax, float TailleDebut, float TailleFin,
                         float Gravite, float Retard, float Frottement, FVector Etirement)
{
	FGrain G;
	G.Mesh = NewObject<UStaticMeshComponent>(this);
	G.Mesh->SetStaticMesh(Sphere);
	G.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	G.Mesh->SetCastShadow(false);
	G.Mesh->SetupAttachment(RootComponent);
	G.Mesh->RegisterComponent();
	G.Mesh->SetWorldLocation(Position);
	G.Mesh->SetWorldScale3D(FVector(Retard > 0.0f ? 0.0f : TailleDebut) * Etirement);
	G.Materiau = UMaterialInstanceDynamic::Create(MateriauLumineux(), this);
	G.Materiau->SetVectorParameterValue(TEXT("Color"), Retard > 0.0f ? FLinearColor::Black : Couleur * Intensite);
	G.Mesh->SetMaterial(0, G.Materiau);
	G.Vitesse = Vitesse;
	G.VieMax = VieMax;
	G.TailleDebut = TailleDebut;
	G.TailleFin = TailleFin;
	G.Gravite = Gravite;
	G.Retard = Retard;
	G.Frottement = Frottement;
	G.Etirement = Etirement;
	Grains.Add(G);
}

void AVespEffet::Eclairer(float Force, float Rayon, float Duree, float Retard, FVector Vitesse)
{
	Lumiere = NewObject<UPointLightComponent>(this);
	Lumiere->SetupAttachment(RootComponent);
	Lumiere->RegisterComponent();
	Lumiere->SetWorldLocation(GetActorLocation());
	Lumiere->SetLightColor(Couleur);
	Lumiere->SetIntensity(0.0f);
	Lumiere->SetAttenuationRadius(Rayon);
	Lumiere->SetCastShadows(false);
	LumiereMax = Force;
	LumiereDuree = Duree;
	LumiereRetard = Retard;
	LumiereVitesse = Vitesse;
}

void AVespEffet::Jouer(UWorld* Monde, EVespEffet Type, FVector Position, FVector Direction, FLinearColor LaCouleur, float Retard, FVector Arrivee)
{
	if (!Monde)
	{
		return;
	}
	AVespEffet* E = Monde->SpawnActor<AVespEffet>(Position, FRotator::ZeroRotator);
	if (!E)
	{
		return;
	}
	E->Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	E->Couleur = LaCouleur;
	const FVector Dir = Direction.GetSafeNormal();
	auto Hasard = [](float A, float B) { return FMath::FRandRange(A, B); };

	// Les effets magiques du pack Free_Magic, par-dessus nos grains de lumiere
	auto AjouterMagie = [E](const TCHAR* Nom, const FVector& P, const FRotator& R, float Echelle, float LeRetard,
	                        const FVector& Vitesse = FVector::ZeroVector, float Duree = 0.0f) {
		UNiagaraSystem* S = Magie(Nom);
		if (!S)
		{
			return false;
		}
		FMagieEnCours M;
		M.Systeme = S;
		M.Position = P;
		M.Rotation = R;
		M.Echelle = Echelle;
		M.Retard = LeRetard;
		M.Vitesse = Vitesse;
		M.Duree = Duree;
		E->Magies.Add(M);
		return true;
	};
	const bool bBleu = LaCouleur.B > LaCouleur.R;									// les coups d'AYLIS
	const bool bVert = LaCouleur.G > LaCouleur.R && LaCouleur.G > LaCouleur.B;		// le poison
	switch (Type)
	{
		case EVespEffet::Impact: AjouterMagie(TEXT("NS_Free_Magic_Hit1"), Position, Dir.Rotation(), 0.7f, Retard); break;
		case EVespEffet::Critique: AjouterMagie(TEXT("NS_Free_Magic_Hit2"), Position, Dir.Rotation(), 1.1f, Retard); break;
		case EVespEffet::Trainee:
			if (AjouterMagie(bBleu ? TEXT("NS_Free_Magic_Slash") : TEXT("NS_Free_Magic_Slash2"), Position + Dir * 40.0f, Dir.Rotation(), 0.8f, Retard))
			{
				return;		// la taillade du pack remplace nos grains
			}
			break;
		case EVespEffet::Projectile:
		{
			const float Duree = 0.3f;
			const FVector Vitesse = (Arrivee - Position) / Duree;
			if (AjouterMagie(bVert ? TEXT("NS_Free_Magic_Projectile2") : TEXT("NS_Free_Magic_Projectile1"), Position, Vitesse.Rotation(), 0.7f, Retard, Vitesse, Duree))
			{
				E->Eclairer(2500.0f, 350.0f, Duree, Retard, Vitesse);		// sa lumiere l'accompagne
				return;
			}
			break;
		}
		case EVespEffet::Soin: AjouterMagie(TEXT("NS_Free_Magic_Buff"), Position, FRotator::ZeroRotator, 0.8f, Retard); break;
		case EVespEffet::Onde: AjouterMagie(TEXT("NS_Free_Magic_Circle2"), Position, FRotator::ZeroRotator, 0.4f, Retard); break;
		case EVespEffet::Etincelles: AjouterMagie(TEXT("NS_Free_Magic_Circle1"), Position, FRotator::ZeroRotator, 0.7f, Retard); break;
		default: break;
	}

	switch (Type)
	{
		case EVespEffet::Impact:
		case EVespEffet::Critique:
		{
			const bool bCrit = Type == EVespEffet::Critique;
			const int32 Nombre = bCrit ? 26 : 14;
			for (int32 i = 0; i < Nombre; i++)
			{
				const FVector V = (Dir * 0.7f + FMath::VRand() * 0.8f + FVector(0, 0, 0.3f)).GetSafeNormal() * Hasard(450.0f, bCrit ? 1300.0f : 850.0f);
				E->Ajouter(Position, V, Hasard(0.25f, 0.5f), Hasard(0.05f, bCrit ? 0.12f : 0.08f), 0.0f, 1400.0f, Retard, 3.0f, FVector(3.0f, 1.0f, 1.0f));
			}
			// Un eclat au centre, qui grossit puis s'eteint
			E->Ajouter(Position, FVector::ZeroVector, 0.14f, 0.2f, bCrit ? 1.1f : 0.6f, 0.0f, Retard);
			if (bCrit)
			{
				// Un anneau d'etincelles, a l'horizontale
				for (int32 i = 0; i < 16; i++)
				{
					const float A = i * 2.0f * PI / 16;
					E->Ajouter(Position, FVector(FMath::Cos(A), FMath::Sin(A), 0) * 900.0f, 0.3f, 0.08f, 0.0f, 0.0f, Retard, 4.0f, FVector(3.0f, 1.0f, 1.0f));
				}
			}
			E->Eclairer(bCrit ? 14000.0f : 5000.0f, bCrit ? 700.0f : 450.0f, bCrit ? 0.3f : 0.18f, Retard);
			break;
		}
		case EVespEffet::Trainee:
		{
			// L'arc de la lame : des grains poses sur un demi-cercle devant l'attaquant, l'un apres l'autre
			const FVector Cote = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
			for (int32 i = 0; i < 14; i++)
			{
				const float T = i / 13.0f;
				const float A = FMath::Lerp(-1.2f, 1.2f, T);
				const FVector P = Position + (Dir * FMath::Cos(A) + Cote * FMath::Sin(A)) * 75.0f + FVector(0, 0, FMath::Lerp(40.0f, -30.0f, T));
				const FVector Tangente = (-Dir * FMath::Sin(A) + Cote * FMath::Cos(A)) * 250.0f;
				E->Ajouter(P, Tangente, 0.2f, 0.11f, 0.02f, 0.0f, Retard + T * 0.08f, 0.0f, FVector(2.5f, 1.0f, 1.0f));
			}
			break;
		}
		case EVespEffet::Projectile:
		{
			const float Duree = 0.3f;
			const FVector V = (Arrivee - Position) / Duree;
			E->Ajouter(Position, V, Duree, 0.18f, 0.14f, 0.0f, Retard, 0.0f, FVector(2.5f, 1.0f, 1.0f));
			for (int32 i = 1; i <= 8; i++)			// la queue
			{
				E->Ajouter(Position, V, Duree, 0.12f - i * 0.011f, 0.0f, 0.0f, Retard + i * 0.02f, 0.0f, FVector(2.0f, 1.0f, 1.0f));
			}
			E->Eclairer(2500.0f, 350.0f, Duree, Retard, V);
			break;
		}
		case EVespEffet::Poussiere:
		{
			E->Intensite = 0.5f;
			for (int32 i = 0; i < 7; i++)
			{
				const float A = Hasard(0.0f, 2.0f * PI);
				const FVector V = FVector(FMath::Cos(A), FMath::Sin(A), 0) * Hasard(70.0f, 160.0f) + FVector(0, 0, Hasard(20.0f, 70.0f));
				E->Ajouter(Position + FVector(0, 0, 8), V, Hasard(0.45f, 0.8f), 0.12f, 0.35f, -30.0f, Retard, 3.0f);
			}
			break;
		}
		case EVespEffet::Soin:
		case EVespEffet::Poison:
		{
			const bool bSoin = Type == EVespEffet::Soin;
			for (int32 i = 0; i < (bSoin ? 18 : 10); i++)
			{
				const float A = Hasard(0.0f, 2.0f * PI);
				const FVector P = Position + FVector(FMath::Cos(A) * 45.0f, FMath::Sin(A) * 45.0f, Hasard(10.0f, 120.0f));
				E->Ajouter(P, FVector(0, 0, Hasard(90.0f, 200.0f)), Hasard(0.6f, 1.1f), Hasard(0.05f, 0.09f), 0.0f, 0.0f, Retard + Hasard(0.0f, 0.3f));
			}
			E->Eclairer(bSoin ? 3000.0f : 1500.0f, 400.0f, 0.7f, Retard);
			break;
		}
		case EVespEffet::Mort:
		{
			E->Intensite = 4.0f;
			for (int32 i = 0; i < 30; i++)
			{
				const FVector P = Position + FVector(Hasard(-40.0f, 40.0f), Hasard(-40.0f, 40.0f), Hasard(20.0f, 160.0f));
				E->Ajouter(P, FVector(Hasard(-30.0f, 30.0f), Hasard(-30.0f, 30.0f), Hasard(60.0f, 220.0f)), Hasard(0.8f, 1.5f), Hasard(0.06f, 0.13f), 0.0f,
				           -40.0f, Retard + Hasard(0.0f, 0.4f));
			}
			E->Eclairer(4000.0f, 500.0f, 1.0f, Retard);
			break;
		}
		case EVespEffet::Onde:
		{
			for (int32 i = 0; i < 28; i++)
			{
				const float A = i * 2.0f * PI / 28;
				const FVector Vers(FMath::Cos(A), FMath::Sin(A), 0);
				E->Ajouter(Position + Vers * 20.0f, Vers * 700.0f, 0.45f, 0.14f, 0.02f, 0.0f, Retard, 2.0f, FVector(2.5f, 1.0f, 0.6f));
			}
			E->Eclairer(9000.0f, 800.0f, 0.35f, Retard);
			break;
		}
		default:		// Etincelles
		{
			for (int32 i = 0; i < 22; i++)
			{
				const FVector V = (FVector(0, 0, 1.2f) + FMath::VRand()).GetSafeNormal() * Hasard(250.0f, 600.0f);
				E->Ajouter(Position, V, Hasard(0.5f, 0.9f), Hasard(0.05f, 0.09f), 0.0f, 700.0f, Retard, 1.5f, FVector(2.0f, 1.0f, 1.0f));
			}
			E->Eclairer(4000.0f, 450.0f, 0.5f, Retard);
			break;
		}
	}
}

void AVespEffet::Tick(float Secondes)
{
	Super::Tick(Secondes);
	Temps += Secondes;
	bool bFini = true;
	for (FGrain& G : Grains)
	{
		if (G.Retard > 0.0f)
		{
			G.Retard -= Secondes;
			bFini = false;
			continue;
		}
		if (G.Vie >= G.VieMax)
		{
			continue;
		}
		G.Vie += Secondes;
		const float T = FMath::Clamp(G.Vie / G.VieMax, 0.0f, 1.0f);
		G.Vitesse.Z -= G.Gravite * Secondes;
		G.Vitesse *= FMath::Max(0.0f, 1.0f - G.Frottement * Secondes);
		G.Mesh->SetWorldLocation(G.Mesh->GetComponentLocation() + G.Vitesse * Secondes);
		if (!G.Vitesse.IsNearlyZero())
		{
			G.Mesh->SetWorldRotation(G.Vitesse.Rotation());
		}
		G.Mesh->SetWorldScale3D(FVector(FMath::Lerp(G.TailleDebut, G.TailleFin, T)) * G.Etirement);
		G.Materiau->SetVectorParameterValue(TEXT("Color"), Couleur * Intensite * FMath::Pow(1.0f - T, 1.5f));
		if (T >= 1.0f)
		{
			G.Mesh->SetVisibility(false);
		}
		else
		{
			bFini = false;
		}
	}
	if (Lumiere)
	{
		if (LumiereRetard > 0.0f)
		{
			LumiereRetard -= Secondes;
			bFini = false;
		}
		else if (LumiereTemps < LumiereDuree)
		{
			LumiereTemps += Secondes;
			const float T = FMath::Clamp(LumiereTemps / LumiereDuree, 0.0f, 1.0f);
			Lumiere->SetIntensity(LumiereMax * (1.0f - T) * (1.0f - T));
			Lumiere->SetWorldLocation(Lumiere->GetComponentLocation() + LumiereVitesse * Secondes);
			bFini = false;
		}
		else
		{
			Lumiere->SetIntensity(0.0f);
		}
	}
	// Les effets magiques : ils attendent leur tour, puis apparaissent (et volent, pour un projectile)
	for (FMagieEnCours& M : Magies)
	{
		if (!M.bLancee)
		{
			M.Retard -= Secondes;
			if (M.Retard > 0.0f)
			{
				bFini = false;
				continue;
			}
			M.bLancee = true;
			const bool bVole = !M.Vitesse.IsNearlyZero();
			M.Composant = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), M.Systeme, M.Position, M.Rotation, FVector(M.Echelle), !bVole);
		}
		if (M.Composant.IsValid() && !M.Vitesse.IsNearlyZero())
		{
			M.Duree -= Secondes;
			if (M.Duree > 0.0f)
			{
				M.Position += M.Vitesse * Secondes;
				M.Composant->SetWorldLocation(M.Position);
				bFini = false;
			}
			else
			{
				M.Composant->SetAutoDestroy(true);		// il finit ses particules, puis disparait
				M.Composant->Deactivate();
				M.Composant = nullptr;
			}
		}
	}
	if (bFini || Temps > 4.0f)
	{
		Destroy();
	}
}
