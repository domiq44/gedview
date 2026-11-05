#ifndef I18N_H
#define I18N_H

/* Sélection de la langue courante et accès aux messages traduits avec repli anglais. */

enum { L_EN, L_FR, L_ES, L_DE, L_IT, L_PT, NLANG };

enum {
    S_NONAME,
    S_T_INDI,
    S_T_FAM,
    S_T_SRC,
    S_T_REPO,
    S_T_OBJ,
    S_T_NOTE,
    S_T_CITA,
    S_DETAIL,
    S_SEARCH,
    S_STATUS,
    S_SMALLSCREEN,
    S_USAGE,
    S_NOINDI,
    S_PARSE_WARNINGS,
    S_BLANK,
    S_H_TITLE,
    S_H_VIEWS,
    S_H_I,
    S_H_F,
    S_H_S,
    S_H_R,
    S_H_M,
    S_H_N,
    S_H_CITA,
    S_H_NAV,
    S_H_UPDOWN,
    S_H_PAGE,
    S_H_HOME,
    S_H_TAB,
    S_H_ENTER,
    S_H_SEARCHT,
    S_H_SEARCH,
    S_H_SEARCHX,
    S_H_SEARCHR,
    S_H_MISC,
    S_H_HELP,
    S_H_LANG,
    S_H_QUIT,
    S_H_BACK,
    S_H_HISTORY,
    S_H_COLOR,
    NSTR,
};

/* Charge les catalogues et choisit forced, sinon la première variable locale définie, puis l'anglais. */
void lang_init(const char *forced);
/* Passe à la langue suivante parmi celles découvertes, avec retour cyclique au début. */
void lang_cycle(void);
/* Retourne le message traduit, puis le repli anglais, puis le nom de clé ; S_BLANK vaut une chaîne vide. */
const char *T(int id);
/* Retourne le libellé traduit du tag, son libellé anglais ou le tag lui-même. */
const char *tag_label(const char *tag);

#endif
