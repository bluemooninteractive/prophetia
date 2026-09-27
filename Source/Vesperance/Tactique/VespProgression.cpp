#include "VespProgression.h"

// ===================== Le Seuil : les 12 etoiles =====================

static const FVespEtoile ETOILES[VespSeuil::Nombre] = {
	// La Lame : frapper fort, frapper juste
	{TEXT("fendant"), TEXT("Fendant"), TEXT("L'attaque lourde fend l'air : une vague de lumiere part devant AYLIS."), EVespVoie::Lame, 1, 0, 0.0f},
	{TEXT("affutee"), TEXT("Lame affutee"), TEXT("+15% de chances de critique, et les critiques font encore plus mal."), EVespVoie::Lame, 2, 0, 0.0f},
	{TEXT("tourbillon"), TEXT("Tourbillon"), TEXT("POUVOIR (1 / LB) : AYLIS tournoie, et tout ce qui l'entoure encaisse."), EVespVoie::Lame, 3, 1, 7.0f},
	{TEXT("execution"), TEXT("Execution"), TEXT("+60% de degats contre les Haschen a moins d'un tiers de leurs pv."), EVespVoie::Lame, 4, 0, 0.0f},
	// Le Rempart : tenir, et renvoyer
	{TEXT("garde"), TEXT("Garde patiente"), TEXT("La parade parfaite est plus facile a reussir, et la garde retient plus."), EVespVoie::Rempart, 1, 0, 0.0f},
	{TEXT("ecorce"), TEXT("Ecorce"), TEXT("+20 pv max et +2 defense."), EVespVoie::Rempart, 2, 0, 0.0f},
	{TEXT("egide"), TEXT("Egide"), TEXT("POUVOIR (2 / RT) : 3 secondes d'un bouclier de lumiere (80% de degats en moins)."), EVespVoie::Rempart, 3, 2, 12.0f},
	{TEXT("riposte"), TEXT("Riposte"), TEXT("Apres une parade parfaite, le coup suivant fait triple degats."), EVespVoie::Rempart, 4, 0, 0.0f},
	// La Prophetie : voir avant les autres
	{TEXT("eveil"), TEXT("Eveil"), TEXT("L'esquive laisse une onde qui blesse les Haschen autour, et revient plus vite."), EVespVoie::Prophetie, 1, 0, 0.0f},
	{TEXT("ether"), TEXT("Lame d'ether"), TEXT("POUVOIR (3 / croix droite) : une lame spectrale traverse tout ce qu'elle croise."), EVespVoie::Prophetie, 2, 3, 4.0f},
	{TEXT("presage"), TEXT("Presage"), TEXT("Une esquive au dernier moment ralentit le temps."), EVespVoie::Prophetie, 3, 0, 0.0f},
	{TEXT("nova"), TEXT("Nova"), TEXT("L'attaque speciale devient une nova immense, qui soigne AYLIS."), EVespVoie::Prophetie, 4, 0, 0.0f},
};

const FVespEtoile& VespSeuil::Etoile(int32 Index)
{
	return ETOILES[FMath::Clamp(Index, 0, Nombre - 1)];
}

int32 VespSeuil::Index(const TCHAR* Id)
{
	for (int32 i = 0; i < Nombre; i++)
	{
		if (FCString::Strcmp(ETOILES[i].Id, Id) == 0)
		{
			return i;
		}
	}
	return -1;
}

FLinearColor VespSeuil::CouleurVoie(EVespVoie Voie)
{
	switch (Voie)
	{
		case EVespVoie::Lame: return FLinearColor(0.95f, 0.55f, 0.35f);
		case EVespVoie::Rempart: return FLinearColor(0.45f, 0.75f, 1.0f);
		default: return FLinearColor(0.75f, 0.55f, 1.0f);
	}
}

const TCHAR* VespSeuil::NomVoie(EVespVoie Voie)
{
	switch (Voie)
	{
		case EVespVoie::Lame: return TEXT("LA LAME");
		case EVespVoie::Rempart: return TEXT("LE REMPART");
		default: return TEXT("LA PROPHETIE");
	}
}

// ===================== Le butin =====================

FLinearColor VespButin::CouleurRarete(EVespRarete R)
{
	switch (R)
	{
		case EVespRarete::Rare: return FLinearColor(0.44f, 0.66f, 1.0f);
		case EVespRarete::Epique: return FLinearColor(0.69f, 0.55f, 1.0f);
		case EVespRarete::Legendaire: return FLinearColor(0.89f, 0.73f, 0.36f);
		default: return FLinearColor(0.66f, 0.62f, 0.74f);
	}
}

const TCHAR* VespButin::NomRarete(EVespRarete R)
{
	switch (R)
	{
		case EVespRarete::Rare: return TEXT("RARE");
		case EVespRarete::Epique: return TEXT("EPIQUE");
		case EVespRarete::Legendaire: return TEXT("LEGENDAIRE");
		default: return TEXT("COMMUN");
	}
}

const TCHAR* VespButin::NomEmplacement(EVespEmplacement E)
{
	switch (E)
	{
		case EVespEmplacement::Tete: return TEXT("TETE");
		case EVespEmplacement::Amulette: return TEXT("AMULETTE");
		case EVespEmplacement::Arme: return TEXT("ARME");
		case EVespEmplacement::Anneau: return TEXT("ANNEAU");
		case EVespEmplacement::MainGauche: return TEXT("MAIN GAUCHE");
		default: return TEXT("CORPS");
	}
}

FString FVespObjet::Lignes() const
{
	TArray<FString> L;
	if (Attaque) L.Add(FString::Printf(TEXT("+%d attaque"), Attaque));
	if (Defense) L.Add(FString::Printf(TEXT("+%d defense"), Defense));
	if (PvMax) L.Add(FString::Printf(TEXT("+%d pv max"), PvMax));
	if (Critique) L.Add(FString::Printf(TEXT("+%d%% de critique"), Critique));
	if (Vitesse > 0.0f) L.Add(FString::Printf(TEXT("+%d%% de vitesse"), FMath::RoundToInt(Vitesse * 100.0f)));
	if (VolDeVie) L.Add(FString::Printf(TEXT("+%d pv a chaque coup porte"), VolDeVie));
	if (Effet == VespEffetCoup::Brulure) L.Add(FString::Printf(TEXT("%d%% de chances de bruler"), FMath::RoundToInt(ChanceEffet * 100.0f)));
	if (Effet == VespEffetCoup::Gel) L.Add(FString::Printf(TEXT("%d%% de chances de geler"), FMath::RoundToInt(ChanceEffet * 100.0f)));
	if (Effet == VespEffetCoup::Poison) L.Add(FString::Printf(TEXT("%d%% de chances d'empoisonner"), FMath::RoundToInt(ChanceEffet * 100.0f)));
	if (Emplacement == EVespEmplacement::Arme)
	{
		switch (TypeArme)
		{
			case EVespArme::DeuxMains: L.Add(TEXT("A deux mains : lent, large, lourd")); break;
			case EVespArme::Dagues: L.Add(TEXT("Deux dagues : des coups tres rapides")); break;
			case EVespArme::Baton: L.Add(TEXT("Baton : les coups partent en sorts, de loin")); break;
			default: break;
		}
	}
	return FString::Join(L, TEXT("\n"));
}

int32 FVespObjet::Valeur() const
{
	return Attaque * 6 + Defense * 6 + PvMax + Critique * 2 + FMath::RoundToInt(Vitesse * 100.0f) + VolDeVie * 8
	     + FMath::RoundToInt(ChanceEffet * 60.0f) + (int32)Rarete * 5;
}

static FString CheminArme(const TCHAR* Dossier, const TCHAR* Nom)
{
	return FString::Printf(TEXT("/Game/StylizedCharacter/Meshes/Item/Weapons/%s/%s.%s"), Dossier, Nom, Nom);
}

FVespObjet VespButin::EpeeDeDepart()
{
	FVespObjet O;
	O.Nom = TEXT("Epee de la vision");
	O.Recit = TEXT("Forgee dans la lumiere d'une prophetie qui n'a pas encore eu lieu.");
	O.Emplacement = EVespEmplacement::Arme;
	O.Modele = CheminArme(TEXT("Sword"), TEXT("SK_Sword_1H_Newbie_02"));
	O.Longueur = 0.5f;
	O.TypeArme = EVespArme::Epee;
	O.Icone = 0;
	return O;
}

FVespObjet VespButin::BouclierDeDepart()
{
	FVespObjet O;
	O.Nom = TEXT("Petit bouclier");
	O.Recit = TEXT("Il a deja arrete plus de fleches qu'on ne peut en compter.");
	O.Emplacement = EVespEmplacement::MainGauche;
	O.Modele = CheminArme(TEXT("Shield"), TEXT("SK_Shield_Newbie_02"));
	O.Icone = 3;
	return O;
}

FVespObjet VespButin::Tirer(int32 Acte, int32 Chance, FRandomStream& H)
{
	// Les armes tombent un peu plus souvent (ce sont elles qu'on attend)
	const int32 D = H.RandRange(0, 99);
	EVespEmplacement E = EVespEmplacement::Arme;
	if (D < 28) E = EVespEmplacement::Arme;
	else if (D < 42) E = EVespEmplacement::MainGauche;
	else if (D < 58) E = EVespEmplacement::Corps;
	else if (D < 72) E = EVespEmplacement::Tete;
	else if (D < 86) E = EVespEmplacement::Amulette;
	else E = EVespEmplacement::Anneau;
	return Tirer(Acte, Chance, E, H);
}

FVespObjet VespButin::Tirer(int32 Acte, int32 Chance, EVespEmplacement E, FRandomStream& H)
{
	FVespObjet O;
	O.Emplacement = E;
	O.Niveau = Acte;
	O.Graine = H.GetUnsignedInt();
	// La rarete : un boss donne au moins de l'epique, une elite au moins du rare
	const int32 R = H.RandRange(0, 99) + Chance * 30;
	O.Rarete = R >= 118 ? EVespRarete::Legendaire : (R >= 88 ? EVespRarete::Epique : (R >= 55 ? EVespRarete::Rare : EVespRarete::Commun));
	if (Chance >= 2 && O.Rarete < EVespRarete::Epique) O.Rarete = EVespRarete::Epique;
	if (Chance == 1 && O.Rarete < EVespRarete::Rare) O.Rarete = EVespRarete::Rare;
	static const float MULT[4] = {1.0f, 1.5f, 2.1f, 3.0f};
	const float M = MULT[(int32)O.Rarete];
	static const TCHAR* COULEURS[4] = {TEXT("Cl"), TEXT("Bl"), TEXT("Gn"), TEXT("Rd")};
	(void)COULEURS;
	auto Au = [&H](std::initializer_list<const TCHAR*> L) { TArray<const TCHAR*> A(L); return FString(A[H.RandRange(0, A.Num() - 1)]); };
	FString Base;
	switch (E)
	{
		case EVespEmplacement::Arme:
		{
			const int32 T = H.RandRange(0, 5);
			float Force = 1.0f;
			switch (T)
			{
				case 0:
					Base = Au({TEXT("Epee"), TEXT("Lame")});
					O.Modele = CheminArme(TEXT("Sword"), (H.FRand() < 0.5f) ? TEXT("SK_Sword_1H_Newbie_01") : TEXT("SK_Sword_1H_Newbie_02"));
					O.Longueur = 0.5f; O.TypeArme = EVespArme::Epee; O.Icone = 0;
					break;
				case 1:
				{
					Base = TEXT("Hache");
					static const TCHAR* H1[3] = {TEXT("SK_Axe_1H_Newbie_01"), TEXT("SK_Axe_1H_Newbie_02"), TEXT("SK_Axe_1H_Newbie_03")};
					O.Modele = CheminArme(TEXT("Axe"), H1[H.RandRange(0, 2)]);
					O.Longueur = 0.46f; O.TypeArme = EVespArme::Epee; O.Icone = 0; Force = 1.1f;
					break;
				}
				case 2:
				{
					Base = TEXT("Dagues");
					static const TCHAR* D1[3] = {TEXT("SK_Dagger_1H_Newbie_01"), TEXT("SK_Dagger_1H_Newbie_02"), TEXT("SK_Dagger_1H_Newbie_03")};
					const TCHAR* Choix = D1[H.RandRange(0, 2)];
					O.Modele = CheminArme(TEXT("Dagger"), Choix);
					O.SecondModele = O.Modele;
					O.Longueur = 0.32f; O.TypeArme = EVespArme::Dagues; O.Icone = 1; Force = 0.8f;
					O.Critique += 6;
					break;
				}
				case 3:
					Base = TEXT("Grande epee");
					O.Modele = CheminArme(TEXT("Sword"), (H.FRand() < 0.5f) ? TEXT("SK_Sword_2H_Newbie_01") : TEXT("SK_Sword_2H_Newbie_02"));
					O.Longueur = 0.66f; O.TypeArme = EVespArme::DeuxMains; O.Icone = 0; Force = 1.4f;
					break;
				case 4:
					Base = TEXT("Hache de guerre");
					O.Modele = CheminArme(TEXT("Axe"), TEXT("SK_Axe_2HL_Newbie_01"));
					O.Longueur = 0.7f; O.TypeArme = EVespArme::DeuxMains; O.Icone = 0; Force = 1.45f;
					break;
				default:
				{
					Base = TEXT("Baton");
					static const TCHAR* B1[4] = {TEXT("SK_Staff_Newbie_01"), TEXT("SK_Staff_Newbie_02"), TEXT("SK_Staff_Newbie_03"), TEXT("SK_Staff_Newbie_04")};
					O.Modele = CheminArme(TEXT("Staff"), B1[H.RandRange(0, 3)]);
					O.Longueur = 0.95f; O.TypeArme = EVespArme::Baton; O.Icone = 12; Force = 0.95f;
					break;
				}
			}
			O.Attaque = FMath::Max(1, FMath::RoundToInt((1.5f + Acte * 1.4f) * M * Force));
			break;
		}
		case EVespEmplacement::MainGauche:
		{
			Base = Au({TEXT("Bouclier"), TEXT("Ecu"), TEXT("Pavois")});
			static const TCHAR* S1[3] = {TEXT("SK_Shield_Newbie_01"), TEXT("SK_Shield_Newbie_02"), TEXT("SK_Shield_Newbie_03")};
			O.Modele = CheminArme(TEXT("Shield"), S1[H.RandRange(0, 2)]);
			O.Defense = FMath::Max(1, FMath::RoundToInt((0.8f + Acte * 0.55f) * M));
			O.Icone = 3;
			break;
		}
		case EVespEmplacement::Corps:
		{
			Base = Au({TEXT("Tunique"), TEXT("Manteau"), TEXT("Cuirasse"), TEXT("Cape")});
			O.Defense = FMath::Max(1, FMath::RoundToInt((0.5f + Acte * 0.45f) * M));
			O.PvMax = FMath::RoundToInt((4 + Acte * 3) * M);
			static const FLinearColor TEINTES[8] = {
				FLinearColor(0.25f, 0.39f, 0.88f), FLinearColor(0.55f, 0.12f, 0.16f), FLinearColor(0.16f, 0.42f, 0.22f), FLinearColor(0.12f, 0.12f, 0.15f),
				FLinearColor(0.8f, 0.62f, 0.2f), FLinearColor(0.45f, 0.22f, 0.7f), FLinearColor(0.5f, 0.52f, 0.58f), FLinearColor(0.85f, 0.82f, 0.74f)};
			O.Teinte = TEINTES[H.RandRange(0, 7)];
			O.Icone = 9;
			break;
		}
		case EVespEmplacement::Tete:
			Base = Au({TEXT("Capuche"), TEXT("Diademe"), TEXT("Heaume")});
			O.PvMax = FMath::RoundToInt((3 + Acte * 2) * M);
			O.Critique += FMath::RoundToInt(2 * M);
			O.Icone = 8;
			break;
		case EVespEmplacement::Amulette:
			Base = Au({TEXT("Amulette"), TEXT("Pendentif"), TEXT("Talisman")});
			O.PvMax = FMath::RoundToInt((5 + Acte * 3) * M);
			O.Vitesse = 0.03f * M;
			O.Icone = 10;
			break;
		default:
			Base = Au({TEXT("Anneau"), TEXT("Bague"), TEXT("Sceau")});
			O.Critique += FMath::RoundToInt(3 * M);
			O.Attaque = FMath::RoundToInt((0.5f + Acte * 0.4f) * M);
			O.Icone = 7;
			break;
	}
	// Au-dela du commun : un pouvoir de plus (bruler, geler, empoisonner, voler la vie)
	FString Suffixe;
	if (O.Rarete >= EVespRarete::Epique || (O.Rarete == EVespRarete::Rare && H.FRand() < 0.4f))
	{
		const int32 P = H.RandRange(0, 3);
		const float C = O.Rarete >= EVespRarete::Legendaire ? 0.3f : 0.18f;
		switch (P)
		{
			case 0: O.Effet = VespEffetCoup::Brulure; O.ChanceEffet = C; Suffixe = TEXT("de braise"); break;
			case 1: O.Effet = VespEffetCoup::Gel; O.ChanceEffet = C; Suffixe = TEXT("de givre"); break;
			case 2: O.Effet = VespEffetCoup::Poison; O.ChanceEffet = C; Suffixe = TEXT("des marais"); break;
			default: O.VolDeVie = 1 + (int32)O.Rarete / 2; Suffixe = TEXT("du sang"); break;
		}
	}
	if (Suffixe.IsEmpty())
	{
		static const TCHAR* LIEUX[7] = {TEXT("des brumes"), TEXT("des pendus"), TEXT("de Sombreval"), TEXT("d'Ashka"), TEXT("du col"), TEXT("de cendre"), TEXT("du Voile")};
		Suffixe = LIEUX[FMath::Clamp(Acte, 1, 7) - 1];
	}
	O.Nom = Base + TEXT(" ") + Suffixe;
	if (O.Rarete == EVespRarete::Legendaire)
	{
		static const TCHAR* LEGENDES[8] = {TEXT("Premiere Aube"), TEXT("Chant de la Matriarche"), TEXT("Couronne du Roi Noye"), TEXT("Coeur de pierre"),
		                                   TEXT("Souffle d'Ashka"), TEXT("Forge de Vorgath"), TEXT("Oeil de l'Oracle"), TEXT("Derniere Vision")};
		O.Nom = FString(LEGENDES[H.RandRange(0, 7)]) + TEXT(", ") + Base.ToLower();
		O.Critique += 5;
		O.PvMax += 5 * Acte;
	}
	static const TCHAR* RECITS[] = {
		TEXT("Trouve pres d'un feu encore tiede."), TEXT("Il a appartenu a quelqu'un qui n'est jamais rentre."),
		TEXT("Les Haschen le gardaient comme un tresor."), TEXT("Une lueur faible bat au creux du metal."),
		TEXT("Le Voile y a laisse une trace, fine comme un cheveu."), TEXT("On dirait qu'il attendait AYLIS."),
		TEXT("Il sent la pluie et la cendre."), TEXT("Une rune a moitie effacee brille dessus.")};
	O.Recit = RECITS[H.RandRange(0, UE_ARRAY_COUNT(RECITS) - 1)];
	return O;
}
