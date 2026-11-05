/* Gère l'état de navigation, l'historique borné et la recherche dans les vues. */
#include "nav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include <wctype.h>

#include "view/label.h"

#define NAV_HISTORY_LIMIT 128

struct NavHistoryEntry {
    Kind kind;
    int sel, top, det_sel, det_top, focus_detail;
};

/* Empile une copie de l'état courant, en conservant au plus NAV_HISTORY_LIMIT entrées. */
static void history_push(Nav *nav);

/* Initialise Nav à la vue par défaut avec toutes les positions nulles. */
void nav_init(Nav *nav) { memset(nav, 0, sizeof *nav); }

/* Libère le tableau d'historique détenu par Nav et efface ses compteurs. */
void nav_destroy(Nav *nav) {
    free(nav->history);
    nav->history = NULL;
    nav->history_count = nav->history_cap = 0;
}

/* Retourne la taille de la liste sélectionnée dans nav. */
int nav_count(const Nav *nav, const Database *db) { return db->lists[nav->kind].count; }

/* Retourne l'élément actif, en bornant une sélection trop grande, ou NULL si la liste est vide. */
const Node *nav_current(const Nav *nav, const Database *db) {
    int n = nav_count(nav, db);
    return n ? db->lists[nav->kind].items[nav->sel < n ? nav->sel : n - 1] : NULL;
}

/* Change de liste en mémorisant sa position et en restaurant celle de la nouvelle vue. */
static void switch_kind(Nav *nav, Kind kind) {
    if (kind == nav->kind) return;
    nav->saved_sel[nav->kind] = nav->sel;
    nav->saved_top[nav->kind] = nav->top;
    nav->kind = kind;
    nav->sel = nav->saved_sel[kind];
    nav->top = nav->saved_top[kind];
    nav->det_sel = nav->det_top = 0;
}

/* Retourne l'enregistrement sélectionné dans la liste de kind, la position des listes inactives étant
 * sauvegardée. */
const Node *nav_record(const Nav *nav, const Database *db, Kind kind) {
    int n = db->lists[kind].count;
    if (!n) return NULL;
    int index = kind == nav->kind ? nav->sel : nav->saved_sel[kind];
    if (index >= n) index = n - 1;
    if (index < 0) index = 0;
    return db->lists[kind].items[index];
}

/* Retrouve xref dans la liste de kind et en fait la sélection de cette liste. */
int nav_restore_record(Nav *nav, const Database *db, Kind kind, const char *xref) {
    const Node *target = db_find(db, xref);
    const RecordList *list = &db->lists[kind];
    for (int i = 0; target && i < list->count; i++) {
        if (list->items[i] != target) continue;
        if (kind == nav->kind) {
            nav->sel = i;
            nav->det_sel = nav->det_top = 0;
        } else {
            nav->saved_sel[kind] = i;
            nav->saved_top[kind] = 0;
        }
        return 1;
    }
    return 0;
}

/* Affiche la liste de kind sans modifier l'historique. */
void nav_restore_view(Nav *nav, Kind kind) {
    switch_kind(nav, kind);
    nav->focus_detail = 0;
    nav->tree = 0;
    nav->menu = 0;
}

/* Ajoute l'état à l'historique si nécessaire, active kind et rend le focus à la liste. */
void nav_set_kind(Nav *nav, Kind kind) {
    if (kind != nav->kind) history_push(nav);
    switch_kind(nav, kind);
    nav->focus_detail = 0;
    nav->tree = 0;
    nav->menu = 0;
}

/* Bascule le focus clavier entre la liste et le détail. */
void nav_toggle_focus(Nav *nav) { nav->focus_detail = !nav->focus_detail; }

/* Sélectionne target dans la liste des individus ; l'historique n'est alimenté que si la sélection change. */
void nav_select_individual(Nav *nav, const Database *db, const Node *target) {
    const RecordList *list = &db->lists[K_INDI];
    for (int i = 0; i < list->count; i++) {
        if (list->items[i] != target) continue;
        if (i != nav->sel) history_push(nav);
        nav->sel = i;
        nav->det_sel = nav->det_top = 0;
        return;
    }
}

/* Déplace la sélection active, la borne à la vue courante et mémorise les déplacements de liste. */
void nav_move(Nav *nav, const Database *db, const Detail *d, int delta) {
    int *sel = nav->focus_detail ? &nav->det_sel : &nav->sel;
    int max = nav->focus_detail ? d->count : nav_count(nav, db);
    int next = *sel + delta;
    if (next >= max) next = max - 1;
    if (next < 0) next = 0;
    if (!nav->focus_detail && next != *sel) history_push(nav);
    *sel = next;
    if (!nav->focus_detail) nav->det_sel = nav->det_top = 0;
}

/* Ajuste top pour rendre sel visible dans une fenêtre de rows lignes. */
static void keep_visible(int sel, int *top, int rows) {
    if (sel < *top) *top = sel;
    if (sel >= *top + rows) *top = sel - rows + 1;
}

/* Borne la sélection du détail puis garde les sélections visibles dans les deux panneaux. */
void nav_scroll(Nav *nav, const Detail *d, int rows) {
    if (nav->det_sel >= d->count) nav->det_sel = d->count - 1;
    if (nav->det_sel < 0) nav->det_sel = 0;
    keep_visible(nav->sel, &nav->top, rows);
    keep_visible(nav->det_sel, &nav->det_top, rows);
}

/* Empile l'état de navigation, supprimant le plus ancien si la limite est atteinte. */
static void history_push(Nav *nav) {
    if (nav->history_count == NAV_HISTORY_LIMIT) {
        memmove(nav->history, nav->history + 1, (NAV_HISTORY_LIMIT - 1) * sizeof *nav->history);
        nav->history_count--;
    }
    if (nav->history_count == nav->history_cap) {
        size_t cap = nav->history_cap ? nav->history_cap * 2 : 8;
        if (cap > NAV_HISTORY_LIMIT) cap = NAV_HISTORY_LIMIT;
        NavHistoryEntry *grown = realloc(nav->history, cap * sizeof *nav->history);
        if (!grown) {
            perror("realloc");
            exit(1);
        }
        nav->history = grown;
        nav->history_cap = cap;
    }
    nav->history[nav->history_count++] =
        (NavHistoryEntry){nav->kind, nav->sel, nav->top, nav->det_sel, nav->det_top, nav->focus_detail};
}

/* Résout xref, ouvre la vue de sa cible et enregistre l'état d'origine si elle existe. */
static void follow(Nav *nav, const Database *db, const char *xref) {
    const Node *target = db_find(db, xref);
    int kind = target ? kind_from_tag(target->tag) : -1;
    if (kind < 0) return;
    const RecordList *list = &db->lists[kind];
    int index = -1;
    for (int i = 0; i < list->count; i++)
        if (list->items[i] == target) {
            index = i;
            break;
        }
    if (index < 0) return;

    history_push(nav);
    switch_kind(nav, (Kind)kind);
    nav->sel = index;
    nav->det_sel = nav->det_top = 0;
}

/* Active le lien de détail sélectionné ou, sinon, transfère le focus au détail. */
void nav_activate(Nav *nav, const Database *db, const Detail *d) {
    if (nav->focus_detail && nav->det_sel < d->count && d->lines[nav->det_sel].link[0] &&
        d->lines[nav->det_sel].link_kind == LINE_LINK_RECORD) {
        follow(nav, db, d->lines[nav->det_sel].link);
    } else {
        nav->focus_detail = 1;
    }
}

/* Restaure l'état le plus récent de l'historique s'il existe. */
void nav_back(Nav *nav) {
    if (!nav->history_count) return;
    NavHistoryEntry previous = nav->history[--nav->history_count];
    if (!nav->history_count) {
        free(nav->history);
        nav->history = NULL;
        nav->history_cap = 0;
    }
    nav->kind = previous.kind;
    nav->sel = previous.sel;
    nav->top = previous.top;
    nav->det_sel = previous.det_sel;
    nav->det_top = previous.det_top;
    nav->focus_detail = previous.focus_detail;
    nav->tree_sel = 1;
    if (nav->kind != K_INDI) nav->tree = 0;
    nav->menu = 0;
}

/* Recherche needle dans text sans casse, en Unicode si les deux chaînes sont convertibles. */
static int contains_case_insensitive(const char *text, const char *needle, size_t byte_len) {
    wchar_t wide_text[256], wide_needle[128];
    size_t text_len = mbstowcs(wide_text, text, sizeof wide_text / sizeof *wide_text);
    size_t needle_len = mbstowcs(wide_needle, needle, sizeof wide_needle / sizeof *wide_needle);
    if (text_len != (size_t)-1 && text_len < sizeof wide_text / sizeof *wide_text &&
        needle_len != (size_t)-1 && needle_len < sizeof wide_needle / sizeof *wide_needle) {
        for (size_t i = 0; i + needle_len <= text_len; i++) {
            size_t j = 0;
            while (j < needle_len && towlower(wide_text[i + j]) == towlower(wide_needle[j]))
                j++;
            if (j == needle_len) return 1;
        }
        return 0;
    }
    for (const char *q = text; *q; q++)
        if (!strncasecmp(q, needle, byte_len)) return 1;
    return 0;
}

/* Cherche needle dans les références REPO et les libellés des dépôts d'une source. */
static int matches_repository(const Database *db, const Node *source, const char *needle, size_t len) {
    if (!source) return 0;
    for (const Node *c = source->child; c; c = c->next) {
        if (strcmp(c->tag, "REPO") != 0) continue;
        if (contains_case_insensitive(c->value, needle, len)) return 1;
        const Node *repo = db_find(db, c->value);
        if (!repo) continue;
        char label[256];
        display_label(db, K_REPO, repo, label, sizeof label);
        if (contains_case_insensitive(label, needle, len)) return 1;
    }
    return 0;
}

/* Parcourt cycliquement la liste après la sélection courante et mémorise un résultat trouvé. */
static void search_from_current(Nav *nav, const Database *db, const char *needle) {
    size_t len = strlen(needle);
    int n = nav_count(nav, db);
    if (!len || !n) return;
    for (int k = 1; k <= n; k++) {
        int i = (nav->sel + k) % n;
        char name[256];
        const Node *record = db->lists[nav->kind].items[i];
        display_label(db, nav->kind, record, name, sizeof name);
        const char *xref = nav->kind == K_CITA ? record->value : record->xref;
        const Node *source = nav->kind == K_SRC    ? record
                             : nav->kind == K_CITA ? db_find(db, record->value)
                                                   : NULL;
        if (contains_case_insensitive(name, needle, len) ||
            (xref && contains_case_insensitive(xref, needle, len)) ||
            matches_repository(db, source, needle, len)) {
            if (i != nav->sel) history_push(nav);
            nav->sel = i;
            nav->det_sel = nav->det_top = 0;
            return;
        }
    }
}

/* Enregistre needle comme recherche courante puis sélectionne le prochain résultat. */
void nav_search(Nav *nav, const Database *db, const char *needle) {
    snprintf(nav->search, sizeof nav->search, "%s", needle);
    nav_search_next(nav, db);
}

/* Réutilise le terme de recherche enregistré et avance jusqu'au prochain résultat. */
void nav_search_next(Nav *nav, const Database *db) {
    if (*nav->search) search_from_current(nav, db, nav->search);
}
