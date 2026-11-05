/* Applique à l'état de navigation les actions propres à l'arbre d'ascendance et à la liste des enfants. */
#include "treenav.h"

#include "view/family.h"

/* Bascule l'arbre pour l'individu courant et sélectionne sa case racine. */
void treenav_toggle(Nav *nav, const Database *db) {
    if (nav->tree) {
        nav->tree = 0;
    } else if (nav->kind == K_INDI && nav_current(nav, db)) {
        nav->tree = 1;
        nav->focus_detail = 0;
    }
    nav->tree_sel = 1;
    nav->child_menu = 0;
}

/* Ouvre la liste des enfants depuis la racine, sinon reconstruit l'arbre pour résoudre le déplacement. */
void treenav_move(Nav *nav, const Database *db, PedigreeMove move) {
    if (move == PED_CHILD && nav->tree_sel == 1) {
        const Node *children[FAMILY_MAX_CHILDREN];
        if (person_children(db, nav_current(nav, db), children, FAMILY_MAX_CHILDREN)) {
            nav->child_menu = 1;
            nav->child_sel = 0;
        }
        return;
    }
    Pedigree p;
    pedigree_build(&p, db, nav_current(nav, db));
    nav->tree_sel = pedigree_move(&p, nav->tree_sel, move);
}

/* Sélectionne l'individu de la case n et ramène la sélection de l'arbre à la racine. */
void treenav_recenter(Nav *nav, const Database *db, int n) {
    Pedigree p;
    pedigree_build(&p, db, nav_current(nav, db));
    if (n < 1 || n >= PEDIGREE_SLOTS || !p.box[n].indi) return;
    nav_select_individual(nav, db, p.box[n].indi);
    nav->tree_sel = 1;
    nav->child_menu = 0;
}

/* Borne child_sel au nombre d'enfants de l'individu courant. */
void treenav_menu_move(Nav *nav, const Database *db, int delta) {
    const Node *children[FAMILY_MAX_CHILDREN];
    int count = person_children(db, nav_current(nav, db), children, FAMILY_MAX_CHILDREN);
    int next = nav->child_sel + delta;
    nav->child_sel = next >= count ? count - 1 : next;
    if (nav->child_sel < 0) nav->child_sel = 0;
}

/* Referme la liste des enfants. */
void treenav_menu_close(Nav *nav) { nav->child_menu = 0; }

/* Sélectionne l'enfant choisi puis referme la liste. */
void treenav_menu_choose(Nav *nav, const Database *db) {
    const Node *children[FAMILY_MAX_CHILDREN];
    int count = person_children(db, nav_current(nav, db), children, FAMILY_MAX_CHILDREN);
    if (nav->child_sel >= 0 && nav->child_sel < count) {
        nav_select_individual(nav, db, children[nav->child_sel]);
        nav->tree_sel = 1;
    }
    nav->child_menu = 0;
}
