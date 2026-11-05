#include <stdlib.h>
#include <string.h>

#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/detail.h"
#include "view/nav.h"

/* Tests d'intégration : fichier réel -> parsing -> index -> navigation -> détail.
 * Ils s'exécutent depuis la racine du projet (make test). */

#define SAMPLE "gedcom-test.ged"

/* Parse le GEDCOM d'exemple et construit sa base pour les scénarios d'intégration. */
static Node *load_sample(Database *db) {
    Node *root = gedcom_parse(SAMPLE);
    if (!root) tl_fail(__FILE__, __LINE__, "cannot read %s (run tests from the project root)", SAMPLE);
    fx_track_root(root);
    if (!db_build(db, root)) tl_fail(__FILE__, __LINE__, "duplicate xref in %s", SAMPLE);
    return root;
}

/* Indique si le détail contient un lien vers le XREF demandé. */
static int has_link(const Detail *d, const char *xref) {
    for (int i = 0; i < d->count; i++)
        if (!strcmp(d->lines[i].link, xref)) return 1;
    return 0;
}

/* Indique si le détail contient un lien externe du type et vers la cible demandés. */
static int has_external_link(const Detail *d, LineLinkKind kind, const char *target) {
    for (int i = 0; i < d->count; i++)
        if (d->lines[i].link_kind == kind && !strcmp(d->lines[i].link, target)) return 1;
    return 0;
}

/* Vérifie que le fichier d'exemple fournit au moins un enregistrement pour chaque vue. */
TEST(sample_file_contains_records_for_every_view) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    load_sample(&db);

    // Then
    ASSERT_TRUE(db.lists[K_INDI].count > 0);
    ASSERT_TRUE(db.lists[K_FAM].count > 0);
    ASSERT_TRUE(db.lists[K_SRC].count > 0);
    ASSERT_TRUE(db.lists[K_REPO].count > 0);
    ASSERT_TRUE(db.lists[K_OBJ].count > 0);
    ASSERT_TRUE(db.lists[K_NOTE].count > 0);
    ASSERT_TRUE(db.lists[K_CITA].count > 0);
}

/* Vérifie que chaque type autre qu'un individu sans nom a un libellé non vide. */
TEST(every_record_of_the_sample_has_a_non_empty_label) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);

    for (int k = 0; k < K_COUNT; k++)
        for (int i = 0; i < db.lists[k].count; i++) {
            // When
            char label[256];
            record_label(&db, (Kind)k, db.lists[k].items[i], label, sizeof label);

            // Then
            if (k != K_INDI) ASSERT_TRUE(label[0] != '\0');
        }
}

/* Vérifie que chaque enregistrement de l'exemple produit un détail affichable. */
TEST(every_record_of_the_sample_can_be_rendered_in_the_detail_panel) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    for (int k = 0; k < K_COUNT; k++)
        for (int i = 0; i < db.lists[k].count; i++) {
            // When
            detail_build(&d, &db, (Kind)k, db.lists[k].items[i]);

            // Then
            ASSERT_TRUE(d.count >= 2);
        }
    detail_destroy(&d);
}

/* Vérifie que chaque lien généré par un détail cible un enregistrement existant. */
TEST(every_link_shown_in_the_detail_panel_leads_to_an_existing_record) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};

    for (int k = 0; k < K_COUNT; k++)
        for (int i = 0; i < db.lists[k].count; i++) {
            // When
            detail_build(&d, &db, (Kind)k, db.lists[k].items[i]);

            // Then
            for (int l = 0; l < d.count; l++)
                if (d.lines[l].link[0] && d.lines[l].link_kind == LINE_LINK_RECORD)
                    ASSERT_NOT_NULL(db_find(&db, d.lines[l].link));
        }
    detail_destroy(&d);
}

/* Vérifie que le GEDCOM d'exemple expose ses URL de note et ses fichiers média comme liens distincts. */
TEST(sample_notes_and_media_have_openable_external_links) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const Node *note = db_find(&db, "@N1@");
    const Node *media = db_find(&db, "@M1@");

    // When
    detail_build(&d, &db, K_NOTE, note);
    int has_https = has_external_link(&d, LINE_LINK_URL, "https://example.com/gedview-note");
    int has_http = has_external_link(&d, LINE_LINK_URL, "http://example.org/gedview-note.png");
    detail_build(&d, &db, K_OBJ, media);

    // Then
    ASSERT_TRUE(has_https);
    ASSERT_TRUE(has_http);
    ASSERT_TRUE(has_external_link(&d, LINE_LINK_MEDIA, "media/portrait-famille.svg"));
    ASSERT_TRUE(has_external_link(&d, LINE_LINK_MEDIA, "media/acte-naissance.pdf"));
    detail_destroy(&d);
}

/* Vérifie qu'un lien vers une famille puis le retour restaure l'individu initial. */
TEST(browsing_from_an_individual_to_a_family_and_back_works_end_to_end) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const Node *start = NULL;
    int link_line = -1;
    for (int i = 0; i < db.lists[K_INDI].count && link_line < 0; i++) {
        nav.sel = i;
        start = nav_current(&nav, &db);
        detail_build(&d, &db, K_INDI, start);
        for (int l = 0; l < d.count; l++) {
            const Node *target = d.lines[l].link[0] ? db_find(&db, d.lines[l].link) : NULL;
            if (target && !strcmp(target->tag, "FAM")) {
                link_line = l;
                break;
            }
        }
    }
    ASSERT_TRUE(link_line >= 0);

    // When
    nav.focus_detail = 1;
    nav.det_sel = link_line;
    nav_activate(&nav, &db, &d);
    int kind_after_follow = nav.kind;
    nav_set_kind(&nav, K_INDI);

    // Then
    ASSERT_EQ_INT(K_FAM, kind_after_follow);
    ASSERT_TRUE(nav_current(&nav, &db) == start);
    detail_destroy(&d);
}

/* Vérifie que le suivi d'une citation vers sa source puis le retour restaure la citation. */
TEST(browsing_from_a_citation_to_its_source_and_back_works_end_to_end) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav_set_kind(&nav, K_CITA);
    const Node *citation = nav_current(&nav, &db);
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    detail_build(&d, &db, K_CITA, citation);
    const Node *source = NULL;
    int link_line = -1;
    for (int i = 0; i < d.count; i++) {
        if (d.lines[i].link_kind != LINE_LINK_RECORD || !d.lines[i].link[0]) continue;
        const Node *target = db_find(&db, d.lines[i].link);
        if (target && !strcmp(target->tag, "SOUR")) {
            source = target;
            link_line = i;
            break;
        }
    }
    ASSERT_NOT_NULL(source);

    // When
    nav.focus_detail = 1;
    nav.det_sel = link_line;
    nav_activate(&nav, &db, &d);
    int kind_after_follow = nav.kind;
    const Node *selected_source = nav_current(&nav, &db);
    nav_back(&nav);

    // Then
    ASSERT_EQ_INT(K_SRC, kind_after_follow);
    ASSERT_TRUE(selected_source == source);
    ASSERT_EQ_INT(K_CITA, nav.kind);
    ASSERT_TRUE(nav_current(&nav, &db) == citation);
    detail_destroy(&d);
    nav_destroy(&nav);
}

/* Vérifie que le détail d'une famille contient des liens vers ses membres. */
TEST(a_family_detail_links_to_its_members) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Detail d TL_AUTOCLEAN(detail_destroy) = {0};
    const Node *family = NULL;
    const Node *husband = NULL;
    for (int i = 0; i < db.lists[K_FAM].count && !husband; i++) {
        family = db.lists[K_FAM].items[i];
        husband = node_sub(family, "HUSB");
    }
    ASSERT_NOT_NULL(husband);

    // When
    detail_build(&d, &db, K_FAM, family);

    // Then
    ASSERT_TRUE(has_link(&d, husband->value));
    detail_destroy(&d);
}

/* Vérifie que rechercher un patronyme de l'exemple sélectionne le bon individu. */
TEST(searching_a_name_of_the_sample_selects_the_matching_individual) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    load_sample(&db);
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);

    // When
    nav_search(&nav, &db, "anselin");

    // Then
    char label[256];
    record_label(&db, K_INDI, nav_current(&nav, &db), label, sizeof label);
    ASSERT_EQ_STR("ANSELIN, Marcel", label);
}

/* Enregistre les parcours réels du fichier d'exemple et retourne leur bilan. */
int main(void) {
    tl_suite("integration/sample");
    RUN_TEST(sample_file_contains_records_for_every_view);
    RUN_TEST(every_record_of_the_sample_has_a_non_empty_label);
    RUN_TEST(every_record_of_the_sample_can_be_rendered_in_the_detail_panel);
    RUN_TEST(every_link_shown_in_the_detail_panel_leads_to_an_existing_record);
    RUN_TEST(sample_notes_and_media_have_openable_external_links);
    RUN_TEST(browsing_from_an_individual_to_a_family_and_back_works_end_to_end);
    RUN_TEST(browsing_from_a_citation_to_its_source_and_back_works_end_to_end);
    RUN_TEST(a_family_detail_links_to_its_members);
    RUN_TEST(searching_a_name_of_the_sample_selects_the_matching_individual);
    return tl_finish();
}
