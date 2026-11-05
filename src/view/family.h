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

/* Écrit dans out au plus max enfants de la famille du premier FAMC de indi (indi compris, ordre du fichier)
 * et retourne leur nombre ; 0 si indi n'a pas de famille parentale connue. */
int person_siblings(const Database *db, const Node *indi, const Node **out, int max);

/* Nombre maximal de proches listés par person_relatives. */
#define FAMILY_MAX_RELATIVES 128

/* Liens de parenté décrits par person_relatives, dans l'ordre de la liste. */
typedef enum { REL_PARENT, REL_SIBLING, REL_HALF_SIBLING, REL_SPOUSE, REL_CHILD, REL_COUNT } Relation;

typedef struct {
    const Node *indi;
    Relation relation;
} Relative;

/* Retourne l'individu référencé par le sous-nœud tag (HUSB, WIFE...) de fam, ou NULL s'il est absent ou
 * invalide. */
const Node *family_member(const Database *db, const Node *fam, const char *tag);

/* Écrit dans out au plus max proches de indi, regroupés par relation (parents, frères et sœurs de la famille
 * parentale, demi-frères et demi-sœurs, conjoints, enfants) ; chaque personne n'apparaît qu'une fois, sous sa
 * première relation, et indi lui-même est exclu. Retourne leur nombre. */
int person_relatives(const Database *db, const Node *indi, Relative *out, int max);

#endif
