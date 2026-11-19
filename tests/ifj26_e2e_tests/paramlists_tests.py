#!/usr/bin/env python3
"""
paramlists_tests.py - testy rozšíření PARAMLISTS (funkce bez seznamu parametrů
a s více seznamy parametrů) pro překladač IFJ26.

Vygenerováno AI (Claude, Anthropic).
Prompt: "pojďme první udělat testovací soubory (stejně jak jsme udělali
         s funexp) každý bude mít spouštění ./extensionname_tests.py prosím
         A chci aby každý testy měly solo složku"

Co se testuje (zadání, kapitola 5.10 PARAMLISTS):
  - funkce bez seznamu parametrů (def answer: Int = 42) se volá bez závorek
    a tělo se vyhodnotí při každém použití znovu,
  - funkce s více seznamy parametrů (def add(a: Int)(b: Int)(c: Int)),
    i s prázdnými seznamy (def offset()(value: Int)),
  - všechny seznamy argumentů se při volání uvádějí ve stejném pořadí a
    rozdělení, argumenty se vyhodnocují zleva doprava před tělem funkce,
  - chyby: duplicitní jména parametrů (4), zápis do parametru (7), špatný
    počet seznamů / argumentů / typ argumentu (5), redefinice (4), rekurze
    bez návratového typu (6).

Testy 01 až 09 nevyužívají FUNEXP ani jiná rozšíření (argumenty volání jsou
jen termy). Kategorie 99_kombinace_* kombinují PARAMLISTS s dalšími
rozšíření (FUNEXP, BOOLTHEN, CONTROL, VALMEM, INTERPOLATEDS); lze je vynechat
přepínačem --skip 99_kombinace.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup. U nejednoznačných chyb je v testu uvedena n-tice
povolených kódů (viz poznámka u testu).

Příklady:
  ./paramlists_tests.py                              # všechny testy
  ./paramlists_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./paramlists_tests.py -f chyby                     # jen testy s "chyby" v názvu
  ./paramlists_tests.py --skip 99_kombinace          # bez kombinací s jinými rozšířeními
  ./paramlists_tests.py --list                       # jen vypíše názvy testů
  ./paramlists_tests.py --keep out_dir               # uloží .ifj26 a .code
  ./paramlists_tests.py --export scala_out           # uloží .scala/.in/.out pro
                                                     # ověření skutečnou Scalou

Bez --compiler / --interp se 'compiler' a 'ic26int' hledají v adresáři skriptu
a ve všech nadřazených adresářích (a ic26int nakonec v PATH).
"""
import argparse
import difflib
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import textwrap
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROLOG = "import ifj26.*\n\n"


class Case:
    def __init__(self, cat, name, src, out, inp, rc, crc, note):
        self.cat = cat
        self.name = name
        self.src = src
        self.out = out      # očekávaný stdout interpretu
        self.inp = inp      # stdin interpretu
        self.rc = rc        # očekávaný návratový kód interpretu
        self.crc = crc      # kód překladače: 0 = přeloží se, jinak číslo chyby
        #                     (nebo n-tice povolených kódů u nejednoznačných chyb)
        self.note = note

    @property
    def full(self):
        return "%s/%s" % (self.cat, self.name)

    @property
    def compile_codes(self):
        if isinstance(self.crc, (tuple, list)):
            return tuple(self.crc)
        return (self.crc,)


CASES = []


def prog(defs, body):
    """Složí program: prolog, pomocné funkce a @main s odsazeným tělem."""
    defs = textwrap.dedent(defs).strip("\n")
    body = textwrap.indent(textwrap.dedent(body).strip("\n"), "  ")
    src = PROLOG
    if defs:
        src += defs + "\n\n"
    return src + "@main\ndef main(): Unit =\n" + body + "\n"


def T(cat, name, defs, body, out="", inp="", rc=0, crc=0, note=""):
    """Test: pomocné definice + tělo funkce main (odsadí se samo)."""
    CASES.append(Case(cat, name, prog(defs, body), out, inp, rc, crc, note))


def RAW(cat, name, source, out="", inp="", rc=0, crc=0, note=""):
    """Test: celý program za prologem přesně tak, jak je zapsán."""
    src = PROLOG + textwrap.dedent(source).lstrip("\n")
    CASES.append(Case(cat, name, src, out, inp, rc, crc, note))


def D(*parts):
    """Spojí několik definic funkcí prázdným řádkem."""
    return "\n\n".join(textwrap.dedent(p).strip("\n") for p in parts)



# ===========================================================================
# testovací případy
# ===========================================================================
ANSWER = "def answer: Int = 42"
DBL = "def dbl(x: Int): Int = x * 2"
ADD3 = "def add(a: Int)(b: Int)(c: Int): Int = a + b + c"
OFFSET = "def offset()(value: Int): Int = value + 1"
FOO = "def foo(): Int = 42"
AVG = "def avg(a: Double)(b: Double): Double = (a + b) / 2.0"
TICK = """
def tick: Int =
  print("t")
  1
"""
TR = """
def tr(t: String, v: Int): Int =
  print(t)
  v
"""

# ===========================================================================
# 01 funkce bez seznamu parametrů
# ===========================================================================
C = "01_bez_seznamu_parametru"
T(C, "01_zakladni_pouziti", ANSWER, 'print(answer)', out="42")
T(C, "02_ve_vyrazu_bez_funexp", ANSWER,
  'val x = answer + 1\nprint(x)', out="43")
T(C, "03_jako_argument_volani", D(ANSWER, DBL),
  'val d = dbl(answer)\nprint(d)', out="84")
T(C, "04_vicekrat_ve_vyrazu", ANSWER,
  'val x = answer * 2 - answer / 2\nprint(x)', out="63")
T(C, "05_telo_se_vyhodnoti_pri_kazdem_pouziti", TICK,
  'val a = tick + tick + tick\nprint(a)', out="ttt3")
T(C, "06_neni_to_promenna_ani_ulozeny_vysledek", TICK,
  '''
  val a = tick
  print(a)
  print(a)
  print(tick)
  ''', out="t11t1")
T(C, "07_odvozeny_navratovy_typ", "def two = 1 + 1",
  'val t = two * 3\nprint(t)', out="6")
T(C, "08_blokove_telo", '''
def sum10: Int =
  var s = 0
  var i = 1
  while i <= 10 do
    s = s + i
    i = i + 1
  s
''', 'print(sum10)', out="55")
T(C, "09_navratovy_typ_unit", 'def hello: Unit = print("hi")',
  'hello\nhello', out="hihi")
RAW(C, "10_volani_pred_definici", '''
@main
def main(): Unit =
  print(late)

def late: Int = 7
''', out="7")
T(C, "11_vola_dalsi_funkci",
  D("def plus(a: Int, b: Int): Int = a + b", "def seven: Int = plus(3, 4)"),
  'print(seven)', out="7")
T(C, "12_retezeni_bezparametrickych_funkci",
  D("def a1: Int = b1 + 1", "def b1: Int = c1 * 2", "def c1: Int = 5"),
  'print(a1)', out="11")
T(C, "13_v_podmince_while", "def limit: Int = 10", '''
  var i = 0
  while i < limit do
    i = i + 1
  print(i)
  ''', out="10")
T(C, "14_v_prikazu_return", D(ANSWER, '''
def f(x: Int): Int =
  return answer + x
'''), 'val r = f(1)\nprint(r)', out="43")
T(C, "15_typ_double", "def half: Double = 2.5",
  'val h = half\nval s = toStr(h)\nprint(s)', out="2.50")
T(C, "16_typ_string", 'def greeting: String = "ahoj"',
  'val g = greeting + "!"\nprint(g)', out="ahoj!")
T(C, "17_v_podmince_if", "def flag: Int = 1", '''
  if flag == 1 then
    print("one")
  else
    print("other")
  ''', out="one")
T(C, "18_posledni_prikaz_tela_funkce", D(ANSWER, '''
def wrap(): Int =
  print("w")
  answer
'''), 'val r = wrap()\nprint(r)', out="w42")
T(C, "19_dvakrat_stejna_funkce_jako_argumenty", D(ANSWER,
  "def add(a: Int, b: Int): Int = a + b"),
  'val s = add(answer, answer)\nprint(s)', out="84")

# ===========================================================================
# 02 funkce s více seznamy parametrů
# ===========================================================================
C = "02_vice_seznamu"
RAW(C, "01_priklad_ze_zadani", '''
def answer: Int = 42
def answerWithParens(): Int = 42
def add(a: Int)(b: Int)(c: Int): Int = a + b + c
def offset()(value: Int): Int = value + 1

@main def run(): Unit =
  val x = answer
  val y = answerWithParens()
  val sum = add(x)(y)(1)
  val result = offset()(sum)
  print(result) // vypíše 86
''', out="86")
T(C, "02_dva_seznamy", "def add2(a: Int)(b: Int): Int = a + b",
  'val r = add2(3)(4)\nprint(r)', out="7")
T(C, "03_tri_seznamy", ADD3, 'val r = add(1)(2)(3)\nprint(r)', out="6")
T(C, "04_vice_parametru_v_seznamech",
  "def vol(a: Int, b: Int)(c: Int, d: Int): Int = a * b + c * d",
  'val r = vol(2, 3)(4, 5)\nprint(r)', out="26")
T(C, "05_prazdny_prvni_seznam", OFFSET,
  'val r = offset()(5)\nprint(r)', out="6")
T(C, "06_prazdny_posledni_seznam", "def f(a: Int)(): Int = a * 2",
  'val r = f(4)()\nprint(r)', out="8")
T(C, "07_same_prazdne_seznamy", "def g()()(): Int = 7",
  'val r = g()()()\nprint(r)', out="7")
T(C, "08_prazdny_prostredni_seznam",
  "def mid(a: Int)()(b: Int): Int = a * 10 + b",
  'val r = mid(1)()(2)\nprint(r)', out="12")
T(C, "09_ruzne_typy_v_seznamech", '''
def rep(s: String)(n: Int): String =
  var r = ""
  var i = 0
  while i < n do
    r = r + s
    i = i + 1
  r
''', 'val r = rep("ab")(3)\nprint(r)', out="ababab")
T(C, "10_parametry_vsech_seznamu_viditelne_v_tele",
  "def f(a: Int)(b: Int): Int = a * 10 + b",
  'val r = f(3)(4)\nprint(r)', out="34")
T(C, "11_odvozeny_navratovy_typ", "def mul(a: Int)(b: Int) = a * b",
  'val r = mul(6)(7)\nprint(r)', out="42")
T(C, "12_return_ve_vetvich", '''
def sgn(x: Int)(y: Int): Int =
  if x < y then
    return -1
  else
    return 1
''', '''
  val a = sgn(1)(2)
  val b = sgn(5)(2)
  print(a)
  print(" ")
  print(b)
  ''', out="-1 1")
T(C, "13_rekurze_s_vice_seznamy", '''
def sum(n: Int)(acc: Int): Int =
  if n == 0 then
    return acc
  else
    return sum(n - 1)(acc + n)
''', 'val r = sum(10)(0)\nprint(r)', out="55")
T(C, "14_funkce_vracejici_unit", '''
def say(a: String)(b: String): Unit =
  print(a)
  print(b)
''', 'say("x")("y")', out="xy")
T(C, "15_double_parametry", AVG,
  'val r = avg(1.0)(2.0)\nval s = toStr(r)\nprint(s)', out="1.50")
T(C, "16_int_literaly_na_double_parametry", AVG,
  'val r = avg(1)(2)\nval s = toStr(r)\nprint(s)', out="1.50")
RAW(C, "17_dopredne_volani", '''
@main
def main(): Unit =
  val r = late(1)(2)
  print(r)

def late(a: Int)(b: Int): Int = a + b
''', out="3")
T(C, "18_lokalni_promenne_v_tele", '''
def calc(a: Int)(b: Int): Int =
  val c = a + b
  val d = c * c
  d - a
''', 'val r = calc(2)(3)\nprint(r)', out="23")
T(C, "19_volani_ve_smycce", "def add2(a: Int)(b: Int): Int = a + b", '''
  var i = 1
  var s = 0
  while i <= 4 do
    val t = add2(i)(i)
    s = s + t
    i = i + 1
  print(s)
  ''', out="20")
T(C, "20_argumenty_jsou_promenne", "def add2(a: Int)(b: Int): Int = a + b",
  'val x = 3\nval y = 4\nval r = add2(x)(y)\nprint(r)', out="7")
T(C, "21_zmena_lokalni_kopie_parametru", '''
def inc(a: Int)(b: Int): Int =
  var t = a
  t = t + b
  t
''', 'val r = inc(5)(6)\nprint(r)', out="11")
T(C, "22_volani_jako_prikaz_bez_pouziti_vysledku",
  D('''
def noisy(a: Int)(b: Int): Int =
  print("n")
  a + b
'''), 'noisy(1)(2)\nnoisy(3)(4)', out="nn")

# ===========================================================================
# 03 všechny tvary funkcí v jednom programu
# ===========================================================================
C = "03_vsechny_tvary_pohromade"
RAW(C, "01_bez_seznamu_prazdny_a_vice_seznamu", '''
def p: Int = 1
def e(): Int = 2
def m(a: Int)(b: Int): Int = a + b
def s(a: Int): Int = a * 10

@main
def main(): Unit =
  val a = p
  val b = e()
  val c = m(a)(b)
  val d = s(c)
  print(d)
''', out="30")
RAW(C, "02_funkce_se_vzajemne_volaji", '''
def base: Int = 5

def twice(x: Int)(y: Int): Int = x + y

def calc(): Int = twice(base)(base)

@main
def main(): Unit =
  val r = calc()
  print(r)
''', out="10")

# ===========================================================================
# 09 chyby
# ===========================================================================
C = "09_chyby"
T(C, "01_bezparametricka_volana_se_zavorkami", ANSWER,
  'val x = answer()\nprint(x)', crc=5,
  note="zadání: chybný počet seznamů argumentů vede na chybu 5")
T(C, "02_funkce_s_prazdnym_seznamem_volana_bez_zavorek", FOO,
  'val x = foo\nprint(x)', crc=5,
  note="zadání: prázdný seznam nelze vynechat; foo() se volá jako foo()")
T(C, "03_castecna_aplikace", ADD3,
  'val x = add(1)(2)\nprint(x)', crc=5,
  note="částečná aplikace není součástí rozšíření")
T(C, "04_moc_seznamu_argumentu", ADD3,
  'val x = add(1)(2)(3)(4)\nprint(x)', crc=5)
T(C, "05_moc_argumentu_v_prvnim_seznamu", ADD3,
  'val x = add(1, 2)(3)(4)\nprint(x)', crc=5)
T(C, "06_malo_argumentu_v_poslednim_seznamu", ADD3,
  'val x = add(1)(2)()\nprint(x)', crc=5)
T(C, "07_spatny_typ_ve_prostrednim_seznamu", ADD3,
  'val x = add(1)("a")(3)\nprint(x)', crc=5)
T(C, "08_vynechany_prazdny_seznam", OFFSET,
  'val x = offset(5)\nprint(x)', crc=5)
T(C, "09_pridany_prazdny_seznam", OFFSET,
  'val x = offset()()(5)\nprint(x)', crc=5)
T(C, "10_jine_rozdeleni_parametru",
  "def add(a: Int, b: Int)(c: Int): Int = a + b + c",
  'val x = add(1)(2, 3)\nprint(x)', crc=5,
  note="volání musí mít stejné rozdělení do seznamů jako definice")
T(C, "11_duplicitni_jmeno_v_ruznych_seznamech",
  "def f(a: Int)(a: Int): Int = a", 'print("x")', crc=4)
T(C, "12_duplicitni_jmeno_pres_prazdny_seznam",
  "def f(a: Int)()(a: Int): Int = a", 'print("x")', crc=4)
T(C, "13_duplicitni_jmeno_ve_stejnem_seznamu",
  "def f(a: Int, a: Int)(b: Int): Int = a", 'print("x")', crc=4)
T(C, "14_zapis_do_parametru_druheho_seznamu", '''
def f(a: Int)(b: Int): Int =
  b = 5
  b
''', 'print("x")', crc=7)
T(C, "15_redefinice_bezparametricke_a_s_prazdnym_seznamem",
  D("def foo: Int = 1", "def foo(): Int = 2"), 'print("x")', crc=4,
  note="přetěžování není podporováno, dvě definice stejného jména")
T(C, "16_redefinice_s_jinym_poctem_seznamu",
  D("def f(a: Int)(b: Int): Int = a + b", "def f(a: Int): Int = a"),
  'print("x")', crc=4)
T(C, "17_nedefinovana_funkce_s_vice_seznamy", "",
  'val x = nope(1)(2)\nprint(x)', crc=3)
T(C, "18_rekurze_bez_typu_vice_seznamu",
  "def f(n: Int)(m: Int) = f(n)(m)", 'print("x")', crc=6)
T(C, "19_rekurze_bez_typu_bezparametricke",
  "def loop = loop", 'print("x")', crc=6)
T(C, "20_spatny_navratovy_typ",
  "def f(a: Int)(b: Int): String = a + b", 'print("x")', crc=6)
T(C, "21_bezparametricka_volana_s_argumenty", ANSWER,
  'val x = answer(1)\nprint(x)', crc=5)
T(C, "22_prirazeni_do_funkce", ANSWER,
  'answer = 5\nprint("x")', crc=(3, 7),
  note="levá strana není modifikovatelná proměnná (7), nebo nedefinovaná (3)")
RAW(C, "23_chybi_zaviraci_zavorka_ve_druhem_seznamu", '''
def f(a: Int)(b: Int = a

@main
def main(): Unit =
  print("x")
''', crc=2)
T(C, "24_bezparametricka_a_retezec_ve_vyrazu", ANSWER,
  'val s = answer + "a"\nprint(s)', crc=6)
T(C, "25_spatny_typ_double_argumentu_v_druhem_seznamu", AVG,
  'val r = avg(1.0)("x")\nprint("y")', crc=5)
T(C, "26_vynechan_posledni_prazdny_seznam",
  "def f(a: Int)(): Int = a", 'val x = f(1)\nprint(x)', crc=5)
T(C, "27_funkce_s_prazdnym_seznamem_bez_zavorek_ve_vyrazu", FOO,
  'val x = foo + 1\nprint(x)', crc=5)
T(C, "28_nedefinovana_bezparametricka_funkce", "",
  'val x = nope + 1\nprint(x)', crc=3)

# ===========================================================================
# 99 kombinace s dalšími rozšířeními
# ===========================================================================
C = "99_kombinace_funexp"
T(C, "01_vnorena_volani_s_vice_seznamy", ADD3,
  'print(toStr(add(add(1)(2)(3))(4)(5)))', out="15")
T(C, "02_vice_seznamu_jako_operand", ADD3,
  'print(toStr(1 + add(1)(2)(3) * 2))', out="13")
T(C, "03_argumenty_zleva_doprava_pres_vsechny_seznamy",
  D(TR, "def f(a: Int)(b: Int)(c: Int): Int = a * 100 + b * 10 + c"),
  '''
  val r = f(tr("a", 1))(tr("b", 2))(tr("c", 3))
  print("|" + toStr(r))
  ''', out="abc|123")
T(C, "04_bezparametricka_se_vedlejsim_efektem_ve_vyrazu", '''
def tickA: Int =
  print("a")
  1

def tickB: Int =
  print("b")
  2
''', '''
  val r = tickA * 10 + tickB
  print("|" + toStr(r))
  ''', out="ab|12")
T(C, "05_bezparametricka_v_argumentu_vestavene_funkce", ANSWER,
  'print(toStr(answer + 1))', out="43")
T(C, "06_argumenty_se_vyhodnoti_pred_telem", D(TR, '''
def g(a: Int)(b: Int): Int =
  print("B")
  a + b
'''), '''
  val r = g(tr("a", 1))(tr("b", 2))
  print("|" + toStr(r))
  ''', out="abB|3")
T(C, "07_bezparametricka_v_poslednim_seznamu", D(ANSWER, OFFSET),
  'print(toStr(offset()(answer)))', out="43")
T(C, "08_bezparametricka_v_argumentu_uzivatelske_funkce", D(ANSWER, DBL),
  'print(toStr(dbl(answer) + dbl(answer)))', out="168")

C = "99_kombinace_boolthen"
T(C, "01_bezparametricka_funkce_typu_boolean", "def yes: Boolean = true", '''
  if yes then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "02_vice_seznamu_vraci_boolean",
  "def less(a: Int)(b: Int): Boolean = a < b",
  'val r = less(1)(2)\nprint(r)', out="true")
T(C, "03_boolean_parametry_ve_vice_seznamech",
  "def both(a: Boolean)(b: Boolean): Boolean = a && b",
  'val r = both(true)(false)\nprint(r)', out="false")

C = "99_kombinace_control"
T(C, "01_bezparametricka_jako_mez_cyklu", "def n: Int = 3",
  'for i <- 1 to n do print(i)', out="123")
T(C, "02_mez_cyklu_se_vyhodnoti_jednou", '''
def tickN: Int =
  print("n")
  3
''', 'for i <- 1 to tickN do print(i)', out="n123")
T(C, "03_if_vyraz_s_vice_seznamy",
  "def pick(c: Boolean)(a: Int)(b: Int): Int = if c then a else b",
  'val r = pick(false)(1)(2)\nprint(r)', out="2")

C = "99_kombinace_valmem"
T(C, "01_clen_na_bezparametricke_funkci", ANSWER,
  'val n = answer.toString.length\nprint(n)', out="2")
T(C, "02_clen_na_vysledku_vice_seznamu", ADD3,
  'val n = add(100)(200)(300).toString.length\nprint(n)', out="3")

C = "99_kombinace_interpolateds"
T(C, "01_bezparametricka_v_interpolaci", ANSWER,
  'val m = s"v=${answer}"\nprint(m)', out="v=42")


# ---------------------------------------------------------------------------
# runner
# ---------------------------------------------------------------------------
def search_up(names, starts):
    """Hledá spustitelný soubor v zadaných adresářích a jejich rodičích."""
    seen = set()
    for start in starts:
        d = os.path.abspath(start)
        for _ in range(8):
            for rel in names:
                p = os.path.join(d, rel)
                if p not in seen:
                    seen.add(p)
                    if os.path.isfile(p) and os.access(p, os.X_OK):
                        return p
            parent = os.path.dirname(d)
            if parent == d:
                break
            d = parent
    return None


def check_exe(path, what):
    """Ověří, že cesta existuje a jde spustit (nebo je v PATH)."""
    if os.path.isfile(path) and os.access(path, os.X_OK):
        return path
    found = shutil.which(path)
    if found:
        return found
    sys.exit("%s '%s' nenalezen nebo není spustitelný" % (what, path))


def find_compiler(arg):
    if arg:
        return check_exe(arg, "překladač")
    found = search_up(["compiler"], [HERE, os.getcwd()])
    if found:
        return found
    sys.exit("překladač nenalezen, použijte --compiler CESTA")


def find_interp(arg):
    if arg:
        parts = shlex.split(arg)
        parts[0] = check_exe(parts[0], "interpret")
        return parts
    found = search_up(["ic26int", "tests/ifj26_e2e_tests/ic26int"],
                      [HERE, os.getcwd()])
    if not found:
        found = shutil.which("ic26int")
    if found:
        return [found]
    sys.exit("interpret ic26int nenalezen, použijte --interp CESTA")


def show_diff(exp, got):
    d = difflib.unified_diff(exp.splitlines(True), got.splitlines(True),
                             "očekáváno", "získáno", n=1)
    return "".join(d)


def run_case(c, compiler, interp, tmp, keep, timeout):
    base = os.path.join(tmp, c.full.replace("/", "__"))
    with open(base + ".ifj26", "w") as f:
        f.write(c.src)
    try:
        p = subprocess.run([compiler], input=c.src.encode(),
                           capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return False, "překladač: timeout"
    code = p.stdout.decode("latin-1")
    if keep:
        os.makedirs(keep, exist_ok=True)
        shutil.copy(base + ".ifj26", os.path.join(
            keep, os.path.basename(base) + ".ifj26"))
    if c.compile_codes != (0,):
        if p.returncode not in c.compile_codes:
            return False, "rc překladače %d, očekáváno %s" % (
                p.returncode, " nebo ".join(str(x) for x in c.compile_codes))
        if p.returncode == 0:
            return True, ""     # 0 je mezi povolenými kódy: program se přeložil
        if code:
            return False, "při chybě kompilace nesmí vzniknout žádný výstup"
        return True, ""
    if p.returncode != 0:
        err = p.stderr.decode(errors="replace").strip().splitlines()[-2:]
        return False, "překladač vrátil %d (čekáno 0)%s" % (
            p.returncode, ("\n    " + "\n    ".join(err)) if err else "")
    with open(base + ".code", "w") as f:
        f.write(code)
    if keep:
        shutil.copy(base + ".code", os.path.join(
            keep, os.path.basename(base) + ".code"))
    try:
        r = subprocess.run(interp + [base + ".code"], input=c.inp.encode(),
                           capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return False, "interpret: timeout"
    out = r.stdout.decode("latin-1")
    msgs = []
    if r.returncode != c.rc:
        msgs.append("rc interpretu %d, očekáváno %d" % (r.returncode, c.rc))
        tail = r.stderr.decode(errors="replace").strip().splitlines()[-2:]
        if tail:
            msgs.append("stderr: " + " | ".join(tail))
    if out != c.out:
        msgs.append("stdout se liší:\n" + textwrap.indent(
            show_diff(c.out, out) or "(rozdíl jen v bílých znacích)", "      "))
    return (not msgs), "\n    ".join(msgs)


def export_scala(directory, cases):
    os.makedirs(directory, exist_ok=True)
    n = 0
    for c in cases:
        if c.compile_codes != (0,) or c.rc != 0:
            continue
        stem = os.path.join(directory, c.full.replace("/", "__"))
        with open(stem + ".scala", "w") as f:
            f.write(c.src)
        with open(stem + ".in", "w") as f:
            f.write(c.inp)
        with open(stem + ".out", "w") as f:
            f.write(c.out)
        n += 1
    print("exportováno %d programů do %s" % (n, directory))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--compiler",
                    help="cesta k překladači (jinak se hledá 'compiler' "
                         "v nadřazených adresářích)")
    ap.add_argument("--interp",
                    help="cesta k ic26int (lze i s přepínači v uvozovkách)")
    ap.add_argument("-f", "--filter", default="",
                    help="jen testy s tímto podřetězcem v názvu")
    ap.add_argument("--skip", default="",
                    help="vynechá testy s tímto podřetězcem v názvu "
                         "(např. --skip 99_kombinace)")
    ap.add_argument("--keep", help="adresář pro .ifj26 a vygenerované .code")
    ap.add_argument("--export", help="uložit programy jako .scala/.in/.out")
    ap.add_argument("--timeout", type=int, default=10,
                    help="limit v sekundách pro překladač i interpret (10)")
    ap.add_argument("--list", action="store_true", help="jen vypsat testy")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    sel = [c for c in CASES if args.filter in c.full
           and not (args.skip and args.skip in c.full)]
    if args.export:
        export_scala(args.export, sel)
        return 0
    if args.list:
        for c in sel:
            print(c.full)
        return 0
    if not sel:
        print("žádný test neodpovídá filtru", file=sys.stderr)
        return 1

    compiler = find_compiler(args.compiler)
    interp = find_interp(args.interp)
    tmp = tempfile.mkdtemp(prefix="ifj26ext_")
    per_cat = {}
    ok = fail = 0
    failed = []
    t0 = time.time()
    try:
        for c in sel:
            good, msg = run_case(c, compiler, interp, tmp, args.keep,
                                 args.timeout)
            stat = per_cat.setdefault(c.cat, [0, 0])
            stat[1] += 1
            if good:
                ok += 1
                stat[0] += 1
                if args.verbose:
                    print("PASS  %s" % c.full)
            else:
                fail += 1
                failed.append(c.full)
                print("FAIL  %s\n    %s" % (c.full, msg))
                if c.note:
                    print("    poznámka: " + c.note)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("\nSouhrn podle kategorií:")
    for cat, (good, total) in per_cat.items():
        print("  %-30s %3d/%d" % (cat, good, total))
    print("\nCelkem: %d   OK: %d   selhalo: %d   (%.1f s)"
          % (ok + fail, ok, fail, time.time() - t0))
    return 1 if fail else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        sys.stderr.close()
        sys.exit(0)
