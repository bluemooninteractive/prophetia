# VESPÉRANCE

Un roguelite d'action en temps réel, vu de dessus, dans un conte nocturne : AYLIS, une vision envoyée par la prophétie, traverse sept terres pour atteindre Karn. À chaque chute, une nouvelle vision recommence, et le monde s'en souvient.

Studio : **Chronicles Games**. Jeu en développement : tout peut encore changer.

## En bref

| | |
|---|---|
| Moteur | Unreal Engine 5.8, tout le jeu est écrit en C++ (très peu de Blueprints) |
| Carte | `Content/Arene.umap` (le monde de chaque acte est généré par le code) |
| Code du jeu | `Source/Vesperance/Tactique/` (environ 17 000 lignes) |
| Suivi des chantiers | `REFONTE.md` |

## Le code (`Source/Vesperance/Tactique`)

| Fichier | Rôle |
|---|---|
| `VespPlayerController` | Le déroulement d'une vision : écran titre, actes, clairières, dialogues, fins, sauvegardes, options, tests automatiques |
| `VespCombat` | Le combat en temps réel : Haschen, élites, boss, attaques annoncées, barrières |
| `VespUnite` / `VespAnim` | Les personnages : déplacements, coups, armes, animations |
| `VespMonde` / `VespSol` | La génération du monde de chaque acte (clairières, sentiers, décor, sols) |
| `VespMaitres` | Les maîtres des éléments et leurs dons, les pactes de Liss |
| `VespProgression` | Le Seuil (compétences), le butin, les cartes du Veilleur |
| `VespSauvegarde` / `VespPartie` / `VespReglages` | La mémoire de la boucle, la vision mise de côté, les options |
| `VespInterface` / `VespMenu` / `VespOptions` | L'interface (Slate) : écran titre, HUD, menus, fins |
| `VespSons` | Les sons et musiques, synthétisés par le code |
| `VespEffet` / `VespMeteo` / `VespLucioles` | Effets visuels, météo, lucioles |
| `VespUsagesCommandlet` | Un outil en ligne de commande (usages des matériaux, tailles, vignettes) |

## Installer le projet

1. **Unreal Engine 5.8** et **Visual Studio** (avec les outils C++ pour le jeu).
2. Cloner ce dépôt.
3. **Installer les packs du marché** (voir ci-dessous) dans `Content/`, sous le même nom de dossier. Ils ne sont pas dans le dépôt : trop lourds, et leur licence interdit de les publier.
4. Compiler (Unreal fermé) :
   ```
   "C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" VesperanceEditor Win64 Development "-Project=<chemin>/Vesperance.uproject" -WaitMutex
   ```
5. Lancer le jeu :
   ```
   UnrealEditor.exe "<chemin>/Vesperance.uproject" /Game/Arene -game
   ```

### Les packs du marché (à installer depuis Fab)

| Dossier dans `Content/` | Taille | Utilisé pour |
|---|---|---|
| `StylizedProvencal` | ~300 Mo | Végétation, rochers, autel, eau |
| `Planet385CY` | ~1,2 Go | La corruption du Voile |
| `Fantastic_Village_Pack` | ~1,2 Go | Textures du sol, maisons |
| `Pack_Bonus` | ~450 Mo | Sols (herbe, pierre, dalles, planches) |
| `Free_Magic` | ~515 Mo | Effets magiques (auras, cercles) |
| `Stylized_EnvirPack` | ~300 Mo | Champignons, fleurs, herbes, grands arbres |
| `RPG_Magic_Staff_Pack` | ~200 Mo | Bâtons de sorts |
| `Fantasy_Forest` | ~160 Mo | Arbres fantastiques |
| `StylizedCharacter` | ~120 Mo | Armes (épées, haches, arcs, dagues, boucliers) |
| `DungeonAndCemetery` | ~110 Mo | Tombes, ossuaires, colonnades |
| `WaterMaterials` | ~90 Mo | Eau |
| `StyleHex_Studio` | ~80 Mo | Végétation |
| `DarkFantasyPack_01` | ~70 Mo | Épées et boucliers rares |
| `Spell_Mix` | ~55 Mo | Icônes des pouvoirs |
| `Magma_Material` | ~8 Mo | Lave (acte VI) |
| `RPG-FlameAttackVFX` | ~8 Mo | Effet de flammes (brûlure) |
| `Vol01_Potions` | ~5 Mo | Icône de potion |
| `UltimateUIMenusSFX` | ~35 Mo | Sons d'interface (pas encore branchés) |
| `GanzSe_Accessories` | ~4 Mo | Pas encore utilisé |

Sans un pack, le jeu se lance quand même : le code remplace ce qui manque par des formes simples.

## Les options de test

Le jeu se teste tout seul avec des options de lancement (ajoutées après `-game`). Aucune ne touche à la sauvegarde du joueur, sauf le mode photo, qui n'écrit rien non plus.

| Option | Ce qu'elle fait |
|---|---|
| `-VespPhotos [-VespActe=N]` | Parcourt les actes et prend des photos promo dans `Saved/Photos` |
| `-VespVeilleur` | Photographie le panneau du Veilleur (les cartes) |
| `-VespOuverture` | Lance une vision et photographie son introduction |
| `-VespFin=1` / `-VespFin=2` | Déroule une fin (nouvel Oracle / vraie fin) |
| `-VespOptions` | Photographie les options (écran titre, puis en jeu) |
| `-VespReprise=1` puis `=2` | Met une vision de côté, puis la reprend |
| `-VespRecompenses` | Photographie les récompenses annoncées (balises, carte) |
| `-VespMaitres` | Photographie un maître, une amélioration, un pacte de Liss |

## Les branches

| Branche | Rôle |
|---|---|
| `main` | Les versions stables et jouables. On n'y travaille jamais directement. |
| `develop` | Le travail en cours, qui compile. Les fonctionnalités y sont fusionnées. |
| `feature/<nom>` | Un chantier (ex. `feature/parties-courtes`), créé depuis `develop` et fusionné dedans quand il est fini. |

Les grandes étapes sont marquées par des tags :

| Tag | Étape |
|---|---|
| `v0.1-tour-par-tour` | Le premier jeu : 7 actes au tour par tour, route, boss, écran titre |
| `v0.2-temps-reel` | La refonte : combat en temps réel, monde continu, butin, Seuil |
| `v0.3-roguelite` | La boucle roguelite : mémoire, Veilleur et cartes, fins, reprise, récompenses annoncées, maîtres des éléments |

Les messages de commit sont en français, au présent, et disent ce que le joueur y gagne.
