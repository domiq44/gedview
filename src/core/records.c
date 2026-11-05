/* Associe les tags GEDCOM aux types supportés et calcule les libellés de ces types. */
#include "database.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*LabelFn)(const Database *, const Node *, char *, size_t);

typedef struct {
    const char *tag;
    LabelFn label;
} KindInfo;

/* Coupe une chaîne au premier retour à la ligne pour produire un libellé de liste. */
static void first_line(char *s) { s[strcspn(s, "\n")] = 0; }

/* Copie la valeur non vide du premier tag a/b trouvé, ou l'XREF, puis garde sa première ligne. */
static void label_from_subs(const Node *rec, const char *a, const char *b, char *out, size_t sz) {
    const Node *t = node_sub(rec, a);
    if (!t && b) t = node_sub(rec, b);
    snprintf(out, sz, "%s", t && *t->value ? t->value : rec->xref);
    first_line(out);
}

/* Retire les espaces aux extrémités d'un intervalle sans modifier la chaîne source. */
static void trim_range(const char **start, const char **end) {
    while (*start < *end && isspace((unsigned char)**start))
        (*start)++;
    /* Faux positif : l'analyseur suppose un strchr non nul sur "" (chemin impossible). */
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.ArraySubscript)
    while (*end > *start && isspace((unsigned char)(*end)[-1]))
        (*end)--;
}

/* Ajoute un intervalle rogné dans out en respectant sa capacité et le terminateur NUL. */
static void append_range(char *out, size_t sz, size_t *used, const char *start, const char *end) {
    trim_range(&start, &end);
    size_t len = (size_t)(end - start);
    size_t available = sz - 1 - *used;
    if (len > available) len = available;
    memcpy(out + *used, start, len);
    *used += len;
    out[*used] = 0;
}

/* Ajoute un séparateur dans la capacité restante de out. */
static void append_separator(char *out, size_t sz, size_t *used, const char *separator, size_t len) {
    size_t available = sz - 1 - *used;
    if (len > available) len = available;
    memcpy(out + *used, separator, len);
    *used += len;
    out[*used] = 0;
}

/* Convertit le nom GEDCOM délimité par des barres en nom lisible, avec troncature bornée. */
void format_name(const char *raw, char *out, size_t sz) {
    if (!sz) return;
    out[0] = 0;
    const char *first = strchr(raw, '/');
    const char *second = first ? strchr(first + 1, '/') : NULL;
    if (!first || !second) {
        size_t used = 0;
        const char *end = raw + strlen(raw);
        append_range(out, sz, &used, raw, end);
        return;
    }

    const char *given_start = raw, *given_end = first;
    const char *surname_start = first + 1, *surname_end = second;
    const char *suffix_start = second + 1, *suffix_end = raw + strlen(raw);
    trim_range(&given_start, &given_end);
    trim_range(&surname_start, &surname_end);
    trim_range(&suffix_start, &suffix_end);

    size_t used = 0;
    append_range(out, sz, &used, surname_start, surname_end);
    if (used && (given_start < given_end || suffix_start < suffix_end))
        append_separator(out, sz, &used, ", ", 2);
    append_range(out, sz, &used, given_start, given_end);
    if (given_start < given_end && suffix_start < suffix_end) append_separator(out, sz, &used, " ", 1);
    append_range(out, sz, &used, suffix_start, suffix_end);
}

/* Produit le libellé d'un individu à partir de son premier champ NAME. */
static void indi_label(const Database *db, const Node *n, char *out, size_t sz) {
    (void)db;
    const Node *nm = node_sub(n, "NAME");
    format_name(nm ? nm->value : "", out, sz);
}

/* Produit le libellé d'une famille en résolvant HUSB et WIFE, avec ? si absent. */
static void fam_label(const Database *db, const Node *f, char *out, size_t sz) {
    char a[256] = "", b[256] = "";
    const Node *h = node_sub(f, "HUSB"), *w = node_sub(f, "WIFE");
    const Node *hi = h ? db_find(db, h->value) : NULL, *wi = w ? db_find(db, w->value) : NULL;
    if (hi) indi_label(db, hi, a, sizeof a);
    if (wi) indi_label(db, wi, b, sizeof b);
    snprintf(out, sz, "%s & %s", *a ? a : "?", *b ? b : "?");
}

/* Préfère TITL puis ABBR pour identifier une source. */
static void src_label(const Database *db, const Node *n, char *out, size_t sz) {
    (void)db;
    label_from_subs(n, "TITL", "ABBR", out, sz);
}

/* Utilise NAME ou l'XREF pour identifier un dépôt. */
static void repo_label(const Database *db, const Node *n, char *out, size_t sz) {
    (void)db;
    label_from_subs(n, "NAME", NULL, out, sz);
}

/* Préfère TITL puis FILE pour identifier un objet multimédia. */
static void obj_label(const Database *db, const Node *n, char *out, size_t sz) {
    (void)db;
    label_from_subs(n, "TITL", "FILE", out, sz);
}

/* Utilise la première ligne du texte d'une note ou son XREF si elle est vide. */
static void note_label(const Database *db, const Node *n, char *out, size_t sz) {
    (void)db;
    snprintf(out, sz, "%s", *n->value ? n->value : n->xref);
    first_line(out);
}

/* Remonte les parents d'une citation jusqu'à l'enregistrement de niveau zéro. */
const Node *citation_owner(const Node *citation) {
    const Node *owner = citation;
    while (owner->parent && owner->level > 0)
        owner = owner->parent;
    return owner;
}

/* Compose le libellé d'une citation avec son propriétaire, son contexte et sa source. */
static void citation_label(const Database *db, const Node *citation, char *out, size_t sz) {
    char source[256] = "", owner_label[256] = "";
    const Node *source_record = db_find(db, citation->value);
    int source_kind = source_record ? kind_from_tag(source_record->tag) : -1;
    if (source_kind >= 0)
        record_label(db, (Kind)source_kind, source_record, source, sizeof source);
    else
        snprintf(source, sizeof source, "%s", citation->value);

    const Node *owner = citation_owner(citation);
    int owner_kind = kind_from_tag(owner->tag);
    if (owner_kind >= 0) record_label(db, (Kind)owner_kind, owner, owner_label, sizeof owner_label);
    const char *context = citation->parent != owner ? citation->parent->tag : NULL;
    if (context)
        snprintf(out, sz, "%s - %s - %s", *owner_label ? owner_label : owner->tag, context,
                 *source ? source : citation->value);
    else
        snprintf(out, sz, "%s - %s", *owner_label ? owner_label : owner->tag,
                 *source ? source : citation->value);
}

static const KindInfo KINDS[K_CITA] = {
    [K_INDI] = {"INDI", indi_label}, [K_FAM] = {"FAM", fam_label},  [K_SRC] = {"SOUR", src_label},
    [K_REPO] = {"REPO", repo_label}, [K_OBJ] = {"OBJE", obj_label}, [K_NOTE] = {"NOTE", note_label},
};

/* Sélectionne le formateur adapté à kind et produit son libellé borné. */
void record_label(const Database *db, Kind kind, const Node *rec, char *out, size_t sz) {
    if (kind == K_CITA) {
        citation_label(db, rec, out, sz);
        return;
    }
    if (kind < 0 || kind >= K_CITA) {
        if (sz) out[0] = 0;
        return;
    }
    KINDS[kind].label(db, rec, out, sz);
}

/* Retourne le type associé à un tag d'enregistrement de niveau zéro, ou -1. */
int kind_from_tag(const char *tag) {
    for (int k = 0; k < K_CITA; k++)
        if (!strcmp(tag, KINDS[k].tag)) return k;
    return -1;
}
