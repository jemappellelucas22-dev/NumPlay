# D-Side 1 — Ville abandonnée

Un chapitre original de plateforme inspiré de Celeste, conçu pour NumWorks N0120.

## Ce qui est dans cette version

- Cinq quartiers originaux, chacun construit sur deux écrans avec caméra qui suit Madeline.
- Déplacement horizontal avec accélération, saut avec tolérance au bord (coyote time) et saut mural.
- Dash directionnel, recharge au sol et cristaux de recharge à récupérer.
- Accroche aux murs avec Toolbox, pics, particules, effets de dash et transitions.
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