/* Fournit à l'interface la version insérée par le système de compilation. */
#include "version.h"

#ifndef GEDVIEW_VERSION
#define GEDVIEW_VERSION "dev"
#endif

/* Retourne la constante de version compilée dans cet objet. */
const char *app_version(void) { return GEDVIEW_VERSION; }
