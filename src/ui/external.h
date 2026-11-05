#ifndef EXTERNAL_H
#define EXTERNAL_H

/* Ouverture contrôlée des liens réseau et médias depuis la vue de détail. */

#include "view/detail.h"

/* Lance xdg-open pour une URL HTTP(S) ou un média autorisé, résolu depuis gedcom_path. */
void external_open(const char *target, LineLinkKind kind, const char *gedcom_path);

#endif