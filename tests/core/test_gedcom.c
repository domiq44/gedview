#include "core/gedcom.h"
#include "support/fixtures.h"
#include "support/testlib.h"

#include <stdio.h>
#include <string.h>

/* Vérifie que le parseur retourne NULL si le fichier demandé n'existe pas. */
TEST(parse_returns_null_when_file_does_not_exist) {
    // Given
    const char *path = "/nonexistent/dir/file.ged";
    size_t ignored_lines = 99;

    // When
    Node *root = gedcom_parse_with_warnings(path, &ignored_lines);

    // Then
    ASSERT_NULL(root);
    ASSERT_EQ_INT(0, (int)ignored_lines);
}

/* Vérifie qu'une erreur de lecture après ouverture du chemin retourne NULL. */
TEST(parse_returns_null_when_reading_a_directory_fails) {
    // Given
    const char *path = fx_tempdir();

    // When
    Node *root = gedcom_parse(path);

    // Then
    ASSERT_NULL(root);
}

/* Vérifie que les enregistrements de niveau zéro sont rattachés à une racine fictive. */
TEST(parse_returns_pseudo_root_above_level_zero_records) {
    // Given
    const char *text = "0 HEAD\n0 TRLR\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_INT(-1, root->level);
    ASSERT_EQ_STR("HEAD", root->child->tag);
    ASSERT_EQ_STR("TRLR", root->child->next->tag);
}

/* Vérifie que la dernière ligne est parsée même si elle ne se termine pas par LF. */
TEST(parse_reads_a_final_line_without_a_newline) {
    // Given
    const char *text = "0 HEAD";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("HEAD", root->child->tag);
}

/* Vérifie que le parseur sépare correctement XREF, tag et valeur. */
TEST(parse_reads_xref_tag_and_value) {
    // Given
    const char *text = "0 @I1@ INDI\n1 NAME Jean Pierre/DUPONT/\n";

    // When
    Node *root = fx_parse(text);

    // Then
    const Node *indi = root->child;
    ASSERT_EQ_STR("@I1@", indi->xref);
    ASSERT_EQ_STR("INDI", indi->tag);
    ASSERT_EQ_STR("", indi->value);
    ASSERT_EQ_STR("Jean Pierre/DUPONT/", indi->child->value);
}

/* Vérifie qu'un enregistrement sans identifiant conserve un XREF vide. */
TEST(parse_leaves_xref_empty_when_record_has_none) {
    // Given
    const char *text = "0 HEAD\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("", root->child->xref);
}

/* Vérifie que chaque ligne de niveau inférieur est attachée à son parent direct. */
TEST(parse_attaches_sublevels_to_their_parent) {
    // Given
    const char *text = "0 @I1@ INDI\n1 BIRT\n2 DATE 1 JAN 1900\n";

    // When
    Node *root = fx_parse(text);

    // Then
    const Node *birt = root->child->child;
    const Node *date = birt->child;
    ASSERT_EQ_STR("BIRT", birt->tag);
    ASSERT_EQ_STR("DATE", date->tag);
    ASSERT_EQ_STR("1 JAN 1900", date->value);
    ASSERT_TRUE(date->parent == birt);
}

/* Vérifie que les enfants d'un même parent gardent leur ordre dans le fichier. */
TEST(parse_keeps_siblings_in_file_order) {
    // Given
    const char *text = "0 @I1@ INDI\n1 NAME A\n1 SEX M\n1 BIRT\n";

    // When
    Node *root = fx_parse(text);

    // Then
    const Node *indi = root->child;
    ASSERT_EQ_STR("NAME", indi->child->tag);
    ASSERT_EQ_STR("SEX", indi->child->next->tag);
    ASSERT_EQ_STR("BIRT", indi->last->tag);
}

/* Vérifie qu'après une branche profonde le parseur reprend le parent de niveau inférieur. */
TEST(parse_returns_to_shallower_level_after_a_deep_branch) {
    // Given
    const char *text = "0 @I1@ INDI\n1 BIRT\n2 DATE 1900\n1 DEAT\n";

    // When
    Node *root = fx_parse(text);

    // Then
    const Node *indi = root->child;
    ASSERT_EQ_STR("DEAT", indi->child->next->tag);
    ASSERT_TRUE(indi->child->next->parent == indi);
}

/* Vérifie qu'une ligne CONC prolonge la valeur par concaténation directe. */
TEST(parse_merges_conc_line_into_parent_value_without_separator) {
    // Given
    const char *text = "0 @N1@ NOTE Hello \n1 CONC world\n";

    // When
    Node *root = fx_parse(text);

    // Then
    const Node *note = root->child;
    ASSERT_EQ_STR("Hello world", note->value);
    ASSERT_NULL(note->child);
}

/* Vérifie que chaque ligne CONT prolonge la valeur après un retour à la ligne. */
TEST(parse_merges_cont_line_into_parent_value_with_newline) {
    // Given
    const char *text = "0 @N1@ NOTE first\n1 CONT second\n1 CONT third\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("first\nsecond\nthird", root->child->value);
}

/* Vérifie que le BOM UTF-8 initial ne fait pas échouer la lecture du premier tag. */
TEST(parse_ignores_utf8_byte_order_mark) {
    // Given
    const char *text = "\xEF\xBB\xBF"
                       "0 HEAD\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_NOT_NULL(root->child);
    ASSERT_EQ_STR("HEAD", root->child->tag);
}

/* Vérifie que les fins de ligne CRLF ne sont pas conservées dans les valeurs. */
TEST(parse_strips_windows_line_endings) {
    // Given
    const char *text = "0 @I1@ INDI\r\n1 NAME Jean\r\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("Jean", root->child->child->value);
}

/* Vérifie qu'une valeur dépassant le tampon initial est lue intégralement. */
TEST(parse_preserves_values_longer_than_the_initial_line_buffer) {
    // Given
    char text[5024];
    const char *prefix = "0 @N1@ NOTE ";
    size_t prefix_len = strlen(prefix);
    memcpy(text, prefix, prefix_len);
    memset(text + prefix_len, 'x', 5000);
    text[prefix_len + 5000] = '\n';
    text[prefix_len + 5001] = 0;

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_INT(5000, (int)strlen(root->child->value));
    ASSERT_EQ_INT('x', root->child->value[4999]);
}

/* Vérifie que les lignes non GEDCOM sont ignorées sans interrompre le parsing. */
TEST(parse_skips_lines_that_do_not_start_with_a_level) {
    // Given
    const char *text = "garbage\n\n0 HEAD\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("HEAD", root->child->tag);
    ASSERT_NULL(root->child->next);
}

/* Vérifie que le parseur compte chaque ligne ignorée tout en conservant les lignes valides. */
TEST(parse_reports_the_number_of_ignored_lines) {
    // Given
    const char *dir = fx_tempdir();
    const char *filename = "warnings.ged";
    fx_write(dir, filename, "garbage\n0 HEAD\n2 DATE orphan\n");
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, filename);
    size_t ignored_lines = 99;

    // When
    Node *root = gedcom_parse_with_warnings(path, &ignored_lines);

    // Then
    ASSERT_NOT_NULL(root);
    ASSERT_EQ_INT(2, (int)ignored_lines);
    ASSERT_EQ_STR("HEAD", root->child->tag);
}

/* Vérifie que le destructeur accepte une racine NULL sans effet. */
TEST(gedcom_free_accepts_a_null_root) {
    // Given
    Node *root = NULL;

    // When
    gedcom_free(root);

    // Then
    ASSERT_NULL(root);
}

/* Vérifie que le destructeur parcourt aussi les enfants de la racine parsée. */
TEST(gedcom_free_releases_a_parsed_tree) {
    // Given
    Node *root = fx_parse("0 @I1@ INDI\n1 NAME Ada/LOVELACE/\n2 GIVN Ada\n");
    ASSERT_NOT_NULL(root->child->child->child);

    // When
    gedcom_free(root);

    // Then
    // L'appel doit parcourir l'arbre sans erreur.
}

/* Vérifie qu'une ligne contenant un niveau mais aucun tag est ignorée. */
TEST(parse_skips_a_line_without_a_tag) {
    // Given
    const char *text = "0\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_NULL(root->child);
}

/* Vérifie qu'une ligne orpheline est ignorée lorsque son niveau parent manque. */
TEST(parse_skips_lines_whose_parent_level_is_missing) {
    // Given
    const char *text = "0 HEAD\n2 DATE orphan\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_NULL(root->child->child);
}

/* Vérifie qu'un niveau supérieur à la profondeur prise en charge est ignoré. */
TEST(parse_skips_levels_beyond_the_supported_depth) {
    // Given
    const char *text = "0 HEAD\n99 DEEP x\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_NULL(root->child->child);
}

/* Vérifie que les espaces précédant le numéro de niveau sont acceptés. */
TEST(parse_ignores_spaces_before_the_level) {
    // Given
    const char *text = "   0 HEAD\n  1 SOUR app\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("HEAD", root->child->tag);
    ASSERT_EQ_STR("app", root->child->child->value);
}

/* Vérifie que plusieurs espaces entre le XREF et le tag sont acceptés. */
TEST(parse_accepts_several_spaces_between_the_xref_and_the_tag) {
    // Given
    const char *text = "0 @I1@   INDI\n";

    // When
    Node *root = fx_parse(text);

    // Then
    ASSERT_EQ_STR("@I1@", root->child->xref);
    ASSERT_EQ_STR("INDI", root->child->tag);
}

/* Vérifie que node_sub retourne le premier enfant correspondant au tag demandé. */
TEST(node_sub_returns_first_child_with_the_tag) {
    // Given
    Node *root = fx_parse("0 @I1@ INDI\n1 NOTE one\n1 NAME A\n1 NOTE two\n");
    const Node *indi = root->child;

    // When
    const Node *note = node_sub(indi, "NOTE");

    // Then
    ASSERT_NOT_NULL(note);
    ASSERT_EQ_STR("one", note->value);
}

/* Vérifie que node_sub retourne NULL quand aucun enfant direct ne porte ce tag. */
TEST(node_sub_returns_null_when_no_child_has_the_tag) {
    // Given
    Node *root = fx_parse("0 @I1@ INDI\n1 NAME A\n");

    // When
    const Node *sex = node_sub(root->child, "SEX");

    // Then
    ASSERT_NULL(sex);
}

/* Vérifie que node_sub ne recherche pas dans les petits-enfants du nœud. */
TEST(node_sub_does_not_look_into_grandchildren) {
    // Given
    Node *root = fx_parse("0 @I1@ INDI\n1 BIRT\n2 DATE 1900\n");

    // When
    const Node *date = node_sub(root->child, "DATE");

    // Then
    ASSERT_NULL(date);
}

/* Vérifie qu'une valeur encadrée par arobase est reconnue comme référence. */
TEST(is_ref_accepts_a_value_between_at_signs) {
    // Given
    const char *value = "@I12@";

    // When
    int result = gedcom_is_ref(value);

    // Then
    ASSERT_TRUE(result);
}

/* Vérifie qu'un texte ordinaire n'est pas reconnu comme référence GEDCOM. */
TEST(is_ref_rejects_plain_text) {
    // Given
    const char *value = "Jean";

    // When
    int result = gedcom_is_ref(value);

    // Then
    ASSERT_FALSE(result);
}

/* Vérifie que la paire d'arobases vide n'est pas une référence valide. */
TEST(is_ref_rejects_empty_pair_of_at_signs) {
    // Given
    const char *value = "@@";

    // When
    int result = gedcom_is_ref(value);

    // Then
    ASSERT_FALSE(result);
}

/* Vérifie qu'une référence avec arobase initial échappé est rejetée. */
TEST(is_ref_rejects_escaped_at_sign_prefix) {
    // Given
    const char *value = "@@mail@";

    // When
    int result = gedcom_is_ref(value);

    // Then
    ASSERT_FALSE(result);
}

/* Vérifie qu'une valeur sans arobase final n'est pas reconnue comme référence. */
TEST(is_ref_rejects_value_without_closing_at_sign) {
    // Given
    const char *value = "@I1";

    // When
    int result = gedcom_is_ref(value);

    // Then
    ASSERT_FALSE(result);
}

/* Enregistre les scénarios du parseur GEDCOM et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("core/gedcom");
    RUN_TEST(parse_returns_null_when_file_does_not_exist);
    RUN_TEST(parse_returns_null_when_reading_a_directory_fails);
    RUN_TEST(parse_returns_pseudo_root_above_level_zero_records);
    RUN_TEST(parse_reads_a_final_line_without_a_newline);
    RUN_TEST(parse_reads_xref_tag_and_value);
    RUN_TEST(parse_leaves_xref_empty_when_record_has_none);
    RUN_TEST(parse_attaches_sublevels_to_their_parent);
    RUN_TEST(parse_keeps_siblings_in_file_order);
    RUN_TEST(parse_returns_to_shallower_level_after_a_deep_branch);
    RUN_TEST(parse_merges_conc_line_into_parent_value_without_separator);
    RUN_TEST(parse_merges_cont_line_into_parent_value_with_newline);
    RUN_TEST(parse_ignores_utf8_byte_order_mark);
    RUN_TEST(parse_strips_windows_line_endings);
    RUN_TEST(parse_preserves_values_longer_than_the_initial_line_buffer);
    RUN_TEST(parse_skips_lines_that_do_not_start_with_a_level);
    RUN_TEST(parse_reports_the_number_of_ignored_lines);
    RUN_TEST(gedcom_free_accepts_a_null_root);
    RUN_TEST(gedcom_free_releases_a_parsed_tree);
    RUN_TEST(parse_skips_a_line_without_a_tag);
    RUN_TEST(parse_skips_lines_whose_parent_level_is_missing);
    RUN_TEST(parse_skips_levels_beyond_the_supported_depth);
    RUN_TEST(parse_ignores_spaces_before_the_level);
    RUN_TEST(parse_accepts_several_spaces_between_the_xref_and_the_tag);
    RUN_TEST(node_sub_returns_first_child_with_the_tag);
    RUN_TEST(node_sub_returns_null_when_no_child_has_the_tag);
    RUN_TEST(node_sub_does_not_look_into_grandchildren);
    RUN_TEST(is_ref_accepts_a_value_between_at_signs);
    RUN_TEST(is_ref_rejects_plain_text);
    RUN_TEST(is_ref_rejects_empty_pair_of_at_signs);
    RUN_TEST(is_ref_rejects_escaped_at_sign_prefix);
    RUN_TEST(is_ref_rejects_value_without_closing_at_sign);
    return tl_finish();
}
