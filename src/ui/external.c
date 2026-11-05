/* Résout et ouvre les cibles externes autorisées sans interprétation par un shell. */
#define _XOPEN_SOURCE 700
#include "external.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ui/external_target.h"

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
    char resolved[PATH_MAX];
    ExternalTargetType type = external_target_resolve(target, kind, gedcom_path, resolved, sizeof resolved);
    if (type == EXTERNAL_TARGET_URL) {
        launch(target);
        return;
    }
    if (type == EXTERNAL_TARGET_MEDIA) launch(resolved);
}