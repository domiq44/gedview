#ifndef DATABASE_H
#define DATABASE_H

/* Index des enregistrements GEDCOM de niveau zéro et des citations imbriquées. */

#include <stddef.h>

#include "records.h"

typedef struct {
    Node **items; /* pointeurs empruntés à l'arbre GEDCOM, triés par libellé */
    int count;    /* nombre d'éléments valides dans items */
} RecordList;

typedef struct XrefEntry XrefEntry;

struct Database {
    RecordList lists[K_COUNT];
    XrefEntry *xref_index;
    size_t xref_count;
};

/* Construit les listes triées et l'index XREF en empruntant les nœuds à root, qui doit rester vivant.
 * `db` doit être neuf ou libéré par db_destroy ; retourne 0 si deux enregistrements partagent un XREF. */
int db_build(Database *db, Node *root);

/* Libère les tableaux possédés par db sans toucher aux nœuds GEDCOM, puis remet db à zéro. */
void db_destroy(Database *db);

/* Retourne un enregistrement de niveau zéro portant xref, en O(log n), ou NULL si absent/non pris en charge.
 */
Node *db_find(const Database *db, const char *xref);

#endif