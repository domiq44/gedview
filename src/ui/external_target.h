#ifndef EXTERNAL_TARGET_H
#define EXTERNAL_TARGET_H

/* Validation et résolution des cibles externes, sans lancer de programme. */

#include <stddef.h>

#include "view/detail.h"

typedef enum { EXTERNAL_TARGET_NONE, EXTERNAL_TARGET_URL, EXTERNAL_TARGET_MEDIA } ExternalTargetType;

/* Reconnaît une URL HTTP(S) ou résout un média autorisé dans resolved. */
ExternalTargetType external_target_resolve(const char *target, LineLinkKind kind, const char *gedcom_path,
                                           char *resolved, size_t resolved_size);

#endif