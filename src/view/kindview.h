#ifndef KINDVIEW_H
#define KINDVIEW_H

/* Correspondance entre les types d'enregistrement et leurs commandes/titres d'interface. */

#include "core/records.h"

/* Retourne l'identifiant i18n du titre associé à kind. */
int kind_title_id(Kind kind);
/* Retourne le type associé à une touche alphabétique, sans distinction de casse, ou -1. */
int kind_from_key(int ch);

#endif
