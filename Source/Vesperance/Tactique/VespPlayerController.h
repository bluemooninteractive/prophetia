// VespPlayerController : le deroulement d'une partie, de l'ecran titre au dernier boss.
//
//   7 actes, chacun avec son monde, ses Haschen, sa regle et son boss :
//     I   Les Terres Brumeuses  - la Foret des Brumes      - Skarn le Brise-Cranes (coups de masse annonces)
//     II  Les Terres Hantees    - le Bois des Pendus        - la Matriarche (malefice, loups, decoctions)
//     III Les Marais Noyes      - les Marais de Sombreval   - le Roi Noye (la maree toxique monte)
//     IV  La Marche d'Ashka     - la forteresse d'Ashka     - le Gardien de Pierre (armure, ecrasement)
//     V   Le Col d'Ashka        - le col gele               - Ashka (pluie de fleches, archers)
//     VI  Les Terres de Cendre  - la faille ardente         - Vorgath le Destructeur (eruptions, deuxieme phase)
//     VII Karn                  - la cite voilee            - l'Oracle de Karn (tout ce qu'AYLIS a affronte)
//
//   Chaque acte est une carte de route de 14 etages. Les chemins se separent et se rejoignent ; des raccourcis
//   (dangereux) sautent plusieurs etages. En ligne droite, un acte dure environ 20 minutes ; en prenant les
//   detours, 40 minutes et plus.
//   Apres chaque victoire : des eclats (la monnaie du marchand) et une rune a choisir parmi 3.
//   L'interface (VespInterface) appelle les fonctions publiques : choisir une action, une rune, une salle...
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VespUnite.h"
#include "VespPlayerController.generated.h"

class AVespGrille;
class ACameraActor;
struct FVespModeleHaschen;

UENUM()
enum class EVespPhase : uint8
{
	Titre,			// l'ecran titre
	TourAylis,
	TourHaschen,
	Dialogue,		// quelqu'un parle (le parchemin)
	ChoixRune,		// apres une victoire : une rune parmi 3
	ChoixSalle,		// la carte de la route : ou aller ensuite
	Marchand,
	Evenement,
	NouvelActe,		// le titre d'un nouvel acte
	Victoire,		// le dernier boss est tombe
	Defaite,		// AYLIS tombe : la vision se brise
};

// Les actions d'AYLIS (les cartes du bas de l'ecran), comme dans le prototype
UENUM()
enum class EVespAction : uint8
{
	Attaque,
	Lourde,		// degats x1.8, mais rate 4 fois sur 10 (et fissure les armures)
	Garde,		// petits degats, et les coups recus font 2 fois moins mal jusqu'au prochain tour
	Potion,		// +15 pv
	Speciale,	// quand la rage est pleine : degats x2.2
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
};

// Une salle sur la carte de la route
struct FVespNoeud
{
	EVespSalle Type = EVespSalle::Combat;
	int32 Etage = 0;
	float Hauteur = 0.5f;			// sa place sur la carte, de 0 (en haut) a 1 (en bas)
	TArray<int32> Suivants;			// les salles ou l'on peut aller ensuite (un raccourci saute des etages)
	bool bVisite = false;
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

public:
	AVespPlayerController();
	virtual void PlayerTick(float Secondes) override;
	virtual void EndPlay(const EEndPlayReason::Type Raison) override;

	void Commencer(AVespGrille* LaGrille, AVespUnite* LAylis, ACameraActor* LaCamera);

	// Ce que l'interface peut demander
	void NouvellePartie(int32 ActeDeDepart = 1);
	void Quitter();
	void ChoisirAction(EVespAction Action);
	void ChoisirRune(int32 Numero);
	void ChoisirNoeud(int32 Noeud);			// une salle de la carte de la route
	void AcheterOffre(int32 Numero);
	void QuitterMarchand();
	void ChoisirEvenement(int32 Choix);
	void AvancerDialogue();
	void ContinuerApresLActe();
	void PasserLeTour();
	void Recommencer();

	static FString NomAction(EVespAction Action);
	static FString DetailAction(EVespAction Action, int32 Potions);
	static FString AideAction(EVespAction Action);
	bool ActionDisponible(EVespAction Action) const;
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

	UPROPERTY(EditAnywhere, Category = "Vesperance") int32 DeplacementParTour = 3;	// comme dans le prototype
	UPROPERTY(EditAnywhere, Category = "Vesperance") float PauseEntreHaschen = 0.45f;
	static constexpr int32 NombreDEtages = 14;		// par acte : 13 etages de salles, puis le boss
	static constexpr int32 NombreDActes = 7;
	static constexpr int32 HaschenMax = 8;			// jamais plus de Haschen debout (les renforts s'arretent la)

private:
	// La route
	void GenererRoute();
	void ProposerSalles();
	void PreparerCombat(EVespSalle Type);
	void ApresVictoire();
	void ProposerRunes();
	void AppliquerRune(int32 Rune);
	int32 RuneAuHasard() const;
	bool RunePossible(int32 Rune) const;
	void OuvrirMarchand();
	void OuvrirEvenement();
	void DialogueDuBoss();
	void AmbianceDeLActe();

	// Les Haschen
	AVespUnite* CreerHaschen(const FVespModeleHaschen& Modele, FIntPoint Case, bool bSelonEtage = true);
	AVespUnite* HaschenAuHasard(TArray<FIntPoint>& Places);
	AVespUnite* EliteAuHasard(TArray<FIntPoint>& Places);
	int32 AppelerRenforts(AVespUnite* Source, const FVespModeleHaschen& Modele, int32 Nombre);	// renvoie combien sont venus
	bool CaseLibrePres(FIntPoint Centre, FIntPoint& Trouvee, int32 RayonMax = 3) const;
	int32 HaschenDebout() const;
	void LancerVague();
	bool VerifierFinDeVague();		// true : plus personne debout (une vague arrive, ou c'est la victoire)

	// Le combat
	void TourDAylis();
	void FinDuTourDAylis();
	void JouerTourHaschen(float Secondes);
	void ReglesDuTour();						// au debut de chaque tour : la regle de l'acte (pieges, maree, eruptions...)
	bool TourDuBoss(AVespUnite* Boss);			// true : il a utilise son tour
	bool TourDeSkarn(AVespUnite* Skarn);
	bool TourDeLaMatriarche(AVespUnite* Matriarche);
	bool TourDuRoiNoye(AVespUnite* Roi);
	bool TourDuGardien(AVespUnite* Gardien);
	bool TourDAshka(AVespUnite* Ashka);
	bool TourDeVorgath(AVespUnite* Vorgath);
	bool TourDeLOracle(AVespUnite* Oracle);
	void Annoncer(const TArray<FIntPoint>& Cases, int32 Degats, int32 Effet, TCHAR Terrain);	// une attaque annoncee
	void FrapperZones(AVespUnite* Source);		// l'attaque annoncee tombe
	void MontrerDangers();
	void EffetDuTerrain(AVespUnite* U);			// fin de son tour sur la lave, les eaux toxiques, une dalle levee
	void InfligerEffet(AVespUnite* Cible, int32 Effet);
	void Blesser(AVespUnite* Cible, int32 Degats, const FString& Cause);	// des degats qui ne viennent pas d'un coup
	void Explosions();							// les Haschen explosifs tombes explosent
	void Fuir(AVespUnite* H);					// un tireur recule
	void MontrerCasesAtteignables();
	bool CaseSousLaSouris(FIntPoint& Case) const;
	bool ResteDesHaschen() const;
	void Ecrire(const FString& Message);			// le journal (les derniers messages)
	void AgirSur(AVespUnite* Cible);
	void ToucherAylis(AVespUnite* Attaquant, int32 Degats, bool bCritique);
	void VerifierDefaite();
	void Trembler(float Force) { Secousse = FMath::Min(1.5f, Secousse + Force); }
	void Ralenti(float Echelle, float Duree);		// un instant de ralenti (un coup fatal, un critique)
	void PlacerCamera(float Secondes);
	int32 DeplacementCeTour() const;

	UPROPERTY() TObjectPtr<AVespGrille> Grille;
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TArray<TObjectPtr<AVespUnite>> Haschen;
	UPROPERTY() TObjectPtr<ACameraActor> CameraArene;
	TSharedPtr<class SVespInterface> Interface;
	FVector PositionCamera;
	FRotator RotationCamera;
	float Secousse = 0.0f;
	float Zoom = 0.0f;				// > 0 : la camera se resserre un instant (un critique)
	double FinDuRalenti = 0.0;

	// L'etat du combat
	EVespPhase Phase = EVespPhase::Titre;
	EVespAction ActionChoisie = EVespAction::Attaque;
	TArray<FString> Journal;
	int32 Tour = 0;
	int32 Potions = 3;
	int32 Rage = 0;					// de 0 a 100 : elle monte quand AYLIS encaisse des coups
	int32 Eclats = 0;				// la monnaie du marchand
	bool bEnGarde = false;
	bool bADejaBouge = false;
	int32 HaschenQuiJoue = 0;		// pendant le tour des Haschen : lequel joue
	int32 EtapeHaschen = 0;			// 0 = il se prepare, 1 = il frappe (une fois arrive)
	float Minuteur = 0.0f;
	int32 VaguesRestantes = 0;		// les renforts qui arriveront quand la vague en cours sera tombee
	int32 VagueActuelle = 1;
	TArray<FIntPoint> ZonesDanger;	// les cases annoncees par un boss
	int32 DegatsDanger = 0;
	int32 EffetDanger = 0;
	TCHAR TerrainDanger = '.';		// ce que deviennent les cases touchees ('.' = rien ne change)
	TArray<FIntPoint> Eruptions;	// les Terres de Cendre : les cases qui vont entrer en eruption
	bool bBlizzard = false;			// le col : ce tour-ci, le blizzard ralentit tout le monde
	int32 PotionsDuBoss = 2;		// les decoctions de la Matriarche

	// La route et les runes
	int32 Acte = 1;
	int32 Etage = 0;				// l'etage de la salle en cours (0 a 13)
	EVespSalle TypeSalle = EVespSalle::Combat;
	TArray<FVespNoeud> Noeuds;		// la carte de la route de l'acte
	int32 NoeudActuel = -1;
	TArray<int32> NoeudsPossibles;	// les salles ou l'on peut aller maintenant
	TArray<int32> RunesProposees;
	TArray<int32> Runes;
	TArray<FVespOffre> Offres;		// le marchand
	int32 EvenementActuel = 0;
	bool bFlamme = false;			// les coups d'AYLIS peuvent bruler
	bool bSeve = false;				// +5 pv a chaque Haschen abattu
	bool bFureur = false;			// la rage monte 2 fois plus vite
	bool bEpines = false;			// renvoie 3 degats a qui touche AYLIS
	bool bSangsue = false;			// +2 pv a chaque coup porte
	bool bFortune = false;			// +50% d'eclats
	bool bRempart = false;			// la garde divise les degats par 3
	bool bGivre = false;			// les coups d'AYLIS peuvent geler
	bool bPiedSur = false;			// les terrains dangereux n'atteignent plus AYLIS
	FString MessageRoute;			// ce qui vient de se passer (affiche sur les ecrans de choix)
	float TempsPhase = 0.0f;		// depuis combien de temps l'ecran en cours est affiche (pour les fondus)

	// Le dialogue
	TArray<FString> Orateurs;
	TArray<FString> Repliques;
	int32 LigneDialogue = 0;
	float Ecriture = 0.0f;			// le nombre de lettres deja ecrites
};
