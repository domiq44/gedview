/* Localise le répertoire des catalogues et énumère les fichiers de langue installés. */
#include "langdir.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Vérifie que le répertoire contient le catalogue de base lisible. */
static int has_base(const char *dir) {
    char path[PATH_MAX + 32];
    snprintf(path, sizeof path, "%s/" BASE_LANG ".lang", dir);
    return access(path, R_OK) == 0;
}

/* Copie dir dans out uniquement si le catalogue de base y est disponible. */
static int accept(const char *dir, char *out, size_t sz) {
    if (!has_base(dir)) return 0;
    snprintf(out, sz, "%s", dir);
    return 1;
}

/* Cherche le dossier de langue dans l'environnement, près de l'exécutable puis aux chemins système. */
int langdir_find(char *out, size_t sz) {
    const char *env = getenv("GEDVIEW_LANG_DIR");
    if (env && accept(env, out, sz)) return 1;

    char exe[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n > 0) {
        exe[n] = 0;
        char *slash = strrchr(exe, '/');
        if (slash) {
            *slash = 0;
            char cand[PATH_MAX + 8];
            snprintf(cand, sizeof cand, "%s/lang", exe);
            if (accept(cand, out, sz)) return 1;
        }
    }
    static const char *const fixed[] = {"lang", "/usr/local/share/gedview/lang", "/usr/share/gedview/lang"};
    for (size_t i = 0; i < sizeof fixed / sizeof *fixed; i++)
        if (accept(fixed[i], out, sz)) return 1;
    return 0;
}

/* Compare deux codes de langue pour produire une liste déterministe. */
static int cmp_codes(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

/* Alloue les codes de fichiers .lang trouvés dans dir, jusqu'à max, puis les trie. */
int langdir_list(const char *dir, char **codes, int max) {
    DIR *d = opendir(dir);
    if (!d) return 0;
    int n = 0;
    while (n < max) {
        const struct dirent *e = readdir(d);
        if (!e) break;
        size_t len = strlen(e->d_name);
        if (len > 5 && len <= 13 && !strcmp(e->d_name + len - 5, ".lang")) {
            codes[n] = strndup(e->d_name, len - 5);
            if (!codes[n]) {
                perror("strndup");
                exit(1);
            }
            n++;
        }
    }
    closedir(d);
    qsort((void *)codes, n, sizeof *codes, cmp_codes);
    return n;
}
