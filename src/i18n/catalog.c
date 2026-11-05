/* Lit un fichier .lang, valide ses conversions printf et conserve les entrées en mémoire. */
#include "catalog.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* `fmt` : unique conversion printf autorisée dans la valeur (0 = aucune). Protège des formats forgés. */
static const struct {
    const char *name;
    char fmt;
} KEYS[NSTR] = {
    [S_NONAME] = {"NONAME", 0},
    [S_T_INDI] = {"T_INDI", 'd'},
    [S_T_FAM] = {"T_FAM", 'd'},
    [S_T_SRC] = {"T_SRC", 'd'},
    [S_T_REPO] = {"T_REPO", 'd'},
    [S_T_OBJ] = {"T_OBJ", 'd'},
    [S_T_NOTE] = {"T_NOTE", 'd'},
    [S_T_CITA] = {"T_CITA", 'd'},
    [S_DETAIL] = {"DETAIL", 0},
    [S_SEARCH] = {"SEARCH", 's'},
    [S_STATUS] = {"STATUS", 0},
    [S_SMALLSCREEN] = {"SMALLSCREEN", 0},
    [S_USAGE] = {"USAGE", 's'},
    [S_NOINDI] = {"NOINDI", 0},
    [S_PARSE_WARNINGS] = {"PARSE_WARNINGS", 's'},
    [S_BLANK] = {"BLANK", 0},
    [S_H_TITLE] = {"H_TITLE", 0},
    [S_H_VIEWS] = {"H_VIEWS", 0},
    [S_H_I] = {"H_I", 0},
    [S_H_F] = {"H_F", 0},
    [S_H_S] = {"H_S", 0},
    [S_H_R] = {"H_R", 0},
    [S_H_M] = {"H_M", 0},
    [S_H_N] = {"H_N", 0},
    [S_H_CITA] = {"H_CITA", 0},
    [S_H_NAV] = {"H_NAV", 0},
    [S_H_UPDOWN] = {"H_UPDOWN", 0},
    [S_H_PAGE] = {"H_PAGE", 0},
    [S_H_HOME] = {"H_HOME", 0},
    [S_H_TAB] = {"H_TAB", 0},
    [S_H_ENTER] = {"H_ENTER", 0},
    [S_H_SEARCHT] = {"H_SEARCHT", 0},
    [S_H_SEARCH] = {"H_SEARCH", 0},
    [S_H_SEARCHX] = {"H_SEARCHX", 0},
    [S_H_SEARCHR] = {"H_SEARCHR", 0},
    [S_H_MISC] = {"H_MISC", 0},
    [S_H_HELP] = {"H_HELP", 0},
    [S_H_LANG] = {"H_LANG", 0},
    [S_H_QUIT] = {"H_QUIT", 0},
    [S_H_BACK] = {"H_BACK", 0},
    [S_H_HISTORY] = {"H_HISTORY", 0},
    [S_H_COLOR] = {"H_COLOR", 0},
    [S_T_TREE] = {"T_TREE", 0},
    [S_STATUS_TREE] = {"STATUS_TREE", 0},
    [S_H_TREE] = {"H_TREE", 0},
    [S_T_CHILDREN] = {"T_CHILDREN", 0},
};

/* Duplique une valeur de catalogue et termine le processus si l'allocation échoue. */
static char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) {
        perror("strdup");
        exit(1);
    }
    return p;
}

/* Décode en place les échappements documentés du format .lang. */
static void unescape(char *s) {
    char *w = s;
    for (const char *r = s; *r; r++) {
        if (*r == '\\' && r[1] == 'n') {
            *w++ = '\n';
            r++;
        } else if (*r == '\\' && r[1] == '\\') {
            *w++ = '\\';
            r++;
        } else {
            *w++ = *r;
        }
    }
    *w = 0;
}

/* Accepte les pourcentages littéraux et au plus la conversion printf attendue. */
static int valid_format(const char *v, char fmt) {
    int used = 0;
    for (const char *p = v; *p; p++) {
        if (*p != '%') continue;
        if (p[1] == '%') {
            p++;
            continue;
        }
        if (fmt && p[1] == fmt && !used) {
            used = 1;
            p++;
            continue;
        }
        return 0;
    }
    return 1;
}

/* Remplace une clé connue ou ajoute un libellé de tag ; ignore les entrées invalides/inconnues. */
static void set_entry(Catalog *c, const char *key, const char *value) {
    if (!strncmp(key, "TAG.", 4)) {
        if (c->ntags < MAX_TAGS && strlen(key + 4) < sizeof c->tags[0].tag) {
            snprintf(c->tags[c->ntags].tag, sizeof c->tags[0].tag, "%s", key + 4);
            c->tags[c->ntags++].label = xstrdup(value);
        }
        return;
    }
    for (int i = 0; i < NSTR; i++)
        if (!strcmp(key, KEYS[i].name) && valid_format(value, KEYS[i].fmt)) {
            free(c->msg[i]);
            c->msg[i] = xstrdup(value);
            return;
        }
}

/* Lit une ligne complète dans un tampon réutilisable et l'agrandit à la demande. */
static int read_line(FILE *f, char **line, size_t *cap) {
    size_t len = 0;
    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (!*cap || len >= *cap - 1) {
            size_t next = *cap ? *cap * 2 : 256;
            if (next <= *cap) {
                errno = ENOMEM;
                perror("catalog line");
                exit(1);
            }
            char *grown = realloc(*line, next);
            if (!grown) {
                perror("realloc");
                exit(1);
            }
            *line = grown;
            *cap = next;
        }
        (*line)[len++] = (char)ch;
        if (ch == '\n') break;
    }
    if (!len && ch == EOF) return 0;
    (*line)[len] = 0;
    return 1;
}

/* Libère les messages et libellés détenus par c puis remet toutes ses entrées à zéro. */
void catalog_clear(Catalog *c) {
    for (int i = 0; i < NSTR; i++)
        free(c->msg[i]);
    for (int i = 0; i < c->ntags; i++)
        free(c->tags[i].label);
    memset(c, 0, sizeof *c);
}

/* Charge les lignes CLE=valeur, décode leurs échappements et laisse les clés absentes intactes. */
void catalog_load(Catalog *c, const char *dir, const char *code) {
    char path[PATH_MAX + 64];
    snprintf(path, sizeof path, "%s/%s.lang", dir, code);
    FILE *f = fopen(path, "r");
    if (!f) return;
    char *line = NULL;
    size_t cap = 0;
    while (read_line(f, &line, &cap)) {
        char *eol = strpbrk(line, "\r\n");
        if (eol) *eol = 0;
        char *eq = strchr(line, '=');
        if (line[0] == '#' || !eq) continue;
        *eq = 0;
        unescape(eq + 1);
        set_entry(c, line, eq + 1);
    }
    free(line);
    fclose(f);
}

/* Retourne le message indexé par id sans transférer sa propriété. */
const char *catalog_msg(const Catalog *c, int id) { return c->msg[id]; }

/* Recherche un libellé de tag dans les entrées du catalogue. */
const char *catalog_tag(const Catalog *c, const char *tag) {
    for (int i = 0; i < c->ntags; i++)
        if (!strcmp(c->tags[i].tag, tag)) return c->tags[i].label;
    return NULL;
}

/* Retourne le nom sérialisé de la clé associée à id. */
const char *catalog_key_name(int id) { return KEYS[id].name; }
