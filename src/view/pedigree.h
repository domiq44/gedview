#ifndef PEDIGREE_H
#define PEDIGREE_H

/* Arbre d'ascendance d'un individu : cases numérotées à la manière Ahnentafel, sans ncurses. */

#include "core/database.h"

/* Nombre maximal de générations (la personne de départ comprise). */
#define PEDIGREE_MAX_GENS 8
/* Les numéros de case vont de 1 à PEDIGREE_SLOTS - 1 : 1 = individu, 2n = père de n, 2n+1 = mère de n. */
#define PEDIGREE_SLOTS (1 << PEDIGREE_MAX_GENS)
/* Hauteur d'une case en lignes et pas vertical entre deux feuilles consécutives. */
#define PEDIGREE_BOX_LINES 2
#define PEDIGREE_PITCH 3

typedef struct {
    const Node *indi; /* NULL si l'ancêtre est inconnu */
    int row;          /* ligne du haut, valide pour les cases visibles après pedigree_layout */
    char label[96];
    char dates[48]; /* « ° naissance - + décès », vide si aucune date */
} PedigreeBox;

typedef struct {
    PedigreeBox box[PEDIGREE_SLOTS];
    int gens;  /* nombre de générations non vides, au moins 1 */
    int top;   /* case la plus à gauche de la disposition courante */
    int shown; /* nombre de générations de la disposition courante */
    int lines; /* hauteur de la disposition courante */
} Pedigree;

typedef enum { PED_CHILD, PED_PARENT, PED_PREV, PED_NEXT } PedigreeMove;

/* Construit l'arbre d'ascendance de root (premier FAMC de chaque individu) ; les cycles sont bornés par la
 * profondeur. La disposition initiale montre toutes les générations depuis la case 1. */
void pedigree_build(Pedigree *p, const Database *db, const Node *root);

/* Retourne la génération (colonne) du numéro de case n : 0 pour l'individu de départ. */
int pedigree_gen(int n);

/* Dispose les ancêtres de la case top sur `shown` générations : chaque feuille occupe une ligne de
   PEDIGREE_PITCH, les ancêtres inconnus ne prennent pas de place et chaque case est centrée sur ses parents.
 */
void pedigree_layout(Pedigree *p, int top, int shown);

/* Indique si la case n fait partie de la disposition courante. */
int pedigree_visible(const Pedigree *p, int n);

/* Retourne la ligne du haut de la case visible n. */
int pedigree_row(const Pedigree *p, int n);

/* Retourne la hauteur totale en lignes de la disposition courante. */
int pedigree_lines(const Pedigree *p);

/* Retourne la case atteinte depuis n selon move (enfant, père sinon mère, voisin précédent/suivant de la même
   génération), ou n si aucun déplacement n'est possible. */
int pedigree_move(const Pedigree *p, int n, PedigreeMove move);

#endif
