#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "support/fixtures.h"
#include "support/testlib.h"
#include "view/session.h"

/* Deux individus, deux familles et un enregistrement de chaque autre type. */
static const char *RECORDS = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n"
                             "0 @I2@ INDI\n1 NAME Bob/BRAVO/\n"
                             "0 @F1@ FAM\n1 HUSB @I1@\n"
                             "0 @F2@ FAM\n1 HUSB @I2@\n"
                             "0 @S1@ SOUR\n1 TITL Registre\n"
                             "0 @R1@ REPO\n1 NAME Archives\n"
                             "0 @M1@ OBJE\n1 FILE photo.jpg\n"
                             "0 @N1@ NOTE Une note\n";

/* La même base sans Bob ni F2. */
static const char *RECORDS_WITHOUT_BOB = "0 @I1@ INDI\n1 NAME Alice/ALPHA/\n"
                                         "0 @F1@ FAM\n1 HUSB @I1@\n";

/* Dirige l'état vers un dossier temporaire neuf et écrit un fichier GEDCOM nommé name ; retourne son chemin.
 */
static const char *gedcom_with_state(const char *name) {
    static char path[PATH_MAX + 64];
    const char *dir = fx_tempdir();
    setenv("GEDVIEW_STATE_DIR", dir, 1);
    fx_write(dir, name, "0 HEAD\n");
    snprintf(path, sizeof path, "%s/%s", dir, name);
    return path;
}

/* Vérifie que sauvegarder puis restaurer sur un Nav neuf redonne les sélections d'individus et de familles.
 */
TEST(session_save_then_restore_reproduces_the_selections_and_the_view) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS, &db);
    const char *ged = gedcom_with_state("a.ged");
    Nav saved TL_AUTOCLEAN(nav_destroy);
    nav_init(&saved);
    nav_restore_record(&saved, &db, K_INDI, "@I2@");
    nav_restore_record(&saved, &db, K_FAM, "@F2@");
    nav_set_kind(&saved, K_FAM);
    session_save(&saved, &db, ged);
    Nav fresh TL_AUTOCLEAN(nav_destroy);
    nav_init(&fresh);

    // When
    session_restore(&fresh, &db, ged);

    // Then
    ASSERT_EQ_INT(K_FAM, fresh.kind);
    ASSERT_EQ_STR("@F2@", nav_current(&fresh, &db)->xref);
    ASSERT_EQ_STR("@I2@", nav_record(&fresh, &db, K_INDI)->xref);
}

/* Vérifie que la vue Citation est mémorisée comme la vue Individus. */
TEST(session_save_stores_the_citations_view_as_individuals) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS, &db);
    const char *ged = gedcom_with_state("a.ged");
    Nav saved TL_AUTOCLEAN(nav_destroy);
    nav_init(&saved);
    nav_set_kind(&saved, K_CITA);
    session_save(&saved, &db, ged);
    Nav fresh TL_AUTOCLEAN(nav_destroy);
    nav_init(&fresh);
    nav_set_kind(&fresh, K_SRC);

    // When
    session_restore(&fresh, &db, ged);

    // Then
    ASSERT_EQ_INT(K_INDI, fresh.kind);
}

/* Vérifie que les sélections des autres types (source, dépôt, objet, note) sont aussi restaurées. */
TEST(session_save_then_restore_covers_every_memorized_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS, &db);
    const char *ged = gedcom_with_state("a.ged");
    Nav saved TL_AUTOCLEAN(nav_destroy);
    nav_init(&saved);
    session_save(&saved, &db, ged);
    Nav fresh TL_AUTOCLEAN(nav_destroy);
    nav_init(&fresh);
    nav_restore_record(&fresh, &db, K_INDI, "@I2@");

    // When
    session_restore(&fresh, &db, ged);

    // Then
    ASSERT_EQ_STR("@I1@", nav_record(&fresh, &db, K_INDI)->xref);
    ASSERT_EQ_STR("@S1@", nav_record(&fresh, &db, K_SRC)->xref);
    ASSERT_EQ_STR("@R1@", nav_record(&fresh, &db, K_REPO)->xref);
    ASSERT_EQ_STR("@M1@", nav_record(&fresh, &db, K_OBJ)->xref);
    ASSERT_EQ_STR("@N1@", nav_record(&fresh, &db, K_NOTE)->xref);
}

/* Vérifie que les enregistrements disparus du fichier sont ignorés à la restauration. */
TEST(session_restore_ignores_records_that_disappeared) {
    // Given
    Database before TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS, &before);
    Database after TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS_WITHOUT_BOB, &after);
    const char *ged = gedcom_with_state("a.ged");
    Nav saved TL_AUTOCLEAN(nav_destroy);
    nav_init(&saved);
    nav_restore_record(&saved, &before, K_INDI, "@I2@");
    nav_restore_record(&saved, &before, K_FAM, "@F2@");
    session_save(&saved, &before, ged);
    Nav fresh TL_AUTOCLEAN(nav_destroy);
    nav_init(&fresh);

    // When
    session_restore(&fresh, &after, ged);

    // Then
    ASSERT_EQ_STR("@I1@", nav_record(&fresh, &after, K_INDI)->xref);
    ASSERT_EQ_STR("@F1@", nav_record(&fresh, &after, K_FAM)->xref);
}

/* Vérifie que sans état mémorisé la navigation reste inchangée. */
TEST(session_restore_leaves_the_navigation_unchanged_when_nothing_is_saved) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS, &db);
    const char *ged = gedcom_with_state("a.ged");
    Nav nav TL_AUTOCLEAN(nav_destroy);
    nav_init(&nav);
    nav.sel = 1;

    // When
    session_restore(&nav, &db, ged);

    // Then
    ASSERT_EQ_INT(K_INDI, nav.kind);
    ASSERT_EQ_INT(1, nav.sel);
}

/* Vérifie que l'état mémorisé pour un fichier n'est pas visible depuis un autre chemin GEDCOM. */
TEST(session_restore_does_not_see_the_state_of_another_gedcom_path) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database(RECORDS, &db);
    const char *saved_path = gedcom_with_state("a.ged");
    Nav saved TL_AUTOCLEAN(nav_destroy);
    nav_init(&saved);
    nav_restore_record(&saved, &db, K_INDI, "@I2@");
    nav_set_kind(&saved, K_FAM);
    session_save(&saved, &db, saved_path);
    Nav fresh TL_AUTOCLEAN(nav_destroy);
    nav_init(&fresh);
    char other_path[PATH_MAX + 64];
    snprintf(other_path, sizeof other_path, "%s/b.ged", getenv("GEDVIEW_STATE_DIR"));

    // When
    session_restore(&fresh, &db, other_path);

    // Then
    ASSERT_EQ_INT(K_INDI, fresh.kind);
    ASSERT_EQ_INT(0, fresh.sel);
}

/* Enregistre les scénarios de reprise de session et retourne leur bilan. */
int main(void) {
    tl_suite("view/session");
    RUN_TEST(session_save_then_restore_reproduces_the_selections_and_the_view);
    RUN_TEST(session_save_stores_the_citations_view_as_individuals);
    RUN_TEST(session_save_then_restore_covers_every_memorized_kind);
    RUN_TEST(session_restore_ignores_records_that_disappeared);
    RUN_TEST(session_restore_leaves_the_navigation_unchanged_when_nothing_is_saved);
    RUN_TEST(session_restore_does_not_see_the_state_of_another_gedcom_path);
    return tl_finish();
}
