/* Construit l'arbre d'ascendance affichable d'un individu à partir des liens FAMC, HUSB et WIFE. */
#include "pedigree.h"

#include <string.h>

#include "view/family.h"

/* Remplit la case n et, tant que la limite de générations n'est pas atteinte, celles de ses parents. */
static void fill(Pedigree *p, const Database *db, int n, const Node *indi) {
    PedigreeBox *box = &p->box[n];
    box->indi = indi;
    person_summary(db, indi, box->label, sizeof box->label, box->dates, sizeof box->dates);
    int gen = pedigree_gen(n);
    if (gen + 1 > p->gens) p->gens = gen + 1;
    if (gen + 1 >= PEDIGREE_MAX_GENS) return;

    const Node *famc = node_sub(indi, "FAMC");
    const Node *fam = famc ? db_find(db, famc->value) : NULL;
    const Node *father = family_member(db, fam, "HUSB"), *mother = family_member(db, fam, "WIFE");
    if (father) fill(p, db, 2 * n, father);
    if (mother) fill(p, db, (2 * n) + 1, mother);
}

/* Réinitialise p puis remplit récursivement les ancêtres de root. */
void pedigree_build(Pedigree *p, const Database *db, const Node *root) {
    memset(p, 0, sizeof *p);
    if (root) fill(p, db, 1, root);
    if (p->gens < 1) p->gens = 1;
    pedigree_layout(p, 1, PEDIGREE_MAX_GENS);
}

/* Calcule le logarithme entier de n : la génération est le rang de son bit de poids fort. */
int pedigree_gen(int n) {
    int gen = 0;
    while (n >>= 1)
        gen++;
    return gen;
}

/* Indique si la case n existe dans p. */
static int present(const Pedigree *p, int n) { return n >= 1 && n < PEDIGREE_SLOTS && p->box[n].indi; }

/* Place n après ses parents visibles : une feuille prend la prochaine ligne libre, sinon la case se centre.
 */
static int place(Pedigree *p, int n, int depth, int *next) {
    int rows[2], count = 0;
    if (depth + 1 < p->shown)
        for (int parent = 2 * n; parent <= (2 * n) + 1; parent++)
            if (present(p, parent)) rows[count++] = place(p, parent, depth + 1, next);
    int row;
    if (count == 0) {
        row = *next;
        *next += PEDIGREE_PITCH;
    } else {
        row = count == 2 ? (rows[0] + rows[1]) / 2 : rows[0];
    }
    p->box[n].row = row;
    return row;
}

/* Dispose le sous-arbre de top, ou de la case 1 si top n'existe pas. */
void pedigree_layout(Pedigree *p, int top, int shown) {
    p->top = present(p, top) ? top : 1;
    p->shown = shown < 1 ? 1 : shown;
    int next = 0;
    place(p, p->top, 0, &next);
    p->lines = next - (PEDIGREE_PITCH - PEDIGREE_BOX_LINES);
}

/* Une case est visible si elle descend de top en moins de `shown` générations. */
int pedigree_visible(const Pedigree *p, int n) {
    int depth = pedigree_gen(n) - pedigree_gen(p->top);
    return present(p, n) && depth >= 0 && depth < p->shown && (n >> depth) == p->top;
}

/* Retourne la ligne calculée par pedigree_layout. */
int pedigree_row(const Pedigree *p, int n) { return p->box[n].row; }

/* Retourne la hauteur calculée par pedigree_layout. */
int pedigree_lines(const Pedigree *p) { return p->lines; }

/* Déplace la sélection vers la case demandée, ou la conserve si elle n'existe pas. */
int pedigree_move(const Pedigree *p, int n, PedigreeMove move) {
    int first = 1 << pedigree_gen(n), last = (2 * first) - 1;
    switch (move) {
    case PED_CHILD:
        return n > 1 ? n / 2 : n;
    case PED_PARENT:
        return present(p, 2 * n) ? 2 * n : present(p, (2 * n) + 1) ? (2 * n) + 1 : n;
    case PED_PREV:
        for (int i = n - 1; i >= first; i--)
            if (present(p, i)) return i;
        return n;
    case PED_NEXT:
        for (int i = n + 1; i <= last; i++)
            if (present(p, i)) return i;
        return n;
    }
    return n;
}
