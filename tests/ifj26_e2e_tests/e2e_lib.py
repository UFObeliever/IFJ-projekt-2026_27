# -*- coding: utf-8 -*-
"""
Společný kód pro general_tests.py a generator_tests.py.

Postup pro každý test (tests/<skupina>/<kategorie>/NN_nazev.rc<kód>[.run<kód>].scala):
  1. překladač dostane zdrojový kód na stdin a na stdout vydá IFJcode26
  2. návratový kód překladače musí být jeden z očekávaných (.rc2, .rc2or6, ...)
  3. při kódu 0 musí výstup začínat řádkem .IFJcode26
  4. kód se spustí v ic26int se vstupem z NN_nazev.in (je-li) a prázdným vstupem jinak
  5. návratový kód interpretu se porovná s .run<kód> (výchozí 0)
  6. stdout interpretu se porovná s NN_nazev.out (je-li)
"""
import argparse
import concurrent.futures
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))

NAME_RE = re.compile(
    r"^(?P<nn>\d+)_(?P<name>.+?)\.rc(?P<rc>\d+(?:or\d+)*)(?:\.run(?P<run>\d+))?\.scala$")

# Co znamenají návratové kódy ic26int (zadání, kap. 7.1) – s tipem, kde hledat chybu v generátoru.
IC_HINTS = {
    50: "chybné parametry příkazové řádky interpretu (zkontrolujte --interp)",
    51: "chyba analýzy IFJcode26 – generátor vydal neplatný kód (překlep v instrukci, špatný zápis operandu "
        "nebo konstanty, chybějící hlavička)",
    52: "sémantická chyba v IFJcode26 – redefinice proměnné nebo návěští (typicky DEFVAR uvnitř cyklu / "
        "rekurze, nebo duplicitní návěští) či skok na neexistující návěští",
    53: "špatné typy operandů (např. Int a Float v jedné instrukci chybí INT2FLOAT, nebo IDIV nad float)",
    54: "přístup k neexistující proměnné (rámec existuje) – chybí DEFVAR nebo špatné jméno proměnné",
    55: "rámec neexistuje – chybí CREATEFRAME/PUSHFRAME/POPFRAME nebo se používá TF/LF ve špatný okamžik",
    56: "chybějící hodnota – čtení neinicializované proměnné, prázdný datový zásobník nebo zásobník volání",
    57: "špatná hodnota operandu – dělení nulou nebo špatná hodnota v EXIT",
    58: "chybná práce s řetězcem (INT2CHAR mimo 0..127, GETCHAR/STRI2INT mimo řetězec)",
    60: "interní chyba interpretu",
}


class Test:
    def __init__(self, root, path):
        self.path = path
        self.rel = os.path.relpath(path, root).replace(os.sep, "/")
        parts = self.rel.split("/")
        self.cat = "/".join(parts[:-1])
        fn = parts[-1]
        m = NAME_RE.match(fn)
        self.ok_name = m is not None
        if m:
            self.rc_set = [int(x) for x in m.group("rc").split("or")]
            self.run_rc = int(m.group("run") or 0)
        base = path[:-len(".scala")]
        self.in_path = base + ".in" if os.path.isfile(base + ".in") else None
        self.out_path = base + ".out" if os.path.isfile(base + ".out") else None
        self.code_path = base  + ".code"


class Result:
    def __init__(self, test, ok, stage="", msg="", details=""):
        self.test, self.ok, self.stage, self.msg, self.details = test, ok, stage, msg, details


def read(path):
    with open(path, "rb") as fh:
        return fh.read()


def run_cmd(cmd, data, timeout):
    """Vrátí (rc, stdout, stderr, vypršel_čas)."""
    try:
        p = subprocess.run(cmd, input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
        return p.returncode, p.stdout, p.stderr, False
    except subprocess.TimeoutExpired as e:
        return None, e.stdout or b"", e.stderr or b"", True


def head(data, n=8):
    text = data.decode("utf-8", "replace").rstrip("\n")
    lines = text.split("\n")
    s = "\n".join("      | " + ln for ln in lines[:n])
    if len(lines) > n:
        s += "\n      | ... (%d dalších řádků)" % (len(lines) - n)
    return s


def show_diff(expected, actual):
    """Čitelný rozdíl dvou výstupů (zobrazuje i neviditelné znaky)."""
    def esc(b, limit=300):
        r = repr(b.decode("latin-1"))
        return r if len(r) <= limit else r[:limit] + "...(+%d znaků)" % (len(r) - limit)

    k = 0
    while k < min(len(expected), len(actual)) and expected[k] == actual[k]:
        k += 1
    lines = ["      očekáváno: " + esc(expected), "      vypsáno:   " + esc(actual)]
    if len(expected) > 300 or len(actual) > 300:
        lo = max(0, k - 20)
        lines.append("      první rozdíl na bajtu %d: očekáváno ...%r  vypsáno ...%r"
                     % (k, expected[lo:k + 30].decode("latin-1"), actual[lo:k + 30].decode("latin-1")))
    return "\n".join(lines)


def run_test(t, opts):
    src = read(t.path)

    # ---- 1) překladač
    rc, code, cerr, timed_out = run_cmd(opts.compiler, src, opts.timeout)
    if timed_out:
        return Result(t, False, "překladač", "překladač neskončil do %g s (nekonečná smyčka?)" % opts.timeout)
    if rc not in t.rc_set:
        exp = " nebo ".join(str(x) for x in t.rc_set)
        extra = ""
        if rc == 0:
            extra = " (překladač program přijal)"
        elif rc in (134, 139, 136, 137) or rc < 0:
            extra = " (pád překladače, signál)"
        return Result(t, False, "kód překladače", "překladač vrátil %s, očekáváno %s%s" % (rc, exp, extra),
                      head(cerr) if cerr else "")
    if rc != 0:
        return Result(t, True)

    # ---- 2) hlavička
    first = code.split(b"\n", 1)[0].strip()
    if first != b".IFJcode26":
        return Result(t, False, "hlavička", "výstup překladače nezačíná řádkem .IFJcode26 (začíná %r)"
                      % code[:30].decode("latin-1"))
    if opts.no_run:
        return Result(t, True)

    # ---- 3) běh v ic26int
    stdin = read(t.in_path) if t.in_path else b""
    with open(t.code_path, "wb") as fh:
        fh.write(code)
    rrc, out, rerr, timed_out = run_cmd(opts.interp + [t.code_path], stdin, opts.timeout)
        

    def saved():
        if opts.save:
            os.makedirs(opts.save, exist_ok=True)
            dest = os.path.join(opts.save, t.rel.replace("/", "__")[:-len(".scala")] + ".code")
            with open(dest, "wb") as fh:
                fh.write(code)

    if timed_out:
        saved()
        return Result(t, False, "běh", "ic26int neskončil do %g s (nekonečná smyčka ve vygenerovaném kódu?)"
                      % opts.timeout)
    if rrc != t.run_rc:
        saved()
        hint = IC_HINTS.get(rrc, "")
        if t.run_rc == 0:
            msg = "ic26int skončil s kódem %d%s" % (rrc, " – " + hint if hint else "")
        else:
            msg = "program měl skončit kódem %d, ale skončil kódem %d%s" % (
                t.run_rc, rrc, " – " + hint if hint else "")
        det = []
        if rerr:
            det.append("    stderr interpretu:\n" + head(rerr, 4))
        if t.out_path:
            det.append(show_diff(read(t.out_path), out))
        return Result(t, False, "kód ic26int", msg, "\n".join(det))

    # ---- 4) stdout
    if t.out_path is not None:
        expected = read(t.out_path)
        if out != expected:
            saved()
            return Result(t, False, "výstup", "výstup programu se liší", show_diff(expected, out))
    return Result(t, True)


def discover(root, subdirs, includes, excludes):
    tests = []
    for sub in subdirs:
        base = os.path.join(root, sub)
        for dirpath, _dirs, files in sorted(os.walk(base)):
            for fn in sorted(files):
                if not fn.endswith(".scala"):
                    continue
                t = Test(root, os.path.join(dirpath, fn))
                if not t.ok_name:
                    continue
                if includes and not any(k in t.rel for k in includes):
                    continue
                if any(k in t.rel for k in excludes):
                    continue
                tests.append(t)
    return tests


def which_cmd(cmd):
    exe = cmd[0]
    if os.sep in exe:
        return os.path.isfile(exe) and os.access(exe, os.X_OK)
    return shutil.which(exe) is not None


def main(subdirs, prog, doc):
    ap = argparse.ArgumentParser(prog=prog, description=doc,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--compiler", default=os.environ.get("IFJ_COMPILER", "./ifj26"),
                    help="příkaz překladače (čte zdroj ze stdin, IFJcode26 vypisuje na stdout); "
                         "výchozí ./ifj26 nebo proměnná IFJ_COMPILER")
    ap.add_argument("--interp", default=os.environ.get("IFJ_INTERP", "ic26int"),
                    help="příkaz interpretu IFJcode26 (dostane soubor s kódem jako poslední argument); "
                         "výchozí ic26int nebo proměnná IFJ_INTERP")
    ap.add_argument("-k", dest="include", action="append", default=[], metavar="TEXT",
                    help="spustí jen testy, jejichž cesta obsahuje TEXT (lze opakovat)")
    ap.add_argument("-x", dest="exclude", action="append", default=[], metavar="TEXT",
                    help="vynechá testy, jejichž cesta obsahuje TEXT (lze opakovat)")
    ap.add_argument("-v", "--verbose", action="store_true", help="u neúspěšných testů vypíše podrobnosti")
    ap.add_argument("-j", "--jobs", type=int, default=min(4, os.cpu_count() or 1), help="počet souběžných testů")
    ap.add_argument("--timeout", type=float, default=10.0, help="limit na překlad i běh jednoho testu (s)")
    ap.add_argument("--save", metavar="DIR", help="uloží IFJcode26 neúspěšných testů do DIR")
    ap.add_argument("--no-run", action="store_true",
                    help="nespouští ic26int, kontroluje jen překlad (kód překladače + hlavičku)")
    ap.add_argument("--list", action="store_true", help="jen vypíše vybrané testy")
    ap.add_argument("--tests", default=os.path.join(HERE, "tests"), help="adresář s testy")
    opts = ap.parse_args()

    opts.compiler = shlex.split(opts.compiler)
    opts.interp = shlex.split(opts.interp)

    tests = discover(opts.tests, subdirs, opts.include, opts.exclude)
    if not tests:
        print("Nenalezen žádný test (adresář %s, skupiny %s)." % (opts.tests, ", ".join(subdirs)), file=sys.stderr)
        return 2
    if opts.list:
        for t in tests:
            print(t.rel)
        print("\n%d testů" % len(tests))
        return 0
    if not which_cmd(opts.compiler):
        print("Překladač '%s' nenalezen nebo není spustitelný (použijte --compiler)." % opts.compiler[0],
              file=sys.stderr)
        return 2
    if not opts.no_run and not which_cmd(opts.interp):
        print("Interpret '%s' nenalezen (použijte --interp, nebo --no-run pro kontrolu jen překladu)."
              % opts.interp[0], file=sys.stderr)
        return 2

    t0 = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, opts.jobs)) as ex:
        results = list(ex.map(lambda t: run_test(t, opts), tests))
    dt = time.time() - t0

    fails = [r for r in results if not r.ok]
    for r in fails:
        print("FAIL  %s" % r.test.rel)
        print("      [%s] %s" % (r.stage, r.msg))
        if opts.verbose and r.details:
            print(r.details)
    if fails:
        print()

    # souhrn podle kategorií
    cats = {}
    for r in results:
        c = cats.setdefault(r.test.cat, [0, 0])
        c[1] += 1
        c[0] += 1 if r.ok else 0
    width = max(len(c) for c in cats)
    print("Souhrn podle kategorií:")
    for c in sorted(cats):
        ok, tot = cats[c]
        print("  %-*s %4d/%-4d %s" % (width, c, ok, tot, "" if ok == tot else "<-- %d selhalo" % (tot - ok)))

    stages = {}
    for r in fails:
        stages[r.stage] = stages.get(r.stage, 0) + 1
    print()
    print("Celkem: %d   OK: %d   selhalo: %d   (%.1f s)" % (len(results), len(results) - len(fails), len(fails), dt))
    if stages:
        print("Selhání podle fáze: " + ", ".join("%s: %d" % (k, v) for k, v in sorted(stages.items())))
        if not opts.verbose:
            print("(podrobnosti: -v, uložení vygenerovaného kódu: --save DIR, jeden test: -k nazev)")
    return 1 if fails else 0
