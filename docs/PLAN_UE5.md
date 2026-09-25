# VESPERANCE en 3D : plan pour Unreal Engine 5

Le prototype 2D (raylib) garde son rôle : c'est là qu'on invente et qu'on règle le jeu.
La version UE5 reprend **les mêmes règles** dans une arène en 3D vue du dessus
(caméra à 45°, comme un diorama). Le concept visuel est dans le canvas « VESPERANCE Key Art »,
planche « Concept UE5 vue du dessus ».

## Direction artistique (décidée)

- **Le prototype 2D reste en pixel art** : il sert à régler les règles, l'équilibre et la route.
- **La version UE5 aura un style illustré, « à la Hades »** : contours à l'encre, ombres franches (cel-shading),
  couleurs saturées, éclairage dramatique. Pas de pixel art.
- La référence est le concept **B, « clairière de la Forêt des Brumes »** du canvas « VESPERANCE Key Art » :
  - un monde organique, avec une **végétation dense et arrondie** (turquoise, vert, violet) ;
  - **des chemins bien colorés** : terre dorée, pierres runiques lumineuses, tapis de fleurs roses et violettes ;
  - des autels aux runes bleues, des esprits-échos, une mare lumineuse ;
  - **des branches sombres au premier plan** pour cadrer la scène ;
  - une grille tactique **discrète**, visible seulement autour d'AYLIS quand c'est son tour.
- **Mise à jour : Plan A « Conte sombre », version 2** (page « Direction A : sombre » du canvas) :
  - plus sombre et plus magique : rayons de lumière, runes qui flottent, feux follets, brume colorée ;
  - des arbres réalistes (écorce, racines, lierre) à la place des buissons ronds ;
  - un chemin détaillé : terre usée, ornières, vieilles dalles grises moussues, cailloux, racines, flaques,
    herbe qui déborde sur les bords. **Plus de pierres colorées par terre** ;
  - beaucoup de flore : tapis de petites fleurs, clochettes lumineuses, digitales, lys des brumes, hautes herbes.
- **Dialogues** : grand portrait, texte écrit sur un **manuscrit magique** (parchemin brûlé, filigrane doré,
  lettrine enluminée, runes et étincelles de la couleur de celui qui parle, nom sur un sceau de cire).
- Le concept A (île de dalles avec chemin en mosaïque) sert pour les lieux construits : la Forteresse de Karn, le Seuil.

## Assets 3D retenus

| Rôle | Asset | Licence | Notes |
|---|---|---|---|
| Haschen (monstres de base) | [Monster 4, par dorlak (CGTrader)](https://www.cgtrader.com/free-3d-models/character/fantasy-character/monster-4-f5757b92-dc9c-4f5e-ad0d-593203d14fe2) | Royalty Free (no AI) | Gratuit, 30k triangles, textures PBR 4k, rig squelette Epic + humanoïde, animé, formats FBX / UASSET. À télécharger soi-même (compte CGTrader). Les variantes (éclaireur, chaman, brute...) se font avec des armes, armures et teintes différentes. |

| AYLIS (placeholder) | [KayKit Adventurers, par Kay Lousberg](https://kaylousberg.itch.io/kaykit-adventurers) | CC0 | Rigé, animé, 25+ armes. Style chibi : contraste fort avec Monster 4, à remplacer ou à harmoniser plus tard. |
| Décor | [Stylized Nature MegaKit, par Quaternius](https://quaternius.itch.io/stylized-nature-megakit) | CC0 | 116 modèles (arbres, fleurs, rochers, herbes, dalles). Version Standard gratuite ; la version Source a un projet Unreal avec les shaders. |
| Détails du chemin | [Poly Haven](https://polyhaven.com/models/nature) : Root Cluster 01, Pine Roots, Rock Moss Set 01, Boulder 01, Tree Stump 01, Dead Tree Trunk 02, Fern 02, Moss 01 | CC0 | Scans réalistes, en petites touches. |

Planches « Assets UE5 » et « Test de rendu » dans le canvas « VESPERANCE Key Art ». Le test de rendu montre que
le décor Quaternius tient bien la nuit et que Monster 4 colle à l'ambiance, mais que le héros chibi détonne.
- En UE5, ce rendu passe par des **matériaux stylisés (toon shading + contour)**, **Lumen** pour la lumière,
  et de la végétation en instances (Foliage), avec des modèles au style peint.

## Ce qu'il faut installer (à faire soi-même)

1. **Epic Games Launcher**, puis **Unreal Engine 5** (onglet Unreal Engine > Bibliothèque).
2. Un éditeur C++ pour Unreal :
   - **Visual Studio 2022** avec la charge de travail « Développement de jeux en C++ » (gratuit, Community), ou
   - **JetBrains Rider** (la version de JetBrains pour Unreal, gratuite pour un usage non commercial).
3. Dans UE5 : nouveau projet **Games > Top Down**, en **C++** (pas Blueprint seul), nommé `Vesperance`.

## Bonne nouvelle : le code est déjà en C++

Unreal utilise du C++. Toute la logique pure du prototype (formules de dégâts, parcours en largeur,
runes, route, mémoire) peut être **recopiée presque telle quelle**. Ce qui change :

- les conventions d'Unreal : `FStruct` pour une struct, `UObject` / `AActor` pour les classes,
  `TArray` au lieu de `std::vector`, `FString` au lieu de `std::string` ;
- les macros `UCLASS()`, `USTRUCT()`, `UPROPERTY()` qui rendent les données visibles dans l'éditeur ;
- l'affichage (raylib) est remplacé par des acteurs 3D, des matériaux, Niagara et UMG.

## Correspondance prototype → Unreal

| Prototype 2D | Unreal Engine 5 | Rôle |
|---|---|---|
| `types.h` (Combattant, Arme) | `FCombattant`, `FArme` (USTRUCT) | les stats, identiques |
| `Pion` (jeu2d.h) | `AUnite` (un `ACharacter`) + un composant `UCombattantComponent` | AYLIS et les Haschen dans l'arène |
| `lieux.cpp` (cartes en texte) | `UCarteLieu` (un `UDataAsset` avec un `TArray<FString>`) | on garde les cartes en lettres ! |
| `decor.cpp` | `AGrilleArene` : lit la carte, pose les cases et les décors (une table lettre → acteur) | `~` = eau, `H` = maison, `#` = rempart... |
| `regles.cpp` | `UReglesCombat` (un `UObject`, logique pure) | dégâts, portée, cases atteignables |
| `main.cpp` (boucle, phases) | `AVesperanceGameMode` (les phases du tour) + `AVesperancePlayerController` (clic sur une case par *line trace*) | le tour par tour |
| `route.cpp`, `haltes.cpp` | `UVesperanceGameInstance` (garde la course d'un niveau à l'autre) + un niveau par lieu | la route, les runes, l'or |
| `memoire.cpp` | `UMemoireSave` (un `USaveGame`) | la mémoire entre les visions |
| `seuil.cpp` | un niveau `Seuil` (l'île flottante) | le hub |
| `animations.cpp` | Anim Montages, Niagara (étincelles, traînée de runes), Timeline | les coups qui ont du poids |
| `dessin.cpp` (panneau, cartes) | widgets UMG | l'interface |
| la carte de lumière | **Lumen** (éclairage dynamique) | l'ambiance, en vrai |

## Les étapes

1. **L'arène vide** : le projet Top Down, une `AGrilleArene` qui lit la carte du gué des Saules et pose des cubes
   (vert = sol, bleu = eau, gris = obstacle). Une caméra fixe à 45°.
2. **AYLIS sur la grille** : cliquer une case → AYLIS y marche. Les cases atteignables en surbrillance
   (le parcours en largeur de `casesAtteignables`, recopié).
3. **Le tour par tour** : les phases du GameMode, un Haschen qui avance (le `distancesJusquAAylis` du prototype),
   l'attaque, les dégâts. Toujours avec des formes simples.
4. **L'interface UMG** : barre des 9 actions, vie / mana / rage, ordre du tour.
5. **L'habillage** : de vrais modèles 3D (packs gratuits du Fab / Quixel), Lumen, eau, brume et feux follets Niagara,
   la traînée de runes violettes d'AYLIS.
6. **La route** : un niveau par lieu, la vision (choix de la salle), les runes, puis le Seuil et la mémoire.

Chaque étape donne quelque chose de jouable : on ne passe à la suivante que quand la précédente marche.
