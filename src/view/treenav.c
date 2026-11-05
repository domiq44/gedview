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
    nav->menu = 0;
}

/* Passe au frère ou à la sœur voisin(e) de l'individu courant, dans l'ordre de leur famille ; sans effet aux
 * extrémités. */
static void step_sibling(Nav *nav, const Database *db, int delta) {
    const Node *current = nav_current(nav, db);
    const Node *siblings[FAMILY_MAX_CHILDREN];
    int count = person_siblings(db, current, siblings, FAMILY_MAX_CHILDREN);
    for (int i = 0; i < count; i++) {
        if (siblings[i] != current) continue;
        if (i + delta >= 0 && i + delta < count) nav_select_individual(nav, db, siblings[i + delta]);
        return;
    }
}

/* Sur la racine, ouvre la liste des enfants ou passe aux frères et sœurs ; sinon résout le déplacement dans
 * l'arbre. */
void treenav_move(Nav *nav, const Database *db, PedigreeMove move) {
    if (nav->tree_sel == 1 && (move == PED_PREV || move == PED_NEXT)) {
        step_sibling(nav, db, move == PED_NEXT ? 1 : -1);
        return;
    }
    if (move == PED_CHILD && nav->tree_sel == 1) {
        const Node *children[FAMILY_MAX_CHILDREN];
        if (person_children(db, nav_current(nav, db), children, FAMILY_MAX_CHILDREN)) {
            nav->menu = NAV_MENU_CHILDREN;
            nav->menu_sel = 0;
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
    nav->menu = 0;
}

/* Ouvre la liste des proches de l'individu courant, s'il en a. */
void treenav_open_relatives(Nav *nav, const Database *db) {
    Relative relatives[FAMILY_MAX_RELATIVES];
    if (!person_relatives(db, nav_current(nav, db), relatives, FAMILY_MAX_RELATIVES)) return;
    nav->menu = NAV_MENU_RELATIVES;
    nav->menu_sel = 0;
}

/* Liste les individus de la liste ouverte (enfants ou proches) et retourne leur nombre. */
static int menu_entries(const Nav *nav, const Database *db, const Node **out, int max) {
    const Node *current = nav_current(nav, db);
    if (nav->menu == NAV_MENU_CHILDREN) return person_children(db, current, out, max);
    Relative relatives[FAMILY_MAX_RELATIVES];
    int count = person_relatives(db, current, relatives, FAMILY_MAX_RELATIVES);
    if (count > max) count = max;
    for (int i = 0; i < count; i++)
        out[i] = relatives[i].indi;
    return count;
}

/* Borne menu_sel au nombre d'entrées de la liste ouverte. */
void treenav_menu_move(Nav *nav, const Database *db, int delta) {
    const Node *entries[FAMILY_MAX_RELATIVES];
    int count = menu_entries(nav, db, entries, FAMILY_MAX_RELATIVES);
    int next = nav->menu_sel + delta;
    nav->menu_sel = next >= count ? count - 1 : next;
    if (nav->menu_sel < 0) nav->menu_sel = 0;
}

/* Referme la liste ouverte. */
void treenav_menu_close(Nav *nav) { nav->menu = NAV_MENU_NONE; }

/* Sélectionne l'entrée choisie puis referme la liste. */
void treenav_menu_choose(Nav *nav, const Database *db) {
    const Node *entries[FAMILY_MAX_RELATIVES];
    int count = menu_entries(nav, db, entries, FAMILY_MAX_RELATIVES);
    if (nav->menu_sel >= 0 && nav->menu_sel < count) {
        nav_select_individual(nav, db, entries[nav->menu_sel]);
        nav->tree_sel = 1;
    }
    nav->menu = NAV_MENU_NONE;
}
