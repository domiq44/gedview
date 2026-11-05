#include "i18n/i18n.h"
#include "support/testlib.h"
#include "view/kindview.h"

/* Vérifie que chaque touche de vue est associée au bon type d'enregistrement. */
TEST(kind_from_key_maps_each_view_key_to_its_kind) {
    // Given
    const int keys[] = {'i', 'f', 's', 'r', 'm', 'n', 'c'};
    const Kind kinds[] = {K_INDI, K_FAM, K_SRC, K_REPO, K_OBJ, K_NOTE, K_CITA};

    for (int i = 0; i < 7; i++) {
        // When
        int kind = kind_from_key(keys[i]);

        // Then
        ASSERT_EQ_INT(kinds[i], kind);
    }
}

/* Vérifie que les touches de vue majuscules et minuscules sont équivalentes. */
TEST(kind_from_key_is_case_insensitive) {
    // Given
    const int key = 'F';

    // When
    int kind = kind_from_key(key);

    // Then
    ASSERT_EQ_INT(K_FAM, kind);
}

/* Vérifie qu'une touche sans vue associée produit le code -1. */
TEST(kind_from_key_returns_minus_one_for_keys_without_a_view) {
    // Given
    const int keys[] = {'x', 'q', 'v', ' ', 0};

    for (int i = 0; i < 5; i++) {
        // When
        int kind = kind_from_key(keys[i]);

        // Then
        ASSERT_EQ_INT(-1, kind);
    }
}

/* Vérifie que les codes spéciaux ncurses hors ASCII ne sélectionnent aucune vue. */
TEST(kind_from_key_returns_minus_one_for_special_keys_beyond_ascii) {
    // Given
    const int key = 0x103; /* KEY_UP et autres codes ncurses */

    // When
    int kind = kind_from_key(key);

    // Then
    ASSERT_EQ_INT(-1, kind);
}

/* Vérifie qu'un code de touche négatif est rejeté sans accès hors limites. */
TEST(kind_from_key_returns_minus_one_for_a_negative_key) {
    // Given
    const int key = -1; /* ERR */

    // When
    int kind = kind_from_key(key);

    // Then
    ASSERT_EQ_INT(-1, kind);
}

/* Vérifie que chaque type de liste possède son propre identifiant de titre. */
TEST(kind_title_id_gives_each_kind_its_own_title) {
    // Given
    const Kind kinds[] = {K_INDI, K_FAM, K_SRC, K_REPO, K_OBJ, K_NOTE, K_CITA};
    const int titles[] = {S_T_INDI, S_T_FAM, S_T_SRC, S_T_REPO, S_T_OBJ, S_T_NOTE, S_T_CITA};

    for (int i = 0; i < 7; i++) {
        // When
        int id = kind_title_id(kinds[i]);

        // Then
        ASSERT_EQ_INT(titles[i], id);
    }
}

/* Enregistre les scénarios de correspondance des vues et retourne leur bilan. */
int main(void) {
    tl_suite("view/kindview");
    RUN_TEST(kind_from_key_maps_each_view_key_to_its_kind);
    RUN_TEST(kind_from_key_is_case_insensitive);
    RUN_TEST(kind_from_key_returns_minus_one_for_keys_without_a_view);
    RUN_TEST(kind_from_key_returns_minus_one_for_special_keys_beyond_ascii);
    RUN_TEST(kind_from_key_returns_minus_one_for_a_negative_key);
    RUN_TEST(kind_title_id_gives_each_kind_its_own_title);
    return tl_finish();
}
