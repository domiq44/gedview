/* Valide les URL et résout les chemins média sans effectuer d'ouverture externe. */
#define _XOPEN_SOURCE 700
#include "external_target.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* Reconnaît uniquement les schémas HTTP et HTTPS, sans tenir compte de la casse. */
static int is_http_url(const char *target) {
    return !strncasecmp(target, "http://", 7) || !strncasecmp(target, "https://", 8);
}

/* Vérifie que le suffixe du chemin appartient aux formats média pris en charge. */
static int is_supported_media(const char *path) {
    static const char *const extensions[] = {
        ".pdf", ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tif", ".tiff", ".webp", ".svg",
    };
    const char *slash = strrchr(path, '/');
    const char *dot = strrchr(path, '.');
    if (!dot || (slash && dot < slash)) return 0;
    for (size_t i = 0; i < sizeof extensions / sizeof *extensions; i++)
        if (!strcasecmp(dot, extensions[i])) return 1;
    return 0;
}

/* Résout un média absolu ou relatif au GEDCOM et échoue si le chemin ne peut être canonisé. */
static int resolve_media_path(const char *path, const char *gedcom_path, char resolved[PATH_MAX]) {
    if (path[0] == '/') return realpath(path, resolved) != NULL;
    if (!gedcom_path) return 0;

    char gedcom[PATH_MAX];
    if (!realpath(gedcom_path, gedcom)) return 0;
    char *slash = strrchr(gedcom, '/');
    if (!slash) return 0;
    slash[1] = 0;

    char candidate[PATH_MAX];
    int written = snprintf(candidate, sizeof candidate, "%s%s", gedcom, path);
    return written >= 0 && (size_t)written < sizeof candidate && realpath(candidate, resolved) != NULL;
}

/* Classe une URL ou valide et canonise un média sans ouvrir la cible. */
ExternalTargetType external_target_resolve(const char *target, LineLinkKind kind, const char *gedcom_path,
                                           char *resolved, size_t resolved_size) {
    if (resolved && resolved_size) resolved[0] = 0;
    if (!target || !*target) return EXTERNAL_TARGET_NONE;
    if (is_http_url(target)) return EXTERNAL_TARGET_URL;
    if (kind != LINE_LINK_MEDIA || !is_supported_media(target) || !resolved || !resolved_size)
        return EXTERNAL_TARGET_NONE;

    char canonical[PATH_MAX];
    if (!resolve_media_path(target, gedcom_path, canonical)) return EXTERNAL_TARGET_NONE;
    int written = snprintf(resolved, resolved_size, "%s", canonical);
    if (written < 0 || (size_t)written >= resolved_size) {
        resolved[0] = 0;
        return EXTERNAL_TARGET_NONE;
    }
    return EXTERNAL_TARGET_MEDIA;
}