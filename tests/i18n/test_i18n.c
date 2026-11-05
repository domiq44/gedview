#include <stdlib.h>

#include "i18n/i18n.h"
#include "support/fixtures.h"
#include "support/testlib.h"

/* Dossier avec en.lang et fr.lang ; fr n'a pas S_DETAIL ni TAG.DATE (repli sur l'anglais attendu). */
/* Crée un répertoire temporaire contenant les catalogues minimaux anglais et français. */
static const char *two_languages(void) {
    const char *dir = fx_tempdir();
    fx_write(dir, "en.lang", "NONAME=Unnamed\nDETAIL= Details \nTAG.NAME=Name\nTAG.DATE=Date\n");
    fx_write(dir, "fr.lang", "NONAME=Sans nom\nTAG.NAME=Nom\n");
    setenv("GEDVIEW_LANG_DIR", dir, 1);
    return dir;
}

/* Supprime les variables de locale afin d'isoler le choix de langue de l'environnement. */
static void clear_locale_environment(void) {
    unsetenv("LC_ALL");
    unsetenv("LC_MESSAGES");
    unsetenv("LANG");
}

/* Vérifie que T retourne le nom de clé si aucun catalogue ne fournit le message. */
TEST(t_returns_the_key_name_when_no_language_defines_the_message) {
    // Given
    two_languages();

    // When
    lang_init("en");

    // Then
    ASSERT_EQ_STR("SEARCH", T(S_SEARCH));
}

/* Vérifie que la clé spéciale BLANK produit toujours une chaîne vide. */
TEST(t_returns_an_empty_string_for_the_blank_message) {
    // Given
    two_languages();
    lang_init("en");

    // When
    const char *blank = T(S_BLANK);

    // Then
    ASSERT_EQ_STR("", blank);
}

/* Vérifie que la langue explicitement imposée est sélectionnée. */
TEST(lang_init_uses_the_forced_language) {
    // Given
    two_languages();

    // When
    lang_init("fr");

    // Then
    ASSERT_EQ_STR("Sans nom", T(S_NONAME));
}

/* Vérifie qu'un code régional forcé est reconnu par son préfixe de langue. */
TEST(lang_init_matches_a_forced_locale_code_by_its_language_prefix) {
    // Given
    two_languages();

    // When
    lang_init("fr_FR.UTF-8");

    // Then
    ASSERT_EQ_STR("Sans nom", T(S_NONAME));
}

/* Vérifie qu'une clé absente de la langue choisie utilise sa valeur anglaise. */
TEST(t_falls_back_to_english_when_the_selected_language_lacks_the_message) {
    // Given
    two_languages();

    // When
    lang_init("fr");

    // Then
    ASSERT_EQ_STR(" Details ", T(S_DETAIL));
}

/* Vérifie que lang_init choisit une langue présente dans les variables d'environnement. */
TEST(lang_init_reads_the_language_from_the_environment) {
    // Given
    two_languages();
    clear_locale_environment();
    setenv("LANG", "fr_FR.UTF-8", 1);

    // When
    lang_init(NULL);

    // Then
    ASSERT_EQ_STR("Sans nom", T(S_NONAME));
}

/* Vérifie que LC_ALL a priorité sur LANG pour choisir la langue. */
TEST(lang_init_gives_lc_all_precedence_over_lang) {
    // Given
    two_languages();
    clear_locale_environment();
    setenv("LC_ALL", "en_US.UTF-8", 1);
    setenv("LANG", "fr_FR.UTF-8", 1);

    // When
    lang_init(NULL);

    // Then
    ASSERT_EQ_STR("Unnamed", T(S_NONAME));
}

/* Vérifie qu'une variable de langue définie mais inconnue bloque les suivantes. */
TEST(lang_init_does_not_look_further_when_the_first_defined_variable_is_unavailable) {
    // Given
    two_languages();
    clear_locale_environment();
    setenv("LC_ALL", "de_DE.UTF-8", 1);
    setenv("LANG", "fr_FR.UTF-8", 1);

    // When
    lang_init(NULL);

    // Then
    ASSERT_EQ_STR("Unnamed", T(S_NONAME));
}

/* Vérifie qu'un code forcé inconnu laisse la sélection retomber sur l'environnement. */
TEST(lang_init_falls_back_to_the_environment_when_the_forced_language_is_unknown) {
    // Given
    two_languages();
    clear_locale_environment();
    setenv("LANG", "fr_FR.UTF-8", 1);

    // When
    lang_init("xx");

    // Then
    ASSERT_EQ_STR("Sans nom", T(S_NONAME));
}

/* Vérifie que l'anglais est choisi lorsqu'aucune préférence de langue n'existe. */
TEST(lang_init_defaults_to_english_without_any_locale_information) {
    // Given
    two_languages();
    clear_locale_environment();

    // When
    lang_init(NULL);

    // Then
    ASSERT_EQ_STR("Unnamed", T(S_NONAME));
}

/* Vérifie qu'une nouvelle initialisation ne conserve pas les anciens catalogues. */
TEST(lang_init_called_again_forgets_the_previous_catalogs) {
    // Given
    const char *first = fx_tempdir();
    fx_write(first, "en.lang", "NONAME=One\nDETAIL=Old\n");
    setenv("GEDVIEW_LANG_DIR", first, 1);
    lang_init("en");
    const char *second = fx_tempdir();
    fx_write(second, "en.lang", "NONAME=Two\n");
    setenv("GEDVIEW_LANG_DIR", second, 1);

    // When
    lang_init("en");

    // Then
    ASSERT_EQ_STR("Two", T(S_NONAME));
    ASSERT_EQ_STR("DETAIL", T(S_DETAIL));
}

/* Vérifie que lang_cycle passe à la langue suivante disponible. */
TEST(lang_cycle_selects_the_next_language) {
    // Given
    two_languages();
    lang_init("en");

    // When
    lang_cycle();

    // Then
    ASSERT_EQ_STR("Sans nom", T(S_NONAME));
}

/* Vérifie que lang_cycle revient à la première langue après la dernière. */
TEST(lang_cycle_wraps_around_to_the_first_language) {
    // Given
    two_languages();
    lang_init("fr");

    // When
    lang_cycle();

    // Then
    ASSERT_EQ_STR("Unnamed", T(S_NONAME));
}

/* Vérifie que tag_label retourne le libellé de la langue active. */
TEST(tag_label_uses_the_selected_language) {
    // Given
    two_languages();
    lang_init("fr");

    // When
    const char *label = tag_label("NAME");

    // Then
    ASSERT_EQ_STR("Nom", label);
}

/* Vérifie qu'un libellé de tag absent dans la langue active retombe sur l'anglais. */
TEST(tag_label_falls_back_to_english_when_the_selected_language_lacks_the_tag) {
    // Given
    two_languages();
    lang_init("fr");

    // When
    const char *label = tag_label("DATE");

    // Then
    ASSERT_EQ_STR("Date", label);
}

/* Vérifie qu'un tag inconnu de tous les catalogues est retourné tel quel. */
TEST(tag_label_returns_the_tag_itself_when_no_language_knows_it) {
    // Given
    two_languages();
    lang_init("fr");

    // When
    const char *label = tag_label("ZZZZ");

    // Then
    ASSERT_EQ_STR("ZZZZ", label);
}

/* Enregistre les scénarios de langue et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("i18n/i18n");
    RUN_TEST(t_returns_the_key_name_when_no_language_defines_the_message);
    RUN_TEST(t_returns_an_empty_string_for_the_blank_message);
    RUN_TEST(lang_init_uses_the_forced_language);
    RUN_TEST(lang_init_matches_a_forced_locale_code_by_its_language_prefix);
    RUN_TEST(t_falls_back_to_english_when_the_selected_language_lacks_the_message);
    RUN_TEST(lang_init_reads_the_language_from_the_environment);
    RUN_TEST(lang_init_gives_lc_all_precedence_over_lang);
    RUN_TEST(lang_init_does_not_look_further_when_the_first_defined_variable_is_unavailable);
    RUN_TEST(lang_init_falls_back_to_the_environment_when_the_forced_language_is_unknown);
    RUN_TEST(lang_init_defaults_to_english_without_any_locale_information);
    RUN_TEST(lang_init_called_again_forgets_the_previous_catalogs);
    RUN_TEST(lang_cycle_selects_the_next_language);
    RUN_TEST(lang_cycle_wraps_around_to_the_first_language);
    RUN_TEST(tag_label_uses_the_selected_language);
    RUN_TEST(tag_label_falls_back_to_english_when_the_selected_language_lacks_the_tag);
    RUN_TEST(tag_label_returns_the_tag_itself_when_no_language_knows_it);
    return tl_finish();
}
