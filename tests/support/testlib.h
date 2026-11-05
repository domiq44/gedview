#ifndef TESTLIB_H
#define TESTLIB_H

/* Assertions et exécution des suites du framework de tests minimal du dépôt. */

/* Mini framework de tests unitaires : une suite par exécutable, un TEST par scénario.
 *
 *   TEST(nom_du_scenario) { // Given ... // When ... // Then ASSERT_...  }
 *   int main(void) { tl_suite("module"); RUN_TEST(nom_du_scenario); return tl_finish(); }
 *
 * La première assertion en échec interrompt le test courant ; les suivants sont exécutés.
 */

typedef void (*TestFn)(void);

/* Initialise les compteurs d'une suite et affiche son nom. */
void tl_suite(const char *name);
/* Exécute fn sous le nom name et consigne son succès ou son premier échec. */
void tl_run(const char *name, TestFn fn);
/* Affiche le bilan de la suite et retourne un code non nul si un test a échoué. */
int tl_finish(void);

/* Signale un échec formaté et interrompt le test courant par longjmp. */
void tl_fail(const char *file, int line, const char *fmt, ...)
    __attribute__((noreturn, format(printf, 3, 4)));
/* Vérifie une égalité entière et signale les opérandes avec leur emplacement source en cas d'écart. */
void tl_check_int(const char *file, int line, const char *expr, long long expected, long long actual);
/* Vérifie l'égalité de deux chaînes, NULL étant comparé explicitement. */
void tl_check_str(const char *file, int line, const char *expr, const char *expected, const char *actual);

#define TEST(name) static void name(void)
#define RUN_TEST(name) tl_run(#name, name)

#define ASSERT_TRUE(cond)                                                                                    \
    do {                                                                                                     \
        if (!(cond)) tl_fail(__FILE__, __LINE__, "expected true: %s", #cond);                                \
    } while (0)

#define ASSERT_FALSE(cond)                                                                                   \
    do {                                                                                                     \
        if (cond) tl_fail(__FILE__, __LINE__, "expected false: %s", #cond);                                  \
    } while (0)

#define ASSERT_NULL(ptr)                                                                                     \
    do {                                                                                                     \
        if ((ptr) != NULL) tl_fail(__FILE__, __LINE__, "expected NULL: %s", #ptr);                           \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                                                 \
    do {                                                                                                     \
        if ((ptr) == NULL) tl_fail(__FILE__, __LINE__, "expected non-NULL: %s", #ptr);                       \
    } while (0)

#define ASSERT_EQ_INT(expected, actual) tl_check_int(__FILE__, __LINE__, #actual, (expected), (actual))
#define ASSERT_EQ_STR(expected, actual) tl_check_str(__FILE__, __LINE__, #actual, (expected), (actual))

#endif
