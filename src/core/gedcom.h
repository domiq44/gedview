#ifndef GEDCOM_H
#define GEDCOM_H

/* Modèle arborescent GEDCOM et fonctions de parsing, de recherche locale et de libération. */

#include <stddef.h>

/* Nœud GEDCOM : child/next décrivent les sous-niveaux et parent/last facilitent leur parcours. */
typedef struct Node {
    int level;
    char *xref, *tag, *value;
    struct Node *parent, *child, *last, *next;
} Node;

/* Parse path et retourne une racine synthétique de niveau -1 ; retourne NULL si le fichier est illisible. */
Node *gedcom_parse(const char *path);

/* Variante qui compte dans ignored_lines les lignes mal formées ou orphelines ; le pointeur peut être NULL.
 */
Node *gedcom_parse_with_warnings(const char *path, size_t *ignored_lines);

/* Libère récursivement l'arbre retourné par gedcom_parse ; accepte NULL. */
void gedcom_free(Node *root);

/* Retourne le premier enfant direct portant tag, sans parcourir les descendants, ou NULL. */
Node *node_sub(const Node *n, const char *tag);

/* Retourne vrai si value a la forme minimale @X@, sans valider l'existence de la cible. */
int gedcom_is_ref(const char *value);

#endif
