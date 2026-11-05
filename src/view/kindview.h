#ifndef KINDVIEW_H
#define KINDVIEW_H

/* Correspondance entre les types d'enregistrement et leurs commandes/titres d'interface. */

#include "core/records.h"

/* Retourne l'identifiant i18n du titre associé à kind. */
int kind_title_id(Kind kind);
/* Retourne le type associé à une touche alphabétique, sans distinction de casse, ou -1. */
int kind_from_key(int ch);
/* Retourne le nom du slot de mémorisation de la sélection de kind, ou NULL si elle ne se mémorise pas
 * (citations). */
const char *kind_slot(Kind kind);
/* Retourne le type dont le slot est `slot`, ou -1. */
int kind_from_slot(const char *slot);

#endif
