#ifndef DETAIL_H
#define DETAIL_H

/* Construction des lignes affichables et des liens associés à un enregistrement GEDCOM. */

#include <stddef.h>

#include "core/records.h"

typedef enum { LINE_LINK_RECORD, LINE_LINK_URL, LINE_LINK_MEDIA } LineLinkKind;

typedef struct {
    char text[512]; /* texte rendu, tronqué si nécessaire */
    char link[512]; /* XREF, URL ou chemin associé, vide si la ligne n'est pas activable */
    LineLinkKind link_kind;
    size_t link_start;  /* décalage en octets du texte visible correspondant au lien */
    size_t link_length; /* longueur en octets de ce texte visible */
} Line;

typedef struct {
    Line *lines;
    int count, cap;
} Detail;

/* Reconstruit les lignes de rec ; Detail possède son tableau et le réutilise si sa capacité suffit. */
void detail_build(Detail *d, const Database *db, Kind kind, const Node *rec);

/* Libère le tableau des lignes et remet Detail à zéro ; accepte NULL. */
void detail_destroy(Detail *d);

#endif
