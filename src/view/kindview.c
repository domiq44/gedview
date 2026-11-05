/* Décrit les raccourcis clavier et les titres associés aux listes de la vue. */
#include "kindview.h"

#include <ctype.h>
#include <string.h>

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

/* Les citations n'ont pas de XREF propre : leur s\u00e9lection n'est pas m\u00e9moris\u00e9e. */
static const char *const SLOTS[K_COUNT] = {
    [K_INDI] = "INDI", [K_FAM] = "FAM",  [K_SRC] = "SOUR",
    [K_REPO] = "REPO", [K_OBJ] = "OBJE", [K_NOTE] = "NOTE",
};

/* Retourne le slot de kind, ou NULL pour les citations. */
const char *kind_slot(Kind kind) { return SLOTS[kind]; }

/* Cherche le type dont le slot est identique \u00e0 slot. */
int kind_from_slot(const char *slot) {
    for (int k = 0; k < K_COUNT; k++)
        if (SLOTS[k] && !strcmp(SLOTS[k], slot)) return k;
    return -1;
}
