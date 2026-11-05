#ifndef SESSION_H
#define SESSION_H

/* Reprise de la derni\u00e8re session : vue affich\u00e9e et s\u00e9lection de chaque liste, par fichier
 * GEDCOM. */

#include "core/database.h"
#include "view/nav.h"

/* Restaure dans nav les s\u00e9lections et la vue m\u00e9moris\u00e9es pour gedcom_path ; les enregistrements
 * disparus sont ignor\u00e9s. */
void session_restore(Nav *nav, const Database *db, const char *gedcom_path);

/* M\u00e9morise la vue et la s\u00e9lection de chaque liste (hors citations) pour gedcom_path ; \u00e9chec
 * silencieux. */
void session_save(const Nav *nav, const Database *db, const char *gedcom_path);

#endif
