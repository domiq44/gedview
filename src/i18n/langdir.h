#ifndef LANGDIR_H
#define LANGDIR_H

/* Découverte du dossier des catalogues et énumération des codes de langue. */

#include <stddef.h>

#define BASE_LANG "en" /* langue de repli, toujours présente */

/* Cherche le dossier selon les emplacements configurés et copie son chemin dans out ; retourne 1 si trouvé.
 */
int langdir_find(char *out, size_t sz);

/* Alloue et trie les codes .lang trouvés dans dir, au plus max ; l'appelant libère les chaînes retournées. */
int langdir_list(const char *dir, char **codes, int max);

#endif
