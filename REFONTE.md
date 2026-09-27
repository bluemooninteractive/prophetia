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
- [ ] Vérifié en jeu (mode photo) et réglé

## 2. Progression RPG
- [ ] XP et niveaux (pv, attaque, points de compétence)
- [ ] Le Seuil : constellation Lame / Rempart / Prophétie, pouvoirs actifs (touches 1, 2, 3)
- [ ] Loot : objets au sol avec rareté (commun, rare, épique, légendaire), coffres, butin des élites et des boss

## 3. Personnage customisable
- [ ] Armes (épée, hache, dague, épée à deux mains, bâton...) : modèle ET animations différentes
- [ ] Boucliers (3 modèles, 4 couleurs)
- [ ] Armures (teintes de la tenue, cape, stats)

## 4. Animations et sons
- [ ] Sons générés (coups, pas, esquive, loot, niveau, interface, ambiances, météo)
- [ ] Musique d'ambiance par acte

## 5. Le monde
- [ ] Sol : vrai matériau (herbe, terre, gravier) avec variation, chemins de terre peints
- [ ] Monde rempli (plus de zones vides)
- [ ] Météo par acte (pluie, neige, cendres, brume, feuilles, éclairs)

## 6. Interface
- [ ] HUD épuré (vie, rage, XP, pouvoirs)
- [ ] Inventaire en anneau (maquette), Seuil en constellation (maquette), écran titre (maquette)
