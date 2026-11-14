# D-Side 1 — Ville abandonnée (prototype)

> Statut : conception du niveau. Ce document ne prétend pas que le chapitre est déjà jouable.

## Intention

Créer un chapitre bonus original, inspiré du niveau de difficulté des D-Sides, sans reprendre les cartes ni les ressources du mod D-Sides. Le premier prototype vise la précision des déplacements et l'utilisation de l'unique dash.

## Règles de conception

- Conserver les commandes, la physique et les collisions du Celeste NumPlay.
- Construire des salles courtes, lisibles et exigeantes ; chaque salle introduit une idée avant de la combiner avec une autre.
- Ne pas ajouter de nouvelle mécanique nécessitant de modifier le moteur tant que le prototype peut être construit avec les entités existantes.
- Prévoir une sortie et un point de réapparition cohérents dans chaque salle.
- Tester sur la version hôte avant toute tentative de compilation NumWorks.

## Parcours proposé

1. **Entrée — Le dernier toit** : prise en main du dash avec un saut mural et une plateforme étroite.
2. **Ruelles — Fenêtre étroite** : enchaînement de sauts muraux et dash horizontal, avec peu de marge d'erreur.
3. **Chantier — Les blocs** : utiliser les blocs mobiles/destructibles déjà gérés par le moteur pour traverser une zone verticale.
4. **Clocher — Dernière charge** : combiner rebond, saut mural et recharge de dash dans une salle plus longue.
5. **Sortie — Au-dessus des nuages** : dernière séquence qui réutilise les trois apprentissages, puis sortie de chapitre.

## État technique vérifié

- Le moteur charge les salles depuis `src/data.bin`; les données de chapitre contiennent les références aux salles.
- `tools/pack.py` reconstruit `src/data.bin` à partir des fichiers de contenu Celeste (cartes et graphismes).
- La liste `ALL_MAPS` du packer est explicite et ne contient pas de carte D-Side.
- Le dépôt ne fournit pas, à lui seul, un éditeur de niveaux intégré qui permette d'ajouter une salle en modifiant uniquement un fichier C.

## Prochaine étape d'implémentation

Il faut créer une carte de salle dans le format d'entrée attendu par le packer, ajouter son nom à la liste des cartes, puis vérifier que le packer génère une entrée de chapitre et que le moteur sait la charger. Tant que cette chaîne n'a pas été testée, aucune modification des chapitres existants ne doit être présentée comme un niveau D-Side jouable.
