// dialogues.cpp : ceux qui parlent a AYLIS
//
// Un dialogue, c'est une liste de repliques (qui parle, et ce qu'il dit). Le texte s'ecrit peu a peu
// sur un parchemin magique (voir dessinerDialogue dans dessin.cpp). A la derniere replique, il peut y avoir
// 2 reponses a choisir : ces choix-la restent (l'honneur, les compagnons, la fin de la route...).
//
// Les boss parlent avant de se battre, les marchands avant d'ouvrir leur boutique, et ce qu'ils disent
// change avec la memoire : combien de fois AYLIS les a deja affrontes, ce qu'AYLIS a fait dans les visions...
//
// AYLIS n'a pas de genre connu : on ne lui accorde jamais rien (ni "pret", ni "seul"...). On tourne les phrases
// autrement, ou on lui parle a la deuxieme personne sans accord.
// (Pas d'accents dans les textes : la police de raylib ne sait pas les afficher.)
#include "jeu2d.h"

// ===================== Qui parle =====================

std::string nomOrateur(int orateur) {
    switch (orateur) {
        case ORATEUR_PROPHETIE: return "La prophetie";
        case BOSS_SKARN: return "Skarn";
        case BOSS_MATRIARCHE: return "La Matriarche";
        case BOSS_ASHKA: return "Ashka";
        case BOSS_VORGATH: return "Vorgath";
        case ORATEUR_AYLIS: return "AYLIS";
        case ORATEUR_MAREN: return "Maren";
        case ORATEUR_DURGAN: return "Durgan";
        case ORATEUR_SILAS: return "Silas";
        case ORATEUR_DESERTEUR: return "Le deserteur";
        case ORATEUR_KERRAK: return "Kerrak";
        default: return "Brenna";
    }
}

std::string titreOrateur(int orateur) {
    switch (orateur) {
        case ORATEUR_PROPHETIE: return "ce qui doit etre";
        case BOSS_SKARN: return "le Brise-Cranes";
        case BOSS_MATRIARCHE: return "mere des loups";
        case BOSS_ASHKA: return "cheffe de guerre";
        case BOSS_VORGATH: return "le Destructeur";
        case ORATEUR_AYLIS: return "la vision qui marche";
        case ORATEUR_MAREN: return "l'herboriste";
        case ORATEUR_DURGAN: return "le forgeron";
        case ORATEUR_SILAS: return "le collectionneur";
        case ORATEUR_DESERTEUR: return "un Haschen sans arme";
        case ORATEUR_KERRAK: return "le deserteur";
        default: return "la mercenaire";
    }
}

// La couleur de l'encre magique de chaque personnage (le nom, la lettrine, les runes du parchemin)
Color couleurOrateur(int orateur) {
    switch (orateur) {
        case ORATEUR_PROPHETIE: return Color{190, 140, 255, 255};
        case BOSS_SKARN: return Color{255, 110, 70, 255};
        case BOSS_MATRIARCHE: return Color{120, 230, 120, 255};
        case BOSS_ASHKA: return Color{255, 200, 80, 255};
        case BOSS_VORGATH: return Color{255, 70, 40, 255};
        case ORATEUR_AYLIS: return Color{120, 160, 255, 255};
        case ORATEUR_MAREN: return Color{110, 230, 220, 255};
        case ORATEUR_DURGAN: return Color{255, 170, 90, 255};
        case ORATEUR_SILAS: return Color{200, 150, 255, 255};
        case ORATEUR_DESERTEUR:
        case ORATEUR_KERRAK: return Color{255, 176, 154, 255};
        default: return Color{240, 190, 120, 255};
    }
}

// ===================== Le deroulement d'un dialogue =====================

void lancerDialogue(Jeu& jeu, const std::vector<Replique>& repliques, SuiteDialogue suite) {
    jeu.dialogue = Dialogue();
    jeu.dialogue.repliques = repliques;
    jeu.dialogue.suite = suite;
    jeu.phase = Phase::Dialogue;
}

// Les lettres s'ecrivent a la plume : 50 par seconde
void mettreAJourDialogue(Jeu& jeu, float secondes) {
    jeu.dialogue.ecriture = jeu.dialogue.ecriture + 50.0f * secondes;
}

bool texteEcrit(const Dialogue& d) {
    return d.ecriture >= (float)d.repliques[d.ligne].texte.size();
}

bool attendUnChoix(const Jeu& jeu) {
    const Dialogue& d = jeu.dialogue;
    return !d.choix.empty() && d.ligne == (int)d.repliques.size() - 1 && texteEcrit(d);
}

// Le dialogue est fini : on passe a la suite prevue
void terminerDialogue(Jeu& jeu) {
    switch (jeu.dialogue.suite) {
        case SuiteDialogue::Combat: preparerCombat(jeu); break;
        case SuiteDialogue::Boutique: jeu.phase = Phase::Marchand; break;
        case SuiteDialogue::Runes: proposerRunes(jeu, true); break;
        case SuiteDialogue::Route: allerPlusLoin(jeu); break;
        case SuiteDialogue::Seuil: entrerAuSeuil(jeu); break;
    }
}

void avancerDialogue(Jeu& jeu) {
    Dialogue& d = jeu.dialogue;
    if (!texteEcrit(d)) {
        d.ecriture = 9999.0f;       // un premier appui ecrit toute la replique d'un coup
        return;
    }
    if (d.ligne < (int)d.repliques.size() - 1) {
        d.ligne = d.ligne + 1;
        d.ecriture = 0.0f;
        return;
    }
    if (!d.choix.empty()) {
        return;                     // il faut choisir une reponse
    }
    terminerDialogue(jeu);
}

// Apres une reponse, les personnages reagissent : ces repliques remplacent le dialogue, sans choix cette fois
void enchainer(Jeu& jeu, const std::vector<Replique>& reaction) {
    jeu.dialogue.repliques = reaction;
    jeu.dialogue.ligne = 0;
    jeu.dialogue.ecriture = 0.0f;
    jeu.dialogue.choix.clear();
    jeu.dialogue.effets.clear();
    jeu.dialogue.sujet = SUJET_AUCUN;
}

void changerHonneurDialogue(Jeu& jeu, int changement) {
    jeu.aylis.stats.honneur = jeu.aylis.stats.honneur + changement;
}

void choisirDansDialogue(Jeu& jeu, int numero) {
    if (!attendUnChoix(jeu) || numero < 0 || numero > 1) {
        return;
    }
    Memoire& m = jeu.memoire;

    if (jeu.dialogue.sujet == SUJET_DESERTEUR) {
        if (numero == 0) {
            // L'epargner : il rejoint AYLIS (si personne ne l'accompagne deja)
            changerHonneurDialogue(jeu, 1);
            m.deserteurEpargne = m.deserteurEpargne + 1;
            if (jeu.compagnon == COMPAGNON_AUCUN) {
                recruterCompagnon(jeu, COMPAGNON_KERRAK);
                m.kerrakRecrute = m.kerrakRecrute + 1;
                enchainer(jeu, {
                    {ORATEUR_DESERTEUR, "Tu... tu me laisses la vie ? Alors elle est a toi. Je m'appelle Kerrak."},
                    {ORATEUR_KERRAK, "Je connais les routes d'Ashka, ses guetteurs, ses pieges. Laisse-moi marcher avec toi."},
                    {ORATEUR_PROPHETIE, "Kerrak rejoint AYLIS. Il se battra a ses cotes, jusqu'au bout de la route."},
                });
            } else {
                gagnerOr(jeu, 15);
                enchainer(jeu, {
                    {ORATEUR_DESERTEUR, "Merci... Prends ca, c'est tout ce que j'ai. Et mefie-toi d'Ashka : elle a peur de toi."},
                    {ORATEUR_PROPHETIE, "Le Haschen disparait entre les arbres. +15 pieces d'or."},
                });
            }
        } else {
            changerHonneurDialogue(jeu, -1);
            m.deserteurDepouille = m.deserteurDepouille + 1;
            gagnerOr(jeu, 30);
            enchainer(jeu, {
                {ORATEUR_DESERTEUR, "Non... pas ma bourse... C'etait pour fuir loin d'ici..."},
                {ORATEUR_PROPHETIE, "+30 pieces d'or. Le Haschen s'enfuit vers le camp. La prophetie, elle, n'oublie rien."},
            });
        }
    } else if (jeu.dialogue.sujet == SUJET_ASHKA) {
        std::vector<Replique> reaction;
        if (numero == 0) {
            // L'epargner : ses guerriers abandonnent Vorgath
            jeu.choixAshka = 1;
            changerHonneurDialogue(jeu, 2);
            m.ashkaEpargnee = m.ashkaEpargnee + 1;
            reaction = {
                {BOSS_ASHKA, "... Pourquoi ? La prophetie disait qu'une seule de nos deux routes continuerait."},
                {ORATEUR_AYLIS, "Alors la prophetie se trompe. Rentre chez les tiens, Ashka. Ce n'est pas contre toi que je marche."},
                {BOSS_ASHKA, "Mes guerriers ne serviront plus Vorgath. Les Terres de cendre seront moins peuplees... pour toi."},
            };
            if (jeu.compagnon == COMPAGNON_KERRAK) {
                reaction.push_back({ORATEUR_KERRAK, "Cheffe... Je savais qu'on pouvait encore choisir."});
            }
            reaction.push_back({ORATEUR_PROPHETIE, "Honneur +2. Dans l'acte IV, les Haschen seront moins nombreux, et Vorgath n'aura pas d'escorte."});
        } else {
            // L'achever : AYLIS prend sa couronne d'epines (+3 attaque), mais les Haschen veulent la venger
            jeu.choixAshka = 2;
            changerHonneurDialogue(jeu, -2);
            m.ashkaAchevee = m.ashkaAchevee + 1;
            jeu.aylis.stats.attaque = jeu.aylis.stats.attaque + 3;
            reaction = {
                {BOSS_ASHKA, "Alors c'est ainsi... que la prophetie... s'accomplit..."},
                {ORATEUR_PROPHETIE, "AYLIS ramasse la couronne d'epines. Elle pese plus lourd qu'elle ne devrait. +3 attaque."},
            };
            if (jeu.compagnon == COMPAGNON_KERRAK) {
                reaction.push_back({ORATEUR_KERRAK, "Elle etait a terre... Je ne peux pas te suivre plus loin. Adieu."});
                reaction.push_back({ORATEUR_PROPHETIE, "Kerrak s'en va sans se retourner."});
                jeu.compagnon = COMPAGNON_AUCUN;
            } else if (jeu.compagnon == COMPAGNON_BRENNA) {
                reaction.push_back({ORATEUR_BRENNA, "Rapide et propre. Je ne pose pas de questions, on me paie pour ca."});
            }
            reaction.push_back({ORATEUR_PROPHETIE, "Honneur -2. Dans l'acte IV, les Haschen se battront avec rage pour venger leur cheffe."});
        }
        enchainer(jeu, reaction);
    }
    enregistrerMemoire(m);
}

// ===================== Les boss parlent avant le combat =====================

void dialogueAvantBoss(Jeu& jeu) {
    const Memoire& m = jeu.memoire;
    int boss = acte(acteDeLaSalle(jeu.salle)).boss;
    int deja = m.bossAffrontes[boss - 1];       // combien de fois AYLIS l'a deja affronte, dans les visions
    bool vaincu = m.bossVaincus[boss - 1] > 0;
    int honneur = jeu.aylis.stats.honneur;
    std::vector<Replique> r;

    if (boss == BOSS_SKARN) {
        if (deja == 0) {
            r = {{BOSS_SKARN, "Encore une petite vision qui marche vers Karn ? Approche. Le sol se souviendra de toi, "
                              "meme quand ton nom sera perdu."},
                 {ORATEUR_AYLIS, "Le sol, peut-etre. Toi, tu vas oublier."}};
        } else if (vaincu) {
            r = {{BOSS_SKARN, "On raconte que je tombe, dans un autre futur. Les reves mentent, petite vision."},
                 {ORATEUR_AYLIS, "Trois coups, puis la masse. Je sais compter, Skarn."}};
        } else {
            r = {{BOSS_SKARN, "Toi. Encore toi. Combien de fois faudra-t-il que ma masse t'efface ?"},
                 {ORATEUR_AYLIS, "Autant de fois qu'il le faudra. Moi, je me releve. Toi, une seule fois suffira."}};
        }
    } else if (boss == BOSS_MATRIARCHE) {
        if (deja == 0) {
            r = {{BOSS_MATRIARCHE, "Mes loups ont senti ta peur bien avant ton odeur. Ils ont faim, et moi, j'ai le temps."}};
        } else {
            r = {{BOSS_MATRIARCHE, "Tu reviens avec le parfum des herbes de Maren. Crois-tu que ses remedes soignent "
                                   "la morsure d'un loup ?"},
                 {ORATEUR_AYLIS, "Non. Mais je frapperai avant que tu n'appelles."}};
        }
    } else if (boss == BOSS_ASHKA) {
        if (m.ashkaEpargnee > 0) {
            r = {{BOSS_ASHKA, "Toi... Dans un autre futur, tu m'as laisse la vie. Je l'ai senti, cette nuit, dans mes reves. "
                              "Je ne sais pas quoi en faire."}};
        } else if (deja == 0) {
            r = {{BOSS_ASHKA, "Alors c'est toi, l'enfant de la prophetie ? Tu tiens moins de place que dans mes cauchemars."}};
        } else {
            r = {{BOSS_ASHKA, "La route te ramene toujours ici. Mes fleches connaissent ton visage, maintenant."}};
        }
        if (jeu.compagnon == COMPAGNON_KERRAK) {
            r.push_back({BOSS_ASHKA, "Et tu marches avec un traitre. Kerrak... je t'avais cru mort."});
            r.push_back({ORATEUR_KERRAK, "Je l'etais, cheffe. AYLIS m'a rendu la vie. Pas toi."});
        } else if (honneur > 0) {
            r.push_back({BOSS_ASHKA, "Un de mes guerriers a deserte. Il parlait de toi avec respect. Je le ferai bruler."});
        }
        r.push_back({ORATEUR_AYLIS, "Personne ne brulera ce soir, Ashka. Sauf si tu m'y obliges."});
    } else {
        r = {{BOSS_VORGATH, "Tout ce qui brule finit en cendre. Les forets, les villages, les prophetes. Et toi aussi."}};
        if (jeu.choixAshka == 1) {
            r.push_back({BOSS_VORGATH, "Ashka m'a trahi pour toi. Ses guerriers ont deserte mes terres. Elle brulera apres toi."});
            r.push_back({ORATEUR_AYLIS, "Elle a choisi. Comme moi. Tu n'as jamais rien choisi, Vorgath : tu te contentes de bruler."});
        } else if (jeu.choixAshka == 2) {
            r.push_back({BOSS_VORGATH, "Tu portes sa couronne d'epines. Tu me ressembles deja, vision. Plus que tu ne le crois."});
            r.push_back({ORATEUR_AYLIS, "Je ne te ressemble pas. Pas encore."});
        } else {
            r.push_back({ORATEUR_AYLIS, "La prophetie m'a montre ta chute. Je viens seulement la rendre vraie."});
        }
        if (jeu.compagnon == COMPAGNON_BRENNA) {
            r.push_back({ORATEUR_BRENNA, "Un dragon de fer. On ne m'a pas assez payee pour ca... Tant pis. Allons-y."});
        }
    }
    lancerDialogue(jeu, r, SuiteDialogue::Combat);
}

// ===================== Les marchands saluent AYLIS =====================

void dialogueDuMarchand(Jeu& jeu) {
    const Memoire& m = jeu.memoire;
    int honneur = jeu.aylis.stats.honneur;
    std::vector<Replique> r;
    if (jeu.marchand == 0) {
        if (honneur < 0) {
            r = {{ORATEUR_MAREN, "On parle de toi sur la route, et pas en bien. Mes potions soignent les corps... pas le reste."}};
        } else if (m.voyageurAide > 0) {
            r = {{ORATEUR_MAREN, "Un voyageur m'a parle d'une main tendue dans la foret. C'etait toi ? Tiens, "
                                 "je t'ai garde le meilleur."}};
        } else {
            r = {{ORATEUR_MAREN, "Encore une vision ? Assieds-toi, le feu est bon ce soir. J'ai garde ce qu'il faut "
                                 "pour la prochaine... et un peu plus, parce que tu reviens toujours."}};
        }
    } else if (jeu.marchand == 1) {
        r = {{ORATEUR_DURGAN, "Hmpf. Durgan, forgeron. Ton arme a besoin de mes mains, ca se voit d'ici."}};
        if (jeu.compagnon == COMPAGNON_AUCUN) {
            r.push_back({ORATEUR_DURGAN, "Et si tu en as assez de marcher sans personne sur cette route, Brenna cherche du travail. "
                                         "Elle frappe plus fort que moi."});
            r.push_back({ORATEUR_BRENNA, "Quarante pieces, et je te suis jusqu'a Vorgath. Pas une de moins."});
        }
    } else {
        if (m.ashkaAchevee > 0) {
            r = {{ORATEUR_SILAS, "Une couronne d'epines... Tu l'as deja portee, dans un autre futur. Elle t'allait trop bien. "
                                 "Ca m'inquiete."}};
        } else if (honneur >= 2) {
            r = {{ORATEUR_SILAS, "Tant d'honneur, sur une route si sale. Tu vas me faire croire aux prophetes."}};
        } else {
            r = {{ORATEUR_SILAS, "Des runes, des secrets, des trophees... tout se vend. Tout s'achete. Meme l'avenir."}};
        }
    }
    lancerDialogue(jeu, r, SuiteDialogue::Boutique);
}

// ===================== La rencontre du deserteur =====================

void dialogueDuDeserteur(Jeu& jeu) {
    std::vector<Replique> r = {
        {ORATEUR_PROPHETIE, "Un jeune Haschen sans arme tremble derriere un rocher. Il a fui le camp d'Ashka."},
        {ORATEUR_DESERTEUR, "Ne... ne frappe pas. Ashka brule ceux qui doutent de la prophetie. Et moi, je doute."},
    };
    if (jeu.memoire.deserteurEpargne > 0) {
        r.push_back({ORATEUR_DESERTEUR, "Attends... Ton visage. Je l'ai vu en reve. Tu m'avais laisse partir, dans ce reve."});
    }
    r.push_back({ORATEUR_DESERTEUR, "Toi aussi, tu doutes, non ?"});
    lancerDialogue(jeu, r, SuiteDialogue::Route);
    jeu.dialogue.sujet = SUJET_DESERTEUR;
    jeu.dialogue.choix = {"Pars. Et ne reviens jamais au camp.", "Ce que tu portes est a moi, maintenant."};
    jeu.dialogue.effets = {jeu.compagnon == COMPAGNON_AUCUN ? "Honneur +1 - il pourrait te suivre" : "Honneur +1 - +15 or",
                           "Honneur -1 - +30 or"};
}

// ===================== Ashka est a terre =====================

void dialogueDAshka(Jeu& jeu) {
    std::vector<Replique> r = {
        {ORATEUR_PROPHETIE, "Ashka tombe a genoux. Son arc se brise sur les pierres du col."},
        {BOSS_ASHKA, "Acheve-moi. C'est ce que la prophetie demande, non ? Une seule de nos deux routes peut continuer."},
    };
    lancerDialogue(jeu, r, SuiteDialogue::Runes);
    jeu.dialogue.sujet = SUJET_ASHKA;
    jeu.dialogue.choix = {"L'epargner", "L'achever"};
    jeu.dialogue.effets = {"Honneur +2 - ses guerriers quittent Vorgath", "Honneur -2 - sa couronne : +3 attaque"};
}

// ===================== L'epilogue : les trois fins =====================

int finSelonHonneur(int honneur) {
    if (honneur >= 2) {
        return FIN_AUBE;
    }
    if (honneur <= -2) {
        return FIN_EPINES;
    }
    return FIN_VOYAGEUR;
}

std::string nomDeLaFin(int fin) {
    switch (fin) {
        case FIN_AUBE: return "FIN : L'AUBE";
        case FIN_EPINES: return "FIN : LA COURONNE D'EPINES";
        default: return "FIN : LE VOYAGEUR";
    }
}

void epilogue(Jeu& jeu) {
    std::vector<Replique> r = {{ORATEUR_PROPHETIE, "Vorgath s'effondre, et avec lui, les murs noirs de la citadelle. "
                                                   "Pour la premiere fois, la vision ne se brise pas."}};
    if (jeu.fin == FIN_AUBE) {
        r.push_back({ORATEUR_PROPHETIE, "Le soleil se leve sur les Terres de cendre. Sur la route, les villages rallument "
                                        "leurs lanternes, un a un."});
        if (jeu.choixAshka == 1) {
            r.push_back({BOSS_ASHKA, "Mon peuple rentre chez lui. Grace a toi. La prophetie parlait d'une chute... "
                                     "Elle ne disait pas laquelle."});
        }
        if (jeu.compagnon == COMPAGNON_KERRAK) {
            r.push_back({ORATEUR_KERRAK, "Je vais retourner au camp. Leur dire qu'on peut encore choisir. Merci, AYLIS."});
        } else if (jeu.compagnon == COMPAGNON_BRENNA) {
            r.push_back({ORATEUR_BRENNA, "Quarante pieces pour sauver le monde. Je crois que je t'ai fait un prix, finalement."});
        }
        r.push_back({ORATEUR_AYLIS, "La prophetie voulait que je tombe. Elle s'est trompee de chute."});
    } else if (jeu.fin == FIN_EPINES) {
        r.push_back({ORATEUR_PROPHETIE, "Le trone de Vorgath est vide. AYLIS s'en approche... et s'y assoit."});
        if (jeu.choixAshka == 2) {
            r.push_back({ORATEUR_PROPHETIE, "La couronne d'epines d'Ashka brille dans la penombre. Elle ne pese plus rien, "
                                            "a present."});
        }
        if (jeu.compagnon == COMPAGNON_BRENNA) {
            r.push_back({ORATEUR_BRENNA, "Je prends ma paie et je m'en vais. Je me bats pour de l'or, pas pour des rois."});
        }
        r.push_back({ORATEUR_PROPHETIE, "Les Haschen s'agenouillent. Ils ont un nouveau maitre. La prophetie ne mentait pas : "
                                        "un destructeur est bien tombe. Un autre s'est releve."});
        r.push_back({ORATEUR_AYLIS, "Qu'ils s'agenouillent. Au moins, eux, ne se releveront plus contre moi."});
    } else {
        r.push_back({ORATEUR_PROPHETIE, "AYLIS ne reste pas. Il y a d'autres routes, d'autres visions, d'autres prophetes "
                                        "qui ont besoin de quelqu'un pour marcher a leur place."});
        if (jeu.compagnon == COMPAGNON_BRENNA) {
            r.push_back({ORATEUR_BRENNA, "Tu me dois quarante pieces. Et une biere. Je t'attends a la prochaine route."});
        } else if (jeu.compagnon == COMPAGNON_KERRAK) {
            r.push_back({ORATEUR_KERRAK, "Ou que tu ailles, je connais les chemins. Je te suis."});
        }
        r.push_back({ORATEUR_AYLIS, "La route continue. Elle continue toujours."});
    }
    lancerDialogue(jeu, r, SuiteDialogue::Seuil);
    jeu.dialogue.titre = nomDeLaFin(jeu.fin);
}
