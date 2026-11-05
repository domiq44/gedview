#include <stddef.h>

#include "core/version.h"
#include "support/testlib.h"

/* Vérifie que la version fournie par le build n'est jamais une chaîne vide. */
TEST(app_version_is_never_empty) {
    // Given
    // (version injectée par le Makefile ou valeur de repli "dev")

    // When
    const char *version = app_version();

    // Then
    ASSERT_NOT_NULL(version);
    ASSERT_TRUE(version[0] != '\0');
}

/* Enregistre le scénario de version et retourne son bilan au processus appelant. */
int main(void) {
    tl_suite("core/version");
    RUN_TEST(app_version_is_never_empty);
    return tl_finish();
}
