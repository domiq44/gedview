CC = gcc
.DEFAULT_GOAL := all
CFLAGS = -Wall -Wextra -O2 -Isrc -MMD -MP
LDLIBS = -lncursesw

# Dossier des fichiers générés ; `make coverage` le remplace pour ne pas mélanger les objets instrumentés.
BUILD ?= build
COV_DIR = build-cov
SAN_DIR = build-asan

# Installation utilisateur (sans root) : programme, traductions et lanceur dans APPDIR, entrée dans DESKTOP_DIR.
DATA_HOME ?= $(or $(XDG_DATA_HOME),$(HOME)/.local/share)
APPDIR ?= $(DATA_HOME)/gedview
DESKTOP_DIR ?= $(DATA_HOME)/applications

ifdef COVERAGE
CFLAGS += --coverage
TEST_CFLAGS += --coverage
LDFLAGS += --coverage
endif

SRC = $(wildcard src/*.c src/*/*.c)
OBJ = $(SRC:src/%.c=$(BUILD)/%.o)
HDR = $(wildcard src/*.h src/*/*.h)

TEST_SRC = $(wildcard tests/*/test_*.c)
TEST_BIN = $(TEST_SRC:tests/%.c=$(BUILD)/tests/%)
TEST_SUPPORT_SRC = $(wildcard tests/support/*.c)
TEST_SUPPORT_OBJ = $(TEST_SUPPORT_SRC:tests/%.c=$(BUILD)/tests/%.o)
TEST_FILES = $(TEST_SRC) $(TEST_SUPPORT_SRC) $(wildcard tests/*/*.h)
# Sans main ni code qui appelle ncurses : les tests ne lient pas ncurses.
UNTESTED_SRC = src/main.c src/ui/screen.c src/ui/treeview.c src/ui/help.c src/ui/external.c
LIB_OBJ = $(filter-out $(UNTESTED_SRC:src/%.c=$(BUILD)/%.o),$(OBJ))
TEST_CFLAGS += -Wall -Wextra -O0 -g -Isrc -Itests -MMD -MP
ifdef SANITIZE
SANITIZER_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS += $(SANITIZER_FLAGS) -O1 -g
TEST_CFLAGS += $(SANITIZER_FLAGS) -O1 -g
LDFLAGS += $(SANITIZER_FLAGS)
endif

.PHONY: all clean distclean lint cppcheck format format-check test coverage sanitize check-tools check-compiler FORCE \
	install-desktop uninstall-desktop \
	check-ncurses check-cppcheck check-clang-format check-clang-tidy check-gcov

# $(call require,commande,paquet Fedora,paquet Debian) : échoue avec un message d'installation si la commande manque.
require = @command -v $(1) >/dev/null 2>&1 || { echo "Erreur : '$(1)' est introuvable. Fedora : sudo dnf install $(2) | Debian/Ubuntu : sudo apt install $(3)" >&2; exit 1; }

check-compiler:
	$(call require,$(CC),gcc,gcc)

check-ncurses: check-compiler
	@tmp=$$(mktemp) && { printf '#include <ncurses.h>\nint main(void) { return 0; }\n' | $(CC) -x c - -lncursesw -o $$tmp 2>/dev/null; r=$$?; rm -f $$tmp; [ $$r -eq 0 ]; } || { echo "Erreur : ncursesw (en-têtes et bibliothèque) est introuvable. Fedora : sudo dnf install ncurses-devel | Debian/Ubuntu : sudo apt install libncurses-dev" >&2; exit 1; }

check-cppcheck:
	$(call require,cppcheck,cppcheck,cppcheck)

check-clang-format:
	$(call require,clang-format,clang-tools-extra,clang-format)

check-clang-tidy:
	$(call require,clang-tidy,clang-tools-extra,clang-tidy)

check-gcov:
	$(call require,gcov,gcc,gcc)

# Liste l'état de tous les outils (compilation, tests, couverture, lint, format) et échoue s'il en manque.
check-tools:
	@missing=0; for t in $(CC) gcov cppcheck clang-format clang-tidy; do \
		if command -v $$t >/dev/null 2>&1; then echo "ok       $$t"; else echo "manquant $$t"; missing=1; fi; \
	done; \
	if $(MAKE) --no-print-directory check-ncurses >/dev/null 2>&1; then echo "ok       ncursesw"; else echo "manquant ncursesw"; missing=1; fi; \
	[ $$missing -eq 0 ] || { echo "Installation : Fedora : sudo dnf install gcc make ncurses-devel cppcheck clang-tools-extra | Debian/Ubuntu : sudo apt install gcc make libncurses-dev cppcheck clang-format clang-tidy" >&2; exit 1; }

all: gedview

gedview: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD)/%.o: src/%.c | check-compiler
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Version git (tag, ou hash court, suffixe -dirty si modifications non commitées) ; "dev" hors dépôt git.
VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
$(BUILD)/core/version.o: CFLAGS += -DGEDVIEW_VERSION='"$(VERSION)"'
$(BUILD)/core/version.o: $(BUILD)/version.stamp

# Ne change de date que si la version change, pour ne recompiler version.o qu'à ce moment.
$(BUILD)/version.stamp: FORCE
	@mkdir -p $(dir $@)
	@printf '%s\n' '$(VERSION)' | cmp -s - $@ || printf '%s\n' '$(VERSION)' > $@

FORCE:

# Seuls les fichiers de src/ui utilisent ncurses.
$(filter $(BUILD)/ui/%,$(OBJ)): | check-ncurses

# Tests unitaires et d'intégration : un exécutable par fichier tests/*/test_*.c, lancés depuis la racine.
$(BUILD)/tests/%.o: tests/%.c | check-compiler
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_BIN): $(BUILD)/tests/%: $(BUILD)/tests/%.o $(TEST_SUPPORT_OBJ) $(LIB_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

test: $(TEST_BIN)
	@status=0; for t in $(TEST_BIN); do ./$$t || status=1; done; exit $$status

# Couverture (gcov) des modules testés, dans $(COV_DIR) ; échoue si un test échoue.
coverage: check-gcov
	@rm -rf $(COV_DIR)
	@$(MAKE) --no-print-directory BUILD=$(COV_DIR) COVERAGE=1 test
	@scripts/coverage.sh $(COV_DIR) $(filter-out $(UNTESTED_SRC),$(SRC))

# AddressSanitizer + UndefinedBehaviorSanitizer + LeakSanitizer.
sanitize: check-compiler
	@rm -rf $(SAN_DIR)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		$(MAKE) --no-print-directory BUILD=$(SAN_DIR) SANITIZE=1 test

# Lint clang-tidy (règles dans .clang-tidy) ; échoue si un contrôle est signalé.
lint: check-clang-tidy
	clang-tidy --quiet $(SRC) $(TEST_SRC) $(TEST_SUPPORT_SRC) -- -Isrc -Itests

# Analyse statique cppcheck ; échoue (code 1) si cppcheck signale un problème.
cppcheck: check-cppcheck
	cppcheck --enable=warning,style,performance,portability --std=c11 --inline-suppr \
		--error-exitcode=1 --quiet --suppress='constVariablePointer:tests/*' -Isrc -Itests src tests

# Reformate les sources en place (style dans .clang-format).
format: check-clang-format
	clang-format -i $(SRC) $(HDR) $(TEST_FILES)

# Ne modifie rien ; échoue si un fichier n'est pas formaté.
format-check: check-clang-format
	clang-format --dry-run --Werror $(SRC) $(HDR) $(TEST_FILES)

# Installe gedview, lang/ et le lanceur dans APPDIR, et l'entrée de menu .desktop (sélecteur de fichier au lancement).
install-desktop: gedview
	install -d $(APPDIR) $(DESKTOP_DIR)
	install -m 755 gedview scripts/gedview-open $(APPDIR)/
	rm -rf $(APPDIR)/lang
	cp -r lang $(APPDIR)/lang
	sed 's|@APPDIR@|$(APPDIR)|' scripts/gedview.desktop.in > $(DESKTOP_DIR)/gedview.desktop
	-update-desktop-database $(DESKTOP_DIR) 2>/dev/null
	@echo "GedView installé dans $(APPDIR) ; entrée de menu : $(DESKTOP_DIR)/gedview.desktop"

# Supprime ce qu'a installé install-desktop.
uninstall-desktop:
	rm -rf $(APPDIR) $(DESKTOP_DIR)/gedview.desktop
	-update-desktop-database $(DESKTOP_DIR) 2>/dev/null

# Supprime les dossiers de build, de couverture et l'exécutable.
clean:
	rm -rf build $(COV_DIR) $(SAN_DIR) gedview

# clean + fichiers parasites (sauvegardes d'éditeur, core dumps, sorties valgrind).
distclean: clean
	find . -path ./.git -prune -o \( -name '*~' -o -name '*.swp' -o -name 'core' -o -name 'core.*' -o -name 'vgcore.*' \) -type f -print -exec rm -f {} +

-include $(OBJ:.o=.d)
-include $(TEST_SRC:tests/%.c=$(BUILD)/tests/%.d) $(TEST_SUPPORT_OBJ:.o=.d)
