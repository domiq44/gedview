/* Indexe les enregistrements pris en charge et construit la vue virtuelle des citations. */
#include "database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct XrefEntry {
    const char *xref;
    Node *record;
};

typedef struct {
    Node *record;
    char label[256];
} LabelEntry;

/* Compare deux libellés selon la collation active pour le tri des listes. */
static int cmp_label(const void *a, const void *b) {
    const LabelEntry *x = a, *y = b;
    return strcoll(x->label, y->label);
}

/* Trie en place les pointeurs d'une liste en conservant les nœuds de l'arbre. */
static void sort_by_label(Database *db, Kind kind) {
    RecordList *list = &db->lists[kind];
    if (list->count < 2) return;

    LabelEntry *entries = calloc((size_t)list->count, sizeof *entries);
    if (!entries) {
        perror("calloc");
        exit(1);
    }
    for (int i = 0; i < list->count; i++) {
        entries[i].record = list->items[i];
        record_label(db, kind, entries[i].record, entries[i].label, sizeof entries[i].label);
    }
    qsort(entries, (size_t)list->count, sizeof *entries, cmp_label);
    for (int i = 0; i < list->count; i++)
        list->items[i] = entries[i].record;
    free(entries);
}

/* Compare les entrées de l'index selon leur XREF pour permettre la recherche binaire. */
static int cmp_xref(const void *a, const void *b) {
    const XrefEntry *x = a, *y = b;
    return strcmp(x->xref, y->xref);
}

/* Compte récursivement les SOUR imbriqués qui contiennent une référence XREF. */
static int count_citations(const Node *node) {
    int count = 0;
    for (const Node *child = node->child; child; child = child->next) {
        if (!strcmp(child->tag, "SOUR") && gedcom_is_ref(child->value)) count++;
        count += count_citations(child);
    }
    return count;
}

/* Ajoute les SOUR référencés de node et de ses descendants au tableau de citations. */
static void collect_citations(Node *node, Node **items, int *count) {
    for (Node *child = node->child; child; child = child->next) {
        if (!strcmp(child->tag, "SOUR") && gedcom_is_ref(child->value)) items[(*count)++] = child;
        collect_citations(child, items, count);
    }
}

/* Libère les index détenus par db, sans libérer les nœuds GEDCOM empruntés. */
void db_destroy(Database *db) {
    if (!db) return;
    for (int k = 0; k < K_COUNT; k++)
        free((void *)db->lists[k].items);
    free(db->xref_index);
    memset(db, 0, sizeof *db);
}

/* Construit les listes, collecte les citations, vérifie les XREF puis trie les libellés. */
int db_build(Database *db, Node *root) {
    memset(db, 0, sizeof *db);
    for (const Node *n = root->child; n; n = n->next) {
        int k = kind_from_tag(n->tag);
        if (k >= 0) db->lists[k].count++;
        db->lists[K_CITA].count += count_citations(n);
    }
    for (int k = 0; k < K_COUNT; k++) {
        db->lists[k].items = (Node **)calloc(db->lists[k].count + 1, sizeof(Node *));
        if (!db->lists[k].items) {
            perror("calloc");
            exit(1);
        }
        db->lists[k].count = 0;
    }
    for (Node *n = root->child; n; n = n->next) {
        int k = kind_from_tag(n->tag);
        if (k >= 0) db->lists[k].items[db->lists[k].count++] = n;
        collect_citations(n, db->lists[K_CITA].items, &db->lists[K_CITA].count);
    }
    for (const Node *n = root->child; n; n = n->next)
        if (n->xref[0]) db->xref_count++;
    if (db->xref_count) {
        db->xref_index = calloc(db->xref_count, sizeof *db->xref_index);
        if (!db->xref_index) {
            perror("calloc");
            exit(1);
        }
        size_t next = 0;
        for (Node *n = root->child; n; n = n->next)
            if (n->xref[0]) db->xref_index[next++] = (XrefEntry){n->xref, n};
        qsort(db->xref_index, db->xref_count, sizeof *db->xref_index, cmp_xref);
        for (size_t i = 1; i < db->xref_count; i++)
            if (!strcmp(db->xref_index[i - 1].xref, db->xref_index[i].xref)) {
                db_destroy(db);
                return 0;
            }
    }
    for (int kind = 0; kind < K_COUNT; kind++)
        sort_by_label(db, (Kind)kind);
    return 1;
}

/* Recherche un XREF dans l'index trié et masque les nœuds qui ne sont pas des enregistrements supportés. */
Node *db_find(const Database *db, const char *xref) {
    if (!xref || !*xref) return NULL;
    size_t low = 0, high = db->xref_count;
    while (low < high) {
        size_t mid = low + ((high - low) / 2);
        int cmp = strcmp(db->xref_index[mid].xref, xref);
        if (cmp < 0)
            low = mid + 1;
        else
            high = mid;
    }
    if (low < db->xref_count && !strcmp(db->xref_index[low].xref, xref)) {
        Node *record = db->xref_index[low].record;
        return kind_from_tag(record->tag) >= 0 ? record : NULL;
    }
    return NULL;
}