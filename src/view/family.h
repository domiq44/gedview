#ifndef FAMILY_H
#define FAMILY_H

/* Informations familiales d'un individu pour l'affichage : résumé (libellé et dates) et enfants. */

#include <stddef.h>

#include "core/database.h"

/* Nombre maximal d'enfants listés par person_children. */
#define FAMILY_MAX_CHILDREN 64

/* Écrit dans label le libellé de indi et dans dates « ° naissance - + décès » (vide si aucune date). */
void person_summary(const Database *db, const Node *indi, char *label, size_t label_sz, char *dates,
                    size_t dates_sz);

/* Écrit dans out au plus max enfants de indi (FAMS puis CHIL, dans l'ordre du fichier) et retourne leur
 * nombre. */
int person_children(const Database *db, const Node *indi, const Node **out, int max);

#endif
