#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/family.h"

/* Jean (I1) est né en 1980 ; Paul (I2) en 1950 et mort en 2020 ; Marie (I3) est morte en 2001 ; Louis (I4)
 * n'a aucune date. */
static const char *PEOPLE = "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n1 BIRT\n2 DATE 1980\n"
                            "0 @I2@ INDI\n1 NAME Paul/DUPONT/\n1 BIRT\n2 DATE 1950\n1 DEAT\n2 DATE 2020\n"
                            "0 @I3@ INDI\n1 NAME Marie/MARTIN/\n1 DEAT\n2 DATE 2001\n"
                            "0 @I4@ INDI\n1 NAME Louis/DUPONT/\n";

/* Alice (I1) a deux enfants, Bob (I2) et Carol (I3), dans une même famille. */
static const char *CHILDREN = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMS @F1@\n"
                              "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n1 FAMC @F1@\n"
                              "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n1 FAMC @F1@\n"
                              "0 @F1@ FAM\n1 HUSB @I1@\n1 CHIL @I2@\n1 CHIL @I3@\n";

/* Vérifie que les enfants sont listés dans l'ordre de leur famille. */
TEST(person_children_lists_the_children_in_file_order) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    const Node *children[FAMILY_MAX_CHILDREN] = {0};

    // When
    int count = person_children(&db, db_find(&db, "@I1@"), children, FAMILY_MAX_CHILDREN);

    // Then
    ASSERT_EQ_INT(2, count);
    ASSERT_EQ_STR("@I2@", children[0]->xref);
    ASSERT_EQ_STR("@I3@", children[1]->xref);
}

/* Vérifie qu'un individu sans famille de conjoint n'a pas d'enfant. */
TEST(person_children_is_empty_without_a_spouse_family) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    const Node *children[FAMILY_MAX_CHILDREN];

    // When
    int count = person_children(&db, db_find(&db, "@I2@"), children, FAMILY_MAX_CHILDREN);

    // Then
    ASSERT_EQ_INT(0, count);
}

/* Vérifie que les enfants inconnus et les références qui ne sont pas des individus sont ignorés. */
TEST(person_children_ignores_unknown_or_non_individual_references) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME A/B/\n1 FAMS @F1@\n1 FAMS @I9@\n"
                "0 @F1@ FAM\n1 CHIL @I9@\n1 CHIL @F1@\n1 CHIL @I1@\n",
                &db);
    const Node *children[FAMILY_MAX_CHILDREN];

    // When
    int count = person_children(&db, db_find(&db, "@I1@"), children, FAMILY_MAX_CHILDREN);

    // Then
    ASSERT_EQ_INT(1, count);
}

/* Vérifie que la liste des enfants respecte la taille maximale demandée. */
TEST(person_children_stops_at_the_requested_maximum) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    const Node *children[1];

    // When
    int count = person_children(&db, db_find(&db, "@I1@"), children, 1);

    // Then
    ASSERT_EQ_INT(1, count);
}

/* Vérifie que person_summary renseigne le libellé et les dates d'un individu né et mort. */
TEST(person_summary_fills_the_label_and_the_dates) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(PEOPLE, &db);
    char label[128];
    char dates[128];

    // When
    person_summary(&db, db_find(&db, "@I2@"), label, sizeof label, dates, sizeof dates);

    // Then
    ASSERT_EQ_STR("DUPONT, Paul", label);
    ASSERT_EQ_STR("° 1950 - + 2020", dates);
}

/* Vérifie qu'une naissance seule n'affiche que la partie « ° ». */
TEST(person_summary_shows_only_the_birth_when_the_death_is_unknown) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(PEOPLE, &db);
    char label[128];
    char dates[128];

    // When
    person_summary(&db, db_find(&db, "@I1@"), label, sizeof label, dates, sizeof dates);

    // Then
    ASSERT_EQ_STR("° 1980", dates);
}

/* Vérifie qu'un décès seul n'affiche que la partie « + ». */
TEST(person_summary_shows_only_the_death_when_the_birth_is_unknown) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(PEOPLE, &db);
    char label[128];
    char dates[128];

    // When
    person_summary(&db, db_find(&db, "@I3@"), label, sizeof label, dates, sizeof dates);

    // Then
    ASSERT_EQ_STR("+ 2001", dates);
}

/* Vérifie qu'un individu sans date a une ligne de dates vide. */
TEST(person_summary_leaves_the_dates_empty_when_both_are_unknown) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(PEOPLE, &db);
    char label[128];
    char dates[128] = "unchanged";

    // When
    person_summary(&db, db_find(&db, "@I4@"), label, sizeof label, dates, sizeof dates);

    // Then
    ASSERT_EQ_STR("DUPONT, Louis", label);
    ASSERT_EQ_STR("", dates);
}

/* Enregistre les scénarios d'informations familiales et retourne leur bilan. */
int main(void) {
    tl_suite("view/family");
    RUN_TEST(person_children_lists_the_children_in_file_order);
    RUN_TEST(person_children_is_empty_without_a_spouse_family);
    RUN_TEST(person_children_ignores_unknown_or_non_individual_references);
    RUN_TEST(person_children_stops_at_the_requested_maximum);
    RUN_TEST(person_summary_fills_the_label_and_the_dates);
    RUN_TEST(person_summary_shows_only_the_birth_when_the_death_is_unknown);
    RUN_TEST(person_summary_shows_only_the_death_when_the_birth_is_unknown);
    RUN_TEST(person_summary_leaves_the_dates_empty_when_both_are_unknown);
    return tl_finish();
}
