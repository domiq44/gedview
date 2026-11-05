/* Traduit les codes de touches ncurses en commandes de navigation via InputHooks. */
#define _XOPEN_SOURCE_EXTENDED 1
#include "input.h"

#include <ncurses.h>

#include "i18n/i18n.h"
#include "view/kindview.h"
#include "view/treenav.h"

#define FAR (1 << 30) /* delta de déplacement pour aller au début/à la fin */

/* Applique les touches de la liste ouverte (enfants ou proches) ; retourne 1 si la touche est consommée. */
static int handle_menu_key(int key, const Database *db, Nav *nav) {
    switch (key) {
    case KEY_UP:
    case 'k':
        treenav_menu_move(nav, db, -1);
        return 1;
    case KEY_DOWN:
    case 'j':
        treenav_menu_move(nav, db, 1);
        return 1;
    case '\n':
    case KEY_ENTER:
        treenav_menu_choose(nav, db);
        return 1;
    case 27:
    case KEY_RIGHT:
        treenav_menu_close(nav);
        return 1;
    case KEY_LEFT:
    case KEY_MOUSE:
        return 1;
    default:
        return 0;
    }
}

/* Applique les touches propres à l'arbre d'ascendance ; retourne 1 si la touche est consommée. */
static int handle_tree_key(int key, const Database *db, Nav *nav, const InputHooks *hooks) {
    if (nav->menu && handle_menu_key(key, db, nav)) return 1;
    switch (key) {
    case 'p':
    case 'P':
        treenav_open_relatives(nav, db);
        return 1;
    case KEY_LEFT:
        treenav_move(nav, db, PED_CHILD);
        return 1;
    case KEY_RIGHT:
        treenav_move(nav, db, PED_PARENT);
        return 1;
    case KEY_UP:
    case 'k':
        treenav_move(nav, db, PED_PREV);
        return 1;
    case KEY_DOWN:
    case 'j':
        treenav_move(nav, db, PED_NEXT);
        return 1;
    case '\n':
    case KEY_ENTER:
        treenav_recenter(nav, db, nav->tree_sel);
        return 1;
    case KEY_MOUSE:
        if (hooks->mouse_pick) treenav_recenter(nav, db, hooks->mouse_pick());
        return 1;
    case '\t':
    case KEY_PPAGE:
    case KEY_NPAGE:
    case KEY_HOME:
    case KEY_END:
    case KEY_F(3):
    case 'g':
    case 'G':
    case '/':
        return 1; /* actions de liste sans objet dans l'arbre */
    default:
        return 0;
    }
}

/* Applique une touche à l'état de navigation ou délègue une action à l'interface appelante. */
int input_handle_key(int key, const Database *db, Nav *nav, const Detail *d, const InputHooks *hooks) {
    if (nav->tree && handle_tree_key(key, db, nav, hooks)) return 1;
    int kind = kind_from_key(key);
    if (kind >= 0) {
        nav_set_kind(nav, (Kind)kind);
        return 1;
    }
    switch (key) {
    case 't':
    case 'T':
        treenav_toggle(nav, db);
        break;
    case 'q':
    case 'Q':
        return 0;
    case '\t':
        nav_toggle_focus(nav);
        break;
    case KEY_UP:
    case 'k':
        nav_move(nav, db, d, -1);
        break;
    case KEY_DOWN:
    case 'j':
        nav_move(nav, db, d, 1);
        break;
    case KEY_PPAGE:
        nav_move(nav, db, d, -hooks->page_size());
        break;
    case KEY_NPAGE:
        nav_move(nav, db, d, hooks->page_size());
        break;
    case KEY_HOME:
        nav_move(nav, db, d, -FAR);
        break;
    case KEY_END:
        nav_move(nav, db, d, FAR);
        break;
    case KEY_F(3):
    case 'g': /* F3 est souvent interceptée par l'éditeur ou le terminal */
    case 'G':
        nav_search_next(nav, db);
        break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
        nav_back(nav);
        break;
    case '\n':
    case KEY_ENTER:
        if (nav->focus_detail && nav->det_sel >= 0 && nav->det_sel < d->count &&
            d->lines[nav->det_sel].link[0] && d->lines[nav->det_sel].link_kind != LINE_LINK_RECORD) {
            if (hooks->open_external)
                hooks->open_external(d->lines[nav->det_sel].link, d->lines[nav->det_sel].link_kind,
                                     hooks->gedcom_path);
        } else {
            nav_activate(nav, db, d);
        }
        break;
    case 'l':
    case 'L':
        lang_cycle();
        break;
    case 'v':
    case 'V':
        if (hooks->toggle_colors) hooks->toggle_colors();
        break;
    case 'h':
    case 'H':
    case KEY_F(1):
        hooks->help();
        break;
    case '/': {
        char needle[128];
        if (hooks->prompt(needle, sizeof needle)) nav_search(nav, db, needle);
        break;
    }
    default:
        break;
    }
    return 1;
}
