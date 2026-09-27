// Le sol vivant du monde (une partie d'AVespMonde).
//
// Un materiau fabrique par le code : trois matieres (herbe, terre, et celle de l'acte) vues d'en haut, repetees
// a deux echelles (pour casser la repetition), melangees par une "carte" peinte ici : la terre battue au milieu
// des sentiers et des clairieres, des plaques de la matiere de l'acte, et de grandes nappes de lumiere.

#include "VespMonde.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#if WITH_EDITORONLY_DATA
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#endif

UMaterialInterface* AVespMonde::MateriauDuSol()
{
	static TWeakObjectPtr<UMaterialInterface> Deja;
	if (Deja.IsValid())
	{
		return Deja.Get();
	}
#if WITH_EDITORONLY_DATA
	UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), TEXT("VespSol"), RF_Transient);
	auto Ajouter = [M](auto* E) { M->GetExpressionCollection().AddExpression(E); return E; };
	UTexture* Blanche = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	// La position dans le monde, vue d'en haut
	UMaterialExpressionWorldPosition* Monde = Ajouter(NewObject<UMaterialExpressionWorldPosition>(M));
	UMaterialExpressionComponentMask* XY = Ajouter(NewObject<UMaterialExpressionComponentMask>(M));
	XY->Input.Connect(0, Monde);
	XY->R = true; XY->G = true; XY->B = false; XY->A = false;
	auto Echelle = [&](float Metres) -> UMaterialExpression* {
		UMaterialExpressionDivide* D = Ajouter(NewObject<UMaterialExpressionDivide>(M));
		D->A.Connect(0, XY);
		D->ConstB = Metres * 100.0f;
		return D;
	};
	auto Texture = [&](const TCHAR* Nom, UMaterialExpression* Uv, bool bLineaire) {
		UMaterialExpressionTextureSampleParameter2D* T = Ajouter(NewObject<UMaterialExpressionTextureSampleParameter2D>(M));
		T->ParameterName = Nom;
		T->Texture = Blanche;
		T->SamplerType = bLineaire ? SAMPLERTYPE_LinearColor : SAMPLERTYPE_Color;
		T->Coordinates.Connect(0, Uv);
		return T;
	};
	auto Couleur = [&](const TCHAR* Nom) {
		UMaterialExpressionVectorParameter* V = Ajouter(NewObject<UMaterialExpressionVectorParameter>(M));
		V->ParameterName = Nom;
		V->DefaultValue = FLinearColor::White;
		return V;
	};
	auto Fois = [&](UMaterialExpression* A, UMaterialExpression* B) -> UMaterialExpression* {
		UMaterialExpressionMultiply* F = Ajouter(NewObject<UMaterialExpressionMultiply>(M));
		F->A.Connect(0, A);
		F->B.Connect(0, B);
		return F;
	};
	auto Melange = [&](UMaterialExpression* A, UMaterialExpression* B, UMaterialExpression* Alpha, int32 SortieAlpha) -> UMaterialExpression* {
		UMaterialExpressionLinearInterpolate* L = Ajouter(NewObject<UMaterialExpressionLinearInterpolate>(M));
		L->A.Connect(0, A);
		L->B.Connect(0, B);
		L->Alpha.Connect(SortieAlpha, Alpha);
		return L;
	};
	// L'herbe, a deux echelles ; la terre ; la matiere de l'acte
	UMaterialExpressionLinearInterpolate* HerbeDouble = Ajouter(NewObject<UMaterialExpressionLinearInterpolate>(M));
	HerbeDouble->A.Connect(0, Texture(TEXT("Herbe"), Echelle(4.5f), false));
	HerbeDouble->B.Connect(0, Texture(TEXT("HerbeLoin"), Echelle(19.0f), false));
	HerbeDouble->ConstAlpha = 0.45f;
	UMaterialExpression* Herbe = Fois(HerbeDouble, Couleur(TEXT("TeinteHerbe")));
	UMaterialExpression* Terre = Fois(Texture(TEXT("Terre"), Echelle(3.8f), false), Couleur(TEXT("TeinteTerre")));
	UMaterialExpression* Autre = Fois(Texture(TEXT("Autre"), Echelle(5.0f), false), Couleur(TEXT("TeinteAutre")));
	// La carte : (position - coin) / taille
	UMaterialExpressionVectorParameter* Coin = Couleur(TEXT("Coin"));
	UMaterialExpressionComponentMask* CoinXY = Ajouter(NewObject<UMaterialExpressionComponentMask>(M));
	CoinXY->Input.Connect(0, Coin);
	CoinXY->R = true; CoinXY->G = true; CoinXY->B = false; CoinXY->A = false;
	UMaterialExpressionComponentMask* TailleXY = Ajouter(NewObject<UMaterialExpressionComponentMask>(M));
	TailleXY->Input.Connect(0, Coin);
	TailleXY->R = false; TailleXY->G = false; TailleXY->B = true; TailleXY->A = true;
	UMaterialExpressionSubtract* Decale = Ajouter(NewObject<UMaterialExpressionSubtract>(M));
	Decale->A.Connect(0, XY);
	Decale->B.Connect(0, CoinXY);
	UMaterialExpressionDivide* UvCarte = Ajouter(NewObject<UMaterialExpressionDivide>(M));
	UvCarte->A.Connect(0, Decale);
	UvCarte->B.Connect(0, TailleXY);
	UMaterialExpressionTextureSampleParameter2D* Carte = Texture(TEXT("Carte"), UvCarte, false);
	// Le melange : l'herbe, puis la terre (rouge de la carte), puis la matiere de l'acte (vert), puis la lumiere (bleu)
	UMaterialExpression* Sol = Melange(Melange(Herbe, Terre, Carte, 1), Autre, Carte, 2);
	UMaterialExpressionLinearInterpolate* Lumiere = Ajouter(NewObject<UMaterialExpressionLinearInterpolate>(M));
	Lumiere->ConstA = 0.72f;
	Lumiere->ConstB = 1.18f;
	Lumiere->Alpha.Connect(3, Carte);
	M->GetEditorOnlyData()->BaseColor.Connect(0, Fois(Sol, Lumiere));
	UMaterialExpressionConstant* Rugueux = Ajouter(NewObject<UMaterialExpressionConstant>(M));
	Rugueux->R = 0.92f;
	M->GetEditorOnlyData()->Roughness.Connect(0, Rugueux);
	UMaterialExpressionConstant* Speculaire = Ajouter(NewObject<UMaterialExpressionConstant>(M));
	Speculaire->R = 0.12f;
	M->GetEditorOnlyData()->Specular.Connect(0, Speculaire);
	M->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, true);
	M->PostEditChange();
	M->AddToRoot();
	Deja = M;
	return M;
#else
	return nullptr;
#endif
}

// Un bruit doux (des vallonnements aleatoires, toujours les memes pour une graine)
static float Bruit(float X, float Y, int32 Graine)
{
	auto Hache = [Graine](int32 I, int32 J) {
		uint32 H = (uint32)I * 374761393u + (uint32)J * 668265263u + (uint32)Graine * 2246822519u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return ((H ^ (H >> 16)) & 0xFFFF) / 65535.0f;
	};
	const int32 I = FMath::FloorToInt(X), J = FMath::FloorToInt(Y);
	const float Fx = X - I, Fy = Y - J;
	const float Sx = Fx * Fx * (3.0f - 2.0f * Fx), Sy = Fy * Fy * (3.0f - 2.0f * Fy);
	const float A = FMath::Lerp(Hache(I, J), Hache(I + 1, J), Sx);
	const float B = FMath::Lerp(Hache(I, J + 1), Hache(I + 1, J + 1), Sx);
	return FMath::Lerp(A, B, Sy);
}

static float Relief(float X, float Y, int32 Graine)
{
	return Bruit(X, Y, Graine) * 0.6f + Bruit(X * 2.1f, Y * 2.1f, Graine + 7) * 0.3f + Bruit(X * 4.3f, Y * 4.3f, Graine + 13) * 0.1f;
}

void AVespMonde::PeindreLeSol(const FBox2D& Limites)
{
	UMaterialInterface* Base = MateriauDuSol();
	if (!Base)
	{
		SolVivant = nullptr;
		return;
	}
	if (!SolVivant)
	{
		SolVivant = UMaterialInstanceDynamic::Create(Base, this);
	}
	// Les matieres de chaque acte (les textures stylisees des packs), et leurs teintes
	auto T = [](const TCHAR* Chemin) { return LoadObject<UTexture2D>(nullptr, Chemin, nullptr, LOAD_NoWarn | LOAD_Quiet); };
	const TCHAR* HERBE_SH = TEXT("/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Shared_Terrain/Shared_Terrain_Textures/Terrain_Textures_FSBF/T_Fsbf_Terrain_Grass_1_BC.T_Fsbf_Terrain_Grass_1_BC");
	const TCHAR* TERRE_1 = TEXT("/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Shared_Terrain/Shared_Terrain_Textures/Terrain_Textures_FSBF/T_Fsbf_Terrain_Dirt_1_BC.T_Fsbf_Terrain_Dirt_1_BC");
	const TCHAR* TERRE_2 = TEXT("/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Shared_Terrain/Shared_Terrain_Textures/Terrain_Textures_FSBF/T_Fsbf_Terrain_Dirt_2_BC.T_Fsbf_Terrain_Dirt_2_BC");
	const TCHAR* HERBE_FV = TEXT("/Game/Fantastic_Village_Pack/textures/T_ENV_TERRAIN_grass_01_BC.T_ENV_TERRAIN_grass_01_BC");
	const TCHAR* GRAVIER = TEXT("/Game/Fantastic_Village_Pack/textures/T_ENV_TERRAIN_gravel_BC.T_ENV_TERRAIN_gravel_BC");
	const TCHAR* GRAVIER_2 = TEXT("/Game/Fantastic_Village_Pack/textures/T_ENV_TERRAIN_gravel_02_BC.T_ENV_TERRAIN_gravel_02_BC");
	const TCHAR* MOUSSE = TEXT("/Game/WaterMaterials/Textures/T_Pond_Moss.T_Pond_Moss");
	struct FMatieres
	{
		const TCHAR* Herbe;
		const TCHAR* Terre;
		const TCHAR* Autre;
		FLinearColor TH, TT, TA;
		float Plaques;
	};
	const FLinearColor B(1, 1, 1);
	const FMatieres ACTES[7] = {
		{HERBE_SH, TERRE_1, HERBE_FV, B * 0.85f, B, FLinearColor(0.9f, 1.0f, 0.8f), 0.35f},													// la foret
		{HERBE_SH, TERRE_2, MOUSSE, FLinearColor(0.5f, 0.58f, 0.46f), FLinearColor(0.72f, 0.66f, 0.6f), FLinearColor(0.55f, 0.62f, 0.45f), 0.45f},	// le bois hante
		{HERBE_FV, TERRE_2, MOUSSE, FLinearColor(0.42f, 0.52f, 0.38f), FLinearColor(0.45f, 0.4f, 0.32f), FLinearColor(0.6f, 0.82f, 0.5f), 0.6f},		// les marais
		{HERBE_FV, GRAVIER_2, TERRE_1, FLinearColor(0.85f, 0.78f, 0.5f), FLinearColor(0.95f, 0.9f, 0.85f), FLinearColor(0.9f, 0.8f, 0.7f), 0.4f},	// la forteresse
		{TERRE_1, GRAVIER, HERBE_SH, FLinearColor(1.9f, 2.0f, 2.3f), FLinearColor(0.75f, 0.78f, 0.88f), FLinearColor(0.6f, 0.72f, 0.78f), 0.3f},	// le col : la neige
		{TERRE_2, GRAVIER, TERRE_1, FLinearColor(0.36f, 0.31f, 0.3f), FLinearColor(0.26f, 0.22f, 0.22f), FLinearColor(1.0f, 0.4f, 0.16f), 0.3f},	// la cendre et les braises
		{HERBE_SH, GRAVIER_2, MOUSSE, FLinearColor(0.46f, 0.37f, 0.58f), FLinearColor(0.42f, 0.32f, 0.52f), FLinearColor(0.62f, 0.36f, 0.85f), 0.5f},	// Karn
	};
	const FMatieres& A = ACTES[FMath::Clamp(Acte, 1, 7) - 1];
	SolVivant->SetTextureParameterValue(TEXT("Herbe"), T(A.Herbe));
	SolVivant->SetTextureParameterValue(TEXT("HerbeLoin"), T(A.Herbe));
	SolVivant->SetTextureParameterValue(TEXT("Terre"), T(A.Terre));
	SolVivant->SetTextureParameterValue(TEXT("Autre"), T(A.Autre));
	SolVivant->SetVectorParameterValue(TEXT("TeinteHerbe"), A.TH);
	SolVivant->SetVectorParameterValue(TEXT("TeinteTerre"), A.TT);
	SolVivant->SetVectorParameterValue(TEXT("TeinteAutre"), A.TA);

	// La carte : 512 x 512 points sur tout le monde de l'acte
	constexpr int32 N = 512;
	const FVector2D Taille = Limites.GetSize();
	TArray<FColor> Points;
	Points.SetNumUninitialized(N * N);
	const int32 Graine = 300 + Acte * 31;
	for (int32 y = 0; y < N; y++)
	{
		for (int32 x = 0; x < N; x++)
		{
			const FVector P(Limites.Min.X + (x + 0.5f) * Taille.X / N, Limites.Min.Y + (y + 0.5f) * Taille.Y / N, 0.0f);
			const float Tremble = (Relief(P.X / 900.0f, P.Y / 900.0f, Graine) - 0.5f) * 220.0f;
			// La terre battue : le milieu des sentiers, et le coeur use des clairieres
			const float DSentier = DistanceAuxSentiers(P) + Tremble;
			float Terre = 1.0f - FMath::SmoothStep(DemiLargeurSentier * 0.3f, DemiLargeurSentier * 0.95f, DSentier);
			for (const FVespZone& Z : Zones)
			{
				const float D = (FVector::Dist2D(P, Z.Centre) + Tremble) / Z.Rayon;
				Terre = FMath::Max(Terre, 0.8f * (1.0f - FMath::SmoothStep(0.25f, 0.85f, D)));
			}
			// Les plaques de la matiere de l'acte (la neige, la mousse, les braises...), surtout hors des chemins
			const float Plaque = Relief(P.X / 2600.0f, P.Y / 2600.0f, Graine + 51);
			const float Autre = FMath::SmoothStep(1.0f - A.Plaques - 0.12f, 1.0f - A.Plaques + 0.12f, Plaque) * (1.0f - Terre * 0.7f);
			// La lumiere : de grandes nappes plus sombres et plus claires
			const float Lumiere = Relief(P.X / 5000.0f, P.Y / 5000.0f, Graine + 99);
			Points[y * N + x] = FColor((uint8)(Terre * 255.0f), (uint8)(Autre * 255.0f), (uint8)(Lumiere * 255.0f), 255);
		}
	}
	CarteDuSol = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
	CarteDuSol->SRGB = true;
	CarteDuSol->Filter = TF_Bilinear;
	CarteDuSol->AddressX = TA_Clamp;
	CarteDuSol->AddressY = TA_Clamp;
	void* Donnees = CarteDuSol->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Donnees, Points.GetData(), N * N * 4);
	CarteDuSol->GetPlatformData()->Mips[0].BulkData.Unlock();
	CarteDuSol->UpdateResource();
	SolVivant->SetTextureParameterValue(TEXT("Carte"), CarteDuSol);
	SolVivant->SetVectorParameterValue(TEXT("Coin"), FLinearColor(Limites.Min.X, Limites.Min.Y, Taille.X, Taille.Y));
}
