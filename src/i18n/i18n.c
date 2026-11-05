/* Choix de la langue courante et accès aux textes (repli sur BASE_LANG). */
#include "i18n.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "catalog.h"
#include "langdir.h"

#define MAX_LANGS 32

static Catalog base, cur;
static char langdir[PATH_MAX + 8];
static char *codes[MAX_LANGS];
static int ncodes, cur_idx = -1;

/* Résout un code exact ou le préfixe langue d'une locale telle que fr_FR.UTF-8. */
static int index_from_code(const char *s) {
    for (int i = 0; i < ncodes; i++)
        if (!strncasecmp(s, codes[i], strlen(codes[i]))) return i;
    return -1;
}

/* Remplace le catalogue courant par celui de idx, en laissant le repli anglais séparé. */
static void select_lang(int idx) {
    cur_idx = idx;
    catalog_clear(&cur);
    if (idx >= 0 && strcmp(codes[idx], BASE_LANG) != 0) catalog_load(&cur, langdir, codes[idx]);
}

/* Réinitialise les catalogues, découvre les langues et choisit la langue initiale. */
void lang_init(const char *forced) {
    catalog_clear(&base);
    catalog_clear(&cur);
    for (int i = 0; i < ncodes; i++)
        free(codes[i]);
    ncodes = 0;
    cur_idx = -1;

    if (!langdir_find(langdir, sizeof langdir)) {
        fprintf(stderr, "gedview: translation directory (lang/) not found\n");
        return;
    }
    ncodes = langdir_list(langdir, codes, MAX_LANGS);
    catalog_load(&base, langdir, BASE_LANG);

    int idx = forced ? index_from_code(forced) : -1;
    if (idx < 0) {
        static const char *const vars[] = {"LC_ALL", "LC_MESSAGES", "LANG"};
        for (size_t i = 0; i < sizeof vars / sizeof *vars; i++) {
            const char *v = getenv(vars[i]);
            if (!v || !*v) continue;
            idx = index_from_code(v); /* la première variable définie fait foi */
            break;
        }
    }
    if (idx < 0) idx = index_from_code(BASE_LANG);
    select_lang(idx);
}

/* Sélectionne la prochaine langue trouvée, en revenant au début de la liste. */
void lang_cycle(void) {
    if (ncodes) select_lang((cur_idx + 1) % ncodes);
}

/* Résout un message dans la langue active puis dans le catalogue de base. */
const char *T(int id) {
    if (id == S_BLANK) return "";
    const char *s = catalog_msg(&cur, id);
    if (!s) s = catalog_msg(&base, id);
    return s ? s : catalog_key_name(id);
}

/* Résout un tag dans la langue active puis retourne le tag brut en dernier recours. */
const char *tag_label(const char *tag) {
    const char *s = catalog_tag(&cur, tag);
    if (!s) s = catalog_tag(&base, tag);
    return s ? s : tag;
}
