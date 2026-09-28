// VespPlayerController : le deroulement d'une partie, de l'ecran titre au dernier boss, et AYLIS au bout des doigts.
//
//   7 actes, chacun avec son monde, ses Haschen, sa regle et son boss :
//     I   Les Terres Brumeuses  - la Foret des Brumes      - Skarn le Brise-Cranes (sa masse brise la terre)
//     II  Les Terres Hantees    - le Bois des Pendus        - la Matriarche (poison, loups, decoctions)
//     III Les Marais Noyes      - les Marais de Sombreval   - le Roi Noye (les eaux toxiques montent)
//     IV  La Marche d'Ashka     - la forteresse d'Ashka     - le Gardien de Pierre (armure, ecrasements)
//     V   Le Col d'Ashka        - le col gele               - Ashka (fleches, pluies de fleches, blizzard)
//     VI  Les Terres de Cendre  - la faille ardente         - Vorgath le Destructeur (charges, eruptions)
//     VII Karn                  - la cite voilee            - l'Oracle de Karn (tout ce qu'AYLIS a affronte)
//
//   Chaque acte est un monde d'un seul tenant (VespMonde), qu'AYLIS parcourt librement, au clavier ou a la manette.
//   Tout se joue en temps reel (VespCombat) : les Haschen attendent dans les clairieres gardees, une barriere
//   se leve quand AYLIS y entre, et il faut les vaincre pour passer. Le seul chargement : entre deux actes.
//
//   Les commandes (clavier / manette) :
//     ZQSD, WASD, fleches / stick gauche : marcher        la souris / stick droit : viser
//     clic gauche, J / X : attaquer (trois coups enchaines)   clic droit, K / Y : attaque lourde (brise les armures)
//     ESPACE / A : esquiver (on traverse les coups)       MAJ, F / gachette gauche (maintenue) : la garde au bouclier
//     R / croix haut : boire une potion                   V / RB : l'attaque speciale (rage pleine)
//     E / B : parler, ouvrir, prendre                      TAB / Select : la carte
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VespUnite.h"
#include "VespProgression.h"
#include "VespCombat.h"
#include "VespPlayerController.generated.h"

class AVespMonde;
class AVespCombat;
class ACameraActor;

UENUM()
enum class EVespPhase : uint8
{
	Titre,			// l'ecran titre
	Dialogue,		// quelqu'un parle (le parchemin)
	ChoixRune,		// apres une victoire : une rune parmi 3
	Exploration,	// le jeu : AYLIS dans le monde de l'acte (on s'y bat aussi)
	Marchand,
	Evenement,
	NouvelActe,		// le titre d'un nouvel acte
	Victoire,		// le dernier boss est tombe
	Defaite,		// AYLIS tombe : la vision se brise
};

// Ce qu'AYLIS est en train de faire
UENUM()
enum class EVespGesteAylis : uint8
{
	Libre,
	Attaque,		// un coup de l'enchainement
	Lourde,
	Esquive,
	Potion,
	Speciale,
	Touchee,		// elle vient d'encaisser un coup : un instant sans agir
};

UENUM()
enum class EVespSalle : uint8
{
	Combat,
	Elite,
	Repos,
	Marchand,
	Evenement,
	Boss,
	Depart,			// la clairiere ou commence l'acte
	Tresor,			// au bout d'un embranchement : un coffre
};

// Une salle sur la carte de la route
struct FVespNoeud
{
	EVespSalle Type = EVespSalle::Combat;
	int32 Etage = 0;
	FVector Centre = FVector::ZeroVector;	// sa place dans le monde
	TArray<int32> Suivants;			// les clairieres reliees par un sentier
	bool bVisite = false;			// deja faite (combat gagne, coffre ouvert...)
	float Rayon = 1000.0f;
};

// Un objet du marchand
struct FVespOffre
{
	FString Nom;
	FString Aide;
	int32 Prix = 0;
	int32 Genre = 0;				// 0 potion, 1 rune, 2 soin, 3 attaque, 4 pv max, 5 defense
	int32 Rune = 0;					// pour une rune : laquelle
	bool bVendue = false;
};

UCLASS()
class VESPERANCE_API AVespPlayerController : public APlayerController
{
	GENERATED_BODY()
	friend class AVespHUD;
	friend class SVespInterface;		// l'interface lit l'etat du jeu
	friend class SVespCarteRoute;		// la carte de la route (dans l'interface)
	friend class SVespMenu;				// l'inventaire et le Seuil
	friend class SVespConstellation;
	friend class SVespOptions;				// le panneau des options

public:
	AVespPlayerController();
	virtual void PlayerTick(float Secondes) override;
	virtual void EndPlay(const EEndPlayReason::Type Raison) override;

	void Commencer(AVespMonde* LeMonde, AVespCombat* LeCombat, AVespUnite* LAylis, ACameraActor* LaCamera);

	// Ce que l'interface peut demander
	void NouvellePartie(int32 ActeDeDepart = 1);
	void Quitter();
	void ChoisirRune(int32 Numero);
	void AcheterOffre(int32 Numero);
	void QuitterMarchand();
	void ChoisirEvenement(int32 Choix);
	void AvancerDialogue();
	void ContinuerApresLActe();
	void Recommencer();
	void ReprendreLaVision();				// l'ecran titre : la vision mise de cote reprend ou elle en etait
	void AcheterDon(int32 Index);			// le Veilleur (ecran titre) : un rang de plus, contre des Souvenirs

	static FString NomSalle(EVespSalle Salle, int32 Acte);
	static FString AideSalle(EVespSalle Salle);
	static FString LettreSalle(EVespSalle Salle);
	static FLinearColor CouleurSalle(EVespSalle Salle);
	static FString NomRune(int32 Rune);
	static FString AideRune(int32 Rune);
	static FString Romain(int32 Nombre);
	FString NomDuLieu() const;
	FString NomDeLActe() const;
	FString RegleDeLActe() const;
	FString NomDuBoss() const;
	FString TitreEvenement() const;
	FString TexteEvenement() const;
	FString ChoixEvenement(int32 Choix) const;
	FString AideEvenement(int32 Choix) const;
	FString Invite() const;					// ce qu'on peut faire ici (parler au marchand...)
	int32 XpPourNiveau(int32 N) const { return 60 + 45 * (N - 1); }

	// Le menu d'AYLIS (I / Start) : l'inventaire et le Seuil. Le jeu s'arrete tant qu'il est ouvert.
	void OuvrirMenu(int32 Onglet);
	void FermerMenu();
	bool EtoileAcquise(int32 Index) const { return Seuil.IsValidIndex(Index) && Seuil[Index]; }
	bool EtoilePossible(int32 Index) const;			// assez de points, et l'etoile d'avant est allumee
	void DebloquerEtoile(int32 Index);
	void EquiperDuSac(int32 IndexSac);
	void Retirer(EVespEmplacement E);
	void JeterDuSac(int32 IndexSac);
	const FVespObjet* Equipe(EVespEmplacement E) const { return bEquipe[(int32)E] ? &Equipement[(int32)E] : nullptr; }
	float RechargeDuPouvoir(int32 N) const;			// de 0 (pret) a 1 (vient d'etre lance)
	int32 EtoileDuPouvoir(int32 N) const;			// l'etoile qui donne ce pouvoir (ou -1)
	static constexpr int32 TailleDuSac = 24;

	static constexpr int32 NombreDActes = 7;

	// Les options (VespReglages) : musique, effets, plein ecran, secousses ; en jeu : abandonner la vision, quitter
	static constexpr int32 LignesReglages = 6;
	static constexpr int32 LignesReglagesTitre = 4;
	void ChangerReglage(int32 Ligne, int32 Sens);		// Sens : -1 / +1 pour regler, 0 pour valider (la souris, ENTREE)
	FString ValeurReglage(int32 Ligne) const;
	static FString NomReglage(int32 Ligne);
	int32 SelectionReglage = 0;
	bool bOptionsOuvertes = false;						// l'ecran titre montre les options

private:
	// Le monde de l'acte
	void GenererMonde();
	void Declencher(int32 Zone);				// AYLIS entre dans une clairiere sans Haschen (repos, tresor, marchand...)
	void RetourExploration();
	void Jouer(float Secondes);					// le jeu : marcher, viser, frapper, esquiver...
	void RouvrirMarchand(int32 Zone);
	bool DirectionPressee(FIntPoint& Direction, float Secondes);	// les menus : fleches, croix ou stick (avec repetition)
	void CommandesDeMenu(float Secondes);
	void ProposerRunes();
	void AppliquerRune(int32 Rune);
	int32 RuneAuHasard() const;
	bool RunePossible(int32 Rune) const;
	void OuvrirMarchand();
	void OuvrirEvenement();
	void DialogueDuBoss();
	void AmbianceDeLActe();

	// AYLIS
	FVector DirectionVisee() const;				// la souris, le stick droit, ou la marche
	void Attaquer(bool bLourde);
	void Esquiver(const FVector& Direction);
	void BoirePotion();
	void AttaqueSpeciale();
	void AvancerGeste(float Secondes);
	void GagnerXp(int32 Quantite);
	void GagnerEclats(int32 Quantite, const FVector& Ou);
	void Pouvoir(int32 N);						// les pouvoirs du Seuil (1, 2, 3)
	void LacherButin(const FVector& Ou, int32 Chance);
	void RamasserButin();
	void AppliquerObjet(const FVespObjet& O, int32 Signe);	// ajoute (+1) ou retire (-1) ce qu'un objet donne
	void HabillerAylis();						// l'arme, le bouclier et la tenue se voient
	void CommandesDuMenu(float Secondes);
	FVespCoup CoupDeBase() const;				// ce que tous les coups d'AYLIS ont en commun (effets de l'arme et des runes)
	EVespArme ArmeEnMain() const { return bEquipe[(int32)EVespEmplacement::Arme] ? Equipement[(int32)EVespEmplacement::Arme].TypeArme : EVespArme::Poings; }
	FVector Deplacement = FVector::ZeroVector;	// l'envie de marcher (clavier ou stick), dans le monde

	// Ce que le combat annonce
	void QuandHaschenTombe(AVespUnite* H, int32 Categorie);
	bool Talent(const TCHAR* Id) const { return EtoileAcquise(VespSeuil::Index(Id)); }
	int32 PouvoirEnCours = 0;					// l'attaque speciale (0) ou le tourbillon du Seuil (1)
	void QuandAylisTouchee(int32 Degats, bool bCritique, AVespUnite* Source);
	void QuandClairiereFermee(int32 Zone);
	void QuandClairiereLiberee(int32 Zone);
	void Ecrire(const FString& Message, float Duree = 4.0f);

	void Trembler(float Force) { if (bSecoussesActives) Secousse = FMath::Min(1.5f, Secousse + Force); }
	void Ralenti(float Echelle, float Duree);		// un instant de ralenti (un coup fatal, un critique)
	void PlacerCamera(float Secondes);
	// Le mode photo (le jeu lance avec -VespPhotos) : il parcourt les 7 actes et prend des photos pour la promo
	void ModePhoto(float Secondes);
	void Photographier(const FString& Nom, bool bAvecInterface);

	UPROPERTY() TObjectPtr<AVespMonde> Monde;
	UPROPERTY() TObjectPtr<AVespCombat> Combat;
	UPROPERTY() TObjectPtr<class AVespMeteo> Meteo;
	UPROPERTY() TObjectPtr<class UNiagaraComponent> AuraRage;	// l'aura d'AYLIS quand la rage est pleine
	bool bContours = true;										// les contours "toon" (F4)
	bool bModePhoto = false;
	int32 PhotoActe = 0;			// 0 : l'ecran titre
	int32 PhotoEtape = 0;
	float PhotoAttente = 0.0f;
	bool bCameraPhoto = false;		// la camera est placee a la main (le panorama)
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TObjectPtr<ACameraActor> CameraArene;
	TSharedPtr<class SVespInterface> Interface;
	FVector PositionCamera;
	FRotator RotationCamera;
	FVector DecalageCamera;			// la camera par rapport a ce qu'elle regarde (toujours le meme angle)
	FVector CameraActuelle;			// ou elle est (elle glisse doucement vers ou elle doit etre)
	bool bCaleCamera = true;		// la prochaine image : la camera se place d'un coup (debut d'un acte)
	float Secousse = 0.0f;
	float Zoom = 0.0f;				// > 0 : la camera se resserre un instant (un critique)
	float Recul = 0.0f;				// la camera recule un peu pendant les combats
	double FinDuRalenti = 0.0;

	// AYLIS
	EVespGesteAylis Geste = EVespGesteAylis::Libre;
	float TempsGeste = 0.0f;
	float DureeGeste = 0.0f;
	float MomentImpact = 0.0f;		// quand le coup en cours touche (en secondes depuis son debut)
	bool bImpactFait = false;
	int32 Combo = 0;				// le coup de l'enchainement (0, 1, 2)
	bool bCoupSuivant = false;		// on a appuye pendant le coup : le suivant s'enchaine
	float FinCombo = 0.0f;			// apres ce delai sans frapper, l'enchainement recommence au premier coup
	FVector DirectionGeste = FVector::ForwardVector;
	float ElanGeste = 0.0f;			// la distance dont AYLIS se fend vers sa cible
	float RechargeEsquive = 0.0f;
	bool bGardeLevee = false;
	float TempsGarde = 0.0f;
	bool bPotionBue = false;
	FVector2D DerniereSouris = FVector2D::ZeroVector;
	float TempsSouris = 0.0f;		// la souris a bouge il y a peu : on vise avec elle
	FVector Regard = FVector::ForwardVector;

	// La partie
	EVespPhase Phase = EVespPhase::Titre;
	int32 Potions = 3;
	int32 Rage = 0;					// de 0 a 100 : elle monte avec les coups donnes et recus
	int32 Eclats = 0;				// la monnaie du marchand
	int32 Niveau = 1;
	int32 Xp = 0;
	int32 PointsDeCompetence = 0;
	float TempsNiveau = 0.0f;		// > 0 : le bandeau "NIVEAU" est affiche
	int32 HaschenVaincus = 0;

	// Le Seuil, les pouvoirs
	TArray<bool> Seuil;
	float Recharges[4] = {0, 0, 0, 0};		// les pouvoirs 1, 2, 3 (secondes restantes)
	float TempsEgide = 0.0f;
	bool bRiposte = false;					// la prochaine attaque apres une parade parfaite
	float RechargePresage = 0.0f;
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> BulleEgide;

	// L'inventaire
	TArray<FVespObjet> Sac;
	FVespObjet Equipement[(int32)EVespEmplacement::Nombre];
	bool bEquipe[(int32)EVespEmplacement::Nombre] = {false, false, false, false, false, false};
	FRandomStream HasardButin;
	UPROPERTY() TArray<TObjectPtr<class AVespButin>> ButinsAuSol;
	FString DernierButin;					// le dernier objet ramasse (un bandeau a droite)
	FLinearColor CouleurDernierButin = FLinearColor::White;
	float TempsButin = 0.0f;
	bool bMenuOuvert = false;
	int32 OngletMenu = 0;					// 0 : l'inventaire, 1 : le Seuil
	int32 SelectionSac = 0;
	int32 SelectionEtoile = 0;
	int32 VueFiche = 0;						// change a chaque nouvel objet regarde (la fiche glisse)

	// La route et les runes
	int32 Acte = 1;
	TArray<FVespNoeud> Noeuds;		// les clairieres du monde de l'acte
	int32 ZoneActuelle = -1;		// la clairiere ou se passe ce qui se passe
	int32 MarchandProche = -1;		// un marchand a portee de voix
	int32 MarchandZone = -1;		// le marchand dont on a les offres
	bool bCarteOuverte = false;		// la carte du monde (TAB)
	float TempsMessage = 0.0f;		// le message s'affiche encore quelques secondes
	// Les menus au clavier et a la manette
	float Repetition = 0.0f;				// une direction tenue se repete
	int32 SelectionMenu = 0;				// la carte choisie au clavier ou a la manette
	bool bSelectionVisible = false;
	TArray<int32> RunesProposees;
	TArray<int32> Runes;
	TArray<FVespOffre> Offres;		// le marchand
	int32 EvenementActuel = 0;
	bool bFlamme = false;			// les coups d'AYLIS peuvent bruler
	bool bSeve = false;				// +5 pv a chaque Haschen abattu
	bool bFureur = false;			// la rage monte 2 fois plus vite
	bool bSangsue = false;			// +1 pv a chaque coup porte
	bool bFortune = false;			// +50% d'eclats
	bool bGivre = false;			// les coups d'AYLIS peuvent geler
	bool bPiedSur = false;			// les pieges et les flaques n'atteignent plus AYLIS
	float BonusVitesse = 0.0f;		// la rune du Vent
	FString MessageRoute;			// ce qui vient de se passer
	float TempsPhase = 0.0f;		// depuis combien de temps l'ecran en cours est affiche (pour les fondus)

	// La memoire de la boucle (VespSauvegarde) et le chronometre
	UPROPERTY() TObjectPtr<class UVespSauvegarde> Memoire;
	int32 NumeroVision = 1;			// la vision en cours
	float ChronoActe = 0.0f;		// secondes de jeu dans l'acte en cours
	float ChronoPartie = 0.0f;		// secondes de jeu depuis le debut de la vision
	TArray<float> TempsDesActes;	// les actes termines de cette vision
	void ChargerMemoire();
	void EcrireMemoire();
	void FinirLActe();				// note le temps de l'acte (boss vaincu)
	void MemoriserLaChute();		// AYLIS vient de tomber
	bool ChronoEnMarche() const;

	// Le Veilleur : les Souvenirs gagnes en route (ils survivent a la mort) et les dons achetes entre deux visions
	int32 SouvenirsDeLaVision = 0;		// gagnes pendant cette vision (deja ranges dans la memoire)
	double MomentSouvenirs = 0.0;		// le dernier gain (la pastille brille un instant)
	bool bSouvenirsPossibles = true;	// une vision commencee plus loin (pour tester) n'en rapporte pas
	bool bSecondSouffle = false;		// le don "Second souffle", pas encore utilise dans cette vision
	bool bVeilleurOuvert = false;		// l'ecran titre montre le Veilleur
	UPROPERTY() TObjectPtr<class UVespReglages> Reglages;
	bool bSecoussesActives = true;
	bool bConfirmerAbandon = false;		// "abandonner la vision" : une seconde fois pour confirmer
	bool bTestOptions = false;			// -VespOptions : photographie les options (titre puis en jeu), puis on quitte

	// La vision en cours, mise de cote pour la reprendre (VespPartie)
	UPROPERTY() TObjectPtr<class UVespPartie> PartieSuspendue;	// sur l'ecran titre : la vision a reprendre (ou rien)
	bool bVisionEnCours = false;		// une vision est lancee (et pas encore tombee, gagnee ou abandonnee)
	bool bReprise = false;				// l'ecran de l'acte qui s'affiche vient d'une reprise
	int32 GraineCarte = 0;				// la carte de l'acte en cours (clairieres, sentiers)
	int32 GraineCarteImposee = 0;		// a la reprise : la carte a refaire a l'identique
	FRandomStream HasardCarte;
	int32 TestReprise = 0;				// -VespReprise=1 (met une vision de cote) puis =2 (la reprend) ; un emplacement a part
	const TCHAR* EmplacementPartie() const { return TestReprise > 0 ? TEXT("VesperancePartieTest") : TEXT("VesperancePartie"); }
	void SauverPartie();
	void EffacerPartie();
	void ChargerReglages();
	void AppliquerReglages();
	void CommandesDesReglages(float Secondes, int32 Nombre);
	bool bTestOuverture = false;			// -VespOuverture : lance une vision, photographie son ouverture, puis on quitte (sans rien sauvegarder)
	int32 TestFin = 0;						// -VespFin=1 ou 2 : deroule cette fin a Karn, la photographie, puis on quitte (sans rien sauvegarder)
	bool bFinLancee = false;
	bool bTestVeilleur = false;			// -VespVeilleur : une photo du Veilleur, puis on quitte
	int32 SelectionDon = 0;				// le don choisi a la manette
	void GagnerSouvenirs(int32 Quantite, const TCHAR* Pourquoi);
	void AppliquerDons();				// au depart d'une vision
	int32 RangDon(int32 Index) const;

	// Les dialogues qui se souviennent (le document narratif) : l'ouverture d'une vision, le Veilleur au feu de camp,
	// les gardiens qui savent s'ils ont deja tue AYLIS ou deja ete vaincus, la ligne de la chute
	TFunction<void()> ApresDialogue;		// ce qui se passe quand le dialogue est fini (le soin du feu de camp...)
	bool bOuvertureAFaire = false;			// la premiere ligne de la vision, en entrant dans le premier acte
	TSet<FString> DejaDitDansLaVision;		// ce qui a deja ete dit dans cette vision (pas deux fois la meme chose)
	void Dire(const TArray<FString>& Qui, const TArray<FString>& Quoi, TFunction<void()> Suite = nullptr);
	void OuvertureDeLaVision();
	void ParlerAuVeilleur();
	FString LigneDeChute() const;			// "Vision 8 : tombee dans les Marais Noyes, sous la maree du Roi Noye."

	// Ce qu'AYLIS dit en route : une ligne sous le jeu, quelques secondes, jamais plus d'une par minute
	FString ParoleAylis;
	float TempsParole = 0.0f;				// > 0 : la ligne s'affiche
	float ProchaineParole = 0.0f;			// > 0 : AYLIS se tait encore
	bool bPvBasDit = false;					// "pas ici, pas encore" : une fois, jusqu'a ce qu'AYLIS reprenne des forces
	void ParlerEnRoute(const TCHAR* const* Lignes, int32 Nombre, bool bForcer = false);

	// Les fins : 1, la route a un nouvel Oracle (AYLIS refuse et garde la porte) ; 2, le Voile se referme (la vraie fin)
	int32 FinObtenue = 0;
	void FinDeLaRoute();					// l'Oracle vient de tomber
	void TerminerLaVision(int32 Fin);
	FString TexteDeLaFin() const;

	// Le dialogue
	TArray<FString> Orateurs;
	TArray<FString> Repliques;
	int32 LigneDialogue = 0;
	float Ecriture = 0.0f;			// le nombre de lettres deja ecrites
	bool bDialogueBoss = false;		// a la fin du dialogue, le boss attaque
};
