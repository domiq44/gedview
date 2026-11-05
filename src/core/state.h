#ifndef STATE_H
#define STATE_H

/* Mémorisation, entre deux sessions, de valeurs nommées (« slots ») propres à chaque fichier GEDCOM. */

#include <stddef.h>

typedef struct {
    const char *slot;  /* nom de la valeur, par exemple INDI */
    const char *value; /* valeur mémorisée, par exemple un XREF */
} StateEntry;

/* Retrouve dans value la valeur mémorisée du slot pour gedcom_path ; retourne 0 si rien n'est mémorisé,
   si la mémoire est illisible ou si value est trop petit. Le dossier est GEDVIEW_STATE_DIR, sinon
   $XDG_STATE_HOME/gedview, sinon ~/.local/state/gedview. */
int state_load(const char *gedcom_path, const char *slot, char *value, size_t sz);

/* Remplace toutes les valeurs mémorisées pour gedcom_path par entries, sans toucher aux autres fichiers ;
   retourne 0 en cas d'échec (dossier introuvable ou non créable, écriture impossible, slot ou valeur vide ou
   contenant une tabulation ou un saut de ligne, chemin contenant une tabulation ou un saut de ligne). */
int state_save(const char *gedcom_path, const StateEntry *entries, int count);

#endif
