#include "VespPlayerController.h"
#include "VespSauvegarde.h"
#include "Misc/App.h"
#include "VespMonde.h"
#include "VespCombat.h"
#include "VespUnite.h"
#include "VespEffet.h"
#include "VespSons.h"
#include "VespInterface.h"
#include "VespButin.h"
#include "VespMeteo.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/GameUserSettings.h"
#include "VespReglages.h"
#include "VespPartie.h"
#include "VespMaitres.h"
#include "VespTexte.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "AssetCompilingManager.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

// ===================== Les 7 actes =====================

struct FVespInfoActe
{
	const TCHAR* Titre;
	const TCHAR* Lieu;
	const TCHAR* LieuDuBoss;
	const TCHAR* Regle;			// ce qui rend cet acte different
	FLinearColor Brume;
	float Densite;
	FLinearColor Lune;
	float IntensiteLune;
	float Ciel;
};

static const FVespInfoActe ACTES[7] = {
	{TEXT("Les Terres Brumeuses"), TEXT("La Forêt des Brumes"), TEXT("Le cercle des anciens"),
	 TEXT("Une forêt calme... pour l'instant. Apprends à lire les Haschen : quand ils rougeoient, ils vont frapper. Esquive, ou pare au dernier moment."),
	 FLinearColor(0.05f, 0.12f, 0.16f), 0.006f, FLinearColor(0.55f, 0.65f, 1.0f), 3.0f, 0.35f},
	{TEXT("Les Terres Hantées"), TEXT("Le Bois des Pendus"), TEXT("La clairière de la Matriarche"),
	 TEXT("Le poison suinte du sol pendant les combats. Les loups chassent en meute et foncent en ligne droite."),
	 FLinearColor(0.07f, 0.13f, 0.05f), 0.012f, FLinearColor(0.62f, 0.78f, 0.55f), 2.4f, 0.3f},
	{TEXT("Les Marais Noyés"), TEXT("Les Marais de Sombreval"), TEXT("Le trône englouti"),
	 TEXT("Les eaux toxiques montent pendant les combats : ne reste pas dedans. Les sangsues volent la vie."),
	 FLinearColor(0.04f, 0.1f, 0.08f), 0.016f, FLinearColor(0.5f, 0.75f, 0.7f), 2.2f, 0.3f},
	{TEXT("La Marche d'Ashka"), TEXT("La forteresse d'Ashka"), TEXT("Le grand portail"),
	 TEXT("Les dalles piégées se lèvent sans cesse. Les armures ne cèdent qu'aux coups lourds : une armure brisée laisse son porteur sonné."),
	 FLinearColor(0.1f, 0.07f, 0.05f), 0.008f, FLinearColor(0.9f, 0.7f, 0.5f), 2.6f, 0.35f},
	{TEXT("Le Col d'Ashka"), TEXT("Le col gelé"), TEXT("Le sommet du col"),
	 TEXT("Le blizzard ralentit tout le monde. Le givre fige qui se laisse toucher deux fois."),
	 FLinearColor(0.12f, 0.14f, 0.2f), 0.01f, FLinearColor(0.75f, 0.85f, 1.0f), 2.2f, 0.4f},
	{TEXT("Les Terres de Cendre"), TEXT("La faille ardente"), TEXT("La forge de Vorgath"),
	 TEXT("Le sol se fissure et entre en éruption pendant les combats : les cercles rouges brûlent tout le monde, Haschen compris."),
	 FLinearColor(0.18f, 0.06f, 0.03f), 0.01f, FLinearColor(1.0f, 0.55f, 0.35f), 2.4f, 0.3f},
	{TEXT("Karn"), TEXT("La cité voilée"), TEXT("Le cœur du Voile"),
	 TEXT("Le Voile se déchire et des échos d'AYLIS en sortent. Tout ce que tu as affronté revient."),
	 FLinearColor(0.08f, 0.04f, 0.14f), 0.012f, FLinearColor(0.7f, 0.5f, 1.0f), 2.6f, 0.35f},
};

static const FVespInfoActe& InfoActe(int32 Acte)
{
	return ACTES[FMath::Clamp(Acte, 1, 7) - 1];
}

// ===================== Ce que le monde retient (les textes du document narratif) =====================

// Ou tombe une vision, et sous quels coups (la ligne de la chute)
static const TCHAR* LIEU_DE_CHUTE[7] = {
	TEXT("dans les Terres Brumeuses"), TEXT("dans les Terres Hantées"), TEXT("dans les Marais Noyés"), TEXT("sur la Marche d'Ashka"),
	TEXT("sur le Col d'Ashka"), TEXT("dans les Terres de Cendre"), TEXT("à Karn"),
};
static const TCHAR* COUPS_DU_GARDIEN[7] = {
	TEXT("sous la masse de Skarn"), TEXT("sous les crocs de la Matriarche"), TEXT("sous la marée du Roi Noyé"), TEXT("sous les poings du Gardien de Pierre"),
	TEXT("sous les flèches d'Ashka"), TEXT("dans les flammes de Vorgath"), TEXT("devant l'Oracle"),
};
// Le gardien, au milieu d'une phrase
static const TCHAR* GARDIEN[7] = {
	TEXT("Skarn"), TEXT("la Matriarche"), TEXT("le Roi Noyé"), TEXT("le Gardien de Pierre"), TEXT("Ashka"), TEXT("Vorgath"), TEXT("l'Oracle"),
};
// Le gardien se souvient : il a deja tue AYLIS (l'une des visions), ou il a deja ete vaincu
static const TCHAR* APRES_UNE_MORT[7] = {
	TEXT("Encore toi ? Le sol se souvient de ta dernière chute."),
	TEXT("Mes loups ont gardé ton odeur. Ils t'attendaient."),
	TEXT("Tu reviens toujours par l'eau. Comme tout le monde."),
	TEXT("INTRUS DÉJÀ VU. PORTE TOUJOURS FERMÉE."),
	TEXT("La prophétie avait raison la dernière fois. Pourquoi pas cette fois ?"),
	TEXT("J'ai gardé tes cendres. Elles ont la couleur de l'aube."),
	TEXT("Et de mille un."),
};
static const TCHAR* DEJA_VAINCU[7] = {
	TEXT("Tu m'as déjà brisé une fois. Je n'ai pas oublié comment."),
	TEXT("Une autre vision m'a déjà chassée d'ici. Elle n'est jamais arrivée plus loin."),
	TEXT("Je me suis déjà noyé deux fois. Une de plus ne me fait pas peur."),
	TEXT("ANOMALIE. CETTE VISION EST DÉJÀ PASSÉE."),
	TEXT("Tu m'as déjà vaincue. Et pourtant tu es encore là, au début. Qu'est-ce que ça t'a donné ?"),
	TEXT("Ma forge s'est rallumée sans toi. Elle se rallume toujours."),
	TEXT("Tu m'as déjà atteint. Alors tu sais ce qu'il y a derrière moi. Tu veux vraiment le revoir ?"),
};
// La premiere ligne d'une vision (a partir de la deuxieme)
static const TCHAR* OUVERTURES[5] = {
	TEXT("Vision %d. La route a la même odeur."),
	TEXT("Vision %d. Les brumes, encore. Mais pas tout à fait les mêmes pas."),
	TEXT("Vision %d. Je me souviens de la route. Pas de la fin."),
	TEXT("Vision %d. Quelqu'un a marché ici avant moi. Moi, sans doute."),
	TEXT("Vision %d. On recommence. On recommence toujours un peu mieux."),
};
// Le Veilleur, quand rien de particulier ne s'est passe, et ce qu'AYLIS lui repond
static const TCHAR* VEILLEUR_AMBIANCE[10] = {
	TEXT("Le feu ne s'éteint jamais tout à fait. Les visions non plus."),
	TEXT("Mange quelque chose. Même une vision a besoin de forces."),
	TEXT("Les Haschen ne s'approchent pas des flammes. Ils ont peur de ce qu'elles montrent."),
	TEXT("J'ai entretenu ce feu pour des milliers de visions. Aucune ne s'est assise exactement comme toi."),
	TEXT("La route est plus courte qu'elle n'en a l'air. C'est la fin qui est longue."),
	TEXT("Écoute le bois craquer. Chaque fois, c'est une vision qui passe quelque part."),
	TEXT("Je ne te demande pas où tu vas. Je le sais. Je te demande si c'est le moment."),
	TEXT("Repose-toi. Les gardiens, eux, ne dorment jamais. C'est leur faiblesse."),
	TEXT("Un jour, une vision m'a demandé mon nom. Je le lui ai donné. Je n'en ai plus eu besoin depuis."),
	TEXT("Quand le Voile se lèvera, j'aimerais voir le soleil. Juste une fois."),
};
static const TCHAR* REPONSES_AU_VEILLEUR[5] = {
	TEXT("Garde le feu allumé."),
	TEXT("Encore un peu de route."),
	TEXT("Merci, Oswin."),
	TEXT("Je ne m'attarde pas."),
	TEXT("Alors à la prochaine vision."),
};
static const TCHAR* VEILLEUR = TEXT("Oswin, le Veilleur");

// Ce qu'AYLIS dit en route (le document narratif : 3 a 5 variantes par declencheur)
static const TCHAR* EN_ENTRANT[7] = {
	TEXT("Une route. Bon. On y va."),
	TEXT("Des arbres qui pendent des choses. Charmant."),
	TEXT("Des marais. Évidemment."),
	TEXT("Des murs. Enfin quelque chose qui ne pousse pas."),
	TEXT("Il fait plus froid ici. Ou c'est que j'existe un peu plus."),
	TEXT("Tout brûle. Au moins, on y voit clair."),
	TEXT("Karn. Cette ville me connaît déjà."),
};
static const TCHAR* CLAIRIERE_LIBEREE[4] = {TEXT("Suivante."), TEXT("Respire."), TEXT("Une de moins."), TEXT("Le chemin est libre.")};
static const TCHAR* PV_BAS[4] = {TEXT("Pas ici. Pas encore."), TEXT("Encore debout."), TEXT("Ça ne finira pas là."), TEXT("Respire. Recommence.")};
static const TCHAR* PLUS_DE_POTIONS[4] = {TEXT("Il faudra faire sans."), TEXT("Plus une goutte."), TEXT("Sans filet, alors."), TEXT("Tant pis. On avance.")};
static const TCHAR* PARADE[4] = {TEXT("Je l'ai vu venir."), TEXT("Trop lent."), TEXT("Pas cette fois."), TEXT("Prévisible.")};
static const TCHAR* LEGENDAIRE[4] = {TEXT("Celle-là, je la garde."), TEXT("Enfin quelque chose à ma mesure."), TEXT("Oh. Ça, c'est une trouvaille."), TEXT("La prophétie a bon goût.")};

// ===================== La mise en route =====================

AVespPlayerController::AVespPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AVespPlayerController::Ecrire(const FString& Message, float Duree)
{
	MessageRoute = Message;
	TempsMessage = Duree;
}

void AVespPlayerController::Commencer(AVespMonde* LeMonde, AVespCombat* LeCombat, AVespUnite* LAylis, ACameraActor* LaCamera)
{
	Monde = LeMonde;
	Combat = LeCombat;
	Aylis = LAylis;
	CameraArene = LaCamera;
	PositionCamera = CameraArene ? CameraArene->GetActorLocation() : FVector::ZeroVector;
	RotationCamera = CameraArene ? CameraArene->GetActorRotation() : FRotator::ZeroRotator;
	DecalageCamera = PositionCamera - Monde->GetActorLocation();
	CameraActuelle = PositionCamera;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	// L'interface (Slate) : posee par-dessus l'ecran de jeu
	if (GEngine && GEngine->GameViewport)
	{
		Interface = SNew(SVespInterface).Joueur(this);
		GEngine->GameViewport->AddViewportWidgetContent(Interface.ToSharedRef());
	}
	// AYLIS, voie de l'epee : une epee et un petit bouclier (la garde)
	Seuil.Init(false, VespSeuil::Nombre);
	HasardButin.Initialize((int32)(FPlatformTime::Cycles() & 0x7FFFFFFF));
	Equipement[(int32)EVespEmplacement::Arme] = VespButin::EpeeDeDepart();
	Equipement[(int32)EVespEmplacement::MainGauche] = VespButin::BouclierDeDepart();
	bEquipe[(int32)EVespEmplacement::Arme] = bEquipe[(int32)EVespEmplacement::MainGauche] = true;
	HabillerAylis();
	// Ce que le combat nous annonce
	Combat->Preparer(Monde, Aylis);
	Meteo = GetWorld()->SpawnActor<AVespMeteo>(FVector::ZeroVector, FRotator::ZeroRotator);
	Meteo->Suivre(Aylis);
	Combat->SurChute = [this](AVespUnite* H, int32 Categorie) { QuandHaschenTombe(H, Categorie); };
	Combat->SurBlessure = [this](int32 D, bool bC, AVespUnite* S) { QuandAylisTouchee(D, bC, S); };
	Combat->SurFermeture = [this](int32 Z) { QuandClairiereFermee(Z); };
	Combat->SurLiberation = [this](int32 Z) { QuandClairiereLiberee(Z); };
	Combat->SurImpact = [this](float Force, bool bFort) {
		Trembler(Force * 0.6f);
		if (bFort)
		{
			Zoom = FMath::Max(Zoom, 0.6f);
			Ralenti(0.2f, 0.07f);			// l'arret sur image d'un coup fort
		}
	};
	Combat->SurMessage = [this](const FString& M) { Ecrire(M, 3.5f); };
	Combat->SurParade = [this]() {
		Rage = FMath::Min(100, Rage + 20);
		ParlerEnRoute(PARADE, UE_ARRAY_COUNT(PARADE));
		Ralenti(0.15f, 0.18f);
		if (Talent(TEXT("riposte")))
		{
			bRiposte = true;
			Aylis->AfficherMessage(TEXT("RIPOSTE"), FColor(255, 190, 120), 40.0f);
		}
	};
	Combat->SurEsquive = [this]() {
		Rage = FMath::Min(100, Rage + 4);
		// Le Presage : une esquive au dernier moment ralentit le temps
		if (Talent(TEXT("presage")) && RechargePresage <= 0.0f)
		{
			RechargePresage = 5.0f;
			Ralenti(0.35f, 1.1f);
			Aylis->AfficherMessage(TEXT("PRÉSAGE"), FColor(200, 170, 255), 40.0f);
			UVespSons::Jouer2D(this, EVespSon::Rune, 0.6f, 1.3f);
		}
	};
	// Le monde du premier acte ; l'ecran titre le montre, la camera tourne lentement autour d'AYLIS.
	// Chaque vision tire son decor (les photos gardent toujours le meme, pour pouvoir les comparer)
	Monde->GraineVision = FParse::Param(FCommandLine::Get(), TEXT("VespPhotos")) ? 0 : 1 + (int32)(FPlatformTime::Cycles() % 100000);
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE decor : graine %d"), Monde->GraineVision);
	GenererMonde();
	if (UVespSons* Sons = GetWorld()->GetSubsystem<UVespSons>())
	{
		Sons->AmbianceDeLActe(Acte);
	}
	if (Meteo)
	{
		Meteo->Configurer(Acte);
	}
	Phase = EVespPhase::Titre;
	TempsPhase = 0.0f;
	bModePhoto = FParse::Param(FCommandLine::Get(), TEXT("VespPhotos"));
	ChargerMemoire();
	ChargerReglages();
	PhotoAttente = 8.0f;
	// -VespVeilleur (pour tester) : l'ecran titre s'ouvre sur le Veilleur, une photo, et on quitte
	bTestVeilleur = bVeilleurOuvert = FParse::Param(FCommandLine::Get(), TEXT("VespVeilleur"));
	if (bTestVeilleur && Memoire)
	{
		// (le test montre un Veilleur deja entame ; rien n'est sauvegarde)
		Memoire->Souvenirs = 185;
		Memoire->Chandelles = 1;
		for (const TCHAR* Id : {TEXT("vigueur"), TEXT("fiole"), TEXT("tranchant"), TEXT("flamme")})
		{
			Memoire->CartesDebloquees[VespVeilleur::Index(Id)] = true;
		}
		Memoire->CartesActives[VespVeilleur::Index(TEXT("vigueur"))] = true;
		Memoire->CartesActives[VespVeilleur::Index(TEXT("tranchant"))] = true;
		SelectionDon = 11;
	}
	bTestOuverture = FParse::Param(FCommandLine::Get(), TEXT("VespOuverture"));
	bTestOptions = bOptionsOuvertes = FParse::Param(FCommandLine::Get(), TEXT("VespOptions"));
	FParse::Value(FCommandLine::Get(), TEXT("VespFin="), TestFin);
	FParse::Value(FCommandLine::Get(), TEXT("VespReprise="), TestReprise);
	bTestRecompenses = FParse::Param(FCommandLine::Get(), TEXT("VespRecompenses"));
	EtapeTestMaitres = FParse::Param(FCommandLine::Get(), TEXT("VespMaitres")) ? 0 : -1;
	if (!bModePhoto && !bTestOuverture && TestFin == 0 && !bTestOptions)
	{
		PartieSuspendue = Cast<UVespPartie>(UGameplayStatics::LoadGameFromSlot(EmplacementPartie(), 0));
		if (PartieSuspendue && TestReprise == 1)
		{
			PartieSuspendue = nullptr;		// (le test repart d'une vision neuve)
		}
	}
	if (Memoire && !bModePhoto)
	{
		Combat->bOracleAylis = Memoire->Victoires > 0;
		Combat->ArmeEcho = Memoire->ArmeDerniereChute;
		Combat->SecondeArmeEcho = Memoire->SecondeArmeDerniereChute;
		Combat->LongueurEcho = Memoire->LongueurArmeDerniereChute;
		Combat->TypeEcho = (EVespArme)Memoire->TypeArmeDerniereChute;
	}
}

void AVespPlayerController::EndPlay(const EEndPlayReason::Type Raison)
{
	SauverPartie();
	EcrireMemoire();
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	if (Interface.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Interface.ToSharedRef());
	}
	Interface.Reset();
	Super::EndPlay(Raison);
}

void AVespPlayerController::NouvellePartie(int32 ActeDeDepart)
{
	if (Phase != EVespPhase::Titre)
	{
		return;
	}
	UVespSons::Jouer2D(this, EVespSon::Clic);
	EffacerPartie();
	bVisionEnCours = true;
	Acte = FMath::Clamp(ActeDeDepart, 1, NombreDActes);
	if (Acte > 1)
	{
		// Commencer plus loin (pour tester) : AYLIS recoit des forces a la hauteur de l'acte
		const int32 Avance = Acte - 1;
		Aylis->Stats.PvMax += 18 * Avance;
		Aylis->Stats.Pv = Aylis->Stats.PvMax;
		Aylis->Stats.Attaque += 3 * Avance;
		Aylis->Stats.Defense += Avance;
		Potions += Avance;
		Eclats += 40 * Avance;
		Niveau += 3 * Avance;
	}
	// Les dons du Veilleur ; une vision commencee plus loin ne rapporte pas de Souvenirs
	bVeilleurOuvert = false;
	bSouvenirsPossibles = Acte == 1;
	SouvenirsDeLaVision = 0;
	AppliquerDons();
	bOuvertureAFaire = Acte == 1;
	DejaDitDansLaVision.Reset();
	AmbianceDeLActe();
	Phase = EVespPhase::NouvelActe;
	TempsPhase = 0.0f;
	// Une nouvelle vision commence : la prophetie s'en souviendra
	ChronoActe = ChronoPartie = 0.0f;
	TempsDesActes.Reset();
	if (Memoire)
	{
		Memoire->Visions = NumeroVision;
		EcrireMemoire();
	}
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE vision %d : depart a l'acte %d"), NumeroVision, Acte);
}

// ===================== La memoire de la boucle, et le chronometre =====================

static const TCHAR* EMPLACEMENT_MEMOIRE = TEXT("Vesperance");

void AVespPlayerController::ChargerMemoire()
{
	Memoire = Cast<UVespSauvegarde>(UGameplayStatics::LoadGameFromSlot(EMPLACEMENT_MEMOIRE, 0));
	if (!Memoire)
	{
		Memoire = Cast<UVespSauvegarde>(UGameplayStatics::CreateSaveGameObject(UVespSauvegarde::StaticClass()));
	}
	Memoire->Completer();
	// L'ancienne sauvegarde avait des dons a rangs : chaque don achete devient sa carte, debloquee et active
	static const TCHAR* ANCIENS_DONS[UVespSauvegarde::NombreDeDons] = {TEXT("vigueur"), TEXT("tranchant"), TEXT("pierre"), TEXT("fiole"), TEXT("bourse"), TEXT("etoile"), TEXT("souffle")};
	for (int32 d = 0; d < UVespSauvegarde::NombreDeDons; d++)
	{
		const int32 c = VespVeilleur::Index(ANCIENS_DONS[d]);
		if (Memoire->Dons[d] > 0 && c >= 0)
		{
			Memoire->CartesDebloquees[c] = true;
			Memoire->CartesActives[c] = true;
			Memoire->Dons[d] = 0;
		}
	}
	NumeroVision = Memoire->Visions + 1;
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE memoire : %d visions, %d victoires, %d gardiens deja vaincus, %s de jeu"),
	       Memoire->Visions, Memoire->Victoires, Memoire->GardiensDejaVaincus(), *UVespSauvegarde::Duree(Memoire->TempsDeJeu));
}

void AVespPlayerController::EcrireMemoire()
{
	if (!Memoire || bModePhoto || bTestOuverture || TestFin > 0 || bTestOptions || TestReprise > 0 || bTestRecompenses || bTestVeilleur || EtapeTestMaitres >= 0)
	{
		return;		// (les photos et les tests ne comptent pas comme des visions)
	}
	UGameplayStatics::SaveGameToSlot(Memoire, EMPLACEMENT_MEMOIRE, 0);
}

bool AVespPlayerController::ChronoEnMarche() const
{
	return !bModePhoto && Phase != EVespPhase::Titre && Phase != EVespPhase::NouvelActe && Phase != EVespPhase::Victoire && Phase != EVespPhase::Defaite;
}

void AVespPlayerController::FinirLActe()
{
	const int32 i = FMath::Clamp(Acte, 1, NombreDActes) - 1;
	TempsDesActes.Add(ChronoActe);
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE chrono : vision %d, acte %s termine en %s (partie : %s)"), NumeroVision, *Romain(Acte),
	       *UVespSauvegarde::Duree(ChronoActe), *UVespSauvegarde::Duree(ChronoPartie));
	if (Memoire)
	{
		Memoire->GardiensVaincus[i] = true;
		Memoire->DerniersActes[i] = ChronoActe;
		if (Memoire->MeilleursActes[i] <= 0.0f || ChronoActe < Memoire->MeilleursActes[i])
		{
			Memoire->MeilleursActes[i] = ChronoActe;
		}
		EcrireMemoire();
	}
	ChronoActe = 0.0f;
}

void AVespPlayerController::MemoriserLaChute()
{
	EffacerPartie();
	const bool bFaceAuBoss = Noeuds.IsValidIndex(ZoneActuelle) && Noeuds[ZoneActuelle].Type == EVespSalle::Boss && Combat && Combat->BossActif();
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE chrono : vision %d tombee a l'acte %s%s, apres %s (acte en cours depuis %s)"), NumeroVision, *Romain(Acte),
	       bFaceAuBoss ? TEXT(" face au gardien") : TEXT(""), *UVespSauvegarde::Duree(ChronoPartie), *UVespSauvegarde::Duree(ChronoActe));
	if (Memoire)
	{
		const int32 i = FMath::Clamp(Acte, 1, NombreDActes) - 1;
		if (bFaceAuBoss)
		{
			Memoire->MortsParGardien[i]++;
		}
		Memoire->DerniereChuteActe = Acte;
		Memoire->bDerniereChuteBoss = bFaceAuBoss;
		const FVespObjet* Arme = Equipe(EVespEmplacement::Arme);
		Memoire->ArmeDerniereChute = Arme ? Arme->Modele : FString();
		Memoire->SecondeArmeDerniereChute = Arme ? Arme->SecondModele : FString();
		Memoire->LongueurArmeDerniereChute = Arme ? Arme->Longueur : 0.5f;
		Memoire->TypeArmeDerniereChute = (uint8)(Arme ? Arme->TypeArme : EVespArme::Poings);
		EcrireMemoire();
	}
}

// ===================== Le Veilleur : les Souvenirs et les dons =====================

static_assert(VespVeilleur::Nombre == UVespSauvegarde::NombreDeCartes, "une case par carte dans la sauvegarde");

int32 AVespPlayerController::Capacite() const
{
	return VespVeilleur::CapaciteDeBase + (Memoire ? FMath::Clamp(Memoire->Chandelles, 0, VespVeilleur::ChandellesMax) : 0);
}

int32 AVespPlayerController::CapaciteUtilisee() const
{
	int32 N = 0;
	for (int32 i = 0; i < VespVeilleur::Nombre; i++)
	{
		N += Memoire && Memoire->Active(i) ? VespVeilleur::Carte(i).Cout : 0;
	}
	return N;
}

void AVespPlayerController::GagnerSouvenirs(int32 Quantite, const TCHAR* Pourquoi)
{
	if (!bSouvenirsPossibles || bModePhoto || Quantite <= 0 || !Memoire)
	{
		return;
	}
	SouvenirsDeLaVision += Quantite;
	Memoire->Souvenirs += Quantite;
	Memoire->SouvenirsGagnes += Quantite;
	MomentSouvenirs = FPlatformTime::Seconds();
	EcrireMemoire();		// gardes tout de suite, meme si le jeu est coupe net
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE souvenirs : +%d (%s), %d dans cette vision, %d en reserve"), Quantite, Pourquoi, SouvenirsDeLaVision, Memoire->Souvenirs);
}

// Au depart d'une vision : les cartes actives, dans la limite de la capacite
void AVespPlayerController::AppliquerDons()
{
	bSecondSouffle = false;
	if (bModePhoto || !Memoire)
	{
		return;		// (les photos montrent AYLIS sans les cartes)
	}
	int32 Place = Capacite();
	FString Emportees;
	for (int32 i = 0; i < VespVeilleur::Nombre; i++)
	{
		const FVespCarte& C = VespVeilleur::Carte(i);
		if (!Memoire->Active(i) || C.Cout > Place)
		{
			continue;
		}
		Place -= C.Cout;
		Emportees += FString(C.Nom) + TEXT(", ");
		const FString Id = C.Id;
		if (Id == TEXT("vigueur")) { Aylis->Stats.PvMax += 25; }
		else if (Id == TEXT("fiole")) { Potions += 2; }
		else if (Id == TEXT("bourse")) { Eclats += 80; }
		else if (Id == TEXT("tranchant")) { Aylis->Stats.Attaque += 4; }
		else if (Id == TEXT("pierre")) { Aylis->Stats.Defense += 2; }
		else if (Id == TEXT("etoile")) { PointsDeCompetence += 1; }
		else if (Id == TEXT("oeil")) { Aylis->Stats.ChanceCritique += 10; }
		else if (Id == TEXT("seve")) { bSeve = true; }
		else if (Id == TEXT("fortune")) { bFortune = true; }
		else if (Id == TEXT("fureur")) { bFureur = true; }
		else if (Id == TEXT("flamme")) { bFlamme = true; }
		else if (Id == TEXT("souffle")) { bSecondSouffle = true; }
	}
	Aylis->Stats.Pv = Aylis->Stats.PvMax;
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE cartes : %s(capacite %d, %d libre)"), Emportees.IsEmpty() ? TEXT("aucune ") : *Emportees, Capacite(), Place);
}

void AVespPlayerController::ActionVeilleur(int32 Ligne)
{
	if (Phase != EVespPhase::Titre || !Memoire || Ligne < 0 || Ligne > VespVeilleur::Nombre)
	{
		return;
	}
	SelectionDon = Ligne;
	auto Refus = [this](const TCHAR* Pourquoi) {
		UVespSons::Jouer2D(this, EVespSon::Survol, 0.5f, 0.7f);
		UE_LOG(LogTemp, Display, TEXT("VESPERANCE veilleur : %s"), Pourquoi);
	};
	// Une chandelle : +1 de capacite
	if (Ligne == 0)
	{
		const int32 Prix = VespVeilleur::PrixChandelle(Memoire->Chandelles);
		if (Prix <= 0 || Memoire->Souvenirs < Prix)
		{
			Refus(TEXT("pas de chandelle"));
			return;
		}
		Memoire->Souvenirs -= Prix;
		Memoire->Chandelles++;
		EcrireMemoire();
		UVespSons::Jouer2D(this, EVespSon::Rune, 0.8f);
		return;
	}
	const int32 i = Ligne - 1;
	const FVespCarte& C = VespVeilleur::Carte(i);
	if (!Memoire->Debloquee(i))
	{
		// Debloquer la carte (et l'activer si elle tient dans la capacite)
		if (Memoire->Souvenirs < C.Prix)
		{
			Refus(TEXT("pas assez de souvenirs"));
			return;
		}
		Memoire->Souvenirs -= C.Prix;
		Memoire->CartesDebloquees[i] = true;
		Memoire->CartesActives[i] = CapaciteUtilisee() + C.Cout <= Capacite();
		UVespSons::Jouer2D(this, EVespSon::Rune, 0.8f);
		UE_LOG(LogTemp, Display, TEXT("VESPERANCE veilleur : carte %s debloquee pour %d souvenirs (reste %d)"), C.Nom, C.Prix, Memoire->Souvenirs);
	}
	else if (Memoire->Active(i))
	{
		Memoire->CartesActives[i] = false;
		UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f, 0.85f);
	}
	else
	{
		if (CapaciteUtilisee() + C.Cout > Capacite())
		{
			Refus(TEXT("capacite pleine"));
			return;
		}
		Memoire->CartesActives[i] = true;
		UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f, 1.15f);
	}
	EcrireMemoire();
}

// ===================== Les options =====================

static const TCHAR* EMPLACEMENT_REGLAGES = TEXT("VesperanceReglages");

void AVespPlayerController::ChargerReglages()
{
	Reglages = Cast<UVespReglages>(UGameplayStatics::LoadGameFromSlot(EMPLACEMENT_REGLAGES, 0));
	if (!Reglages)
	{
		Reglages = Cast<UVespReglages>(UGameplayStatics::CreateSaveGameObject(UVespReglages::StaticClass()));
	}
	AppliquerReglages();
}

void AVespPlayerController::AppliquerReglages()
{
	if (!Reglages)
	{
		return;
	}
	UVespSons::VolumeMusique = FMath::Clamp(Reglages->VolumeMusique, 0.0f, 1.0f);
	UVespSons::VolumeEffets = FMath::Clamp(Reglages->VolumeEffets, 0.0f, 1.0f);
	bSecoussesActives = Reglages->bSecousses;
	if (!bSecoussesActives)
	{
		Secousse = 0.0f;
	}
}

static bool EstEnPleinEcran()
{
	const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return S && S->GetFullscreenMode() != EWindowMode::Windowed;
}

FString AVespPlayerController::NomReglage(int32 Ligne)
{
	static const TCHAR* NOMS[LignesReglages] = {TEXT("MUSIQUE"), TEXT("EFFETS SONORES"), TEXT("PLEIN ÉCRAN"), TEXT("SECOUSSES DE LA CAMÉRA"),
	                                            TEXT("ABANDONNER LA VISION"), TEXT("QUITTER LE JEU")};
	return NOMS[FMath::Clamp(Ligne, 0, LignesReglages - 1)];
}

FString AVespPlayerController::ValeurReglage(int32 Ligne) const
{
	switch (Ligne)
	{
		case 0: return Reglages ? FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Reglages->VolumeMusique * 100.0f)) : FString();
		case 1: return Reglages ? FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Reglages->VolumeEffets * 100.0f)) : FString();
		case 2: return EstEnPleinEcran() ? TEXT("OUI") : TEXT("NON");
		case 3: return Reglages && Reglages->bSecousses ? TEXT("OUI") : TEXT("NON");
		case 4: return bConfirmerAbandon ? TEXT("ENCORE UNE FOIS POUR CONFIRMER") : FString();
		default: return FString();
	}
}

void AVespPlayerController::ChangerReglage(int32 Ligne, int32 Sens)
{
	if (!Reglages)
	{
		return;
	}
	if (Ligne != 4)
	{
		bConfirmerAbandon = false;
	}
	switch (Ligne)
	{
		case 0:
		case 1:
		{
			float& V = Ligne == 0 ? Reglages->VolumeMusique : Reglages->VolumeEffets;
			const float Pas = 0.1f;
			V = Sens == 0 ? (V >= 0.99f ? 0.0f : V + Pas) : V + Sens * Pas;		// un clic fait le tour (0 apres 100 %)
			V = FMath::Clamp(FMath::RoundToFloat(V * 10.0f) / 10.0f, 0.0f, 1.0f);
			break;
		}
		case 2:
			if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
			{
				S->SetFullscreenMode(EstEnPleinEcran() ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen);
				S->ApplySettings(false);
				S->SaveSettings();
			}
			break;
		case 3:
			Reglages->bSecousses = !Reglages->bSecousses;
			break;
		case 4:
			if (Phase == EVespPhase::Titre || Sens != 0)
			{
				return;
			}
			if (!bConfirmerAbandon)
			{
				bConfirmerAbandon = true;		// une seconde fois pour de bon
				UVespSons::Jouer2D(this, EVespSon::Annonce, 0.6f);
				return;
			}
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE vision %d abandonnee a l'acte %s apres %s"), NumeroVision, *Romain(Acte), *UVespSauvegarde::Duree(ChronoPartie));
			bMenuOuvert = false;
			EffacerPartie();
			Recommencer();
			return;
		default:
			if (Sens == 0)
			{
				Quitter();
			}
			return;
	}
	AppliquerReglages();
	if (!bModePhoto && !bTestOptions)
	{
		UGameplayStatics::SaveGameToSlot(Reglages, EMPLACEMENT_REGLAGES, 0);
	}
	UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f);
}

// Les options au clavier et a la manette : haut / bas pour choisir, gauche / droite pour regler, ENTREE / A pour valider
void AVespPlayerController::CommandesDesReglages(float Secondes, int32 Nombre)
{
	FIntPoint D;
	if (DirectionPressee(D, Secondes))
	{
		if (D.Y != 0)
		{
			SelectionReglage = FMath::Clamp(SelectionReglage + D.Y, 0, Nombre - 1);
			bConfirmerAbandon = false;
			UVespSons::Jouer2D(this, EVespSon::Survol);
		}
		else if (D.X != 0 && SelectionReglage <= 3)
		{
			ChangerReglage(SelectionReglage, SelectionReglage <= 1 ? D.X : 0);
		}
	}
	if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		ChangerReglage(FMath::Clamp(SelectionReglage, 0, Nombre - 1), 0);
	}
}

void AVespPlayerController::Quitter()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

// ===================== Les noms (salles, runes, lieux) =====================

FString AVespPlayerController::NomSalle(EVespSalle S, int32 LActe)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Combat");
		case EVespSalle::Elite: return TEXT("Élite");
		case EVespSalle::Repos: return TEXT("Feu de camp");
		case EVespSalle::Marchand: return TEXT("Marchand");
		case EVespSalle::Evenement: return TEXT("Inconnu");
		case EVespSalle::Depart: return TEXT("Depart");
		case EVespSalle::Tresor: return TEXT("Trésor");
		default: return AVespCombat::NomDuBoss(LActe);
	}
}

FString AVespPlayerController::AideSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("Des Haschen, parfois en plusieurs vagues. À la fin : des éclats, et la récompense annoncée sur la balise.");
		case EVespSalle::Elite: return TEXT("Un Haschen d'élite et son escorte. Dur... mais beaucoup d'éclats.");
		case EVespSalle::Repos: return TEXT("Un feu de camp : +60% pv et une potion.");
		case EVespSalle::Marchand: return TEXT("Un marchand ambulant. Des potions, des dons de maîtres... contre des éclats.");
		case EVespSalle::Evenement: return TEXT("La vision est trouble. Une rencontre, un trésor... ou un piège.");
		case EVespSalle::Depart: return TEXT("Le début de la route.");
		case EVespSalle::Tresor: return TEXT("Un coffre, au bout du chemin : des éclats, et un maître se manifeste.");
		default: return TEXT("Le gardien de cette terre.");
	}
}

FString AVespPlayerController::LettreSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return TEXT("X");
		case EVespSalle::Elite: return TEXT("E");
		case EVespSalle::Repos: return TEXT("R");
		case EVespSalle::Marchand: return TEXT("$");
		case EVespSalle::Evenement: return TEXT("?");
		case EVespSalle::Depart: return TEXT("");
		case EVespSalle::Tresor: return TEXT("T");
		default: return TEXT("B");
	}
}

FLinearColor AVespPlayerController::CouleurSalle(EVespSalle S)
{
	switch (S)
	{
		case EVespSalle::Combat: return FLinearColor(0.92f, 0.42f, 0.36f);
		case EVespSalle::Elite: return FLinearColor(0.95f, 0.7f, 0.25f);
		case EVespSalle::Repos: return FLinearColor(0.48f, 0.85f, 0.52f);
		case EVespSalle::Marchand: return FLinearColor(0.45f, 0.8f, 0.95f);
		case EVespSalle::Evenement: return FLinearColor(0.75f, 0.58f, 1.0f);
		case EVespSalle::Depart: return FLinearColor(0.73f, 0.55f, 1.0f);
		case EVespSalle::Tresor: return FLinearColor(1.0f, 0.85f, 0.4f);
		default: return FLinearColor(1.0f, 0.3f, 0.3f);
	}
}

FString AVespPlayerController::NomRecompense(EVespRecompense R)
{
	switch (R)
	{
		case EVespRecompense::Rune: return TEXT("RUNE");
		case EVespRecompense::Eclats: return TEXT("ÉCLATS");
		case EVespRecompense::Objet: return TEXT("OBJET");
		case EVespRecompense::Fiole: return TEXT("FIOLE");
		case EVespRecompense::Etoile: return TEXT("ÉTOILE");
		case EVespRecompense::Souvenirs: return TEXT("SOUVENIRS");
		case EVespRecompense::Felure: return TEXT("FÊLURE");
		default: return FString();
	}
}

FLinearColor AVespPlayerController::CouleurRecompense(EVespRecompense R)
{
	switch (R)
	{
		case EVespRecompense::Rune: return FLinearColor(0.73f, 0.55f, 1.0f);
		case EVespRecompense::Eclats: return FLinearColor(0.95f, 0.8f, 0.35f);
		case EVespRecompense::Objet: return FLinearColor(0.45f, 0.66f, 1.0f);
		case EVespRecompense::Fiole: return FLinearColor(0.95f, 0.45f, 0.5f);
		case EVespRecompense::Etoile: return FLinearColor(0.7f, 0.92f, 1.0f);
		case EVespRecompense::Souvenirs: return FLinearColor(0.82f, 0.74f, 1.0f);
		case EVespRecompense::Felure: return VespMaitres::Couleur(VespMaitres::Liss);
		default: return FLinearColor::White;
	}
}

FString AVespPlayerController::EtiquetteRecompense(const FVespNoeud& N)
{
	if (N.Recompense == EVespRecompense::Rune && N.Maitre >= 0)
	{
		return VespMajuscules(VespMaitres::Nom(N.Maitre));
	}
	return N.Recompense == EVespRecompense::Felure ? FString(TEXT("LISS")) : NomRecompense(N.Recompense);
}

FLinearColor AVespPlayerController::CouleurDeLaRecompense(const FVespNoeud& N)
{
	return N.Recompense == EVespRecompense::Rune && N.Maitre >= 0 ? VespMaitres::Couleur(N.Maitre) : CouleurRecompense(N.Recompense);
}

// La recompense d'une clairiere de combat (tiree avec la carte : une vision reprise retrouve les memes)
static EVespRecompense TirerRecompense(bool bElite, FRandomStream& Hasard)
{
	// un maitre, eclats, objet, fiole, etoile, souvenirs, la Felure
	static const int32 POIDS[7] = {38, 13, 16, 12, 7, 9, 5};
	static const int32 POIDS_ELITE[7] = {30, 9, 26, 10, 12, 8, 5};
	const int32* P = bElite ? POIDS_ELITE : POIDS;
	int32 Tirage = Hasard.RandRange(0, 99);
	for (int32 i = 0; i < 7; i++)
	{
		if (Tirage < P[i])
		{
			return (EVespRecompense)(i + 1);
		}
		Tirage -= P[i];
	}
	return EVespRecompense::Rune;
}

FString AVespPlayerController::Romain(int32 Nombre)
{
	static const TCHAR* R[] = {TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI"), TEXT("VII")};
	return R[FMath::Clamp(Nombre, 1, 7) - 1];
}

FString AVespPlayerController::NomDuLieu() const
{
	return Noeuds.IsValidIndex(ZoneActuelle) && Noeuds[ZoneActuelle].Type == EVespSalle::Boss && !Noeuds[ZoneActuelle].bVisite
	           ? InfoActe(Acte).LieuDuBoss : InfoActe(Acte).Lieu;
}

FString AVespPlayerController::NomDeLActe() const { return InfoActe(Acte).Titre; }
FString AVespPlayerController::RegleDeLActe() const { return InfoActe(Acte).Regle; }
FString AVespPlayerController::NomDuBoss() const { return AVespCombat::NomDuBoss(Acte); }

// ===================== Les evenements (les salles "?") =====================
// Toujours sans genre pour AYLIS.

struct FVespTexteEvenement
{
	const TCHAR* Titre;
	const TCHAR* Texte;
	const TCHAR* Choix[2];
	const TCHAR* Aide[2];
	int32 ActeMin;
	int32 ActeMax;
};

static const FVespTexteEvenement EVENEMENTS[] = {
	{TEXT("L'autel oublié"),
	 TEXT("Une pierre couverte de runes, a moitié avalee par la mousse. Une voix murmure : du sang contre une vision."),
	 {TEXT("Offrir son sang"), TEXT("Passer son chemin")}, {TEXT("-8 pv, un don au hasard"), TEXT("Rien ne se passe")}, 1, 7},
	{TEXT("La source claire"),
	 TEXT("Une eau si pure qu'elle brille dans la nuit. Les feux follets tournent autour sans oser la toucher."),
	 {TEXT("Boire"), TEXT("Remplir une fiole")}, {TEXT("+35% pv"), TEXT("+1 potion")}, 1, 7},
	{TEXT("Le Haschen blessé"),
	 TEXT("Un eclaireur Haschen, adosse a un arbre. Il ne peut plus se battre. Il te regarde sans rien dire."),
	 {TEXT("L'achever"), TEXT("L'epargner")}, {TEXT("+25 éclats"), TEXT("+6 pv max : la prophétie s'en souviendra")}, 1, 7},
	{TEXT("Le coffre sous la mousse"),
	 TEXT("Un coffre a moitié enterré. La serrure est rouillée... ou piégée ? Quelque chose bouge dans les fourrés."),
	 {TEXT("L'ouvrir"), TEXT("Le laisser")}, {TEXT("Une chance sur deux : 45 éclats... ou une embuscade"), TEXT("Rien ne se passe")}, 1, 7},
	{TEXT("Le colporteur des brumes"),
	 TEXT("Une silhouette encapuchonnee sort du brouillard. Elle tend une main pleine de runes et reclame tes potions."),
	 {TEXT("Échanger 2 potions"), TEXT("Refuser")}, {TEXT("-2 potions, un don au hasard"), TEXT("La silhouette disparaît")}, 1, 7},
	{TEXT("Les pendus"),
	 TEXT("Des cordes grincent au-dessus du sentier. L'un des pendus ouvre les yeux et murmure le nom d'AYLIS."),
	 {TEXT("Écouter"), TEXT("Couper la corde")}, {TEXT("-10 pv, +2 attaque"), TEXT("+30 éclats")}, 2, 3},
	{TEXT("Le feu des voyageurs"),
	 TEXT("Un feu encore tiede, abandonne en hate. Des provisions, et des sacs a moitié ouverts."),
	 {TEXT("Manger"), TEXT("Fouiller les sacs")}, {TEXT("+20% pv"), TEXT("+20 éclats")}, 1, 7},
	{TEXT("Le déserteur"),
	 TEXT("Un jeune Haschen sans arme tremble derriere un rocher. Il a fui le camp d'Ashka."),
	 {TEXT("L'aider"), TEXT("Le chasser")}, {TEXT("-1 potion, +35 éclats"), TEXT("Il s'enfuit dans la nuit")}, 4, 6},
	{TEXT("La cloche engloutie"),
	 TEXT("Une cloche rouillée dépasse de la vase. Si on la sonne, quelque chose répondra."),
	 {TEXT("Sonner la cloche"), TEXT("La laisser dormir")}, {TEXT("Une embuscade d'élite (et sa récompense)"), TEXT("Rien ne se passe")}, 3, 3},
	{TEXT("L'abri de pierre"),
	 TEXT("Le blizzard se lève. Un abri de pierre, à peine assez grand, et des traces de pas qui continuent dans la neige."),
	 {TEXT("S'abriter"), TEXT("Suivre les traces")}, {TEXT("+30% pv"), TEXT("-8 pv, +30 éclats")}, 5, 5},
	{TEXT("L'autel de braise"),
	 TEXT("Un autel de pierre noire, brûlant. C'est ici que Vorgath trempe ses lames."),
	 {TEXT("Tremper l'épée"), TEXT("Refroidir l'autel")}, {TEXT("-12 pv, +3 attaque"), TEXT("+1 défense")}, 6, 6},
	{TEXT("Le miroir du Voile"),
	 TEXT("Une flaque d'argent reflete AYLIS... mais le reflet sourit, et tend la main."),
	 {TEXT("Toucher le reflet"), TEXT("Briser le miroir")}, {TEXT("Une chance sur deux : un don... ou des échos"), TEXT("+40 éclats")}, 7, 7},
	{TEXT("La statue d'Ashka"),
	 TEXT("Une statue de la cheffe de guerre, couronnée d'épines. À ses pieds, des offrandes de ses guerriers."),
	 {TEXT("Prendre les offrandes"), TEXT("Briser la statue")}, {TEXT("+35 éclats, -6 pv"), TEXT("+1 attaque")}, 4, 5},
};
static constexpr int32 NOMBRE_EVENEMENTS = UE_ARRAY_COUNT(EVENEMENTS);

// Le dernier choix, devant le coeur du Voile (pas un evenement de la route : EvenementActuel = COEUR_DU_VOILE)
static constexpr int32 COEUR_DU_VOILE = -1;
static const TCHAR* CHOIX_DU_VOILE[2] = {TEXT("Se dissoudre dans le Voile"), TEXT("Refuser, et garder la porte")};
static const TCHAR* AIDE_DU_VOILE[2] = {
	TEXT("Le Voile se referme et les sept terres se réveillent. AYLIS s'efface. C'est la fin de la route."),
	TEXT("Comme l'Oracle avant. AYLIS devient le nouvel Oracle, et la boucle continue."),
};

FString AVespPlayerController::TitreEvenement() const { return EvenementActuel == COEUR_DU_VOILE ? TEXT("Le cœur du Voile") : EVENEMENTS[EvenementActuel].Titre; }
FString AVespPlayerController::TexteEvenement() const
{
	return EvenementActuel == COEUR_DU_VOILE ? TEXT("Mille visions sont tombées sur cette route. Une seule peut refermer le Voile : celle qui accepte de cesser d'exister.")
	                                          : EVENEMENTS[EvenementActuel].Texte;
}
FString AVespPlayerController::ChoixEvenement(int32 Choix) const { return EvenementActuel == COEUR_DU_VOILE ? CHOIX_DU_VOILE[Choix & 1] : EVENEMENTS[EvenementActuel].Choix[Choix]; }
FString AVespPlayerController::AideEvenement(int32 Choix) const { return EvenementActuel == COEUR_DU_VOILE ? AIDE_DU_VOILE[Choix & 1] : EVENEMENTS[EvenementActuel].Aide[Choix]; }

// ===================== Le monde de l'acte =====================
// Un sentier principal d'ouest en est : 9 clairieres, du depart jusqu'au boss. Et des embranchements de 1 a 3
// clairieres vers le nord ou le sud, qui finissent sur un tresor ou une elite.

static EVespSalle SalleDEmbranchement(bool bDerniere, FRandomStream& Hasard)
{
	const int32 D = Hasard.RandRange(0, 99);
	if (bDerniere)
	{
		return D < 55 ? EVespSalle::Tresor : EVespSalle::Elite;
	}
	if (D < 45) return EVespSalle::Combat;
	if (D < 60) return EVespSalle::Elite;
	if (D < 82) return EVespSalle::Evenement;
	if (D < 94) return EVespSalle::Marchand;
	return EVespSalle::Repos;
}

void AVespPlayerController::GenererMonde()
{
	// La carte est tiree avec une graine : une vision reprise retrouve la meme
	GraineCarte = GraineCarteImposee != 0 ? GraineCarteImposee : 1 + (int32)(FPlatformTime::Cycles() % 1000000);
	GraineCarteImposee = 0;
	HasardCarte.Initialize(GraineCarte);
	Noeuds.Reset();
	ZoneActuelle = -1;
	MarchandZone = -1;
	Offres.Reset();
	const FVector Base = Monde->GetActorLocation();
	const float Pas = 2900.0f;
	// Le sentier principal
	const EVespSalle Principal[9] = {
		EVespSalle::Depart, EVespSalle::Combat, (HasardCarte.FRand() < 0.5f) ? EVespSalle::Combat : EVespSalle::Evenement, EVespSalle::Elite,
		(HasardCarte.FRand() < 0.5f) ? EVespSalle::Marchand : EVespSalle::Evenement, EVespSalle::Combat, EVespSalle::Combat, EVespSalle::Repos, EVespSalle::Boss};
	for (int32 i = 0; i < 9; i++)
	{
		FVespNoeud N;
		N.Type = Principal[i];
		N.Etage = i;
		N.Centre = Base + FVector(i > 0 && i < 8 ? HasardCarte.FRandRange(-450.0f, 450.0f) : 0.0f, i * Pas, 0);
		if (i > 0)
		{
			Noeuds[i - 1].Suivants.Add(i);
		}
		Noeuds.Add(N);
	}
	// Les embranchements (au moins 3)
	int32 Branches = 0;
	for (int32 i = 1; i <= 6; i++)
	{
		if (HasardCarte.RandRange(0, 99) >= 70 && !(i >= 5 && Branches < 3))
		{
			continue;
		}
		const float Cote = (HasardCarte.FRand() < 0.5f) ? 1.0f : -1.0f;
		const int32 Longueur = HasardCarte.RandRange(1, 3);
		const FVector Depuis = Noeuds[i].Centre;
		const FVector Positions[3] = {
			FVector(Base.X + Cote * (2700.0f + HasardCarte.FRandRange(-250.0f, 250.0f)), Depuis.Y + 1000.0f, Base.Z),
			FVector(Base.X + Cote * 5200.0f, Depuis.Y + 1800.0f, Base.Z),
			FVector(Base.X + Cote * 7700.0f, Depuis.Y + 1000.0f, Base.Z),
		};
		int32 Precedent = i;
		for (int32 k = 0; k < Longueur; k++)
		{
			FVespNoeud N;
			N.Type = SalleDEmbranchement(k == Longueur - 1, HasardCarte);
			N.Etage = i + k + 1;
			N.Centre = Positions[k];
			const int32 Nouveau = Noeuds.Add(N);
			Noeuds[Precedent].Suivants.Add(Nouveau);
			Precedent = Nouveau;
		}
		Branches++;
	}
	Noeuds[0].bVisite = true;

	// Chaque clairiere de combat annonce sa recompense
	for (FVespNoeud& N : Noeuds)
	{
		if (N.Type == EVespSalle::Combat || N.Type == EVespSalle::Elite)
		{
			N.Recompense = TirerRecompense(N.Type == EVespSalle::Elite, HasardCarte);
			N.Maitre = N.Recompense == EVespRecompense::Rune ? HasardCarte.RandRange(0, VespMaitres::Nombre - 1) : -1;
		}
	}

	// On construit le monde (c'est le chargement de l'acte)
	TArray<FVespZone> Zones;
	TArray<FVespCouloir> Couloirs;
	TArray<FVespLieu> Lieux;
	for (int32 i = 0; i < Noeuds.Num(); i++)
	{
		FVespNoeud& N = Noeuds[i];
		N.Rayon = N.Type == EVespSalle::Boss ? 1300.0f : ((N.Type == EVespSalle::Combat || N.Type == EVespSalle::Elite) ? 1050.0f : 850.0f);
		FVespZone Z;
		Z.Centre = N.Centre;
		Z.Type = (int32)N.Type;
		Z.Lettre = LettreSalle(N.Type);
		Z.Couleur = CouleurSalle(N.Type);
		Z.Rayon = N.Rayon;
		Z.Recompense = EtiquetteRecompense(N);
		Z.CouleurRecompense = CouleurDeLaRecompense(N);
		Zones.Add(Z);
		FVespLieu L;
		L.Centre = N.Centre;
		L.Rayon = N.Rayon;
		L.Type = (int32)N.Type;
		L.Etage = N.Etage;
		L.Suivants = N.Suivants;
		Lieux.Add(L);
		for (int32 S : N.Suivants)
		{
			FVespCouloir C;
			C.A = i;
			C.B = S;
			Couloirs.Add(C);
		}
	}
	Monde->Construire(Acte, Zones, Couloirs);
	Combat->Peupler(Acte, Lieux);

	// AYLIS au depart, tournee vers l'est
	Aylis->Teleporter(Noeuds[0].Centre + FVector(0, -250.0f, 0));
	Aylis->SetActorRotation(FRotator(0, 90.0f, 0));
	Geste = EVespGesteAylis::Libre;
	bCaleCamera = true;
	bCarteOuverte = false;
}

// AYLIS entre dans une clairiere sans Haschen
void AVespPlayerController::Declencher(int32 Zone)
{
	ZoneActuelle = Zone;
	FVespNoeud& N = Noeuds[Zone];
	switch (N.Type)
	{
		case EVespSalle::Evenement:
			N.bVisite = true;
			Monde->MarquerZoneFaite(Zone);
			OuvrirEvenement();
			return;
		case EVespSalle::Marchand:
			N.bVisite = true;
			OuvrirMarchand();
			return;
		case EVespSalle::Repos:
			N.bVisite = true;
			Monde->MarquerZoneFaite(Zone);
			ParlerAuVeilleur();		// Oswin parle, puis AYLIS se repose
			return;
		case EVespSalle::Tresor:
		{
			N.bVisite = true;
			Monde->MarquerZoneFaite(Zone);
			const int32 Gain = 30 + Acte * 8 + FMath::RandRange(0, 10);
			GagnerEclats(Gain, N.Centre);
			UVespSons::Jouer(this, EVespSon::Coffre, N.Centre);
			Aylis->Jouer(EVespGeste::Interagir, 1.2f);
			LacherButin(N.Centre + FVector(0, 0, 40), 1);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, N.Centre + FVector(0, 0, 80), FVector::UpVector, FLinearColor(1.0f, 0.85f, 0.4f));
			MessageRoute = FString::Printf(TEXT("Le coffre s'ouvre : +%d éclats... et un maître se manifeste."), Gain);
			ProposerDons(FMath::RandRange(0, VespMaitres::Nombre - 1), false);
			return;
		}
		default:
			return;
	}
}

// Retour au jeu (apres un menu : runes, marchand, evenement, dialogue)
void AVespPlayerController::RetourExploration()
{
	Phase = EVespPhase::Exploration;
	TempsPhase = 0.0f;
	TempsMessage = MessageRoute.IsEmpty() ? 0.0f : 5.0f;
	SauverPartie();
}

FString AVespPlayerController::Invite() const
{
	if (Phase == EVespPhase::Exploration && MarchandProche >= 0 && !Combat->EnCombat())
	{
		return TEXT("E  /  (B)  :  parler au marchand");
	}
	return FString();
}

// ===================== Le marchand =====================

void AVespPlayerController::OuvrirMarchand()
{
	Offres.Reset();
	auto Ajouter = [this](const FString& Nom, const FString& Aide, int32 Prix, int32 Genre, int32 R = 0, int32 Rarete = 0) {
		FVespOffre O;
		O.Nom = Nom;
		O.Aide = Aide;
		O.Prix = Prix + Acte * 4 + FMath::RandRange(-3, 3);
		O.Genre = Genre;
		O.Rune = R;
		O.Rarete = Rarete;
		Offres.Add(O);
	};
	int32 Don = 0, Rarete = 0;
	DonAuHasard(Don, Rarete);
	const VespMaitres::FDon& DM = VespMaitres::Don(Don);
	Ajouter(TEXT("Potion"), TEXT("+1 potion (40% des pv)"), 16, 0);
	Ajouter(FString::Printf(TEXT("%s (%s)"), DM.Nom, VespMaitres::Nom(DM.Maitre)), VespMaitres::Description(Don, Rarete), 42 + Rarete * 18, 1, Don, Rarete);
	TArray<int32> Autres = {2, 3, 4, 5};
	for (int32 k = 0; k < 2; k++)
	{
		const int32 Autre = Autres[FMath::RandRange(0, Autres.Num() - 1)];
		Autres.Remove(Autre);
		switch (Autre)
		{
			case 2: Ajouter(TEXT("Onguent"), TEXT("Rend 50% des pv"), 20, 2); break;
			case 3: Ajouter(TEXT("Pierre a aiguiser"), TEXT("+1 attaque"), 28, 3); break;
			case 4: Ajouter(TEXT("Amulette de sureau"), TEXT("+6 pv max"), 26, 4); break;
			default: Ajouter(TEXT("Bouclier cloute"), TEXT("+1 défense"), 30, 5); break;
		}
	}
	MarchandZone = ZoneActuelle;
	SelectionMenu = 0;
	Phase = EVespPhase::Marchand;
	TempsPhase = 0.0f;
}

void AVespPlayerController::RouvrirMarchand(int32 Zone)
{
	ZoneActuelle = Zone;
	if (Zone != MarchandZone || Offres.Num() == 0)
	{
		OuvrirMarchand();
		return;
	}
	SelectionMenu = 0;
	Phase = EVespPhase::Marchand;
	TempsPhase = 0.0f;
}

void AVespPlayerController::AcheterOffre(int32 Numero)
{
	if (Phase != EVespPhase::Marchand || !Offres.IsValidIndex(Numero))
	{
		return;
	}
	FVespOffre& O = Offres[Numero];
	if (O.bVendue || Eclats < O.Prix)
	{
		return;
	}
	Eclats -= O.Prix;
	O.bVendue = true;
	switch (O.Genre)
	{
		case 0: Potions++; break;
		case 1: PrendreDon(O.Rune, O.Rarete); break;
		case 2: Aylis->Soigner(Aylis->Stats.PvMax / 2); break;
		case 3: Aylis->Stats.Attaque += 1; break;
		case 4: Aylis->Stats.PvMax += 6; Aylis->Soigner(6); break;
		default: Aylis->Stats.Defense += 1; break;
	}
	UVespSons::Jouer2D(this, EVespSon::Eclats);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Aylis->GetActorLocation() + FVector(0, 0, 100), FVector::UpVector, FLinearColor(1.0f, 0.8f, 0.35f));
}

void AVespPlayerController::QuitterMarchand()
{
	if (Phase == EVespPhase::Marchand)
	{
		MessageRoute = TEXT("\"Reviens quand tu veux. Je ne bouge pas d'ici.\"");
		RetourExploration();
	}
}

// ===================== Les evenements =====================

void AVespPlayerController::OuvrirEvenement()
{
	TArray<int32> Possibles;
	for (int32 i = 0; i < NOMBRE_EVENEMENTS; i++)
	{
		if (Acte >= EVENEMENTS[i].ActeMin && Acte <= EVENEMENTS[i].ActeMax)
		{
			Possibles.Add(i);
			if (EVENEMENTS[i].ActeMin == EVENEMENTS[i].ActeMax)
			{
				Possibles.Add(i);		// les evenements propres a un acte reviennent plus souvent
			}
		}
	}
	EvenementActuel = Possibles[FMath::RandRange(0, Possibles.Num() - 1)];
	SelectionMenu = 0;
	Phase = EVespPhase::Evenement;
	TempsPhase = 0.0f;
}

void AVespPlayerController::ChoisirEvenement(int32 Choix)
{
	if (Phase != EVespPhase::Evenement || TempsPhase < 0.4f)
	{
		return;
	}
	UVespSons::Jouer2D(this, EVespSon::Clic);
	if (EvenementActuel == COEUR_DU_VOILE)
	{
		TerminerLaVision(Choix == 0 ? 2 : 1);
		return;
	}
	const FVector Ici = Aylis->GetActorLocation() + FVector(0, 0, 100);
	auto PerdrePv = [this](int32 N) { Aylis->Stats.Pv = FMath::Max(1, Aylis->Stats.Pv - N); };
	auto RuneOfferte = [this](const TCHAR* Debut) {
		int32 Don = 0, Rarete = 0;
		DonAuHasard(Don, Rarete);
		PrendreDon(Don, Rarete);
		const VespMaitres::FDon& DM = VespMaitres::Don(Don);
		MessageRoute = FString::Printf(TEXT("%s le don « %s » de %s : %s"), Debut, DM.Nom, VespMaitres::Nom(DM.Maitre), *VespMaitres::Description(Don, Rarete));
	};
	auto Embuscade = [this](bool bElite) {
		Combat->Embuscade(Aylis->GetActorLocation(), bElite);
		RetourExploration();
	};
	MessageRoute = TEXT("AYLIS reprend la route.");
	switch (EvenementActuel * 2 + Choix)
	{
		case 0:
			PerdrePv(8);
			RuneOfferte(TEXT("L'autel boit le sang... et offre"));
			AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, Ici, FVector::UpVector, FLinearColor(0.9f, 0.2f, 0.3f));
			break;
		case 2:
		{
			const int32 Soin = Aylis->Stats.PvMax * 35 / 100;
			Aylis->Soigner(Soin);
			MessageRoute = FString::Printf(TEXT("L'eau est glacée et douce. +%d pv."), Soin);
			break;
		}
		case 3: Potions++; MessageRoute = TEXT("Une fiole de lumière liquide : +1 potion."); break;
		case 4: GagnerEclats(25, Ici); MessageRoute = TEXT("Il ne dit rien, jusqu'au bout. +25 éclats."); break;
		case 5:
			Aylis->Stats.PvMax += 6;
			Aylis->Soigner(6);
			MessageRoute = TEXT("Le Haschen disparaît dans la brume. +6 pv max : la prophétie s'en souviendra.");
			break;
		case 6:
			if (FMath::RandBool())
			{
				GagnerEclats(45, Ici);
				MessageRoute = TEXT("Le coffre cède : +45 éclats !");
				AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Ici, FVector::UpVector, FLinearColor(1.0f, 0.8f, 0.35f));
				break;
			}
			MessageRoute = TEXT("Une embuscade ! Les fourrés s'ouvrent...");
			Embuscade(false);
			return;
		case 8:
			if (Potions >= 2)
			{
				Potions -= 2;
				RuneOfferte(TEXT("Marche conclu :"));
			}
			else
			{
				MessageRoute = TEXT("Pas assez de potions... la silhouette s'efface en riant.");
			}
			break;
		case 10:
			PerdrePv(10);
			Aylis->Stats.Attaque += 2;
			MessageRoute = TEXT("Le murmure brûle... mais l'épée semble plus lourde de colère. +2 attaque.");
			AVespEffet::Jouer(GetWorld(), EVespEffet::Mort, Ici, FVector::UpVector, FLinearColor(0.5f, 1.0f, 0.4f));
			break;
		case 11: GagnerEclats(30, Ici); MessageRoute = TEXT("Le corps tombe dans la mousse. Dans sa poche : +30 éclats."); break;
		case 12:
		{
			const int32 Soin = Aylis->Stats.PvMax / 5;
			Aylis->Soigner(Soin);
			MessageRoute = FString::Printf(TEXT("Un vrai repas, enfin. +%d pv."), Soin);
			break;
		}
		case 13: GagnerEclats(20, Ici); MessageRoute = TEXT("Au fond d'un sac : +20 éclats."); break;
		case 14:
			if (Potions > 0)
			{
				Potions--;
				GagnerEclats(35, Ici);
				MessageRoute = TEXT("Il boit, et glisse 35 éclats dans ta main : \"Ashka a peur de la prophétie... elle a peur de toi.\"");
			}
			else
			{
				GagnerEclats(10, Ici);
				MessageRoute = TEXT("Tu n'as rien à lui donner. Il file quand même, en laissant tomber 10 éclats.");
			}
			break;
		case 16:
			MessageRoute = TEXT("La cloche sonne sous la vase... et quelque chose répond.");
			Embuscade(true);
			return;
		case 18:
		{
			const int32 Soin = Aylis->Stats.PvMax * 30 / 100;
			Aylis->Soigner(Soin);
			MessageRoute = FString::Printf(TEXT("Le vent hurle dehors. A l'abri, AYLIS reprend des forces : +%d pv."), Soin);
			break;
		}
		case 19:
			PerdrePv(8);
			GagnerEclats(30, Ici);
			MessageRoute = TEXT("Les traces mènent à un campement gelé. -8 pv, +30 éclats.");
			break;
		case 20:
			PerdrePv(12);
			Aylis->Stats.Attaque += 3;
			MessageRoute = TEXT("La lame rougit, puis noircit. -12 pv, +3 attaque.");
			AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, Ici, FVector::UpVector, FLinearColor(1.0f, 0.45f, 0.1f));
			break;
		case 21: Aylis->Stats.Defense += 1; MessageRoute = TEXT("La pierre refroidit en sifflant. +1 défense."); break;
		case 22:
			if (FMath::RandBool())
			{
				RuneOfferte(TEXT("Le reflet se fond dans AYLIS et laisse"));
				break;
			}
			MessageRoute = TEXT("Le reflet sort du miroir... et il n'est pas seul.");
			Embuscade(false);
			return;
		case 23: GagnerEclats(40, Ici); MessageRoute = TEXT("Le miroir vole en éclats : +40 éclats."); break;
		case 24: PerdrePv(6); GagnerEclats(35, Ici); MessageRoute = TEXT("Les épines de la statue griffent AYLIS. -6 pv, +35 éclats."); break;
		case 25: Aylis->Stats.Attaque += 1; MessageRoute = TEXT("La couronne d'épines roule dans la poussière. +1 attaque."); break;
		default: break;
	}
	RetourExploration();
}

// ===================== Les combats (ce que le combat nous annonce) =====================

int32 AVespPlayerController::EclatsAvecBonus(int32 Quantite) const
{
	return FMath::RoundToInt(Quantite * (1.0f + (bFortune ? 0.5f : 0.0f) + ValeurDon(TEXT("a_passif")) / 100.0f));
}

void AVespPlayerController::GagnerEclats(int32 Quantite, const FVector& Ou)
{
	const int32 Gain = EclatsAvecBonus(Quantite);
	Eclats += Gain;
	UVespSons::Jouer(this, EVespSon::Eclats, Ou, 0.6f);
}

void AVespPlayerController::GagnerXp(int32 Quantite)
{
	Xp += Quantite;
	while (Xp >= XpPourNiveau(Niveau))
	{
		Xp -= XpPourNiveau(Niveau);
		Niveau++;
		PointsDeCompetence++;
		Aylis->Stats.PvMax += 5;
		Aylis->Stats.Attaque += 1;
		if (Niveau % 3 == 0)
		{
			Aylis->Stats.Defense += 1;
		}
		Aylis->Soigner(5 + Aylis->Stats.PvMax / 4);
		TempsNiveau = 3.5f;
		UVespSons::Jouer2D(this, EVespSon::Niveau, 0.9f);
		AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Buff"), Aylis->GetActorLocation(), FRotator::ZeroRotator, 1.3f);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(1.0f, 0.85f, 0.4f));
		Aylis->Surbrillance(0.5f, FLinearColor(1.0f, 0.85f, 0.4f));
	}
}

void AVespPlayerController::QuandHaschenTombe(AVespUnite* H, int32 Categorie)
{
	HaschenVaincus++;
	const bool bBoss = H->Stats.Boss > 0;
	// Le butin : parfois pour un Haschen, toujours pour une elite (rare ou mieux), deux pieces pour un boss
	if (Categorie == 2)
	{
		LacherButin(H->GetActorLocation(), 2);
		LacherButin(H->GetActorLocation(), 2);
	}
	else if (Categorie == 1 || HasardButin.FRand() < 0.12f)
	{
		LacherButin(H->GetActorLocation(), Categorie);
	}
	GagnerXp(bBoss ? 150 + 40 * Acte : 6 + H->Stats.PvMax / 3);
	GagnerEclats(bBoss ? 0 : 2 + H->Stats.PvMax / 10, H->GetActorLocation());
	Rage = FMath::Min(100, Rage + (bFureur ? 16 : 8));
	if (bSeve)
	{
		Aylis->Soigner(5);
	}
	if (const float Seve = ValeurDon(TEXT("o_passif")))
	{
		Aylis->Soigner(FMath::RoundToInt(Seve * MultSoins()));
	}
	AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, H->GetActorLocation() + FVector(0, 0, 60), FVector::UpVector, FLinearColor(1.0f, 0.82f, 0.4f));
	// Parfois, une fiole roule au sol
	if (!bBoss && FMath::FRand() < 0.07f)
	{
		Potions++;
		H->AfficherMessage(TEXT("+1 potion"), FColor(255, 150, 150), 34.0f);
		UVespSons::Jouer(this, EVespSon::Ramasser, H->GetActorLocation(), 0.8f);
	}
	if (!Combat->EnCombat())
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	}
}

void AVespPlayerController::QuandAylisTouchee(int32 Degats, bool bCritique, AVespUnite* Source)
{
	if (Degats <= 0)
	{
		return;
	}
	Rage = FMath::Min(100, Rage + Degats * (bFureur ? 6 : 3));
	Trembler(bCritique ? 0.9f : 0.45f);
	if (bCritique)
	{
		Ralenti(0.3f, 0.1f);
	}
	if (!Aylis->EstDebout())
	{
		// Le don du Veilleur : une fois par vision, AYLIS se releve
		if (bSecondSouffle && Phase != EVespPhase::Defaite)
		{
			bSecondSouffle = false;
			Aylis->Relever(Aylis->Stats.PvMax / 2);
			Geste = EVespGesteAylis::Libre;
			bGardeLevee = false;
			Trembler(1.0f);
			Ralenti(0.25f, 0.8f);
			UVespSons::Jouer2D(this, EVespSon::Niveau, 0.9f);
			AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Buff"), Aylis->GetActorLocation(), FRotator::ZeroRotator, 1.3f);
			Ecrire(TEXT("Second souffle : le Veilleur retient la vision. AYLIS se relève."), 4.0f);
			return;
		}
		if (Phase != EVespPhase::Defaite)
		{
			Phase = EVespPhase::Defaite;
			TempsPhase = 0.0f;
			MemoriserLaChute();
			Ralenti(0.2f, 0.6f);
			if (UVespSons* Sons = GetWorld()->GetSubsystem<UVespSons>())
			{
				Sons->Tension(0.0f);
			}
		}
		return;
	}
	if (!bPvBasDit && Aylis->Stats.Pv * 3 < Aylis->Stats.PvMax)
	{
		bPvBasDit = true;
		ParlerEnRoute(PV_BAS, UE_ARRAY_COUNT(PV_BAS));
	}
	// Un coup encaisse interrompt ce qu'AYLIS faisait (sauf la garde, et les grands gestes deja lances)
	if (!bGardeLevee && Source && Geste != EVespGesteAylis::Lourde && Geste != EVespGesteAylis::Speciale && Geste != EVespGesteAylis::Esquive)
	{
		Geste = EVespGesteAylis::Touchee;
		TempsGeste = 0.0f;
		bCoupSuivant = false;
	}
}

void AVespPlayerController::QuandClairiereFermee(int32 Zone)
{
	ZoneActuelle = Zone;
	Monde->CacherBalise(Zone, true);
	bCarteOuverte = false;
	if (Noeuds.IsValidIndex(Zone) && Noeuds[Zone].Type == EVespSalle::Boss)
	{
		DialogueDuBoss();
		return;
	}
	Ecrire(Noeuds.IsValidIndex(Zone) && Noeuds[Zone].Type == EVespSalle::Elite ? TEXT("Une élite garde cette clairière. La barrière se ferme !")
	                                                                            : TEXT("Les Haschen sortent de terre. La barrière se ferme !"), 3.0f);
}

void AVespPlayerController::QuandClairiereLiberee(int32 Zone)
{
	if (!Noeuds.IsValidIndex(Zone))
	{
		return;
	}
	FVespNoeud& N = Noeuds[Zone];
	N.bVisite = true;
	Monde->MarquerZoneFaite(Zone);
	if (N.Type == EVespSalle::Boss)
	{
		// Des Souvenirs pour le gardien (bien plus la premiere fois qu'il tombe)
		const bool bPremiereFois = Memoire && !Memoire->GardiensVaincus[FMath::Clamp(Acte, 1, NombreDActes) - 1];
		FinirLActe();
		GagnerSouvenirs(15 + 5 * Acte + (bPremiereFois ? 25 : 0), bPremiereFois ? TEXT("gardien vaincu pour la premiere fois") : TEXT("gardien"));
		if (Acte >= NombreDActes)
		{
			FinDeLaRoute();		// le dernier gardien est tombe : l'Oracle parle, puis l'une des fins
			return;
		}
		// Un nouvel acte : AYLIS reprend toutes ses forces, et la prophetie la renforce
		Acte++;
		Eclats += 60 + Acte * 10;
		Aylis->Stats.PvMax += 6;
		Aylis->Stats.Attaque += 1;
		Aylis->Soigner(Aylis->Stats.PvMax);
		Aylis->Poison = Aylis->Brulure = Aylis->Gel = 0.0f;
		Potions++;
		AmbianceDeLActe();
		Phase = EVespPhase::NouvelActe;
		TempsPhase = 0.0f;
		return;
	}
	// Le pacte de Liss : une clairiere de moins avant le bonus
	PacteTenu.Reset();
	if (ClairieresMaudites > 0 && --ClairieresMaudites == 0)
	{
		TenirPacte();
	}
	// Toujours : quelques eclats, et AYLIS reprend son souffle. Puis la recompense annoncee par la balise.
	const bool bElite = N.Type == EVespSalle::Elite;
	const int32 Gain = bElite ? FMath::RandRange(16, 22) + Acte * 3 : FMath::RandRange(6, 10) + Acte * 2;
	GagnerEclats(Gain, Aylis->GetActorLocation());
	GagnerSouvenirs(bElite ? 5 : 2, TEXT("clairiere"));
	ParlerEnRoute(CLAIRIERE_LIBEREE, UE_ARRAY_COUNT(CLAIRIERE_LIBEREE));
	const int32 Soin = FMath::RoundToInt(Aylis->Stats.PvMax / 6 * MultSoins());
	Aylis->Soigner(Soin);
	Aylis->Jouer(EVespGeste::Victoire, 1.0f, true);
	const auto Eclats1 = [this](int32 G) { return EclatsAvecBonus(G); };
	const FString Base = FString::Printf(TEXT("La clairière est libérée ! +%d éclats, +%d pv.%s"), Eclats1(Gain), Soin, *PacteTenu);
	EVespRecompense R = N.Recompense;
	if (R == EVespRecompense::Souvenirs && !bSouvenirsPossibles)
	{
		R = EVespRecompense::Eclats;		// (une vision commencee plus loin ne rapporte pas de Souvenirs)
	}
	switch (R)
	{
		case EVespRecompense::Eclats:
		{
			const int32 Bourse = bElite ? 60 + Acte * 10 : 35 + Acte * 6;
			GagnerEclats(Bourse, N.Centre);
			MessageRoute = Base + FString::Printf(TEXT("  Récompense : +%d éclats."), Eclats1(Bourse));
			break;
		}
		case EVespRecompense::Objet:
			LacherButin(N.Centre, bElite ? 2 : 1);
			MessageRoute = Base + TEXT("  Récompense : un objet attend au centre de la clairière.");
			break;
		case EVespRecompense::Fiole:
		{
			Potions++;
			const int32 Grand = Aylis->Stats.PvMax * 30 / 100;
			Aylis->Soigner(Grand);
			UVespSons::Jouer(this, EVespSon::Potion, Aylis->GetActorLocation());
			MessageRoute = Base + FString::Printf(TEXT("  Récompense : +1 potion et +%d pv."), Grand);
			break;
		}
		case EVespRecompense::Etoile:
			PointsDeCompetence++;
			UVespSons::Jouer2D(this, EVespSon::Niveau, 0.8f);
			AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.7f, 0.92f, 1.0f));
			MessageRoute = Base + TEXT("  Récompense : une étoile s'éveille, +1 point du Seuil (I pour l'allumer).");
			break;
		case EVespRecompense::Souvenirs:
		{
			const int32 S = bElite ? 15 : 8;
			GagnerSouvenirs(S, TEXT("recompense"));
			MessageRoute = Base + FString::Printf(TEXT("  Récompense : +%d Souvenirs pour le Veilleur."), S);
			break;
		}
		case EVespRecompense::Felure:
			MessageRoute = Base;
			ProposerPactes();
			return;
		default:		// un maitre (et les clairieres sans recompense annoncee : un maitre au hasard)
			MessageRoute = Base;
			ProposerDons(N.Maitre >= 0 ? N.Maitre : FMath::RandRange(0, VespMaitres::Nombre - 1), bElite);
			return;
	}
	RetourExploration();
}

// L'ambiance de l'acte : la brume, la lune, le ciel, les sons, et le monde lui-meme
void AVespPlayerController::AmbianceDeLActe()
{
	const FVespInfoActe& A = InfoActe(Acte);
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		It->GetComponent()->SetFogInscatteringColor(A.Brume);
		It->GetComponent()->SetFogDensity(A.Densite);
	}
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		It->GetLightComponent()->SetLightColor(A.Lune);
		It->GetLightComponent()->SetIntensity(A.IntensiteLune);
	}
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		It->GetLightComponent()->SetIntensity(A.Ciel);
	}
	if (UVespSons* Sons = GetWorld()->GetSubsystem<UVespSons>())
	{
		Sons->AmbianceDeLActe(Acte);
	}
	if (Meteo)
	{
		Meteo->Configurer(Acte);
	}
	// Le butin oublie au sol reste dans l'acte d'avant
	for (AVespButin* B : ButinsAuSol)
	{
		if (B) B->Destroy();
	}
	ButinsAuSol.Reset();
	GenererMonde();
}

void AVespPlayerController::ContinuerApresLActe()
{
	if (Phase == EVespPhase::NouvelActe && TempsPhase > 1.0f)
	{
		UVespSons::Jouer2D(this, EVespSon::Clic);
		bReprise = false;
		MessageRoute = FString::Printf(TEXT("%s. Suis le sentier vers l'est : %s attend au bout."), InfoActe(Acte).Lieu, *NomDuBoss());
		RetourExploration();
		if (bOuvertureAFaire)
		{
			bOuvertureAFaire = false;
			OuvertureDeLaVision();
		}
		else if (Acte > 1)
		{
			ParlerEnRoute(&EN_ENTRANT[FMath::Clamp(Acte, 1, NombreDActes) - 1], 1, true);
		}
	}
}

// ===================== Les maitres des elements =====================

int32 AVespPlayerController::RareteDon(const TCHAR* Id) const
{
	const int32 k = DonsPris.Find(VespMaitres::Index(Id));
	return k != INDEX_NONE && RaretesDons.IsValidIndex(k) ? RaretesDons[k] : -1;
}

float AVespPlayerController::ValeurDon(const TCHAR* Id) const
{
	const int32 R = RareteDon(Id);
	return R >= 0 ? VespMaitres::Don(VespMaitres::Index(Id)).Valeurs[R] : 0.0f;
}

// Plus loin sur la route, et contre une elite, les dons sont plus souvent rares
int32 AVespPlayerController::TirerRarete(bool bElite) const
{
	const int32 Bonus = (Acte - 1) * 3 + (bElite ? 12 : 0);
	const int32 Heroique = 2 + Bonus / 4;
	const int32 Epique = Heroique + 8 + Bonus / 2;
	const int32 Rare = Epique + 25 + Bonus / 2;
	const int32 Tirage = FMath::RandRange(0, 99);
	return Tirage < Heroique ? 3 : (Tirage < Epique ? 2 : (Tirage < Rare ? 1 : 0));
}

void AVespPlayerController::DonAuHasard(int32& Don, int32& Rarete) const
{
	TArray<int32> Possibles;
	for (int32 d = 0; d < VespMaitres::NombreDons(); d++)
	{
		if (VespMaitres::Don(d).Maitre2 < 0 && RareteDon(VespMaitres::Don(d).Id) < VespMaitres::Raretes - 1)
		{
			Possibles.Add(d);
		}
	}
	Don = Possibles.Num() > 0 ? Possibles[FMath::RandRange(0, Possibles.Num() - 1)] : 0;
	Rarete = FMath::Max(TirerRarete(false), FMath::Min(RareteDon(VespMaitres::Don(Don).Id) + 1, VespMaitres::Raretes - 1));
}

// Un maitre apparait (par projection) et propose trois dons
void AVespPlayerController::ProposerDons(int32 Maitre, bool bElite)
{
	const int32 M = FMath::Clamp(Maitre, 0, VespMaitres::Nombre - 1);
	auto ADesDonsDe = [this](int32 Qui) {
		for (int32 d : DonsPris)
		{
			if (VespMaitres::Don(d).Maitre == Qui && VespMaitres::Don(d).Maitre2 < 0)
			{
				return true;
			}
		}
		return false;
	};
	TArray<int32> Simples, Doubles;
	for (int32 d = 0; d < VespMaitres::NombreDons(); d++)
	{
		const VespMaitres::FDon& Don = VespMaitres::Don(d);
		if (RareteDon(Don.Id) >= VespMaitres::Raretes - 1)
		{
			continue;		// deja au plus haut
		}
		if (Don.Maitre2 < 0 && Don.Maitre == M)
		{
			Simples.Add(d);
		}
		else if (Don.Maitre2 >= 0 && (Don.Maitre == M || Don.Maitre2 == M) && ADesDonsDe(Don.Maitre) && ADesDonsDe(Don.Maitre2))
		{
			Doubles.Add(d);
		}
	}
	OffresDons.Reset();
	OffresRaretes.Reset();
	if (Doubles.Num() > 0 && FMath::RandBool())
	{
		OffresDons.Add(Doubles[FMath::RandRange(0, Doubles.Num() - 1)]);
	}
	while (OffresDons.Num() < 3 && Simples.Num() > 0)
	{
		const int32 k = FMath::RandRange(0, Simples.Num() - 1);
		OffresDons.Add(Simples[k]);
		Simples.RemoveAt(k);
	}
	for (int32 d : OffresDons)
	{
		// un don deja pris revient un cran plus haut
		OffresRaretes.Add(FMath::Max(TirerRarete(bElite), FMath::Min(RareteDon(VespMaitres::Don(d).Id) + 1, VespMaitres::Raretes - 1)));
	}
	if (OffresDons.Num() == 0)
	{
		GagnerEclats(40 + Acte * 8, Aylis->GetActorLocation());		// (tout est deja au plus haut)
		RetourExploration();
		return;
	}
	MaitreOffrant = M;
	RepliqueOffre = VespMaitres::Replique(M);
	SelectionMenu = 0;
	bSelectionVisible = false;
	Phase = EVespPhase::ChoixRune;
	TempsPhase = 0.0f;
	// La projection : une colonne de lumiere a la couleur du maitre
	const FLinearColor C = VespMaitres::Couleur(M);
	AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Buff"), Aylis->GetActorLocation(), FRotator::ZeroRotator, 1.4f);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, C);
	Aylis->Surbrillance(0.6f, C);
	UVespSons::Jouer2D(this, EVespSon::Rune, 0.9f, 0.9f);
}

// Liss, la Felure, propose trois pactes : une malediction pendant quelques clairieres, puis un bonus
void AVespPlayerController::ProposerPactes()
{
	OffresDons.Reset();
	OffresRaretes.Reset();
	TArray<int32> Maledictions = {0, 1, 2};
	TArray<int32> Bonus = {0, 1, 2, 3, 4};
	for (int32 k = 0; k < 3; k++)
	{
		const int32 M = Maledictions[FMath::RandRange(0, Maledictions.Num() - 1)];
		const int32 B = Bonus[FMath::RandRange(0, Bonus.Num() - 1)];
		Maledictions.Remove(M);
		Bonus.Remove(B);
		OffresDons.Add(M * 10 + B);
		OffresRaretes.Add(0);
	}
	MaitreOffrant = VespMaitres::Liss;
	RepliqueOffre = VespMaitres::Replique(VespMaitres::Liss);
	SelectionMenu = 0;
	bSelectionVisible = false;
	Phase = EVespPhase::ChoixRune;
	TempsPhase = 0.0f;
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, VespMaitres::Couleur(VespMaitres::Liss));
	UVespSons::Jouer2D(this, EVespSon::Annonce, 0.7f, 0.8f);
}

void AVespPlayerController::PrendreDon(int32 Don, int32 Rarete)
{
	if (Don < 0 || Don >= VespMaitres::NombreDons())
	{
		return;
	}
	const VespMaitres::FDon& D = VespMaitres::Don(Don);
	const float VentAvant = ValeurDon(TEXT("v_passif"));
	const int32 Deja = DonsPris.Find(Don);
	if (Deja != INDEX_NONE)
	{
		RaretesDons[Deja] = FMath::Max(RaretesDons[Deja], Rarete);		// le meme don : il monte d'un cran
	}
	else
	{
		// Un seul don par geste : le nouveau remplace l'ancien
		if (D.Geste < VespMaitres::GestesUniques)
		{
			for (int32 k = DonsPris.Num() - 1; k >= 0; k--)
			{
				if (VespMaitres::Don(DonsPris[k]).Geste == D.Geste)
				{
					DonsPris.RemoveAt(k);
					RaretesDons.RemoveAt(k);
				}
			}
		}
		DonsPris.Add(Don);
		RaretesDons.Add(FMath::Clamp(Rarete, 0, VespMaitres::Raretes - 1));
	}
	BonusVitesse += (ValeurDon(TEXT("v_passif")) - VentAvant) / 100.0f;
	RecalculerDons();
	UVespSons::Jouer2D(this, EVespSon::Rune);
	Aylis->Surbrillance(0.5f, VespMaitres::Couleur(D.Maitre));
	AVespEffet::Jouer(GetWorld(), EVespEffet::Soin, Aylis->GetActorLocation(), FVector::UpVector, VespMaitres::Couleur(D.Maitre));
}

void AVespPlayerController::RecalculerDons()
{
	if (!Combat)
	{
		return;
	}
	Combat->BonusContreBrules = ValeurDon(TEXT("b_passif")) / 100.0f;
	Combat->BonusFenetreParade = ValeurDon(TEXT("i_passif")) / 1000.0f;
	Combat->PuissanceVapeur = ValeurDon(TEXT("d_vapeur")) / 100.0f;
	Combat->bCritiquesBrulent = RareteDon(TEXT("d_radieuse")) >= 0;
	Combat->BonusCritiques = ValeurDon(TEXT("d_radieuse")) / 100.0f;
	Combat->bSansCritique = Malediction == 2;
	Combat->MultDegatsAylis = Malediction == 0 ? 1.3f : 1.0f;
}

// Le pacte arrive a son terme : la malediction s'en va, le bonus arrive
void AVespPlayerController::TenirPacte()
{
	switch (BonusPromis)
	{
		case 0: Aylis->Stats.PvMax += 30; Aylis->Soigner(30); break;
		case 1: Aylis->Stats.Attaque += 5; break;
		case 2: PointsDeCompetence += 2; break;
		case 3: GagnerEclats(150, Aylis->GetActorLocation()); break;
		case 4:
		{
			int32 Don = 0, Rarete = 0;
			DonAuHasard(Don, Rarete);
			PrendreDon(Don, VespMaitres::Raretes - 1);
			break;
		}
		default: break;
	}
	PacteTenu = FString::Printf(TEXT("  Liss tient parole : %s."), VespMaitres::Bonus(BonusPromis));
	Malediction = -1;
	BonusPromis = -1;
	RecalculerDons();
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, VespMaitres::Couleur(VespMaitres::Liss));
	UVespSons::Jouer2D(this, EVespSon::Niveau, 0.8f);
}

void AVespPlayerController::ChoisirRune(int32 Numero)
{
	if (Phase != EVespPhase::ChoixRune || !OffresDons.IsValidIndex(Numero))
	{
		return;
	}
	if (MaitreOffrant == VespMaitres::Liss)
	{
		const int32 Code = OffresDons[Numero];
		Malediction = Code / 10;
		BonusPromis = Code % 10;
		ClairieresMaudites = VespMaitres::DureePacte;
		RecalculerDons();
		MessageRoute = FString::Printf(TEXT("Pacte avec Liss : %s pendant %d clairières, puis %s."), VespMaitres::Malediction(Malediction),
		                               VespMaitres::DureePacte, VespMaitres::Bonus(BonusPromis));
	}
	else
	{
		const int32 Don = OffresDons[Numero];
		PrendreDon(Don, OffresRaretes[Numero]);
		MessageRoute = FString::Printf(TEXT("%s (%s) : %s"), VespMaitres::Don(Don).Nom, VespMaitres::NomRarete(OffresRaretes[Numero]), *VespMaitres::Description(Don, OffresRaretes[Numero]));
	}
	RetourExploration();
}

// Ce que l'ecran de choix affiche
FString AVespPlayerController::EnteteOffre() const
{
	return FString::Printf(TEXT("%s  ·  %s"), *VespMajuscules(VespMaitres::Nom(MaitreOffrant)), *VespMajuscules(VespMaitres::Titre(MaitreOffrant)));
}

FString AVespPlayerController::TitreOffre(int32 i) const
{
	if (!OffresDons.IsValidIndex(i))
	{
		return FString();
	}
	return MaitreOffrant == VespMaitres::Liss ? FString::Printf(TEXT("Pacte : %s"), VespMaitres::Malediction(OffresDons[i] / 10)) : FString(VespMaitres::Don(OffresDons[i]).Nom);
}

FString AVespPlayerController::TexteOffre(int32 i) const
{
	if (!OffresDons.IsValidIndex(i))
	{
		return FString();
	}
	if (MaitreOffrant == VespMaitres::Liss)
	{
		return FString::Printf(TEXT("Pendant %d clairières : %s\nPuis : %s."), VespMaitres::DureePacte, VespMaitres::AideMalediction(OffresDons[i] / 10), VespMaitres::Bonus(OffresDons[i] % 10));
	}
	const int32 d = OffresDons[i];
	const VespMaitres::FDon& Don = VespMaitres::Don(d);
	FString T = VespMaitres::Description(d, OffresRaretes[i]);
	const int32 Deja = RareteDon(Don.Id);
	if (Deja >= 0)
	{
		T += FString::Printf(TEXT("\n(Amélioration : %s → %s.)"), VespMaitres::NomRarete(Deja), VespMaitres::NomRarete(OffresRaretes[i]));
	}
	else if (Don.Geste < VespMaitres::GestesUniques)
	{
		for (int32 p : DonsPris)
		{
			if (VespMaitres::Don(p).Geste == Don.Geste)
			{
				T += FString::Printf(TEXT("\n(Remplace « %s ».)"), VespMaitres::Don(p).Nom);
			}
		}
	}
	return T;
}

FString AVespPlayerController::PiedOffre(int32 i) const
{
	if (!OffresDons.IsValidIndex(i))
	{
		return FString();
	}
	if (MaitreOffrant == VespMaitres::Liss)
	{
		return TEXT("PACTE");
	}
	const VespMaitres::FDon& Don = VespMaitres::Don(OffresDons[i]);
	return FString::Printf(TEXT("%s  ·  %s%s"), VespMaitres::NomGeste(Don.Geste), VespMaitres::NomRarete(OffresRaretes[i]),
	                       Don.Maitre2 >= 0 ? TEXT("  ·  DOUBLE") : TEXT(""));
}

FLinearColor AVespPlayerController::CouleurOffre(int32 i) const
{
	if (MaitreOffrant == VespMaitres::Liss || !OffresRaretes.IsValidIndex(i))
	{
		return VespMaitres::Couleur(VespMaitres::Liss);
	}
	return OffresRaretes[i] == 0 ? VespMaitres::Couleur(MaitreOffrant) : VespButin::CouleurRarete((EVespRarete)OffresRaretes[i]);
}

// Un eclair (Velka) : il frappe tout autour d'un point
int32 AVespPlayerController::Foudre(const FVector& Centre, float Rayon, float Puissance)
{
	FVespCoup C = CoupDeBase();
	C.Origine = Centre;
	C.Portee = Rayon;
	C.DemiAngle = 180.0f;
	C.Puissance = Puissance;
	C.Poussee = 150.0f;
	C.Couleur = VespMaitres::Couleur(VespMaitres::Velka);
	const int32 Touches = Combat->FrappeDAylis(C);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Critique, Centre + FVector(0, 0, 70), FVector::UpVector, C.Couleur, 0.05f);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, Centre + FVector(0, 0, 40), FVector::UpVector, C.Couleur);
	UVespSons::Jouer(this, EVespSon::Tonnerre, Centre, 0.45f, 1.4f);
	if (const float Pluie = ValeurDon(TEXT("d_pluie")))
	{
		Aylis->Soigner(FMath::RoundToInt(Touches * Pluie * MultSoins()));
	}
	return Touches;
}

// La foudre tombe sur les Haschen les plus proches d'AYLIS
void AVespPlayerController::FoudreSurProches(int32 Nombre, float Portee, float Puissance)
{
	const FVector Ici = Aylis->GetActorLocation();
	TArray<AVespUnite*> Cibles;
	for (const FVespHaschen& H : Combat->GetHaschen())
	{
		AVespUnite* U = H.U.Get();
		if (U && U->EstDebout() && !U->IsHidden() && H.Etat != EVespIntention::Surgir && FVector::Dist2D(U->GetActorLocation(), Ici) < Portee)
		{
			Cibles.Add(U);
		}
	}
	Cibles.Sort([&Ici](const AVespUnite& A, const AVespUnite& B) { return FVector::DistSquared2D(A.GetActorLocation(), Ici) < FVector::DistSquared2D(B.GetActorLocation(), Ici); });
	for (int32 k = 0; k < Cibles.Num() && k < Nombre; k++)
	{
		Foudre(Cibles[k]->GetActorLocation(), 110.0f, Puissance);
	}
}

// Une gerbe (feu, givre) : un cercle qui frappe et porte un effet
int32 AVespPlayerController::Gerbe(const FVector& Centre, float Rayon, float Puissance, int32 Effet, const FLinearColor& Couleur)
{
	FVespCoup C = CoupDeBase();
	C.Origine = Centre;
	C.Portee = Rayon;
	C.DemiAngle = 180.0f;
	C.Puissance = Puissance;
	C.bPeutCritiquer = false;
	C.Poussee = 300.0f;
	C.Effet = Effet;
	C.ChanceEffet = Effet != 0 ? 1.0f : 0.0f;
	C.Couleur = Couleur;
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Centre + FVector(0, 0, 10), FVector::UpVector, Couleur);
	return Combat->FrappeDAylis(C);
}

// Le boss parle avant le combat (toujours sans genre pour AYLIS)
void AVespPlayerController::DialogueDuBoss()
{
	switch (Acte)
	{
		case 1:
			Orateurs = {TEXT("Skarn"), TEXT("AYLIS"), TEXT("Skarn")};
			Repliques = {
				TEXT("Encore une petite vision qui marche vers Karn ? Approche. Le sol se souviendra de toi, même quand ton nom sera perdu."),
				TEXT("Le sol, peut-être. Toi, tu vas oublier."),
				TEXT("Quand je lève ma masse, la terre se brise. Regarde bien où tu poses les pieds, petite vision."),
			};
			break;
		case 2:
			Orateurs = {TEXT("La Matriarche"), TEXT("AYLIS"), TEXT("La Matriarche")};
			Repliques = {
				TEXT("Mes loups ont senti ta peur bien avant ton odeur. Ils ont faim, et moi, j'ai le temps."),
				TEXT("Tes loups auront faim longtemps. Ce n'est pas pour eux que je marche."),
				TEXT("Tous viennent pour moi, à la fin. Approche, que je te couvre de mon maléfice."),
			};
			break;
		case 3:
			Orateurs = {TEXT("Le Roi Noyé"), TEXT("AYLIS"), TEXT("Le Roi Noyé")};
			Repliques = {
				TEXT("Tout finit dans l'eau, petite vision. Les rois, les armées, les prophètes. Moi, j'ai simplement commencé plus tôt."),
				TEXT("Alors tu as eu le temps de t'y habituer. Moi, je ne fais que passer."),
				TEXT("Personne ne passe. Les marais gardent tout ce qu'ils touchent. Regarde : la marée monte déjà."),
			};
			break;
		case 4:
			Orateurs = {TEXT("La prophétie"), TEXT("Le Gardien de Pierre"), TEXT("AYLIS")};
			Repliques = {
				TEXT("Le grand portail s'ouvre sur un géant de pierre. Des runes s'allument une à une sur son torse."),
				TEXT("INTRUS. LA MARCHE D'ASHKA EST FERMÉE. RETOURNE À LA POUSSIÈRE."),
				TEXT("Tu as été taillé pour garder une porte. Moi, pour la traverser. Voyons qui a été le mieux fait."),
			};
			break;
		case 5:
			Orateurs = {TEXT("Ashka"), TEXT("AYLIS"), TEXT("Ashka")};
			Repliques = {
				TEXT("Alors voilà la vision qui fait trembler mes guerriers. Mes flèches ont déjà vu pire."),
				TEXT("Tes guerriers ont raison de trembler. Pas à cause de moi : à cause de ce qui vient après toi."),
				TEXT("La prophétie dit que tu tomberas sur ce col. Je suis là pour qu'elle ne mente pas."),
			};
			break;
		case 6:
			Orateurs = {TEXT("Vorgath"), TEXT("AYLIS"), TEXT("Vorgath")};
			Repliques = {
				TEXT("Tout ce qui brûle finit en cendre. Les forêts, les villages, les prophètes. Et toi aussi."),
				TEXT("Personne ne brûlera ce soir. Sauf ta forge."),
				TEXT("Approche. Je vais te faire une place dans ma collection de cendres."),
			};
			break;
		default:
			Orateurs = {TEXT("L'Oracle"), TEXT("AYLIS"), TEXT("L'Oracle")};
			Repliques = {
				TEXT("Mille fois, j'ai vu ta route finir ici, AYLIS. Dans chaque vision, tu tombes au cœur du Voile."),
				TEXT("Alors regarde bien celle-ci. Elle est différente."),
				TEXT("Il n'y a pas de visions différentes. Il n'y a que moi... et la fin de la route."),
			};
			break;
	}
	// Le gardien se souvient des visions passees : il a tue la derniere, ou il a deja ete vaincu
	const int32 i = FMath::Clamp(Acte, 1, NombreDActes) - 1;
	if (Memoire && !bModePhoto)
	{
		const bool bIlATueLaDerniere = Memoire->bDerniereChuteBoss && Memoire->DerniereChuteActe == Acte;
		const bool bDejaVaincu = Memoire->GardiensVaincus[i];
		const TCHAR* Souvenir = bIlATueLaDerniere ? APRES_UNE_MORT[i] : (bDejaVaincu ? DEJA_VAINCU[i] : (Memoire->MortsParGardien[i] > 0 ? APRES_UNE_MORT[i] : nullptr));
		if (Souvenir)
		{
			// (au 4e acte, la prophetie decrit d'abord le Gardien de Pierre)
			const int32 Ou = Orateurs.Num() > 1 && Orateurs[0] == TEXT("La prophétie") ? 1 : 0;
			const FString Gardien = Orateurs[Ou];
			const bool bVainqueur = Souvenir == APRES_UNE_MORT[i];
			Orateurs.Insert(Gardien, Ou);
			Repliques.Insert(Souvenir, Ou);
			Orateurs.Insert(TEXT("AYLIS"), Ou + 1);
			Repliques.Insert(bVainqueur ? TEXT("Toi.") : (i == 6 ? TEXT("Oui.") : TEXT("Alors tu sais comment ça finit.")), Ou + 1);
		}
	}
	// A partir de la dixieme vision, les gardiens connaissent son nom
	if (NumeroVision >= 10)
	{
		for (int32 k = 0; k < Repliques.Num(); k++)
		{
			if (Orateurs[k] != TEXT("AYLIS"))
			{
				Repliques[k] = Repliques[k].Replace(TEXT(", petite vision"), TEXT(", AYLIS"));
			}
		}
	}
	ApresDialogue = nullptr;
	LigneDialogue = 0;
	Ecriture = 0.0f;
	bDialogueBoss = true;
	Phase = EVespPhase::Dialogue;
	TempsPhase = 0.0f;
	Geste = EVespGesteAylis::Libre;
	Aylis->TenirGarde(false);
}

void AVespPlayerController::AvancerDialogue()
{
	if (Phase != EVespPhase::Dialogue)
	{
		return;
	}
	if (Ecriture < Repliques[LigneDialogue].Len())
	{
		Ecriture = 9999.0f;			// un premier appui ecrit toute la replique
	}
	else if (++LigneDialogue < Repliques.Num())
	{
		Ecriture = 0.0f;
	}
	else
	{
		if (bDialogueBoss)
		{
			MessageRoute = NomDuBoss() + TEXT(" attaque !");
		}
		bDialogueBoss = false;
		TFunction<void()> Suite = MoveTemp(ApresDialogue);
		ApresDialogue = nullptr;
		RetourExploration();
		if (Suite)
		{
			Suite();
		}
	}
}

// Un dialogue hors combat (le parchemin) ; Suite : ce qui se passe une fois la derniere replique lue
void AVespPlayerController::Dire(const TArray<FString>& Qui, const TArray<FString>& Quoi, TFunction<void()> Suite)
{
	if (Quoi.Num() == 0 || bModePhoto)
	{
		if (Suite)
		{
			Suite();
		}
		return;
	}
	Orateurs = Qui;
	Repliques = Quoi;
	ApresDialogue = MoveTemp(Suite);
	LigneDialogue = 0;
	Ecriture = 0.0f;
	bDialogueBoss = false;
	Phase = EVespPhase::Dialogue;
	TempsPhase = 0.0f;
	Geste = EVespGesteAylis::Libre;
	Aylis->TenirGarde(false);
}

// La premiere vision : la prophetie raconte. Les suivantes : une ligne d'AYLIS, qui se souvient de la derniere chute
void AVespPlayerController::OuvertureDeLaVision()
{
	if (NumeroVision <= 1)
	{
		Dire({TEXT("La prophétie"), TEXT("La prophétie"), TEXT("La prophétie"), TEXT("La prophétie"), TEXT("AYLIS")},
		     {TEXT("Le Voile avance. Sept terres, et plus une seule qui dorme."),
		      TEXT("Alors la prophétie a fait ce qu'elle fait toujours : elle a imaginé quelqu'un qui arrive au bout."),
		      TEXT("Ce quelqu'un n'existe pas encore. Il faudra marcher pour exister."),
		      TEXT("AYLIS ouvre les yeux au milieu des brumes."),
		      TEXT("Une route. Bon. On y va.")});
		return;
	}
	const int32 Chute = Memoire ? Memoire->DerniereChuteActe : 0;
	FString Ligne;
	if (Chute >= 1 && Memoire->bDerniereChuteBoss)
	{
		Ligne = FString::Printf(TEXT("Vision %d. Cette fois, %s ne verra pas le coup venir."), NumeroVision, GARDIEN[Chute - 1]);
	}
	else if (Chute > 1 && FMath::RandBool())
	{
		Ligne = FString::Printf(TEXT("Vision %d. La dernière s'est arrêtée %s. Pas celle-ci."), NumeroVision, LIEU_DE_CHUTE[Chute - 1]);
	}
	else
	{
		Ligne = FString(OUVERTURES[FMath::RandRange(0, UE_ARRAY_COUNT(OUVERTURES) - 1)]).Replace(TEXT("%d"), *FString::FromInt(NumeroVision));
	}
	Dire({TEXT("AYLIS")}, {Ligne});
}

// Oswin, au feu de camp : la premiere condition remplie (et pas encore entendue), sinon une replique d'ambiance.
// Puis le repos : +60 % des pv et une potion.
void AVespPlayerController::ParlerAuVeilleur()
{
	TArray<FString> Qui, Quoi;
	auto Oswin = [&](const FString& L) { Qui.Add(VEILLEUR); Quoi.Add(L); };
	auto Reponse = [&](const FString& L) { Qui.Add(TEXT("AYLIS")); Quoi.Add(L); };
	auto Nouveau = [this](const TCHAR* Id) {
		if (!Memoire || Memoire->Entendues.Contains(Id))
		{
			return false;
		}
		Memoire->Entendues.Add(Id);
		return true;
	};
	const bool bMortIciLaDerniereFois = Memoire && Memoire->bDerniereChuteBoss && Memoire->DerniereChuteActe == Acte;
	if (Nouveau(TEXT("veilleur_premiere")))
	{
		Oswin(TEXT("Assieds-toi. Les visions ont toujours froid, au début."));
		Reponse(TEXT("Qui es-tu ?"));
		Oswin(TEXT("Oswin. Je garde les feux le long de la route, et je me souviens de chaque vision qui s'y est assise. Ce que tu rapportes, je le garde pour la suivante."));
	}
	else if (bMortIciLaDerniereFois && !DejaDitDansLaVision.Contains(TEXT("veilleur_chute")))
	{
		DejaDitDansLaVision.Add(TEXT("veilleur_chute"));
		Oswin(TEXT("La dernière est passée ici aussi. Elle avait ta façon de tenir la lame."));
		Reponse(TEXT("Jusqu'où est-elle allée ?"));
		Oswin(FString::Printf(TEXT("Jusqu'à %s. Pas plus loin."), GARDIEN[FMath::Clamp(Acte, 1, NombreDActes) - 1]));
	}
	else if (NumeroVision >= 5 && Nouveau(TEXT("veilleur_demarche")))
	{
		Oswin(TEXT("Je commence à reconnaître ta démarche. Ce n'est pas bon signe, pour toi."));
		Reponse(TEXT("Alors regarde bien. Celle-ci va plus loin."));
	}
	else if (Memoire && Memoire->GardiensVaincus[0] && Nouveau(TEXT("veilleur_skarn")))
	{
		Oswin(TEXT("Skarn ne rit plus. Tu sais ce qu'on dit, quand Skarn ne rit plus ? Non. Personne ne le sait encore."));
	}
	else if (Acte >= 6 && Nouveau(TEXT("veilleur_oracle")))
	{
		Oswin(TEXT("L'Oracle... je l'ai connu, avant. Quand il avait encore des traits de lumière, comme toi."));
		Reponse(TEXT("Comme moi ?"));
		Oswin(TEXT("Il regardait la route de la même façon. Va. Il t'attend depuis longtemps."));
	}
	else
	{
		TArray<int32> Libres;
		for (int32 k = 0; k < UE_ARRAY_COUNT(VEILLEUR_AMBIANCE); k++)
		{
			if (!DejaDitDansLaVision.Contains(VEILLEUR_AMBIANCE[k]))
			{
				Libres.Add(k);
			}
		}
		const TCHAR* Ligne = VEILLEUR_AMBIANCE[Libres.Num() > 0 ? Libres[FMath::RandRange(0, Libres.Num() - 1)] : FMath::RandRange(0, UE_ARRAY_COUNT(VEILLEUR_AMBIANCE) - 1)];
		DejaDitDansLaVision.Add(Ligne);
		Oswin(Ligne);
		Reponse(REPONSES_AU_VEILLEUR[FMath::RandRange(0, UE_ARRAY_COUNT(REPONSES_AU_VEILLEUR) - 1)]);
	}
	EcrireMemoire();
	Dire(Qui, Quoi, [this]() {
		const int32 Soin = FMath::RoundToInt(Aylis->Stats.PvMax * 0.6f * MultSoins());
		Aylis->Soigner(Soin);
		Potions++;
		UVespSons::Jouer(this, EVespSon::Soin, Aylis->GetActorLocation());
		Ecrire(FString::Printf(TEXT("AYLIS se repose au coin du feu : +%d pv et une potion."), Soin), 5.0f);
	});
}

FString AVespPlayerController::LigneDeChute() const
{
	const int32 i = FMath::Clamp(Acte, 1, NombreDActes) - 1;
	const bool bSousLeGardien = Memoire && Memoire->bDerniereChuteBoss && Memoire->DerniereChuteActe == Acte;
	return bSousLeGardien ? FString::Printf(TEXT("Vision %d : tombée %s, %s."), NumeroVision, LIEU_DE_CHUTE[i], COUPS_DU_GARDIEN[i])
	                      : FString::Printf(TEXT("Vision %d : tombée %s."), NumeroVision, LIEU_DE_CHUTE[i]);
}

// ===================== Les paroles en route, et les fins =====================

void AVespPlayerController::ParlerEnRoute(const TCHAR* const* Lignes, int32 Nombre, bool bForcer)
{
	if (bModePhoto || Nombre <= 0 || (!bForcer && ProchaineParole > 0.0f))
	{
		return;
	}
	TArray<int32> Libres;
	for (int32 k = 0; k < Nombre; k++)
	{
		if (!DejaDitDansLaVision.Contains(Lignes[k]))
		{
			Libres.Add(k);
		}
	}
	const TCHAR* Ligne = Lignes[Libres.Num() > 0 ? Libres[FMath::RandRange(0, Libres.Num() - 1)] : FMath::RandRange(0, Nombre - 1)];
	DejaDitDansLaVision.Add(Ligne);
	ParoleAylis = Ligne;
	TempsParole = 3.5f;
	ProchaineParole = 60.0f;
}

// L'Oracle vient de tomber. La premiere fois, il revele le secret : il a ete la premiere vision.
// Ensuite (l'Oracle a le visage de la vision d'avant), AYLIS peut choisir de refermer le Voile.
void AVespPlayerController::FinDeLaRoute()
{
	const bool bPremiereVictoire = !Memoire || Memoire->Victoires == 0;
	if (bPremiereVictoire)
	{
		Dire({TEXT("L'Oracle"), TEXT("AYLIS"), TEXT("L'Oracle"), TEXT("L'Oracle"), TEXT("AYLIS"), TEXT("L'Oracle"), TEXT("La prophétie")},
		     {TEXT("Tu as atteint le bout. Comme moi, il y a mille visions."),
		      TEXT("Comme toi ?"),
		      TEXT("Je suis la toute première vision. La prophétie m'a envoyé bien avant toi, et j'ai marché jusqu'ici. En arrivant, j'ai compris : le Voile ne se referme que si une vision accepte de s'y dissoudre."),
		      TEXT("J'ai refusé. Alors je suis resté devant la porte, et j'ai regardé toutes les autres tomber."),
		      TEXT("Et maintenant ?"),
		      TEXT("Maintenant, c'est ta porte."),
		      TEXT("AYLIS prend sa place devant la porte de Karn. Le Voile, lui, attend encore.")},
		     [this]() { TerminerLaVision(1); });
		return;
	}
	Dire({TEXT("L'Oracle"), TEXT("AYLIS"), TEXT("L'Oracle"), TEXT("La prophétie")},
	     {TEXT("Tu as mon visage. Ou c'est moi qui ai le tien."),
	      TEXT("Tu étais moi. La vision d'avant."),
	      TEXT("Et j'ai refusé, comme la première. Derrière moi, le Voile attend toujours une vision qui accepte de s'y fondre."),
	      TEXT("La porte de Karn s'ouvre sur le cœur du Voile.")},
	     [this]() {
		     EvenementActuel = COEUR_DU_VOILE;
		     SelectionMenu = 0;
		     bSelectionVisible = false;
		     Phase = EVespPhase::Evenement;
		     TempsPhase = 0.0f;
	     });
}

void AVespPlayerController::TerminerLaVision(int32 Fin)
{
	EffacerPartie();
	FinObtenue = Fin;
	Phase = EVespPhase::Victoire;
	TempsPhase = 0.0f;
	GagnerSouvenirs(Fin == 2 ? 100 : 60, Fin == 2 ? TEXT("la vraie fin") : TEXT("victoire"));
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE fin %d : vision %d arrivee au bout en %s"), Fin, NumeroVision, *UVespSauvegarde::Duree(ChronoPartie));
	if (Memoire)
	{
		Memoire->Victoires++;
		Memoire->VraiesFins += Fin == 2 ? 1 : 0;
		if (Memoire->MeilleurePartie <= 0.0f || ChronoPartie < Memoire->MeilleurePartie)
		{
			Memoire->MeilleurePartie = ChronoPartie;
		}
		EcrireMemoire();
	}
}

FString AVespPlayerController::TexteDeLaFin() const
{
	if (FinObtenue == 2)
	{
		const int32 Avant = FMath::Max(1, NumeroVision - 1);
		return FString::Printf(TEXT("Vision %d, arrivée au bout en %s. AYLIS s'efface trait par trait, et le Voile se referme avec. Les sept terres se réveillent. Sur la route, %s"),
		                       NumeroVision, *UVespSauvegarde::Duree(ChronoPartie),
		                       *(Avant == 1 ? FString(TEXT("une vision attend en silence : celle qui a marché avant.")) : FString::Printf(TEXT("%d visions attendent en silence : toutes celles qui ont marché avant."), Avant)));
	}
	return FString::Printf(TEXT("Vision %d, arrivée au bout en %s. AYLIS garde désormais la porte de Karn, comme l'Oracle avant. La prochaine vision trouvera un Oracle au visage familier."),
	                       NumeroVision, *UVespSauvegarde::Duree(ChronoPartie));
}

// ===================== Mettre la vision de cote, et la reprendre =====================

void AVespPlayerController::SauverPartie()
{
	if (!bVisionEnCours || bModePhoto || bTestOuverture || TestFin > 0 || bTestOptions || bTestRecompenses || EtapeTestMaitres >= 0 || !IsValid(Aylis) || !Monde || !Combat)
	{
		return;
	}
	if (Phase != EVespPhase::Exploration || Combat->EnCombat())
	{
		return;		// (en plein combat, on garde la derniere : celle d'avant le combat)
	}
	UVespPartie* P = Cast<UVespPartie>(UGameplayStatics::CreateSaveGameObject(UVespPartie::StaticClass()));
	P->NumeroVision = NumeroVision;
	P->Acte = Acte;
	P->GraineVision = Monde->GraineVision;
	P->GraineCarte = GraineCarte;
	for (const FVespNoeud& N : Noeuds)
	{
		P->Visites.Add(N.bVisite);
	}
	P->Position = Aylis->GetActorLocation();
	P->Stats = Aylis->Stats;
	P->Potions = Potions;
	P->Rage = Rage;
	P->Eclats = Eclats;
	P->Niveau = Niveau;
	P->Xp = Xp;
	P->PointsDeCompetence = PointsDeCompetence;
	P->HaschenVaincus = HaschenVaincus;
	P->BonusVitesse = BonusVitesse;
	P->DonsPris = DonsPris;
	P->RaretesDons = RaretesDons;
	P->Malediction = Malediction;
	P->ClairieresMaudites = ClairieresMaudites;
	P->BonusPromis = BonusPromis;
	const bool Effets[8] = {bFlamme, bSeve, bFureur, bSangsue, bFortune, bGivre, bPiedSur, Combat->bEpines};
	for (int32 b = 0; b < 8; b++)
	{
		P->EffetsDesRunes |= Effets[b] ? (1 << b) : 0;
	}
	P->BonusParade = Combat->BonusParade;
	P->Seuil = Seuil;
	for (const FVespObjet& O : Sac)
	{
		P->Sac.Add(FVespObjetSauve::De(O));
	}
	for (int32 e = 0; e < (int32)EVespEmplacement::Nombre; e++)
	{
		P->Equipement.Add(FVespObjetSauve::De(Equipement[e]));
		P->Equipe.Add(bEquipe[e]);
	}
	P->ChronoPartie = ChronoPartie;
	P->ChronoActe = ChronoActe;
	P->TempsDesActes = TempsDesActes;
	P->SouvenirsDeLaVision = SouvenirsDeLaVision;
	P->bSouvenirsPossibles = bSouvenirsPossibles;
	P->bSecondSouffle = bSecondSouffle;
	P->DejaDit = DejaDitDansLaVision.Array();
	UGameplayStatics::SaveGameToSlot(P, EmplacementPartie(), 0);
}

void AVespPlayerController::EffacerPartie()
{
	bVisionEnCours = false;
	PartieSuspendue = nullptr;
	if (!bModePhoto && UGameplayStatics::DoesSaveGameExist(EmplacementPartie(), 0))
	{
		UGameplayStatics::DeleteGameInSlot(EmplacementPartie(), 0);
	}
}

void AVespPlayerController::ReprendreLaVision()
{
	if (Phase != EVespPhase::Titre || !PartieSuspendue)
	{
		return;
	}
	const UVespPartie* P = PartieSuspendue;
	UVespSons::Jouer2D(this, EVespSon::Clic);
	bVeilleurOuvert = bOptionsOuvertes = false;
	NumeroVision = P->NumeroVision;
	Acte = FMath::Clamp(P->Acte, 1, NombreDActes);
	// L'acte, refait a l'identique : le meme decor, la meme carte
	Monde->GraineVision = P->GraineVision;
	GraineCarteImposee = P->GraineCarte;
	AmbianceDeLActe();
	// AYLIS, telle qu'elle etait (les stats comprennent deja les objets, les runes et les etoiles)
	Aylis->Stats = P->Stats;
	Potions = P->Potions;
	Rage = P->Rage;
	Eclats = P->Eclats;
	Niveau = P->Niveau;
	Xp = P->Xp;
	PointsDeCompetence = P->PointsDeCompetence;
	HaschenVaincus = P->HaschenVaincus;
	BonusVitesse = P->BonusVitesse;
	DonsPris = P->DonsPris;
	RaretesDons = P->RaretesDons;
	RaretesDons.SetNum(DonsPris.Num());
	Malediction = P->Malediction;
	ClairieresMaudites = P->ClairieresMaudites;
	BonusPromis = P->BonusPromis;
	bool* Effets[7] = {&bFlamme, &bSeve, &bFureur, &bSangsue, &bFortune, &bGivre, &bPiedSur};
	for (int32 b = 0; b < 7; b++)
	{
		*Effets[b] = (P->EffetsDesRunes & (1 << b)) != 0;
	}
	Combat->bPiedSur = bPiedSur;
	Combat->bEpines = (P->EffetsDesRunes & (1 << 7)) != 0;
	Combat->BonusParade = P->BonusParade;
	RecalculerDons();
	Seuil = P->Seuil;
	Seuil.SetNum(VespSeuil::Nombre);
	Sac.Reset();
	for (const FVespObjetSauve& O : P->Sac)
	{
		Sac.Add(O.Objet());
	}
	for (int32 e = 0; e < (int32)EVespEmplacement::Nombre; e++)
	{
		bEquipe[e] = P->Equipe.IsValidIndex(e) && P->Equipe[e] && P->Equipement.IsValidIndex(e);
		if (bEquipe[e])
		{
			Equipement[e] = P->Equipement[e].Objet();
		}
	}
	HabillerAylis();
	ChronoPartie = P->ChronoPartie;
	ChronoActe = P->ChronoActe;
	TempsDesActes = P->TempsDesActes;
	SouvenirsDeLaVision = P->SouvenirsDeLaVision;
	bSouvenirsPossibles = P->bSouvenirsPossibles;
	bSecondSouffle = P->bSecondSouffle;
	DejaDitDansLaVision.Reset();
	DejaDitDansLaVision.Append(P->DejaDit);
	// Les clairieres deja faites le restent ; AYLIS reprend ou elle s'etait arretee
	int32 Faites = 0;
	for (int32 i = 0; i < Noeuds.Num() && i < P->Visites.Num(); i++)
	{
		if (P->Visites[i])
		{
			Noeuds[i].bVisite = true;
			if (Noeuds[i].Type != EVespSalle::Marchand)
			{
				Monde->MarquerZoneFaite(i);
			}
			Combat->LibererSansCombat(i);
			Faites++;
		}
	}
	Aylis->Teleporter(P->Position);
	bCaleCamera = true;
	bVisionEnCours = true;
	bOuvertureAFaire = false;
	bReprise = true;
	PartieSuspendue = nullptr;
	Phase = EVespPhase::NouvelActe;			// le titre de l'acte, puis la route
	TempsPhase = 0.0f;
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE reprise : vision %d, acte %s, %d clairieres deja faites, niveau %d, %d eclats, %d dons, %s de route"),
	       NumeroVision, *Romain(Acte), Faites, Niveau, Eclats, DonsPris.Num(), *UVespSauvegarde::Duree(ChronoPartie));
}

void AVespPlayerController::Recommencer()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));	// une nouvelle vision
}

void AVespPlayerController::Ralenti(float Echelle, float Duree)
{
	UGameplayStatics::SetGlobalTimeDilation(this, Echelle);
	FinDuRalenti = FPlatformTime::Seconds() + Duree;
}

// ===================== AYLIS : les gestes =====================

// Ou vise AYLIS : la souris (si elle a bouge il y a peu), le stick droit, sinon la ou elle marche (ou regarde)
FVector AVespPlayerController::DirectionVisee() const
{
	const FVector2D Stick(GetInputAnalogKeyState(EKeys::Gamepad_RightX), GetInputAnalogKeyState(EKeys::Gamepad_RightY));
	const float Yaw = FMath::DegreesToRadians(RotationCamera.Yaw);
	const FVector Avant(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f);
	const FVector Droite(-FMath::Sin(Yaw), FMath::Cos(Yaw), 0.0f);
	if (Stick.Size() > 0.35f)
	{
		return (Avant * Stick.Y + Droite * Stick.X).GetSafeNormal();
	}
	if (TempsSouris > 0.0f)
	{
		FVector Depart, Direction;
		if (DeprojectMousePositionToWorld(Depart, Direction) && Direction.Z < -0.01f)
		{
			const float Sol = Aylis->GetActorLocation().Z + 60.0f;
			const FVector Point = Depart + Direction * ((Sol - Depart.Z) / Direction.Z);
			FVector Vers = Point - Aylis->GetActorLocation();
			Vers.Z = 0.0f;
			if (Vers.SizeSquared() > 400.0f)
			{
				return Vers.GetSafeNormal();
			}
		}
	}
	if (!Deplacement.IsNearlyZero())
	{
		return Deplacement.GetSafeNormal2D();
	}
	return Aylis->Avant();
}

void AVespPlayerController::Attaquer(bool bLourde)
{
	// Pendant un coup : on retient la demande, le suivant s'enchainera
	if (Geste == EVespGesteAylis::Attaque && !bLourde && TempsGeste < DureeGeste * 0.6f)
	{
		bCoupSuivant = true;
		return;
	}
	const bool bApresImpact = (Geste == EVespGesteAylis::Attaque || Geste == EVespGesteAylis::Lourde) && bImpactFait;
	if (Geste != EVespGesteAylis::Libre && !bApresImpact)
	{
		return;
	}
	const bool bEnchaine = Geste == EVespGesteAylis::Attaque || FinCombo > 0.0f;
	bGardeLevee = false;
	Aylis->TenirGarde(false);
	// La cible : le Haschen le plus proche devant AYLIS (elle se tourne vers lui et se fend)
	DirectionGeste = Regard;
	ElanGeste = 50.0f;
	if (AVespUnite* Cible = Combat->CibleProche(Aylis->GetActorLocation(), Regard, 480.0f, 75.0f))
	{
		FVector V = Cible->GetActorLocation() - Aylis->GetActorLocation();
		V.Z = 0.0f;
		DirectionGeste = V.GetSafeNormal();
		ElanGeste = FMath::Clamp(V.Size() - Aylis->Rayon() - Cible->Rayon() - 70.0f, 0.0f, 180.0f);
	}
	Aylis->TournerDUnCoup(DirectionGeste);
	// L'arme en main change le rythme : les dagues sont vives, les armes a deux mains lentes
	const EVespArme Type = ArmeEnMain();
	const float Rythme = Type == EVespArme::Dagues ? 1.3f : (Type == EVespArme::DeuxMains ? 0.75f : (Type == EVespArme::Baton ? 0.95f : 1.0f));
	float Vitesse = 1.0f;
	EVespGeste G;
	if (bLourde)
	{
		Geste = EVespGesteAylis::Lourde;
		G = EVespGeste::Lourde;
		Vitesse = 1.05f * (Type == EVespArme::DeuxMains ? 0.85f : 1.0f);
		Combo = 0;
	}
	else
	{
		Combo = bEnchaine ? (Combo + 1) % 3 : 0;
		Geste = EVespGesteAylis::Attaque;
		G = Combo == 0 ? EVespGeste::Attaque1 : (Combo == 1 ? EVespGeste::Attaque2 : EVespGeste::Attaque3);
		Vitesse = (Combo == 2 ? 1.2f : 1.4f) * Rythme;
	}
	DureeGeste = Aylis->DureeGeste(G) / Vitesse;
	MomentImpact = DureeGeste * (bLourde ? 0.45f : 0.36f);
	Aylis->Jouer(G, Vitesse);
	TempsGeste = 0.0f;
	bImpactFait = false;
	bCoupSuivant = false;
	UVespSons::Jouer(this, bLourde ? EVespSon::FrappeLourde : EVespSon::Frappe, Aylis->GetActorLocation(), 0.8f, bLourde ? 0.9f : 1.0f + Combo * 0.06f);
}

void AVespPlayerController::Esquiver(const FVector& Direction)
{
	if (RechargeEsquive > 0.0f || Geste == EVespGesteAylis::Potion || Geste == EVespGesteAylis::Speciale || Geste == EVespGesteAylis::Esquive
	    || (Geste == EVespGesteAylis::Lourde && !bImpactFait))
	{
		return;
	}
	DirectionGeste = Direction.IsNearlyZero() ? Aylis->Avant() : Direction.GetSafeNormal2D();
	// L'animation : vers l'avant, l'arriere, la gauche ou la droite d'AYLIS (elle garde son regard)
	const float Face = FVector::DotProduct(DirectionGeste, Aylis->Avant());
	const float Cote = FVector::CrossProduct(Aylis->Avant(), DirectionGeste).Z;
	EVespGeste G = EVespGeste::EsquiveAvant;
	if (FMath::Abs(Face) >= FMath::Abs(Cote))
	{
		G = Face >= 0.0f ? EVespGeste::EsquiveAvant : EVespGeste::EsquiveArriere;
	}
	else
	{
		G = Cote > 0.0f ? EVespGeste::EsquiveDroite : EVespGeste::EsquiveGauche;
	}
	Geste = EVespGesteAylis::Esquive;
	TempsGeste = 0.0f;
	DureeGeste = 0.42f;
	Aylis->Jouer(G, FMath::Max(0.8f, Aylis->DureeGeste(G) / 0.5f));
	Aylis->Invulnerable = 0.34f;
	RechargeEsquive = 0.42f + 0.35f;
	bGardeLevee = false;
	Aylis->TenirGarde(false);
	bCoupSuivant = false;
	UVespSons::Jouer(this, EVespSon::Esquive, Aylis->GetActorLocation(), 0.8f);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Poussiere, Aylis->GetActorLocation(), FVector::UpVector, FLinearColor(0.45f, 0.45f, 0.7f));
	// Les dons d'esquive des maitres
	const FVector Depart = Aylis->GetActorLocation();
	if (const float Braise = ValeurDon(TEXT("b_esquive")))
	{
		Gerbe(Depart, 220.0f, Braise / 100.0f, VespEffetCoup::Brulure, VespMaitres::Couleur(VespMaitres::Brann));
	}
	if (const float Givre = ValeurDon(TEXT("i_esquive")))
	{
		Gerbe(Depart, 240.0f, Givre / 100.0f, VespEffetCoup::Gel, VespMaitres::Couleur(VespMaitres::Isaure));
	}
	if (const float Leger = ValeurDon(TEXT("o_esquive")))
	{
		Aylis->Soigner(FMath::RoundToInt(Leger * MultSoins()));
	}
	if (const float Bond = ValeurDon(TEXT("v_esquive")))
	{
		RechargeEsquive = FMath::Max(0.25f, RechargeEsquive - Bond / 1000.0f);
	}
	if (const float Presage = ValeurDon(TEXT("a_esquive")))
	{
		PresageAube = 1.5f;
		BonusPresage = Presage;
	}
	// L'Eveil : l'esquive laisse une onde qui blesse, et revient plus vite
	if (Talent(TEXT("eveil")))
	{
		RechargeEsquive -= 0.15f;
		FVespCoup C;
		C.Origine = Aylis->GetActorLocation();
		C.Portee = 230.0f;
		C.DemiAngle = 180.0f;
		C.Puissance = 0.6f;
		C.bPeutCritiquer = false;
		C.Poussee = 350.0f;
		C.Couleur = FLinearColor(0.75f, 0.55f, 1.0f);
		Combat->FrappeDAylis(C);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, C.Couleur);
	}
}

// ===================== Le Seuil : les pouvoirs =====================

int32 AVespPlayerController::EtoileDuPouvoir(int32 N) const
{
	for (int32 i = 0; i < VespSeuil::Nombre; i++)
	{
		if (VespSeuil::Etoile(i).Pouvoir == N)
		{
			return i;
		}
	}
	return -1;
}

float AVespPlayerController::RechargeDuPouvoir(int32 N) const
{
	const int32 E = EtoileDuPouvoir(N);
	return E >= 0 && N >= 1 && N <= 3 ? FMath::Clamp(Recharges[N] / FMath::Max(0.1f, VespSeuil::Etoile(E).Recharge), 0.0f, 1.0f) : 1.0f;
}

void AVespPlayerController::Pouvoir(int32 N)
{
	const int32 E = EtoileDuPouvoir(N);
	if (E < 0 || !EtoileAcquise(E))
	{
		Ecrire(FString::Printf(TEXT("Ce pouvoir s'apprend au Seuil (touche I) : %s."), VespSeuil::Etoile(FMath::Max(0, E)).Nom), 3.0f);
		return;
	}
	if (Recharges[N] > 0.0f)
	{
		return;
	}
	const bool bLibre = Geste == EVespGesteAylis::Libre || ((Geste == EVespGesteAylis::Attaque || Geste == EVespGesteAylis::Lourde) && bImpactFait);
	switch (N)
	{
		case 1:		// le Tourbillon
			if (!bLibre)
			{
				return;
			}
			PouvoirEnCours = 1;
			Geste = EVespGesteAylis::Speciale;
			TempsGeste = 0.0f;
			bImpactFait = false;
			DureeGeste = FMath::Max(0.45f, Aylis->Jouer(EVespGeste::Tourbillon, 1.4f));
			MomentImpact = DureeGeste * 0.4f;
			break;
		case 2:		// l'Egide : 3 secondes de bouclier de lumiere
			TempsEgide = 3.0f;
			AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Buff"), Aylis->GetActorLocation(), FRotator::ZeroRotator, 1.0f);
			break;
		default:	// la Lame d'ether : elle traverse tout
		{
			if (!bLibre)
			{
				return;
			}
			Aylis->Jouer(EVespGeste::Lancer, 1.5f, true);
			FVespCoup C = CoupDeBase();
			C.Origine = Aylis->GetActorLocation() + FVector(0, 0, 90.0f);
			C.Direction = Regard;
			C.Portee = 1150.0f;
			C.Puissance = 1.4f;
			C.Poussee = 300.0f;
			C.Couleur = FLinearColor(0.7f, 0.5f, 1.0f);
			Aylis->TournerDUnCoup(Regard);
			Combat->TirDAylis(C, 1600.0f, 95.0f, true, 1.2f);
			UVespSons::Jouer(this, EVespSon::Sort, Aylis->GetActorLocation(), 0.9f, 0.9f);
			break;
		}
	}
	UVespSons::Jouer(this, EVespSon::Pouvoir, Aylis->GetActorLocation(), 0.8f);
	Recharges[N] = VespSeuil::Etoile(E).Recharge;
}

// Les effets que portent tous les coups : les runes, et les pouvoirs des objets (on garde le plus fort)
FVespCoup AVespPlayerController::CoupDeBase(int32 GesteDuCoup) const
{
	FVespCoup C;
	C.Couleur = FLinearColor(0.55f, 0.7f, 1.0f);
	auto Proposer = [&C](int32 Effet, float Chance) {
		if (Effet != 0 && Chance > C.ChanceEffet)
		{
			C.Effet = Effet;
			C.ChanceEffet = Chance;
		}
	};
	if (bFlamme) Proposer(VespEffetCoup::Brulure, 0.33f);
	if (bGivre) Proposer(VespEffetCoup::Gel, 0.25f);
	if (GesteDuCoup == VespMaitres::Attaque)
	{
		Proposer(VespEffetCoup::Brulure, ValeurDon(TEXT("b_attaque")) / 100.0f);
		Proposer(VespEffetCoup::Gel, ValeurDon(TEXT("i_attaque")) / 100.0f);
		C.BonusCritique += FMath::RoundToInt(ValeurDon(TEXT("a_attaque")));
	}
	else if (GesteDuCoup == VespMaitres::Lourde)
	{
		C.BonusCritique += FMath::RoundToInt(ValeurDon(TEXT("a_lourde")));
	}
	for (int32 e = 0; e < (int32)EVespEmplacement::Nombre; e++)
	{
		if (bEquipe[e])
		{
			Proposer(Equipement[e].Effet, Equipement[e].ChanceEffet);
		}
	}
	if (C.Effet == VespEffetCoup::Brulure) C.Couleur = FLinearColor(1.0f, 0.55f, 0.25f);
	if (C.Effet == VespEffetCoup::Gel) C.Couleur = FLinearColor(0.5f, 0.8f, 1.0f);
	if (C.Effet == VespEffetCoup::Poison) C.Couleur = FLinearColor(0.5f, 1.0f, 0.35f);
	return C;
}

// ===================== Le butin et l'equipement =====================

void AVespPlayerController::LacherButin(const FVector& Ou, int32 Chance)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AVespButin* B = GetWorld()->SpawnActor<AVespButin>(FVector(Ou.X, Ou.Y, Monde->GetActorLocation().Z), FRotator::ZeroRotator, Params);
	if (!B)
	{
		return;
	}
	B->Preparer(VespButin::Tirer(Acte, Chance, HasardButin));
	// Il ne tombe pas hors du chemin
	const FVector P = B->GetActorLocation();
	if (!Monde->EstPraticable(P))
	{
		B->SetActorLocation(Monde->Contraindre(Aylis->GetActorLocation(), P));
	}
	ButinsAuSol.Add(B);
	if (B->Objet.Rarete >= EVespRarete::Epique)
	{
		UVespSons::Jouer(this, EVespSon::Rune, P, 0.7f, 1.2f);
	}
}

void AVespPlayerController::RamasserButin()
{
	for (int32 i = ButinsAuSol.Num() - 1; i >= 0; i--)
	{
		AVespButin* B = ButinsAuSol[i];
		if (!B)
		{
			ButinsAuSol.RemoveAt(i);
			continue;
		}
		if (B->Age < 0.7f || FVector::Dist2D(B->GetActorLocation(), Aylis->GetActorLocation()) > 140.0f)
		{
			continue;
		}
		if (Sac.Num() >= TailleDuSac)
		{
			if (TempsMessage <= 0.0f)
			{
				Ecrire(TEXT("Le sac est plein : ouvre l'inventaire (I) pour faire de la place."), 2.5f);
			}
			continue;
		}
		const FVespObjet O = B->Objet;
		const EVespEmplacement E = O.Emplacement;
		// Un emplacement vide : AYLIS s'en equipe tout de suite
		Sac.Add(O);
		if (!bEquipe[(int32)E])
		{
			EquiperDuSac(Sac.Num() - 1);
		}
		DernierButin = O.Nom;
		CouleurDernierButin = VespButin::CouleurRarete(O.Rarete);
		if (O.Rarete == EVespRarete::Legendaire)
		{
			ParlerEnRoute(LEGENDAIRE, UE_ARRAY_COUNT(LEGENDAIRE), true);
		}
		TempsButin = 4.0f;
		UVespSons::Jouer(this, EVespSon::Ramasser, B->GetActorLocation(), 0.9f);
		AVespEffet::Jouer(GetWorld(), EVespEffet::Etincelles, B->GetActorLocation() + FVector(0, 0, 70), FVector::UpVector, CouleurDernierButin);
		if (Geste == EVespGesteAylis::Libre)
		{
			Aylis->Jouer(EVespGeste::Ramasser, 1.8f, true);
		}
		B->Destroy();
		ButinsAuSol.RemoveAt(i);
	}
}

void AVespPlayerController::AppliquerObjet(const FVespObjet& O, int32 Signe)
{
	FVespStats& S = Aylis->Stats;
	S.Attaque += Signe * O.Attaque;
	S.Defense += Signe * O.Defense;
	S.ChanceCritique += Signe * O.Critique;
	S.PvMax += Signe * O.PvMax;
	S.Pv = Signe > 0 ? S.Pv + O.PvMax : FMath::Clamp(S.Pv, 1, S.PvMax);
	BonusVitesse += Signe * O.Vitesse;
}

void AVespPlayerController::HabillerAylis()
{
	static const TCHAR* COULEURS[4] = {TEXT("Cl"), TEXT("Bl"), TEXT("Gn"), TEXT("Rd")};
	const FVespObjet* A = Equipe(EVespEmplacement::Arme);
	const FVespObjet* B = Equipe(EVespEmplacement::MainGauche);
	const EVespArme Type = A ? A->TypeArme : EVespArme::Poings;
	// Les armes a deux mains, les dagues et le baton laissent le bouclier dans le dos (il ne se voit pas)
	const bool bBouclierVisible = B && (Type == EVespArme::Epee || Type == EVespArme::Poings);
	Aylis->Equiper(A ? A->Modele : FString(), A ? A->Longueur : 0.5f, false, bBouclierVisible ? B->Modele : FString(), Type);
	if (A && Type == EVespArme::Dagues && !A->SecondModele.IsEmpty())
	{
		Aylis->EquiperSecondeArme(A->SecondModele, A->Longueur);
	}
	if (A) Aylis->ColorerArme(A->Modele, COULEURS[(int32)A->Rarete]);
	if (bBouclierVisible) Aylis->ColorerArme(B->Modele, COULEURS[(int32)B->Rarete]);
	const FVespObjet* Corps = Equipe(EVespEmplacement::Corps);
	Aylis->TeindreTenue(Corps ? Corps->Teinte : FLinearColor(0.25f, 0.39f, 0.88f));
}

void AVespPlayerController::EquiperDuSac(int32 IndexSac)
{
	if (!Sac.IsValidIndex(IndexSac))
	{
		return;
	}
	const FVespObjet O = Sac[IndexSac];
	const int32 E = (int32)O.Emplacement;
	Sac.RemoveAt(IndexSac);
	if (bEquipe[E])
	{
		AppliquerObjet(Equipement[E], -1);
		Sac.Insert(Equipement[E], FMath::Min(IndexSac, Sac.Num()));		// l'ancien prend sa place dans le sac
	}
	Equipement[E] = O;
	bEquipe[E] = true;
	AppliquerObjet(O, +1);
	HabillerAylis();
	UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f, 0.9f);
	VueFiche++;
}

void AVespPlayerController::Retirer(EVespEmplacement E)
{
	const int32 i = (int32)E;
	if (!bEquipe[i] || Sac.Num() >= TailleDuSac)
	{
		return;
	}
	AppliquerObjet(Equipement[i], -1);
	Sac.Add(Equipement[i]);
	bEquipe[i] = false;
	HabillerAylis();
	UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f, 0.8f);
}

void AVespPlayerController::JeterDuSac(int32 IndexSac)
{
	if (!Sac.IsValidIndex(IndexSac))
	{
		return;
	}
	// Jeter un objet rend quelques eclats (la prophetie recupere sa lumiere)
	GagnerEclats(3 + (int32)Sac[IndexSac].Rarete * 6, Aylis->GetActorLocation());
	Sac.RemoveAt(IndexSac);
	SelectionSac = FMath::Clamp(SelectionSac, 0, FMath::Max(0, Sac.Num() - 1));
	VueFiche++;
}

// ===================== Le Seuil =====================

bool AVespPlayerController::EtoilePossible(int32 Index) const
{
	if (!Seuil.IsValidIndex(Index) || Seuil[Index] || PointsDeCompetence <= 0)
	{
		return false;
	}
	const FVespEtoile& E = VespSeuil::Etoile(Index);
	for (int32 i = 0; i < VespSeuil::Nombre; i++)
	{
		const FVespEtoile& Autre = VespSeuil::Etoile(i);
		if (Autre.Voie == E.Voie && Autre.Rang == E.Rang - 1 && !Seuil[i])
		{
			return false;
		}
	}
	return true;
}

void AVespPlayerController::DebloquerEtoile(int32 Index)
{
	if (!EtoilePossible(Index))
	{
		return;
	}
	Seuil[Index] = true;
	PointsDeCompetence--;
	const FString Id = VespSeuil::Etoile(Index).Id;
	FVespStats& S = Aylis->Stats;
	if (Id == TEXT("affutee")) { S.ChanceCritique += 15; Combat->MultCritique = 2.5f; }
	else if (Id == TEXT("execution")) { Combat->bExecution = true; }
	else if (Id == TEXT("garde")) { Combat->FenetreParade = 0.35f; Combat->BonusParade = FMath::Min(1.0f, Combat->BonusParade + 0.5f); }
	else if (Id == TEXT("ecorce")) { S.PvMax += 20; S.Pv += 20; S.Defense += 2; }
	UVespSons::Jouer2D(this, EVespSon::Rune, 1.0f, 1.1f);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Soin, Aylis->GetActorLocation(), FVector::UpVector, VespSeuil::CouleurVoie(VespSeuil::Etoile(Index).Voie));
}

// ===================== Le menu d'AYLIS =====================

void AVespPlayerController::OuvrirMenu(int32 Onglet)
{
	if (Phase != EVespPhase::Exploration)
	{
		return;
	}
	bMenuOuvert = true;
	OngletMenu = Onglet;
	bCarteOuverte = false;
	SelectionSac = FMath::Clamp(SelectionSac, 0, FMath::Max(0, Sac.Num() - 1));
	UVespSons::Jouer2D(this, EVespSon::Clic);
	if (bGardeLevee)
	{
		bGardeLevee = false;
		Aylis->TenirGarde(false);
	}
}

void AVespPlayerController::FermerMenu()
{
	bMenuOuvert = false;
	bConfirmerAbandon = false;
	UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f, 0.8f);
}

void AVespPlayerController::CommandesDuMenu(float Secondes)
{
	auto Appui = [this](std::initializer_list<FKey> Touches) {
		for (const FKey& K : Touches)
		{
			if (WasInputKeyJustPressed(K))
			{
				return true;
			}
		}
		return false;
	};
	if (Appui({EKeys::I, EKeys::Escape, EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Right}))
	{
		FermerMenu();
		return;
	}
	if (Appui({EKeys::Tab, EKeys::Gamepad_RightShoulder}) || Appui({EKeys::Gamepad_LeftShoulder}))
	{
		OngletMenu = (OngletMenu + (WasInputKeyJustPressed(EKeys::Gamepad_LeftShoulder) ? 2 : 1)) % 3;
		bConfirmerAbandon = false;
		UVespSons::Jouer2D(this, EVespSon::Survol);
		return;
	}
	if (OngletMenu == 2)
	{
		CommandesDesReglages(Secondes, LignesReglages);
		return;
	}
	FIntPoint D;
	const bool bDirection = DirectionPressee(D, Secondes);
	if (OngletMenu == 0)
	{
		if (bDirection && Sac.Num() > 0)
		{
			SelectionSac = FMath::Clamp(SelectionSac + D.X + D.Y * 6, 0, Sac.Num() - 1);
			VueFiche++;
			UVespSons::Jouer2D(this, EVespSon::Survol);
		}
		if (Appui({EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom}))
		{
			EquiperDuSac(SelectionSac);
		}
		if (Appui({EKeys::Delete, EKeys::X, EKeys::Gamepad_FaceButton_Left}))
		{
			JeterDuSac(SelectionSac);
		}
	}
	else if (OngletMenu == 1)
	{
		if (bDirection)
		{
			// Gauche / droite : la voie ; haut / bas : le rang
			const FVespEtoile& E = VespSeuil::Etoile(SelectionEtoile);
			const int32 Voie = FMath::Clamp((int32)E.Voie + D.X, 0, 2);
			const int32 Rang = FMath::Clamp(E.Rang - D.Y, 1, 4);
			for (int32 i = 0; i < VespSeuil::Nombre; i++)
			{
				if ((int32)VespSeuil::Etoile(i).Voie == Voie && VespSeuil::Etoile(i).Rang == Rang)
				{
					SelectionEtoile = i;
				}
			}
			UVespSons::Jouer2D(this, EVespSon::Survol);
		}
		if (Appui({EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom}))
		{
			DebloquerEtoile(SelectionEtoile);
		}
	}
}

void AVespPlayerController::BoirePotion()
{
	if (Potions <= 0 || Geste != EVespGesteAylis::Libre)
	{
		if (Potions <= 0)
		{
			Aylis->AfficherMessage(TEXT("plus de potions"), FColor(200, 190, 220), 30.0f);
		}
		return;
	}
	Potions--;
	if (Potions == 0)
	{
		ParlerEnRoute(PLUS_DE_POTIONS, UE_ARRAY_COUNT(PLUS_DE_POTIONS));
	}
	Geste = EVespGesteAylis::Potion;
	TempsGeste = 0.0f;
	bPotionBue = false;
	DureeGeste = FMath::Max(0.6f, Aylis->Jouer(EVespGeste::Potion, 1.5f, true));
	bGardeLevee = false;
	Aylis->TenirGarde(false);
}

void AVespPlayerController::AttaqueSpeciale()
{
	if (Rage < 100 || (Geste != EVespGesteAylis::Libre && Geste != EVespGesteAylis::Attaque))
	{
		return;
	}
	Rage = 0;
	PouvoirEnCours = 0;
	Geste = EVespGesteAylis::Speciale;
	TempsGeste = 0.0f;
	bImpactFait = false;
	DureeGeste = FMath::Max(0.5f, Aylis->Jouer(EVespGeste::Tourbillon, 1.1f));
	MomentImpact = DureeGeste * 0.45f;
	Aylis->Invulnerable = DureeGeste;
	bGardeLevee = false;
	Aylis->TenirGarde(false);
	UVespSons::Jouer(this, EVespSon::Pouvoir, Aylis->GetActorLocation(), 1.0f);
	AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + FVector(0, 0, 10), FVector::UpVector, FLinearColor(0.6f, 0.5f, 1.0f));
}

void AVespPlayerController::AvancerGeste(float Secondes)
{
	TempsGeste += Secondes;
	RechargeEsquive = FMath::Max(0.0f, RechargeEsquive - Secondes);
	PresageAube = FMath::Max(0.0f, PresageAube - Secondes);
	FinCombo = FMath::Max(0.0f, FinCombo - Secondes);
	switch (Geste)
	{
		case EVespGesteAylis::Attaque:
		case EVespGesteAylis::Lourde:
		{
			const bool bLourde = Geste == EVespGesteAylis::Lourde;
			// AYLIS se fend vers sa cible jusqu'a l'impact
			if (TempsGeste < MomentImpact && ElanGeste > 0.0f)
			{
				Aylis->Deplacer(DirectionGeste * (ElanGeste / FMath::Max(0.05f, MomentImpact)), Secondes, false);
			}
			if (!bImpactFait && TempsGeste >= MomentImpact)
			{
				bImpactFait = true;
				const EVespArme Type = ArmeEnMain();
				FVespCoup C = CoupDeBase(bLourde ? VespMaitres::Lourde : VespMaitres::Attaque);
				C.Origine = Aylis->GetActorLocation();
				C.Direction = DirectionGeste;
				C.Portee = bLourde ? 240.0f : 205.0f;
				C.DemiAngle = bLourde ? 85.0f : 70.0f;
				C.Puissance = bLourde ? 2.1f : (Combo == 2 ? 1.45f : 1.0f);
				C.bLourd = bLourde;
				C.Poussee = bLourde ? 650.0f : (Combo == 2 ? 480.0f : 220.0f);
				if (Type == EVespArme::Dagues)
				{
					C.Portee = 185.0f;
					C.DemiAngle = 65.0f;
					C.Puissance = bLourde ? 1.8f : (Combo == 2 ? 1.0f : 0.7f);
				}
				else if (Type == EVespArme::DeuxMains)
				{
					C.Portee = bLourde ? 270.0f : 255.0f;
					C.DemiAngle = 95.0f;
					C.Puissance = bLourde ? 2.3f : (Combo == 2 ? 1.5f : 1.1f);
					C.Poussee *= 1.3f;
				}
				if (bRiposte)
				{
					C.Puissance *= 3.0f;
					bRiposte = false;
				}
				// Isaure : la lourde gele toujours, et frappe plus fort
				if (bLourde && RareteDon(TEXT("i_lourde")) >= 0)
				{
					C.Effet = VespEffetCoup::Gel;
					C.ChanceEffet = 1.0f;
					C.Puissance *= 1.0f + ValeurDon(TEXT("i_lourde")) / 100.0f;
				}
				// Aurel : le presage d'une esquive
				if (PresageAube > 0.0f)
				{
					C.BonusCritique += FMath::RoundToInt(BonusPresage);
					PresageAube = 0.0f;
				}
				int32 Touches = 0;
				if (Type == EVespArme::Baton)
				{
					// Le baton : chaque coup part en sort (l'attaque lourde, une grosse boule qui repousse)
					C.Origine += FVector(0, 0, 110.0f) + DirectionGeste * 60.0f;
					C.Portee = 950.0f;
					C.Puissance = bLourde ? 2.0f : 0.95f;
					C.Couleur = FLinearColor(0.7f, 0.5f, 1.0f);
					Combat->TirDAylis(C, bLourde ? 1100.0f : 1500.0f, bLourde ? 110.0f : 60.0f, false, bLourde ? 1.8f : 1.0f);
					UVespSons::Jouer(this, EVespSon::Sort, Aylis->GetActorLocation(), 0.8f, bLourde ? 0.8f : 1.1f);
				}
				else
				{
					Touches = Combat->FrappeDAylis(C);
					AVespEffet::Jouer(GetWorld(), EVespEffet::Trainee, Aylis->GetActorLocation() + FVector(0, 0, Aylis->Taille * 0.6f), DirectionGeste,
					                  bLourde ? FLinearColor(1.0f, 0.8f, 0.4f) : C.Couleur);
				}
				// Le Fendant : l'attaque lourde envoie une vague de lumiere
				if (bLourde && Type != EVespArme::Baton && Talent(TEXT("fendant")))
				{
					FVespCoup Vague = C;
					Vague.Origine = Aylis->GetActorLocation() + FVector(0, 0, 60.0f);
					Vague.Portee = 750.0f;
					Vague.Puissance = 1.0f;
					Vague.bLourd = false;
					Vague.Couleur = FLinearColor(1.0f, 0.8f, 0.4f);
					Combat->TirDAylis(Vague, 1400.0f, 110.0f, true, 1.3f);
				}
				if (Touches > 0)
				{
					Rage = FMath::Min(100, Rage + Touches * (bFureur ? 6 : 3) * (bLourde ? 2 : 1));
					int32 Vol = (bSangsue ? 1 : 0) + (bLourde ? 0 : FMath::RoundToInt(ValeurDon(TEXT("o_attaque")) * MultSoins()));
					for (int32 e = 0; e < (int32)EVespEmplacement::Nombre; e++)
					{
						Vol += bEquipe[e] ? Equipement[e].VolDeVie : 0;
					}
					if (Vol > 0)
					{
						Aylis->Soigner(Touches * Vol);
					}
				}
				// Les dons des maitres, apres l'impact
				const FVector Devant = Aylis->GetActorLocation() + DirectionGeste * 180.0f;
				if (bLourde)
				{
					if (const float Forge = ValeurDon(TEXT("b_lourde")))
					{
						Gerbe(Devant, 300.0f, Forge / 100.0f, VespEffetCoup::Brulure, VespMaitres::Couleur(VespMaitres::Brann));
					}
					if (const float Racines = ValeurDon(TEXT("o_lourde")))
					{
						Aylis->Soigner(FMath::RoundToInt(Aylis->Stats.PvMax * Racines / 100.0f * FMath::Min(Touches, 3) * MultSoins()));
					}
					if (const float Chaine = ValeurDon(TEXT("v_lourde")))
					{
						FoudreSurProches(3, 700.0f, Chaine / 100.0f);
					}
				}
				else if (Combo == 2)
				{
					if (const float Eclair = ValeurDon(TEXT("v_attaque")))
					{
						Foudre(Devant, 280.0f, Eclair / 100.0f);
					}
				}
				if (bLourde)
				{
					Trembler(0.3f);
					AVespEffet::Jouer(GetWorld(), EVespEffet::Onde, Aylis->GetActorLocation() + DirectionGeste * 150.0f + FVector(0, 0, 10), FVector::UpVector, FLinearColor(1.0f, 0.8f, 0.4f));
				}
			}
			if (bCoupSuivant && TempsGeste >= DureeGeste * 0.6f)
			{
				Attaquer(false);
				return;
			}
			if (TempsGeste >= DureeGeste)
			{
				Geste = EVespGesteAylis::Libre;
				FinCombo = bLourde ? 0.0f : 0.45f;
			}
			break;
		}
		case EVespGesteAylis::Esquive:
		{
			// Rapide au debut, puis elle freine (la rune du Vent porte plus loin)
			const float T = FMath::Clamp(TempsGeste / DureeGeste, 0.0f, 1.0f);
			const float Distance = 470.0f * (1.0f + BonusVitesse) * (RareteDon(TEXT("v_esquive")) >= 0 ? 1.2f : 1.0f);
			const float Vmax = 2.0f * Distance / DureeGeste;
			Aylis->Deplacer(DirectionGeste * Vmax * (1.0f - T), Secondes, false);
			if (TempsGeste >= DureeGeste)
			{
				Geste = EVespGesteAylis::Libre;
			}
			break;
		}
		case EVespGesteAylis::Potion:
			if (!bPotionBue && TempsGeste >= DureeGeste * 0.45f)
			{
				bPotionBue = true;
				Aylis->Soigner(FMath::RoundToInt(Aylis->Stats.PvMax * 0.4f * MultSoins()));
				Aylis->Poison = 0.0f;
				Aylis->Brulure = 0.0f;
				UVespSons::Jouer(this, EVespSon::Potion, Aylis->GetActorLocation(), 0.9f);
			}
			if (TempsGeste >= DureeGeste)
			{
				Geste = EVespGesteAylis::Libre;
			}
			break;
		case EVespGesteAylis::Speciale:
			if (!bImpactFait && TempsGeste >= MomentImpact)
			{
				bImpactFait = true;
				// L'attaque speciale (la rage), ou le tourbillon du Seuil ; la Nova l'agrandit et soigne
				const bool bTourbillon = PouvoirEnCours == 1;
				const bool bNova = !bTourbillon && Talent(TEXT("nova"));
				FVespCoup C = CoupDeBase();
				C.Origine = Aylis->GetActorLocation();
				C.Direction = Aylis->Avant();
				C.Portee = bTourbillon ? 290.0f : (bNova ? 520.0f : 340.0f);
				C.DemiAngle = 180.0f;
				C.Puissance = bTourbillon ? 1.8f : 2.4f;
				C.bLourd = !bTourbillon;
				C.Poussee = 800.0f;
				C.Couleur = bTourbillon ? FLinearColor(1.0f, 0.65f, 0.35f) : FLinearColor(0.7f, 0.5f, 1.0f);
				// Les dons de la speciale (pas le Tourbillon du Seuil)
				if (!bTourbillon)
				{
					if (const float Nova = ValeurDon(TEXT("b_speciale")))
					{
						C.Puissance *= 1.0f + Nova / 100.0f;
						C.Effet = VespEffetCoup::Brulure;
						C.ChanceEffet = 1.0f;
						C.Couleur = VespMaitres::Couleur(VespMaitres::Brann);
					}
					if (const float Tempete = ValeurDon(TEXT("i_speciale")))
					{
						C.Puissance *= 1.0f + Tempete / 100.0f;
						C.Effet = VespEffetCoup::Gel;
						C.ChanceEffet = 1.0f;
						C.Couleur = VespMaitres::Couleur(VespMaitres::Isaure);
					}
					if (const float Jour = ValeurDon(TEXT("a_speciale")))
					{
						C.Portee *= 1.0f + Jour / 100.0f;
						C.BonusCritique += 100;
					}
				}
				Combat->FrappeDAylis(C);
				if (!bTourbillon)
				{
					if (const float Floraison = ValeurDon(TEXT("o_speciale")))
					{
						Aylis->Soigner(FMath::RoundToInt(Aylis->Stats.PvMax * Floraison / 100.0f * MultSoins()));
					}
					if (const float Pluie = ValeurDon(TEXT("v_speciale")))
					{
						FoudreSurProches(6, 900.0f, Pluie / 100.0f);
					}
				}
				Trembler(bTourbillon ? 0.4f : 0.9f);
				AVespEffet::JouerMagie(GetWorld(), TEXT("NS_Free_Magic_Circle2"), Aylis->GetActorLocation() + FVector(0, 0, 5), FRotator::ZeroRotator, bNova ? 1.6f : (bTourbillon ? 0.7f : 1.0f));
				if (bNova)
				{
					Aylis->Soigner(Aylis->Stats.PvMax / 5);
				}
			}
			if (TempsGeste >= DureeGeste)
			{
				Geste = EVespGesteAylis::Libre;
			}
			break;
		case EVespGesteAylis::Touchee:
			if (TempsGeste >= 0.32f)
			{
				Geste = EVespGesteAylis::Libre;
			}
			break;
		default: break;
	}
}

// ===================== Le jeu : a chaque image =====================

void AVespPlayerController::Jouer(float Secondes)
{
	// La carte du monde
	if (WasInputKeyJustPressed(EKeys::Tab) || WasInputKeyJustPressed(EKeys::M) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Left))
	{
		bCarteOuverte = !bCarteOuverte;
		UVespSons::Jouer2D(this, EVespSon::Clic);
	}
	auto Tenue = [this](std::initializer_list<FKey> Touches) {
		for (const FKey& K : Touches)
		{
			if (IsInputKeyDown(K))
			{
				return true;
			}
		}
		return false;
	};
	auto Appui = [this](std::initializer_list<FKey> Touches) {
		for (const FKey& K : Touches)
		{
			if (WasInputKeyJustPressed(K))
			{
				return true;
			}
		}
		return false;
	};
	// La direction : ZQSD (AZERTY), WASD (QWERTY), les fleches, ou le stick gauche
	FVector2D Entree(0.0f, 0.0f);
	if (Tenue({EKeys::Z, EKeys::W, EKeys::Up})) Entree.Y += 1.0f;
	if (Tenue({EKeys::S, EKeys::Down})) Entree.Y -= 1.0f;
	if (Tenue({EKeys::Q, EKeys::A, EKeys::Left})) Entree.X -= 1.0f;
	if (Tenue({EKeys::D, EKeys::Right})) Entree.X += 1.0f;
	const FVector2D Stick(GetInputAnalogKeyState(EKeys::Gamepad_LeftX), GetInputAnalogKeyState(EKeys::Gamepad_LeftY));
	if (Stick.Size() > 0.2f)
	{
		Entree += Stick;
	}
	const float Force = FMath::Min(1.0f, Entree.Size());
	if (Force > 0.05f)
	{
		Entree /= Entree.Size();
	}
	// Le haut de l'ecran, et sa droite, sur le sol
	const float Yaw = FMath::DegreesToRadians(RotationCamera.Yaw);
	const FVector Avant(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.0f);
	const FVector Droite(-FMath::Sin(Yaw), FMath::Cos(Yaw), 0.0f);
	Deplacement = (Avant * Entree.Y + Droite * Entree.X) * Force;
	// La souris a bouge : on vise avec elle pendant quelques secondes
	float SourisX = 0.0f, SourisY = 0.0f;
	GetMousePosition(SourisX, SourisY);
	if (FVector2D(SourisX, SourisY) != DerniereSouris)
	{
		if (!DerniereSouris.IsZero())
		{
			TempsSouris = 3.0f;
		}
		DerniereSouris = FVector2D(SourisX, SourisY);
	}
	TempsSouris = FMath::Max(0.0f, TempsSouris - Secondes);
	Regard = DirectionVisee();

	// Le combat avance (les Haschen, les attaques annoncees, les projectiles)
	Combat->Avancer(Secondes);
	if (Phase != EVespPhase::Exploration)
	{
		return;		// (une clairiere de boss vient de se fermer : le dialogue commence)
	}
	const bool bEnCombat = Combat->EnCombat();

	// Les commandes
	if (!bCarteOuverte)
	{
		if (Appui({EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom}))
		{
			Esquiver(Deplacement);
		}
		else if (Appui({EKeys::LeftMouseButton, EKeys::J, EKeys::Gamepad_FaceButton_Left}))
		{
			Attaquer(false);
		}
		else if (Appui({EKeys::RightMouseButton, EKeys::K, EKeys::Gamepad_FaceButton_Top}))
		{
			Attaquer(true);
		}
		else if (Appui({EKeys::R, EKeys::Gamepad_DPad_Up}))
		{
			BoirePotion();
		}
		else if (Appui({EKeys::V, EKeys::Gamepad_RightShoulder}))
		{
			AttaqueSpeciale();
		}
		else if (Appui({EKeys::One, EKeys::NumPadOne, EKeys::Gamepad_LeftShoulder}))
		{
			Pouvoir(1);
		}
		else if (Appui({EKeys::Two, EKeys::NumPadTwo}) || (GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) > 0.5f && Recharges[2] <= 0.0f && Talent(TEXT("egide"))))
		{
			Pouvoir(2);
		}
		else if (Appui({EKeys::Three, EKeys::NumPadThree, EKeys::Gamepad_DPad_Right}))
		{
			Pouvoir(3);
		}
		else if (Appui({EKeys::I, EKeys::Gamepad_Special_Right}))
		{
			OuvrirMenu(0);
			return;
		}
		else if (Appui({EKeys::Escape}))
		{
			OuvrirMenu(2);			// la pause : les options, abandonner, quitter
			return;
		}
	}
	// Les recharges des pouvoirs, l'Egide
	for (float& R : Recharges)
	{
		R = FMath::Max(0.0f, R - Secondes);
	}
	RechargePresage = FMath::Max(0.0f, RechargePresage - Secondes);
	TempsButin = FMath::Max(0.0f, TempsButin - Secondes);
	TempsEgide = FMath::Max(0.0f, TempsEgide - Secondes);
	if (TempsEgide > 0.0f && !BulleEgide)
	{
		BulleEgide = NewObject<UStaticMeshComponent>(Aylis);
		BulleEgide->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		BulleEgide->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BulleEgide->SetCastShadow(false);
		BulleEgide->SetupAttachment(Aylis->GetRootComponent());
		BulleEgide->RegisterComponent();
		BulleEgide->SetRelativeLocation(FVector(0, 0, 95.0f));
		BulleEgide->SetRelativeScale3D(FVector(2.3f));
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(AVespEffet::MateriauLumineux(), this);
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.45f, 1.0f) * 0.35f);
		BulleEgide->SetMaterial(0, M);
	}
	else if (TempsEgide <= 0.0f && BulleEgide)
	{
		BulleEgide->DestroyComponent();
		BulleEgide = nullptr;
	}
	RamasserButin();
	// La garde : tenue tant qu'on appuie (et qu'AYLIS n'est pas occupee)
	const bool bVeutGarde = !bCarteOuverte && (Tenue({EKeys::LeftShift, EKeys::RightShift, EKeys::F}) || GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis) > 0.4f);
	const bool bPeutGarde = Geste == EVespGesteAylis::Libre;
	if (bVeutGarde && bPeutGarde)
	{
		if (!bGardeLevee)
		{
			bGardeLevee = true;
			TempsGarde = 0.0f;
			Aylis->TenirGarde(true);
			Aylis->Jouer(EVespGeste::GardeLevee, 1.6f, true);
		}
		TempsGarde += Secondes;
	}
	else if (bGardeLevee)
	{
		bGardeLevee = false;
		Aylis->TenirGarde(false);
	}
	Combat->SetGarde(bGardeLevee, TempsGarde);
	if (Meteo)
	{
		Meteo->Blizzard(Combat->BlizzardEnCours() > 0.0f);
	}
	Aylis->ReductionDegats = TempsEgide > 0.0f ? 0.2f : 1.0f;

	AvancerGeste(Secondes);

	// La marche : plus lente en garde, pendant un coup ou une potion ; immobile pendant l'attaque lourde
	float Vitesse = 520.0f * (1.0f + BonusVitesse);
	switch (Geste)
	{
		case EVespGesteAylis::Attaque: Vitesse *= 0.15f; break;
		case EVespGesteAylis::Potion: Vitesse *= 0.45f; break;
		case EVespGesteAylis::Libre: Vitesse *= bGardeLevee ? 0.38f : 1.0f; break;
		default: Vitesse = 0.0f; break;
	}
	if (Combat->BlizzardEnCours() > 0.0f)
	{
		Vitesse *= 0.7f;
	}
	if (Vitesse > 0.0f && Aylis->EstDebout())
	{
		// En combat (ou en garde), AYLIS fait face a ce qu'elle vise ; sinon, elle regarde ou elle va
		const bool bFaceAuRegard = bGardeLevee || (bEnCombat && TempsSouris > 0.0f);
		Aylis->Deplacer(Deplacement * Vitesse, Secondes, !bFaceAuRegard && Geste == EVespGesteAylis::Libre);
		if (bFaceAuRegard && Geste == EVespGesteAylis::Libre)
		{
			Aylis->Tourner(Regard, Secondes, 16.0f);
		}
	}

	// Une clairiere sans Haschen (repos, tresor, evenement, marchand) : on y entre
	if (!bEnCombat)
	{
		for (int32 i = 0; i < Noeuds.Num(); i++)
		{
			const FVespNoeud& N = Noeuds[i];
			const bool bSansHaschen = N.Type == EVespSalle::Repos || N.Type == EVespSalle::Tresor || N.Type == EVespSalle::Evenement || N.Type == EVespSalle::Marchand;
			if (!N.bVisite && bSansHaschen && FVector::Dist2D(Aylis->GetActorLocation(), N.Centre) < 480.0f)
			{
				bCarteOuverte = false;
				Declencher(i);
				return;
			}
		}
	}
	// Un marchand deja rencontre : on peut lui reparler
	MarchandProche = -1;
	for (int32 i = 0; i < Noeuds.Num(); i++)
	{
		if (Noeuds[i].Type == EVespSalle::Marchand && FVector::Dist2D(Aylis->GetActorLocation(), Noeuds[i].Centre) < 700.0f)
		{
			MarchandProche = i;
		}
	}
	if (MarchandProche >= 0 && !bEnCombat && Appui({EKeys::E, EKeys::Gamepad_FaceButton_Right}))
	{
		RouvrirMarchand(MarchandProche);
	}
}

// ===================== La camera =====================
// Sur l'ecran titre, elle tourne lentement autour d'AYLIS. En jeu, elle suit AYLIS, toujours sous le meme angle ;
// pendant un combat, elle recule un peu (on voit toute la clairiere), tremble apres un coup, se resserre sur un critique.

void AVespPlayerController::PlacerCamera(float Secondes)
{
	if (!CameraArene)
	{
		return;
	}
	if (Phase == EVespPhase::Titre)
	{
		const FVector Centre = Aylis->GetActorLocation();
		const float Angle = FMath::Sin(TempsPhase * 0.1f) * 0.5f;
		const FVector Ecart = DecalageCamera.RotateAngleAxis(FMath::RadiansToDegrees(Angle), FVector::UpVector) * 0.7f;
		const FVector Position = Centre + FVector(Ecart.X, Ecart.Y, Ecart.Z * 0.5f);
		CameraArene->SetActorLocation(Position);
		CameraArene->SetActorRotation((Centre + FVector(0, 0, 180) - Position).Rotation());
		bCaleCamera = true;
		return;
	}
	const FVespGroupe* Clairiere = Combat ? Combat->GroupeFerme() : nullptr;
	const float ReculVoulu = Clairiere ? (Clairiere->bBoss ? 1.0f : 0.8f) : 0.0f;
	Recul = FMath::FInterpTo(Recul, ReculVoulu, Secondes, 1.5f);
	FVector Cible = Aylis->GetActorLocation() + FVector(0, 0, 60);
	if (Clairiere)
	{
		Cible = FMath::Lerp(Cible, Clairiere->Centre + FVector(0, 0, 60), 0.25f * Recul);
	}
	const FVector Voulue = Cible + DecalageCamera * (0.78f + 0.2f * Recul);
	CameraActuelle = bCaleCamera ? Voulue : FMath::VInterpTo(CameraActuelle, Voulue, Secondes, 5.0f);
	bCaleCamera = false;
	Secousse = FMath::Max(0.0f, Secousse - Secondes * 3.0f);
	Zoom = FMath::Max(0.0f, Zoom - Secondes * 3.0f);
	CameraArene->SetActorLocation(CameraActuelle + FMath::VRand() * Secousse * 10.0f);
	CameraArene->SetActorRotation(RotationCamera);
	CameraArene->GetCameraComponent()->SetFieldOfView(46.0f - Zoom * 4.0f);
	Monde->OrienterTextes(CameraActuelle);
	// L'oreille du joueur est sur AYLIS
	if (UVespSons* Sons = GetWorld()->GetSubsystem<UVespSons>())
	{
		Sons->PlacerOreille(this, Aylis->GetActorLocation() + FVector(0, 0, 150), RotationCamera);
		Sons->Tension(Phase == EVespPhase::Exploration ? (Combat->BossActif() ? 1.0f : (Clairiere ? 0.75f : (Combat->EnCombat() ? 0.45f : 0.0f))) : 0.0f);
	}
}

// ===================== A chaque image =====================

void AVespPlayerController::PlayerTick(float Secondes)
{
	Super::PlayerTick(Secondes);
	if (!Aylis || !Monde || !Combat)
	{
		return;
	}
	// Le ralenti se compte en vraies secondes (le temps du jeu, lui, est ralenti)
	if (FinDuRalenti > 0.0 && FPlatformTime::Seconds() >= FinDuRalenti)
	{
		FinDuRalenti = 0.0;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	}
	TempsPhase += Secondes;
	// Le chronometre : du vrai temps (pas ralenti par les effets), seulement quand on joue
	if (ChronoEnMarche())
	{
		const float Vrai = FMath::Min((float)FApp::GetDeltaTime(), 0.25f);
		ChronoActe += Vrai;
		ChronoPartie += Vrai;
		if (Memoire)
		{
			Memoire->TempsDeJeu += Vrai;
		}
	}
	TempsMessage = FMath::Max(0.0f, TempsMessage - Secondes);
	TempsParole = FMath::Max(0.0f, TempsParole - Secondes);
	if (Phase == EVespPhase::Exploration)
	{
		ProchaineParole = FMath::Max(0.0f, ProchaineParole - Secondes);
	}
	if (bPvBasDit && Aylis->Stats.Pv * 2 > Aylis->Stats.PvMax)
	{
		bPvBasDit = false;
	}
	TempsNiveau = FMath::Max(0.0f, TempsNiveau - Secondes);
	if (bTestOuverture)
	{
		if (Phase == EVespPhase::Titre && TempsPhase > 25.0f) NouvellePartie(1);
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) ContinuerApresLActe();
		else if (Phase == EVespPhase::Dialogue && TempsPhase > 3.0f && LigneDialogue < 2) { UE_LOG(LogTemp, Display, TEXT("VESPERANCE test : %s / %s"), *Orateurs[LigneDialogue], *Repliques[LigneDialogue]); Photographier(FString::Printf(TEXT("Ouverture%d"), LigneDialogue), true); Ecriture = 9999.0f; AvancerDialogue(); TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Dialogue && LigneDialogue >= 2) { Quitter(); return; }
	}
	if (EtapeTestMaitres >= 0)
	{
		if (Phase == EVespPhase::Titre && TempsPhase > 25.0f) NouvellePartie(1);
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) ContinuerApresLActe();
		else if (Phase == EVespPhase::Dialogue && TempsPhase > 0.5f) { Ecriture = 9999.0f; AvancerDialogue(); TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Exploration && TempsPhase > 1.5f)
		{
			switch (EtapeTestMaitres++)
			{
				case 0: ProposerDons(VespMaitres::Brann, false); break;
				case 1: PrendreDon(VespMaitres::Index(TEXT("i_attaque")), 1); ProposerDons(VespMaitres::Brann, true); break;
				case 2: ProposerPactes(); break;
				case 3: bCarteOuverte = true; TempsPhase = 0.0f; break;
				default: Photographier(TEXT("Maitres_Carte"), true); Quitter(); return;
			}
		}
		else if (Phase == EVespPhase::ChoixRune && TempsPhase > 1.5f)
		{
			Photographier(FString::Printf(TEXT("Maitres_%d"), EtapeTestMaitres), true);
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE test maitres : %s / %s / %s"), *TitreOffre(0), *TitreOffre(1), *TitreOffre(2));
			ChoisirRune(0);
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE test maitres : %s ; %d dons"), *MessageRoute, DonsPris.Num());
		}
	}
	if (bTestRecompenses)
	{
		if (Phase == EVespPhase::Titre && TempsPhase > 25.0f) NouvellePartie(1);
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) ContinuerApresLActe();
		else if (Phase == EVespPhase::Dialogue && TempsPhase > 0.5f) { Ecriture = 9999.0f; AvancerDialogue(); TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Exploration && !bFinLancee && TempsPhase > 2.0f)
		{
			// AYLIS devant la premiere fourche : les clairieres suivantes et leurs recompenses
			bFinLancee = true;
			Aylis->Teleporter(FMath::Lerp(Noeuds[2].Centre, Noeuds[3].Centre, 0.5f));
			DecalageCamera *= 2.4f;		// (le test recule la camera pour voir les balises des deux clairieres)
			bCaleCamera = true;
			TempsPhase = 0.0f;
			FString Liste;
			for (const FVespNoeud& N : Noeuds)
			{
				Liste += FString::Printf(TEXT("%s%s "), *LettreSalle(N.Type), N.Recompense != EVespRecompense::Aucune ? *(TEXT(":") + EtiquetteRecompense(N)) : TEXT(""));
			}
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE test recompenses : %s"), *Liste);
		}
		else if (Phase == EVespPhase::Exploration && bFinLancee && !bCarteOuverte && TempsPhase > 2.5f) { Photographier(TEXT("Recompenses_Monde"), true); bCarteOuverte = true; TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Exploration && bCarteOuverte && TempsPhase > 1.5f) { Photographier(TEXT("Recompenses_Carte"), true); Quitter(); return; }
	}
	if (TestReprise == 1)
	{
		if (Phase == EVespPhase::Titre && TempsPhase > 25.0f) NouvellePartie(2);
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) ContinuerApresLActe();
		else if (Phase == EVespPhase::Dialogue && TempsPhase > 0.5f) { Ecriture = 9999.0f; AvancerDialogue(); TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Exploration && TempsPhase > 3.0f)
		{
			// des progres : une clairiere faite, des eclats, une rune, AYLIS un peu plus loin
			Noeuds[1].bVisite = true;
			Monde->MarquerZoneFaite(1);
			Combat->LibererSansCombat(1);
			Eclats = 123;
			PrendreDon(0, 1);
			Aylis->Teleporter(Noeuds[1].Centre);
			bCaleCamera = true;
			SauverPartie();
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE test reprise : vision de cote (graine %d, %d clairieres)"), GraineCarte, Noeuds.Num());
			PlacerCamera(0.0f);
			Photographier(TEXT("Reprise_Avant"), true);
			bVisionEnCours = false;		// (quitter ne la reecrit pas)
			Quitter();
			return;
		}
	}
	if (TestReprise == 2)
	{
		if (Phase == EVespPhase::Titre && TempsPhase > 25.0f && !bFinLancee) { Photographier(TEXT("Reprise_Titre"), true); bFinLancee = true; ReprendreLaVision(); }
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) { Photographier(TEXT("Reprise_Acte"), true); ContinuerApresLActe(); }
		else if (Phase == EVespPhase::Exploration && TempsPhase > 3.0f)
		{
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE test reprise : graine %d, clairiere 1 faite %d, %d clairieres"), GraineCarte, Noeuds.IsValidIndex(1) && Noeuds[1].bVisite ? 1 : 0, Noeuds.Num());
			Photographier(TEXT("Reprise_Apres"), true);
			EffacerPartie();
			Quitter();
			return;
		}
	}
	if (bTestOptions)
	{
		if (Phase == EVespPhase::Titre && bOptionsOuvertes && TempsPhase > 28.0f)
		{
			SelectionReglage = 1;
			Photographier(TEXT("Options_Titre"), true);
			bOptionsOuvertes = false;
			NouvellePartie(1);
		}
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) ContinuerApresLActe();
		else if (Phase == EVespPhase::Dialogue && TempsPhase > 0.5f) { Ecriture = 9999.0f; AvancerDialogue(); TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Exploration && !bMenuOuvert && TempsPhase > 2.0f) { OuvrirMenu(2); SelectionReglage = 4; bConfirmerAbandon = true; TempsPhase = 0.0f; }
		else if (Phase == EVespPhase::Exploration && bMenuOuvert && TempsPhase > 1.5f) { Photographier(TEXT("Options_Jeu"), true); Quitter(); return; }
	}
	if (TestFin > 0)
	{
		if (Phase == EVespPhase::Titre && TempsPhase > 25.0f) NouvellePartie(NombreDActes);
		else if (Phase == EVespPhase::NouvelActe && TempsPhase > 3.0f) ContinuerApresLActe();
		else if (Phase == EVespPhase::Exploration && !bFinLancee && TempsPhase > 3.0f)
		{
			bFinLancee = true;
			if (Memoire) Memoire->Victoires = TestFin == 2 ? 1 : 0;
			Photographier(TEXT("FinKarn"), true);
			FinDeLaRoute();
		}
		else if (Phase == EVespPhase::Dialogue && TempsPhase > 3.0f)
		{
			Photographier(FString::Printf(TEXT("Fin%d_Ligne%d"), TestFin, LigneDialogue), true);
			Ecriture = 9999.0f;
			AvancerDialogue();
			TempsPhase = 0.0f;
		}
		else if (Phase == EVespPhase::Evenement && TempsPhase > 2.0f) { Photographier(TEXT("Fin2_Choix"), true); ChoisirEvenement(0); }
		else if (Phase == EVespPhase::Victoire && TempsPhase > 2.5f) { Photographier(FString::Printf(TEXT("Fin%d_Ecran"), TestFin), true); Quitter(); return; }
	}
	if (bTestVeilleur && !bFinLancee && Phase == EVespPhase::Titre && TempsPhase > 30.0f)
	{
		bFinLancee = true;		// (le drapeau du test reste leve : rien n'est sauvegarde en quittant)
		Photographier(TEXT("Veilleur"), true);
		Quitter();
		return;
	}
	if (bModePhoto)
	{
		ModePhoto(Secondes);
		if (!bCameraPhoto)
		{
			PlacerCamera(Secondes);
		}
		return;
	}
	// La rage pleine : une aura magique enveloppe AYLIS
	const bool bRagePleine = Rage >= 100 && Phase == EVespPhase::Exploration;
	if (bRagePleine && !AuraRage)
	{
		if (UNiagaraSystem* Aura = AVespEffet::Magie(TEXT("NS_Free_Magic_Aura")))
		{
			AuraRage = UNiagaraFunctionLibrary::SpawnSystemAttached(Aura, Aylis->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			                                                        FVector(0.5f), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None);
		}
	}
	else if (!bRagePleine && AuraRage)
	{
		AuraRage->DestroyComponent();
		AuraRage = nullptr;
	}
	// F4 : les contours "toon" (le filtre du pack StylizedProvencal), en marche ou pas
	if (WasInputKeyJustPressed(EKeys::F4))
	{
		bContours = !bContours;
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			for (FWeightedBlendable& B : It->Settings.WeightedBlendables.Array)
			{
				B.Weight = bContours ? 1.0f : 0.0f;
			}
		}
	}
	Repetition = FMath::Max(0.0f, Repetition - Secondes);
	PlacerCamera(Secondes);
	const bool bValider = WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);

	switch (Phase)
	{
		case EVespPhase::Titre:
			if (bOptionsOuvertes)
			{
				if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
				{
					bOptionsOuvertes = false;
					UVespSons::Jouer2D(this, EVespSon::Clic, 0.8f, 0.8f);
				}
				else
				{
					CommandesDesReglages(Secondes, LignesReglagesTitre);
				}
				return;
			}
			// Le Veilleur : Y ouvre et ferme ; la croix choisit, E / X (manette : X) achete ou active
			if (WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
			{
				bVeilleurOuvert = !bVeilleurOuvert;
				UVespSons::Jouer2D(this, EVespSon::Clic);
			}
			if (bVeilleurOuvert)
			{
				// (la chandelle en haut, puis les cartes sur deux colonnes : haut / bas saute une ligne, gauche / droite change de colonne)
				FIntPoint Direction;
				if (DirectionPressee(Direction, Secondes))
				{
					const int32 Lignes = VespVeilleur::Nombre + 1;
					if (SelectionDon == 0)
					{
						SelectionDon = Direction.Y > 0 ? 1 : 0;
					}
					else if (Direction.Y != 0)
					{
						const int32 Suivante = SelectionDon + Direction.Y * 2;
						SelectionDon = Suivante < 1 ? 0 : FMath::Min(Suivante, Lignes - 1);
					}
					else if (Direction.X != 0)
					{
						SelectionDon = FMath::Clamp(SelectionDon + Direction.X, 1, Lignes - 1);
					}
					UVespSons::Jouer2D(this, EVespSon::Survol);
				}
				if (WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left) || WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::X))
				{
					ActionVeilleur(SelectionDon);
				}
				if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
				{
					bVeilleurOuvert = false;
				}
			}
			if (bValider || WasInputKeyJustPressed(EKeys::SpaceBar))
			{
				if (PartieSuspendue)
				{
					ReprendreLaVision();
				}
				else
				{
					NouvellePartie(1);
				}
			}
			return;
		case EVespPhase::Victoire:
		case EVespPhase::Defaite:
			if (WasInputKeyJustPressed(EKeys::R) || WasInputKeyJustPressed(EKeys::Gamepad_Special_Right) || (bValider && TempsPhase > 1.5f))
			{
				Recommencer();
			}
			return;
		case EVespPhase::Dialogue:
			Ecriture += Secondes * 45.0f;
			if (bValider || WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::LeftMouseButton))
			{
				AvancerDialogue();
			}
			return;
		case EVespPhase::NouvelActe:
			if (bValider || WasInputKeyJustPressed(EKeys::SpaceBar))
			{
				ContinuerApresLActe();
			}
			return;
		case EVespPhase::ChoixRune:
		case EVespPhase::Marchand:
		case EVespPhase::Evenement:
			CommandesDeMenu(Secondes);
			return;
		case EVespPhase::Exploration:
			if (bMenuOuvert)
			{
				CommandesDuMenu(Secondes);		// le jeu s'arrete tant que le menu est ouvert
				return;
			}
			Jouer(Secondes);
			return;
		default:
			return;
	}
}

// Une direction pressee (les menus) : les fleches, la croix, ou le stick gauche (qui se repete si on le tient)
bool AVespPlayerController::DirectionPressee(FIntPoint& Direction, float Secondes)
{
	Direction = FIntPoint(0, 0);
	auto Presse = [this](std::initializer_list<FKey> Touches) {
		for (const FKey& K : Touches)
		{
			if (WasInputKeyJustPressed(K))
			{
				return true;
			}
		}
		return false;
	};
	if (Presse({EKeys::Up, EKeys::Gamepad_DPad_Up})) Direction.Y = -1;
	else if (Presse({EKeys::Down, EKeys::Gamepad_DPad_Down})) Direction.Y = 1;
	else if (Presse({EKeys::Left, EKeys::Gamepad_DPad_Left, EKeys::Q, EKeys::A})) Direction.X = -1;
	else if (Presse({EKeys::Right, EKeys::Gamepad_DPad_Right, EKeys::D})) Direction.X = 1;
	if (Direction != FIntPoint(0, 0))
	{
		return true;
	}
	const float SX = GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const float SY = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Max(FMath::Abs(SX), FMath::Abs(SY)) < 0.55f)
	{
		Repetition = 0.0f;
		return false;
	}
	if (Repetition > 0.0f)
	{
		return false;
	}
	Repetition = 0.2f;
	if (FMath::Abs(SX) > FMath::Abs(SY))
	{
		Direction.X = SX > 0 ? 1 : -1;
	}
	else
	{
		Direction.Y = SY > 0 ? -1 : 1;
	}
	return true;
}

// ===================== Les menus (runes, marchand, evenement) au clavier ou a la manette =====================

void AVespPlayerController::CommandesDeMenu(float Secondes)
{
	const int32 Nombre = Phase == EVespPhase::ChoixRune ? OffresDons.Num() : (Phase == EVespPhase::Marchand ? Offres.Num() : 2);
	const FKey Touches[4] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four};
	auto Choisir = [this](int32 i) {
		if (Phase == EVespPhase::ChoixRune) ChoisirRune(i);
		else if (Phase == EVespPhase::Marchand) AcheterOffre(i);
		else if (i < 2) ChoisirEvenement(i);
	};
	for (int32 i = 0; i < 4; i++)
	{
		if (WasInputKeyJustPressed(Touches[i]))
		{
			Choisir(i);
			return;
		}
	}
	FIntPoint Direction;
	if (DirectionPressee(Direction, Secondes) && Direction.X != 0 && Nombre > 0)
	{
		SelectionMenu = bSelectionVisible ? (SelectionMenu + Direction.X + Nombre) % Nombre : 0;
		bSelectionVisible = true;
		UVespSons::Jouer2D(this, EVespSon::Survol);
	}
	if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		if (bSelectionVisible)
		{
			Choisir(FMath::Clamp(SelectionMenu, 0, FMath::Max(0, Nombre - 1)));
		}
		else
		{
			SelectionMenu = 0;
			bSelectionVisible = true;
		}
		return;
	}
	if (Phase == EVespPhase::Marchand && (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)))
	{
		QuitterMarchand();
	}
}

// ===================== Le mode photo (pour la promo) =====================
// Lance le jeu avec -VespPhotos : il attend que tout soit pret (shaders, modeles), puis, acte par acte, il prend
// 4 photos (l'exploration, un panorama, un combat au moment d'un coup, le boss) dans Saved/Photos, et quitte.

void AVespPlayerController::Photographier(const FString& Nom, bool bAvecInterface)
{
	const FString Fichier = FPaths::ProjectSavedDir() / TEXT("Photos") / (Nom + TEXT(".png"));
	if (!bAvecInterface)
	{
		FScreenshotRequest::RequestScreenshot(Fichier, false, false);		// l'image du jeu seule
		return;
	}
	// Avec l'interface : on photographie toute la fenetre (le jeu et l'interface Slate)
	TSharedPtr<SWindow> Fenetre = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
	TArray<FColor> Pixels;
	FIntVector Taille;
	if (Fenetre.IsValid() && FSlateApplication::Get().TakeScreenshot(Fenetre.ToSharedRef(), Pixels, Taille) && Pixels.Num() > 0)
	{
		for (FColor& P : Pixels)
		{
			P.A = 255;
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Taille.X, Taille.Y, Pixels, Png);
		FFileHelper::SaveArrayToFile(Png, *Fichier);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VESPERANCE photo ratee : %s"), *Fichier);
	}
}

void AVespPlayerController::ModePhoto(float Secondes)
{
	// Les ecrans de choix et les dialogues se passent tout seuls (les photos veulent le jeu)
	if (Phase == EVespPhase::ChoixRune)
	{
		ChoisirRune(0);
	}
	if (Phase == EVespPhase::Dialogue)
	{
		Phase = EVespPhase::Exploration;
	}
	// Le jeu continue pendant les photos (les Haschen bougent, les coups partent)
	if (Phase == EVespPhase::Exploration)
	{
		Combat->Avancer(Secondes);
		AvancerGeste(Secondes);
	}
	PhotoAttente -= Secondes;
	if (PhotoAttente > 0.0f)
	{
		return;
	}
	// On attend que les shaders et les modeles soient prets (la premiere fois, ca peut etre long)
#if WITH_EDITOR
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		UE_LOG(LogTemp, Display, TEXT("VESPERANCE photo : les shaders compilent encore (%d)"), GShaderCompilingManager->GetNumRemainingJobs());
		PhotoAttente = 1.0f;
		return;
	}
#endif
	// (on attend les modeles qui se preparent, mais pas plus de 90 secondes d'affilee : un pack lourd peut bloquer longtemps)
	static float AttenteAssets = 0.0f;
	if (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0 && AttenteAssets < 90.0f)
	{
		UE_LOG(LogTemp, Display, TEXT("VESPERANCE photo : des assets se preparent encore (%d)"), FAssetCompilingManager::Get().GetNumRemainingAssets());
		AttenteAssets += 1.0f;
		PhotoAttente = 1.0f;
		return;
	}
	AttenteAssets = 0.0f;
	UE_LOG(LogTemp, Display, TEXT("VESPERANCE photo : acte %d, etape %d (Haschen engages : %d, combat : %d)"), PhotoActe, PhotoEtape,
	       Combat->HaschenEngages(), Combat->EnCombat() ? 1 : 0);
	const FString Prefixe = FString::Printf(TEXT("Acte%d_%s"), PhotoActe, *Romain(FMath::Max(1, PhotoActe)));
	auto PremiereZone = [this](std::initializer_list<EVespSalle> Types) {
		for (int32 i = 0; i < Noeuds.Num(); i++)
		{
			for (EVespSalle T : Types)
			{
				if (Noeuds[i].Type == T)
				{
					return i;
				}
			}
		}
		return 1;
	};
	switch (PhotoEtape)
	{
		case 0:		// l'ecran titre
			if (PhotoActe == 0)
			{
				static bool bPrepare = false;
				if (!bPrepare)
				{
					bPrepare = true;
					ConsoleCommand(TEXT("r.SetRes 1920x1080w"));
					for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
					{
						It->Settings.AutoExposureBias += 1.4f;
					}
					PhotoAttente = 4.0f;
					return;
				}
				Photographier(TEXT("00_Ecran_titre"), true);
				// -VespActe=3 : seulement cet acte (pour verifier vite)
				PhotoActe = 1;
				FParse::Value(FCommandLine::Get(), TEXT("VespActe="), PhotoActe);
				PhotoActe = FMath::Clamp(PhotoActe, 1, NombreDActes);
				PhotoAttente = 1.5f;
				return;
			}
			Acte = PhotoActe;
			Aylis->Stats.Pv = Aylis->Stats.PvMax = 999;
			AmbianceDeLActe();
			Phase = EVespPhase::Exploration;
			bCameraPhoto = false;
			PhotoEtape = 1;
			PhotoAttente = 8.0f;
			return;
		case 1:		// AYLIS sur le sentier : un plan rapproche, de trois quarts
		{
			const FVector P = FMath::Lerp(Noeuds[1].Centre, Noeuds[2].Centre, 0.45f);
			Aylis->Teleporter(Monde->Contraindre(P, P));
			Aylis->SetActorRotation(FRotator(0, -125.0f, 0));		// de trois quarts, face a la camera
			const FVector Vise = Aylis->GetActorLocation() + FVector(0, 0, 110.0f);
			const FVector Position = Vise + FVector(-520.0f, -330.0f, 260.0f);
			bCameraPhoto = true;
			CameraArene->SetActorLocation(Position);
			CameraArene->SetActorRotation((Vise + FVector(250.0f, 250.0f, 60.0f) - Position).Rotation());
			CameraArene->GetCameraComponent()->SetFieldOfView(50.0f);
			PhotoEtape = 2;
			PhotoAttente = 3.0f;
			return;
		}
		case 2:
			Photographier(Prefixe + TEXT("_1_Exploration"), false);
			PhotoEtape = 3;
			PhotoAttente = 1.0f;
			return;
		case 3:		// un panorama : en hauteur, dans l'axe du sentier
		{
			const FVector Cible = FMath::Lerp(Noeuds[2].Centre, Noeuds[3].Centre, 0.35f) + FVector(0, 0, 180.0f);
			const FVector Position = Cible + FVector(-700.0f, -2900.0f, 1500.0f);
			bCameraPhoto = true;
			CameraArene->SetActorLocation(Position);
			CameraArene->SetActorRotation((Cible - Position).Rotation());
			CameraArene->GetCameraComponent()->SetFieldOfView(55.0f);
			PhotoEtape = 4;
			PhotoAttente = 3.0f;
			return;
		}
		case 4:
			Photographier(Prefixe + TEXT("_2_Panorama"), false);
			PhotoEtape = 5;
			PhotoAttente = 1.0f;
			return;
		case 5:		// un combat : AYLIS entre dans une clairiere gardee, les Haschen surgissent
		{
			bCameraPhoto = false;
			const int32 Zone = PremiereZone({EVespSalle::Combat, EVespSalle::Elite});
			Aylis->Teleporter(Noeuds[Zone].Centre + FVector(0, -300.0f, 0));
			bCaleCamera = true;
			Rage = 100;
			Aylis->Stats.Attaque = 3;		// (les Haschen doivent tenir jusqu'a la photo)
			PhotoEtape = 6;
			PhotoAttente = 3.5f;
			return;
		}
		case 6:		// AYLIS frappe : la photo au moment de l'impact
		{
			Regard = Aylis->Avant();
			if (AVespUnite* Cible = Combat->CibleProche(Aylis->GetActorLocation(), FVector(0, 1, 0), 2000.0f, 180.0f))
			{
				Regard = (Cible->GetActorLocation() - Aylis->GetActorLocation()).GetSafeNormal2D();
			}
			Attaquer(true);		// l'attaque lourde : l'eclat de lumiere, sans tout balayer
			PhotoEtape = 7;
			PhotoAttente = MomentImpact + 0.05f;
			return;
		}
		case 7:
			Photographier(Prefixe + TEXT("_3_Combat"), true);
			PhotoEtape = 8;
			PhotoAttente = 1.5f;
			return;
		case 8:		// le boss, dans sa clairiere
		{
			const int32 Zone = PremiereZone({EVespSalle::Boss});
			Combat->LeverLaBarriere();		// (la clairiere d'avant retiendrait AYLIS)
			Aylis->Teleporter(Noeuds[Zone].Centre + FVector(0, -500.0f, 0));
			bCaleCamera = true;
			PhotoEtape = 9;
			PhotoAttente = 1.0f;
			return;
		}
		case 9:		// (le dialogue du boss : on le passe)
			if (Phase == EVespPhase::Dialogue)
			{
				Phase = EVespPhase::Exploration;
			}
			PhotoEtape = 10;
			PhotoAttente = 4.0f;
			return;
		case 10:
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE photo : AYLIS %s (pv %d), camera %s, boss %s"), *Aylis->GetActorLocation().ToString(), Aylis->Stats.Pv,
			       *CameraArene->GetActorLocation().ToString(), Combat->BossActif() ? *Combat->BossActif()->GetActorLocation().ToString() : TEXT("aucun"));
			Photographier(Prefixe + TEXT("_4_Boss"), true);
			PhotoEtape = PhotoActe == 1 ? 11 : 30;
			PhotoAttente = 1.0f;
			return;
		case 11:	// (acte I) le menu d'AYLIS : un sac bien rempli, quelques etoiles allumees
		{
			for (int32 i = 0; i < 9; i++)
			{
				Sac.Add(VespButin::Tirer(1 + i / 2, i % 3, HasardButin));
			}
			PointsDeCompetence = 4;
			DebloquerEtoile(VespSeuil::Index(TEXT("fendant")));
			DebloquerEtoile(VespSeuil::Index(TEXT("garde")));
			DebloquerEtoile(VespSeuil::Index(TEXT("ecorce")));
			SelectionSac = 2;
			SelectionEtoile = VespSeuil::Index(TEXT("egide"));
			OuvrirMenu(0);
			PhotoEtape = 12;
			PhotoAttente = 1.5f;
			return;
		}
		case 12:
			Photographier(Prefixe + TEXT("_5_Inventaire"), true);
			OngletMenu = 1;
			PhotoEtape = 13;
			PhotoAttente = 1.5f;
			return;
		case 13:
			Photographier(Prefixe + TEXT("_6_Seuil"), true);
			FermerMenu();
			PhotoEtape = 14;
			PhotoAttente = 1.0f;
			return;
		case 14:	// (acte I) l'armurerie : AYLIS de pres, avec chaque famille d'arme et une armure d'une autre couleur
		case 16:
		case 18:
		case 20:
		{
			const int32 k = (PhotoEtape - 14) / 2;
			const EVespArme Voulue[] = {EVespArme::Epee, EVespArme::Dagues, EVespArme::DeuxMains, EVespArme::Baton};
			FVespObjet Arme = VespButin::Tirer(3, 2, EVespEmplacement::Arme, HasardButin);
			for (int32 Essai = 0; Essai < 400 && Arme.TypeArme != Voulue[k]; Essai++)
			{
				Arme = VespButin::Tirer(3, 2, EVespEmplacement::Arme, HasardButin);
			}
			auto Mettre = [this](const FVespObjet& O) { Sac.Reset(); Sac.Add(O); EquiperDuSac(0); };
			Mettre(Arme);
			Mettre(VespButin::Tirer(3, k, EVespEmplacement::Corps, HasardButin));
			if (k == 0)
			{
				Mettre(VespButin::Tirer(3, 2, EVespEmplacement::MainGauche, HasardButin));
			}
			Sac.Reset();
			Combat->LeverLaBarriere();		// (la barriere du boss retiendrait AYLIS)
			const FVector P = Noeuds[0].Centre;
			Aylis->Teleporter(Monde->Contraindre(P, P));
			Aylis->SetActorRotation(FRotator(0, -125.0f, 0));
			const FVector Vise = Aylis->GetActorLocation() + FVector(0, 0, 80.0f);
			const FVector Position = Vise + FVector(-440.0f, -280.0f, 180.0f);
			bCameraPhoto = true;
			CameraArene->SetActorLocation(Position);
			CameraArene->SetActorRotation((Vise - Position).Rotation());
			CameraArene->GetCameraComponent()->SetFieldOfView(40.0f);
			UE_LOG(LogTemp, Display, TEXT("VESPERANCE photo : arme %s (type %d), armure %s"), *Arme.Nom, (int32)Arme.TypeArme, *Equipement[(int32)EVespEmplacement::Corps].Nom);
			PhotoEtape++;
			PhotoAttente = 2.0f;
			return;
		}
		case 15:
		case 17:
		case 19:
		case 21:
		{
			const TCHAR* Noms[] = {TEXT("Epee_Bouclier"), TEXT("Dagues"), TEXT("Deux_Mains"), TEXT("Baton")};
			Photographier(Prefixe + FString::Printf(TEXT("_7_Arme_%s"), Noms[(PhotoEtape - 15) / 2]), false);
			PhotoEtape = PhotoEtape == 21 ? 30 : PhotoEtape + 1;
			PhotoAttente = 1.0f;
			return;
		}
		default:	// l'acte suivant, ou la fin
			PhotoActe++;
			PhotoEtape = 0;
			PhotoAttente = 1.0f;
			Phase = EVespPhase::Exploration;
			int32 Seul = 0;
			if (PhotoActe > NombreDActes || FParse::Value(FCommandLine::Get(), TEXT("VespActe="), Seul))
			{
				bModePhoto = false;
				Quitter();
			}
			return;
	}
}
