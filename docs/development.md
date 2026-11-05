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
| `make lint` | Exécute `clang-tidy` sur `src/` et `tests/` (règles dans `.clang-tidy`) |
| `make cppcheck` | Exécute l'analyse statique sur `src/` et `tests/` |
| `make format-check` | Vérifie le format sans modifier les fichiers (`.clang-format`) |
| `make format` | Reformate les sources et tests avec le style de `.clang-format` |
| `make check-tools` | Vérifie la présence de GCC, gcov, ncursesw, cppcheck, clang-format et clang-tidy |
| `make install-desktop` | Installe le programme, `lang/` et une entrée de menu pour l'utilisateur courant |
| `make uninstall-desktop` | Supprime ce qu'a installé `install-desktop` |
| `make clean` | Supprime `build/`, `build-cov/` et l'exécutable |
| `make distclean` | `clean` et supprime aussi `*~`, `*.swp`, `core*` et `vgcore.*` |

Les tests s'exécutent depuis la racine du dépôt. Les conventions détaillées, la structure des tests et les chemins ncurses volontairement exclus sont décrits dans [AGENTS.md](../AGENTS.md).

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
