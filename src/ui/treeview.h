#ifndef TREEVIEW_H
#define TREEVIEW_H

/* Dessin ncurses de l'arbre d'ascendance et de la liste des enfants, et test de clic sur les cases. */

#include <ncurses.h>

#include "core/database.h"
#include "view/nav.h"

/* Dessine dans win l'arbre de l'individu courant, centré, et mémorise les zones cliquables. */
void treeview_draw(WINDOW *win, const Database *db, const Nav *nav);

/* Dessine par-dessus l'écran la liste des enfants de l'individu courant, l'enfant sélectionné en évidence. */
void treeview_draw_children(const Database *db, const Nav *nav);

/* Lit le clic de souris en attente et retourne le numéro de la case d'arbre touchée, ou 0. */
int treeview_pick(void);

#endif
