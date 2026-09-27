# VESPERANCE : la grande refonte (demandée le 27/09/2026)

Suivi des étapes. Une session qui reprend le travail lit ce fichier et continue à la première case vide.
Compiler : Unreal fermé, puis
`"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" VesperanceEditor Win64 Development "-Project=<chemin>/Vesperance.uproject" -WaitMutex`

## 1. Le combat en temps réel (plus de tour par tour)
- [x] Animations mélangées en C++ (VespAnim : marche/course, attaques sur le haut du corps, fondus)
- [x] AYLIS : attaque (combo 3 coups), attaque lourde, esquive (invulnérable), garde et parade parfaite, potion, spéciale
- [x] Les Haschen en temps réel (VespCombat) : poursuite, attaque annoncée, tireurs, chargeurs, sauteurs, soigneurs, invocateurs, explosifs
- [x] Les clairières : les Haschen sortent de terre, une barrière ferme la clairière, vagues de renfort ; patrouilles sur les sentiers
- [x] Les 7 boss en temps réel (15 motifs : écrasements, charges, pluies de flèches, salves, flaques, éruptions, bonds, nova... ; rage à mi-vie)
- [x] Les règles de chaque acte en temps réel (poison, eaux toxiques, dalles piégées, blizzard, éruptions, échos du Voile)
- [x] Suppression de la grille (VespGrille)
- [x] Sons synthétisés (VespSons) + musique et tambours de combat par acte
- [x] Vérifié en jeu (mode photo) et réglé (cercles runiques du pack retirés des parades, impacts et barrières : ils masquaient les zones rouges)

## 2. Progression RPG
- [x] XP et niveaux (pv, attaque, points de compétence) — bandeau « NIVEAU »
- [x] Le Seuil : 12 étoiles (Lame / Rempart / Prophétie), 3 pouvoirs (Tourbillon, Égide, Lame d'éther), 9 talents
- [x] Loot : objets au sol (faisceau de la couleur de la rareté), coffres, élites, boss ; 6 emplacements, sac de 24

## 3. Personnage customisable
- [x] Armes : épée, hache, dagues (deux mains), grande épée / hache de guerre, bâton (sorts) : modèle, couleur, rythme et portée différents
- [x] Boucliers (3 modèles, 4 couleurs selon la rareté)
- [x] Armures (le corps teinte la tenue d'AYLIS) ; tête, amulette, anneau : stats et pouvoirs
- [x] Armes tenues vérifiées (photos « armurerie » en mode photo : épée+bouclier, dagues, deux mains agrandies, bâton ; la tenue prend la couleur de l'armure)

## 4. Animations et sons
- [x] Sons générés (VespSons) : coups, pas, esquive, parade, os, tirs, sorts, loot, niveau, interface, tonnerre
- [x] Musique par acte + tambours de combat + ambiances (forêt, marais, vent, braises, Voile) + pluie
- [x] Animations KayKit mélangées (VespAnim) : combos, lourde, esquives directionnelles, garde, coups reçus, mort, potion, ramasser

## 5. Le monde
- [x] Sol vivant (VespSol) : herbe / terre battue / matière de l'acte, carte peinte d'après les chemins
- [x] Sol : matériau corrigé (il ne compilait pas → matériau par défaut gris) ; couleurs réelles par acte, la texture ne donne que le grain
- [x] Végétation : les SMF_* sont des types de feuillage, pas des modèles → remplacés par les SM_* (arbres, buissons, rochers, herbes enfin visibles) ; acte V : cyprès et arbres morts au lieu de cônes
- [x] Monde rempli : anneaux des clairières et bas-côtés des sentiers
- [x] Météo (VespMeteo) : feuilles, averses et éclairs, bruine, escarbilles, neige et blizzard, cendre et braises, poussières du Voile
- [x] 7 actes vérifiés en photo ; feuillage Provençal teinté par acte, lave de l'acte VI moins criarde

## 6. Interface
- [x] HUD : vie, rage, XP, pouvoirs et recharges, barre du boss, clairière scellée, objet trouvé
- [x] Menu d'AYLIS (VespMenu, touche I) : inventaire en anneau et Seuil en constellation (d'après les maquettes)
- [x] Écran titre d'après la maquette (sceau qui se dessine puis tourne, menu centré, liserés dorés au survol)
- [x] Accents dans les textes de l'interface (138 textes)
