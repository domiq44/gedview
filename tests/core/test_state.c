#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/state.h"
#include "support/fixtures.h"
#include "support/testlib.h"

/* Dirige l'état vers un sous-dossier neuf (non créé) du dossier temporaire et retourne son chemin. */
static const char *fresh_state_dir(void) {
    static char dir[512];
    snprintf(dir, sizeof dir, "%s/a/b/state", fx_tempdir());
    setenv("GEDVIEW_STATE_DIR", dir, 1);
    return dir;
}

/* Crée un fichier GEDCOM vide dans un dossier temporaire et retourne son chemin. */
static const char *gedcom_file(const char *name) {
    static char path[512];
    const char *dir = fx_tempdir();
    fx_write(dir, name, "0 HEAD\n");
    snprintf(path, sizeof path, "%s/%s", dir, name);
    return path;
}

/* Vérifie qu'une valeur enregistrée est relue, y compris dans des dossiers créés à la volée. */
TEST(state_save_then_load_returns_the_saved_value_and_creates_directories) {
    // Given
    fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    const StateEntry entries[] = {{"INDI", "@I195@"}, {"FAM", "@F3@"}};
    char value[64];

    // When
    int saved = state_save(ged, entries, 2);
    int loaded = state_load(ged, "FAM", value, sizeof value);

    // Then
    ASSERT_EQ_INT(1, saved);
    ASSERT_EQ_INT(1, loaded);
    ASSERT_EQ_STR("@F3@", value);
}

/* Vérifie qu'un slot jamais enregistré, ou une mémoire absente, ne renvoie rien. */
TEST(state_load_returns_zero_when_nothing_is_saved) {
    // Given
    fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    char value[64] = "unchanged";

    // When
    int without_file = state_load(ged, "INDI", value, sizeof value);
    state_save(ged, &(StateEntry){"FAM", "@F1@"}, 1);
    int without_slot = state_load(ged, "INDI", value, sizeof value);

    // Then
    ASSERT_EQ_INT(0, without_file);
    ASSERT_EQ_INT(0, without_slot);
    ASSERT_EQ_STR("unchanged", value);
}

/* Vérifie qu'une nouvelle sauvegarde remplace entièrement les anciennes valeurs du même fichier. */
TEST(state_save_replaces_the_previous_values_of_the_same_file) {
    // Given
    fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    state_save(ged, (StateEntry[]){{"INDI", "@I1@"}, {"FAM", "@F1@"}}, 2);
    char value[64];

    // When
    state_save(ged, &(StateEntry){"INDI", "@I2@"}, 1);

    // Then
    ASSERT_EQ_INT(1, state_load(ged, "INDI", value, sizeof value));
    ASSERT_EQ_STR("@I2@", value);
    ASSERT_EQ_INT(0, state_load(ged, "FAM", value, sizeof value));
}

/* Vérifie que deux fichiers GEDCOM ont chacun leur mémoire. */
TEST(state_keeps_separate_values_for_each_gedcom_file) {
    // Given
    fresh_state_dir();
    const char *first = gedcom_file("one.ged");
    char first_copy[512];
    snprintf(first_copy, sizeof first_copy, "%s", first);
    const char *second = gedcom_file("two.ged");
    state_save(first_copy, &(StateEntry){"INDI", "@I1@"}, 1);
    state_save(second, &(StateEntry){"INDI", "@I2@"}, 1);
    char value[64];

    // When
    int loaded = state_load(first_copy, "INDI", value, sizeof value);

    // Then
    ASSERT_EQ_INT(1, loaded);
    ASSERT_EQ_STR("@I1@", value);
}

/* Vérifie qu'un chemin équivalent (avec « ./ ») retrouve la même mémoire. */
TEST(state_identifies_a_file_by_its_canonical_path) {
    // Given
    fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    char alias[600];
    snprintf(alias, sizeof alias, "%.*s/./a.ged", (int)(strrchr(ged, '/') - ged), ged);
    state_save(ged, &(StateEntry){"INDI", "@I7@"}, 1);
    char value[64];

    // When
    int loaded = state_load(alias, "INDI", value, sizeof value);

    // Then
    ASSERT_EQ_INT(1, loaded);
    ASSERT_EQ_STR("@I7@", value);
}

/* Vérifie qu'un tampon trop petit fait échouer la lecture sans le modifier. */
TEST(state_load_fails_when_the_buffer_is_too_small) {
    // Given
    fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    state_save(ged, &(StateEntry){"INDI", "@I195@"}, 1);
    char value[4] = "ok";

    // When
    int loaded = state_load(ged, "INDI", value, sizeof value);

    // Then
    ASSERT_EQ_INT(0, loaded);
    ASSERT_EQ_STR("ok", value);
}

/* Vérifie qu'un slot ou une valeur vide ou contenant tabulation/saut de ligne est refusé sans rien écrire. */
TEST(state_save_rejects_invalid_fields) {
    // Given
    fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    const StateEntry bad[] = {
        {"", "@I1@"}, {"INDI", ""}, {"IN\tDI", "@I1@"}, {"INDI", "@I\n1@"}, {NULL, "@I1@"},
    };
    char value[64];

    for (int i = 0; i < 5; i++) {
        // When
        int saved = state_save(ged, &bad[i], 1);

        // Then
        ASSERT_EQ_INT(0, saved);
        ASSERT_EQ_INT(0, state_load(ged, "INDI", value, sizeof value));
    }
}

/* Vérifie que les lignes mal formées du fichier d'état sont ignorées sans gêner les valeurs valides. */
TEST(state_ignores_malformed_lines) {
    // Given
    const char *dir = fresh_state_dir();
    const char *ged = gedcom_file("a.ged");
    state_save(ged, &(StateEntry){"FAM", "@F1@"}, 1);
    char file[600], content[1024] = "garbage\nINDI\t\t/x\n\tvalue\t/x\n";
    snprintf(file, sizeof file, "%s/last-person", dir);
    FILE *f = fopen(file, "r");
    ASSERT_NOT_NULL(f);
    size_t used = strlen(content);
    used += fread(content + used, 1, sizeof content - used - 1, f);
    content[used] = 0;
    fclose(f);
    f = fopen(file, "w");
    ASSERT_NOT_NULL(f);
    fputs(content, f);
    fclose(f);
    char value[64];

    // When
    int kept = state_load(ged, "FAM", value, sizeof value);
    state_save(ged, &(StateEntry){"INDI", "@I1@"}, 1);

    // Then
    ASSERT_EQ_INT(1, kept);
    ASSERT_EQ_INT(1, state_load(ged, "INDI", value, sizeof value));
}

/* Vérifie qu'en l'absence de GEDVIEW_STATE_DIR le dossier est $XDG_STATE_HOME/gedview. */
TEST(state_falls_back_to_xdg_state_home) {
    // Given
    unsetenv("GEDVIEW_STATE_DIR");
    char xdg[512];
    snprintf(xdg, sizeof xdg, "%s/xdg", fx_tempdir());
    setenv("XDG_STATE_HOME", xdg, 1);
    const char *ged = gedcom_file("a.ged");
    char value[64], file[600];
    snprintf(file, sizeof file, "%s/gedview/last-person", xdg);

    // When
    int saved = state_save(ged, &(StateEntry){"INDI", "@I1@"}, 1);

    // Then
    ASSERT_EQ_INT(1, saved);
    ASSERT_EQ_INT(1, state_load(ged, "INDI", value, sizeof value));
    ASSERT_EQ_INT(0, remove(file));
    unsetenv("XDG_STATE_HOME");
}

/* Vérifie qu'en dernier recours le dossier est ~/.local/state/gedview. */
TEST(state_falls_back_to_the_home_directory) {
    // Given
    unsetenv("GEDVIEW_STATE_DIR");
    unsetenv("XDG_STATE_HOME");
    char home[512];
    snprintf(home, sizeof home, "%s/home", fx_tempdir());
    setenv("HOME", home, 1);
    const char *ged = gedcom_file("a.ged");
    char value[64];

    // When
    int saved = state_save(ged, &(StateEntry){"INDI", "@I1@"}, 1);

    // Then
    ASSERT_EQ_INT(1, saved);
    ASSERT_EQ_INT(1, state_load(ged, "INDI", value, sizeof value));
}

/* Vérifie qu'aucune mémorisation n'est possible sans variable permettant de trouver un dossier. */
TEST(state_fails_without_any_usable_directory_variable) {
    // Given
    unsetenv("GEDVIEW_STATE_DIR");
    unsetenv("XDG_STATE_HOME");
    unsetenv("HOME");
    const char *ged = gedcom_file("a.ged");
    char value[64];

    // When
    int saved = state_save(ged, &(StateEntry){"INDI", "@I1@"}, 1);
    int loaded = state_load(ged, "INDI", value, sizeof value);

    // Then
    ASSERT_EQ_INT(0, saved);
    ASSERT_EQ_INT(0, loaded);
}

/* Vérifie qu'un dossier d'état non créable (un fichier le bloque) fait échouer la sauvegarde. */
TEST(state_save_fails_when_the_directory_cannot_be_created) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "blocker", "x");
    char blocked[600];
    snprintf(blocked, sizeof blocked, "%s/blocker/state", dir);
    setenv("GEDVIEW_STATE_DIR", blocked, 1);
    const char *ged = gedcom_file("a.ged");

    // When
    int saved = state_save(ged, &(StateEntry){"INDI", "@I1@"}, 1);

    // Then
    ASSERT_EQ_INT(0, saved);
}

int main(void) {
    tl_suite("core/state");
    RUN_TEST(state_save_then_load_returns_the_saved_value_and_creates_directories);
    RUN_TEST(state_load_returns_zero_when_nothing_is_saved);
    RUN_TEST(state_save_replaces_the_previous_values_of_the_same_file);
    RUN_TEST(state_keeps_separate_values_for_each_gedcom_file);
    RUN_TEST(state_identifies_a_file_by_its_canonical_path);
    RUN_TEST(state_load_fails_when_the_buffer_is_too_small);
    RUN_TEST(state_save_rejects_invalid_fields);
    RUN_TEST(state_ignores_malformed_lines);
    RUN_TEST(state_falls_back_to_xdg_state_home);
    RUN_TEST(state_falls_back_to_the_home_directory);
    RUN_TEST(state_fails_without_any_usable_directory_variable);
    RUN_TEST(state_save_fails_when_the_directory_cannot_be_created);
    return tl_finish();
}
