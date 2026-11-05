#ifndef INPUT_H
#define INPUT_H

/* Adaptation des touches clavier aux actions de navigation et aux services d'interface injectés. */

#include <stddef.h>

#include "core/database.h"
#include "view/detail.h"
#include "view/nav.h"

/* Services d'interface injectés pour séparer le routage des touches de ncurses et faciliter les tests. */
typedef struct {
    int (*page_size)(void);              /* lignes visibles dans un panneau */
    int (*prompt)(char *buf, size_t sz); /* saisie d'un texte ; 1 si validé, 0 si annulé */
    void (*help)(void);                  /* affiche l'aide */
    void (*open_external)(const char *target, LineLinkKind kind,
                          const char *gedcom_path); /* ouvre une URL ou un média autorisé */
    const char *gedcom_path;                        /* chemin utilisé pour résoudre les médias relatifs */
    void (*toggle_colors)(void);                    /* inverse l'affichage couleur si disponible */
} InputHooks;

/* Applique l'action associée à key ; retourne 0 pour demander la fermeture, 1 pour poursuivre. */
int input_handle_key(int key, const Database *db, Nav *nav, const Detail *d, const InputHooks *hooks);

#endif
