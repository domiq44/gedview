#include <string.h>

#include "i18n/catalog.h"
#include "support/fixtures.h"
#include "support/testlib.h"

/* Écrit `content` dans un en.lang temporaire et le charge dans `c`. */
static const char *load_en(Catalog *c, const char *content) {
    const char *dir = fx_tempdir();
    fx_write(dir, "en.lang", content);
    memset(c, 0, sizeof *c);
    catalog_load(c, dir, "en");
    return dir;
}

/* Vérifie qu'une valeur de traduction est chargée par sa clé. */
TEST(catalog_load_reads_a_message_by_key) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=Unnamed\n");

    // Then
    ASSERT_EQ_STR("Unnamed", catalog_msg(&c, S_NONAME));
}

/* Vérifie que le message signalant un terminal trop petit est chargé. */
TEST(catalog_load_reads_the_small_terminal_message) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "SMALLSCREEN=Resize terminal\n");

    // Then
    ASSERT_EQ_STR("Resize terminal", catalog_msg(&c, S_SMALLSCREEN));
}

/* Vérifie qu'une clé absente du catalogue retourne NULL. */
TEST(catalog_msg_returns_null_for_a_key_not_in_the_file) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=Unnamed\n");

    // Then
    ASSERT_NULL(catalog_msg(&c, S_DETAIL));
}

/* Vérifie que les commentaires et lignes sans signe égal ne créent pas d'entrée. */
TEST(catalog_load_ignores_comments_and_lines_without_equals_sign) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "# NONAME=commented out\nthis is not an entry\n\nDETAIL=Details\n");

    // Then
    ASSERT_NULL(catalog_msg(&c, S_NONAME));
    ASSERT_EQ_STR("Details", catalog_msg(&c, S_DETAIL));
}

/* Vérifie que les clés inconnues sont ignorées sans perturber les clés valides. */
TEST(catalog_load_ignores_unknown_keys) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "BOGUS=1\nNONAME=Unnamed\n");

    // Then
    ASSERT_EQ_STR("Unnamed", catalog_msg(&c, S_NONAME));
    ASSERT_EQ_INT(0, c.ntags);
}

/* Vérifie que la séquence échappée \n devient un retour à la ligne. */
TEST(catalog_load_turns_backslash_n_into_a_newline) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NOINDI=None found.\\n\n");

    // Then
    ASSERT_EQ_STR("None found.\n", catalog_msg(&c, S_NOINDI));
}

/* Vérifie qu'une double barre oblique inversée devient un seul caractère. */
TEST(catalog_load_turns_a_double_backslash_into_a_single_one) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=a\\\\b\n");

    // Then
    ASSERT_EQ_STR("a\\b", catalog_msg(&c, S_NONAME));
}

/* Vérifie que le chargement conserve les espaces autour de la valeur. */
TEST(catalog_load_keeps_leading_and_trailing_spaces) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "DETAIL= Details \n");

    // Then
    ASSERT_EQ_STR(" Details ", catalog_msg(&c, S_DETAIL));
}

/* Vérifie que seul le premier signe égal sépare la clé de sa valeur. */
TEST(catalog_load_splits_at_the_first_equals_sign_only) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=a=b\n");

    // Then
    ASSERT_EQ_STR("a=b", catalog_msg(&c, S_NONAME));
}

/* Vérifie que les fins de ligne CRLF sont retirées des valeurs chargées. */
TEST(catalog_load_strips_windows_line_endings) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=Unnamed\r\nDETAIL=Details\r\n");

    // Then
    ASSERT_EQ_STR("Unnamed", catalog_msg(&c, S_NONAME));
    ASSERT_EQ_STR("Details", catalog_msg(&c, S_DETAIL));
}

/* Vérifie qu'une valeur de traduction dépasse le tampon initial sans être tronquée. */
TEST(catalog_load_reads_a_message_longer_than_the_initial_buffer) {
    // Given
    char content[3200];
    char expected[3001];
    memcpy(content, "NONAME=", 7);
    memset(content + 7, 'x', 3000);
    content[3007] = 0;
    memset(expected, 'x', 3000);
    expected[3000] = 0;
    const char *dir = fx_tempdir();
    fx_write(dir, "en.lang", content);
    Catalog c TL_AUTOCLEAN(catalog_clear) = {0};

    // When
    catalog_load(&c, dir, "en");

    // Then
    ASSERT_EQ_STR(expected, catalog_msg(&c, S_NONAME));
}

/* Vérifie que les libellés des tags GEDCOM sont chargés par tag. */
TEST(catalog_load_reads_tag_labels) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "TAG.NAME=Name\nTAG.BIRT=Birth\n");

    // Then
    ASSERT_EQ_STR("Name", catalog_tag(&c, "NAME"));
    ASSERT_EQ_STR("Birth", catalog_tag(&c, "BIRT"));
}

/* Vérifie qu'un tag sans libellé dans le catalogue retourne NULL. */
TEST(catalog_tag_returns_null_for_an_unknown_tag) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "TAG.NAME=Name\n");

    // Then
    ASSERT_NULL(catalog_tag(&c, "DATE"));
}

/* Vérifie qu'un nom de tag trop long pour la table est ignoré. */
TEST(catalog_load_ignores_a_tag_name_too_long_for_the_table) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "TAG.TOOLONGTAG=x\n");

    // Then
    ASSERT_EQ_INT(0, c.ntags);
}

/* Vérifie que les conversions printf autorisées sont acceptées pour chaque message. */
TEST(catalog_load_accepts_the_conversion_a_message_is_meant_to_have) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "T_INDI= Individuals (%d) \nSEARCH=Search: %s\n");

    // Then
    ASSERT_EQ_STR(" Individuals (%d) ", catalog_msg(&c, S_T_INDI));
    ASSERT_EQ_STR("Search: %s", catalog_msg(&c, S_SEARCH));
}

/* Vérifie qu'une conversion incompatible avec le type attendu est rejetée. */
TEST(catalog_load_rejects_a_conversion_of_the_wrong_type) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "T_INDI=%s\nSEARCH=%d\n");

    // Then
    ASSERT_NULL(catalog_msg(&c, S_T_INDI));
    ASSERT_NULL(catalog_msg(&c, S_SEARCH));
}

/* Vérifie que la conversion dangereuse %n est rejetée. */
TEST(catalog_load_rejects_a_dangerous_conversion) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "SEARCH=%n\n");

    // Then
    ASSERT_NULL(catalog_msg(&c, S_SEARCH));
}

/* Vérifie qu'un message contenant deux conversions est rejeté. */
TEST(catalog_load_rejects_a_message_with_two_conversions) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "T_INDI=%d %d\n");

    // Then
    ASSERT_NULL(catalog_msg(&c, S_T_INDI));
}

/* Vérifie qu'une conversion est rejetée pour un message sans argument printf. */
TEST(catalog_load_rejects_a_conversion_in_a_message_that_takes_none) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=100%\nDETAIL=%s\n");

    // Then
    ASSERT_NULL(catalog_msg(&c, S_NONAME));
    ASSERT_NULL(catalog_msg(&c, S_DETAIL));
}

/* Vérifie qu'un pourcentage échappé est accepté comme texte littéral. */
TEST(catalog_load_accepts_an_escaped_percent_sign) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);

    // When
    load_en(&c, "NONAME=100%%\n");

    // Then
    ASSERT_EQ_STR("100%%", catalog_msg(&c, S_NONAME));
}

/* Vérifie qu'une nouvelle valeur remplace celle déjà chargée pour la même clé. */
TEST(catalog_load_replaces_a_value_loaded_earlier) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);
    const char *dir = load_en(&c, "NONAME=First\n");
    fx_write(dir, "fr.lang", "NONAME=Second\n");

    // When
    catalog_load(&c, dir, "fr");

    // Then
    ASSERT_EQ_STR("Second", catalog_msg(&c, S_NONAME));
}

/* Vérifie qu'un chargement complémentaire conserve les clés qu'il ne redéfinit pas. */
TEST(catalog_load_keeps_values_the_second_file_does_not_define) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);
    const char *dir = load_en(&c, "NONAME=First\nDETAIL=Details\n");
    fx_write(dir, "fr.lang", "NONAME=Second\n");

    // When
    catalog_load(&c, dir, "fr");

    // Then
    ASSERT_EQ_STR("Details", catalog_msg(&c, S_DETAIL));
}

/* Vérifie qu'un fichier de langue manquant ne modifie pas le catalogue existant. */
TEST(catalog_load_leaves_the_catalog_untouched_when_the_file_is_missing) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);
    const char *dir = load_en(&c, "NONAME=Unnamed\n");

    // When
    catalog_load(&c, dir, "xx");

    // Then
    ASSERT_EQ_STR("Unnamed", catalog_msg(&c, S_NONAME));
}

/* Vérifie que catalog_clear libère et efface les messages et libellés de tags. */
TEST(catalog_clear_removes_every_message_and_tag) {
    // Given
    Catalog c TL_AUTOCLEAN(catalog_clear);
    load_en(&c, "NONAME=Unnamed\nTAG.NAME=Name\n");

    // When
    catalog_clear(&c);

    // Then
    ASSERT_NULL(catalog_msg(&c, S_NONAME));
    ASSERT_NULL(catalog_tag(&c, "NAME"));
    ASSERT_EQ_INT(0, c.ntags);
}

/* Vérifie que l'identifiant d'une clé correspond à son nom dans les fichiers .lang. */
TEST(catalog_key_name_is_the_name_used_in_lang_files) {
    // Given
    const int id = S_T_INDI;

    // When
    const char *name = catalog_key_name(id);

    // Then
    ASSERT_EQ_STR("T_INDI", name);
}

/* Enregistre les scénarios du catalogue et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("i18n/catalog");
    RUN_TEST(catalog_load_reads_a_message_by_key);
    RUN_TEST(catalog_load_reads_the_small_terminal_message);
    RUN_TEST(catalog_msg_returns_null_for_a_key_not_in_the_file);
    RUN_TEST(catalog_load_ignores_comments_and_lines_without_equals_sign);
    RUN_TEST(catalog_load_ignores_unknown_keys);
    RUN_TEST(catalog_load_turns_backslash_n_into_a_newline);
    RUN_TEST(catalog_load_turns_a_double_backslash_into_a_single_one);
    RUN_TEST(catalog_load_keeps_leading_and_trailing_spaces);
    RUN_TEST(catalog_load_splits_at_the_first_equals_sign_only);
    RUN_TEST(catalog_load_strips_windows_line_endings);
    RUN_TEST(catalog_load_reads_a_message_longer_than_the_initial_buffer);
    RUN_TEST(catalog_load_reads_tag_labels);
    RUN_TEST(catalog_tag_returns_null_for_an_unknown_tag);
    RUN_TEST(catalog_load_ignores_a_tag_name_too_long_for_the_table);
    RUN_TEST(catalog_load_accepts_the_conversion_a_message_is_meant_to_have);
    RUN_TEST(catalog_load_rejects_a_conversion_of_the_wrong_type);
    RUN_TEST(catalog_load_rejects_a_dangerous_conversion);
    RUN_TEST(catalog_load_rejects_a_message_with_two_conversions);
    RUN_TEST(catalog_load_rejects_a_conversion_in_a_message_that_takes_none);
    RUN_TEST(catalog_load_accepts_an_escaped_percent_sign);
    RUN_TEST(catalog_load_replaces_a_value_loaded_earlier);
    RUN_TEST(catalog_load_keeps_values_the_second_file_does_not_define);
    RUN_TEST(catalog_load_leaves_the_catalog_untouched_when_the_file_is_missing);
    RUN_TEST(catalog_clear_removes_every_message_and_tag);
    RUN_TEST(catalog_key_name_is_the_name_used_in_lang_files);
    return tl_finish();
}
