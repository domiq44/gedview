#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "support/fixtures.h"
#include "support/testlib.h"
#include "ui/external_target.h"

/* Vérifie que les URL HTTP(S) sont reconnues sans dépendre du type de lien. */
TEST(external_target_accepts_http_and_https_urls) {
    // Given
    char resolved[PATH_MAX];

    // When
    ExternalTargetType http =
        external_target_resolve("http://example.test", LINE_LINK_URL, NULL, resolved, sizeof resolved);
    ExternalTargetType https =
        external_target_resolve("HTTPS://example.test", LINE_LINK_RECORD, NULL, resolved, sizeof resolved);

    // Then
    ASSERT_EQ_INT(EXTERNAL_TARGET_URL, http);
    ASSERT_EQ_INT(EXTERNAL_TARGET_URL, https);
}

/* Vérifie que les schémas non HTTP, les cibles vides et les types non média sont rejetés. */
TEST(external_target_rejects_unsupported_schemes_and_media_types) {
    // Given
    char resolved[PATH_MAX];

    // When
    ExternalTargetType file_url =
        external_target_resolve("file:///tmp/image.png", LINE_LINK_URL, NULL, resolved, sizeof resolved);
    ExternalTargetType empty = external_target_resolve("", LINE_LINK_MEDIA, NULL, resolved, sizeof resolved);
    ExternalTargetType unsupported =
        external_target_resolve("notes.txt", LINE_LINK_MEDIA, NULL, resolved, sizeof resolved);
    ExternalTargetType record =
        external_target_resolve("image.png", LINE_LINK_RECORD, NULL, resolved, sizeof resolved);

    // Then
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, file_url);
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, empty);
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, unsupported);
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, record);
}

/* Vérifie qu'un média relatif est résolu depuis le GEDCOM et canonisé. */
TEST(external_target_resolves_a_relative_media_path_from_the_gedcom) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "tree.ged", "");
    fx_write(dir, "portrait.JPG", "image");
    char gedcom_path[PATH_MAX], media_path[PATH_MAX], expected[PATH_MAX], resolved[PATH_MAX];
    snprintf(gedcom_path, sizeof gedcom_path, "%s/tree.ged", dir);
    snprintf(media_path, sizeof media_path, "%s/portrait.JPG", dir);
    ASSERT_NOT_NULL(realpath(media_path, expected));

    // When
    ExternalTargetType type =
        external_target_resolve("portrait.JPG", LINE_LINK_MEDIA, gedcom_path, resolved, sizeof resolved);

    // Then
    ASSERT_EQ_INT(EXTERNAL_TARGET_MEDIA, type);
    ASSERT_EQ_STR(expected, resolved);
}

/* Vérifie que les médias absents et les chemins relatifs sans GEDCOM sont rejetés. */
TEST(external_target_rejects_unresolvable_media_paths) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "tree.ged", "");
    char gedcom_path[PATH_MAX], resolved[PATH_MAX];
    snprintf(gedcom_path, sizeof gedcom_path, "%s/tree.ged", dir);

    // When
    ExternalTargetType missing =
        external_target_resolve("missing.pdf", LINE_LINK_MEDIA, gedcom_path, resolved, sizeof resolved);
    ExternalTargetType no_gedcom =
        external_target_resolve("missing.pdf", LINE_LINK_MEDIA, NULL, resolved, sizeof resolved);

    // Then
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, missing);
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, no_gedcom);
}

/* Vérifie qu'un chemin canonique trop long pour le tampon est rejeté sans laisser de préfixe. */
TEST(external_target_rejects_a_resolved_path_that_does_not_fit) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "scan.pdf", "document");
    char media_path[PATH_MAX];
    char resolved[1] = {'x'};
    snprintf(media_path, sizeof media_path, "%s/scan.pdf", dir);

    // When
    ExternalTargetType type =
        external_target_resolve(media_path, LINE_LINK_MEDIA, NULL, resolved, sizeof resolved);

    // Then
    ASSERT_EQ_INT(EXTERNAL_TARGET_NONE, type);
    ASSERT_EQ_STR("", resolved);
}

/* Vérifie qu'un média absolu est accepté après canonicalisation. */
TEST(external_target_resolves_an_absolute_media_path) {
    // Given
    const char *dir = fx_tempdir();
    fx_write(dir, "scan.pdf", "document");
    char media_path[PATH_MAX], expected[PATH_MAX], resolved[PATH_MAX];
    snprintf(media_path, sizeof media_path, "%s/scan.pdf", dir);
    ASSERT_NOT_NULL(realpath(media_path, expected));

    // When
    ExternalTargetType type =
        external_target_resolve(media_path, LINE_LINK_MEDIA, NULL, resolved, sizeof resolved);

    // Then
    ASSERT_EQ_INT(EXTERNAL_TARGET_MEDIA, type);
    ASSERT_EQ_STR(expected, resolved);
}

int main(void) {
    tl_suite("ui/external_target");
    RUN_TEST(external_target_accepts_http_and_https_urls);
    RUN_TEST(external_target_rejects_unsupported_schemes_and_media_types);
    RUN_TEST(external_target_resolves_a_relative_media_path_from_the_gedcom);
    RUN_TEST(external_target_rejects_unresolvable_media_paths);
    RUN_TEST(external_target_rejects_a_resolved_path_that_does_not_fit);
    RUN_TEST(external_target_resolves_an_absolute_media_path);
    return tl_finish();
}