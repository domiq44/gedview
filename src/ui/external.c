/* Résout et ouvre les cibles externes autorisées sans interprétation par un shell. */
#define _XOPEN_SOURCE 700
#include "external.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
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

/* Détache xdg-open du terminal tout en récoltant le premier processus enfant. */
static void launch(const char *target) {
    pid_t child = fork();
    if (child < 0) return;
    if (child == 0) {
        pid_t opener = fork();
        if (opener < 0) _exit(127);
        if (opener > 0) _exit(0);

        (void)setsid();
        int null_fd = open("/dev/null", O_RDWR);
        if (null_fd >= 0) {
            for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; fd++)
                (void)dup2(null_fd, fd);
            if (null_fd > STDERR_FILENO) close(null_fd);
        }
        execlp("xdg-open", "xdg-open", target, (char *)NULL);
        _exit(127);
    }

    int status;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
}

/* Filtre les cibles selon leur type puis démarre xdg-open pour les URL ou médias valides. */
void external_open(const char *target, LineLinkKind kind, const char *gedcom_path) {
    if (!target || !*target) return;
    if (is_http_url(target)) {
        launch(target);
        return;
    }
    if (kind != LINE_LINK_MEDIA || !is_supported_media(target)) return;

    char resolved[PATH_MAX];
    if (resolve_media_path(target, gedcom_path, resolved)) launch(resolved);
}