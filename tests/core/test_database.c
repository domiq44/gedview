#include "core/database.h"
#include "support/fixtures.h"
#include "support/testlib.h"

/* Vérifie que db_build classe chaque enregistrement de niveau zéro dans sa liste. */
TEST(db_build_groups_level_zero_records_by_kind) {
    // Given
    const char *text = "0 HEAD\n0 @I1@ INDI\n0 @I2@ INDI\n0 @F1@ FAM\n0 @S1@ SOUR\n"
                       "0 @R1@ REPO\n0 @M1@ OBJE\n0 @N1@ NOTE\n0 @N2@ NOTE\n0 TRLR\n";
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    fx_database(text, &db);

    // Then
    ASSERT_EQ_INT(2, db.lists[K_INDI].count);
    ASSERT_EQ_INT(1, db.lists[K_FAM].count);
    ASSERT_EQ_INT(1, db.lists[K_SRC].count);
    ASSERT_EQ_INT(1, db.lists[K_REPO].count);
    ASSERT_EQ_INT(1, db.lists[K_OBJ].count);
    ASSERT_EQ_INT(2, db.lists[K_NOTE].count);
}

/* Vérifie que les listes restent vides quand le fichier ne contient aucun enregistrement. */
TEST(db_build_produces_empty_lists_when_file_has_no_records) {
    // Given
    const char *text = "0 HEAD\n0 TRLR\n";
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    fx_database(text, &db);

    // Then
    for (int k = 0; k < K_COUNT; k++)
        ASSERT_EQ_INT(0, db.lists[k].count);
}

/* Vérifie que seules les citations SOUR imbriquées avec XREF sont comptées dans la vue virtuelle. */
TEST(db_build_collects_nested_source_citations_but_not_source_records_or_inline_text) {
    // Given
    Node *root = fx_parse("0 @I1@ INDI\n1 BIRT\n2 SOUR @X1@\n3 PAGE 10\n1 SOUR Inline source\n"
                          "0 @X1@ SOUR\n1 TITL Register\n");
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    db_build(&db, root);

    // Then
    ASSERT_EQ_INT(1, db.lists[K_CITA].count);
    ASSERT_TRUE(db.lists[K_CITA].items[0] == root->child->child->child);
}

/* Vérifie que les individus sont triés selon leur libellé Nom, Prénom. */
TEST(db_build_sorts_individuals_by_display_label) {
    // Given
    const char *text = "0 @I1@ INDI\n1 NAME Zoe/ADAMS/\n0 @I2@ INDI\n1 NAME Alan/BAKER/\n";
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    fx_database(text, &db);

    // Then
    ASSERT_EQ_STR("@I1@", db.lists[K_INDI].items[0]->xref);
    ASSERT_EQ_STR("@I2@", db.lists[K_INDI].items[1]->xref);
}

/* Vérifie que les familles sont triées selon le libellé de leurs conjoints. */
TEST(db_build_sorts_families_by_display_label) {
    // Given
    const char *text = "0 @I1@ INDI\n1 NAME Jean/ZULU/\n0 @I2@ INDI\n1 NAME Alice/ALPHA/\n"
                       "0 @I3@ INDI\n1 NAME Bob/BRAVO/\n"
                       "0 @F2@ FAM\n1 HUSB @I1@\n1 WIFE @I2@\n"
                       "0 @F1@ FAM\n1 HUSB @I3@\n1 WIFE @I2@\n";
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    fx_database(text, &db);

    // Then
    ASSERT_EQ_STR("@F1@", db.lists[K_FAM].items[0]->xref);
    ASSERT_EQ_STR("@F2@", db.lists[K_FAM].items[1]->xref);
}

/* Vérifie que sources, dépôts, médias, notes et citations suivent tous l'ordre de leur libellé. */
TEST(db_build_sorts_other_views_by_display_label) {
    // Given
    const char *text = "0 @I2@ INDI\n1 NAME Zoe/ZULU/\n1 SOUR @S2@\n"
                       "0 @I1@ INDI\n1 NAME Amy/ALPHA/\n1 SOUR @S1@\n"
                       "0 @S2@ SOUR\n1 TITL Zebra Source\n0 @S1@ SOUR\n1 TITL Alpha Source\n"
                       "0 @R2@ REPO\n1 NAME Zebra Repository\n0 @R1@ REPO\n1 NAME Alpha Repository\n"
                       "0 @M2@ OBJE\n1 TITL Zebra Media\n0 @M1@ OBJE\n1 TITL Alpha Media\n"
                       "0 @N2@ NOTE Zebra Note\n0 @N1@ NOTE Alpha Note\n";
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    fx_database(text, &db);

    // Then
    ASSERT_EQ_STR("@S1@", db.lists[K_SRC].items[0]->xref);
    ASSERT_EQ_STR("@R1@", db.lists[K_REPO].items[0]->xref);
    ASSERT_EQ_STR("@M1@", db.lists[K_OBJ].items[0]->xref);
    ASSERT_EQ_STR("@N1@", db.lists[K_NOTE].items[0]->xref);
    ASSERT_EQ_STR("@I1@", db.lists[K_CITA].items[0]->parent->xref);
}

/* Vérifie que db_find retrouve un enregistrement connu quel que soit son type. */
TEST(db_find_returns_the_record_whatever_its_kind) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n0 @S1@ SOUR\n0 @N1@ NOTE\n", &db);

    // When
    const Node *source = db_find(&db, "@S1@");

    // Then
    ASSERT_NOT_NULL(source);
    ASSERT_EQ_STR("SOUR", source->tag);
}

/* Vérifie qu'un identifiant absent de l'index XREF retourne NULL. */
TEST(db_find_returns_null_for_an_unknown_xref) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n", &db);

    // When
    const Node *node = db_find(&db, "@X9@");

    // Then
    ASSERT_NULL(node);
}

/* Vérifie qu'un XREF de type non pris en charge n'est pas retourné par db_find. */
TEST(db_find_returns_null_for_an_unsupported_record_type) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @U1@ _CUSTOM\n", &db);

    // When
    const Node *node = db_find(&db, "@U1@");

    // Then
    ASSERT_NULL(node);
}

/* Vérifie que db_build rejette les XREF dupliqués, même entre types différents. */
TEST(db_find_does_not_resolve_a_duplicate_xref_ambiguously) {
    // Given
    Node *root = fx_parse("0 @X1@ _CUSTOM\n0 @X1@ INDI\n");
    Database db TL_AUTOCLEAN(db_destroy);

    // When
    int built = db_build(&db, root);
    const Node *node = db_find(&db, "@X1@");

    // Then
    ASSERT_FALSE(built);
    ASSERT_NULL(node);
}

/* Vérifie que db_destroy libère et remet à zéro tous les index de la base. */
TEST(db_destroy_resets_the_database) {
    // Given
    Database db TL_AUTOCLEAN(db_destroy);
    fx_database("0 @I1@ INDI\n1 NAME Ada/LOVELACE/\n", &db);

    // When
    db_destroy(&db);

    // Then
    ASSERT_NULL(db.lists[K_INDI].items);
    ASSERT_EQ_INT(0, db.lists[K_INDI].count);
    ASSERT_NULL(db.xref_index);
    ASSERT_EQ_INT(0, (int)db.xref_count);
}

/* Enregistre les scénarios de construction et recherche de base puis retourne leur bilan. */
int main(void) {
    tl_suite("core/database");
    RUN_TEST(db_build_groups_level_zero_records_by_kind);
    RUN_TEST(db_build_produces_empty_lists_when_file_has_no_records);
    RUN_TEST(db_build_collects_nested_source_citations_but_not_source_records_or_inline_text);
    RUN_TEST(db_build_sorts_individuals_by_display_label);
    RUN_TEST(db_build_sorts_families_by_display_label);
    RUN_TEST(db_build_sorts_other_views_by_display_label);
    RUN_TEST(db_find_returns_the_record_whatever_its_kind);
    RUN_TEST(db_find_returns_null_for_an_unknown_xref);
    RUN_TEST(db_find_returns_null_for_an_unsupported_record_type);
    RUN_TEST(db_find_does_not_resolve_a_duplicate_xref_ambiguously);
    RUN_TEST(db_destroy_resets_the_database);
    return tl_finish();
}