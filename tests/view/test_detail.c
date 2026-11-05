#include <stdlib.h>
#include <string.h>

#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/detail.h"

/* Sans catalogue chargé, tag_label() renvoie le tag lui-même : "NAME: valeur". */

/* Parse le GEDCOM de scénario et reconstruit le détail de l'enregistrement xref. */
static void build(Detail *d, const char *text, Kind kind, const char *xref) {
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(text, &db);
    detail_build(d, &db, kind, db_find(&db, xref));
}

/* Vérifie que le détail commence par le libellé et le XREF de l'enregistrement. */
TEST(detail_starts_with_a_header_line_made_of_the_label_and_the_xref) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("DUPONT, Jean  @I1@", d.lines[0].text);
    detail_destroy(&d);
}

/* Vérifie qu'une ligne vide sépare l'en-tête du contenu du détail. */
TEST(detail_header_is_followed_by_a_blank_line) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("", d.lines[1].text);
    detail_destroy(&d);
}

/* Vérifie que le nom affiché dans le détail ne contient pas les barres GEDCOM. */
TEST(detail_shows_name_without_slashes) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("NAME: DUPONT, Jean", d.lines[2].text);
    ASSERT_EQ_STR("", d.lines[2].link);
    detail_destroy(&d);
}

/* Vérifie qu'une valeur ordinaire est affichée sans lien navigable. */
TEST(detail_shows_plain_values_without_link) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 OCCU Farmer\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("OCCU: Farmer", d.lines[2].text);
    ASSERT_EQ_STR("", d.lines[2].link);
    detail_destroy(&d);
}

/* Vérifie l'indentation des sous-niveaux et l'absence de deux-points pour une valeur vide. */
TEST(detail_indents_sub_levels_and_omits_colon_for_empty_values) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 BIRT\n2 DATE 1 JAN 1900\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("BIRT", d.lines[2].text);
    ASSERT_EQ_STR("  DATE: 1 JAN 1900", d.lines[3].text);
    ASSERT_EQ_INT(4, d.count);
    detail_destroy(&d);
}

/* Vérifie que les segments CONT d'une valeur occupent des lignes de détail séparées. */
TEST(detail_spreads_a_multi_line_value_over_several_lines) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 NOTE first\n2 CONT second\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("NOTE: first", d.lines[2].text);
    ASSERT_EQ_STR("  second", d.lines[3].text);
    detail_destroy(&d);
}

/* Vérifie que le détail d'une note inclut son texte complet, y compris les CONT. */
TEST(detail_of_a_note_record_shows_its_full_text) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @N1@ NOTE first\n1 CONT second\n", K_NOTE, "@N1@");

    // Then
    ASSERT_EQ_STR("first  @N1@", d.lines[0].text);
    ASSERT_EQ_STR("TEXT: first", d.lines[2].text);
    ASSERT_EQ_STR("  second", d.lines[3].text);
    detail_destroy(&d);
}

/* Vérifie que le détail d'une citation montre sa source, son origine et ses informations de page. */
TEST(detail_shows_source_citation_and_origin_links) {
    // Given
    const char *text = "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n1 BIRT\n2 SOUR @X1@\n3 PAGE 10\n3 QUAY 2\n"
                       "0 @X1@ SOUR\n1 TITL Registre\n";
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(text, &db);
    const Node *individual = db_find(&db, "@I1@");
    const Node *citation = node_sub(node_sub(individual, "BIRT"), "SOUR");
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    detail_build(&d, &db, K_CITA, citation);

    // Then
    ASSERT_EQ_STR("DUPONT, Jean - BIRT - Registre  @X1@", d.lines[0].text);
    ASSERT_EQ_STR("SOUR: Registre", d.lines[2].text);
    ASSERT_EQ_STR("@X1@", d.lines[2].link);
    ASSERT_EQ_STR("ORIGIN: DUPONT, Jean", d.lines[3].text);
    ASSERT_EQ_STR("@I1@", d.lines[3].link);
    ASSERT_EQ_STR("PAGE: 10", d.lines[4].text);
    ASSERT_EQ_STR("QUAY: 2", d.lines[5].text);
    detail_destroy(&d);
}

/* Vérifie que les URL HTTP(S) dans une note deviennent des liens sans ponctuation finale. */
TEST(detail_marks_http_urls_in_notes_as_links) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const char *text = "0 @N1@ NOTE HTTPS https://example.test/page,\n"
                       "1 CONT HTTP http://example.test/image.jpg.";

    // When
    build(&d, text, K_NOTE, "@N1@");

    // Then
    ASSERT_EQ_STR("https://example.test/page", d.lines[2].link);
    ASSERT_EQ_INT(12, (int)d.lines[2].link_start);
    ASSERT_EQ_INT((int)strlen("https://example.test/page"), (int)d.lines[2].link_length);
    ASSERT_EQ_STR("http://example.test/image.jpg", d.lines[3].link);
    ASSERT_EQ_INT(7, (int)d.lines[3].link_start);
    ASSERT_EQ_INT((int)strlen("http://example.test/image.jpg"), (int)d.lines[3].link_length);
    detail_destroy(&d);
}

/* Vérifie que le chemin FILE d'un objet multimédia est présenté comme un lien média. */
TEST(detail_marks_media_file_paths_as_links) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @M1@ OBJE\n1 FILE media/portrait.jpg\n", K_OBJ, "@M1@");

    // Then
    ASSERT_EQ_STR("media/portrait.jpg", d.lines[2].link);
    ASSERT_EQ_INT(LINE_LINK_MEDIA, d.lines[2].link_kind);
    ASSERT_EQ_INT(6, (int)d.lines[2].link_start);
    ASSERT_EQ_INT((int)strlen("media/portrait.jpg"), (int)d.lines[2].link_length);
    detail_destroy(&d);
}

/* Vérifie qu'une référence à un individu affiche son nom et conserve son lien XREF. */
TEST(detail_shows_a_reference_to_an_individual_as_a_link_with_the_target_name) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const char *text = "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n0 @F1@ FAM\n1 HUSB @I1@\n";

    // When
    build(&d, text, K_FAM, "@F1@");

    // Then
    ASSERT_EQ_STR("HUSB: DUPONT, Jean", d.lines[2].text);
    ASSERT_EQ_STR("@I1@", d.lines[2].link);
    ASSERT_EQ_INT(6, (int)d.lines[2].link_start);
    ASSERT_EQ_INT((int)strlen("DUPONT, Jean"), (int)d.lines[2].link_length);
    detail_destroy(&d);
}

/* Vérifie qu'une référence à une source affiche son titre et reste cliquable. */
TEST(detail_shows_a_reference_to_a_source_with_its_title) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const char *text = "0 @I1@ INDI\n1 SOUR @S1@\n0 @S1@ SOUR\n1 TITL Registre paroissial\n";

    // When
    build(&d, text, K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("SOUR: Registre paroissial", d.lines[2].text);
    ASSERT_EQ_STR("@S1@", d.lines[2].link);
    detail_destroy(&d);
}

/* Vérifie qu'une référence inconnue reste en texte brut sans lien. */
TEST(detail_shows_a_reference_to_an_unknown_record_as_raw_text_without_link) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    // When
    build(&d, "0 @I1@ INDI\n1 NOTE @N9@\n", K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("NOTE: @N9@", d.lines[2].text);
    ASSERT_EQ_STR("", d.lines[2].link);
    detail_destroy(&d);
}

/* Vérifie qu'une référence à une famille est affichée comme XREF cliquable. */
TEST(detail_shows_the_family_reference_itself_as_a_link) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const char *text = "0 @I1@ INDI\n1 FAMS @F1@\n0 @F1@ FAM\n";

    // When
    build(&d, text, K_INDI, "@I1@");

    // Then
    ASSERT_EQ_STR("FAMS: @F1@", d.lines[2].text);
    ASSERT_EQ_STR("@F1@", d.lines[2].link);
    detail_destroy(&d);
}

/* Vérifie qu'une ligne FAMC développe les membres et le mariage de la famille. */
TEST(detail_expands_the_family_under_a_famc_line) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const char *text = "0 @I1@ INDI\n1 NAME Kid/X/\n1 FAMC @F1@\n"
                       "0 @I2@ INDI\n1 NAME Dad/D/\n0 @I3@ INDI\n1 NAME Mom/M/\n"
                       "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I3@\n1 CHIL @I1@\n1 MARR\n2 DATE 1990\n";

    // When
    build(&d, text, K_INDI, "@I1@");

    // Then
    ASSERT_EQ_INT(9, d.count);
    ASSERT_EQ_STR("FAMC: @F1@", d.lines[3].text);
    ASSERT_EQ_STR("  HUSB: D, Dad", d.lines[4].text);
    ASSERT_EQ_STR("@I2@", d.lines[4].link);
    ASSERT_EQ_INT(8, (int)d.lines[4].link_start);
    ASSERT_EQ_INT((int)strlen("D, Dad"), (int)d.lines[4].link_length);
    ASSERT_EQ_STR("  WIFE: M, Mom", d.lines[5].text);
    ASSERT_EQ_STR("  CHIL: X, Kid", d.lines[6].text);
    ASSERT_EQ_STR("  MARR", d.lines[7].text);
    ASSERT_EQ_STR("    DATE: 1990", d.lines[8].text);
    detail_destroy(&d);
}

/* Vérifie que le détail d'une famille n'ajoute pas une seconde expansion de ses membres. */
TEST(detail_does_not_expand_the_family_of_a_family_record) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const char *text = "0 @I1@ INDI\n1 NAME Dad/D/\n0 @F1@ FAM\n1 HUSB @I1@\n";

    // When
    build(&d, text, K_FAM, "@F1@");

    // Then
    ASSERT_EQ_INT(3, d.count);
    detail_destroy(&d);
}

/* Vérifie qu'une nouvelle construction remplace les anciennes lignes du détail. */
TEST(detail_build_replaces_the_previous_content) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME A/B/\n1 SEX M\n1 OCCU X\n0 @I2@ INDI\n1 NAME C/D/\n", &db);
    detail_build(&d, &db, K_INDI, db_find(&db, "@I1@"));

    // When
    detail_build(&d, &db, K_INDI, db_find(&db, "@I2@"));

    // Then
    ASSERT_EQ_INT(3, d.count);
    ASSERT_EQ_STR("D, C  @I2@", d.lines[0].text);
    detail_destroy(&d);
}

/* Vérifie qu'un tag long est tronqué à la taille de ligne sans débordement mémoire. */
TEST(detail_truncates_a_long_tag_without_writing_past_the_line_buffer) {
    // Given
    char text[3100];
    const char *prefix = "0 @I1@ INDI\n1 ";
    size_t prefix_len = strlen(prefix);
    memcpy(text, prefix, prefix_len);
    memset(text + prefix_len, 'X', 3000);
    text[prefix_len + 3000] = '\n';
    text[prefix_len + 3001] = 0;
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(text, &db);
    Detail d TL_AUTOCLEAN(detail_destroy) = {.lines = malloc(sizeof(Line)), .cap = 1};
    ASSERT_NOT_NULL(d.lines);

    // When
    detail_build(&d, &db, K_INDI, db_find(&db, "@I1@"));

    // Then
    ASSERT_EQ_INT((int)sizeof d.lines[2].text - 1, (int)strlen(d.lines[2].text));
    detail_destroy(&d);
}

/* Vérifie que detail_destroy libère les lignes et remet tous les champs à zéro. */
TEST(detail_destroy_resets_the_detail) {
    // Given
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    build(&d, "0 @I1@ INDI\n1 NAME Ada/LOVELACE/\n", K_INDI, "@I1@");

    // When
    detail_destroy(&d);

    // Then
    ASSERT_NULL(d.lines);
    ASSERT_EQ_INT(0, d.count);
    ASSERT_EQ_INT(0, d.cap);
}

/* Enregistre les scénarios de rendu et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("view/detail");
    RUN_TEST(detail_starts_with_a_header_line_made_of_the_label_and_the_xref);
    RUN_TEST(detail_header_is_followed_by_a_blank_line);
    RUN_TEST(detail_shows_name_without_slashes);
    RUN_TEST(detail_shows_plain_values_without_link);
    RUN_TEST(detail_indents_sub_levels_and_omits_colon_for_empty_values);
    RUN_TEST(detail_spreads_a_multi_line_value_over_several_lines);
    RUN_TEST(detail_of_a_note_record_shows_its_full_text);
    RUN_TEST(detail_shows_source_citation_and_origin_links);
    RUN_TEST(detail_marks_http_urls_in_notes_as_links);
    RUN_TEST(detail_marks_media_file_paths_as_links);
    RUN_TEST(detail_shows_a_reference_to_an_individual_as_a_link_with_the_target_name);
    RUN_TEST(detail_shows_a_reference_to_a_source_with_its_title);
    RUN_TEST(detail_shows_a_reference_to_an_unknown_record_as_raw_text_without_link);
    RUN_TEST(detail_shows_the_family_reference_itself_as_a_link);
    RUN_TEST(detail_expands_the_family_under_a_famc_line);
    RUN_TEST(detail_does_not_expand_the_family_of_a_family_record);
    RUN_TEST(detail_build_replaces_the_previous_content);
    RUN_TEST(detail_truncates_a_long_tag_without_writing_past_the_line_buffer);
    RUN_TEST(detail_destroy_resets_the_detail);
    return tl_finish();
}
