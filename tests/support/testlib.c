/* Exécute les tests séquentiellement et utilise setjmp pour isoler un échec d'assertion. */
#include "testlib.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *suite_name = "";
static int passed, failed;
static jmp_buf test_env;
static char failure[512];

/* Démarre une nouvelle suite et remet ses compteurs à zéro. */
void tl_suite(const char *name) {
    suite_name = name;
    passed = failed = 0;
    printf("%s\n", name);
}

/* Exécute un scénario et transforme le premier tl_fail en résultat d'échec de ce scénario. */
void tl_run(const char *name, TestFn fn) {
    if (setjmp(test_env) == 0) {
        fn();
        passed++;
        printf("  ok    %s\n", name);
    } else {
        failed++;
        printf("  FAIL  %s\n        %s\n", name, failure);
    }
    fflush(stdout);
}

/* Affiche les totaux de la suite et retourne le statut attendu par make test. */
int tl_finish(void) {
    printf("  => %s: %d passed, %d failed\n\n", suite_name, passed, failed);
    return failed ? 1 : 0;
}

/* Formate l'échec courant puis revient au pilote du test au moyen de longjmp. */
void tl_fail(const char *file, int line, const char *fmt, ...) {
    char msg[384];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    snprintf(failure, sizeof failure, "%s:%d: %s", file, line, msg);
    longjmp(test_env, 1);
}

/* Compare deux entiers et délègue le signalement au mécanisme commun en cas d'écart. */
void tl_check_int(const char *file, int line, const char *expr, long long expected, long long actual) {
    if (expected != actual) tl_fail(file, line, "%s: expected %lld, got %lld", expr, expected, actual);
}

/* Fournit une représentation textuelle non NULL pour les diagnostics d'assertion. */
static const char *show(const char *s) { return s ? s : "(null)"; }

/* Compare deux chaînes en tenant compte des pointeurs NULL et signale toute différence. */
void tl_check_str(const char *file, int line, const char *expr, const char *expected, const char *actual) {
    int same = (expected == NULL && actual == NULL) || (expected && actual && !strcmp(expected, actual));
    if (!same) tl_fail(file, line, "%s: expected \"%s\", got \"%s\"", expr, show(expected), show(actual));
}
