/* Résume un individu et liste ses enfants à partir des liens FAMS, CHIL, BIRT et DEAT. */
#include "family.h"

#include <stdio.h>
#include <string.h>

#include "view/label.h"

/* Écrit dans out « ° naissance - + décès » à partir des dates BIRT et DEAT de indi. */
static void format_dates(const Node *indi, char *out, size_t sz) {
    const Node *birth = node_sub(indi, "BIRT"), *death = node_sub(indi, "DEAT");
    const Node *born = birth ? node_sub(birth, "DATE") : NULL, *died = death ? node_sub(death, "DATE") : NULL;
    const char *b = born ? born->value : "", *d = died ? died->value : "";
    if (*b && *d)
        snprintf(out, sz, "° %s - + %s", b, d);
    else if (*b)
        snprintf(out, sz, "° %s", b);
    else if (*d)
        snprintf(out, sz, "+ %s", d);
    else
        out[0] = 0;
}

/* Renseigne le libellé puis les dates de indi. */
void person_summary(const Database *db, const Node *indi, char *label, size_t label_sz, char *dates,
                    size_t dates_sz) {
    display_label(db, K_INDI, indi, label, label_sz);
    format_dates(indi, dates, dates_sz);
}

/* Ajoute à out, après count éléments, les individus référencés par les CHIL de fam ; retourne le nouveau
 * total. */
static int collect_children(const Database *db, const Node *fam, const Node **out, int count, int max) {
    for (const Node *c = fam ? fam->child : NULL; c && count < max; c = c->next) {
        const Node *child = !strcmp(c->tag, "CHIL") ? db_find(db, c->value) : NULL;
        if (child && !strcmp(child->tag, "INDI")) out[count++] = child;
    }
    return count;
}

/* Parcourt les familles de conjoint de indi et collecte leurs enfants individuels. */
int person_children(const Database *db, const Node *indi, const Node **out, int max) {
    int count = 0;
    for (const Node *s = indi ? indi->child : NULL; s; s = s->next)
        if (!strcmp(s->tag, "FAMS")) count = collect_children(db, db_find(db, s->value), out, count, max);
    return count;
}

/* Collecte les enfants de la famille du premier FAMC de indi, indi compris. */
int person_siblings(const Database *db, const Node *indi, const Node **out, int max) {
    const Node *famc = indi ? node_sub(indi, "FAMC") : NULL;
    return collect_children(db, famc ? db_find(db, famc->value) : NULL, out, 0, max);
}

/* Retourne l'individu référencé par le sous-nœud tag de fam, ou NULL s'il est absent ou invalide. */
const Node *family_member(const Database *db, const Node *fam, const char *tag) {
    const Node *ref = fam ? node_sub(fam, tag) : NULL;
    const Node *indi = ref ? db_find(db, ref->value) : NULL;
    return indi && !strcmp(indi->tag, "INDI") ? indi : NULL;
}

/* Ajoute person à out s'il existe, n'est pas `excluded`, n'y figure pas déjà et qu'il reste de la place. */
static int add_relative(Relative *out, int count, int max, const Node *excluded, const Node *person,
                        Relation relation) {
    if (!person || person == excluded || count >= max) return count;
    for (int i = 0; i < count; i++)
        if (out[i].indi == person) return count;
    out[count++] = (Relative){person, relation};
    return count;
}

/* Ajoute les enfants de fam avec la relation donnée. */
static int add_family_children(const Database *db, const Node *fam, Relative *out, int count, int max,
                               const Node *self, Relation relation) {
    const Node *children[FAMILY_MAX_CHILDREN];
    int n = collect_children(db, fam, children, 0, FAMILY_MAX_CHILDREN);
    for (int i = 0; i < n; i++)
        count = add_relative(out, count, max, self, children[i], relation);
    return count;
}

/* Ajoute les enfants des autres familles de conjoint de parent, hors la famille parentale `own`. */
static int add_half_siblings(const Database *db, const Node *parent, const Node *own, Relative *out,
                             int count, int max, const Node *self) {
    for (const Node *s = parent ? parent->child : NULL; s; s = s->next) {
        const Node *fam = !strcmp(s->tag, "FAMS") ? db_find(db, s->value) : NULL;
        if (fam && fam != own) count = add_family_children(db, fam, out, count, max, self, REL_HALF_SIBLING);
    }
    return count;
}

/* Rassemble parents, frères et sœurs, demi-frères et demi-sœurs, conjoints puis enfants, dans cet ordre. */
int person_relatives(const Database *db, const Node *indi, Relative *out, int max) {
    const Node *famc = indi ? node_sub(indi, "FAMC") : NULL;
    const Node *own = famc ? db_find(db, famc->value) : NULL;
    const Node *father = family_member(db, own, "HUSB"), *mother = family_member(db, own, "WIFE");
    int count = 0;
    count = add_relative(out, count, max, indi, father, REL_PARENT);
    count = add_relative(out, count, max, indi, mother, REL_PARENT);
    count = add_family_children(db, own, out, count, max, indi, REL_SIBLING);
    count = add_half_siblings(db, father, own, out, count, max, indi);
    count = add_half_siblings(db, mother, own, out, count, max, indi);
    for (const Node *s = indi ? indi->child : NULL; s; s = s->next) {
        const Node *fam = !strcmp(s->tag, "FAMS") ? db_find(db, s->value) : NULL;
        count = add_relative(out, count, max, indi, family_member(db, fam, "HUSB"), REL_SPOUSE);
        count = add_relative(out, count, max, indi, family_member(db, fam, "WIFE"), REL_SPOUSE);
    }
    for (const Node *s = indi ? indi->child : NULL; s; s = s->next) {
        const Node *fam = !strcmp(s->tag, "FAMS") ? db_find(db, s->value) : NULL;
        count = add_family_children(db, fam, out, count, max, indi, REL_CHILD);
    }
    return count;
}
