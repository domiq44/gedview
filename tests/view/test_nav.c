#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/nav.h"

#include <locale.h>

#define FAR (1 << 30)

/* Individus triés : Alice (I1), Bob (I2), Carol (I3). Une famille F1, une source S1. */
static const char *TREE = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n"
                          "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n"
                          "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n"
                          "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I1@\n"
                          "0 @S1@ SOUR\n1 TITL Registre\n";

/* --- nav_init / nav_current / nav_count --- */

/* Vérifie que l'état initial affiche la liste des individus en haut. */
TEST(nav_init_starts_on_the_individuals_list_at_the_top) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);

    // When
    nav_init(&nav);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(0, nav.sel);
    ASSERT_EQ_INT(0, nav.focus_detail);
}

/* Vérifie que nav_count reflète la taille de la liste de la vue active. */
TEST(nav_count_is_the_size_of_the_displayed_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    int individuals = nav_count(&nav, &db);
    nav_set_kind(&nav, K_FAM);
    int families = nav_count(&nav, &db);

    // Then
    ASSERT_EQ_INT(3, individuals);
    ASSERT_EQ_INT(1, families);
}

/* Vérifie que nav_current retourne l'enregistrement sélectionné. */
TEST(nav_current_returns_the_selected_record) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    const Node *cur = nav_current(&nav, &db);

    // Then
    ASSERT_EQ_STR("@I2@", cur->xref);
}

/* Vérifie que nav_current retourne NULL lorsque la liste active est vide. */
TEST(nav_current_returns_null_when_the_list_is_empty) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_NOTE);

    // When
    const Node *cur = nav_current(&nav, &db);

    // Then
    ASSERT_NULL(cur);
}

/* Vérifie qu'une sélection hors limites est ramenée au dernier élément. */
TEST(nav_current_clamps_a_selection_beyond_the_end_of_the_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 99;

    // When
    const Node *cur = nav_current(&nav, &db);

    // Then
    ASSERT_EQ_STR("@I3@", cur->xref);
}

/* --- nav_move --- */

/* Vérifie qu'un déplacement vers le bas sélectionne l'élément suivant. */
TEST(nav_move_down_selects_the_next_item) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, 1);

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie qu'un déplacement vers le bas ne dépasse pas le dernier élément. */
TEST(nav_move_down_stops_at_the_last_item) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 2;
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, 1);

    // Then
    ASSERT_EQ_INT(2, nav.sel);
}

/* Vérifie qu'un déplacement vers le haut ne précède pas le premier élément. */
TEST(nav_move_up_stops_at_the_first_item) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, -1);

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie qu'un grand delta place la sélection à la borne correspondante. */
TEST(nav_move_by_a_huge_delta_jumps_to_the_ends) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, FAR);
    int at_end = nav.sel;
    nav_move(&nav, &db, &d, -FAR);

    // Then
    ASSERT_EQ_INT(2, at_end);
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie qu'un déplacement dans une liste vide conserve la sélection à zéro. */
TEST(nav_move_on_an_empty_list_keeps_the_selection_at_zero) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_NOTE);
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, 1);

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie qu'un déplacement dans la liste remet la sélection du détail à zéro. */
TEST(nav_move_in_the_list_resets_the_detail_position) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 4;
    nav.det_top = 2;
    Detail d = {0};

    // When
    nav_move(&nav, &db, &d, 1);

    // Then
    ASSERT_EQ_INT(0, nav.det_sel);
    ASSERT_EQ_INT(0, nav.det_top);
}

/* Vérifie qu'un déplacement dans le détail ne change pas la sélection de liste. */
TEST(nav_move_in_the_detail_moves_the_detail_selection_only) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    Detail d = {.count = 5};

    // When
    nav_move(&nav, &db, &d, 2);

    // Then
    ASSERT_EQ_INT(2, nav.det_sel);
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que la sélection du détail ne dépasse pas sa dernière ligne. */
TEST(nav_move_in_the_detail_stops_at_the_last_line) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    Detail d = {.count = 5};

    // When
    nav_move(&nav, &db, &d, FAR);

    // Then
    ASSERT_EQ_INT(4, nav.det_sel);
}

/* --- nav_set_kind / focus --- */

/* Vérifie que nav_set_kind active la liste correspondant au type demandé. */
TEST(nav_set_kind_switches_the_displayed_list) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_set_kind(&nav, K_SRC);

    // Then
    ASSERT_EQ_INT(K_SRC, nav.kind);
}

/* Vérifie que revenir à une liste restaure sa sélection et son défilement précédents. */
TEST(nav_set_kind_restores_the_position_of_a_list_when_coming_back) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 2;
    nav.top = 1;

    // When
    nav_set_kind(&nav, K_FAM);
    int sel_in_families = nav.sel;
    nav_set_kind(&nav, K_INDI);

    // Then
    ASSERT_EQ_INT(0, sel_in_families);
    ASSERT_EQ_INT(2, nav.sel);
    ASSERT_EQ_INT(1, nav.top);
}

/* Vérifie que changer de type rend le focus à la liste. */
TEST(nav_set_kind_gives_the_focus_back_to_the_list) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;

    // When
    nav_set_kind(&nav, K_FAM);

    // Then
    ASSERT_EQ_INT(0, nav.focus_detail);
}

/* Vérifie qu'un changement de type réinitialise la sélection et le défilement du détail. */
TEST(nav_set_kind_resets_the_detail_position) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 3;
    nav.det_top = 1;

    // When
    nav_set_kind(&nav, K_FAM);

    // Then
    ASSERT_EQ_INT(0, nav.det_sel);
    ASSERT_EQ_INT(0, nav.det_top);
}

/* Vérifie que chaque bascule de focus alterne entre liste et détail. */
TEST(nav_toggle_focus_alternates_between_list_and_detail) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_toggle_focus(&nav);
    int after_first = nav.focus_detail;
    nav_toggle_focus(&nav);

    // Then
    ASSERT_EQ_INT(1, after_first);
    ASSERT_EQ_INT(0, nav.focus_detail);
}

/* --- nav_activate --- */

/* Vérifie qu'Entrée depuis la liste donne le focus au panneau de détail. */
TEST(nav_activate_from_the_list_gives_the_focus_to_the_detail) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    nav_activate(&nav, &db, &d);

    // Then
    ASSERT_EQ_INT(1, nav.focus_detail);
}

/* Vérifie qu'activer un lien d'individu ouvre sa liste et le sélectionne. */
TEST(nav_activate_on_a_link_to_an_individual_selects_that_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    Line lines[2] = {
        {"header", "", LINE_LINK_RECORD, 0, 0},
        {"HUSB: Carol", "@I3@", LINE_LINK_RECORD, 0, 0},
    };
    Detail d = {.lines = lines, .count = 2};
    nav.det_sel = 1;

    // When
    nav_activate(&nav, &db, &d);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(2, nav.sel);
    ASSERT_EQ_INT(0, nav.det_sel);
}

/* Vérifie qu'un lien vers un autre type ouvre la liste correspondante. */
TEST(nav_activate_on_a_link_to_another_kind_switches_to_that_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    Line lines[1] = {{"FAMS: @F1@", "@F1@", LINE_LINK_RECORD, 0, 0}};
    Detail d = {.lines = lines, .count = 1};

    // When
    nav_activate(&nav, &db, &d);

    // Then
    ASSERT_EQ_INT(K_FAM, nav.kind);
    ASSERT_EQ_INT(0, nav.sel);
    ASSERT_EQ_INT(1, nav.focus_detail);
}

/* Vérifie qu'activer une ligne sans lien ne modifie pas la navigation. */
TEST(nav_activate_on_a_line_without_link_changes_nothing) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    nav.sel = 1;
    Line lines[1] = {{"OCCU: Farmer", "", LINE_LINK_RECORD, 0, 0}};
    Detail d = {.lines = lines, .count = 1};

    // When
    nav_activate(&nav, &db, &d);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie qu'un lien vers un XREF inconnu ne modifie pas la navigation. */
TEST(nav_activate_on_a_link_to_an_unknown_record_changes_nothing) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    nav.sel = 1;
    Line lines[1] = {{"NOTE: @N9@", "@N9@", LINE_LINK_RECORD, 0, 0}};
    Detail d = {.lines = lines, .count = 1};

    // When
    nav_activate(&nav, &db, &d);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie que suivre un lien puis revenir restaure la position de la liste quittée. */
TEST(nav_activate_keeps_the_position_of_the_list_it_leaves) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;
    nav.focus_detail = 1;
    Line lines[1] = {{"SOUR: Registre", "@S1@", LINE_LINK_RECORD, 0, 0}};
    Detail d = {.lines = lines, .count = 1};

    // When
    nav_activate(&nav, &db, &d);
    nav_set_kind(&nav, K_INDI);

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie que le retour arrière restaure les enregistrements visités et leur lien sélectionné. */
TEST(nav_back_restores_each_linked_record_in_order) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;
    nav.det_sel = 1;
    Line individual_lines[] = {
        {"header", "", LINE_LINK_RECORD, 0, 0},
        {"FAMS: @F1@", "@F1@", LINE_LINK_RECORD, 0, 0},
    };
    Detail individual_detail = {.lines = individual_lines, .count = 2};
    nav_activate(&nav, &db, &individual_detail);
    Line family_lines[] = {{"HUSB: Bob", "@I2@", LINE_LINK_RECORD, 0, 0}};
    Detail family_detail = {.lines = family_lines, .count = 1};
    nav_activate(&nav, &db, &family_detail);

    // When
    nav_back(&nav);
    int first_kind = nav.kind;
    int first_det_sel = nav.det_sel;
    nav_back(&nav);

    // Then
    ASSERT_EQ_INT(K_FAM, first_kind);
    ASSERT_EQ_INT(0, first_det_sel);
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(1, nav.det_sel);
    ASSERT_EQ_INT(1, nav.focus_detail);
    nav_destroy(&nav);
}

/* Vérifie qu'un retour arrière sans visite de lien ne change pas la navigation. */
TEST(nav_back_does_nothing_when_history_is_empty) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_back(&nav);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_INT(0, nav.focus_detail);
    nav_destroy(&nav);
}

/* Vérifie que le retour arrière restaure l'individu précédent après un déplacement dans la liste. */
TEST(nav_back_restores_the_previous_list_selection) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    nav_move(&nav, &db, &d, 1);

    // When
    nav_back(&nav);

    // Then
    ASSERT_EQ_INT(0, nav.sel);
    ASSERT_EQ_INT(0, nav.focus_detail);
    nav_destroy(&nav);
}

/* Vérifie que le retour arrière restaure la vue et la sélection quittées. */
TEST(nav_back_restores_the_previous_view) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_set_kind(&nav, K_FAM);
    nav_back(&nav);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_INT(0, nav.focus_detail);
    nav_destroy(&nav);
}

/* --- nav_search --- */

/* Vérifie que la recherche sélectionne le prochain libellé contenant le texte demandé. */
TEST(nav_search_selects_the_next_item_whose_label_contains_the_text) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "brav");

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie qu'une recherche dans la vue Individus trouve aussi le XREF d'un individu. */
TEST(nav_search_matches_an_individual_xref) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "@I3@");

    // Then
    ASSERT_EQ_INT(2, nav.sel);
    ASSERT_EQ_STR("@I3@", nav_current(&nav, &db)->xref);
}

/* Vérifie qu'une recherche dans la vue Citation trouve le XREF de la source citée. */
TEST(nav_search_matches_a_citation_source_xref) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME Amy/ALPHA/\n1 SOUR @X1@\n"
                "0 @I2@ INDI\n1 NAME Zoe/ZULU/\n1 SOUR @Y2@\n"
                "0 @X1@ SOUR\n1 TITL Alpha source\n0 @Y2@ SOUR\n1 TITL Zulu source\n",
                &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_CITA);

    // When
    nav_search(&nav, &db, "@Y2@");

    // Then
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_STR("@Y2@", nav_current(&nav, &db)->value);
}

static const char *REPO_TREE = "0 @I1@ INDI\n1 NAME Amy/ALPHA/\n1 SOUR @S1@\n"
                               "0 @I2@ INDI\n1 NAME Zoe/ZULU/\n1 SOUR @S2@\n"
                               "0 @S1@ SOUR\n1 TITL Alpha source\n1 REPO @R1@\n"
                               "0 @S2@ SOUR\n1 TITL Beta source\n1 REPO @R2@\n"
                               "0 @R1@ REPO\n1 NAME Archives Nord\n0 @R2@ REPO\n1 NAME Mairie\n";

/* Vérifie que la vue Source trouve une source par le XREF de son dépôt. */
TEST(nav_search_finds_a_source_by_its_repository_xref) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(REPO_TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_SRC);

    // When
    nav_search(&nav, &db, "@R2@");

    // Then
    ASSERT_EQ_STR("@S2@", nav_current(&nav, &db)->xref);
}

/* Vérifie que la vue Source trouve une source par le nom de son dépôt. */
TEST(nav_search_finds_a_source_by_its_repository_name) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(REPO_TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_SRC);

    // When
    nav_search(&nav, &db, "mairie");

    // Then
    ASSERT_EQ_STR("@S2@", nav_current(&nav, &db)->xref);
}

/* Vérifie que la vue Citation trouve une citation par le nom du dépôt de la source citée. */
TEST(nav_search_finds_a_citation_by_the_repository_name_of_its_source) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(REPO_TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_CITA);

    // When
    nav_search(&nav, &db, "mairie");

    // Then
    ASSERT_EQ_STR("@S2@", nav_current(&nav, &db)->value);
}

/* Vérifie que la vue Citation trouve une citation par le XREF du dépôt de la source citée. */
TEST(nav_search_finds_a_citation_by_the_repository_xref_of_its_source) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(REPO_TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_CITA);

    // When
    nav_search(&nav, &db, "@R2@");

    // Then
    ASSERT_EQ_STR("@S2@", nav_current(&nav, &db)->value);
}

/* Vérifie que la vue Individus ignore les dépôts : seule la vue Source ou Citation les cherche. */
TEST(nav_search_does_not_use_repositories_in_the_individual_view) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(REPO_TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "mairie");

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que la recherche ne distingue pas les majuscules des minuscules. */
TEST(nav_search_ignores_case) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "CHARLIE");

    // Then
    ASSERT_EQ_INT(2, nav.sel);
}

/* Vérifie que la recherche ignore la casse des caractères accentués en UTF-8. */
TEST(nav_search_ignores_case_for_utf8_text) {
    // Given
    ASSERT_NOT_NULL(setlocale(LC_CTYPE, "C.UTF-8"));
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME Émile/LAMBERT/\n0 @I2@ INDI\n1 NAME Zoe/ALPHA/\n", &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "émile");

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie que la recherche reprend au début après avoir atteint la fin de la liste. */
TEST(nav_search_wraps_around_to_the_beginning_of_the_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 2;

    // When
    nav_search(&nav, &db, "alice");

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que la recherche commence après l'élément actuellement sélectionné. */
TEST(nav_search_starts_after_the_current_selection) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 0;

    // When
    nav_search(&nav, &db, "a"); /* Alice (courant), Bob BRAVO, Carol CHARLIE contiennent tous un "a" */

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie que le terme mémorisé avance au résultat suivant puis reboucle sans nouvelle saisie. */
TEST(nav_search_next_advances_and_wraps_with_the_saved_term) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "a");
    int first_match = nav.sel;
    nav_search_next(&nav, &db);
    int second_match = nav.sel;
    nav_search_next(&nav, &db);
    int third_match = nav.sel;

    // Then
    ASSERT_EQ_INT(1, first_match);
    ASSERT_EQ_INT(2, second_match);
    ASSERT_EQ_INT(0, third_match);
    nav_destroy(&nav);
}

/* Vérifie qu'une recherche sans résultat laisse la sélection inchangée. */
TEST(nav_search_keeps_the_selection_when_nothing_matches) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_search(&nav, &db, "zzz");

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie qu'une chaîne de recherche vide ne change pas la sélection. */
TEST(nav_search_ignores_an_empty_text) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_search(&nav, &db, "");

    // Then
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie que rechercher dans une liste vide ne modifie pas la navigation. */
TEST(nav_search_on_an_empty_list_does_nothing) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_NOTE);

    // When
    nav_search(&nav, &db, "x");

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que la recherche porte sur les libellés du type actuellement affiché. */
TEST(nav_search_searches_the_labels_of_the_displayed_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_SRC);

    // When
    nav_search(&nav, &db, "regis");

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie qu'un résultat de recherche remet la position du détail à zéro. */
TEST(nav_search_resets_the_detail_position_on_a_match) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 3;
    nav.det_top = 2;

    // When
    nav_search(&nav, &db, "carol");

    // Then
    ASSERT_EQ_INT(0, nav.det_sel);
    ASSERT_EQ_INT(0, nav.det_top);
}

/* --- nav_scroll --- */

/* Vérifie que le défilement descend pour garder la sélection de liste visible. */
TEST(nav_scroll_moves_the_window_down_to_keep_the_selection_visible) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 10;
    Detail d = {.count = 1};

    // When
    nav_scroll(&nav, &d, 5);

    // Then
    ASSERT_EQ_INT(6, nav.top);
}

/* Vérifie que le défilement remonte pour garder la sélection de liste visible. */
TEST(nav_scroll_moves_the_window_up_to_keep_the_selection_visible) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.top = 8;
    nav.sel = 3;
    Detail d = {.count = 1};

    // When
    nav_scroll(&nav, &d, 5);

    // Then
    ASSERT_EQ_INT(3, nav.top);
}

/* Vérifie que le défilement ne bouge pas si la sélection est déjà visible. */
TEST(nav_scroll_leaves_the_window_alone_when_the_selection_is_visible) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.top = 4;
    nav.sel = 6;
    Detail d = {.count = 1};

    // When
    nav_scroll(&nav, &d, 5);

    // Then
    ASSERT_EQ_INT(4, nav.top);
}

/* Vérifie que le défilement du détail garde sa ligne sélectionnée à l'écran. */
TEST(nav_scroll_keeps_the_detail_selection_visible) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 9;
    Detail d = {.count = 20};

    // When
    nav_scroll(&nav, &d, 4);

    // Then
    ASSERT_EQ_INT(6, nav.det_top);
}

/* Vérifie que la sélection du détail est ramenée dans les limites de ses lignes. */
TEST(nav_scroll_clamps_the_detail_selection_to_the_number_of_lines) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 9;
    Detail d = {.count = 3};

    // When
    nav_scroll(&nav, &d, 5);

    // Then
    ASSERT_EQ_INT(2, nav.det_sel);
}

/* Vérifie que l'absence de lignes remet la sélection du détail à zéro. */
TEST(nav_scroll_resets_the_detail_selection_when_there_are_no_lines) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 4;
    Detail d = {.count = 0};

    // When
    nav_scroll(&nav, &d, 5);

    // Then
    ASSERT_EQ_INT(0, nav.det_sel);
}

/* --- nav_select_individual --- */

/* Vérifie que l'individu ciblé est sélectionné et que l'ancienne position entre dans l'historique. */
TEST(nav_select_individual_selects_the_target_and_records_history) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.det_sel = 3;
    nav.det_top = 2;

    // When
    nav_select_individual(&nav, &db, db_find(&db, "@I3@"));

    // Then
    ASSERT_EQ_INT(2, nav.sel);
    ASSERT_EQ_STR("@I3@", nav_current(&nav, &db)->xref);
    ASSERT_EQ_INT(1, (int)nav.history_count);
    ASSERT_EQ_INT(0, nav.det_sel);
    ASSERT_EQ_INT(0, nav.det_top);
}

/* Vérifie que sélectionner l'individu déjà sélectionné n'ajoute rien à l'historique. */
TEST(nav_select_individual_does_not_record_history_when_the_selection_is_unchanged) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_select_individual(&nav, &db, db_find(&db, "@I2@"));

    // Then
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* Vérifie qu'un enregistrement qui n'est pas un individu laisse la sélection inchangée. */
TEST(nav_select_individual_ignores_a_node_that_is_not_an_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_select_individual(&nav, &db, db_find(&db, "@F1@"));

    // Then
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* Vérifie qu'un enregistrement inconnu (NULL) laisse la sélection inchangée. */
TEST(nav_select_individual_ignores_an_unknown_node) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    nav_select_individual(&nav, &db, db_find(&db, "@I9@"));

    // Then
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* --- nav_record --- */

/* Vérifie que pour la vue active nav_record retourne l'enregistrement sélectionné. */
TEST(nav_record_uses_the_selection_for_the_current_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    const Node *rec = nav_record(&nav, &db, K_INDI);

    // Then
    ASSERT_EQ_STR("@I2@", rec->xref);
}

/* Vérifie que pour une liste inactive nav_record utilise la sélection mémorisée. */
TEST(nav_record_uses_the_saved_selection_for_an_inactive_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 2;
    nav_set_kind(&nav, K_FAM);

    // When
    const Node *rec = nav_record(&nav, &db, K_INDI);

    // Then
    ASSERT_EQ_STR("@I3@", rec->xref);
}

/* Vérifie que nav_record retourne NULL pour une liste vide. */
TEST(nav_record_returns_null_for_an_empty_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    const Node *rec = nav_record(&nav, &db, K_NOTE);

    // Then
    ASSERT_NULL(rec);
}

/* Vérifie qu'une sélection hors limites est ramenée au dernier enregistrement. */
TEST(nav_record_clamps_an_out_of_range_selection) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 99;

    // When
    const Node *rec = nav_record(&nav, &db, K_INDI);

    // Then
    ASSERT_EQ_STR("@I3@", rec->xref);
}

/* --- nav_restore_record --- */

/* Vérifie que la restauration dans la vue active sélectionne l'enregistrement sans historique. */
TEST(nav_restore_record_selects_the_record_in_the_current_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    int found = nav_restore_record(&nav, &db, K_INDI, "@I3@");

    // Then
    ASSERT_EQ_INT(1, found);
    ASSERT_EQ_INT(2, nav.sel);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* Vérifie que la restauration dans une liste inactive met à jour sa sélection mémorisée sans changer de vue.
 */
TEST(nav_restore_record_updates_the_saved_selection_of_an_inactive_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_FAM);

    // When
    int found = nav_restore_record(&nav, &db, K_INDI, "@I2@");

    // Then
    ASSERT_EQ_INT(1, found);
    ASSERT_EQ_INT(K_FAM, nav.kind);
    ASSERT_EQ_STR("@I2@", nav_record(&nav, &db, K_INDI)->xref);
}

/* Vérifie qu'un XREF inconnu retourne 0 et laisse la sélection inchangée. */
TEST(nav_restore_record_returns_zero_for_an_unknown_xref) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    int found = nav_restore_record(&nav, &db, K_INDI, "@I9@");

    // Then
    ASSERT_EQ_INT(0, found);
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie qu'un XREF appartenant à un autre type retourne 0 et laisse la sélection inchangée. */
TEST(nav_restore_record_returns_zero_for_an_xref_of_another_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    int found = nav_restore_record(&nav, &db, K_INDI, "@F1@");

    // Then
    ASSERT_EQ_INT(0, found);
    ASSERT_EQ_INT(1, nav.sel);
}

/* --- nav_restore_view --- */

/* Vérifie que nav_restore_view affiche la liste demandée sans toucher à l'historique. */
TEST(nav_restore_view_switches_the_list_without_history) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_restore_view(&nav, K_SRC);

    // Then
    ASSERT_EQ_INT(K_SRC, nav.kind);
    ASSERT_EQ_INT(0, (int)nav.history_count);
}

/* Vérifie que nav_restore_view referme l'arbre et la liste des enfants. */
TEST(nav_restore_view_closes_the_tree_and_the_child_menu) {
    // Given
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.tree = 1;
    nav.child_menu = 1;

    // When
    nav_restore_view(&nav, K_FAM);

    // Then
    ASSERT_EQ_INT(0, nav.tree);
    ASSERT_EQ_INT(0, nav.child_menu);
}

/* Enregistre les scénarios d'état, historique et recherche puis retourne leur bilan. */
int main(void) {
    tl_suite("view/nav");
    RUN_TEST(nav_select_individual_selects_the_target_and_records_history);
    RUN_TEST(nav_select_individual_does_not_record_history_when_the_selection_is_unchanged);
    RUN_TEST(nav_select_individual_ignores_a_node_that_is_not_an_individual);
    RUN_TEST(nav_select_individual_ignores_an_unknown_node);
    RUN_TEST(nav_record_uses_the_selection_for_the_current_kind);
    RUN_TEST(nav_record_uses_the_saved_selection_for_an_inactive_kind);
    RUN_TEST(nav_record_returns_null_for_an_empty_list);
    RUN_TEST(nav_record_clamps_an_out_of_range_selection);
    RUN_TEST(nav_restore_record_selects_the_record_in_the_current_kind);
    RUN_TEST(nav_restore_record_updates_the_saved_selection_of_an_inactive_kind);
    RUN_TEST(nav_restore_record_returns_zero_for_an_unknown_xref);
    RUN_TEST(nav_restore_record_returns_zero_for_an_xref_of_another_kind);
    RUN_TEST(nav_restore_view_switches_the_list_without_history);
    RUN_TEST(nav_restore_view_closes_the_tree_and_the_child_menu);
    RUN_TEST(nav_init_starts_on_the_individuals_list_at_the_top);
    RUN_TEST(nav_count_is_the_size_of_the_displayed_list);
    RUN_TEST(nav_current_returns_the_selected_record);
    RUN_TEST(nav_current_returns_null_when_the_list_is_empty);
    RUN_TEST(nav_current_clamps_a_selection_beyond_the_end_of_the_list);
    RUN_TEST(nav_move_down_selects_the_next_item);
    RUN_TEST(nav_move_down_stops_at_the_last_item);
    RUN_TEST(nav_move_up_stops_at_the_first_item);
    RUN_TEST(nav_move_by_a_huge_delta_jumps_to_the_ends);
    RUN_TEST(nav_move_on_an_empty_list_keeps_the_selection_at_zero);
    RUN_TEST(nav_move_in_the_list_resets_the_detail_position);
    RUN_TEST(nav_move_in_the_detail_moves_the_detail_selection_only);
    RUN_TEST(nav_move_in_the_detail_stops_at_the_last_line);
    RUN_TEST(nav_set_kind_switches_the_displayed_list);
    RUN_TEST(nav_set_kind_restores_the_position_of_a_list_when_coming_back);
    RUN_TEST(nav_set_kind_gives_the_focus_back_to_the_list);
    RUN_TEST(nav_set_kind_resets_the_detail_position);
    RUN_TEST(nav_toggle_focus_alternates_between_list_and_detail);
    RUN_TEST(nav_activate_from_the_list_gives_the_focus_to_the_detail);
    RUN_TEST(nav_activate_on_a_link_to_an_individual_selects_that_individual);
    RUN_TEST(nav_activate_on_a_link_to_another_kind_switches_to_that_list);
    RUN_TEST(nav_activate_on_a_line_without_link_changes_nothing);
    RUN_TEST(nav_activate_on_a_link_to_an_unknown_record_changes_nothing);
    RUN_TEST(nav_activate_keeps_the_position_of_the_list_it_leaves);
    RUN_TEST(nav_back_restores_each_linked_record_in_order);
    RUN_TEST(nav_back_does_nothing_when_history_is_empty);
    RUN_TEST(nav_back_restores_the_previous_list_selection);
    RUN_TEST(nav_back_restores_the_previous_view);
    RUN_TEST(nav_search_selects_the_next_item_whose_label_contains_the_text);
    RUN_TEST(nav_search_matches_an_individual_xref);
    RUN_TEST(nav_search_matches_a_citation_source_xref);
    RUN_TEST(nav_search_finds_a_source_by_its_repository_xref);
    RUN_TEST(nav_search_finds_a_source_by_its_repository_name);
    RUN_TEST(nav_search_finds_a_citation_by_the_repository_name_of_its_source);
    RUN_TEST(nav_search_finds_a_citation_by_the_repository_xref_of_its_source);
    RUN_TEST(nav_search_does_not_use_repositories_in_the_individual_view);
    RUN_TEST(nav_search_ignores_case);
    RUN_TEST(nav_search_ignores_case_for_utf8_text);
    RUN_TEST(nav_search_wraps_around_to_the_beginning_of_the_list);
    RUN_TEST(nav_search_starts_after_the_current_selection);
    RUN_TEST(nav_search_next_advances_and_wraps_with_the_saved_term);
    RUN_TEST(nav_search_keeps_the_selection_when_nothing_matches);
    RUN_TEST(nav_search_ignores_an_empty_text);
    RUN_TEST(nav_search_on_an_empty_list_does_nothing);
    RUN_TEST(nav_search_searches_the_labels_of_the_displayed_kind);
    RUN_TEST(nav_search_resets_the_detail_position_on_a_match);
    RUN_TEST(nav_scroll_moves_the_window_down_to_keep_the_selection_visible);
    RUN_TEST(nav_scroll_moves_the_window_up_to_keep_the_selection_visible);
    RUN_TEST(nav_scroll_leaves_the_window_alone_when_the_selection_is_visible);
    RUN_TEST(nav_scroll_keeps_the_detail_selection_visible);
    RUN_TEST(nav_scroll_clamps_the_detail_selection_to_the_number_of_lines);
    RUN_TEST(nav_scroll_resets_the_detail_selection_when_there_are_no_lines);
    return tl_finish();
}
