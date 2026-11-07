#!/usr/bin/env bash
# Spouštěč testů parseru IFJ26.
#
#   ./run_tests.sh <cesta_ke_kompilátoru> [filtr]
#
# Kompilátor dostane zdrojový kód na stdin (výstup se zahazuje) a porovnává se
# POUZE návratový kód. Očekávaný kód je v názvu souboru: *.rc2.scala, *.rc2or6.scala.
# Filtr je podřetězec cesty testu, např. ./run_tests.sh ./ifj26 09_rozsahy
#
# Proměnné prostředí:
#   TIMEOUT=5    limit na jeden test v sekundách
#   VERBOSE=1    u neúspěšných testů vypíše prvních pár řádků stderr kompilátoru

COMPILER="${1:-}"
FILTER="${2:-}"

if [[ -z "$COMPILER" ]]; then
  echo "Použití: $0 <kompilátor> [filtr]" >&2
  exit 2
fi
if [[ ! -x "$COMPILER" ]]; then
  echo "Soubor '$COMPILER' neexistuje nebo není spustitelný." >&2
  exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS="$ROOT/tests"
TIMEOUT="${TIMEOUT:-5}"
ERR="$(mktemp)"
trap 'rm -f "$ERR"' EXIT

# --- Způsob spuštění kompilátoru (upravte, pokud čtete ze souboru / používáte přepínače) ---
run_one() {
  timeout "$TIMEOUT" "$COMPILER" < "$1" > /dev/null 2> "$ERR"
}
# -----------------------------------------------------------------------------------------

describe_rc() {
  case "$1" in
    124) echo "124 (timeout)" ;;
    134) echo "134 (SIGABRT, např. assert)" ;;
    136) echo "136 (SIGFPE)" ;;
    139) echo "139 (SIGSEGV)" ;;
    *)   echo "$1" ;;
  esac
}

pass=0
fail=0
crash=0

while IFS= read -r -d '' f; do
  rel="${f#"$TESTS"/}"
  if [[ -n "$FILTER" && "$rel" != *"$FILTER"* ]]; then
    continue
  fi

  base="${f##*/}"
  base="${base%.scala}"
  exp="${base##*.rc}"

  ( run_one "$f" ) 2>/dev/null
  rc=$?

  ok=0
  for code in ${exp//or/ }; do
    [[ "$rc" == "$code" ]] && ok=1
  done

  if (( ok )); then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    case "$rc" in 124|134|136|139) crash=$((crash + 1)) ;; esac
    printf 'FAIL  %-75s očekáváno %s, vráceno %s\n' "$rel" "${exp//or/ nebo }" "$(describe_rc "$rc")"
    if [[ -n "${VERBOSE:-}" && -s "$ERR" ]]; then
      head -n 500 "$ERR" | sed 's/^/        | /'
    fi
  fi
done < <(find "$TESTS" -name '*.scala' -print0 | LC_ALL=C sort -z)

echo
echo "Celkem: $((pass + fail))   OK: $pass   selhalo: $fail   (z toho pády/timeouty: $crash)"
(( fail == 0 ))
