/* Fournit les valeurs de remplacement et le formatage final des libellés de vue. */
#include "label.h"

#include <stdio.h>

#include "i18n/i18n.h"

/* Formate un nom GEDCOM et remplace un résultat vide par le texte NONAME traduit. */
void display_name(const char *raw, char *out, size_t sz) {
    format_name(raw, out, sz);
    if (!*out) snprintf(out, sz, "%s", T(S_NONAME));
}

/* Formate le libellé métier puis fournit le texte NONAME si celui-ci est vide. */
void display_label(const Database *db, Kind kind, const Node *rec, char *out, size_t sz) {
    record_label(db, kind, rec, out, sz);
    if (!*out) snprintf(out, sz, "%s", T(S_NONAME));
}
