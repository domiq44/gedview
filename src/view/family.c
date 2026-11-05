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

/* Parcourt les familles de conjoint de indi et collecte leurs enfants individuels. */
int person_children(const Database *db, const Node *indi, const Node **out, int max) {
    int count = 0;
    for (const Node *s = indi ? indi->child : NULL; s; s = s->next) {
        const Node *fam = !strcmp(s->tag, "FAMS") ? db_find(db, s->value) : NULL;
        for (const Node *c = fam ? fam->child : NULL; c && count < max; c = c->next) {
            const Node *child = !strcmp(c->tag, "CHIL") ? db_find(db, c->value) : NULL;
            if (child && !strcmp(child->tag, "INDI")) out[count++] = child;
        }
    }
    return count;
}
