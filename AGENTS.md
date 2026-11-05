# AGENTS.md — tests unitaires et d'intégration

Consignes pour écrire et maintenir les tests de ce projet. Pas de framework externe : tout repose sur le mini
framework maison `tests/support/testlib.h`.

## Commandes

| Commande | Rôle |
| --- | --- |
| `make test` | compile et lance tous les tests (code de sortie ≠ 0 si un test échoue) |
| `make coverage` | lance les tests instrumentés (`build-cov/`) et affiche la couverture gcov par fichier (`scripts/coverage.sh`) |
| `make sanitize` | lance les tests avec AddressSanitizer, LeakSanitizer et UndefinedBehaviorSanitizer (`build-asan/`) |
| `make lint` | `clang-tidy` sur `src/` et `tests/` (règles dans `.clang-tidy`) |
| `make cppcheck` | `cppcheck` sur `src/` et `tests/` |
| `make format` / `make format-check` | `clang-format` sur `src/` et `tests/` |
| `make check-tools` | vérifie que les outils (gcc, gcov, ncursesw, cppcheck, clang-format, clang-tidy) sont installés |
| `make install-desktop` / `make uninstall-desktop` | installent/retirent le programme et l'entrée de menu de l'utilisateur (`APPDIR`, `DESKTOP_DIR` ; tester avec des dossiers dans `/tmp`, jamais dans le `$HOME` réel) |

Les cibles vérifient elles-mêmes la présence de leur outil. Si un outil manque, ne pas contourner la vérification :
l'indiquer à l'utilisateur avec la commande d'installation affichée.

Les tests se lancent **depuis la racine du projet** (certains lisent `gedcom-test.ged` et `lang/`). Avant de
terminer un travail : `make test`, `make lint`, `make cppcheck`, `make format-check` et `make sanitize` doivent
passer. La cible de sanitizers nécessite un compilateur compatible avec `-fsanitize=address,undefined`.

## Documentation

La documentation du projet doit être maintenue à jour en permanence et refléter fidèlement le comportement courant.
Toute modification du code ou du comportement utilisateur impose la mise à jour des documents concernés dans le même
changement :

- `README.md` : fonctionnalités, touches, cibles du `Makefile`, prérequis, arborescence de `src/` et `tests/`,
  procédure d'ajout d'un type d'enregistrement ou d'une langue ;
- `AGENTS.md` : commandes, conventions de test, points d'attention ;
- l'aide intégrée (`lang/*.lang`, clés `H_*`) quand une touche ou un comportement utilisateur change ;
- les commentaires d'en-tête (`.h`) des fonctions dont le contrat change.

Un module, une cible `make`, une touche ou un outil ajouté, renommé ou supprimé sans mise à jour de ces fichiers est
un changement incomplet.

## Règles de conception

- Appliquer le principe de responsabilité unique (SRP) à l'échelle des fonctions et des modules : chacun doit avoir
  une responsabilité cohérente et une raison principale d'évoluer.
- Garder ensemble les éléments qui changent pour la même règle métier ; séparer un module seulement s'il porte des
  responsabilités indépendantes, avec des raisons de changement distinctes. Ne pas créer de fichiers pour de simples
  fonctions utilitaires dans le seul but de réduire leur taille.
- Respecter les frontières existantes : `core/` traite le GEDCOM et les données sans ncurses ni traductions, `view/`
  prépare la présentation sans ncurses, `i18n/` gère les catalogues et la langue, `ui/` adapte ces fonctions au terminal.
- Lors d'une évolution, identifier le module propriétaire du comportement et éviter d'y mélanger parsing, présentation,
  accès terminal ou chargement de traductions qui relèvent d'autres couches.

## Organisation

```text
tests/
  support/          testlib.c/h (framework), fixtures.c/h (fichiers et bases temporaires)
  core/             un fichier test_<module>.c par module de src/core/
  i18n/             idem pour src/i18n/
  view/             idem pour src/view/
  ui/               input.c et external_target.c ; le lancement externe est remplacé par des doubles `InputHooks`
  integration/      scénarios qui traversent plusieurs modules
```

- Un fichier `tests/<dossier>/test_<module>.c` par module testé : il devient son propre exécutable
  (`build/tests/<dossier>/test_<module>`). Le `Makefile` détecte automatiquement les nouveaux fichiers.
- `src/ui/screen.c` et `src/ui/help.c` (ncurses), `src/ui/external.c` (lance un programme système) et `src/main.c`
  ne sont pas testés unitairement. La validation et la résolution des cibles sont isolées dans `external_target.c`
  et testées par `tests/ui/test_external_target.c`; le routage est testé avec `InputHooks`. Garder la résolution
  séparée du lancement des programmes pour qu'elle reste testable.
- Les tests ne lient pas ncurses.
- **Couverture** : viser les lignes et branches de logique ; les chemins d'échec d'allocation (`perror` + `exit`) sont
  volontairement non testés. Après un changement, vérifier avec `make coverage` qu'aucune nouvelle ligne de logique
  n'est non couverte.

## Règles pour un test unitaire

1. **Un test par scénario.** Un test vérifie un seul comportement.
2. **Structure AAA** avec les trois commentaires `// Given`, `// When`, `// Then`, dans cet ordre.
3. **Nom = phrase décrivant le comportement**, en snake_case, préfixé par la fonction ou le sujet testé :
   `nav_move_down_stops_at_the_last_item`. Pas de `test1`.
4. **Commentaire descriptif** : chaque fonction `TEST(...)` est immédiatement précédée d'un commentaire C en
  français qui décrit précisément le scénario et le résultat attendu. Le commentaire apporte une phrase lisible,
  il ne se contente pas de recopier le nom du test.
5. **Indépendance** : un test ne dépend ni de l'ordre d'exécution ni de l'état laissé par un autre. Tout l'état
   nécessaire est construit dans `// Given`.
6. **Pas d'accès réseau, pas d'attente, pas d'écriture hors dossier temporaire.** Utiliser `fx_tempdir`,
   `fx_write`, `fx_parse`, `fx_database` (nettoyés automatiquement à la fin).
7. **Un bug corrigé = un test de non-régression** écrit d'abord, qui échoue avant le correctif.
8. Une boucle dans un test est acceptable pour vérifier une même règle sur plusieurs valeurs ; garder les trois
   commentaires AAA (`// When` et `// Then` dans la boucle).

Modèle :

```c
/* Vérifie que le déplacement vers le bas reste bloqué sur le dernier élément. */
TEST(nav_move_down_stops_at_the_last_item) {
    // Given
    Database db;
    fx_database(TREE, &db);
    Nav nav;
    nav_init(&nav);
    nav.sel = 2;
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, 1);

    // Then
    ASSERT_EQ_INT(2, nav.sel);
}

int main(void) {
    tl_suite("view/nav");
    RUN_TEST(nav_move_down_stops_at_the_last_item);
    return tl_finish();
}
```

Tout nouveau `TEST` doit être enregistré avec `RUN_TEST` dans le `main` du fichier (sinon il ne s'exécute pas).

## Framework (`tests/support/testlib.h`)

- `TEST(nom)`, `RUN_TEST(nom)`, `tl_suite("module")`, `tl_finish()`.
- Assertions : `ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_NULL`, `ASSERT_NOT_NULL`, `ASSERT_EQ_INT(attendu, obtenu)`,
  `ASSERT_EQ_STR(attendu, obtenu)` (gère `NULL`).
- La première assertion en échec interrompt le test courant (`longjmp`), les autres tests continuent. Les destructeurs
  automatiques ne s'exécutent pas sur ce chemin; une suite qui échoue peut aussi signaler des fuites.
- Les arbres retournés par `fx_parse` sont libérés à la fin du processus. Déclarer les bases, historiques, catalogues
  et détails dynamiques avec `TL_AUTOCLEAN(destructeur)` afin que les scénarios réussis restent propres sous LSan.
- Besoin d'une nouvelle assertion : l'ajouter à `testlib.h/.c` plutôt que de répéter un `if` + `tl_fail` dans les tests.

## Points d'attention

- **Sans catalogue chargé**, `T(id)` renvoie le nom de la clé et `tag_label(tag)` renvoie le tag. Les tests de
  `view/` s'appuient sur cela (`"NAME: Jean DUPONT"`) ; pour un texte traduit, appeler `lang_init` avec un dossier
  temporaire (voir `tests/i18n/test_i18n.c`, variable `GEDVIEW_LANG_DIR`).
- `lang_init` réinitialise l'état de `i18n` : l'appeler dans le `Given` de chaque test qui dépend de la langue.
  Nettoyer `LC_ALL`, `LC_MESSAGES` et `LANG` si le scénario lit l'environnement.
- Les fichiers GEDCOM de test sont des chaînes littérales courtes, écrites dans le test lui-même ou dans une
  constante du fichier (ex. `TREE`). Garder des données minimales et lisibles.
- `Detail` peut être construit à la main (`Line lines[] = {...}; Detail d = {.lines = lines, .count = 2};`)
  pour isoler `nav` de `detail`.

## Tests d'intégration (`tests/integration/`)

- Ils traversent plusieurs modules sans simulation : fichier réel → `gedcom_parse` → `db_build` → `detail_build` →
  `nav_*`.
- Ils utilisent le fichier d'exemple `gedcom-test.ged` (lu depuis la racine du projet), pas des chaînes
  construites dans le test.
- Même règles AAA et mêmes noms descriptifs que pour les tests unitaires, un fichier `test_<sujet>.c` par thème.
- Préférer des **propriétés** (« chaque lien du détail pointe vers un enregistrement existant ») à des valeurs
  figées, sauf pour vérifier un cas précis du fichier d'exemple. Si `gedcom-test.ged` change, ces tests doivent
  rester valides.
- Ne pas tester ncurses (`src/ui/screen.c`, `help.c`) : pas de terminal en test.
