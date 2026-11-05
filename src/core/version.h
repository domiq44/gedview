#ifndef VERSION_H
#define VERSION_H

/* Version de construction intégrée à l'exécutable. */

/* Retourne la chaîne GEDVIEW_VERSION définie par le Makefile, ou « dev » hors dépôt Git. */
const char *app_version(void);

#endif
