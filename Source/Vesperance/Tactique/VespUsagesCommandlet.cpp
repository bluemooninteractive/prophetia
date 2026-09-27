#include "VespUsagesCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Misc/Paths.h"

int32 UVespUsagesCommandlet::Main(const FString& Parametres)
{
#if WITH_EDITOR
	IAssetRegistry& Registre = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Registre.SearchAllAssets(true);

	// Tous les materiaux (et instances de materiaux) du projet ; on remonte a leur materiau de base
	TArray<FAssetData> Assets;
	Registre.GetAssetsByClass(UMaterialInterface::StaticClass()->GetClassPathName(), Assets, true);
	TSet<UMaterial*> Bases;
	for (const FAssetData& A : Assets)
	{
		if (!A.PackagePath.ToString().StartsWith(TEXT("/Game")))
		{
			continue;
		}
		if (UMaterialInterface* M = Cast<UMaterialInterface>(A.GetAsset()))
		{
			if (UMaterial* Base = M->GetMaterial())
			{
				if (Base->GetOutermost()->GetName().StartsWith(TEXT("/Game")))
				{
					Bases.Add(Base);
				}
			}
		}
	}
	int32 Modifies = 0;
	for (UMaterial* Base : Bases)
	{
		if (Base->bUsedWithInstancedStaticMeshes)
		{
			continue;
		}
		bool bRecompiler = false;
		Base->SetMaterialUsage(bRecompiler, MATUSAGE_InstancedStaticMeshes);
		Base->bUsedWithInstancedStaticMeshes = true;
		UPackage* Paquet = Base->GetOutermost();
		Paquet->MarkPackageDirty();
		const FString Fichier = FPackageName::LongPackageNameToFilename(Paquet->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		if (UPackage::SavePackage(Paquet, Base, *Fichier, Args))
		{
			Modifies++;
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE : instances autorisees pour %s"), *Base->GetPathName());
		}
	}
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE : %d materiaux de base, %d modifies"), Bases.Num(), Modifies);
	// -Tailles=/Game/Pack1+/Game/Pack2 : la taille de chaque modele (pour choisir sa place et sa hauteur dans le monde)
	FString Dossiers;
	if (FParse::Value(*Parametres, TEXT("Tailles="), Dossiers))
	{
		TArray<FString> Liste;
		Dossiers.ParseIntoArray(Liste, TEXT("+"));
		for (const FString& D : Liste)
		{
			TArray<FAssetData> Modeles;
			Registre.GetAssetsByPath(FName(*D), Modeles, true);
			for (const FAssetData& A : Modeles)
			{
				if (A.AssetClassPath == USkeletalMesh::StaticClass()->GetClassPathName())
				{
					if (USkeletalMesh* S = Cast<USkeletalMesh>(A.GetAsset()))
					{
						const FBox B = S->GetBounds().GetBox();
						UE_LOG(LogTemp, Display, TEXT("VESPERANCE taille : %s  min %s  max %s (squelette)"), *S->GetPathName(), *B.Min.ToString(), *B.Max.ToString());
					}
					continue;
				}
				if (A.AssetClassPath != UStaticMesh::StaticClass()->GetClassPathName())
				{
					continue;
				}
				if (UStaticMesh* M = Cast<UStaticMesh>(A.GetAsset()))
				{
					const FBox B = M->GetBoundingBox();
					const FVector T = B.GetSize();
					UE_LOG(LogTemp, Display, TEXT("VESPERANCE taille : %s  %.0f x %.0f x %.0f  (bas %.0f)  min %s  max %s  %d mat"), *M->GetPathName(), T.X, T.Y, T.Z, B.Min.Z, *B.Min.ToString(), *B.Max.ToString(), M->GetStaticMaterials().Num());
				}
			}
		}
	}
	// -Vignettes=/Game/Dossier1+/Game/Dossier2 : chaque texture en PNG dans Saved/Vignettes (pour les regarder)
	FString DossiersVignettes;
	if (FParse::Value(*Parametres, TEXT("Vignettes="), DossiersVignettes))
	{
		TArray<FString> Liste;
		DossiersVignettes.ParseIntoArray(Liste, TEXT("+"));
		for (const FString& D : Liste)
		{
			TArray<FAssetData> Textures;
			Registre.GetAssetsByPath(FName(*D), Textures, true);
			for (const FAssetData& A : Textures)
			{
				UTexture2D* T = A.AssetClassPath == UTexture2D::StaticClass()->GetClassPathName() ? Cast<UTexture2D>(A.GetAsset()) : nullptr;
				FImage Image;
				if (T && T->Source.IsValid() && T->Source.GetMipImage(Image, 0))
				{
					const FString Fichier = FPaths::ProjectSavedDir() / TEXT("Vignettes") / (T->GetName() + TEXT(".png"));
					FImageUtils::SaveImageByExtension(*Fichier, Image);
				}
			}
		}
	}
#endif
	return 0;
}
