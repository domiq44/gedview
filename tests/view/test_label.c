#include "i18n/i18n.h"
#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/label.h"

/* Vérifie qu'un nom GEDCOM est affiché sans ses séparateurs obliques. */
TEST(display_name_formats_a_gedcom_name) {
    // Given
    const char *raw = "Jean/DUPONT/";
    char out[64];

    // When
    display_name(raw, out, sizeof out);

    // Then
    ASSERT_EQ_STR("DUPONT, Jean", out);
}

/* Vérifie qu'un nom vide est remplacé par le texte « sans nom ». */
TEST(display_name_substitutes_placeholder_when_name_is_empty) {
    // Given
    const char *raw = "//";
    char out[64];

    // When
    display_name(raw, out, sizeof out);

    // Then
    ASSERT_EQ_STR(T(S_NONAME), out);
}

/* Vérifie que display_label restitue le libellé calculé pour l'enregistrement. */
TEST(display_label_returns_the_record_label) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME Marie/CURIE/\n", &db);
    char out[64];

    // When
    display_label(&db, K_INDI, db_find(&db, "@I1@"), out, sizeof out);

    // Then
    ASSERT_EQ_STR("CURIE, Marie", out);
}

/* Vérifie qu'un individu sans nom reçoit le libellé de remplacement. */
TEST(display_label_substitutes_placeholder_for_a_nameless_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n", &db);
    char out[64];

    // When
    display_label(&db, K_INDI, db_find(&db, "@I1@"), out, sizeof out);

    // Then
    ASSERT_EQ_STR(T(S_NONAME), out);
}

/* Vérifie que les autres types conservent leur XREF lorsqu'aucun libellé n'existe. */
TEST(display_label_keeps_xref_fallback_of_other_kinds) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @S1@ SOUR\n", &db);
    char out[64];

    // When
    display_label(&db, K_SRC, db_find(&db, "@S1@"), out, sizeof out);

    // Then
    ASSERT_EQ_STR("@S1@", out);
}

/* Enregistre les scénarios de libellés et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("view/label");
    RUN_TEST(display_name_formats_a_gedcom_name);
    RUN_TEST(display_name_substitutes_placeholder_when_name_is_empty);
    RUN_TEST(display_label_returns_the_record_label);
    RUN_TEST(display_label_substitutes_placeholder_for_a_nameless_individual);
    RUN_TEST(display_label_keeps_xref_fallback_of_other_kinds);
    return tl_finish();
}
