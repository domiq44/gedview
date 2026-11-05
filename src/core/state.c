/* Fichier d'état « last-person » : une ligne « slot<tab>valeur<tab>chemin canonique » par valeur et par
 * fichier. */
#define _XOPEN_SOURCE 700
#include "state.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Écrit dans out le chemin du fichier d'état ; retourne 0 si aucun dossier ne peut être déterminé. */
static int state_file(char *out, size_t sz) {
    const char *dir = getenv("GEDVIEW_STATE_DIR"), *xdg = getenv("XDG_STATE_HOME"), *home = getenv("HOME");
    int n;
    if (dir && *dir)
        n = snprintf(out, sz, "%s/last-person", dir);
    else if (xdg && *xdg)
        n = snprintf(out, sz, "%s/gedview/last-person", xdg);
    else if (home && *home)
        n = snprintf(out, sz, "%s/.local/state/gedview/last-person", home);
    else
        return 0;
    return n > 0 && (size_t)n < sz;
}

/* Crée le dossier parent de file et ses ancêtres manquants ; les échecs se voient à l'ouverture du fichier.
 */
static void make_parents(char *file) {
    for (char *p = strchr(file + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = 0;
        mkdir(file, 0700);
        *p = '/';
    }
}

/* Écrit dans key le chemin canonique de gedcom_path, ou le chemin tel quel s'il n'existe pas. */
static int state_key(const char *gedcom_path, char *key, size_t sz) {
    char resolved[PATH_MAX];
    const char *path = realpath(gedcom_path, resolved) ? resolved : gedcom_path;
    return !strpbrk(path, "\t\n") && snprintf(key, sz, "%s", path) < (int)sz;
}

/* Sépare une ligne « slot<tab>valeur<tab>chemin » en place ; retourne 1 et renseigne value et path si elle
 * est bien formée. */
static int split_line(char *line, char **value, char **path) {
    char *newline = strchr(line, '\n');
    if (newline) *newline = 0;
    char *first = strchr(line, '\t');
    char *second = first ? strchr(first + 1, '\t') : NULL;
    if (!second || first == line || second == first + 1) return 0;
    *first = *second = 0;
    *value = first + 1;
    *path = second + 1;
    return 1;
}

/* Recherche la valeur du slot pour gedcom_path dans le fichier d'état. */
int state_load(const char *gedcom_path, const char *slot, char *value, size_t sz) {
    char file[PATH_MAX], key[PATH_MAX];
    if (!state_file(file, sizeof file) || !state_key(gedcom_path, key, sizeof key)) return 0;
    FILE *in = fopen(file, "r");
    if (!in) return 0;

    int found = 0;
    char *line = NULL;
    size_t cap = 0;
    while (!found && getline(&line, &cap, in) != -1) {
        char *stored, *path;
        if (split_line(line, &stored, &path) && !strcmp(line, slot) && !strcmp(path, key) &&
            strlen(stored) < sz) {
            snprintf(value, sz, "%s", stored);
            found = 1;
        }
    }
    free(line);
    fclose(in);
    return found;
}

/* Indique si text est non vide et ne contient ni tabulation ni saut de ligne. */
static int is_field(const char *text) { return text && *text && !strpbrk(text, "\t\n"); }

/* Réécrit le fichier d'état sans les anciennes entrées de key, puis ajoute les nouvelles. */
int state_save(const char *gedcom_path, const StateEntry *entries, int count) {
    char file[PATH_MAX], temp[PATH_MAX + 8], key[PATH_MAX];
    if (!state_file(file, sizeof file) || !state_key(gedcom_path, key, sizeof key)) return 0;
    for (int i = 0; i < count; i++)
        if (!is_field(entries[i].slot) || !is_field(entries[i].value)) return 0;
    make_parents(file);
    if (snprintf(temp, sizeof temp, "%s.tmp", file) >= (int)sizeof temp) return 0;
    FILE *out = fopen(temp, "w");
    if (!out) return 0;

    FILE *in = fopen(file, "r");
    char *line = NULL;
    size_t cap = 0;
    while (in && getline(&line, &cap, in) != -1) {
        char *value, *path;
        if (split_line(line, &value, &path) && strcmp(path, key) != 0)
            fprintf(out, "%s\t%s\t%s\n", line, value, path);
    }
    free(line);
    if (in) fclose(in);

    int ok = 1;
    for (int i = 0; i < count; i++)
        ok = fprintf(out, "%s\t%s\t%s\n", entries[i].slot, entries[i].value, key) > 0 && ok;
    ok = fclose(out) == 0 && ok;
    if (!ok || rename(temp, file) != 0) {
        remove(temp);
        return 0;
    }
    return 1;
}
