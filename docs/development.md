# Développement

## Prérequis

- GCC et `make`
- `ncursesw` (en-têtes et bibliothèque)
- Pour les contrôles de développement : `gcov`, `cppcheck`, `clang-format` et `clang-tidy`

`make check-tools` récapitule les outils disponibles. Chaque cible de contrôle vérifie également son outil et affiche une suggestion d'installation s'il manque.

Pour les outils de développement, Fedora fournit `cppcheck` et `clang-tools-extra`; Debian/Ubuntu fournit `cppcheck`, `clang-format` et `clang-tidy`. `gcov` est fourni avec GCC.

## Commandes

| Commande | Rôle |
| --- | --- |
| `make` | Compile `gedview` (objets dans `build/`) |
| `make test` | Compile et lance les tests unitaires et d'intégration |
| `make coverage` | Lance les tests instrumentés dans `build-cov/` et résume la couverture gcov |
| `make sanitize` | Recompile et lance les tests instrumentés par ASan, LSan et UBSan dans `build-asan/` |
| `make lint` | Analyse statique avec `clang-tidy` sur `src/` et `tests/` |
| `make cppcheck` | Analyse statique complémentaire avec `cppcheck` sur `src/` et `tests/` |
| `make format-check` | Vérifie le format sans modifier les fichiers (`.clang-format`) |
| `make format` | Reformate les sources et tests avec le style de `.clang-format` |
| `make check-tools` | Vérifie la présence de GCC, gcov, ncursesw, cppcheck, clang-format et clang-tidy |
| `make install-desktop` | Installe le programme, `lang/` et une entrée de menu pour l'utilisateur courant |
| `make uninstall-desktop` | Supprime ce qu'a installé `install-desktop` |
| `make clean` | Supprime `build/`, `build-cov/`, `build-asan/` et l'exécutable |
| `make distclean` | `clean` et supprime aussi `*~`, `*.swp`, `core*` et `vgcore.*` |

Les tests s'exécutent depuis la racine du dépôt. Les conventions détaillées, la structure des tests et les chemins ncurses volontairement exclus sont décrits dans [AGENTS.md](../AGENTS.md).

`make lint` applique les contrôles clang-tidy définis dans `.clang-tidy` : il signale des défauts potentiels et des
écarts aux règles du projet sans exécuter le programme. `make cppcheck` examine aussi le code sans l'exécuter, avec
des contrôles de défauts probables, de style, de performance et de portabilité.

`make sanitize` instrumente le code exécuté par les tests et arrête la suite au premier diagnostic : AddressSanitizer
(ASan) repère notamment les accès hors limites, les use-after-free et les doubles libérations; LeakSanitizer (LSan)
signale les allocations encore vivantes à la fin du processus; UndefinedBehaviorSanitizer (UBSan) détecte plusieurs
formes de comportement indéfini, comme certains débordements signés, décalages invalides ou accès mal alignés.
Ces contrôles ne couvrent que les chemins effectivement exercés par les tests. La cible utilise `CC` avec
`-fsanitize=address,undefined` et exige un compilateur compatible. Les fixtures libèrent les arbres GEDCOM à la fin
du processus; les ressources locales sont détruites automatiquement avec `TL_AUTOCLEAN`.

La classification des URL et la résolution canonique des chemins média sont testées dans
`tests/ui/test_external_target.c`; ces tests n'exécutent pas `xdg-open`. Le lancement réel reste à vérifier
manuellement dans un environnement de bureau.

## Ajouter un type d'enregistrement

1. Ajouter une valeur à l'enum `Kind` et une entrée à la table `KINDS` dans `src/core/records.c`.
2. Ajouter une entrée à `VIEWS` dans `src/view/kindview.c` (touche et titre).
3. Ajouter les textes correspondants dans `src/i18n/i18n.h` et `src/i18n/catalog.c`.
4. Ajouter les clés et valeurs correspondantes dans chaque `lang/*.lang`.
5. Couvrir le nouveau type dans les tests des modules concernés et dans l'intégration si le parcours le justifie.

## Traductions

Chaque fichier `lang/<code>.lang` utilise le format UTF-8 `CLE=valeur`. `#` commence un commentaire, `\n` représente un retour à la ligne et `\\` un antislash. Les libellés de tags utilisent `TAG.NAME=Nom`. `en.lang` sert de repli pour les clés absentes; les conversions de format autorisées sont validées au chargement.

Pour ajouter une langue, copier `lang/en.lang` vers `lang/xx.lang`, traduire ses valeurs et vérifier l'affichage avec `./gedview -l xx gedcom-test.ged`. Les langues sont détectées automatiquement, sans recompilation.

La langue est choisie avec `-l xx`, sinon selon `LC_ALL`, `LC_MESSAGES`, puis `LANG`, et enfin l'anglais. Le dossier des catalogues est recherché dans cet ordre : `$GEDVIEW_LANG_DIR`, à côté de l'exécutable, `./lang`, `/usr/local/share/gedview/lang`, `/usr/share/gedview/lang`.

Voir [Architecture](architecture.md) pour les responsabilités des modules et les parcours de données, et [Publication](releasing.md) pour les versions et tags Git.
