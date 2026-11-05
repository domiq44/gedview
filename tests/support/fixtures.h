#ifndef FIXTURES_H
#define FIXTURES_H

/* Helpers de fixtures GEDCOM et de fichiers temporaires pour les tests. */

#include "core/gedcom.h"
#include "core/database.h"

/* Les fichiers et dossiers créés ici sont supprimés à la fin du programme de test. */

/* Crée un dossier temporaire qui sera supprimé automatiquement à la fin du test. */
const char *fx_tempdir(void);
/* Écrit content dans dir/filename et enregistre le fichier pour le nettoyage final. */
void fx_write(const char *dir, const char *filename, const char *content);

/* Écrit `gedcom_text` dans un fichier temporaire et le parse ; fait échouer le test si la lecture échoue. */
Node *fx_parse(const char *gedcom_text);

/* fx_parse + db_build. Retourne la racine parsée. */
Node *fx_database(const char *gedcom_text, Database *db);

#endif
