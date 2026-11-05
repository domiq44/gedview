#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "i18n/langdir.h"
#include "support/fixtures.h"
#include "support/testlib.h"

/* Vérifie que GEDVIEW_LANG_DIR est utilisé lorsqu'il contient la langue de base. */
TEST(langdir_find_returns_the_directory_given_by_the_environment) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "en.lang", "NONAME=x\n");
    setenv("GEDVIEW_LANG_DIR", dir, 1);
    char out[PATH_MAX + 8];

    // When
    int found = langdir_find(out, sizeof out);

    // Then
    ASSERT_TRUE(found);
    ASSERT_EQ_STR(dir, out);
}

/* Vérifie qu'un dossier d'environnement sans anglais n'est pas retenu. */
TEST(langdir_find_ignores_an_environment_directory_without_the_base_language) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "fr.lang", "NONAME=x\n");
    setenv("GEDVIEW_LANG_DIR", dir, 1);
    char out[PATH_MAX + 8] = "";

    // When
    int found = langdir_find(out, sizeof out);

    // Then
    ASSERT_TRUE(!found || strcmp(out, dir) != 0);
}

/* Vérifie que la recherche retombe sur le dossier lang du projet. */
TEST(langdir_find_falls_back_to_the_lang_directory_of_the_project) {
    // Given
    unsetenv("GEDVIEW_LANG_DIR");
    char out[PATH_MAX + 8] = "";

    // When
    int found = langdir_find(out, sizeof out);

    // Then
    ASSERT_TRUE(found); /* les tests s'exécutent depuis la racine du projet (make test) */
}

/* Vérifie que les codes de langue trouvés sont retournés par ordre alphabétique. */
TEST(langdir_list_returns_the_language_codes_in_alphabetical_order) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "fr.lang", "");
    fx_write(dir, "en.lang", "");
    fx_write(dir, "de.lang", "");
    char *codes[8];

    // When
    int n = langdir_list(dir, codes, 8);

    // Then
    ASSERT_EQ_INT(3, n);
    ASSERT_EQ_STR("de", codes[0]);
    ASSERT_EQ_STR("en", codes[1]);
    ASSERT_EQ_STR("fr", codes[2]);
}

/* Vérifie que les fichiers sans suffixe .lang ne sont pas listés comme langues. */
TEST(langdir_list_ignores_files_that_are_not_lang_files) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "en.lang", "");
    fx_write(dir, "readme.txt", "");
    fx_write(dir, "fr.lang.bak", "");
    fx_write(dir, ".lang", "");
    char *codes[8];

    // When
    int n = langdir_list(dir, codes, 8);

    // Then
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_STR("en", codes[0]);
}

/* Vérifie que les codes de langue dépassant huit caractères sont ignorés. */
TEST(langdir_list_ignores_codes_longer_than_eight_characters) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "abcdefgh.lang", "");
    fx_write(dir, "abcdefghi.lang", "");
    char *codes[8];

    // When
    int n = langdir_list(dir, codes, 8);

    // Then
    ASSERT_EQ_INT(1, n);
    ASSERT_EQ_STR("abcdefgh", codes[0]);
}

/* Vérifie que la liste ne dépasse jamais la capacité fournie par l'appelant. */
TEST(langdir_list_never_returns_more_codes_than_the_limit) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "de.lang", "");
    fx_write(dir, "en.lang", "");
    fx_write(dir, "fr.lang", "");
    char *codes[2];

    // When
    int n = langdir_list(dir, codes, 2);

    // Then
    ASSERT_EQ_INT(2, n);
}

/* Vérifie qu'un dossier sans fichiers .lang produit une liste vide. */
TEST(langdir_list_returns_zero_for_a_directory_without_lang_files) {
    // Given
    const char *dir = fx_tempdir();
    char *codes[4];

    // When
    int n = langdir_list(dir, codes, 4);

    // Then
    ASSERT_EQ_INT(0, n);
}

/* Vérifie qu'un dossier de langues inexistant ne fournit aucun code. */
TEST(langdir_list_returns_zero_for_a_missing_directory) {
    // Given
    const char *dir = "/nonexistent/lang/dir";
    char *codes[4];

    // When
    int n = langdir_list(dir, codes, 4);

    // Then
    ASSERT_EQ_INT(0, n);
}

/* Enregistre les scénarios de découverte des catalogues et retourne leur bilan. */
int main(void) {
    tl_suite("i18n/langdir");
    RUN_TEST(langdir_find_returns_the_directory_given_by_the_environment);
    RUN_TEST(langdir_find_ignores_an_environment_directory_without_the_base_language);
    RUN_TEST(langdir_find_falls_back_to_the_lang_directory_of_the_project);
    RUN_TEST(langdir_list_returns_the_language_codes_in_alphabetical_order);
    RUN_TEST(langdir_list_ignores_files_that_are_not_lang_files);
    RUN_TEST(langdir_list_ignores_codes_longer_than_eight_characters);
    RUN_TEST(langdir_list_never_returns_more_codes_than_the_limit);
    RUN_TEST(langdir_list_returns_zero_for_a_directory_without_lang_files);
    RUN_TEST(langdir_list_returns_zero_for_a_missing_directory);
    return tl_finish();
}
