/* Dessine l'arbre d'ascendance (cases, connecteurs) et la liste des enfants avec ncurses. */
#define _XOPEN_SOURCE_EXTENDED 1
#include "treeview.h"

#include <stdio.h>
#include <string.h>

#include "i18n/i18n.h"
#include "ui/screen.h"
#include "view/family.h"
#include "view/pedigree.h"

/* Dimensions d'une case de l'arbre et espace réservé aux connecteurs entre deux générations. */
enum { TREE_BOX_WIDTH = 26, TREE_GAP = 4 };

/* Zone cliquable d'une case de l'arbre, en coordonnées écran, remplie à chaque dessin. */
static struct {
    int y, x, h, w;
} tree_hits[PEDIGREE_SLOTS];

/* Dessine le cadre de win et son titre. */
static void draw_frame(WINDOW *win, const char *title) {
    int colors = screen_colors_enabled();
    int frame = colors ? COLOR_PAIR(SCREEN_PAIR_FRAME) : A_NORMAL;
    int heading = colors ? (int)(COLOR_PAIR(SCREEN_PAIR_TITLE) | A_BOLD) : A_NORMAL;
    wattrset(win, frame);
    box(win, 0, 0);
    wattrset(win, heading);
    mvwprintw(win, 0, 2, "%s", title);
    wattrset(win, A_NORMAL);
}

/* Retourne l'attribut de la ligne sélectionnée, avec repli monochrome. */
static int selected_attrs(void) {
    return screen_colors_enabled() ? (int)(COLOR_PAIR(SCREEN_PAIR_SELECTED) | A_BOLD) : A_REVERSE;
}

/* Choisit le caractère de tracé d'une cellule du connecteur selon les directions qui la traversent. */
static const char *connector_glyph(int up, int down, int left, int right) {
    static const char *const GLYPHS[16] = {
        [0] = " ",  [1] = "─", /* right */
        [2] = "─",             /* left */
        [3] = "─",             /* left, right */
        [4] = "│",             /* down */
        [5] = "┌",             /* down, right */
        [6] = "┐",             /* down, left */
        [7] = "┬",             /* down, left, right */
        [8] = "│",             /* up */
        [9] = "└",             /* up, right */
        [10] = "┘",            /* up, left */
        [11] = "┴",            /* up, left, right */
        [12] = "│",            /* up, down */
        [13] = "├",            /* up, down, right */
        [14] = "┤",            /* up, down, left */
        [15] = "┼",            /* all */
    };
    return GLYPHS[(up << 3) | (down << 2) | (left << 1) | right];
}

/* Dessine, entre la case n et ses parents visibles, un connecteur horizontal à trois branches. */
static void draw_links(WINDOW *win, const Pedigree *p, int n, int x, int y_origin, int first_row,
                       int last_row) {
    int child_row = y_origin + pedigree_row(p, n);
    int rows[2], count = 0;
    for (int parent = 2 * n; parent <= (2 * n) + 1; parent++)
        if (pedigree_visible(p, parent)) rows[count++] = y_origin + pedigree_row(p, parent);
    if (!count) return;

    int top = child_row, bottom = child_row;
    for (int i = 0; i < count; i++) {
        if (rows[i] < top) top = rows[i];
        if (rows[i] > bottom) bottom = rows[i];
    }
    int stem = x + 1;
    for (int r = top; r <= bottom; r++) {
        if (r < first_row || r > last_row) continue;
        int is_parent = 0;
        for (int i = 0; i < count; i++)
            if (rows[i] == r) is_parent = 1;
        mvwaddstr(win, r, stem, connector_glyph(r > top, r < bottom, r == child_row, is_parent));
        if (r == child_row) mvwaddstr(win, r, x, "─");
        if (is_parent)
            for (int c = stem + 1; c < x + TREE_GAP; c++)
                mvwaddstr(win, r, c, "─");
    }
}

/* Dessine la case n à (x, y), clippée aux lignes 1..rows, et enregistre sa zone cliquable. */
static void draw_box(WINDOW *win, const Pedigree *p, int n, int x, int y, int box_w, int rows, int selected) {
    const char *text[PEDIGREE_BOX_LINES] = {p->box[n].label, p->box[n].dates};
    int first_visible = -1, visible = 0;
    for (int i = 0; i < PEDIGREE_BOX_LINES; i++) {
        if (y + i < 1 || y + i > rows) continue;
        if (first_visible < 0) first_visible = y + i;
        visible++;
        wattrset(win, selected ? selected_attrs() : 0);
        mvwhline(win, y + i, x, ' ', box_w);
        mvwaddnstr(win, y + i, x, text[i], box_w);
        wattrset(win, A_NORMAL);
    }
    if (visible) {
        tree_hits[n].y = first_visible;
        tree_hits[n].x = x;
        tree_hits[n].h = visible;
        tree_hits[n].w = box_w;
    }
}

/* Dispose l'arbre pour la largeur w et retourne la ligne de l'écran qui correspond à la ligne 0 de l'arbre.
 */
static int arrange(Pedigree *p, int sel, int w, int h, int *first_gen, int *box_w, int *step) {
    *box_w = w - 2 < TREE_BOX_WIDTH ? w - 2 : TREE_BOX_WIDTH;
    *step = *box_w + TREE_GAP;
    int fit = (w - 2 + TREE_GAP) / *step;
    if (fit < 1) fit = 1;
    *first_gen = pedigree_gen(sel) - fit + 1;
    if (*first_gen < 0) *first_gen = 0;
    /* Seul le sous-arbre de l'ancêtre de la colonne de gauche est disposé : il reste compact. */
    pedigree_layout(p, sel >> (pedigree_gen(sel) - *first_gen), fit);

    int rows = h - 2, total = pedigree_lines(p);
    int scroll = (total - rows) / 2; /* centré si l'arbre tient, sinon centré sur la sélection */
    if (total > rows) {
        scroll = pedigree_row(p, sel) + (PEDIGREE_BOX_LINES / 2) - (rows / 2);
        if (scroll > total - rows) scroll = total - rows;
        if (scroll < 0) scroll = 0;
    }
    return 1 - scroll;
}

/* Dessine le cadre puis chaque case visible avec ses connecteurs. */
void treeview_draw(WINDOW *win, const Database *db, const Nav *nav) {
    int h, w;
    getmaxyx(win, h, w);
    draw_frame(win, T(S_T_TREE));

    memset(tree_hits, 0, sizeof tree_hits);
    Pedigree p;
    pedigree_build(&p, db, nav_current(nav, db));
    int sel =
        nav->tree_sel >= 1 && nav->tree_sel < PEDIGREE_SLOTS && p.box[nav->tree_sel].indi ? nav->tree_sel : 1;

    int first_gen, box_w, step;
    int y_origin = arrange(&p, sel, w, h, &first_gen, &box_w, &step);
    for (int n = 1; n < PEDIGREE_SLOTS; n++) {
        if (!pedigree_visible(&p, n)) continue;
        int x = 1 + ((pedigree_gen(n) - first_gen) * step);
        draw_links(win, &p, n, x + box_w, y_origin, 1, h - 2);
        draw_box(win, &p, n, x, y_origin + pedigree_row(&p, n), box_w, h - 2, n == sel);
    }
}

/* Dessine dans une fenêtre centrée les enfants (libellé et dates), défilante autour de la sélection. */
void treeview_draw_children(const Database *db, const Nav *nav) {
    const Node *children[FAMILY_MAX_CHILDREN];
    int count = person_children(db, nav_current(nav, db), children, FAMILY_MAX_CHILDREN);
    int w = COLS - 2 < 64 ? COLS - 2 : 64;
    int rows = count < LINES - 5 ? count : LINES - 5;
    if (count < 1 || rows < 1 || w < 10) return;
    int h = rows + 2;
    WINDOW *menu = newwin(h, w, (LINES - 1 - h) / 2, (COLS - w) / 2);
    if (!menu) return;

    int sel = nav->child_sel < count ? nav->child_sel : count - 1;
    int first = sel < rows ? 0 : sel - rows + 1;
    draw_frame(menu, T(S_T_CHILDREN));
    for (int i = 0; i < rows; i++) {
        char label[96], dates[48], line[sizeof label + sizeof dates + 2];
        person_summary(db, children[first + i], label, sizeof label, dates, sizeof dates);
        snprintf(line, sizeof line, "%s  %s", label, dates);
        if (first + i == sel) wattrset(menu, selected_attrs());
        mvwhline(menu, i + 1, 1, ' ', w - 2);
        mvwaddnstr(menu, i + 1, 1, line, w - 2);
        wattrset(menu, A_NORMAL);
    }
    wnoutrefresh(menu);
    delwin(menu);
}

/* Retourne la case dont la zone contient le clic gauche en attente. */
int treeview_pick(void) {
    MEVENT event;
    if (getmouse(&event) != OK || !(event.bstate & BUTTON1_PRESSED)) return 0;
    for (int n = 1; n < PEDIGREE_SLOTS; n++)
        if (tree_hits[n].h && event.y >= tree_hits[n].y && event.y < tree_hits[n].y + tree_hits[n].h &&
            event.x >= tree_hits[n].x && event.x < tree_hits[n].x + tree_hits[n].w)
            return n;
    return 0;
}
