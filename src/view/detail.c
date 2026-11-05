/* Aplatit un enregistrement GEDCOM en lignes lisibles et marque les références activables. */
#include "detail.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "i18n/i18n.h"
#include "core/database.h"
#include "view/label.h"

typedef struct {
    Detail *d;
    const Database *db;
} Ctx;

/* Réserve une ligne dans le tableau de Detail et initialise ses champs de lien. */
static Line *new_line(Detail *d) {
    if (d->count == d->cap) {
        d->cap = d->cap ? d->cap * 2 : 128;
        Line *grown = realloc(d->lines, d->cap * sizeof *d->lines);
        if (!grown) {
            perror("realloc");
            exit(1);
        }
        d->lines = grown;
    }
    Line *l = &d->lines[d->count++];
    l->text[0] = 0;
    l->link[0] = 0;
    l->link_kind = LINE_LINK_RECORD;
    l->link_start = 0;
    l->link_length = 0;
    return l;
}

/* Ajoute au texte de ligne la portion qui tient encore dans le tampon fixe. */
static void append_text(Line *line, size_t *used, const char *text, size_t len) {
    size_t available = sizeof line->text - 1 - *used;
    if (len > available) len = available;
    memcpy(line->text + *used, text, len);
    *used += len;
    line->text[*used] = 0;
}

/* Repère la première URL HTTP(S) d'un fragment NOTE/TEXT et la marque comme lien. */
static void set_note_url(Line *line, const char *text, size_t len, size_t text_start) {
    for (size_t i = 0; i < len; i++) {
        size_t scheme_len = !strncasecmp(text + i, "https://", 8)  ? 8
                            : !strncasecmp(text + i, "http://", 7) ? 7
                                                                   : 0;
        if (!scheme_len) continue;

        size_t end = i + scheme_len;
        while (end < len && text[end] > 0x20 && text[end] != '<' && text[end] != '>' && text[end] != '"' &&
               text[end] != '\'')
            end++;
        while (end > i + scheme_len && strchr(".,;:!?)]}", text[end - 1]))
            end--;
        size_t url_len = end - i;
        if (url_len && url_len < sizeof line->link) {
            memcpy(line->link, text + i, url_len);
            line->link[url_len] = 0;
            line->link_kind = LINE_LINK_URL;
            line->link_start = text_start + i;
            line->link_length = url_len;
        }
        return;
    }
}

/* Décompose les retours à la ligne et ajoute un libellé, une valeur et un lien éventuel. */
static void add_text(Ctx *c, int depth, const char *tag, const char *value, const char *link) {
    const char *p = value;
    int first = 1;
    do {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        Line *l = new_line(c->d);
        size_t used = 0;
        size_t indent = (size_t)depth * 2;
        if (indent > sizeof l->text - 1) indent = sizeof l->text - 1;
        memset(l->text, ' ', indent);
        used = indent;
        l->text[used] = 0;
        if (first) {
            const char *label = tag_label(tag);
            append_text(l, &used, label, strlen(label));
            if (len) append_text(l, &used, ": ", 2);
        } else {
            append_text(l, &used, "  ", 2);
        }
        size_t text_start = used;
        append_text(l, &used, p, len);
        size_t displayed_len = used - text_start;
        if (first && link) {
            size_t link_len = strlen(link);
            if (link_len < sizeof l->link) {
                memcpy(l->link, link, link_len + 1);
                l->link_start = text_start;
                l->link_length = displayed_len;
                if (!strcmp(tag, "FILE")) l->link_kind = LINE_LINK_MEDIA;
            }
        } else if (!link && (!strcmp(tag, "NOTE") || !strcmp(tag, "TEXT"))) {
            set_note_url(l, p, displayed_len, text_start);
        }
        first = 0;
        p = nl ? nl + 1 : NULL;
    } while (p);
}

/* Résout une référence, affiche son libellé et ne rend le XREF activable que si la cible existe. */
static void render_ref(Ctx *c, int depth, const Node *n) {
    const Node *t = db_find(c->db, n->value);
    int kind = t ? kind_from_tag(t->tag) : -1;
    if (kind < 0) {
        add_text(c, depth, n->tag, n->value, NULL);
    } else if (kind == K_FAM) {
        add_text(c, depth, n->tag, n->value, n->value);
    } else {
        char name[256];
        display_label(c->db, (Kind)kind, t, name, sizeof name);
        add_text(c, depth, n->tag, name, n->value);
    }
}

/* Reconnaît les liens de famille qui identifient un conjoint ou un enfant. */
static int is_family_member_tag(const char *tag) {
    return !strcmp(tag, "HUSB") || !strcmp(tag, "WIFE") || !strcmp(tag, "CHIL");
}

static void render_tree(Ctx *c, const Node *n, int depth);
/* Développe les membres et l'événement de mariage d'une famille liée par FAMC/FAMS. */
static void render_family_members(Ctx *c, const Node *fam, int depth) {
    for (const Node *m = fam->child; m; m = m->next) {
        if (gedcom_is_ref(m->value) && is_family_member_tag(m->tag)) {
            render_ref(c, depth, m);
        } else if (!strcmp(m->tag, "MARR")) {
            add_text(c, depth, m->tag, m->value, NULL);
            render_tree(c, m, depth + 1);
        }
    }
}

/* Parcourt récursivement les enfants en formatant noms, valeurs, références et fichiers. */
static void render_tree(Ctx *c, const Node *n, int depth) {
    for (const Node *ch = n->child; ch; ch = ch->next) {
        if (!strcmp(ch->tag, "NAME")) {
            char name[256];
            display_name(ch->value, name, sizeof name);
            add_text(c, depth, ch->tag, name, NULL);
        } else if (!strcmp(ch->tag, "FILE")) {
            add_text(c, depth, ch->tag, ch->value, ch->value);
        } else if (gedcom_is_ref(ch->value)) {
            render_ref(c, depth, ch);
            const Node *t = db_find(c->db, ch->value);
            if (t && !strcmp(t->tag, "FAM") && (!strcmp(ch->tag, "FAMC") || !strcmp(ch->tag, "FAMS")))
                render_family_members(c, t, depth + 1);
        } else {
            add_text(c, depth, ch->tag, ch->value, NULL);
        }
        render_tree(c, ch, depth + 1);
    }
}

/* Rend une citation avec des liens vers la source et l'enregistrement qui la contient. */
static void render_citation(Ctx *c, const Node *citation) {
    const Node *source = db_find(c->db, citation->value);
    int source_kind = source ? kind_from_tag(source->tag) : -1;
    char source_name[256];
    if (source_kind >= 0)
        record_label(c->db, (Kind)source_kind, source, source_name, sizeof source_name);
    else
        snprintf(source_name, sizeof source_name, "%s", citation->value);
    add_text(c, 0, "SOUR", source_name, source_kind >= 0 ? citation->value : NULL);

    const Node *owner = citation_owner(citation);
    int owner_kind = kind_from_tag(owner->tag);
    char owner_name[256];
    if (owner_kind >= 0)
        record_label(c->db, (Kind)owner_kind, owner, owner_name, sizeof owner_name);
    else
        snprintf(owner_name, sizeof owner_name, "%s", owner->tag);
    add_text(c, 0, "ORIGIN", owner_name, owner->xref[0] ? owner->xref : NULL);

    render_tree(c, citation, 0);
}

/* Remplace les lignes de d par l'en-tête et le rendu complet de rec. */
void detail_build(Detail *d, const Database *db, Kind kind, const Node *rec) {
    Ctx c = {d, db};
    d->count = 0;
    char name[256];
    display_label(db, kind, rec, name, sizeof name);
    Line *l = new_line(d);
    snprintf(l->text, sizeof l->text, "%s  %s", name, kind == K_CITA ? rec->value : rec->xref);
    new_line(d);
    if (kind == K_NOTE) add_text(&c, 0, "TEXT", rec->value, NULL);
    if (kind == K_CITA) {
        render_citation(&c, rec);
        return;
    }
    render_tree(&c, rec, 0);
}

/* Libère le tampon dynamique de lignes et réinitialise d. */
void detail_destroy(Detail *d) {
    if (!d) return;
    free(d->lines);
    memset(d, 0, sizeof *d);
}
