#!/bin/sh
# Résumé de couverture gcov par fichier, puis total.
# Usage : scripts/coverage.sh <dossier-de-build> <fichier.c>...
dir=$1
shift
export LC_ALL=C

for f in "$@"; do
    gcov -n -b -o "$dir/$(dirname "${f#src/}")" "$f" 2>/dev/null |
        awk -v f="$f" -v q="'" '
            $0 == "File " q f q { p = 1; next }
            /^File / { p = 0 }
            p && /^Lines executed:/ { split($0, a, /[:% ]+/); lp = a[3]; ln = a[5] }
            p && /^Taken at least once:/ { split($0, a, /[:% ]+/); bp = a[5]; bn = a[7] }
            END { printf "%s %s %s %s %s\n", f, lp + 0, ln + 0, bp + 0, bn + 0 }'
done | awk '
    function row(name, lp, ln, bp, bn) {
        printf "%-24s lignes %5.1f%% sur %-4d branches %5.1f%% sur %d\n", name, lp, ln, bp, bn
    }
    { sub(/^src\//, "", $1); row($1, $2, $3, $4, $5); l += $3; lc += $2 * $3 / 100; b += $5; bc += $4 * $5 / 100 }
    END { if (l) row("TOTAL", 100 * lc / l, l, b ? 100 * bc / b : 0, b) }'
