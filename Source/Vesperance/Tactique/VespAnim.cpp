#include "VespAnim.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"

static float Longueur(const UAnimSequence* A)
{
	return A ? FMath::Max(0.01f, A->GetPlayLength()) : 1.0f;
}

// ===================== Ce que le jeu demande =====================

void UVespAnimInstance::Locomotion(const UAnimSequence* Repos, const UAnimSequence* Marche, const UAnimSequence* Course,
                                   float VitesseDeMarche, float VitesseDeCourse)
{
	Etat.Loco[0].Anim = Repos;
	Etat.Loco[1].Anim = Marche ? Marche : Course;
	Etat.Loco[2].Anim = Course ? Course : Marche;
	for (FVespCouche& C : Etat.Loco)
	{
		C.bBoucle = true;
	}
	VitesseMarche = FMath::Max(50.0f, VitesseDeMarche);
	VitesseCourse = FMath::Max(VitesseMarche + 50.0f, VitesseDeCourse);
}

void UVespAnimInstance::ChangerRepos(const UAnimSequence* Repos)
{
	if (Repos && Repos != Etat.Loco[0].Anim)
	{
		Etat.Loco[0].Anim = Repos;
		Etat.Loco[0].Temps = 0.0f;
	}
}

float UVespAnimInstance::JouerAction(const UAnimSequence* Anim, float Vitesse, bool bHautDuCorps, bool bTenir, float Entree, float Sortie)
{
	if (!Anim)
	{
		return 0.0f;
	}
	// L'action en cours (s'il y en a une) s'efface pendant que la nouvelle arrive
	if (Etat.Action.Anim && Etat.Action.Poids > 0.02f)
	{
		Etat.ActionAvant = Etat.Action;
		Etat.bAvantHautDuCorps = Etat.bActionHautDuCorps;
	}
	Etat.Action.Anim = Anim;
	Etat.Action.Temps = 0.0f;
	Etat.Action.Poids = 0.0f;
	Etat.Action.bBoucle = false;
	Etat.bActionHautDuCorps = bHautDuCorps;
	VitesseAction = FMath::Max(0.05f, Vitesse);
	EntreeAction = FMath::Max(0.01f, Entree);
	SortieAction = FMath::Max(0.01f, Sortie);
	bTenirAction = bTenir;
	bSortieDemandee = false;
	return Longueur(Anim) / VitesseAction;
}

void UVespAnimInstance::ArreterAction(float Sortie)
{
	if (Etat.Action.Anim)
	{
		SortieAction = FMath::Max(0.01f, Sortie);
		bSortieDemandee = true;
		bTenirAction = false;
	}
}

bool UVespAnimInstance::ActionEnCours() const
{
	return Etat.Action.Anim && !bSortieDemandee && (bTenirAction || Etat.Action.Temps < Longueur(Etat.Action.Anim));
}

float UVespAnimInstance::ProgressionAction() const
{
	return Etat.Action.Anim ? FMath::Clamp(Etat.Action.Temps / Longueur(Etat.Action.Anim), 0.0f, 1.0f) : 1.0f;
}

void UVespAnimInstance::SetPosture(const UAnimSequence* Boucle)
{
	PostureVoulue = Boucle;
	if (Boucle && Etat.Posture.Anim != Boucle)
	{
		Etat.Posture.Anim = Boucle;
		Etat.Posture.Temps = 0.0f;
		Etat.Posture.bBoucle = true;
	}
}

// ===================== A chaque image (le jeu) =====================

void UVespAnimInstance::NativeUpdateAnimation(float Dt)
{
	Super::NativeUpdateAnimation(Dt);

	// 1. La locomotion : le repos, la marche et la course se melangent selon la vitesse
	VitesseLissee = FMath::FInterpTo(VitesseLissee, VitesseVoulue, Dt, 12.0f);
	const float V = VitesseLissee;
	float PoidsRepos = 1.0f, PoidsMarche = 0.0f, PoidsCourse = 0.0f;
	if (V > 12.0f)
	{
		if (V < VitesseMarche)
		{
			PoidsMarche = FMath::Clamp((V - 12.0f) / (VitesseMarche * 0.5f), 0.0f, 1.0f);
			PoidsRepos = 1.0f - PoidsMarche;
		}
		else
		{
			PoidsCourse = FMath::Clamp((V - VitesseMarche) / (VitesseCourse - VitesseMarche), 0.0f, 1.0f);
			PoidsMarche = 1.0f - PoidsCourse;
			PoidsRepos = 0.0f;
		}
	}
	// La marche et la course partagent la meme phase : les pieds tombent au meme moment pendant le fondu
	const float AlluresMarche = FMath::Clamp(V / VitesseMarche, 0.7f, 1.5f);
	const float AlluresCourse = FMath::Clamp(V / VitesseCourse, 0.7f, 1.4f);
	const float CycleMarche = Longueur(Etat.Loco[1].Anim) / AlluresMarche;
	const float CycleCourse = Longueur(Etat.Loco[2].Anim) / AlluresCourse;
	const float Cycle = FMath::Lerp(CycleMarche, CycleCourse, PoidsCourse);
	if (PoidsMarche + PoidsCourse > 0.0f)
	{
		Phase = FMath::Fmod(Phase + Dt / FMath::Max(0.1f, Cycle), 1.0f);
	}
	Etat.Loco[0].Temps = FMath::Fmod(Etat.Loco[0].Temps + Dt, Longueur(Etat.Loco[0].Anim));
	Etat.Loco[1].Temps = Phase * Longueur(Etat.Loco[1].Anim);
	Etat.Loco[2].Temps = Phase * Longueur(Etat.Loco[2].Anim);
	Etat.Loco[0].Poids = PoidsRepos;
	Etat.Loco[1].Poids = PoidsMarche;
	Etat.Loco[2].Poids = PoidsCourse;

	// 2. La posture (la garde) : elle arrive et repart en fondu
	FVespCouche& P = Etat.Posture;
	if (P.Anim)
	{
		P.Temps = FMath::Fmod(P.Temps + Dt, Longueur(P.Anim));
		P.Poids = FMath::Clamp(P.Poids + (PostureVoulue ? Dt : -Dt) / 0.12f, 0.0f, 1.0f);
		if (!PostureVoulue && P.Poids <= 0.0f)
		{
			P.Anim = nullptr;
		}
	}

	// 3. L'action d'avant s'efface ; la nouvelle arrive, joue, et repart en fondu a la fin
	FVespCouche& Avant = Etat.ActionAvant;
	if (Avant.Anim)
	{
		Avant.Temps = FMath::Min(Avant.Temps + Dt * VitesseAction, Longueur(Avant.Anim));
		Avant.Poids -= Dt / EntreeAction;
		if (Avant.Poids <= 0.0f)
		{
			Avant.Anim = nullptr;
			Avant.Poids = 0.0f;
		}
	}
	FVespCouche& A = Etat.Action;
	if (A.Anim)
	{
		const float L = Longueur(A.Anim);
		A.Temps = FMath::Min(A.Temps + Dt * VitesseAction, L);
		if (bSortieDemandee)
		{
			A.Poids -= Dt / SortieAction;
		}
		else
		{
			A.Poids = FMath::Min(1.0f, A.Poids + Dt / EntreeAction);
			if (!bTenirAction)
			{
				// Elle s'efface juste avant sa fin (pour retrouver la locomotion sans a-coup)
				const float Restant = (L - A.Temps) / VitesseAction;
				A.Poids = FMath::Min(A.Poids, Restant / SortieAction);
				bSortieDemandee = Restant <= 0.0f;
			}
		}
		if (A.Poids <= 0.0f && (bSortieDemandee || A.Temps >= L))
		{
			A.Anim = nullptr;
			A.Poids = 0.0f;
			bSortieDemandee = false;
		}
	}

	// Le calcul des poses (peut-etre sur un autre fil) travaille sur une copie
	GetProxyOnGameThread<FVespAnimProxy>().Etat = Etat;
}

FAnimInstanceProxy* UVespAnimInstance::CreateAnimInstanceProxy()
{
	return new FVespAnimProxy(this);
}

void UVespAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FVespAnimProxy*>(InProxy);
}

// ===================== Le calcul des poses =====================

static void Echantillonner(const FVespCouche& C, FPoseContext& Sortie)
{
	FAnimationPoseData Donnees(Sortie);
	C.Anim->GetAnimationPose(Donnees, FAnimExtractContext((double)C.Temps, false, FDeltaTimeRecord(), C.bBoucle));
}

// Le haut du corps : la colonne vertebrale et tout ce qui en part (bras, tete) ; les hanches et les jambes n'en sont pas
void FVespAnimProxy::PreparerMasque(const FBoneContainer& Os)
{
	if (bMasquePret && SerieMasque == Os.GetSerialNumber() && Masque.Num() == Os.GetCompactPoseNumBones())
	{
		return;
	}
	const int32 Nombre = Os.GetCompactPoseNumBones();
	Masque.Init(0.0f, Nombre);
	const FReferenceSkeleton& Reference = Os.GetReferenceSkeleton();
	bool bColonneTrouvee = false;
	for (int32 i = 0; i < Nombre; i++)
	{
		const FCompactPoseBoneIndex Index(i);
		const FCompactPoseBoneIndex Parent = Os.GetParentBoneIndex(Index);
		const FString Nom = Reference.GetBoneName(Os.MakeMeshPoseIndex(Index).GetInt()).ToString().ToLower();
		if (!bColonneTrouvee && Nom.Contains(TEXT("spine")))
		{
			Masque[i] = 1.0f;			// la premiere vertebre : tout ce qui suit en fait partie
			bColonneTrouvee = true;
		}
		else if (Parent.GetInt() >= 0 && Parent.GetInt() < Nombre)
		{
			Masque[i] = Masque[Parent.GetInt()];
		}
	}
	SerieMasque = Os.GetSerialNumber();
	bMasquePret = true;
}

void FVespAnimProxy::Superposer(FPoseContext& Sortie, const FVespCouche& Couche, bool bHautDuCorps)
{
	if (!Couche.Anim || Couche.Poids <= 0.001f)
	{
		return;
	}
	FPoseContext Dessus(Sortie);
	Echantillonner(Couche, Dessus);
	if (!bHautDuCorps)
	{
		FAnimationPoseData Base(Sortie);
		FAnimationRuntime::BlendTwoPosesTogetherInPlace(Base, FAnimationPoseData(Dessus), 1.0f - Couche.Poids);
		return;
	}
	const FBoneContainer& Os = Sortie.Pose.GetBoneContainer();
	PreparerMasque(Os);
	TArray<float> Poids;
	Poids.SetNumUninitialized(Masque.Num());
	for (int32 i = 0; i < Masque.Num(); i++)
	{
		Poids[i] = Masque[i] * Couche.Poids;
	}
	FCompactPose Resultat;
	Resultat.SetBoneContainer(&Os);
	FAnimationRuntime::BlendTwoPosesTogetherPerBone(Sortie.Pose, Dessus.Pose, Poids, Resultat);
	Sortie.Pose.CopyBonesFrom(Resultat);
}

bool FVespAnimProxy::Evaluate(FPoseContext& Output)
{
	// 1. La locomotion : jusqu'a trois animations, melangees selon leurs poids
	float Cumul = 0.0f;
	for (const FVespCouche& C : Etat.Loco)
	{
		if (!C.Anim || C.Poids <= 0.001f)
		{
			continue;
		}
		if (Cumul <= 0.0f)
		{
			Echantillonner(C, Output);
			Cumul = C.Poids;
			continue;
		}
		FPoseContext Autre(Output);
		Echantillonner(C, Autre);
		FAnimationPoseData Base(Output);
		FAnimationRuntime::BlendTwoPosesTogetherInPlace(Base, FAnimationPoseData(Autre), Cumul / (Cumul + C.Poids));
		Cumul += C.Poids;
	}
	if (Cumul <= 0.0f)
	{
		Output.ResetToRefPose();
	}
	// 2. et 3. La posture, puis les actions, par-dessus
	Superposer(Output, Etat.Posture, true);
	Superposer(Output, Etat.ActionAvant, Etat.bAvantHautDuCorps);
	Superposer(Output, Etat.Action, Etat.bActionHautDuCorps);
	return true;
}
