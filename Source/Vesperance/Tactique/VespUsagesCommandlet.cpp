#include "VespUsagesCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

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
#endif
	return 0;
}
