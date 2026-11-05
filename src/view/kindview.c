/* Décrit les raccourcis clavier et les titres associés aux listes de la vue. */
#include "kindview.h"

#include <ctype.h>

#include "i18n/i18n.h"

static const struct {
    int key;
    int title_id;
} VIEWS[K_COUNT] = {
    [K_INDI] = {'i', S_T_INDI}, [K_FAM] = {'f', S_T_FAM}, [K_SRC] = {'s', S_T_SRC},
    [K_REPO] = {'r', S_T_REPO}, [K_OBJ] = {'m', S_T_OBJ}, [K_NOTE] = {'n', S_T_NOTE},
    [K_CITA] = {'c', S_T_CITA},
};

/* Retourne la clé i18n du titre de la liste kind. */
int kind_title_id(Kind kind) { return VIEWS[kind].title_id; }

/* Traduit une touche ASCII en vue, sans tenir compte de sa casse. */
int kind_from_key(int ch) {
    for (int k = 0; k < K_COUNT; k++)
        if (ch >= 0 && ch < 256 && tolower(ch) == VIEWS[k].key) return k;
    return -1;
}
