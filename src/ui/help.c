/* Affiche les messages d'aide du catalogue dans une vue ncurses temporaire. */
#define _XOPEN_SOURCE_EXTENDED 1
#include "help.h"

#include <ncurses.h>

#include "i18n/i18n.h"
#include "screen.h"

/* Indique quelles clés d'aide doivent recevoir le style de titre. */
static int is_heading(int id) {
    return id == S_H_VIEWS || id == S_H_NAV || id == S_H_SEARCHT || id == S_H_MISC;
}

/* Dessine la page d'aide, attend une touche et demande la reconstruction de l'écran principal. */
void help_show(void) {
    static const int ids[] = {
        S_H_TITLE, S_BLANK,   S_H_VIEWS,   S_H_I,      S_H_F,       S_H_S,       S_H_R,
        S_H_M,     S_H_N,     S_H_CITA,    S_H_NAV,    S_H_UPDOWN,  S_H_PAGE,    S_H_HOME,
        S_H_TAB,   S_H_ENTER, S_H_SEARCHT, S_H_SEARCH, S_H_SEARCHX, S_H_SEARCHR, S_H_MISC,
        S_H_HELP,  S_H_LANG,  S_H_COLOR,   S_H_QUIT,   S_H_HISTORY, S_H_BACK,
    };
    do {
        erase();
        for (size_t i = 0; i < sizeof ids / sizeof *ids && (int)i < LINES; i++) {
            int styled = i == 0 || is_heading(ids[i]);
            if (styled) attron(A_BOLD | (screen_colors_enabled() ? COLOR_PAIR(SCREEN_PAIR_TITLE) : 0));
            mvaddnstr((int)i, 1, T(ids[i]), COLS - 2);
            if (styled) attroff(A_BOLD | (screen_colors_enabled() ? COLOR_PAIR(SCREEN_PAIR_TITLE) : 0));
        }
        refresh();
    } while (getch() == KEY_RESIZE);
    screen_invalidate();
}
