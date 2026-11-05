/* Gère l'état ncurses et dessine la liste, le détail, l'aide de statut et les invites. */
#define _XOPEN_SOURCE_EXTENDED 1
#include "screen.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "core/version.h"
#include "i18n/i18n.h"
#include "view/kindview.h"
#include "view/label.h"
#include "ui/treeview.h"

static WINDOW *list_win, *detail_win, *tree_win;

/* Largeur minimale du terminal pour afficher les secondes dans l'horloge. */
enum { CLOCK_SECONDS_MIN_COLS = 100 };
static int invalid = 1;
static int colors_available;
static int colors_enabled;

/* Configure les paires de couleurs disponibles et active le mode couleur si possible. */
static void init_colors(void) {
    colors_available = 0;
    colors_enabled = 0;
    if (!has_colors() || start_color() == ERR || COLORS < 8 || COLOR_PAIRS <= SCREEN_PAIR_SELECTED) return;
    short background = use_default_colors() == OK ? -1 : COLOR_BLACK;
    if (init_pair(SCREEN_PAIR_FRAME, COLOR_CYAN, background) == ERR ||
        init_pair(SCREEN_PAIR_TITLE, COLOR_GREEN, background) == ERR ||
        init_pair(SCREEN_PAIR_LINK, COLOR_YELLOW, background) == ERR ||
        init_pair(SCREEN_PAIR_STATUS, COLOR_WHITE, COLOR_BLUE) == ERR ||
        init_pair(SCREEN_PAIR_SELECTED, COLOR_BLACK, COLOR_CYAN) == ERR)
        return;
    colors_available = 1;
    colors_enabled = 1;
}

/* Initialise ncurses et active les options de terminal nécessaires à l'application. */
void screen_open(void) {
    initscr();
    init_colors();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    mousemask(BUTTON1_PRESSED, NULL);
    mouseinterval(0);
}

/* Détruit les fenêtres auxiliaires et remet leurs pointeurs à NULL. */
static void delete_windows(void) {
    if (list_win) delwin(list_win);
    if (detail_win) delwin(detail_win);
    if (tree_win) delwin(tree_win);
    list_win = detail_win = tree_win = NULL;
}

/* Libère les fenêtres puis restaure le terminal à son état normal. */
void screen_close(void) {
    delete_windows();
    endwin();
}

/* Retourne l'état courant du mode couleur. */
int screen_colors_enabled(void) { return colors_enabled; }

/* Inverse le mode couleur uniquement si l'initialisation a réussi. */
void screen_toggle_colors(void) {
    if (colors_available) colors_enabled = !colors_enabled;
}

/* Signale que les fenêtres doivent être recréées au prochain dessin. */
void screen_invalidate(void) { invalid = 1; }

/* Lit une touche avec temporisation pour permettre le rafraîchissement périodique de l'horloge. */
int screen_getkey(void) {
    timeout(1000); /* réveil chaque seconde pour rafraîchir l'horloge ; getch renvoie alors ERR */
    int c = getch();
    timeout(-1);
    if (c == KEY_RESIZE) invalid = 1;
    return c;
}

/* Calcule le nombre de lignes utiles d'un panneau pour le déplacement par page. */
int screen_page_size(void) { return LINES > 4 ? LINES - 3 : 1; }

/* Recrée les fenêtres pour la taille courante ou affiche le message de terminal trop petit. */
static int layout(void) {
    delete_windows();
    if (LINES < 4 || COLS < 9) {
        erase();
        if (colors_enabled) attron(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
        if (LINES > 0 && COLS > 0) mvaddnstr(0, 0, T(S_SMALLSCREEN), COLS);
        if (colors_enabled) attroff(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
        refresh();
        invalid = 1;
        return 0;
    }
    clear();
    list_win = newwin(LINES - 1, COLS / 3, 0, 0);
    detail_win = newwin(LINES - 1, COLS - (COLS / 3), 0, COLS / 3);
    tree_win = newwin(LINES - 1, COLS, 0, 0);
    if (!list_win || !detail_win || !tree_win) {
        delete_windows();
        erase();
        if (colors_enabled) attron(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
        mvaddnstr(0, 0, T(S_SMALLSCREEN), COLS);
        if (colors_enabled) attroff(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
        refresh();
        invalid = 1;
        return 0;
    }
    invalid = 0;
    return 1;
}

/* Dessine la liste active, sa sélection et son titre dans le panneau gauche. */
static void draw_list(const Database *db, const Nav *nav) {
    int h, w;
    getmaxyx(list_win, h, w);
    if (colors_enabled) wattron(list_win, COLOR_PAIR(SCREEN_PAIR_FRAME));
    box(list_win, 0, 0);
    if (colors_enabled) wattroff(list_win, COLOR_PAIR(SCREEN_PAIR_FRAME));
    if (colors_enabled) wattron(list_win, COLOR_PAIR(SCREEN_PAIR_TITLE) | A_BOLD);
    mvwprintw(list_win, 0, 2, T(kind_title_id(nav->kind)), nav_count(nav, db));
    if (colors_enabled) wattroff(list_win, COLOR_PAIR(SCREEN_PAIR_TITLE) | A_BOLD);

    int rows = h - 2, count = nav_count(nav, db);
    for (int i = 0; i < rows && nav->top + i < count; i++) {
        char name[256];
        display_label(db, nav->kind, db->lists[nav->kind].items[nav->top + i], name, sizeof name);
        int selected = nav->top + i == nav->sel;
        if (selected) {
            if (colors_enabled)
                wattron(list_win,
                        COLOR_PAIR(SCREEN_PAIR_SELECTED) | A_BOLD | (nav->focus_detail ? A_UNDERLINE : 0));
            else
                wattron(list_win, nav->focus_detail ? A_UNDERLINE : A_REVERSE);
        }
        mvwhline(list_win, i + 1, 1, ' ', w - 2);
        mvwaddnstr(list_win, i + 1, 1, name, w - 2); /* ncursesw gère l'UTF-8 */
        if (selected) wattrset(list_win, A_NORMAL);
    }
}

/* Dessine les lignes du détail en appliquant les styles de sélection et de lien. */
static void draw_detail(const Nav *nav, const Detail *d) {
    int h, w;
    getmaxyx(detail_win, h, w);
    if (colors_enabled) wattron(detail_win, COLOR_PAIR(SCREEN_PAIR_FRAME));
    box(detail_win, 0, 0);
    if (colors_enabled) wattroff(detail_win, COLOR_PAIR(SCREEN_PAIR_FRAME));
    if (colors_enabled) wattron(detail_win, COLOR_PAIR(SCREEN_PAIR_TITLE) | A_BOLD);
    mvwprintw(detail_win, 0, 2, "%s", T(S_DETAIL));
    if (colors_enabled) wattroff(detail_win, COLOR_PAIR(SCREEN_PAIR_TITLE) | A_BOLD);

    char tag[64];
    snprintf(tag, sizeof tag, " gedview %s ", app_version());
    int tag_len = (int)strlen(tag);
    if (w >= tag_len + (int)strlen(T(S_DETAIL)) + 6) {
        if (colors_enabled) wattron(detail_win, COLOR_PAIR(SCREEN_PAIR_FRAME));
        mvwaddstr(detail_win, 0, w - tag_len - 2, tag);
        if (colors_enabled) wattroff(detail_win, COLOR_PAIR(SCREEN_PAIR_FRAME));
    }

    int rows = h - 2;
    for (int i = 0; i < rows && nav->det_top + i < d->count; i++) {
        const Line *l = &d->lines[nav->det_top + i];
        int attrs = 0;
        int selected = nav->focus_detail && nav->det_top + i == nav->det_sel;
        int is_link = l->link[0] != 0;
        if (nav->det_top + i == 0) attrs |= A_BOLD;
        if (colors_enabled) {
            /* Une seule paire par ligne : OR-er deux COLOR_PAIR produit un numéro de paire invalide. */
            if (selected)
                attrs |= COLOR_PAIR(SCREEN_PAIR_SELECTED) | A_BOLD;
            else if (is_link)
                attrs |= COLOR_PAIR(SCREEN_PAIR_LINK);
            else if (nav->det_top + i == 0)
                attrs |= COLOR_PAIR(SCREEN_PAIR_TITLE);
        } else if (selected) {
            attrs |= A_REVERSE;
        }
        wattron(detail_win, attrs);
        if (selected) mvwhline(detail_win, i + 1, 1, ' ', w - 2);
        wmove(detail_win, i + 1, 1);
        size_t text_len = strlen(l->text);
        if (text_len > (size_t)(w - 2)) text_len = (size_t)(w - 2);
        if (is_link && l->link_start < text_len && l->link_length) {
            size_t link_len = l->link_length;
            if (link_len > text_len - l->link_start) link_len = text_len - l->link_start;
            waddnstr(detail_win, l->text, (int)l->link_start);
            wattron(detail_win, A_UNDERLINE);
            waddnstr(detail_win, l->text + l->link_start, (int)link_len);
            wattroff(detail_win, A_UNDERLINE);
            waddnstr(detail_win, l->text + l->link_start + link_len,
                     (int)(text_len - l->link_start - link_len));
        } else {
            waddnstr(detail_win, l->text, (int)text_len);
        }
        wattrset(detail_win, A_NORMAL);
    }
}

/* Dessine l'aide rapide et l'heure dans la barre inférieure. */
static void draw_status(int tree) {
    char clock[32] = "";
    time_t now = time(NULL);
    const struct tm *tm = localtime(&now);
    const char *format = COLS >= CLOCK_SECONDS_MIN_COLS ? " %Y-%m-%d %H:%M:%S " : " %Y-%m-%d %H:%M ";
    if (tm && !strftime(clock, sizeof clock, format, tm)) clock[0] = 0;
    int clock_len = (int)strlen(clock);
    if (clock_len >= COLS) clock_len = 0;

    if (colors_enabled) attron(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
    mvhline(LINES - 1, 0, ' ', COLS);
    mvaddnstr(LINES - 1, 0, T(tree ? S_STATUS_TREE : S_STATUS), COLS - clock_len);
    if (clock_len) mvaddstr(LINES - 1, COLS - clock_len, clock);
    if (colors_enabled) attroff(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
}

/* Met à jour les panneaux et compose leurs fenêtres avec ncurses. */
void screen_draw(const Database *db, const Nav *nav, const Detail *d) {
    if (invalid && !layout()) return;
    draw_status(nav->tree);
    wnoutrefresh(stdscr); /* avant les fenêtres : après clear(), il les effacerait */
    if (nav->tree) {
        werase(tree_win);
        treeview_draw(tree_win, db, nav);
        wnoutrefresh(tree_win);
        if (nav->menu) treeview_draw_menu(db, nav);
        doupdate();
        return;
    }
    werase(list_win);
    werase(detail_win);
    draw_list(db, nav);
    draw_detail(nav, d);
    wnoutrefresh(list_win);
    wnoutrefresh(detail_win);
    doupdate();
}

/* Saisit une recherche en bas du terminal jusqu'à validation ou annulation. */
int screen_prompt(char *buf, size_t sz) {
    if (!buf || !sz || LINES < 2) return 0;
    size_t len = 0;
    buf[0] = 0;
    for (;;) {
        if (colors_enabled) attron(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
        move(LINES - 1, 0);
        clrtoeol();
        printw(T(S_SEARCH), buf);
        if (colors_enabled) attroff(COLOR_PAIR(SCREEN_PAIR_STATUS) | A_BOLD);
        refresh();
        int c = getch();
        if (c == 27) return 0;
        if (c == '\n' || c == KEY_ENTER) return 1;
        if ((c == KEY_BACKSPACE || c == 127 || c == 8) && len) {
            len--;
            while (len && ((unsigned char)buf[len] & 0xC0) == 0x80)
                len--;
            buf[len] = 0;
        } else if (c >= 32 && c < 256 && len + 1 < sz) {
            buf[len++] = (char)c;
            buf[len] = 0;
        }
    }
}

/* Affiche un message modal et recommence son placement après un redimensionnement. */
void screen_notice(const char *message) {
    for (;;) {
        erase();
        if (message && LINES > 0 && COLS > 0) {
            size_t len = strlen(message);
            int x = len < (size_t)COLS ? (COLS - (int)len) / 2 : 0;
            mvaddnstr(LINES / 2, x, message, COLS - x);
        }
        refresh();
        if (getch() != KEY_RESIZE) break;
    }
    invalid = 1;
}
