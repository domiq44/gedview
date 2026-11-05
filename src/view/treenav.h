#ifndef TREENAV_H
#define TREENAV_H

/* Navigation dans l'arbre d'ascendance : ouverture, sélection d'une case et liste des enfants.
   L'état (tree, tree_sel, child_menu, child_sel) est porté par Nav ; l'individu courant reste celui de la
   liste. */

#include "core/database.h"
#include "view/nav.h"
#include "view/pedigree.h"

/* Affiche ou masque l'arbre de l'individu courant ; sans effet hors de la vue Individus. */
void treenav_toggle(Nav *nav, const Database *db);
/* Déplace la sélection de l'arbre ; elle reste inchangée si la case visée n'existe pas. Vers l'enfant depuis
   la case 1, ouvre la liste des enfants de l'individu s'il en a. */
void treenav_move(Nav *nav, const Database *db, PedigreeMove move);
/* Fait de la case n l'individu courant et l'enregistre dans l'historique ; l'arbre est recentré.
   Sans effet si la case n'existe pas. */
void treenav_recenter(Nav *nav, const Database *db, int n);
/* Déplace la sélection de la liste des enfants, bornée à ses extrémités. */
void treenav_menu_move(Nav *nav, const Database *db, int delta);
/* Ferme la liste des enfants sans changer d'individu. */
void treenav_menu_close(Nav *nav);
/* Fait de l'enfant sélectionné l'individu courant, enregistre l'historique et ferme la liste. */
void treenav_menu_choose(Nav *nav, const Database *db);

#endif
