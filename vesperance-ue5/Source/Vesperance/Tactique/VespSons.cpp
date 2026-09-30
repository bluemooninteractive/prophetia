#include "VespSons.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundAttenuation.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

// ===================== La boite a outils du son =====================
// Tout est calcule en nombres a virgule (de -1 a 1), a 32 000 echantillons par seconde, puis converti en 16 bits.

namespace
{
	constexpr int32 FREQ = 32000;
	constexpr float DEUX_PI = 2.0f * PI;
	using FTampon = TArray<float>;

	struct FHasard
	{
		uint32 Etat;
		explicit FHasard(uint32 Graine) : Etat(Graine * 2654435761u + 12345u) {}
		float Uni()			// de 0 a 1
		{
			Etat ^= Etat << 13;
			Etat ^= Etat >> 17;
			Etat ^= Etat << 5;
			return (Etat & 0xFFFFFF) / float(0xFFFFFF);
		}
		float Bruit() { return Uni() * 2.0f - 1.0f; }
		float Entre(float A, float B) { return A + (B - A) * Uni(); }
	};

	FTampon Vide(float Secondes)
	{
		FTampon T;
		T.SetNumZeroed(FMath::Max(1, FMath::RoundToInt(Secondes * FREQ)));
		return T;
	}

	void Ajouter(FTampon& Dans, const FTampon& Son, float Debut, float Gain = 1.0f)
	{
		const int32 D = FMath::RoundToInt(Debut * FREQ);
		for (int32 i = 0; i < Son.Num() && D + i < Dans.Num(); i++)
		{
			if (D + i >= 0)
			{
				Dans[D + i] += Son[i] * Gain;
			}
		}
	}

	// Un filtre simple (une "pente") : il adoucit (passe-bas)
	struct FPasseBas
	{
		float Y = 0.0f;
		float Traiter(float X, float Coupure)
		{
			const float A = 1.0f - FMath::Exp(-DEUX_PI * FMath::Clamp(Coupure, 10.0f, FREQ * 0.45f) / FREQ);
			Y += A * (X - Y);
			return Y;
		}
	};

	// Un filtre a resonance (passe-bande) : il ne garde qu'une "couleur" du bruit (Chamberlin)
	struct FResonant
	{
		float Bas = 0.0f, Bande = 0.0f;
		float Traiter(float X, float Centre, float Q)
		{
			const float F = 2.0f * FMath::Sin(PI * FMath::Clamp(Centre, 20.0f, 7000.0f) / FREQ);
			const float Amort = 1.0f / FMath::Max(0.5f, Q);
			const float Haut = X - Bas - Amort * Bande;
			Bande += F * Haut;
			Bas += F * Bande;
			return Bande;
		}
	};

	float Enveloppe(float T, float Attaque, float Chute)
	{
		if (T < 0.0f) return 0.0f;
		if (T < Attaque) return T / Attaque;
		return FMath::Exp(-(T - Attaque) / FMath::Max(0.001f, Chute));
	}

	float Cloche(float T, float Duree)		// monte puis redescend (en forme de cloche)
	{
		if (T < 0.0f || T > Duree) return 0.0f;
		const float S = FMath::Sin(PI * T / Duree);
		return S * S;
	}

	float Note(float Midi) { return 440.0f * FMath::Pow(2.0f, (Midi - 69.0f) / 12.0f); }

	// Un son de cloche (modulation de frequence) : clair, puis de plus en plus doux
	FTampon Clochette(float Frequence, float Duree, float Brillance, float Rapport = 1.41f)
	{
		FTampon T = Vide(Duree);
		float Pc = 0.0f, Pm = 0.0f;
		for (int32 i = 0; i < T.Num(); i++)
		{
			const float t = float(i) / FREQ;
			const float Indice = Brillance * FMath::Exp(-t * 6.0f);
			Pm += DEUX_PI * Frequence * Rapport / FREQ;
			Pc += DEUX_PI * Frequence / FREQ;
			T[i] = FMath::Sin(Pc + Indice * FMath::Sin(Pm)) * Enveloppe(t, 0.004f, Duree * 0.3f);
		}
		return T;
	}

	// Un choc metallique : des harmoniques "non justes" qui s'eteignent a des vitesses differentes
	FTampon Metal(float F0, float Duree, float Gain)
	{
		static const float Rapports[5] = {1.0f, 2.76f, 5.40f, 8.93f, 13.3f};
		static const float Amps[5] = {0.5f, 0.35f, 0.25f, 0.15f, 0.08f};
		FTampon T = Vide(Duree);
		for (int32 k = 0; k < 5; k++)
		{
			float P = 0.0f;
			const float Chute = Duree * 0.35f / (1.0f + k * 0.6f);
			for (int32 i = 0; i < T.Num(); i++)
			{
				P += DEUX_PI * F0 * Rapports[k] / FREQ;
				T[i] += FMath::Sin(P) * Amps[k] * Gain * Enveloppe(float(i) / FREQ, 0.001f, Chute);
			}
		}
		return T;
	}

	// Une corde pincee (Karplus-Strong) : la corde d'un arc
	FTampon Corde(float Frequence, float Duree, FHasard& H)
	{
		FTampon T = Vide(Duree);
		const int32 Periode = FMath::Max(2, FMath::RoundToInt(FREQ / Frequence));
		TArray<float> Ligne;
		Ligne.SetNum(Periode);
		for (float& X : Ligne) X = H.Bruit();
		int32 k = 0;
		for (int32 i = 0; i < T.Num(); i++)
		{
			const int32 Suivant = (k + 1) % Periode;
			const float V = 0.5f * (Ligne[k] + Ligne[Suivant]) * 0.994f;
			T[i] = Ligne[k];
			Ligne[k] = V;
			k = Suivant;
		}
		return T;
	}

	// Un "boum" : un sinus dont la hauteur tombe
	FTampon Boum(float De, float A, float Duree, float Chute)
	{
		FTampon T = Vide(Duree);
		float P = 0.0f;
		for (int32 i = 0; i < T.Num(); i++)
		{
			const float t = float(i) / FREQ;
			const float F = A + (De - A) * FMath::Exp(-t * 18.0f);
			P += DEUX_PI * F / FREQ;
			T[i] = FMath::Sin(P) * Enveloppe(t, 0.002f, Chute);
		}
		return T;
	}

	// Du bruit colore par un filtre dont le centre suit une fonction du temps
	template <typename FCentre, typename FAmp>
	FTampon Souffle(float Duree, FHasard& H, FCentre Centre, FAmp Amplitude, float Q = 1.5f)
	{
		FTampon T = Vide(Duree);
		FResonant R;
		for (int32 i = 0; i < T.Num(); i++)
		{
			const float t = float(i) / FREQ;
			T[i] = R.Traiter(H.Bruit(), Centre(t), Q) * Amplitude(t);
		}
		return T;
	}

	// Un echo de salle (Schroeder : quatre peignes et deux passe-tout)
	void Echo(FTampon& T, float Taille, float Melange)
	{
		static const int32 Peignes[4] = {810, 862, 927, 984};
		static const int32 PasseTout[2] = {404, 320};
		FTampon Mouille;
		Mouille.SetNumZeroed(T.Num());
		for (int32 c = 0; c < 4; c++)
		{
			TArray<float> Tampon;
			Tampon.SetNumZeroed(Peignes[c]);
			int32 k = 0;
			float Filtre = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float Sortie = Tampon[k];
				Filtre = Sortie * 0.75f + Filtre * 0.25f;
				Tampon[k] = T[i] + Filtre * Taille;
				k = (k + 1) % Peignes[c];
				Mouille[i] += Sortie * 0.25f;
			}
		}
		for (int32 a = 0; a < 2; a++)
		{
			TArray<float> Tampon;
			Tampon.SetNumZeroed(PasseTout[a]);
			int32 k = 0;
			for (int32 i = 0; i < Mouille.Num(); i++)
			{
				const float B = Tampon[k];
				const float Y = -Mouille[i] + B;
				Tampon[k] = Mouille[i] + B * 0.5f;
				k = (k + 1) % PasseTout[a];
				Mouille[i] = Y;
			}
		}
		for (int32 i = 0; i < T.Num(); i++)
		{
			T[i] = T[i] * (1.0f - Melange * 0.5f) + Mouille[i] * Melange;
		}
	}

	void Saturer(FTampon& T, float Force)
	{
		for (float& X : T) X = FMath::Tanh(X * Force) / FMath::Tanh(Force);
	}

	// Une boucle sans couture : la fin (qui deborde) revient se melanger au debut
	void Boucler(FTampon& T, float DureeBoucle)
	{
		const int32 N = FMath::RoundToInt(DureeBoucle * FREQ);
		if (T.Num() <= N)
		{
			return;
		}
		for (int32 i = N; i < T.Num(); i++)
		{
			T[i - N] += T[i];
		}
		T.SetNum(N);
	}

	TSharedPtr<TArray<uint8>> EnOctets(FTampon T, float Gain, bool bNormaliser = true)
	{
		float Crete = 0.0001f;
		for (float X : T) Crete = FMath::Max(Crete, FMath::Abs(X));
		const float Echelle = (bNormaliser ? 0.9f / Crete : 1.0f) * Gain;
		// Un tout petit fondu au debut et a la fin (pas de "clic")
		const int32 Fondu = FMath::Min(64, T.Num() / 4);
		TSharedPtr<TArray<uint8>> O = MakeShared<TArray<uint8>>();
		O->SetNumUninitialized(T.Num() * 2);
		for (int32 i = 0; i < T.Num(); i++)
		{
			float X = T[i] * Echelle;
			if (i < Fondu) X *= float(i) / Fondu;
			if (i > T.Num() - Fondu) X *= float(T.Num() - i) / Fondu;
			const int16 S = (int16)FMath::Clamp(FMath::RoundToInt(X * 32767.0f), -32767, 32767);
			(*O)[i * 2] = uint8(S & 0xFF);
			(*O)[i * 2 + 1] = uint8((S >> 8) & 0xFF);
		}
		return O;
	}
}

// ===================== Les sons, un par un =====================

static FTampon Fabriquer(EVespSon Son, uint32 Graine)
{
	FHasard H(Graine + (uint32)Son * 7919u);
	switch (Son)
	{
		case EVespSon::Pas:
		{
			FTampon T = Vide(0.14f);
			FPasseBas Bas;
			const float Coupure = H.Entre(600.0f, 1100.0f);
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				T[i] = Bas.Traiter(H.Bruit(), Coupure) * Enveloppe(t, 0.004f, 0.035f) * 2.0f;
				T[i] += H.Bruit() * Enveloppe(t, 0.001f, 0.006f) * 0.15f;		// le craquement de l'herbe
			}
			Ajouter(T, Boum(110.0f, 70.0f, 0.1f, 0.03f), 0.0f, 0.35f);
			return T;
		}
		case EVespSon::Frappe:
		case EVespSon::FrappeLourde:
		{
			const bool bLourde = Son == EVespSon::FrappeLourde;
			const float D = bLourde ? 0.42f : H.Entre(0.22f, 0.28f);
			const float Bas = bLourde ? 250.0f : H.Entre(450.0f, 650.0f);
			const float Haut = bLourde ? 1500.0f : H.Entre(2400.0f, 3200.0f);
			FTampon T = Souffle(D, H, [=](float t) { return Bas + Haut * FMath::Pow(FMath::Sin(PI * FMath::Clamp(t / (D * 0.85f), 0.0f, 1.0f)), 1.5f); },
			                    [=](float t) { return Cloche(t, D); }, 2.5f);
			if (bLourde)
			{
				FTampon Grave = Vide(D);
				float P = 0.0f;
				for (int32 i = 0; i < Grave.Num(); i++)
				{
					P += DEUX_PI * 70.0f / FREQ;
					Grave[i] = FMath::Sin(P) * Cloche(float(i) / FREQ, D);
				}
				Ajouter(T, Grave, 0.0f, 0.3f);
			}
			return T;
		}
		case EVespSon::Impact:
		case EVespSon::Critique:
		case EVespSon::ImpactArmure:
		{
			const bool bCrit = Son == EVespSon::Critique;
			FTampon T = Vide(bCrit ? 0.9f : 0.35f);
			FPasseBas Bas;
			FResonant Milieu;
			const float CentreMilieu = H.Entre(700.0f, 1200.0f);
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				const float B = H.Bruit();
				T[i] += (B - Bas.Traiter(B, 1500.0f)) * Enveloppe(t, 0.0005f, 0.012f) * 0.8f;		// le claquement
				T[i] += Milieu.Traiter(B, CentreMilieu, 2.0f) * Enveloppe(t, 0.001f, 0.05f) * 1.2f;	// la chair
			}
			Ajouter(T, Boum(H.Entre(130.0f, 160.0f), 55.0f, 0.3f, 0.07f), 0.0f, 1.0f);				// le choc sourd
			if (Son == EVespSon::ImpactArmure)
			{
				Ajouter(T, Metal(H.Entre(480.0f, 560.0f), 0.35f, 1.0f), 0.0f, 0.8f);
			}
			if (bCrit)
			{
				Ajouter(T, Metal(H.Entre(850.0f, 950.0f), 0.8f, 1.0f), 0.0f, 0.6f);
				Ajouter(T, Boum(80.0f, 42.0f, 0.6f, 0.25f), 0.0f, 1.0f);
				Echo(T, 0.7f, 0.25f);
			}
			Saturer(T, 1.8f);
			return T;
		}
		case EVespSon::Parade:
		{
			FTampon T = Metal(H.Entre(640.0f, 720.0f), 0.8f, 1.0f);
			FTampon Clic = Vide(0.02f);
			for (int32 i = 0; i < Clic.Num(); i++) Clic[i] = H.Bruit() * Enveloppe(float(i) / FREQ, 0.0005f, 0.004f);
			Ajouter(T, Clic, 0.0f, 0.8f);
			Ajouter(T, Boum(160.0f, 90.0f, 0.2f, 0.05f), 0.0f, 0.6f);
			Echo(T, 0.72f, 0.3f);
			return T;
		}
		case EVespSon::Esquive:
		{
			const float D = 0.34f;
			FTampon T = Souffle(D, H, [](float t) { return 700.0f + 900.0f * t / 0.34f; },
			                    [&H](float t) { return Cloche(t, 0.3f) * (0.7f + 0.3f * FMath::Sin(t * DEUX_PI * 28.0f)); }, 1.2f);
			Ajouter(T, Boum(90.0f, 60.0f, 0.08f, 0.03f), 0.28f, 0.35f);		// l'appui au sol
			return T;
		}
		case EVespSon::Blessure:
		{
			FTampon T = Boum(120.0f, 60.0f, 0.4f, 0.12f);
			FPasseBas Bas;
			FTampon Dissonance = Vide(0.4f);
			float P1 = 0.0f, P2 = 0.0f;
			for (int32 i = 0; i < Dissonance.Num(); i++)
			{
				const float t = float(i) / FREQ;
				P1 += DEUX_PI * 185.0f / FREQ;
				P2 += DEUX_PI * 196.0f / FREQ;
				Dissonance[i] = (FMath::Sin(P1) + FMath::Sin(P2)) * Enveloppe(t, 0.005f, 0.14f) * 0.3f + Bas.Traiter(H.Bruit(), 600.0f) * Enveloppe(t, 0.001f, 0.06f);
			}
			Ajouter(T, Dissonance, 0.0f, 1.0f);
			Saturer(T, 1.5f);
			return T;
		}
		case EVespSon::Os:
		{
			// Des os qui s'entrechoquent et tombent, et l'ame qui s'echappe
			FTampon T = Vide(1.0f);
			const int32 Nombre = 12 + int32(H.Uni() * 8);
			for (int32 k = 0; k < Nombre; k++)
			{
				const float Debut = FMath::Pow(H.Uni(), 1.6f) * 0.6f;
				const float Centre = H.Entre(1600.0f, 4200.0f);
				FResonant R;
				FTampon Clic = Vide(0.05f);
				for (int32 i = 0; i < Clic.Num(); i++) Clic[i] = R.Traiter(H.Bruit(), Centre, 6.0f) * Enveloppe(float(i) / FREQ, 0.0005f, 0.008f);
				Ajouter(T, Clic, Debut, H.Entre(0.4f, 1.0f) * (1.0f - Debut));
				if (H.Uni() < 0.3f)
				{
					Ajouter(T, Boum(H.Entre(300.0f, 600.0f), 250.0f, 0.05f, 0.02f), Debut, 0.3f);
				}
			}
			Ajouter(T, Souffle(0.9f, H, [](float t) { return 1200.0f - 900.0f * t / 0.9f; }, [](float t) { return Cloche(t, 0.9f); }, 3.0f), 0.05f, 0.25f);
			return T;
		}
		case EVespSon::Tir:
		{
			FTampon T = Corde(H.Entre(105.0f, 140.0f), 0.3f, H);
			for (int32 i = 0; i < T.Num(); i++) T[i] *= Enveloppe(float(i) / FREQ, 0.001f, 0.08f);
			Ajouter(T, Souffle(0.18f, H, [](float t) { return 2200.0f - 1400.0f * t / 0.18f; }, [](float t) { return Cloche(t, 0.18f); }, 2.0f), 0.02f, 0.5f);
			return T;
		}
		case EVespSon::Sort:
		case EVespSon::Pouvoir:
		{
			const bool bPouvoir = Son == EVespSon::Pouvoir;
			const float F = bPouvoir ? H.Entre(330.0f, 392.0f) : H.Entre(660.0f, 880.0f);
			FTampon T = Clochette(F, 0.8f, 3.0f);
			Ajouter(T, Clochette(F * 1.5f, 0.7f, 1.5f), 0.05f, 0.4f);
			Ajouter(T, Clochette(F * 2.0f, 0.6f, 1.0f), 0.1f, 0.3f);
			Ajouter(T, Souffle(0.6f, H, [](float t) { return 3000.0f + 2000.0f * t; }, [](float t) { return Cloche(t, 0.6f); }, 1.0f), 0.0f, 0.18f);
			if (bPouvoir)
			{
				Ajouter(T, Boum(70.0f, 38.0f, 0.8f, 0.4f), 0.0f, 1.2f);
			}
			Echo(T, 0.8f, 0.35f);
			return T;
		}
		case EVespSon::Explosion:
		{
			FTampon T = Vide(1.6f);
			FPasseBas Bas;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				T[i] = Bas.Traiter(H.Bruit(), 150.0f + 5000.0f * FMath::Exp(-t * 3.5f)) * Enveloppe(t, 0.004f, 0.45f) * 2.2f;
				if (t < 0.8f && H.Uni() < 0.0015f)
				{
					T[i] += H.Bruit() * 0.8f * (1.0f - t);			// les crepitements
				}
			}
			Ajouter(T, Boum(60.0f, 32.0f, 1.0f, 0.35f), 0.0f, 1.2f);
			Saturer(T, 2.0f);
			Echo(T, 0.75f, 0.25f);
			return T;
		}
		case EVespSon::Soin:
		{
			FTampon T = Vide(1.1f);
			const float Notes[3] = {84.0f, 88.0f, 91.0f};		// do, mi, sol
			for (int32 k = 0; k < 3; k++)
			{
				Ajouter(T, Clochette(Note(Notes[k]), 0.8f, 0.8f, 2.0f), k * 0.08f, 0.35f);
			}
			Echo(T, 0.8f, 0.4f);
			return T;
		}
		case EVespSon::Potion:
		{
			FTampon T = Vide(0.8f);
			for (int32 k = 0; k < 5; k++)
			{
				FTampon Bulle = Vide(0.06f);
				float P = 0.0f;
				const float De = H.Entre(250.0f, 350.0f);
				for (int32 i = 0; i < Bulle.Num(); i++)
				{
					const float t = float(i) / FREQ;
					P += DEUX_PI * (De + 450.0f * t / 0.06f) / FREQ;
					Bulle[i] = FMath::Sin(P) * Cloche(t, 0.06f);
				}
				Ajouter(T, Bulle, k * 0.08f + H.Entre(0.0f, 0.02f), 0.6f);
			}
			Ajouter(T, Clochette(Note(88.0f), 0.4f, 0.6f, 2.0f), 0.42f, 0.3f);
			return T;
		}
		case EVespSon::Ramasser:
		{
			FTampon T = Vide(0.45f);
			Ajouter(T, Clochette(Note(88.0f), 0.25f, 0.5f, 2.0f), 0.0f, 0.6f);
			Ajouter(T, Clochette(Note(93.0f), 0.35f, 0.5f, 2.0f), 0.07f, 0.6f);
			Echo(T, 0.7f, 0.25f);
			return T;
		}
		case EVespSon::Eclats:
		{
			FTampon T = Vide(0.6f);
			const int32 Nombre = 5 + int32(H.Uni() * 4);
			for (int32 k = 0; k < Nombre; k++)
			{
				Ajouter(T, Clochette(H.Entre(2000.0f, 4500.0f), H.Entre(0.1f, 0.2f), 0.4f, 2.4f), H.Entre(0.0f, 0.25f), H.Entre(0.3f, 0.6f));
			}
			Echo(T, 0.7f, 0.2f);
			return T;
		}
		case EVespSon::Niveau:
		{
			FTampon T = Vide(2.6f);
			const float Arpege[5] = {72.0f, 76.0f, 79.0f, 84.0f, 88.0f};
			for (int32 k = 0; k < 5; k++)
			{
				Ajouter(T, Clochette(Note(Arpege[k]), 1.2f, 1.5f, 2.0f), k * 0.1f, 0.45f);
			}
			// Une nappe (un accord qui gonfle)
			FTampon Nappe = Vide(2.4f);
			const float Accord[3] = {60.0f, 64.0f, 67.0f};
			for (int32 k = 0; k < 3; k++)
			{
				float P = 0.0f;
				FPasseBas Bas;
				for (int32 i = 0; i < Nappe.Num(); i++)
				{
					const float t = float(i) / FREQ;
					P += DEUX_PI * Note(Accord[k]) / FREQ;
					const float Scie = FMath::Fmod(P / DEUX_PI, 1.0f) * 2.0f - 1.0f;
					Nappe[i] += Bas.Traiter(Scie, 1200.0f) * Enveloppe(t, 0.4f, 0.9f) * 0.3f;
				}
			}
			Ajouter(T, Nappe, 0.0f, 0.5f);
			Echo(T, 0.85f, 0.45f);
			return T;
		}
		case EVespSon::Rune:
		case EVespSon::Victoire:
		{
			const bool bVictoire = Son == EVespSon::Victoire;
			FTampon T = Vide(1.8f);
			const float Accord1[4] = {57.0f, 60.0f, 64.0f, 74.0f};		// la mineur, et un re en haut
			const float Accord2[4] = {53.0f, 57.0f, 60.0f, 65.0f};		// fa majeur
			const float Accord3[4] = {48.0f, 55.0f, 64.0f, 72.0f};		// do majeur
			auto Pad = [&T](const float* Notes, float Debut, float Duree, float Gain) {
				for (int32 k = 0; k < 4; k++)
				{
					FTampon N = Vide(Duree);
					float P1 = 0.0f, P2 = 0.0f;
					for (int32 i = 0; i < N.Num(); i++)
					{
						const float t = float(i) / FREQ;
						P1 += DEUX_PI * Note(Notes[k]) / FREQ;
						P2 += DEUX_PI * Note(Notes[k] + 0.08f) / FREQ;
						N[i] = (FMath::Sin(P1) + FMath::Sin(P2)) * Enveloppe(t, 0.2f, Duree * 0.5f) * 0.25f;
					}
					Ajouter(T, N, Debut, Gain);
				}
			};
			if (bVictoire)
			{
				Pad(Accord2, 0.0f, 0.8f, 0.6f);
				Pad(Accord3, 0.5f, 1.2f, 0.7f);
				Ajouter(T, Clochette(Note(84.0f), 1.0f, 1.2f, 2.0f), 0.5f, 0.3f);
			}
			else
			{
				Pad(Accord1, 0.0f, 1.6f, 0.7f);
				Ajouter(T, Clochette(Note(86.0f), 1.0f, 1.5f, 2.0f), 0.15f, 0.3f);
			}
			Echo(T, 0.85f, 0.5f);
			return T;
		}
		case EVespSon::Clic:
		{
			FTampon T = Vide(0.05f);
			float P1 = 0.0f, P2 = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				P1 += DEUX_PI * 2200.0f / FREQ;
				P2 += DEUX_PI * 3300.0f / FREQ;
				T[i] = FMath::Sin(P1) * Enveloppe(t, 0.0005f, 0.008f) + FMath::Sin(P2) * Enveloppe(t, 0.0005f, 0.004f) * 0.5f;
			}
			return T;
		}
		case EVespSon::Survol:
		{
			FTampon T = Vide(0.04f);
			float P = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				P += DEUX_PI * 1600.0f / FREQ;
				T[i] = FMath::Sin(P) * Enveloppe(float(i) / FREQ, 0.001f, 0.006f);
			}
			return T;
		}
		case EVespSon::Barriere:
		{
			FTampon T = Vide(1.8f);
			FPasseBas Bas;
			float P = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				P += DEUX_PI * 55.0f / FREQ;
				const float Scie = FMath::Fmod(P / DEUX_PI, 1.0f) * 2.0f - 1.0f;
				T[i] = Bas.Traiter(Scie, 400.0f) * Enveloppe(t, 0.3f, 0.7f);
			}
			Ajouter(T, Souffle(1.0f, H, [](float t) { return 300.0f + 2500.0f * t; }, [](float t) { return Cloche(t, 1.0f); }, 2.0f), 0.0f, 0.4f);
			const float Penta[5] = {81.0f, 84.0f, 86.0f, 88.0f, 91.0f};
			for (int32 k = 0; k < 4; k++)
			{
				Ajouter(T, Clochette(Note(Penta[int32(H.Uni() * 4.99f)]), 0.8f, 1.0f, 2.0f), 0.2f + k * 0.12f, 0.15f);
			}
			Echo(T, 0.85f, 0.45f);
			return T;
		}
		case EVespSon::Rugissement:
		{
			FTampon T = Vide(1.8f);
			FResonant F1, F2, F3;
			float P = 0.0f;
			const float Base = H.Entre(52.0f, 68.0f);
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				const float F = Base * (1.0f + 0.06f * FMath::Sin(t * DEUX_PI * 6.0f) + 0.2f * FMath::Exp(-t * 3.0f)) + H.Bruit() * 4.0f;
				P += DEUX_PI * F / FREQ;
				const float Source = (FMath::Fmod(P / DEUX_PI, 1.0f) * 2.0f - 1.0f) * 0.7f + H.Bruit() * 0.5f;
				const float Voix = F1.Traiter(Source, 420.0f, 5.0f) + F2.Traiter(Source, 950.0f, 6.0f) * 0.7f + F3.Traiter(Source, 2400.0f, 7.0f) * 0.4f;
				T[i] = Voix * Enveloppe(t, 0.15f, 0.9f);
			}
			Saturer(T, 2.5f);
			Echo(T, 0.75f, 0.2f);
			return T;
		}
		case EVespSon::Annonce:
		{
			FTampon T = Vide(0.6f);
			float P = 0.0f, P2 = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				P += DEUX_PI * (90.0f + 140.0f * t / 0.6f) / FREQ;
				P2 += DEUX_PI * (1200.0f + 600.0f * t / 0.6f) / FREQ;
				const float Trem = 0.6f + 0.4f * FMath::Sin(t * DEUX_PI * 18.0f);
				T[i] = (FMath::Sin(P) * Trem + FMath::Sin(P2) * 0.08f) * FMath::Min(1.0f, t / 0.05f) * FMath::Min(1.0f, (0.6f - t) / 0.1f);
			}
			return T;
		}
		case EVespSon::Tonnerre:
		{
			FTampon T = Vide(4.0f);
			FPasseBas Bas, Bas2;
			float Brun = 0.0f;
			float Relief = 1.0f, Cible = 1.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float t = float(i) / FREQ;
				if (i % 2000 == 0) Cible = H.Entre(0.3f, 1.0f);
				Relief += (Cible - Relief) * 0.0008f;
				Brun = FMath::Clamp(Brun + H.Bruit() * 0.05f, -1.0f, 1.0f);
				T[i] = Bas2.Traiter(Bas.Traiter(Brun, 380.0f), 380.0f) * Relief * Enveloppe(t, 0.05f, 1.4f) * 3.0f;
				T[i] += H.Bruit() * Enveloppe(t, 0.001f, 0.03f) * 0.6f;		// le claquement
			}
			Echo(T, 0.88f, 0.5f);
			return T;
		}
		case EVespSon::Coffre:
		{
			FTampon T = Vide(1.3f);
			FResonant R;
			float P = 0.0f;
			FTampon Grincement = Vide(0.45f);
			for (int32 i = 0; i < Grincement.Num(); i++)
			{
				const float t = float(i) / FREQ;
				P += DEUX_PI * (85.0f + 20.0f * FMath::Sin(t * 23.0f)) / FREQ;
				const float Scie = FMath::Fmod(P / DEUX_PI, 1.0f) * 2.0f - 1.0f;
				Grincement[i] = R.Traiter(Scie, 1000.0f + 300.0f * t, 8.0f) * Cloche(t, 0.45f);
			}
			Ajouter(T, Grincement, 0.0f, 0.5f);
			Ajouter(T, Boum(140.0f, 80.0f, 0.2f, 0.06f), 0.45f, 0.8f);
			Ajouter(T, Clochette(Note(84.0f), 0.8f, 1.0f, 2.0f), 0.55f, 0.3f);
			Ajouter(T, Clochette(Note(91.0f), 0.8f, 1.0f, 2.0f), 0.65f, 0.25f);
			Echo(T, 0.8f, 0.35f);
			return T;
		}
		case EVespSon::Glace:
		{
			FTampon T = Vide(0.6f);
			for (int32 k = 0; k < 14; k++)
			{
				Ajouter(T, Clochette(H.Entre(3000.0f, 6000.0f), 0.08f, 0.5f, 2.7f), H.Entre(0.0f, 0.3f), H.Entre(0.2f, 0.5f));
			}
			FPasseBas Bas;
			for (int32 i = 0; i < T.Num(); i++)
			{
				const float B = H.Bruit();
				T[i] += (B - Bas.Traiter(B, 3000.0f)) * Enveloppe(float(i) / FREQ, 0.001f, 0.05f) * 0.4f;
			}
			Echo(T, 0.7f, 0.25f);
			return T;
		}
		case EVespSon::Feu:
		{
			FTampon T = Souffle(0.6f, H, [](float t) { return 500.0f; }, [](float t) { return Cloche(t, 0.6f) * 0.6f; }, 0.7f);
			for (int32 k = 0; k < 10; k++)
			{
				FResonant R;
				FTampon Clic = Vide(0.03f);
				const float C = H.Entre(1500.0f, 3200.0f);
				for (int32 i = 0; i < Clic.Num(); i++) Clic[i] = R.Traiter(H.Bruit(), C, 4.0f) * Enveloppe(float(i) / FREQ, 0.0005f, 0.006f);
				Ajouter(T, Clic, H.Entre(0.0f, 0.5f), H.Entre(0.3f, 0.8f));
			}
			return T;
		}
		default:
			return Vide(0.05f);
	}
}

// ===================== Les ambiances et la musique =====================

static FTampon FabriquerAmbiance(EVespAmbiance A)
{
	const float Duree = 30.0f;
	FTampon T = Vide(Duree + 2.0f);
	FHasard H(4242 + (uint32)A);
	// Le vent : un bruit doux qui ondule lentement
	const float ForceVent = A == EVespAmbiance::Vent ? 1.0f : (A == EVespAmbiance::Voile ? 0.3f : 0.45f);
	{
		FResonant R;
		for (int32 i = 0; i < T.Num(); i++)
		{
			const float t = float(i) / FREQ;
			const float Centre = 380.0f + 260.0f * FMath::Sin(t * DEUX_PI / Duree * 3.0f) + 120.0f * FMath::Sin(t * DEUX_PI / Duree * 7.0f);
			const float Rafale = 0.55f + 0.3f * FMath::Sin(t * DEUX_PI / Duree * 2.0f) + 0.15f * FMath::Sin(t * DEUX_PI / Duree * 5.0f);
			T[i] += R.Traiter(H.Bruit(), Centre, 1.2f) * Rafale * ForceVent * 0.5f;
		}
	}
	switch (A)
	{
		case EVespAmbiance::Foret:
		{
			// Les grillons : des trilles aigus, par petits groupes
			const float Voix[3] = {4100.0f, 4600.0f, 3800.0f};
			for (int32 v = 0; v < 3; v++)
			{
				for (float t = H.Entre(0.0f, 1.0f); t < Duree; t += H.Entre(0.6f, 1.3f))
				{
					for (int32 p = 0; p < 4; p++)
					{
						FTampon Pulse = Vide(0.02f);
						float P = 0.0f;
						for (int32 i = 0; i < Pulse.Num(); i++)
						{
							P += DEUX_PI * Voix[v] / FREQ;
							Pulse[i] = FMath::Sin(P) * Cloche(float(i) / FREQ, 0.02f);
						}
						Ajouter(T, Pulse, t + p * 0.032f, 0.05f);
					}
				}
			}
			// Une chouette, de temps en temps
			for (float t = 5.0f; t < Duree - 2.0f; t += H.Entre(9.0f, 14.0f))
			{
				for (int32 n = 0; n < 2; n++)
				{
					FTampon Hou = Vide(n == 0 ? 0.3f : 0.55f);
					float P = 0.0f;
					for (int32 i = 0; i < Hou.Num(); i++)
					{
						const float u = float(i) / FREQ;
						P += DEUX_PI * (390.0f - 20.0f * u + 4.0f * FMath::Sin(u * 30.0f)) / FREQ;
						Hou[i] = FMath::Sin(P) * Cloche(u, Hou.Num() / float(FREQ));
					}
					Ajouter(T, Hou, t + n * 0.45f, 0.08f);
				}
			}
			break;
		}
		case EVespAmbiance::Marais:
		{
			// Les grenouilles : des coassements graves, par series
			for (float t = H.Entre(0.0f, 1.0f); t < Duree; t += H.Entre(0.8f, 2.5f))
			{
				const int32 Nombre = 3 + int32(H.Uni() * 6);
				const float F = H.Entre(150.0f, 230.0f);
				for (int32 k = 0; k < Nombre; k++)
				{
					FTampon Croa = Vide(0.07f);
					float Pc = 0.0f, Pm = 0.0f;
					for (int32 i = 0; i < Croa.Num(); i++)
					{
						Pm += DEUX_PI * F * 0.5f / FREQ;
						Pc += DEUX_PI * F / FREQ;
						Croa[i] = FMath::Sin(Pc + 4.0f * FMath::Sin(Pm)) * Cloche(float(i) / FREQ, 0.07f);
					}
					Ajouter(T, Croa, t + k * 0.085f, 0.07f);
				}
			}
			// Des bulles
			for (float t = 0.5f; t < Duree; t += H.Entre(0.4f, 1.6f))
			{
				FTampon Bulle = Vide(0.05f);
				float P = 0.0f;
				const float De = H.Entre(180.0f, 320.0f);
				for (int32 i = 0; i < Bulle.Num(); i++)
				{
					const float u = float(i) / FREQ;
					P += DEUX_PI * (De + 400.0f * u / 0.05f) / FREQ;
					Bulle[i] = FMath::Sin(P) * Cloche(u, 0.05f);
				}
				Ajouter(T, Bulle, t, 0.06f);
			}
			break;
		}
		case EVespAmbiance::Braises:
		{
			FPasseBas Bas, Bas2;
			float Brun = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				Brun = FMath::Clamp(Brun + H.Bruit() * 0.04f, -1.0f, 1.0f);
				T[i] += Bas2.Traiter(Bas.Traiter(Brun, 110.0f), 110.0f) * 1.2f;		// le grondement de la terre
			}
			for (float t = 0.0f; t < Duree; t += H.Entre(0.05f, 0.35f))
			{
				FResonant R;
				FTampon Clic = Vide(0.02f);
				const float C = H.Entre(1500.0f, 3500.0f);
				for (int32 i = 0; i < Clic.Num(); i++) Clic[i] = R.Traiter(H.Bruit(), C, 4.0f) * Enveloppe(float(i) / FREQ, 0.0005f, 0.005f);
				Ajouter(T, Clic, t, H.Entre(0.03f, 0.12f));
			}
			break;
		}
		case EVespAmbiance::Voile:
		{
			// Un bourdonnement qui bat lentement, et des notes de verre, tres loin
			float P1 = 0.0f, P2 = 0.0f, P3 = 0.0f;
			for (int32 i = 0; i < T.Num(); i++)
			{
				P1 += DEUX_PI * 55.0f / FREQ;
				P2 += DEUX_PI * 55.35f / FREQ;
				P3 += DEUX_PI * 82.5f / FREQ;
				T[i] += (FMath::Sin(P1) + FMath::Sin(P2) + FMath::Sin(P3) * 0.5f) * 0.12f;
			}
			const float Ton[6] = {84.0f, 86.0f, 88.0f, 90.0f, 92.0f, 94.0f};
			for (float t = 1.0f; t < Duree; t += H.Entre(2.0f, 4.5f))
			{
				Ajouter(T, Clochette(Note(Ton[int32(H.Uni() * 5.99f)]), 3.0f, 0.6f, 2.7f), t, 0.06f);
			}
			break;
		}
		default: break;
	}
	Echo(T, 0.8f, 0.25f);
	Boucler(T, Duree);
	return T;
}

static FTampon FabriquerPluie()
{
	const float Duree = 12.0f;
	FTampon T = Vide(Duree + 1.0f);
	FHasard H(777);
	FPasseBas Bas;
	for (int32 i = 0; i < T.Num(); i++)
	{
		const float B = H.Bruit();
		T[i] = (B - Bas.Traiter(B, 900.0f)) * 0.3f;
		if (H.Uni() < 0.012f)
		{
			T[i] += H.Bruit() * 0.5f;		// une goutte plus lourde
		}
	}
	FPasseBas Doux;
	for (float& X : T) X = Doux.Traiter(X, 6000.0f);
	Boucler(T, Duree);
	return T;
}

// La musique de chaque acte : quatre accords qui s'enchainent lentement (une nappe), et une cloche qui chante
// quelques notes de sa gamme. Tambours : la meme boucle, rythmee, pour les combats.
struct FVespPartition
{
	float Accords[4][4];
	float Gamme[6];
	float Tempo;
	float Coupure;			// la nappe : plus sombre (bas) ou plus claire (haut)
};

static const FVespPartition PARTITIONS[7] = {
	{{{50, 57, 60, 65}, {46, 53, 57, 62}, {53, 57, 60, 65}, {48, 55, 62, 64}}, {62, 65, 67, 69, 72, 74}, 76.0f, 900.0f},		// I : re dorien, calme
	{{{45, 52, 57, 60}, {41, 48, 57, 60}, {50, 53, 57, 62}, {40, 52, 56, 59}}, {69, 72, 74, 76, 79, 81}, 84.0f, 700.0f},		// II : la mineur, inquietant
	{{{40, 47, 52, 55}, {41, 48, 53, 57}, {40, 47, 52, 55}, {40, 50, 53, 57}}, {64, 65, 67, 71, 72, 76}, 72.0f, 550.0f},		// III : mi phrygien, trouble
	{{{36, 48, 51, 55}, {44, 48, 51, 56}, {46, 50, 53, 58}, {43, 50, 55, 59}}, {60, 63, 65, 67, 70, 72}, 104.0f, 800.0f},		// IV : do mineur, martial
	{{{54, 61, 64, 68}, {50, 57, 61, 66}, {57, 61, 64, 69}, {52, 59, 64, 66}}, {78, 81, 83, 85, 88, 90}, 88.0f, 1400.0f},		// V : fa diese mineur, froid et clair
	{{{47, 54, 59, 62}, {43, 50, 55, 59}, {49, 55, 58, 61}, {42, 54, 58, 61}}, {71, 74, 76, 78, 81, 83}, 112.0f, 750.0f},	// VI : si mineur, intense
	{{{48, 52, 56, 60}, {50, 54, 58, 62}, {52, 56, 60, 64}, {46, 50, 54, 58}}, {72, 74, 76, 78, 80, 82}, 96.0f, 900.0f},		// VII : par tons, etrange
};

static const float DUREE_MUSIQUE = 48.0f;

static FTampon FabriquerMusique(int32 Acte)
{
	const FVespPartition& P = PARTITIONS[FMath::Clamp(Acte, 1, 7) - 1];
	FTampon T = Vide(DUREE_MUSIQUE + 6.0f);
	FHasard H(1000 + Acte);
	const float DureeAccord = DUREE_MUSIQUE / 4.0f;
	for (int32 a = 0; a < 4; a++)
	{
		for (int32 n = 0; n < 4; n++)
		{
			// Chaque note : deux dents de scie un peu desaccordees, adoucies, qui arrivent et repartent lentement
			FTampon Voix = Vide(DureeAccord + 4.0f);
			float P1 = 0.0f, P2 = 0.0f, Ps = 0.0f;
			FPasseBas Bas, Bas2;
			const float F = Note(P.Accords[a][n]);
			for (int32 i = 0; i < Voix.Num(); i++)
			{
				const float t = float(i) / FREQ;
				P1 += DEUX_PI * F / FREQ;
				P2 += DEUX_PI * F * 1.0035f / FREQ;
				Ps += DEUX_PI * F * 0.5f / FREQ;
				const float Scie = (FMath::Fmod(P1 / DEUX_PI, 1.0f) + FMath::Fmod(P2 / DEUX_PI, 1.0f)) - 1.0f;
				const float Coupure = P.Coupure * (0.8f + 0.3f * FMath::Sin(t * 0.6f + n));
				const float Forme = FMath::Min(1.0f, t / 2.5f) * FMath::Min(1.0f, FMath::Max(0.0f, (DureeAccord + 4.0f - t) / 3.5f));
				Voix[i] = (Bas2.Traiter(Bas.Traiter(Scie, Coupure), Coupure) + (n == 0 ? FMath::Sin(Ps) * 0.5f : 0.0f)) * Forme;
			}
			Ajouter(T, Voix, a * DureeAccord - 1.0f, n == 0 ? 0.3f : 0.18f);
		}
	}
	// La cloche : quelques notes, rares
	for (float t = 2.0f; t < DUREE_MUSIQUE; t += H.Entre(1.5f, 3.5f))
	{
		if (H.Uni() < 0.35f)
		{
			continue;
		}
		Ajouter(T, Clochette(Note(P.Gamme[int32(H.Uni() * 5.99f)]), 2.5f, 1.2f, 2.0f), t, 0.1f);
	}
	Echo(T, 0.88f, 0.55f);
	Boucler(T, DUREE_MUSIQUE);
	return T;
}

static FTampon FabriquerTambours(int32 Acte)
{
	const FVespPartition& P = PARTITIONS[FMath::Clamp(Acte, 1, 7) - 1];
	FTampon T = Vide(DUREE_MUSIQUE + 2.0f);
	FHasard H(2000 + Acte);
	const int32 Temps = FMath::Max(16, FMath::RoundToInt(DUREE_MUSIQUE * P.Tempo / 60.0f / 4.0f) * 4);
	const float Battement = DUREE_MUSIQUE / Temps;
	for (int32 b = 0; b < Temps; b++)
	{
		const float t = b * Battement;
		const int32 Place = b % 8;
		// La grosse caisse (un tambour grave) sur les temps forts, un tambour plus aigu en contretemps
		if (Place == 0 || Place == 3 || Place == 4 || (Place == 6 && Acte >= 4))
		{
			Ajouter(T, Boum(95.0f, 48.0f, 0.4f, 0.16f), t, Place == 0 ? 1.0f : 0.75f);
		}
		if (Place == 2 || Place == 6)
		{
			FTampon Tom = Boum(180.0f, 120.0f, 0.25f, 0.08f);
			FPasseBas Bas;
			for (int32 i = 0; i < Tom.Num(); i++) Tom[i] += Bas.Traiter(H.Bruit(), 1200.0f) * Enveloppe(float(i) / FREQ, 0.001f, 0.04f) * 0.8f;
			Ajouter(T, Tom, t, 0.55f);
		}
		// Un petit tambour sur chaque demi-temps (plus present dans les actes guerriers)
		FTampon Tic = Vide(0.06f);
		FResonant R;
		for (int32 i = 0; i < Tic.Num(); i++) Tic[i] = R.Traiter(H.Bruit(), 2500.0f, 2.0f) * Enveloppe(float(i) / FREQ, 0.0005f, 0.015f);
		Ajouter(T, Tic, t + Battement * 0.5f, Acte >= 4 ? 0.25f : 0.12f);
	}
	Saturer(T, 1.4f);
	Echo(T, 0.7f, 0.2f);
	Boucler(T, DUREE_MUSIQUE);
	return T;
}

// ===================== Le cache des sons =====================

static constexpr int32 VARIANTES = 3;

static TSharedPtr<TArray<uint8>> Son(EVespSon S, int32 Variante)
{
	static TMap<int32, TSharedPtr<TArray<uint8>>> Cache;
	const int32 Cle = (int32)S * 16 + Variante;
	if (TSharedPtr<TArray<uint8>>* Deja = Cache.Find(Cle))
	{
		return *Deja;
	}
	// Le volume relatif de chaque son (les pas discrets, les explosions fortes)
	float Gain = 0.8f;
	switch (S)
	{
		case EVespSon::Pas: Gain = 0.35f; break;
		case EVespSon::Survol: Gain = 0.25f; break;
		case EVespSon::Clic: Gain = 0.4f; break;
		case EVespSon::Frappe: Gain = 0.55f; break;
		case EVespSon::Esquive: Gain = 0.5f; break;
		case EVespSon::Explosion: case EVespSon::Tonnerre: case EVespSon::Rugissement: Gain = 1.0f; break;
		default: break;
	}
	TSharedPtr<TArray<uint8>> O = EnOctets(Fabriquer(S, 17 + Variante * 101), Gain);
	Cache.Add(Cle, O);
	return O;
}

// ===================== La lecture =====================

USoundWaveProcedural* UVespSons::Creer(const TSharedPtr<TArray<uint8>>& Donnees, bool bBoucle)
{
	USoundWaveProcedural* W = NewObject<USoundWaveProcedural>(this);
	W->SetSampleRate(FREQ);
	W->NumChannels = 1;
	W->Duration = bBoucle ? INDEFINITELY_LOOPING_DURATION : Donnees->Num() / (2.0f * FREQ);
	W->SoundGroup = SOUNDGROUP_Default;
	W->bLooping = bBoucle;
	W->QueueAudio(Donnees->GetData(), Donnees->Num());
	if (bBoucle)
	{
		TSharedPtr<TArray<uint8>> Garde = Donnees;
		W->OnSoundWaveProceduralUnderflow = FOnSoundWaveProceduralUnderflow::CreateLambda([Garde](USoundWaveProcedural* Onde, int32)
		{
			Onde->QueueAudio(Garde->GetData(), Garde->Num());		// la boucle recommence
		});
	}
	return W;
}

void UVespSons::Lancer(EVespSon S, const FVector* Position, float Volume, float Hauteur)
{
	UWorld* Monde = GetWorld();
	if (!Monde || Monde->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (VolumeEffets <= 0.001f)
	{
		return;
	}
	Volume *= VolumeEffets;
	const double Maintenant = FPlatformTime::Seconds();
	double& Derniere = DerniereFois.FindOrAdd((uint8)S);
	if (S != EVespSon::Pas && Maintenant - Derniere < 0.035)
	{
		return;			// le meme son, dans la meme image : une seule fois
	}
	Derniere = Maintenant;
	if (Vivants.Num() >= 40)
	{
		return;
	}
	if (!Attenuation)
	{
		Attenuation = NewObject<USoundAttenuation>(this);
		FSoundAttenuationSettings& R = Attenuation->Attenuation;
		R.bAttenuate = true;
		R.bSpatialize = true;
		R.AttenuationShape = EAttenuationShape::Sphere;
		R.AttenuationShapeExtents = FVector(500.0f, 0.0f, 0.0f);
		R.FalloffDistance = 3200.0f;
		R.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	}
	USoundWaveProcedural* W = Creer(Son(S, FMath::RandRange(0, VARIANTES - 1)), false);
	const float Pitch = Hauteur * FMath::FRandRange(0.95f, 1.05f);
	if (Position)
	{
		UGameplayStatics::PlaySoundAtLocation(Monde, W, *Position, Volume, Pitch, 0.0f, Attenuation);
	}
	else
	{
		UGameplayStatics::PlaySound2D(Monde, W, Volume, Pitch);
	}
	Vivants.Add(W);
	FinsDesVivants.Add(Maintenant + W->Duration / FMath::Max(0.1f, Pitch) + 0.5);
}

void UVespSons::Jouer(const UObject* Contexte, EVespSon S, const FVector& Position, float Volume, float Hauteur)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (UVespSons* Sons = Monde ? Monde->GetSubsystem<UVespSons>() : nullptr)
	{
		Sons->Lancer(S, &Position, Volume, Hauteur);
	}
}

void UVespSons::Jouer2D(const UObject* Contexte, EVespSon S, float Volume, float Hauteur)
{
	UWorld* Monde = Contexte ? Contexte->GetWorld() : nullptr;
	if (UVespSons* Sons = Monde ? Monde->GetSubsystem<UVespSons>() : nullptr)
	{
		Sons->Lancer(S, nullptr, Volume, Hauteur);
	}
}

void UVespSons::PlacerOreille(APlayerController* Joueur, const FVector& Position, const FRotator& Regard)
{
	if (Joueur)
	{
		Joueur->SetAudioListenerOverride(nullptr, Position, FRotator(0.0f, Regard.Yaw, 0.0f));
	}
}

UAudioComponent* UVespSons::Boucle(const TSharedPtr<TArray<uint8>>& Donnees, float Volume)
{
	UAudioComponent* C = UGameplayStatics::CreateSound2D(GetWorld(), Creer(Donnees, true), Volume, 1.0f, 0.0f, nullptr, true, false);
	if (C)
	{
		C->bIsUISound = false;
		C->FadeIn(2.0f, Volume);
	}
	return C;
}

void UVespSons::AmbianceDeLActe(int32 Acte)
{
	if (Acte == ActeJoue)
	{
		return;
	}
	ActeJoue = Acte;
	for (UAudioComponent* C : {Ambiance.Get(), Musique.Get(), Tambours.Get()})
	{
		if (C)
		{
			C->FadeOut(1.5f, 0.0f);
		}
	}
	static const EVespAmbiance AMBIANCES[7] = {EVespAmbiance::Foret, EVespAmbiance::Foret, EVespAmbiance::Marais, EVespAmbiance::Vent,
	                                           EVespAmbiance::Vent, EVespAmbiance::Braises, EVespAmbiance::Voile};
	static TMap<int32, TSharedPtr<TArray<uint8>>> Deja;
	auto Obtenir = [](int32 Cle, TFunction<FTampon()> Faire, float Gain) {
		if (TSharedPtr<TArray<uint8>>* D = Deja.Find(Cle))
		{
			return *D;
		}
		TSharedPtr<TArray<uint8>> O = EnOctets(Faire(), Gain);
		Deja.Add(Cle, O);
		return O;
	};
	const int32 A = FMath::Clamp(Acte, 1, 7);
	Ambiance = Boucle(Obtenir(100 + (int32)AMBIANCES[A - 1], [A]() { return FabriquerAmbiance(AMBIANCES[A - 1]); }, 0.55f), 0.8f);
	Musique = Boucle(Obtenir(200 + A, [A]() { return FabriquerMusique(A); }, 0.5f), 0.45f);
	Tambours = Boucle(Obtenir(300 + A, [A]() { return FabriquerTambours(A); }, 0.6f), 0.001f);
	TensionActuelle = 0.0f;
}

void UVespSons::Tension(float Niveau)
{
	TensionVoulue = FMath::Clamp(Niveau, 0.0f, 1.0f);
}

void UVespSons::Pluie(float Force)
{
	PluieVoulue = FMath::Clamp(Force, 0.0f, 1.0f);
	if (PluieVoulue > 0.0f && !BouclePluie)
	{
		static TSharedPtr<TArray<uint8>> Donnees;
		if (!Donnees)
		{
			Donnees = EnOctets(FabriquerPluie(), 0.6f);
		}
		BouclePluie = Boucle(Donnees, 0.001f);
		PluieActuelle = 0.0f;
	}
}

void UVespSons::CouperTout()
{
	for (UAudioComponent* C : {Ambiance.Get(), Musique.Get(), Tambours.Get(), BouclePluie.Get()})
	{
		if (C)
		{
			C->Stop();
		}
	}
	Ambiance = Musique = Tambours = BouclePluie = nullptr;
	ActeJoue = 0;
}

void UVespSons::Tick(float Secondes)
{
	// Les sons finis sont oublies
	const double Maintenant = FPlatformTime::Seconds();
	for (int32 i = Vivants.Num() - 1; i >= 0; i--)
	{
		if (Maintenant > FinsDesVivants[i])
		{
			Vivants.RemoveAt(i);
			FinsDesVivants.RemoveAt(i);
		}
	}
	// Les tambours montent et descendent en douceur avec la tension ; la musique calme s'efface un peu
	TensionActuelle = FMath::FInterpTo(TensionActuelle, TensionVoulue, Secondes, 0.8f);
	if (Tambours)
	{
		Tambours->SetVolumeMultiplier(FMath::Max(0.001f, TensionActuelle * 0.7f * VolumeMusique));
	}
	if (Musique)
	{
		Musique->SetVolumeMultiplier(FMath::Max(0.001f, 0.45f * (1.0f - TensionActuelle * 0.3f) * VolumeMusique));
	}
	PluieActuelle = FMath::FInterpTo(PluieActuelle, PluieVoulue, Secondes, 0.5f);
	if (BouclePluie)
	{
		BouclePluie->SetVolumeMultiplier(FMath::Max(0.001f, PluieActuelle * 0.9f * VolumeEffets));
	}
	if (Ambiance)
	{
		Ambiance->SetVolumeMultiplier(FMath::Max(0.001f, 0.8f * VolumeEffets));
	}
}

void UVespSons::Deinitialize()
{
	CouperTout();
	Vivants.Reset();
	FinsDesVivants.Reset();
	Super::Deinitialize();
}
