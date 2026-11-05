/* Relie l'\u00e9tat de navigation au fichier d'\u00e9tat : une valeur par liste plus la vue affich\u00e9e. */
#include "session.h"

#include "core/state.h"
#include "view/kindview.h"

#define VIEW_SLOT "VIEW"

/* Relit la s\u00e9lection de chaque liste, puis la vue ; les valeurs absentes ou obsol\u00e8tes sont
 * ignor\u00e9es. */
void session_restore(Nav *nav, const Database *db, const char *gedcom_path) {
    char value[128];
    for (int k = 0; k < K_COUNT; k++) {
        const char *slot = kind_slot((Kind)k);
        if (slot && state_load(gedcom_path, slot, value, sizeof value))
            nav_restore_record(nav, db, (Kind)k, value);
    }
    int view = state_load(gedcom_path, VIEW_SLOT, value, sizeof value) ? kind_from_slot(value) : -1;
    if (view >= 0) nav_restore_view(nav, (Kind)view);
}

/* Enregistre les s\u00e9lections puis la vue ; les citations, non m\u00e9morisables, sont remplac\u00e9es par
 * les individus. */
void session_save(const Nav *nav, const Database *db, const char *gedcom_path) {
    StateEntry entries[K_COUNT + 1];
    int count = 0;
    for (int k = 0; k < K_COUNT; k++) {
        const char *slot = kind_slot((Kind)k);
        const Node *record = slot ? nav_record(nav, db, (Kind)k) : NULL;
        if (record && record->xref) entries[count++] = (StateEntry){slot, record->xref};
    }
    const char *view = kind_slot(nav->kind);
    entries[count++] = (StateEntry){VIEW_SLOT, view ? view : kind_slot(K_INDI)};
    state_save(gedcom_path, entries, count);
}
