#ifndef LABEL_H
#define LABEL_H

/* Adaptation des libellés métier à l'affichage, avec textes de remplacement traduits. */

#include "core/records.h"

/* Écrit le libellé de rec dans out, ou S_NONAME si le libellé métier est vide. */
void display_label(const Database *db, Kind kind, const Node *rec, char *out, size_t sz);

/* Formate raw comme un nom GEDCOM lisible, ou utilise S_NONAME si le résultat est vide. */
void display_name(const char *raw, char *out, size_t sz);

#endif
