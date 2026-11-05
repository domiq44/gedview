/* Point d'entrée gedview [-l CODE] fichier.ged : initialise, exécute la boucle et libère les ressources. */
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "core/gedcom.h"
#include "core/database.h"
#include "i18n/i18n.h"
#include "ui/external.h"
#include "ui/help.h"
#include "ui/input.h"
#include "ui/screen.h"
#include "view/detail.h"
#include "view/nav.h"

/* Valide les arguments puis orchestre la durée de vie complète d'une session GedView. */
int main(int argc, char **argv) {
    const char *forced = NULL;
    if (argc == 4 && !strcmp(argv[1], "-l")) {
        forced = argv[2];
        argv += 2;
        argc -= 2;
    }
    lang_init(forced);
    if (argc != 2) {
        fprintf(stderr, T(S_USAGE), argv[0]);
        return 1;
    }
    setlocale(LC_ALL, "");

    const char *gedcom_path = argv[1];
    size_t ignored_lines = 0;
    Node *root = gedcom_parse_with_warnings(gedcom_path, &ignored_lines);
    if (!root) return 1;
    Database db;
    if (!db_build(&db, root)) {
        fputs("gedview: duplicate record XREF\n", stderr);
        db_destroy(&db);
        gedcom_free(root);
        return 1;
    }
    if (!db.lists[K_INDI].count) {
        fputs(T(S_NOINDI), stderr);
        db_destroy(&db);
        gedcom_free(root);
        return 1;
    }

    Nav nav;
    nav_init(&nav);
    Detail detail = {0};
    const InputHooks hooks = {
        screen_page_size, screen_prompt, help_show, external_open, gedcom_path, screen_toggle_colors,
    };

    screen_open();
    if (ignored_lines) {
        char count[32], message[256];
        snprintf(count, sizeof count, "%zu", ignored_lines);
        snprintf(message, sizeof message, T(S_PARSE_WARNINGS), count);
        screen_notice(message);
    }
    do {
        const Node *cur = nav_current(&nav, &db);
        if (cur)
            detail_build(&detail, &db, nav.kind, cur);
        else
            detail.count = 0;
        nav_scroll(&nav, &detail, screen_page_size());
        screen_draw(&db, &nav, &detail);
    } while (input_handle_key(screen_getkey(), &db, &nav, &detail, &hooks));
    screen_close();
    nav_destroy(&nav);
    detail_destroy(&detail);
    db_destroy(&db);
    gedcom_free(root);
    return 0;
}
