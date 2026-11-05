#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/nav.h"
#include "view/treenav.h"

/* Alice (I1) a pour père Bob (I2) et pour mère Carol (I3). */
static const char *ANCESTRY = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMC @F1@\n"
                              "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n"
                              "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n"
                              "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I3@\n1 CHIL @I1@\n";

/* Alice (I1) a deux enfants : Bob (I2) et Carol (I3). */
static const char *CHILDREN = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMS @F1@\n"
                              "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n1 FAMC @F1@\n"
                              "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n1 FAMC @F1@\n"
                              "0 @F1@ FAM\n1 HUSB @I1@\n1 CHIL @I2@\n1 CHIL @I3@\n";

/* Alice (I1) a pour parents Bob (I2) et Carol (I3) et pour enfant Dan (I4). */
static const char *PARENTS_AND_CHILD = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMC @F1@\n1 FAMS @F2@\n"
                                       "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n"
                                       "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n"
                                       "0 @I4@ INDI\n1 NAME Dan/DELTA/\n1 FAMC @F2@\n"
                                       "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I3@\n1 CHIL @I1@\n"
                                       "0 @F2@ FAM\n1 HUSB @I1@\n1 CHIL @I4@\n";

/* Vérifie que la bascule affiche l'arbre avec la case de l'individu courant sélectionnée. */
TEST(treenav_toggle_opens_the_tree_on_the_root_box) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    treenav_toggle(&nav, &db);

    // Then
    ASSERT_EQ_INT(1, nav.tree);
    ASSERT_EQ_INT(1, nav.tree_sel);
}

/* Vérifie qu'une seconde bascule referme l'arbre. */
TEST(treenav_toggle_closes_the_tree) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_toggle(&nav, &db);

    // Then
    ASSERT_EQ_INT(0, nav.tree);
}

/* Vérifie que l'arbre ne s'ouvre pas depuis une autre liste que les individus. */
TEST(treenav_toggle_does_nothing_outside_the_individuals_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_FAM);

    // When
    treenav_toggle(&nav, &db);

    // Then
    ASSERT_EQ_INT(0, nav.tree);
}

/* Vérifie que le déplacement vers les parents sélectionne la case du père. */
TEST(treenav_move_selects_the_father) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_move(&nav, &db, PED_PARENT);

    // Then
    ASSERT_EQ_INT(2, nav.tree_sel);
}

/* Vérifie que recentrer sur une case sélectionne l'ancêtre dans la liste et ramène la sélection à la racine.
 */
TEST(treenav_recenter_selects_the_ancestor_in_the_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_recenter(&nav, &db, 3);

    // Then
    ASSERT_EQ_STR("@I3@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(1, nav.tree_sel);
    ASSERT_EQ_INT(1, nav.tree);
}

/* Vérifie qu'une case inexistante ne change ni la sélection ni l'historique. */
TEST(treenav_recenter_ignores_a_missing_box) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_recenter(&nav, &db, 5);
    treenav_recenter(&nav, &db, 0);

    // Then
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* Vérifie que recentrer l'arbre referme la liste des enfants restée ouverte. */
TEST(treenav_recenter_closes_an_open_child_menu) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(PARENTS_AND_CHILD, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_move(&nav, &db, PED_CHILD);
    int menu_was_open = nav.menu;

    // When
    treenav_recenter(&nav, &db, 2);

    // Then
    ASSERT_EQ_INT(1, menu_was_open);
    ASSERT_EQ_INT(0, nav.menu);
    ASSERT_EQ_STR("@I2@", nav_current(&nav, &db)->xref);
}

/* Vérifie que choisir une liste referme l'arbre. */
TEST(nav_set_kind_closes_the_tree) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    nav_set_kind(&nav, K_INDI);

    // Then
    ASSERT_EQ_INT(0, nav.tree);
}

/* Vérifie que le retour arrière après un recentrage restaure l'individu précédent, arbre conservé. */
TEST(nav_back_after_a_recenter_returns_to_the_previous_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_recenter(&nav, &db, 2);

    // When
    nav_back(&nav);

    // Then
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(1, nav.tree);
    ASSERT_EQ_INT(1, nav.tree_sel);
}

/* Vérifie que « vers l'enfant » sur la personne centrale ouvre la liste de ses enfants. */
TEST(treenav_move_child_on_the_root_opens_the_children_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_move(&nav, &db, PED_CHILD);

    // Then
    ASSERT_EQ_INT(1, nav.menu);
    ASSERT_EQ_INT(0, nav.menu_sel);
}

/* Vérifie qu'aucune liste ne s'ouvre pour une personne centrale sans enfant. */
TEST(treenav_move_child_on_the_root_without_children_changes_nothing) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;
    treenav_toggle(&nav, &db);

    // When
    treenav_move(&nav, &db, PED_CHILD);

    // Then
    ASSERT_EQ_INT(0, nav.menu);
    ASSERT_EQ_INT(1, nav.tree_sel);
}

/* Vérifie que la sélection de la liste des enfants reste bornée à ses extrémités. */
TEST(treenav_menu_move_stays_within_the_children) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_move(&nav, &db, PED_CHILD);

    // When
    treenav_menu_move(&nav, &db, -1);
    int first = nav.menu_sel;
    treenav_menu_move(&nav, &db, 5);

    // Then
    ASSERT_EQ_INT(0, first);
    ASSERT_EQ_INT(1, nav.menu_sel);
}

/* Vérifie que choisir un enfant en fait l'individu courant, ferme la liste et alimente l'historique. */
TEST(treenav_menu_choose_selects_the_child_and_closes_the_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_move(&nav, &db, PED_CHILD);
    treenav_menu_move(&nav, &db, 1);

    // When
    treenav_menu_choose(&nav, &db);

    // Then
    ASSERT_EQ_STR("@I3@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(0, nav.menu);
    ASSERT_EQ_INT(1, nav.tree_sel);
    ASSERT_EQ_INT(1, (int)nav.history_count);
}

/* Vérifie qu'un enfant sélectionné hors limites referme la liste sans changer d'individu. */
TEST(treenav_menu_choose_with_an_out_of_range_child_closes_the_list_without_moving) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_move(&nav, &db, PED_CHILD);
    nav.menu_sel = 9;

    // When
    treenav_menu_choose(&nav, &db);

    // Then
    ASSERT_EQ_INT(0, nav.menu);
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* Vérifie que fermer la liste conserve l'individu courant. */
TEST(treenav_menu_close_keeps_the_current_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_move(&nav, &db, PED_CHILD);

    // When
    treenav_menu_close(&nav);

    // Then
    ASSERT_EQ_INT(0, nav.menu);
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
}

/* Alice (I1, index 0) a une sœur Dora (I6, index 2) et un parent Bob (I2, index 1) ; Zoe (I7, index 3) est
 * isolée. */
static const char *SIBLINGS = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMC @F1@\n"
                              "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n1 FAMS @F1@\n"
                              "0 @I6@ INDI\n1 NAME Dora/DELTA/\n1 FAMC @F1@\n"
                              "0 @I7@ INDI\n1 NAME Zoe/ZULU/\n"
                              "0 @F1@ FAM\n1 HUSB @I2@\n1 CHIL @I1@\n1 CHIL @I6@\n";

/* Vérifie que « suivant » sur la personne centrale passe à son frère ou sa sœur suivant(e) et alimente
 * l'historique. */
TEST(treenav_move_next_on_the_root_selects_the_next_sibling) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_move(&nav, &db, PED_NEXT);

    // Then
    ASSERT_EQ_STR("@I6@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(1, nav.tree_sel);
    ASSERT_EQ_INT(1, (int)nav.history_count);
}

/* Vérifie que « précédent » revient à la personne précédente de la fratrie. */
TEST(treenav_move_prev_on_the_root_selects_the_previous_sibling) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 2;
    treenav_toggle(&nav, &db);

    // When
    treenav_move(&nav, &db, PED_PREV);

    // Then
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
}

/* Vérifie qu'aux extrémités de la fratrie ou sans fratrie la personne centrale ne change pas. */
TEST(treenav_move_between_siblings_stops_at_the_ends_and_without_siblings) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav first TL_AUTOCLEAN(nav_destroy);
    nav_init(&first);
    treenav_toggle(&first, &db);
    Nav alone TL_AUTOCLEAN(nav_destroy);
    nav_init(&alone);
    alone.sel = 3;
    treenav_toggle(&alone, &db);

    // When
    treenav_move(&first, &db, PED_PREV);
    treenav_move(&alone, &db, PED_NEXT);

    // Then
    ASSERT_EQ_STR("@I1@", nav_current(&first, &db)->xref);
    ASSERT_EQ_STR("@I7@", nav_current(&alone, &db)->xref);
    ASSERT_EQ_INT(0, (int)first.history_count);
}

/* Vérifie que la liste des proches s'ouvre sur la première entrée quand la personne en a. */
TEST(treenav_open_relatives_opens_the_list_on_the_first_entry) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);

    // When
    treenav_open_relatives(&nav, &db);

    // Then
    ASSERT_EQ_INT(NAV_MENU_RELATIVES, nav.menu);
    ASSERT_EQ_INT(0, nav.menu_sel);
}

/* Vérifie qu'aucune liste ne s'ouvre pour une personne sans proches. */
TEST(treenav_open_relatives_does_nothing_without_relatives) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 3;
    treenav_toggle(&nav, &db);

    // When
    treenav_open_relatives(&nav, &db);

    // Then
    ASSERT_EQ_INT(NAV_MENU_NONE, nav.menu);
}

/* Vérifie que choisir un proche (ici la sœur, après le parent) en fait la personne centrale et ferme la
 * liste. */
TEST(treenav_menu_choose_in_the_relatives_list_selects_that_person) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_open_relatives(&nav, &db);
    treenav_menu_move(&nav, &db, 1);

    // When
    treenav_menu_choose(&nav, &db);

    // Then
    ASSERT_EQ_STR("@I6@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(NAV_MENU_NONE, nav.menu);
    ASSERT_EQ_INT(1, nav.tree_sel);
}

/* Vérifie que la sélection de la liste des proches reste bornée à ses extrémités. */
TEST(treenav_menu_move_in_the_relatives_list_stays_within_the_entries) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(SIBLINGS, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    treenav_toggle(&nav, &db);
    treenav_open_relatives(&nav, &db);

    // When
    treenav_menu_move(&nav, &db, -3);
    int first = nav.menu_sel;
    treenav_menu_move(&nav, &db, 10);

    // Then
    ASSERT_EQ_INT(0, first);
    ASSERT_EQ_INT(1, nav.menu_sel);
}

/* Enregistre les scénarios de l'arbre d'ascendance et retourne leur bilan. */
int main(void) {
    tl_suite("view/treenav");
    RUN_TEST(treenav_move_next_on_the_root_selects_the_next_sibling);
    RUN_TEST(treenav_move_prev_on_the_root_selects_the_previous_sibling);
    RUN_TEST(treenav_move_between_siblings_stops_at_the_ends_and_without_siblings);
    RUN_TEST(treenav_open_relatives_opens_the_list_on_the_first_entry);
    RUN_TEST(treenav_open_relatives_does_nothing_without_relatives);
    RUN_TEST(treenav_menu_choose_in_the_relatives_list_selects_that_person);
    RUN_TEST(treenav_menu_move_in_the_relatives_list_stays_within_the_entries);
    RUN_TEST(treenav_toggle_opens_the_tree_on_the_root_box);
    RUN_TEST(treenav_toggle_closes_the_tree);
    RUN_TEST(treenav_toggle_does_nothing_outside_the_individuals_list);
    RUN_TEST(treenav_move_selects_the_father);
    RUN_TEST(treenav_recenter_selects_the_ancestor_in_the_list);
    RUN_TEST(treenav_recenter_ignores_a_missing_box);
    RUN_TEST(treenav_recenter_closes_an_open_child_menu);
    RUN_TEST(nav_set_kind_closes_the_tree);
    RUN_TEST(nav_back_after_a_recenter_returns_to_the_previous_individual);
    RUN_TEST(treenav_move_child_on_the_root_opens_the_children_list);
    RUN_TEST(treenav_move_child_on_the_root_without_children_changes_nothing);
    RUN_TEST(treenav_menu_move_stays_within_the_children);
    RUN_TEST(treenav_menu_choose_selects_the_child_and_closes_the_list);
    RUN_TEST(treenav_menu_choose_with_an_out_of_range_child_closes_the_list_without_moving);
    RUN_TEST(treenav_menu_close_keeps_the_current_individual);
    return tl_finish();
}
