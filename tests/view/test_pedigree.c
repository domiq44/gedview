#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/pedigree.h"

/* Jean (I1) a pour père Paul (I2) et pour mère Marie (I3) ; Paul a pour père Louis (I4). */
static const char *TREE =
    "0 @I1@ INDI\n1 NAME Jean/DUPONT/\n1 BIRT\n2 DATE 1980\n1 FAMC @F1@\n"
    "0 @I2@ INDI\n1 NAME Paul/DUPONT/\n1 BIRT\n2 DATE 1950\n1 DEAT\n2 DATE 2020\n1 FAMC @F2@\n"
    "0 @I3@ INDI\n1 NAME Marie/MARTIN/\n1 DEAT\n2 DATE 2001\n"
    "0 @I4@ INDI\n1 NAME Louis/DUPONT/\n"
    "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I3@\n1 CHIL @I1@\n"
    "0 @F2@ FAM\n1 HUSB @I4@\n1 CHIL @I2@\n";

/* Construit l'arbre de l'individu xref dans p. */
static void build(Pedigree *p, const Database *db, const char *xref) {
    pedigree_build(p, db, db_find(db, xref));
}

/* Vérifie que les cases suivent la numérotation 2n pour le père et 2n+1 pour la mère. */
TEST(pedigree_build_places_father_at_2n_and_mother_at_2n_plus_1) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_STR("@I1@", p.box[1].indi->xref);
    ASSERT_EQ_STR("@I2@", p.box[2].indi->xref);
    ASSERT_EQ_STR("@I3@", p.box[3].indi->xref);
    ASSERT_EQ_STR("@I4@", p.box[4].indi->xref);
    ASSERT_NULL(p.box[5].indi);
    ASSERT_EQ_INT(3, p.gens);
}

/* Vérifie qu'un individu sans FAMC donne un arbre d'une seule case. */
TEST(pedigree_build_without_parents_has_a_single_generation) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;

    // When
    build(&p, &db, "@I4@");

    // Then
    ASSERT_EQ_INT(1, p.gens);
    ASSERT_NOT_NULL(p.box[1].indi);
    ASSERT_NULL(p.box[2].indi);
}

/* Vérifie que le libellé de la case est celui de la liste des individus. */
TEST(pedigree_build_uses_the_individual_label) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_STR("DUPONT, Jean", p.box[1].label);
}

/* Vérifie que naissance et décès sont affichés sous la forme « ° date - + date ». */
TEST(pedigree_build_formats_birth_and_death_dates) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_STR("° 1950 - + 2020", p.box[2].dates);
}

/* Vérifie qu'une seule date connue n'affiche que la partie correspondante. */
TEST(pedigree_build_keeps_only_the_known_date) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_STR("° 1980", p.box[1].dates);
    ASSERT_EQ_STR("+ 2001", p.box[3].dates);
}

/* Vérifie qu'un individu sans date a une ligne de dates vide. */
TEST(pedigree_build_leaves_dates_empty_when_unknown) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_STR("", p.box[4].dates);
}

/* Vérifie qu'un cycle d'ascendance (individu parent de lui-même) est borné par PEDIGREE_MAX_GENS. */
TEST(pedigree_build_stops_on_an_ancestry_loop) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME Loop/ER/\n1 FAMC @F1@\n0 @F1@ FAM\n1 HUSB @I1@\n1 CHIL @I1@\n", &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_INT(PEDIGREE_MAX_GENS, p.gens);
}

/* Vérifie qu'une référence parent qui n'est pas un individu est ignorée. */
TEST(pedigree_build_ignores_a_parent_that_is_not_an_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME A/B/\n1 FAMC @F1@\n0 @F1@ FAM\n1 HUSB @F1@\n1 WIFE @I9@\n", &db);
    Pedigree p;

    // When
    build(&p, &db, "@I1@");

    // Then
    ASSERT_EQ_INT(1, p.gens);
}

/* Vérifie que la génération est le rang du bit de poids fort du numéro de case. */
TEST(pedigree_gen_is_the_floor_of_log2) {
    // Given
    const int numbers[] = {1, 2, 3, 4, 7, 8, 255};
    const int expected[] = {0, 1, 1, 2, 2, 3, 7};

    for (int i = 0; i < 7; i++) {
        // When
        int gen = pedigree_gen(numbers[i]);

        // Then
        ASSERT_EQ_INT(expected[i], gen);
    }
}

/* Vérifie que les ancêtres inconnus ne prennent pas de place et que chaque case est centrée sur ses parents.
 */
TEST(pedigree_row_centers_a_box_between_its_parents) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;
    build(&p, &db, "@I1@");

    // When
    int root = pedigree_row(&p, 1);
    int father = pedigree_row(&p, 2);
    int mother = pedigree_row(&p, 3);
    int grandfather = pedigree_row(&p, 4);

    // Then
    ASSERT_EQ_INT(0, grandfather);
    ASSERT_EQ_INT(0, father);
    ASSERT_EQ_INT(3, mother);
    ASSERT_EQ_INT(1, root);
    ASSERT_EQ_INT(5, pedigree_lines(&p));
}

/* Vérifie qu'une disposition limitée à deux générations masque les grands-parents. */
TEST(pedigree_layout_hides_generations_beyond_the_limit) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;
    build(&p, &db, "@I1@");

    // When
    pedigree_layout(&p, 1, 2);

    // Then
    ASSERT_TRUE(pedigree_visible(&p, 2));
    ASSERT_FALSE(pedigree_visible(&p, 4));
    ASSERT_EQ_INT(5, pedigree_lines(&p));
}

/* Vérifie qu'une disposition enracinée sur le père ne montre que sa lignée, sur une hauteur réduite. */
TEST(pedigree_layout_rooted_on_an_ancestor_shows_only_its_lineage) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;
    build(&p, &db, "@I1@");

    // When
    pedigree_layout(&p, 2, 2);

    // Then
    ASSERT_TRUE(pedigree_visible(&p, 2));
    ASSERT_TRUE(pedigree_visible(&p, 4));
    ASSERT_FALSE(pedigree_visible(&p, 1));
    ASSERT_FALSE(pedigree_visible(&p, 3));
    ASSERT_EQ_INT(2, pedigree_lines(&p));
}

/* Vérifie que PED_PARENT choisit le père, puis la mère s'il manque, puis reste en place. */
TEST(pedigree_move_parent_prefers_father_then_mother) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;
    build(&p, &db, "@I1@");

    // When
    int from_root = pedigree_move(&p, 1, PED_PARENT);
    int from_mother = pedigree_move(&p, 3, PED_PARENT);

    // Then
    ASSERT_EQ_INT(2, from_root);
    ASSERT_EQ_INT(3, from_mother);
}

/* Vérifie que PED_CHILD remonte vers l'enfant et reste sur la racine. */
TEST(pedigree_move_child_goes_back_toward_the_root) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;
    build(&p, &db, "@I1@");

    // When
    int from_grandfather = pedigree_move(&p, 4, PED_CHILD);
    int from_root = pedigree_move(&p, 1, PED_CHILD);

    // Then
    ASSERT_EQ_INT(2, from_grandfather);
    ASSERT_EQ_INT(1, from_root);
}

/* Vérifie que PED_PREV et PED_NEXT passent d'une case à sa voisine de la même génération, sans sortir de la
 * colonne. */
TEST(pedigree_move_prev_and_next_stay_within_the_generation) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Pedigree p;
    build(&p, &db, "@I1@");

    // When
    int next = pedigree_move(&p, 2, PED_NEXT);
    int prev = pedigree_move(&p, 3, PED_PREV);
    int last = pedigree_move(&p, 3, PED_NEXT);
    int first = pedigree_move(&p, 2, PED_PREV);
    int lone = pedigree_move(&p, 4, PED_NEXT);

    // Then
    ASSERT_EQ_INT(3, next);
    ASSERT_EQ_INT(2, prev);
    ASSERT_EQ_INT(3, last);
    ASSERT_EQ_INT(2, first);
    ASSERT_EQ_INT(4, lone);
}

int main(void) {
    tl_suite("view/pedigree");
    RUN_TEST(pedigree_build_places_father_at_2n_and_mother_at_2n_plus_1);
    RUN_TEST(pedigree_build_without_parents_has_a_single_generation);
    RUN_TEST(pedigree_build_uses_the_individual_label);
    RUN_TEST(pedigree_build_formats_birth_and_death_dates);
    RUN_TEST(pedigree_build_keeps_only_the_known_date);
    RUN_TEST(pedigree_build_leaves_dates_empty_when_unknown);
    RUN_TEST(pedigree_build_stops_on_an_ancestry_loop);
    RUN_TEST(pedigree_build_ignores_a_parent_that_is_not_an_individual);
    RUN_TEST(pedigree_gen_is_the_floor_of_log2);
    RUN_TEST(pedigree_row_centers_a_box_between_its_parents);
    RUN_TEST(pedigree_layout_hides_generations_beyond_the_limit);
    RUN_TEST(pedigree_layout_rooted_on_an_ancestor_shows_only_its_lineage);
    RUN_TEST(pedigree_move_parent_prefers_father_then_mother);
    RUN_TEST(pedigree_move_child_goes_back_toward_the_root);
    RUN_TEST(pedigree_move_prev_and_next_stay_within_the_generation);
    return tl_finish();
}
