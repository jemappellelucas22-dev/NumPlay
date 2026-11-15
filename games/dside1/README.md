# D-Side 1 — Ville abandonnée

Un chapitre original de plateforme inspiré de Celeste, conçu pour NumWorks N0120.

## Ce qui est dans cette version

- Cinq quartiers originaux, chacun construit sur deux écrans avec caméra qui suit Madeline.
- Physique adaptée du code public de Celeste Classic : accélération, inertie, coyote time, jump buffer, saut mural et dash accéléré.
- Dash directionnel sur 8 directions, recharge au sol et cristaux de recharge.
- Accroche/slide mural avec Toolbox, pics, particules et transitions.
- Cinq salles originales conçues autour du saut, du wall-jump et du dash.
- Menu, écran de commandes, pause, compteur de morts et écran de fin.
- Graphismes dessinés en 16 bits, ambiance nocturne de ville abandonnée.

## Commandes

- Flèches : courir et viser le dash
- OK : sauter
- Back : dash
- Toolbox (maintenir) : s'accrocher aux murs
- Backspace : pause / reprendre
- Home : quitter l'application

## Compilation

Depuis la racine du dépôt :

```sh
make -C games/dside1
make -C games/dside1 check
```

L'application générée est `games/dside1/output/DSide1.nwa`.

Le jeu doit encore être testé sur une NumWorks N0120 réelle pour ajuster la difficulté, les collisions et le rythme des salles.

## Référence et licence

Le déplacement est adapté du fichier public `Source/PICO-8/Classic.cs` du dépôt NoelFB/Celeste, publié sous licence MIT. Voir `THIRD_PARTY_NOTICES.md` pour l'attribution et le texte de licence. Les niveaux, graphismes et autres éléments de D-Side 1 sont originaux ; aucun asset du Celeste commercial n'est repris.
