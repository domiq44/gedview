#ifndef RECORDS_H
#define RECORDS_H

/* Types d'enregistrements pris en charge et construction de leurs libellés métier. */

#include <stddef.h>

#include "gedcom.h"

/* Types d'enregistrements de niveau 0. Pour en ajouter un : l'enum et la table KINDS (records.c). */
typedef enum { K_INDI, K_FAM, K_SRC, K_REPO, K_OBJ, K_NOTE, K_CITA, K_COUNT } Kind;

typedef struct Database Database;

/* Formate dans out le libellé court de rec ; le résultat est limité à sz octets. */
void record_label(const Database *db, Kind kind, const Node *rec, char *out, size_t sz);

/* Remonte depuis une citation jusqu'à l'enregistrement GEDCOM de niveau zéro qui la contient. */
const Node *citation_owner(const Node *citation);

/* Convertit le format GEDCOM « prénom/nom/suffixe » en « nom, prénom suffixe », avec troncature à sz. */
void format_name(const char *raw, char *out, size_t sz);

/* Associe un tag de niveau zéro à Kind ; retourne -1 pour un tag non pris en charge. */
int kind_from_tag(const char *tag);

#endif
