# Publication

## Version affichée

La version affichée dans le cadre du détail est calculée par `make` avec `git describe --tags --always --dirty` et compilée dans `src/core/version.c` via `GEDVIEW_VERSION`.

| État du dépôt | Version affichée |
| --- | --- |
| `HEAD` exactement sur `v1.0.0` | `v1.0.0` |
| 3 commits après ce tag | `v1.0.0-3-g1a2b3c4` |
| Modifications non commitées | suffixe `-dirty` (par exemple `v1.0.0-dirty`) |
| Aucun tag disponible | hash court du commit (par exemple `7977c76`) |
| Sources hors dépôt Git | `dev` |

Pour publier une version, créer un tag puis le pousser. Le tag peut être créé localement ou par la création d'une release GitHub :

```sh
git tag v1.0.0
git push --tags
```

Une release GitHub peut également créer le tag; dans ce cas, le récupérer localement avec `git fetch --tags`. Recompiler ensuite avec `make`, ou avec `make install-desktop` pour actualiser la copie installée. Le Makefile détecte le changement de version et ne recompile que `src/core/version.c` lorsque le reste est à jour.

Pour contrôler une version depuis un clone propre :

```sh
git clone <url>
cd gedview
git checkout v1.0.0
make
git describe --tags --always --dirty
```

La sortie attendue est le tag de la version publiée.

Voir [Développement](development.md) pour les commandes de test et les conventions, et [Architecture](architecture.md) pour le fonctionnement interne.

## Vérifications manuelles avant publication

Depuis la racine du dépôt, lancer `./gedview gedcom-test.ged` dans un terminal interactif et vérifier :

- le rendu des deux panneaux, les vues principales, `Tab`, le suivi d'un lien et le retour arrière ;
- la recherche, l'aide, le changement de langue et la fermeture avec `q` sans laisser le terminal dans un état altéré ;
- le redimensionnement vers un terminal trop petit, l'apparition du message dédié, puis le retour au rendu normal après agrandissement ;
- l'ouverture d'une URL de note et d'un média d'exemple dans un environnement où `xdg-open` et les applications associées sont disponibles.

Ces vérifications complètent `make sanitize` et les tests automatisés, qui ne couvrent ni le rendu ncurses ni le lancement réel de programmes externes.
