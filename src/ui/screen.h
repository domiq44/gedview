#ifndef SCREEN_H
#define SCREEN_H

/* Interface ncurses : fenêtres, dessin, saisie et adaptation au terminal. */

#include <stddef.h>

#include "core/database.h"
#include "view/detail.h"
#include "view/nav.h"

enum {
    SCREEN_PAIR_FRAME = 1,
    SCREEN_PAIR_TITLE,
    SCREEN_PAIR_LINK,
    SCREEN_PAIR_STATUS,
    SCREEN_PAIR_SELECTED,
};

/* Initialise ncurses, les couleurs et les options de saisie du terminal. */
void screen_open(void);
/* Supprime les fenêtres et restaure le terminal avant de quitter ncurses. */
void screen_close(void);
/* Indique si l'affichage couleur est actuellement activé. */
int screen_colors_enabled(void);
/* Inverse l'affichage couleur si le terminal sait gérer les couleurs. */
void screen_toggle_colors(void);

/* Force la recréation des fenêtres au prochain dessin (redimensionnement, retour de l'aide). */
void screen_invalidate(void);

/* Dessine les données de navigation courantes dans les fenêtres de liste et de détail. */
void screen_draw(const Database *db, const Nav *nav, const Detail *d);

/* Attend une touche avec réveil périodique pour l'horloge ; invalide le layout sur KEY_RESIZE. */
int screen_getkey(void);

/* Retourne le nombre de lignes de contenu visibles dans un panneau. */
int screen_page_size(void);

/* Saisit au plus sz-1 octets dans buf ; retourne 1 sur Entrée, 0 sur annulation ou arguments invalides. */
int screen_prompt(char *buf, size_t sz);

/* Affiche un message bloquant jusqu'à la prochaine touche ou un redimensionnement. */
void screen_notice(const char *message);

#endif
