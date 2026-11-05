#include "core/database.h"
#include "support/fixtures.h"
#include "support/testlib.h"

/* Résout xref dans db puis calcule son libellé de test, ou une chaîne vide s'il manque. */
static void label_of(const Database *db, Kind kind, const char *xref, char *out, size_t sz) {
    record_label(db, kind, db_find(db, xref), out, sz);
}

/* --- format_name --- */

/* Vérifie que le patronyme précède le prénom et en est séparé par une virgule. */
TEST(format_name_places_surname_before_given_name) {
    // Given
    const char *raw = "Jean/DUPONT/";
    char out[64];

    // When
    format_name(raw, out, sizeof out);

    // Then
    ASSERT_EQ_STR("DUPONT, Jean", out);
}

/* Vérifie qu'un patronyme initial garde la même présentation Nom, Prénom. */
TEST(format_name_formats_surname_first_input_with_a_comma) {
    // Given
    const char *raw = "/DUPONT/Jean";
    char out[64];

    // When
    format_name(raw, out, sizeof out);

    // Then
    ASSERT_EQ_STR("DUPONT, Jean", out);
}

/* Vérifie qu'un nom GEDCOM vide produit une chaîne vide. */
TEST(format_name_returns_empty_string_when_name_is_empty) {
    // Given
    const char *raw = "//";
    char out[64] = "junk";

    // When
    format_name(raw, out, sizeof out);

    // Then
    ASSERT_EQ_STR("", out);
}

/* Vérifie que format_name tronque le résultat sans dépasser le tampon fourni. */
TEST(format_name_truncates_to_the_output_size) {
    // Given
    const char *raw = "Jean/DUPONT/";
    char out[5];

    // When
    format_name(raw, out, sizeof out);

    // Then
    ASSERT_EQ_STR("DUPO", out);
}

/* --- kind_from_tag --- */

/* Vérifie que chaque tag d'enregistrement connu correspond à son enum Kind. */
TEST(kind_from_tag_maps_every_record_tag_to_its_kind) {
    // Given
    const char *tags[] = {"INDI", "FAM", "SOUR", "REPO", "OBJE", "NOTE"};
    const Kind kinds[] = {K_INDI, K_FAM, K_SRC, K_REPO, K_OBJ, K_NOTE};

    for (int i = 0; i < 6; i++) {
        // When
        int kind = kind_from_tag(tags[i]);

        // Then
        ASSERT_EQ_INT(kinds[i], kind);
    }
}

/* Vérifie qu'un tag d'en-tête non pris en charge retourne -1. */
TEST(kind_from_tag_returns_minus_one_for_non_record_tags) {
    // Given
    const char *tag = "HEAD";

    // When
    int kind = kind_from_tag(tag);

    // Then
    ASSERT_EQ_INT(-1, kind);
}

/* --- record_label --- */

/* Vérifie que le libellé d'un individu est son nom GEDCOM formaté. */
TEST(record_label_of_individual_is_the_formatted_name) {
    // Given
    Database db;
    fx_database("0 @I1@ INDI\n1 NAME Marie/CURIE/\n", &db);
    char out[64];

    // When
    label_of(&db, K_INDI, "@I1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("CURIE, Marie", out);
}

/* Vérifie qu'un individu sans tag NAME reçoit un libellé vide. */
TEST(record_label_of_individual_without_name_is_empty) {
    // Given
    Database db;
    fx_database("0 @I1@ INDI\n", &db);
    char out[64] = "junk";

    // When
    label_of(&db, K_INDI, "@I1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("", out);
}

/* Vérifie que le libellé d'une famille réunit les noms des deux conjoints. */
TEST(record_label_of_family_joins_husband_and_wife_names) {
    // Given
    Database db;
    fx_database("0 @I1@ INDI\n1 NAME Pierre/CURIE/\n0 @I2@ INDI\n1 NAME Marie/CURIE/\n"
                "0 @F1@ FAM\n1 HUSB @I1@\n1 WIFE @I2@\n",
                &db);
    char out[128];

    // When
    label_of(&db, K_FAM, "@F1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("CURIE, Pierre & CURIE, Marie", out);
}

/* Vérifie qu'un conjoint référencé mais introuvable est indiqué par un point d'interrogation. */
TEST(record_label_of_family_shows_question_mark_for_a_missing_spouse) {
    // Given
    Database db;
    fx_database("0 @I1@ INDI\n1 NAME Pierre/CURIE/\n0 @F1@ FAM\n1 HUSB @I1@\n1 WIFE @I9@\n", &db);
    char out[128];

    // When
    label_of(&db, K_FAM, "@F1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("CURIE, Pierre & ?", out);
}

/* Vérifie qu'un conjoint sans nom affichable est remplacé par un point d'interrogation. */
TEST(record_label_of_family_shows_question_mark_for_a_nameless_spouse) {
    // Given
    Database db;
    fx_database("0 @I1@ INDI\n0 @F1@ FAM\n1 WIFE @I1@\n", &db);
    char out[128];

    // When
    label_of(&db, K_FAM, "@F1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("? & ?", out);
}

/* Vérifie que le titre d'une source prévaut sur son abréviation. */
TEST(record_label_of_source_prefers_title_over_abbreviation) {
    // Given
    Database db;
    fx_database("0 @S1@ SOUR\n1 ABBR Reg\n1 TITL Registre paroissial\n", &db);
    char out[64];

    // When
    label_of(&db, K_SRC, "@S1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("Registre paroissial", out);
}

/* Vérifie que l'abréviation sert de libellé si la source n'a pas de titre. */
TEST(record_label_of_source_falls_back_to_abbreviation) {
    // Given
    Database db;
    fx_database("0 @S1@ SOUR\n1 ABBR Reg\n", &db);
    char out[64];

    // When
    label_of(&db, K_SRC, "@S1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("Reg", out);
}

/* Vérifie que le XREF sert de libellé si une source n'a ni titre ni abréviation. */
TEST(record_label_of_source_falls_back_to_xref) {
    // Given
    Database db;
    fx_database("0 @S1@ SOUR\n", &db);
    char out[64];

    // When
    label_of(&db, K_SRC, "@S1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("@S1@", out);
}

/* Vérifie que le nom est utilisé comme libellé d'un dépôt. */
TEST(record_label_of_repository_is_its_name) {
    // Given
    Database db;
    fx_database("0 @R1@ REPO\n1 NAME Archives departementales\n", &db);
    char out[64];

    // When
    label_of(&db, K_REPO, "@R1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("Archives departementales", out);
}

/* Vérifie que le XREF remplace le nom absent d'un dépôt. */
TEST(record_label_of_repository_without_name_is_its_xref) {
    // Given
    Database db;
    fx_database("0 @R1@ REPO\n", &db);
    char out[64];

    // When
    label_of(&db, K_REPO, "@R1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("@R1@", out);
}

/* Vérifie que le titre d'un média prévaut sur son nom de fichier. */
TEST(record_label_of_media_prefers_title_over_file) {
    // Given
    Database db;
    fx_database("0 @M1@ OBJE\n1 FILE photo.jpg\n1 TITL Portrait\n", &db);
    char out[64];

    // When
    label_of(&db, K_OBJ, "@M1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("Portrait", out);
}

/* Vérifie que le média utilise d'abord le fichier, puis son XREF en dernier recours. */
TEST(record_label_of_media_falls_back_to_file_then_xref) {
    // Given
    Database db;
    fx_database("0 @M1@ OBJE\n1 FILE photo.jpg\n0 @M2@ OBJE\n", &db);
    char with_file[64], without[64];

    // When
    label_of(&db, K_OBJ, "@M1@", with_file, sizeof with_file);
    label_of(&db, K_OBJ, "@M2@", without, sizeof without);

    // Then
    ASSERT_EQ_STR("photo.jpg", with_file);
    ASSERT_EQ_STR("@M2@", without);
}

/* Vérifie que le libellé d'une note ne contient que sa première ligne. */
TEST(record_label_of_note_is_its_first_line) {
    // Given
    Database db;
    fx_database("0 @N1@ NOTE first line\n1 CONT second line\n", &db);
    char out[64];

    // When
    label_of(&db, K_NOTE, "@N1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("first line", out);
}

/* Vérifie qu'une note vide utilise son XREF comme libellé. */
TEST(record_label_of_empty_note_is_its_xref) {
    // Given
    Database db;
    fx_database("0 @N1@ NOTE\n", &db);
    char out[64];

    // When
    label_of(&db, K_NOTE, "@N1@", out, sizeof out);

    // Then
    ASSERT_EQ_STR("@N1@", out);
}

/* Enregistre les scénarios du module et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("core/records");
    RUN_TEST(format_name_places_surname_before_given_name);
    RUN_TEST(format_name_formats_surname_first_input_with_a_comma);
    RUN_TEST(format_name_returns_empty_string_when_name_is_empty);
    RUN_TEST(format_name_truncates_to_the_output_size);
    RUN_TEST(kind_from_tag_maps_every_record_tag_to_its_kind);
    RUN_TEST(kind_from_tag_returns_minus_one_for_non_record_tags);
    RUN_TEST(record_label_of_individual_is_the_formatted_name);
    RUN_TEST(record_label_of_individual_without_name_is_empty);
    RUN_TEST(record_label_of_family_joins_husband_and_wife_names);
    RUN_TEST(record_label_of_family_shows_question_mark_for_a_missing_spouse);
    RUN_TEST(record_label_of_family_shows_question_mark_for_a_nameless_spouse);
    RUN_TEST(record_label_of_source_prefers_title_over_abbreviation);
    RUN_TEST(record_label_of_source_falls_back_to_abbreviation);
    RUN_TEST(record_label_of_source_falls_back_to_xref);
    RUN_TEST(record_label_of_repository_is_its_name);
    RUN_TEST(record_label_of_repository_without_name_is_its_xref);
    RUN_TEST(record_label_of_media_prefers_title_over_file);
    RUN_TEST(record_label_of_media_falls_back_to_file_then_xref);
    RUN_TEST(record_label_of_note_is_its_first_line);
    RUN_TEST(record_label_of_empty_note_is_its_xref);
    return tl_finish();
}
