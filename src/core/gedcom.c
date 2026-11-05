/* Parse le format ligne GEDCOM en arbre et centralise la propriété des chaînes et nœuds. */
#include "gedcom.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEVEL 62

/* Duplique une chaîne, en remplaçant NULL par une chaîne vide et en arrêtant en cas d'échec mémoire. */
static char *xstrdup(const char *s) {
    char *p = strdup(s ? s : "");
    if (!p) {
        perror("strdup");
        exit(1);
    }
    return p;
}

/* Alloue un nœud indépendant de ses parents et duplique ses champs textuels. */
static Node *new_node(int level, const char *xref, const char *tag, const char *value) {
    Node *n = calloc(1, sizeof *n);
    if (!n) {
        perror("calloc");
        exit(1);
    }
    n->level = level;
    n->xref = xstrdup(xref);
    n->tag = xstrdup(tag);
    n->value = xstrdup(value);
    return n;
}

/* Rattache c à la fin de la liste des enfants de p en maintenant parent et last. */
static void add_child(Node *p, Node *c) {
    c->parent = p;
    if (p->last)
        p->last->next = c;
    else
        p->child = c;
    p->last = c;
}

/* Ajoute s à la valeur de n, avec un saut de ligne lorsque newline est vrai. */
static void append_value(Node *n, const char *s, int newline) {
    size_t a = strlen(n->value), b = strlen(s);
    char *grown = realloc(n->value, a + b + 2);
    if (!grown) {
        perror("realloc");
        exit(1);
    }
    n->value = grown;
    if (newline) n->value[a++] = '\n';
    memcpy(n->value + a, s, b + 1);
}

/* Lit une ligne complète dans un tampon réutilisable qui grandit selon sa longueur. */
static int read_line(FILE *f, char **buf, size_t *cap, size_t *line_len) {
    size_t len = 0;
    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (!*cap || len >= *cap - 1) {
            size_t next = *cap ? *cap * 2 : 4096;
            if (next <= *cap) {
                errno = ENOMEM;
                perror("GEDCOM line");
                exit(1);
            }
            char *grown = realloc(*buf, next);
            if (!grown) {
                perror("realloc");
                exit(1);
            }
            *buf = grown;
            *cap = next;
        }
        (*buf)[len++] = (char)ch;
        if (ch == '\n') break;
    }
    if (!len && ch == EOF) return 0;
    (*buf)[len] = 0;
    *line_len = len;
    return 1;
}

/* Libère récursivement une chaîne de frères et chacun de leurs sous-arbres. */
static void free_tree(Node *n) {
    while (n) {
        Node *next = n->next;
        free_tree(n->child);
        free(n->xref);
        free(n->tag);
        free(n->value);
        free(n);
        n = next;
    }
}

/* Point d'entrée public du destructeur de l'arbre GEDCOM. */
void gedcom_free(Node *root) { free_tree(root); }

/* Signale une ligne écartée et incrémente le compteur facultatif. */
static void warn_line(const char *path, size_t line_number, const char *reason, size_t *ignored_lines) {
    if (ignored_lines) (*ignored_lines)++;
    fprintf(stderr, "%s:%zu: %s\n", path, line_number, reason);
}

/* Lit les lignes, reconstitue la hiérarchie et retourne NULL si l'accès au fichier échoue. */
Node *gedcom_parse_with_warnings(const char *path, size_t *ignored_lines) {
    if (ignored_lines) *ignored_lines = 0;
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return NULL;
    }
    Node *root = new_node(-1, "", "ROOT", "");
    Node *stack[MAX_LEVEL + 2] = {root}; /* stack[l] = parent d'un noeud de niveau l */
    char *buf = NULL;
    size_t cap = 0;
    size_t line_number = 0;
    size_t line_len = 0;
    while (read_line(f, &buf, &cap, &line_len)) {
        line_number++;
        size_t eol = 0;
        while (eol < line_len && buf[eol] != '\r' && buf[eol] != '\n')
            eol++;
        buf[eol] = 0;
        char *p = buf;
        if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
        while (*p == ' ')
            p++;
        if (!*p) continue;
        if (*p < '0' || *p > '9') {
            warn_line(path, line_number, "ignoring malformed GEDCOM line", ignored_lines);
            continue;
        }
        errno = 0;
        char *level_end;
        long parsed_level = strtol(p, &level_end, 10);
        if (errno == ERANGE || parsed_level > MAX_LEVEL || (*level_end && *level_end != ' ')) {
            warn_line(path, line_number, "ignoring invalid GEDCOM level", ignored_lines);
            continue;
        }
        int level = (int)parsed_level;
        p = level_end;
        while (*p == ' ')
            p++;
        const char *xref = "";
        if (*p == '@') {
            xref = p;
            while (*p && *p != ' ')
                p++;
            if (*p) *p++ = 0;
        }
        while (*p == ' ')
            p++;
        const char *tag = p;
        while (*p && *p != ' ')
            p++;
        if (p == tag) {
            warn_line(path, line_number, "ignoring GEDCOM line without a tag", ignored_lines);
            continue;
        }
        if (*p) *p++ = 0;
        const char *value = p;

        if (!stack[level]) {
            warn_line(path, line_number, "ignoring GEDCOM line with a missing parent level", ignored_lines);
            continue;
        }
        if (level > 0 && (!strcmp(tag, "CONC") || !strcmp(tag, "CONT"))) {
            append_value(stack[level], value, tag[3] == 'T');
            continue;
        }
        Node *n = new_node(level, xref, tag, value);
        add_child(stack[level], n);
        stack[level + 1] = n;
        for (int i = level + 2; i < MAX_LEVEL + 2; i++)
            stack[i] = NULL;
    }
    int read_error = ferror(f);
    free(buf);
    if (fclose(f) != 0) read_error = 1;
    if (read_error) {
        fprintf(stderr, "%s: read error\n", path);
        free_tree(root);
        return NULL;
    }
    return root;
}

/* Variante du parseur qui ne demande pas le nombre de lignes ignorées. */
Node *gedcom_parse(const char *path) { return gedcom_parse_with_warnings(path, NULL); }

/* Retourne le premier enfant direct dont le tag correspond exactement à tag. */
Node *node_sub(const Node *n, const char *tag) {
    for (Node *c = n->child; c; c = c->next)
        if (!strcmp(c->tag, tag)) return c;
    return NULL;
}

/* Reconnaît la forme minimale d'une référence XREF sans consulter la base. */
int gedcom_is_ref(const char *v) {
    size_t l = strlen(v);
    return l > 2 && v[0] == '@' && v[l - 1] == '@' && v[1] != '@';
}
