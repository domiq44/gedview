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

/* Alice (I1) a pour parents Bob (I2) et Carol (I3), une sœur Dora (I6), un demi-frère Hugo (I7, fils de
   Bob et Gina), un époux Eric (I4) et une fille Fanny (I9). */
static const char *RELATIVES = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMC @F1@\n1 FAMS @F2@\n"
                               "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n1 FAMS @F1@\n1 FAMS @F3@\n"
                               "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n1 FAMS @F1@\n"
                               "0 @I4@ INDI\n1 NAME Eric/ECHO/\n1 FAMS @F2@\n"
                               "0 @I5@ INDI\n1 NAME Gina/GOLF/\n1 FAMS @F3@\n"
                               "0 @I6@ INDI\n1 NAME Dora/DELTA/\n1 FAMC @F1@\n"
                               "0 @I7@ INDI\n1 NAME Hugo/HOTEL/\n1 FAMC @F3@\n"
                               "0 @I9@ INDI\n1 NAME Fanny/FOXTROT/\n1 FAMC @F2@\n"
                               "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I3@\n1 CHIL @I1@\n1 CHIL @I6@\n"
                               "0 @F2@ FAM\n1 HUSB @I4@\n1 WIFE @I1@\n1 CHIL @I9@\n"
                               "0 @F3@ FAM\n1 HUSB @I2@\n1 WIFE @I5@\n1 CHIL @I7@\n";

/* Vérifie que les proches sont regroupés dans l'ordre parents, fratrie, demi-fratrie, conjoints, enfants. */
TEST(person_relatives_groups_relatives_by_relation_in_order) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RELATIVES, &db);
    Relative relatives[FAMILY_MAX_RELATIVES];
    const char *xrefs[] = {"@I2@", "@I3@", "@I6@", "@I7@", "@I4@", "@I9@"};
    const Relation relations[] = {
        REL_PARENT, REL_PARENT, REL_SIBLING, REL_HALF_SIBLING, REL_SPOUSE, REL_CHILD,
    };

    // When
    int count = person_relatives(&db, db_find(&db, "@I1@"), relatives, FAMILY_MAX_RELATIVES);

    // Then
    ASSERT_EQ_INT(6, count);
    for (int i = 0; i < 6; i++) {
        ASSERT_EQ_STR(xrefs[i], relatives[i].indi->xref);
        ASSERT_EQ_INT(relations[i], relatives[i].relation);
    }
}

/* Vérifie qu'une personne qui est à la fois conjoint dans deux familles n'apparaît qu'une fois. */
TEST(person_relatives_lists_a_person_only_once) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME A/B/\n1 FAMS @F1@\n1 FAMS @F2@\n"
                "0 @I2@ INDI\n1 NAME C/D/\n1 FAMS @F1@\n1 FAMS @F2@\n"
                "0 @F1@ FAM\n1 HUSB @I1@\n1 WIFE @I2@\n"
                "0 @F2@ FAM\n1 HUSB @I1@\n1 WIFE @I2@\n",
                &db);
    Relative relatives[FAMILY_MAX_RELATIVES];

    // When
    int count = person_relatives(&db, db_find(&db, "@I1@"), relatives, FAMILY_MAX_RELATIVES);

    // Then
    ASSERT_EQ_INT(1, count);
    ASSERT_EQ_STR("@I2@", relatives[0].indi->xref);
}

/* Vérifie que la liste des proches respecte la taille maximale demandée. */
TEST(person_relatives_stops_at_the_requested_maximum) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RELATIVES, &db);
    Relative relatives[2];

    // When
    int count = person_relatives(&db, db_find(&db, "@I1@"), relatives, 2);

    // Then
    ASSERT_EQ_INT(2, count);
}

/* Vérifie qu'un individu sans aucun lien familial n'a pas de proches. */
TEST(person_relatives_is_empty_for_an_isolated_person) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME A/B/\n", &db);
    Relative relatives[FAMILY_MAX_RELATIVES];

    // When
    int count = person_relatives(&db, db_find(&db, "@I1@"), relatives, FAMILY_MAX_RELATIVES);

    // Then
    ASSERT_EQ_INT(0, count);
}

/* Vérifie que person_siblings retourne les enfants de la famille parentale, la personne comprise. */
TEST(person_siblings_includes_the_person_in_file_order) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RELATIVES, &db);
    const Node *siblings[FAMILY_MAX_CHILDREN] = {NULL};

    // When
    int count = person_siblings(&db, db_find(&db, "@I1@"), siblings, FAMILY_MAX_CHILDREN);

    // Then
    ASSERT_EQ_INT(2, count);
    ASSERT_EQ_STR("@I1@", siblings[0]->xref);
    ASSERT_EQ_STR("@I6@", siblings[1]->xref);
}

/* Vérifie qu'un individu sans famille parentale n'a pas de fratrie. */
TEST(person_siblings_is_empty_without_a_parental_family) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RELATIVES, &db);
    const Node *siblings[FAMILY_MAX_CHILDREN] = {NULL};

    // When
    int count = person_siblings(&db, db_find(&db, "@I2@"), siblings, FAMILY_MAX_CHILDREN);

    // Then
    ASSERT_EQ_INT(0, count);
}

/* Vérifie que family_member retourne l'individu référencé, et NULL pour une référence absente ou non
 * individuelle. */
TEST(family_member_returns_only_existing_individuals) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME A/B/\n0 @F1@ FAM\n1 HUSB @I1@\n1 WIFE @I9@\n", &db);
    const Node *fam = db_find(&db, "@F1@");

    // When
    const Node *husband = family_member(&db, fam, "HUSB");
    const Node *unknown_wife = family_member(&db, fam, "WIFE");
    const Node *without_family = family_member(&db, NULL, "HUSB");

    // Then
    ASSERT_EQ_STR("@I1@", husband->xref);
    ASSERT_NULL(unknown_wife);
    ASSERT_NULL(without_family);
}

/* Enregistre les scénarios d'informations familiales et retourne leur bilan. */
int main(void) {
    tl_suite("view/family");
    RUN_TEST(person_relatives_groups_relatives_by_relation_in_order);
    RUN_TEST(person_relatives_lists_a_person_only_once);
    RUN_TEST(person_relatives_stops_at_the_requested_maximum);
    RUN_TEST(person_relatives_is_empty_for_an_isolated_person);
    RUN_TEST(person_siblings_includes_the_person_in_file_order);
    RUN_TEST(person_siblings_is_empty_without_a_parental_family);
    RUN_TEST(family_member_returns_only_existing_individuals);
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
