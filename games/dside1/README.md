# D-Side 1 — Ville abandonnée

Premier prototype jouable original pour NumWorks N0120, construit comme une application autonome à partir du SDK de NumPlay.

## Commandes
- Flèches : déplacement et direction du dash
- OK : sauter / valider
- Back : dash (et reprendre depuis la pause)
- Backspace : pause
- Home : quitter

Le chapitre contient cinq salles sur un écran chacune. Les cartes, plateformes, obstacles et couleurs sont dessinés par le programme : aucune carte ou ressource du mod D-Sides n'est copiée.

## Compilation
Depuis la racine du dépôt, après `python tools/setup.py` et l'activation de l'environnement NumPlay :

```sh
make -C games/dside1
make -C games/dside1 check
```

Le fichier généré est `games/dside1/output/DSide1.nwa`. Le binaire doit encore être compilé et testé dans l'émulateur puis sur une NumWorks N0120 avant d'être annoncé comme validé.
