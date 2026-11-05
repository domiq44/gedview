#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i18n/i18n.h"
#include "support/fixtures.h"
#include "support/testlib.h"
#include "ui/input.h"

/* Individus triés : Alice (I1), Bob (I2), Carol (I3). Une famille F1. */
static const char *TREE = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n"
                          "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n"
                          "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n"
                          "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I1@\n";

/* Doubles des actions d'interface. */
static int help_calls;
static int prompt_result;
static const char *prompt_text;
static const char *opened_target;
static const char *opened_gedcom_path;
static LineLinkKind opened_kind;
static int color_toggle_calls;

/* Fournit une taille de page stable aux tests de routage clavier. */
static int fake_page_size(void) { return 2; }

/* Simule la saisie utilisateur et copie la réponse configurée dans buf. */
static int fake_prompt(char *buf, size_t sz) {
    snprintf(buf, sz, "%s", prompt_text);
    return prompt_result;
}

/* Compte les demandes d'ouverture de la page d'aide. */
static void fake_help(void) { help_calls++; }

/* Mémorise la cible transmise au service d'ouverture externe simulé. */
static void fake_open_external(const char *target, LineLinkKind kind, const char *gedcom_path) {
    opened_target = target;
    opened_kind = kind;
    opened_gedcom_path = gedcom_path;
}

/* Compte les demandes de bascule des couleurs. */
static void fake_toggle_colors(void) { color_toggle_calls++; }

/* Simule un clic sur la case d'arbre configurée. */
static int picked_box;
static int fake_mouse_pick(void) { return picked_box; }

static const InputHooks HOOKS = {
    fake_page_size,    fake_prompt,        fake_help,       fake_open_external,
    "gedcom-test.ged", fake_toggle_colors, fake_mouse_pick,
};

/* Envoie key au routeur avec les hooks simulés partagés par les scénarios. */
static int press(int key, const Database *db, Nav *nav, const Detail *d) {
    return input_handle_key(key, db, nav, d, &HOOKS);
}

/* Réinitialise les réponses et compteurs des doubles entre deux scénarios. */
static void reset_doubles(void) {
    help_calls = 0;
    prompt_result = 0;
    prompt_text = "";
    opened_target = NULL;
    opened_gedcom_path = NULL;
    opened_kind = LINE_LINK_RECORD;
    color_toggle_calls = 0;
    picked_box = 0;
}

/* Vérifie que les touches q et Q demandent l'arrêt de la boucle principale. */
TEST(input_quit_keys_ask_the_program_to_stop) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    const int keys[] = {'q', 'Q'};

    for (int i = 0; i < 2; i++) {
        // When
        int keep_running = press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(0, keep_running);
    }
}

/* Vérifie qu'une touche inconnue laisse l'état inchangé et conserve l'exécution. */
TEST(input_unknown_key_changes_nothing_and_keeps_running) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;
    Detail d = {0};

    // When
    int keep_running = press('x', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, keep_running);
    ASSERT_EQ_INT(1, nav.sel);
    ASSERT_EQ_INT(K_INDI, nav.kind);
}

/* Vérifie qu'une touche de vue ouvre la liste correspondant à son type. */
TEST(input_view_key_switches_to_the_matching_list) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    int keep_running = press('f', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, keep_running);
    ASSERT_EQ_INT(K_FAM, nav.kind);
}

/* Vérifie que v et V demandent le changement du mode couleur. */
TEST(input_color_keys_toggle_the_display_mode) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    reset_doubles();
    const int keys[] = {'v', 'V'};

    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) {
        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT((int)i + 1, color_toggle_calls);
    }
    nav_destroy(&nav);
}

/* Vérifie que Tab bascule le focus entre liste et détail. */
TEST(input_tab_toggles_the_focus_between_list_and_detail) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    press('\t', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, nav.focus_detail);
}

/* Vérifie que KEY_DOWN et j déplacent la sélection vers le bas. */
TEST(input_down_keys_move_the_selection_down) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Detail d = {0};
    const int keys[] = {KEY_DOWN, 'j'};

    for (int i = 0; i < 2; i++) {
        Nav nav TL_AUTOCLEAN(nav_destroy);
        nav_init(&nav);

        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(1, nav.sel);
    }
}

/* Vérifie que KEY_UP et k déplacent la sélection vers le haut. */
TEST(input_up_keys_move_the_selection_up) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Detail d = {0};
    const int keys[] = {KEY_UP, 'k'};

    for (int i = 0; i < 2; i++) {
        Nav nav TL_AUTOCLEAN(nav_destroy);
        nav_init(&nav);
        nav.sel = 2;

        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(1, nav.sel);
    }
}

/* Vérifie que Page Down déplace la sélection d'une taille de page. */
TEST(input_page_down_moves_by_the_number_of_visible_lines) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    press(KEY_NPAGE, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(2, nav.sel);
}

/* Vérifie que Page Up remonte la sélection d'une taille de page. */
TEST(input_page_up_moves_back_by_the_number_of_visible_lines) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 2;
    Detail d = {0};

    // When
    press(KEY_PPAGE, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que End et Home sélectionnent respectivement le dernier et le premier élément. */
TEST(input_end_and_home_jump_to_the_last_and_first_items) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    press(KEY_END, &db, &nav, &d);
    int after_end = nav.sel;
    press(KEY_HOME, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(2, after_end);
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que les deux codes Entrée donnent le focus au détail. */
TEST(input_enter_gives_the_focus_to_the_detail) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    const int keys[] = {'\n', KEY_ENTER};

    for (int i = 0; i < 2; i++) {
        nav.focus_detail = 0;

        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(1, nav.focus_detail);
    }
}

/* Vérifie qu'Entrée transmet les liens URL et média au gestionnaire externe injecté. */
TEST(input_enter_opens_external_links) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Line lines[] = {
        {.text = "NOTE: https://example.test", .link = "https://example.test", .link_kind = LINE_LINK_URL},
        {.text = "FILE: media/photo.jpg", .link = "media/photo.jpg", .link_kind = LINE_LINK_MEDIA},
    };
    Detail d = {.lines = lines, .count = 2};
    const LineLinkKind kinds[] = {LINE_LINK_URL, LINE_LINK_MEDIA};
    const char *targets[] = {"https://example.test", "media/photo.jpg"};
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.focus_detail = 1;

    for (int i = 0; i < 2; i++) {
        nav.det_sel = i;
        reset_doubles();

        // When
        press('\n', &db, &nav, &d);

        // Then
        ASSERT_EQ_STR(targets[i], opened_target);
        ASSERT_EQ_INT(kinds[i], opened_kind);
        ASSERT_EQ_STR("gedcom-test.ged", opened_gedcom_path);
    }
    nav_destroy(&nav);
}

/* Vérifie que les variantes courantes de Retour arrière restaurent l'enregistrement précédent. */
TEST(input_backspace_keys_return_to_the_previous_record) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Detail d = {0};
    Line lines[] = {{"FAMS: @F1@", "@F1@", LINE_LINK_RECORD, 0, 0}};
    d.lines = lines;
    d.count = 1;
    const int keys[] = {KEY_BACKSPACE, 127, 8};

    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) {
        Nav nav TL_AUTOCLEAN(nav_destroy);
        nav_init(&nav);
        nav.focus_detail = 1;
        nav_activate(&nav, &db, &d);

        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(K_INDI, nav.kind);
        ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
    }
}

/* Vérifie que Retour arrière annule aussi un déplacement entre les individus de la liste. */
TEST(input_backspace_undoes_a_list_selection_change) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('j', &db, &nav, &d);

    // When
    press(127, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(0, nav.sel);
    nav_destroy(&nav);
}

/* Vérifie qu'une recherche validée sélectionne l'élément correspondant au texte saisi. */
TEST(input_search_selects_the_item_matching_the_text_typed_by_the_user) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    reset_doubles();
    prompt_text = "carol";
    prompt_result = 1;

    // When
    press('/', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(2, nav.sel);
}

/* Vérifie que F3, g et G reprennent le dernier terme sans demander de nouvelle saisie. */
TEST(input_f3_searches_the_next_match_without_prompting_again) {
    // Given
    const int keys[] = {KEY_F(3), 'g', 'G'};
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) {
        Database db TL_AUTOCLEAN(db_destroy);
        fx_database(TREE, &db);
        Nav nav TL_AUTOCLEAN(nav_destroy);
        nav_init(&nav);
        Detail d = {0};
        reset_doubles();
        prompt_text = "a";
        prompt_result = 1;

        // When
        press('/', &db, &nav, &d);
        prompt_result = 0;
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(2, nav.sel);
        nav_destroy(&nav);
    }
}

/* Vérifie que l'annulation de la saisie de recherche ne change pas la sélection. */
TEST(input_search_does_nothing_when_the_user_cancels_the_prompt) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    reset_doubles();
    prompt_text = "carol";
    prompt_result = 0;

    // When
    press('/', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(0, nav.sel);
}

/* Vérifie que h, H et F1 déclenchent chacun l'affichage de l'aide. */
TEST(input_help_keys_show_the_help_page) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    reset_doubles();
    const int keys[] = {'h', 'H', KEY_F(1)};

    for (int i = 0; i < 3; i++) {
        // When
        press(keys[i], &db, &nav, &d);
    }

    // Then
    ASSERT_EQ_INT(3, help_calls);
}

/* Vérifie que la touche l passe au catalogue de langue suivant. */
TEST(input_language_key_switches_to_the_next_language) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(TREE, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    const char *dir = fx_tempdir();
    fx_write(dir, "en.lang", "NONAME=Unnamed\n");
    fx_write(dir, "fr.lang", "NONAME=Sans nom\n");
    setenv("GEDVIEW_LANG_DIR", dir, 1);
    lang_init("en");

    // When
    press('l', &db, &nav, &d);

    // Then
    ASSERT_EQ_STR("Sans nom", T(S_NONAME));
}

/* Alice (I1) a pour père Bob (I2) et pour mère Carol (I3). */
static const char *ANCESTRY = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMC @F1@\n"
                              "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n"
                              "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n"
                              "0 @F1@ FAM\n1 HUSB @I2@\n1 WIFE @I3@\n1 CHIL @I1@\n";

/* Vérifie que la touche t ouvre l'arbre et qu'une seconde pression le ferme. */
TEST(input_t_key_toggles_the_ancestor_tree) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};

    // When
    press('t', &db, &nav, &d);
    int opened = nav.tree;
    press('t', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, opened);
    ASSERT_EQ_INT(0, nav.tree);
}

/* Vérifie que flèche droite puis Entrée recentrent l'arbre sur le père. */
TEST(input_tree_right_then_enter_recenters_on_the_father) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);

    // When
    press(KEY_RIGHT, &db, &nav, &d);
    press('\n', &db, &nav, &d);

    // Then
    ASSERT_EQ_STR("@I2@", nav_current(&nav, &db)->xref);
}

/* Vérifie que les flèches gauche, haut et bas déplacent la sélection de l'arbre. */
TEST(input_tree_arrows_move_the_selection_between_boxes) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);
    press(KEY_RIGHT, &db, &nav, &d);

    // When
    press(KEY_DOWN, &db, &nav, &d);
    int mother = nav.tree_sel;
    press('k', &db, &nav, &d);
    int father = nav.tree_sel;
    press('j', &db, &nav, &d);
    press(KEY_UP, &db, &nav, &d);
    press(KEY_LEFT, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(3, mother);
    ASSERT_EQ_INT(2, father);
    ASSERT_EQ_INT(1, nav.tree_sel);
}

/* Vérifie qu'un clic sur une case de l'arbre en fait l'individu courant. */
TEST(input_tree_mouse_click_recenters_on_the_clicked_box) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);
    picked_box = 3;

    // When
    press(KEY_MOUSE, &db, &nav, &d);

    // Then
    ASSERT_EQ_STR("@I3@", nav_current(&nav, &db)->xref);
}

/* Vérifie qu'un clic hors de toute case ne change pas l'individu courant. */
TEST(input_tree_mouse_click_outside_a_box_changes_nothing) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);

    // When
    press(KEY_MOUSE, &db, &nav, &d);

    // Then
    ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
}

/* Vérifie que les déplacements de liste sont sans effet pendant l'affichage de l'arbre. */
TEST(input_tree_ignores_list_navigation_keys) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);
    const int keys[] = {KEY_NPAGE, KEY_END, '\t', '/'};

    for (int i = 0; i < 4; i++) {
        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(0, nav.sel);
        ASSERT_EQ_INT(0, nav.focus_detail);
    }
}

/* Vérifie qu'une touche de vue ferme l'arbre et affiche la liste demandée. */
TEST(input_view_key_closes_the_tree) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(ANCESTRY, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);

    // When
    press('f', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(0, nav.tree);
    ASSERT_EQ_INT(K_FAM, nav.kind);
}

/* Alice (I1) a deux enfants : Bob (I2) et Carol (I3). */
static const char *CHILDREN = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n1 FAMS @F1@\n"
                              "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n1 FAMC @F1@\n"
                              "0 @I3@ INDI\n1 NAME Carol/CHARLIE/\n1 FAMC @F1@\n"
                              "0 @F1@ FAM\n1 HUSB @I1@\n1 CHIL @I2@\n1 CHIL @I3@\n";

/* Vérifie que ← sur la personne centrale ouvre la liste des enfants, puis que ↓ et Entrée choisissent le
 * second. */
TEST(input_tree_left_on_the_root_lets_the_user_pick_a_child) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);

    // When
    press(KEY_LEFT, &db, &nav, &d);
    int opened = nav.child_menu;
    press(KEY_DOWN, &db, &nav, &d);
    press('\n', &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, opened);
    ASSERT_EQ_INT(0, nav.child_menu);
    ASSERT_EQ_STR("@I3@", nav_current(&nav, &db)->xref);
}

/* Vérifie que k et ↑ remontent dans la liste des enfants. */
TEST(input_children_list_up_keys_move_back_up) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);
    press(KEY_LEFT, &db, &nav, &d);
    press('j', &db, &nav, &d);
    int down = nav.child_sel;

    // When
    press('k', &db, &nav, &d);
    int after_k = nav.child_sel;
    press('j', &db, &nav, &d);
    press(KEY_UP, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, down);
    ASSERT_EQ_INT(0, after_k);
    ASSERT_EQ_INT(0, nav.child_sel);
}

/* Vérifie que Échap et → ferment la liste des enfants sans changer d'individu. */
TEST(input_children_list_closes_on_escape_or_right_arrow) {
    // Given
    reset_doubles();
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    const int keys[] = {27, KEY_RIGHT};
    press('t', &db, &nav, &d);

    for (int i = 0; i < 2; i++) {
        press(KEY_LEFT, &db, &nav, &d);

        // When
        press(keys[i], &db, &nav, &d);

        // Then
        ASSERT_EQ_INT(0, nav.child_menu);
        ASSERT_EQ_STR("@I1@", nav_current(&nav, &db)->xref);
    }
}

/* Vérifie que ← et la souris sont sans effet pendant que la liste des enfants est ouverte. */
TEST(input_children_list_ignores_left_arrow_and_mouse) {
    // Given
    reset_doubles();
    picked_box = 1;
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(CHILDREN, &db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d = {0};
    press('t', &db, &nav, &d);
    press(KEY_LEFT, &db, &nav, &d);

    // When
    press(KEY_LEFT, &db, &nav, &d);
    press(KEY_MOUSE, &db, &nav, &d);

    // Then
    ASSERT_EQ_INT(1, nav.child_menu);
    ASSERT_EQ_INT(0, nav.child_sel);
}

/* Enregistre les scénarios de saisie et retourne leur bilan au processus appelant. */
int main(void) {
    tl_suite("ui/input");
    RUN_TEST(input_tree_left_on_the_root_lets_the_user_pick_a_child);
    RUN_TEST(input_children_list_up_keys_move_back_up);
    RUN_TEST(input_children_list_closes_on_escape_or_right_arrow);
    RUN_TEST(input_children_list_ignores_left_arrow_and_mouse);
    RUN_TEST(input_t_key_toggles_the_ancestor_tree);
    RUN_TEST(input_tree_right_then_enter_recenters_on_the_father);
    RUN_TEST(input_tree_arrows_move_the_selection_between_boxes);
    RUN_TEST(input_tree_mouse_click_recenters_on_the_clicked_box);
    RUN_TEST(input_tree_mouse_click_outside_a_box_changes_nothing);
    RUN_TEST(input_tree_ignores_list_navigation_keys);
    RUN_TEST(input_view_key_closes_the_tree);
    RUN_TEST(input_quit_keys_ask_the_program_to_stop);
    RUN_TEST(input_unknown_key_changes_nothing_and_keeps_running);
    RUN_TEST(input_view_key_switches_to_the_matching_list);
    RUN_TEST(input_color_keys_toggle_the_display_mode);
    RUN_TEST(input_tab_toggles_the_focus_between_list_and_detail);
    RUN_TEST(input_down_keys_move_the_selection_down);
    RUN_TEST(input_up_keys_move_the_selection_up);
    RUN_TEST(input_page_down_moves_by_the_number_of_visible_lines);
    RUN_TEST(input_page_up_moves_back_by_the_number_of_visible_lines);
    RUN_TEST(input_end_and_home_jump_to_the_last_and_first_items);
    RUN_TEST(input_enter_gives_the_focus_to_the_detail);
    RUN_TEST(input_enter_opens_external_links);
    RUN_TEST(input_backspace_keys_return_to_the_previous_record);
    RUN_TEST(input_backspace_undoes_a_list_selection_change);
    RUN_TEST(input_search_selects_the_item_matching_the_text_typed_by_the_user);
    RUN_TEST(input_f3_searches_the_next_match_without_prompting_again);
    RUN_TEST(input_search_does_nothing_when_the_user_cancels_the_prompt);
    RUN_TEST(input_help_keys_show_the_help_page);
    RUN_TEST(input_language_key_switches_to_the_next_language);
    return tl_finish();
}
