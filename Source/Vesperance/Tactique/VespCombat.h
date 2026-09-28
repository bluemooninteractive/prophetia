// VespCombat : le combat en temps reel, dans le monde de l'acte.
//
// Les Haschen attendent dans les clairieres gardees (ils sortent de terre quand AYLIS arrive) ou patrouillent
// le long des sentiers. Des qu'AYLIS entre dans une clairiere gardee, une barriere de lumiere la ferme : il faut
// vaincre tous les Haschen (et leurs renforts) pour qu'elle s'ouvre.
//
// Tout ce qui fait mal est ANNONCE avant de frapper, pour qu'on puisse le lire et l'eviter :
//   - un Haschen qui va frapper rougeoie, et une tache rouge se remplit au sol devant lui ;
//   - un chargeur trace sa ligne avant de foncer ; un sauteur marque l'endroit ou il va retomber ;
//   - les boss couvrent le sol de cercles (pluies de fleches, eruptions, ecrasements...).
// Les cercles rouges ne se parent pas : il faut en sortir, ou les traverser d'une esquive.
// Les coups au contact et les projectiles, eux, se parent au bouclier (une parade juste au bon moment les renvoie).
//
// Le combat ne decide rien pour AYLIS : le PlayerController lui transmet ses coups (FrappeDAylis...) et
// sa garde, et le combat le previent de ce qui arrive (un Haschen tombe, AYLIS est touchee, une clairiere se libere).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VespUnite.h"
#include "VespSons.h"
#include "VespCombat.generated.h"

class AVespMonde;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UNiagaraComponent;

// Un lieu du monde que le combat doit peupler (une clairiere de la route)
struct FVespLieu
{
	FVector Centre = FVector::ZeroVector;
	float Rayon = 1000.0f;
	int32 Type = 0;			// (int32) EVespSalle
	int32 Etage = 0;
	TArray<int32> Suivants;
};

// Un coup d'AYLIS (ou d'un de ses pouvoirs)
struct FVespCoup
{
	FVector Origine = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float Portee = 200.0f;
	float DemiAngle = 60.0f;		// en degres (180 : tout autour)
	float Puissance = 1.0f;			// multiplie l'attaque
	bool bLourd = false;			// fissure les armures, fait flechir meme les grands
	float Poussee = 250.0f;
	int32 Effet = 0;				// VespEffetCoup
	float ChanceEffet = 0.0f;
	bool bPeutCritiquer = true;
	FLinearColor Couleur = FLinearColor(0.55f, 0.7f, 1.0f);
};

// Une attaque annoncee au sol : la tache rouge se remplit, puis frappe (et parfois reste : poison, lave)
struct FVespDanger
{
	FVector Centre = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float Rayon = 200.0f;
	float Longueur = 0.0f;			// > 0 : une ligne (une charge), de Longueur x 2 Rayon
	float Delai = 1.0f;
	float DelaiTotal = 1.0f;
	int32 Degats = 10;
	int32 Effet = 0;
	bool bContreAylis = true;
	bool bContreHaschen = false;
	bool bParable = false;
	bool bDegats = true;			// false : une simple annonce (la charge fait mal elle-meme)
	float Persistance = 0.0f;		// apres l'impact : la zone reste (degats chaque seconde)
	float Tic = 0.0f;
	float Poussee = 300.0f;
	bool bFrappee = false;
	EVespSon Son = EVespSon::Impact;
	FLinearColor Couleur = FLinearColor(1.0f, 0.12f, 0.05f);
	TWeakObjectPtr<AVespUnite> Source;
	bool bAnnuleeSiSourceTombe = true;	// un Haschen abattu (ou sonne) avant de frapper : son attaque n'a pas lieu
	UStaticMeshComponent* Fond = nullptr;
	UStaticMeshComponent* Remplissage = nullptr;
	UMaterialInstanceDynamic* MatFond = nullptr;
	UMaterialInstanceDynamic* MatRemplissage = nullptr;
};

// Un projectile (fleche, sort, eclat) : il vole en ligne droite
struct FVespProjectile
{
	FVector Position = FVector::ZeroVector;
	FVector Vitesse = FVector::ZeroVector;
	float Vie = 2.0f;
	float Rayon = 50.0f;
	int32 Degats = 8;
	int32 Effet = 0;
	bool bContreAylis = true;
	bool bAttire = false;
	bool bRenvoye = false;			// renvoye par une parade parfaite
	bool bTraversant = false;		// il traverse ce qu'il touche (la lame d'ether)
	TArray<TWeakObjectPtr<AVespUnite>> DejaTouches;
	FLinearColor Couleur = FLinearColor(1.0f, 0.4f, 0.2f);
	TWeakObjectPtr<AVespUnite> Source;
	UStaticMeshComponent* Visuel = nullptr;
	UPointLightComponent* Lumiere = nullptr;
	FVespCoup Coup;					// les projectiles d'AYLIS : leur coup
};

// Ce qu'un Haschen est en train de faire
UENUM()
enum class EVespIntention : uint8
{
	Dormir,			// sous terre (clairiere gardee), ou au repos
	Errer,			// une patrouille qui marche
	Surgir,			// il sort de terre
	Poursuivre,
	Preparer,		// il va frapper (l'annonce)
	Frapper,
	Recuperer,
	Charger,
	Incanter,
	Sauter,
};

struct FVespHaschen
{
	TWeakObjectPtr<AVespUnite> U;
	EVespIntention Etat = EVespIntention::Dormir;
	float Temps = 0.0f;				// depuis combien de temps dans cet etat
	float Recharge = 1.0f;			// avant sa prochaine attaque
	float Recharge2 = 3.0f;			// avant son prochain talent (soin, invocation, saut)
	int32 Groupe = -1;
	int32 Motif = -1;				// un boss : l'attaque en cours
	int32 Geste = 0;				// ce qu'il prepare : 0 frappe, 1 tir, 2 charge, 3 saut, 4 soin, 5 invocation
	FVector Vise = FVector::ZeroVector;
	FVector Errance = FVector::ZeroVector;
	FVector ErranceB = FVector::ZeroVector;
	bool bVersB = true;
	bool bImpactFait = false;
	int32 Invoques = 0;
	TWeakObjectPtr<AVespUnite> Invocateur;
	float Contournement = 1.0f;		// il tourne autour d'AYLIS dans ce sens quand il attend son tour
	bool bBoss = false;
	bool bElite = false;
	bool bTombe = false;			// sa chute a deja ete traitee
	int32 PvDebut = 0;				// un boss : ses pv au debut du motif (pour l'interrompre)
	int32 DernierMotif = -1;
	int32 Etape = 0;				// un motif en plusieurs temps : ou il en est
};

// Un Haschen qui va apparaitre (une invocation, un renfort) : cree apres la reflexion de tous
struct FVespAppel
{
	FVector Position = FVector::ZeroVector;
	int32 Modele = 0;
	int32 Categorie = 0;
	int32 Groupe = -1;
	TWeakObjectPtr<AVespUnite> Invocateur;
};

// Un groupe : les Haschen d'une clairiere gardee, ou une patrouille
struct FVespGroupe
{
	int32 Zone = -1;				// la clairiere (-1 : une patrouille)
	int32 Type = 0;
	bool bEngage = false;
	bool bFerme = false;			// la barriere est levee
	bool bLibere = false;
	bool bBoss = false;
	int32 VaguesRestantes = 0;
	FVector Centre = FVector::ZeroVector;
	float Rayon = 1000.0f;
	int32 Etage = 0;
	TArray<FVector> Pieges;			// la forteresse : les dalles piegees de la clairiere
	float Chrono = 0.0f;			// pour les regles de l'acte
};

UCLASS()
class VESPERANCE_API AVespCombat : public AActor
{
	GENERATED_BODY()

public:
	AVespCombat();

	void Preparer(AVespMonde* LeMonde, AVespUnite* LAylis);
	// Les Haschen de l'acte : un groupe par clairiere gardee, et des patrouilles sur les sentiers
	void Peupler(int32 Acte, const TArray<FVespLieu>& Lieux);
	void Vider();
	// A chaque image de jeu (le PlayerController l'appelle ; rien ne bouge pendant les menus)
	void Avancer(float Secondes);

	// ----- AYLIS -----
	int32 FrappeDAylis(const FVespCoup& Coup);		// renvoie le nombre de Haschen touches
	void TirDAylis(const FVespCoup& Coup, float VitesseTir, float Rayon, bool bTraversant = false, float Taille = 1.0f);
	AVespUnite* CibleProche(const FVector& Depuis, const FVector& Direction, float Portee, float DemiAngle) const;
	void SetGarde(bool bLevee, float DepuisQuand) { bGarde = bLevee; TempsGarde = DepuisQuand; }
	// Ce qui fait mal a AYLIS passe ici (esquive, garde, parade, reduction, effets)
	void ToucherAylis(int32 Degats, int32 Effet, AVespUnite* Source, const FVector& Direction, float Poussee, bool bParable, EVespSon Son);

	// ----- Ce que le jeu veut savoir -----
	bool EnCombat() const;					// une clairiere est fermee, ou des Haschen chassent AYLIS
	const FVespGroupe* GroupeFerme() const;
	AVespUnite* BossActif() const;
	int32 HaschenEngages() const;
	const TArray<FVespHaschen>& GetHaschen() const { return Haschen; }
	void Blizzard(float Duree) { TempsBlizzard = Duree; }
	float BlizzardEnCours() const { return TempsBlizzard; }
	static FString NomDuBoss(int32 Acte);

	// La barriere : la clairiere ou AYLIS est enfermee (Rayon 0 : aucune)
	FVector CentreEnclos = FVector::ZeroVector;
	float RayonEnclos = 0.0f;

	// ----- Ce que le combat annonce au jeu -----
	TFunction<void(AVespUnite*, int32)> SurChute;					// un Haschen tombe (l'XP, le butin) ; 0 normal, 1 elite, 2 boss
	TFunction<void(int32, bool, AVespUnite*)> SurBlessure;			// AYLIS encaisse (degats, critique, qui)
	TFunction<void(int32)> SurFermeture;							// une clairiere se ferme : le combat commence
	TFunction<void(int32)> SurLiberation;							// une clairiere est liberee
	TFunction<void(float, bool)> SurImpact;							// un coup porte (force, critique) : la camera tremble
	TFunction<void(const FString&)> SurMessage;
	TFunction<void()> SurParade;									// une parade parfaite
	TFunction<void()> SurEsquive;									// une esquive au dernier moment

	// Les runes d'AYLIS qui touchent au combat
	// Ce dont le monde se souvient (le PlayerController le donne au debut de la vision)
	bool bOracleAylis = false;		// apres une premiere victoire, l'Oracle a le visage d'AYLIS
	FString ArmeEcho;				// l'arme de la derniere vision tombee (vide : l'epee des echos)
	FString SecondeArmeEcho;
	float LongueurEcho = 0.5f;
	EVespArme TypeEcho = EVespArme::Epee;
	bool bEpines = false;			// qui touche AYLIS au contact se blesse
	bool bPiedSur = false;			// les pieges, flaques et eruptions (sans lanceur) n'atteignent pas AYLIS
	float MultCritique = 2.0f;		// un critique d'AYLIS multiplie les degats par ceci
	bool bExecution = false;		// +60% contre les Haschen a moins d'un tiers de leurs pv
	float FenetreParade = 0.22f;	// la parade parfaite : secondes apres avoir leve la garde
	// Une embuscade (un evenement) : des Haschen surgissent autour d'AYLIS, sans barriere
	void Embuscade(const FVector& Centre, bool bElite);
	// Le mode photo : la barriere tombe et ses Haschen s'effacent (AYLIS change de clairiere d'un coup)
	void LeverLaBarriere();
	float BonusParade = 0.0f;		// la garde encaisse davantage (0 a 1)
	int32 DefenseAylis() const;

private:
	// Les Haschen
	AVespUnite* Creer(int32 IndexModele, int32 Categorie, const FVector& Position, int32 Groupe, int32 Etage);	// Categorie : 0 normal, 1 elite, 2 boss
	void Penser(FVespHaschen& H, float Secondes);
	void PenserBoss(FVespHaschen& H, float Secondes);
	void Bouger(FVespHaschen& H, const FVector& Vers, float Vitesse, float Secondes, bool bTourner = true);
	void Eloigner(float Secondes);			// ils ne se marchent pas dessus
	void Changer(FVespHaschen& H, EVespIntention Etat);
	void PreparerAttaque(FVespHaschen& H, int32 Geste);
	void Frapper(FVespHaschen& H);
	int32 DegatsDe(const AVespUnite* U, float Multiplicateur) const;
	int32 AttaquantsAuContact() const;
	void Engager(int32 Groupe);
	void Fermer(int32 Groupe);
	void Ouvrir(int32 Groupe);
	void VagueSuivante(int32 Groupe);
	void ReglesDeLActe(FVespGroupe& G, float Secondes);
	void Mort(FVespHaschen& H);
	void Invoquer(FVespHaschen& Source, int32 Nombre, int32 IndexModele, int32 Categorie = 0);
	void CreerAppels();
	FVespHaschen* Trouver(const AVespUnite* U);
	void Blesser(AVespUnite* Cible, int32 Degats, bool bCritique, const FVector& Direction, float Poussee, bool bLourd, int32 Effet, const FLinearColor& Couleur);
	void AppliquerEffet(AVespUnite* Cible, int32 Effet);

	// Les attaques annoncees et les projectiles
	FVespDanger& Annoncer(const FVector& Centre, float Rayon, float Delai, int32 Degats, AVespUnite* Source);
	FVespDanger& AnnoncerLigne(const FVector& Depart, const FVector& Direction, float Longueur, float Largeur, float Delai, AVespUnite* Source);
	void Tirer(AVespUnite* Source, const FVector& Depart, const FVector& Direction, float VitesseTir, int32 Degats, int32 Effet, bool bAttire, const FLinearColor& Couleur);
	void AvancerDangers(float Secondes);
	void AvancerProjectiles(float Secondes);
	void EffacerDanger(FVespDanger& Z);
	bool DansDanger(const FVespDanger& Z, const FVector& P, float Marge) const;
	UStaticMeshComponent* Disque(const FLinearColor& Couleur, UMaterialInstanceDynamic*& Materiau, bool bCube);

	// Les motifs des boss
	void LancerMotif(FVespHaschen& H, int32 Motif);
	void AvancerMotif(FVespHaschen& H, float Secondes);

	// La barriere
	void ConstruireBarriere(const FVector& Centre, float Rayon);
	void EffacerBarriere();

	UPROPERTY() TObjectPtr<AVespMonde> Monde;
	UPROPERTY() TObjectPtr<AVespUnite> Aylis;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylindre;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Racine;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Barriere;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> LumieresBarriere;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatBarriere;
	UPROPERTY() TArray<TObjectPtr<UActorComponent>> Recyclage;		// les disques et projectiles caches, prets a resservir

	TArray<FVespHaschen> Haschen;
	TArray<FVespGroupe> Groupes;
	TArray<FVespDanger> Dangers;
	TArray<FVespProjectile> Projectiles;
	TArray<FVespAppel> Appels;
	int32 Acte = 1;
	bool bGarde = false;
	float TempsGarde = 0.0f;
	float TempsBlizzard = 0.0f;
	float TempsBarriere = 0.0f;
	float Horloge = 0.0f;
	float SolZ = 0.0f;
};
