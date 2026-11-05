#ifndef CATALOG_H
#define CATALOG_H

/* Chargement et consultation d'un catalogue de messages et de libellés GEDCOM. */

#include "i18n.h"

#define MAX_TAGS 64

/* Messages possédés par le catalogue et libellés possédés des tags GEDCOM. */
typedef struct {
    char *msg[NSTR];
    struct {
        char tag[8];
        char *label;
    } tags[MAX_TAGS];
    int ntags;
} Catalog;

/* Charge dir/code.lang ; remplace les clés déjà présentes et conserve celles absentes du fichier. */
void catalog_load(Catalog *c, const char *dir, const char *code);
/* Libère toutes les chaînes du catalogue et réinitialise ses champs. */
void catalog_clear(Catalog *c);

/* Retourne le message d'id sans transfert de propriété, ou NULL si la clé est absente. */
const char *catalog_msg(const Catalog *c, int id);
/* Retourne le libellé traduit de tag sans transfert de propriété, ou NULL si absent. */
const char *catalog_tag(const Catalog *c, const char *tag);
/* Retourne le nom de clé correspondant à id dans les fichiers .lang. */
const char *catalog_key_name(int id);

#endif
