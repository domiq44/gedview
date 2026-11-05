#ifndef NAV_H
#define NAV_H

/* État indépendant de l'écran pour les vues, sélections, recherches et retours arrière. */

#include <stddef.h>

#include "core/database.h"
#include "view/detail.h"

typedef struct NavHistoryEntry NavHistoryEntry;

/* État de navigation, indépendant de l'affichage. */
typedef struct {
    Kind kind;            /* liste affichée */
    int sel, top;         /* sélection et première ligne visible de la liste */
    int det_sel, det_top; /* idem pour le panneau de détail */
    int focus_detail;     /* 1 si le panneau de détail a le focus */
    int tree;             /* 1 si l'arbre d'ascendance remplace la liste et le détail (treenav) */
    int tree_sel;         /* numéro de la case sélectionnée de l'arbre (1 = individu) */
    int child_menu;       /* 1 si la liste des enfants de l'individu est ouverte */
    int child_sel;        /* enfant sélectionné dans cette liste */
    char search[128];     /* dernier terme de recherche */
    int saved_sel[K_COUNT], saved_top[K_COUNT]; /* positions des listes inactives */
    NavHistoryEntry *history;
    size_t history_count, history_cap;
} Nav;

/* Initialise toutes les sélections et l'historique à l'état de départ. */
void nav_init(Nav *nav);
/* Libère l'historique possédé par nav ; accepte un état déjà vide. */
void nav_destroy(Nav *nav);
/* Retourne le nombre d'enregistrements de la vue active. */
int nav_count(const Nav *nav, const Database *db);
/* Retourne l'enregistrement sélectionné, ou NULL si la vue est vide. */
const Node *nav_current(const Nav *nav, const Database *db);

/* Retourne l'enregistrement sélectionné dans la liste de kind, même si une autre vue est affichée, ou NULL si
 * vide. */
const Node *nav_record(const Nav *nav, const Database *db, Kind kind);
/* Sélectionne dans la liste de kind l'enregistrement d'XREF xref, sans historique ; retourne 0 s'il est
 * absent. */
int nav_restore_record(Nav *nav, const Database *db, Kind kind, const char *xref);
/* Affiche la liste de kind, sans historique. */
void nav_restore_view(Nav *nav, Kind kind);

/* Change de vue en conservant les positions et en empilant l'état précédent. */
void nav_set_kind(Nav *nav, Kind kind);
/* Bascule le panneau auquel s'appliquent les déplacements. */
void nav_toggle_focus(Nav *nav);

/* Sélectionne l'individu target dans la liste des individus et l'enregistre dans l'historique s'il change ;
   sans effet s'il est absent de la liste. */
void nav_select_individual(Nav *nav, const Database *db, const Node *target);

/* Déplace et borne la sélection active ; les déplacements de liste modifient l'historique. */
void nav_move(Nav *nav, const Database *db, const Detail *d, int delta);

/* Borne la sélection du détail et ajuste les deux fenêtres de défilement à rows lignes. */
void nav_scroll(Nav *nav, const Detail *d, int rows);

/* Suit le lien sélectionné, enregistre l'origine dans l'historique ; sans lien, donne le focus au détail. */
void nav_activate(Nav *nav, const Database *db, const Detail *d);

/* Restaure l'état de navigation précédent, si disponible. */
void nav_back(Nav *nav);

/* Cherche le prochain élément dont le libellé ou le XREF contient `needle` (insensible à la casse).
   Dans les vues Source et Citation, le XREF et le nom du dépôt (REPO) de la source comptent aussi. */
void nav_search(Nav *nav, const Database *db, const char *needle);

/* Réutilise le dernier terme et sélectionne le prochain résultat, si disponible. */
void nav_search_next(Nav *nav, const Database *db);

#endif
