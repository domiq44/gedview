/* Implémente les fixtures temporaires partagées par les tests unitaires et d'intégration. */
#include "fixtures.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testlib.h"

#define MAX_ITEMS 256

static char dirs[MAX_ITEMS][PATH_MAX];
static char files[MAX_ITEMS][PATH_MAX + 64];
static int ndirs, nfiles, registered;

/* Supprime les fichiers temporaires puis les dossiers créés par ce processus de test. */
static void cleanup(void) {
    for (int i = 0; i < nfiles; i++)
        unlink(files[i]);
    for (int i = ndirs - 1; i >= 0; i--)
        rmdir(dirs[i]);
}

/* Enregistre une seule fois le nettoyage automatique exécuté à la sortie du processus. */
static void ensure_cleanup_registered(void) {
    if (!registered) {
        atexit(cleanup);
        registered = 1;
    }
}

/* Crée et mémorise un dossier temporaire unique, ou interrompt le test en cas d'échec. */
const char *fx_tempdir(void) {
    ensure_cleanup_registered();
    if (ndirs == MAX_ITEMS) tl_fail(__FILE__, __LINE__, "too many temporary directories");
    const char *base = getenv("TMPDIR");
    snprintf(dirs[ndirs], sizeof dirs[ndirs], "%s/gedview-test-XXXXXX", base && *base ? base : "/tmp");
    if (!mkdtemp(dirs[ndirs])) tl_fail(__FILE__, __LINE__, "mkdtemp failed");
    return dirs[ndirs++];
}

/* Écrit une fixture dans dir et la consigne pour le nettoyage final. */
void fx_write(const char *dir, const char *filename, const char *content) {
    if (nfiles == MAX_ITEMS) tl_fail(__FILE__, __LINE__, "too many temporary files");
    snprintf(files[nfiles], sizeof files[nfiles], "%s/%s", dir, filename);
    FILE *f = fopen(files[nfiles], "w");
    if (!f) tl_fail(__FILE__, __LINE__, "cannot create %s", files[nfiles]);
    nfiles++; /* enregistré avant l'écriture pour être nettoyé même si elle échoue */
    fputs(content, f);
    fclose(f);
}

/* Écrit puis parse un GEDCOM temporaire ; échoue immédiatement si le parseur retourne NULL. */
Node *fx_parse(const char *gedcom_text) {
    static const char *dir;
    static int counter;
    if (!dir) dir = fx_tempdir();
    char name[32];
    snprintf(name, sizeof name, "f%d.ged", counter++);
    fx_write(dir, name, gedcom_text);
    char path[PATH_MAX + 64];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    Node *root = gedcom_parse(path);
    if (!root) tl_fail(__FILE__, __LINE__, "gedcom_parse failed for %s", path);
    return root;
}

/* Parse le texte GEDCOM et construit sa base ; échoue si les XREF sont dupliqués. */
Node *fx_database(const char *gedcom_text, Database *db) {
    Node *root = fx_parse(gedcom_text);
    if (!db_build(db, root)) tl_fail(__FILE__, __LINE__, "duplicate xref in GEDCOM fixture");
    return root;
}
