#!/usr/bin/env python3
"""
extkw_tests.py - testy rozšíření EXTKW (uživatelské rozšiřující metody,
klíčové slovo extension) pro překladač IFJ26.

Vygenerováno AI (Claude, Anthropic).
Prompt: "pojďme první udělat testovací soubory (stejně jak jsme udělali
         s funexp) každý bude mít spouštění ./extensionname_tests.py prosím
         A chci aby každý testy měly solo složku"

Co se testuje (zadání, kapitola 5.9 EXTKW):
  - extension (value: Int) + odsazený blok metod na nejvyšší úrovni,
    příjemce je jediný explicitně typovaný parametr,
  - bezparametrická metoda (def twice: Int, volá se x.twice bez závorek),
    metoda s prázdným seznamem (def foo(): String, volá se x.foo()),
    metoda s parametry (x.plus(12)); metody se volají výhradně členským
    zápisem a nelze x.foo ani x.twice(),
  - metoda je viditelná v celém programu bez ohledu na pořadí definice,
  - stejné jméno lze použít pro různé typy příjemce, pro tentýž typ ne (4),
  - rekurze, vzájemná rekurze, řetězení metod, příjemcem může být proměnná,
    literál v závorkách nebo libovolný výraz,
  - chyby: neznámá metoda (3), chybný počet / typ argumentů (5), redefinice
    (4), zápis do příjemce (7), syntaxe bloku extension (2), typové chyby (6).

Testy 01 až 04 nevyužívají FUNEXP ani VALMEM: v tělech metod se místo
.toString používá toStr přes proměnnou a argumenty volání jsou jen termy.
Kategorie 99_kombinace_* kombinují EXTKW s dalšími rozšířeními (FUNEXP, VALMEM,
BOOLTHEN, CONTROL, PARAMLISTS, INTERPOLATEDS) včetně příkladu ze zadání;
lze je vynechat přepínačem --skip 99_kombinace.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup. U nejednoznačných chyb je v testu uvedena n-tice
povolených kódů (viz poznámka u testu).

Příklady:
  ./extkw_tests.py                                   # všechny testy
  ./extkw_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./extkw_tests.py -f chyby                          # jen testy s "chyby" v názvu
  ./extkw_tests.py --skip 99_kombinace               # bez kombinací s jinými rozšířeními
  ./extkw_tests.py --list                            # jen vypíše názvy testů
  ./extkw_tests.py --keep out_dir                    # uloží .ifj26 a .code
  ./extkw_tests.py --export scala_out                # uloží .scala/.in/.out pro
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
EXT_INT = """
extension (value: Int)
  def twice: Int = value * 2
  def foo(): Int = value + 1
  def plus(other: Int): Int = value + other
"""
TWICE = """
extension (v: Int)
  def twice: Int = v * 2
"""
TR = """
def tr(t: String, v: Int): Int =
  print(t)
  v
"""

# ===========================================================================
# 01 základní použití
# ===========================================================================
C = "01_zakladni"
T(C, "01_bezparametricka_metoda", TWICE,
  'val x = 6\nval y = x.twice\nprint(y)', out="12")
T(C, "02_metoda_s_prazdnym_seznamem", EXT_INT,
  'val x = 6\nval z = x.foo()\nprint(z)', out="7")
T(C, "03_metoda_s_parametrem", EXT_INT,
  'val x = 6\nval w = x.plus(12)\nprint(w)', out="18")
T(C, "04_tri_druhy_metod_v_jednom_bloku", '''
extension (value: Int)
  // bezparametrická metoda
  def twice: Int = value * 2
  // metoda s prázdným seznamem parametrů
  def foo(): String =
    val s = toStr(value)
    "foo" + s
  // metoda s jedním parametrem
  def plus(other: Int): Int = value + other
''', '''
  val x: Int = 6
  val y = x.twice
  val z = x.foo()
  val w = x.plus(12)
  print(y)
  print(",")
  print(z)
  print(",")
  print(w)
  ''', out="12,foo6,18")
T(C, "05_prijemce_je_literal_v_zavorkach", TWICE,
  'val y = (5).twice\nprint(y)', out="10")
T(C, "06_prijemce_je_vyraz", TWICE,
  'val x = 6\nval y = (x + 1).twice\nprint(y)', out="14")
T(C, "07_prijemce_typu_double", '''
extension (d: Double)
  def half: Double = d / 2.0
''', 'val d = 5.0\nval r = d.half\nval s = toStr(r)\nprint(s)', out="2.50")
T(C, "08_prijemce_typu_string", '''
extension (s: String)
  def shout: String = s + "!"
''', 'val a = "hey"\nval b = a.shout\nprint(b)', out="hey!")
T(C, "09_prijemce_je_retezcovy_literal", '''
extension (s: String)
  def shout: String = s + "!"
''', 'val b = "hey".shout\nprint(b)', out="hey!")
T(C, "10_metoda_vracejici_unit", '''
extension (value: Int)
  def show: Unit = print(value)
''', 'val x = 6\nx.show\nx.show', out="66")
T(C, "11_retezeni_metod_s_parametrem", EXT_INT,
  'val x = 6\nval r = x.twice.plus(1)\nprint(r)', out="13")
T(C, "12_retezeni_bezparametrickych_metod", TWICE,
  'val x = 6\nval r = x.twice.twice\nprint(r)', out="24")
T(C, "13_metoda_vola_jinou_metodu_stejneho_typu", '''
extension (v: Int)
  def twice: Int = v * 2
  def quad: Int = v.twice.twice
''', 'val x = 3\nval r = x.quad\nprint(r)', out="12")
T(C, "14_vice_parametru_a_return", '''
extension (v: Int)
  def clamp(lo: Int, hi: Int): Int =
    if v < lo then
      return lo
    else if v > hi then
      return hi
    else
      return v
''', '''
  val a = 15
  val b = 0 - 5
  val c = 5
  val r1 = a.clamp(0, 10)
  val r2 = b.clamp(0, 10)
  val r3 = c.clamp(0, 10)
  print(r1)
  print(" ")
  print(r2)
  print(" ")
  print(r3)
  ''', out="10 0 5")
T(C, "15_telo_s_cyklem_a_lokalnimi_promennymi", '''
extension (n: Int)
  def sumTo: Int =
    var s = 0
    var i = 1
    while i <= n do
      s = s + i
      i = i + 1
    s
''', 'val x = 10\nval r = x.sumTo\nprint(r)', out="55")
T(C, "16_rekurze", '''
extension (n: Int)
  def fact: Int =
    if n <= 1 then
      return 1
    else
      val m = (n - 1).fact
      return n * m
''', 'val x = 5\nval r = x.fact\nprint(r)', out="120")
T(C, "17_metoda_ve_vyrazu", TWICE, '''
  val x = 6
  val r = x.twice + x.twice * 2
  print(r)
  ''', out="36")
T(C, "18_odvozeny_navratovy_typ", '''
extension (value: Int)
  def triple = value * 3
''', 'val x = 4\nval r = x.triple\nprint(r)', out="12")
T(C, "19_jmeno_prijemce_je_libovolne", '''
extension (self: Int)
  def inc: Int = self + 1
''', 'val x = 4\nval r = x.inc\nprint(r)', out="5")
T(C, "20_metoda_vola_bezny_funkci", D("def dbl(x: Int): Int = x * 2", '''
extension (v: Int)
  def viaDbl: Int = dbl(v)
'''), 'val x = 5\nval r = x.viaDbl\nprint(r)', out="10")
T(C, "21_metoda_s_retezcovymi_parametry", '''
extension (s: String)
  def wrap(l: String, r: String): String = l + s + r
''', 'val a = "x"\nval b = a.wrap("[", "]")\nprint(b)', out="[x]")
T(C, "22_metoda_s_cyklem_nad_retezcem", '''
extension (s: String)
  def rep(n: Int): String =
    var r = ""
    var i = 0
    while i < n do
      r = r + s
      i = i + 1
    r
''', 'val a = "ab"\nval b = a.rep(3)\nprint(b)', out="ababab")
T(C, "23_metoda_s_double_parametrem_a_int_argumentem", '''
extension (d: Double)
  def scale(f: Double): Double = d * f
''', 'val d = 1.25\nval r = d.scale(2)\nval s = toStr(r)\nprint(s)', out="2.50",
  note="Int argument se implicitně převede na Double parametr")
T(C, "24_navratova_hodnota_int_pro_double_typ", '''
extension (v: Int)
  def asDouble: Double = v
''', '''
  val x = 3
  val r = x.asDouble
  if r == 3.0 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "25_receiver_int_double_vysledek", '''
extension (v: Int)
  def half: Double = v / 2.0
''', 'val x = 5\nval r = x.half\nval s = toStr(r)\nprint(s)', out="2.50")
T(C, "26_prijemce_se_nemeni_po_volani", '''
extension (v: Int)
  def bump: Int =
    var t = v
    t = t + 100
    t
''', '''
  var x = 1
  val a = x.bump
  x = 5
  val b = x.bump
  print(a)
  print(",")
  print(b)
  print(",")
  print(x)
  ''', out="101,105,5")
T(C, "27_metoda_s_return_bez_else_if_retezce", '''
extension (v: Int)
  def kind: String =
    if v < 0 then
      return "neg"
    else if v == 0 then
      return "zero"
    else
      return "pos"
''', '''
  val a = 0 - 3
  val b = 0
  val c = 9
  val r1 = a.kind
  val r2 = b.kind
  val r3 = c.kind
  print(r1)
  print(r2)
  print(r3)
  ''', out="negzeropos")

# ===========================================================================
# 02 viditelnost a pořadí definic
# ===========================================================================
C = "02_viditelnost_a_poradi"
RAW(C, "01_metoda_definovana_az_po_main", '''
@main
def main(): Unit =
  val x = 6
  val y = x.twice
  print(y)

extension (v: Int)
  def twice: Int = v * 2
''', out="12")
RAW(C, "02_metoda_mezi_funkcemi_a_pouziti_driv", '''
def useIt(x: Int): Int =
  val r = x.twice
  r + 1

extension (v: Int)
  def twice: Int = v * 2

@main
def main(): Unit =
  val r = useIt(5)
  print(r)
''', out="11")
RAW(C, "03_dva_bloky_pro_stejny_typ_ruzne_metody", '''
extension (v: Int)
  def twice: Int = v * 2

extension (v: Int)
  def thrice: Int = v * 3

@main
def main(): Unit =
  val x = 2
  val a = x.twice
  val b = x.thrice
  print(a)
  print(b)
''', out="46")
RAW(C, "04_stejne_jmeno_pro_ruzne_typy", '''
extension (v: Int)
  def twice: Int = v * 2

extension (s: String)
  def twice: String = s + s

@main
def main(): Unit =
  val x = 12
  val a = x.twice
  val b = "ab".twice
  print(a)
  print(b)
''', out="24abab")
RAW(C, "05_metoda_vola_metodu_definovanou_pozdeji_v_bloku", '''
extension (v: Int)
  def a: Int = v.b + 1
  def b: Int = v * 10

@main
def main(): Unit =
  val x = 2
  val r = x.a
  print(r)
''', out="21")
RAW(C, "06_metoda_vola_metodu_z_pozdejsiho_bloku", '''
extension (v: Int)
  def a: Int = v.b + 1

extension (v: Int)
  def b: Int = v * 10

@main
def main(): Unit =
  val x = 3
  val r = x.a
  print(r)
''', out="31")
RAW(C, "07_vzajemna_rekurze_metod", '''
extension (n: Int)
  def evenFlag: Int =
    if n == 0 then
      return 1
    else
      return (n - 1).oddFlag
  def oddFlag: Int =
    if n == 0 then
      return 0
    else
      return (n - 1).evenFlag

@main
def main(): Unit =
  val a = 10
  val b = 7
  val r1 = a.evenFlag
  val r2 = b.evenFlag
  print(r1)
  print(r2)
''', out="10")
RAW(C, "08_metoda_a_funkce_se_stejnym_telem_ruzne_jmeno", '''
def plusOne(x: Int): Int = x + 1

extension (v: Int)
  def plusTwo: Int = v + 2

@main
def main(): Unit =
  val x = 1
  val a = plusOne(x)
  val b = x.plusTwo
  print(a)
  print(b)
''', out="23")
RAW(C, "09_metoda_pro_double_a_string_a_int_dohromady", '''
extension (v: Int)
  def describe: String = "int"

extension (v: Double)
  def describe: String = "double"

extension (v: String)
  def describe: String = "string"

@main
def main(): Unit =
  val a = 1
  val b = 1.5
  val c = "x"
  val r1 = a.describe
  val r2 = b.describe
  val r3 = c.describe
  print(r1)
  print(",")
  print(r2)
  print(",")
  print(r3)
''', out="int,double,string")
RAW(C, "10_komentare_mezi_metodami", '''
extension (v: Int)
  // první
  def twice: Int = v * 2
  /* druhá */
  def thrice: Int = v * 3

@main
def main(): Unit =
  val x = 1
  val a = x.twice
  val b = x.thrice
  print(a)
  print(b)
''', out="23")

# ===========================================================================
# 03 pořadí vyhodnocení a typy (bez FUNEXP)
# ===========================================================================
C = "03_vyhodnoceni_a_typy"
T(C, "01_metoda_se_provede_pri_kazdem_volani", '''
extension (v: Int)
  def noisy: Int =
    print("n")
    v
''', '''
  val x = 1
  val a = x.noisy
  val b = x.noisy
  val c = x.noisy
  print(a + b + c)
  ''', out="nnn3")
T(C, "02_vedlejsi_efekty_v_poradi_zleva_doprava", '''
extension (v: Int)
  def tagA: Int =
    print("a")
    v
  def tagB: Int =
    print("b")
    v
''', '''
  val x = 1
  val r = x.tagA + x.tagB * x.tagA
  print("|")
  print(r)
  ''', out="aba|2")
T(C, "03_telo_metody_vidi_jen_prijemce_a_parametry", '''
extension (v: Int)
  def f(a: Int): Int =
    val t = v * 10
    t + a
''', 'val x = 3\nval r = x.f(4)\nprint(r)', out="34")
T(C, "04_parametr_metody_stejneho_jmena_jako_promenna_volajiciho", '''
extension (v: Int)
  def f(a: Int): Int = v + a
''', 'val a = 100\nval x = 1\nval r = x.f(5)\nprint(r)', out="6")
T(C, "05_promenna_v_main_stejneho_jmena_jako_receiver", '''
extension (value: Int)
  def twice: Int = value * 2
''', 'val value = 7\nval r = value.twice\nprint(r)', out="14")
T(C, "06_vysledek_metody_do_promenne_s_typem", TWICE, '''
  val x = 4
  val r: Int = x.twice
  print(r)
  ''', out="8")
T(C, "07_metoda_v_podmince_if_a_while", TWICE, '''
  val x = 3
  if x.twice > 5 then
    print("big")
  else
    print("small")
  var i = 0
  while i.twice < 6 do
    i = i + 1
  print(i)
  ''', out="big3")
T(C, "08_metoda_se_nezamenuje_s_promennou_stejneho_jmena", TWICE, '''
  val twice = 99
  val x = 5
  val r = x.twice
  print(r)
  print(",")
  print(twice)
  ''', out="10,99")
T(C, "09_navratovy_typ_string_a_porovnani", '''
extension (v: Int)
  def label: String = "n"
''', '''
  val x = 1
  val s = x.label
  if s == "n" then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "10_ulozeni_do_var_a_prepsani", TWICE, '''
  var x = 1
  x = x.twice
  x = x.twice
  x = x.twice
  print(x)
  ''', out="8")
T(C, "11_metoda_s_mnoha_parametry", '''
extension (v: Int)
  def f(a: Int, b: Int, c: Int): Int = v * 1000 + a * 100 + b * 10 + c
''', 'val x = 1\nval r = x.f(2, 3, 4)\nprint(r)', out="1234")
T(C, "12_receiver_string_s_metodou_vracejici_int", '''
extension (s: String)
  def wordCount: Int =
    var n = 0
    if strLength(s) > 0 then
      n = 1
    n
''', 'val a = "x"\nval b = ""\nval r1 = a.wordCount\nval r2 = b.wordCount\n'
    'print(r1)\nprint(r2)', out="10")

# ===========================================================================
# 05 chyby
# ===========================================================================
C = "04_chyby"
NOARG = (3, 5, 6)
T(C, "01_metoda_s_prazdnym_seznamem_volana_bez_zavorek", EXT_INT,
  'val x = 6\nval r = x.foo\nprint("x")', crc=NOARG,
  note="zadání: nelze x.foo; kód není v zadání uveden jednoznačně")
T(C, "02_bezparametricka_metoda_volana_se_zavorkami", EXT_INT,
  'val x = 6\nval r = x.twice()\nprint("x")', crc=NOARG,
  note="zadání: nelze x.twice()")
T(C, "03_chybi_argument", EXT_INT,
  'val x = 6\nval r = x.plus()\nprint("x")', crc=5)
T(C, "04_moc_argumentu", EXT_INT,
  'val x = 6\nval r = x.plus(1, 2)\nprint("x")', crc=5)
T(C, "05_spatny_typ_argumentu", EXT_INT,
  'val x = 6\nval r = x.plus("a")\nprint("x")', crc=5)
T(C, "06_neexistujici_metoda", EXT_INT,
  'val x = 6\nval r = x.nope\nprint("x")', crc=3)
T(C, "07_metoda_neni_definovana_pro_typ_prijemce", EXT_INT,
  'val s = "abc"\nval r = s.twice\nprint("x")', crc=(3, 6))
T(C, "08_metoda_pro_int_na_double", EXT_INT,
  'val d = 1.5\nval r = d.twice\nprint("x")', crc=(3, 6),
  note="příjemce Double není Int, žádná implicitní konverze příjemce")
T(C, "09_duplicitni_metoda_ve_stejnem_bloku", '''
extension (v: Int)
  def twice: Int = v * 2
  def twice: Int = v + v
''', 'print("x")', crc=4)
T(C, "10_duplicitni_metoda_ve_dvou_blocich_stejneho_typu", '''
extension (v: Int)
  def twice: Int = v * 2

extension (w: Int)
  def twice: Int = w + w
''', 'print("x")', crc=4)
T(C, "11_duplicita_s_jinym_seznamem_parametru", '''
extension (v: Int)
  def f: Int = v

extension (v: Int)
  def f(a: Int): Int = v + a
''', 'print("x")', crc=4, note="přetěžování není podporováno")
T(C, "12_zapis_do_prijemce", '''
extension (v: Int)
  def bad: Int =
    v = 5
    v
''', 'print("x")', crc=7)
T(C, "13_zapis_do_parametru_metody", '''
extension (v: Int)
  def bad(a: Int): Int =
    a = 5
    v + a
''', 'print("x")', crc=7)
RAW(C, "14_prijemce_bez_typu", '''
extension (value)
  def twice: Int = value * 2

@main
def main(): Unit =
  print("x")
''', crc=2)
RAW(C, "15_dva_prijemci", '''
extension (a: Int, b: Int)
  def sum: Int = a + b

@main
def main(): Unit =
  print("x")
''', crc=2)
T(C, "16_extension_uvnitr_funkce", "", 'extension (v: Int)\n  def twice: Int = v * 2\nprint("x")', crc=2,
  note="extension se definuje jen na nejvyšší úrovni")
RAW(C, "17_prazdny_blok_extension", '''
extension (v: Int)

@main
def main(): Unit =
  print("x")
''', crc=2)
RAW(C, "18_v_bloku_neni_def", '''
extension (v: Int)
  val x = 1

@main
def main(): Unit =
  print("x")
''', crc=2)
RAW(C, "19_def_neni_odsazena", '''
extension (v: Int)
def twice: Int = v * 2

@main
def main(): Unit =
  print("x")
''', crc=2)
T(C, "20_navratovy_typ_neodpovida_tele", '''
extension (v: Int)
  def twice: String = v * 2
''', 'print("x")', crc=6)
T(C, "21_chybny_typ_v_tele_metody", '''
extension (v: Int)
  def bad: Int = v + "a"
''', 'print("x")', crc=6)
T(C, "22_rekurze_bez_navratoveho_typu", '''
extension (v: Int)
  def loop = v.loop
''', 'print("x")', crc=6)
T(C, "23_vysledek_metody_do_spatneho_typu", TWICE,
  'val x = 1\nval s: String = x.twice\nprint("x")', crc=6)
T(C, "24_dalsi_metoda_na_vysledku_neexistuje", TWICE,
  'val x = 1\nval r = x.twice.nope\nprint("x")', crc=3)
T(C, "25_metoda_volana_jako_funkce", TWICE,
  'val x = 1\nval r = twice(x)\nprint("x")', crc=3,
  note="rozšiřující metodu lze volat výhradně členským zápisem")
RAW(C, "26_neznamy_typ_prijemce", '''
extension (v: Foo)
  def twice: Int = 1

@main
def main(): Unit =
  print("x")
''', crc=3)
T(C, "27_extension_jako_identifikator", "",
  'val extension = 1\nprint("x")', crc=2)
T(C, "28_pozdejsi_nedefinovana_promenna_v_tele_metody", '''
extension (v: Int)
  def bad: Int = v + nope
''', 'print("x")', crc=3)
T(C, "29_prijemce_je_nedefinovana_promenna", TWICE,
  'val r = nope.twice\nprint("x")', crc=3)

# ===========================================================================
# 99 kombinace s dalšími rozšířeními
# ===========================================================================
C = "99_kombinace_funexp"
T(C, "01_argument_je_vyraz", EXT_INT,
  'val x = 6\nprint(toStr(x.plus(1 + 2)))', out="9")
T(C, "02_prijemce_je_vysledek_volani", D("def dbl(x: Int): Int = x * 2", TWICE),
  'print(toStr(dbl(3).twice))', out="12")
T(C, "03_metoda_jako_argument_vestavene_funkce", TWICE,
  'val x = 6\nprint(toStr(x.twice))', out="12")
T(C, "04_vnoreni_metody_do_argumentu", EXT_INT,
  'val x = 6\nprint(toStr(x.plus(x.twice)))', out="18")
T(C, "05_prijemce_vyhodnocen_pred_argumenty", D(TR, EXT_INT), '''
  val r = tr("r", 1).plus(tr("a", 2))
  print("|")
  print(r)
  ''', out="ra|3")
T(C, "06_argumenty_zleva_doprava", D(TR, '''
extension (v: Int)
  def span(lo: Int, hi: Int): Int = hi - lo + v
'''), '''
  val x = 1
  val r = x.span(tr("a", 0), tr("b", 10))
  print("|")
  print(r)
  ''', out="ab|11")
T(C, "07_prijemce_je_readInt", TWICE,
  'print(toStr(readInt().twice))', inp="4\n", out="8")
T(C, "08_metoda_v_podmince_s_volanim", D(TWICE, "def dbl(x: Int): Int = x * 2"),
  '''
  val x = 3
  if dbl(x).twice > 10 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "09_retezeni_volani_a_metod", D(EXT_INT, "def dbl(x: Int): Int = x * 2"),
  'print(toStr(dbl(2).plus(dbl(1)).twice))', out="12")
T(C, "10_vestavena_funkce_v_tele_metody", '''
extension (s: String)
  def len2: Int = strLength(s) * 2
''', 'val a = "abc"\nprint(toStr(a.len2))', out="6")
T(C, "11_metoda_uvnitr_argumentu_uzivatelske_funkce",
  D(TWICE, "def add(a: Int, b: Int): Int = a + b"),
  'val x = 2\nprint(toStr(add(x.twice, x.twice)))', out="8")

C = "99_kombinace_valmem"
RAW(C, "01_priklad_ze_zadani_s_toString", '''
extension (value: Int)
  // bezparametrická metoda
  def twice: Int = value * 2
  // metoda s prázdným seznamem parametrů
  def foo(): String = "foo" + value.toString
  // metoda s jedním parametrem
  def plus(other: Int): Int = value + other

def someFn(): Unit =
  val x: Int = 6
  val y = x.twice
  val z = x.foo()
  val w = x.plus(12)
  print(y)
  print(z)
  print(w)

@main
def main(): Unit =
  someFn()
''', out="12foo618")
T(C, "02_clen_na_vysledku_metody", '''
extension (s: String)
  def shout: String = s + "!"
''', 'val n = "abc".shout.length\nprint(n)', out="4")
T(C, "03_floor_a_toInt_v_metode", '''
extension (d: Double)
  def rounded: Int = (d + 0.5).floor.toInt
''', 'val a = 2.4\nval b = 2.5\nval r1 = a.rounded\nval r2 = b.rounded\n'
    'print(r1)\nprint(r2)', out="23")
T(C, "04_toString_na_vysledku_metody", TWICE,
  'val x = 6\nval l = x.twice.toString.length\nprint(l)', out="2")
T(C, "05_length_v_metode_na_stringu", '''
extension (s: String)
  def doubled: Int = s.length * 2
''', 'val n = "abcd".doubled\nprint(n)', out="8")

C = "99_kombinace_boolthen"
T(C, "01_metoda_na_boolean", '''
extension (b: Boolean)
  def num: Int =
    if b then
      return 1
    else
      return 0
''', 'val x = true\nval y = false\nval a = x.num\nval c = y.num\n'
    'print(a)\nprint(c)', out="10")
T(C, "02_metoda_vraci_boolean_a_pouzije_se_v_podmince", '''
extension (n: Int)
  def isPositive: Boolean = n > 0
''', '''
  val a = 5
  val b = 0 - 5
  if a.isPositive then
    print("a")
  if b.isPositive then
    print("b")
  print("|")
  ''', out="a|")
T(C, "03_zkracene_vyhodnoceni_s_metodami", '''
extension (n: Int)
  def noisy: Boolean =
    print("n")
    n > 0
''', '''
  val a = 0 - 1
  val b = 1
  val r = a.noisy && b.noisy
  print("|")
  print(r)
  ''', out="n|false")
T(C, "04_parametr_boolean", '''
extension (n: Int)
  def pick(flag: Boolean): Int =
    if flag then
      return n
    else
      return 0
''', 'val x = 7\nval a = x.pick(true)\nval b = x.pick(false)\n'
    'print(a)\nprint(b)', out="70")

C = "99_kombinace_control"
T(C, "01_if_vyraz_v_tele_metody", '''
extension (v: Int)
  def abs2: Int = if v < 0 then 0 - v else v
''', 'val x = 0 - 7\nval r = x.abs2\nprint(r)', out="7")
T(C, "02_metoda_v_mezi_cyklu_for", TWICE,
  'val x = 2\nfor i <- 1 to x.twice do print(i)', out="1234")
T(C, "03_metoda_ve_strazi_cyklu", '''
extension (n: Int)
  def isEven: Boolean = n / 2 * 2 == n
''', 'for i <- 1 to 6 if i.isEven do print(i)', out="246")
T(C, "04_cyklus_for_v_tele_metody", '''
extension (n: Int)
  def countdown: Unit =
    for i <- 1 to n do
      print(i)
''', 'val x = 3\nx.countdown', out="123")
T(C, "05_if_vyraz_vybira_metodu", TWICE, '''
  val x = 4
  val r = if x > 3 then x.twice else x
  print(r)
  ''', out="8")

C = "99_kombinace_paramlists"
T(C, "01_vice_seznamu_v_metode", '''
extension (v: Int)
  def add(a: Int)(b: Int): Int = v + a + b
''', 'val x = 1\nval r = x.add(10)(100)\nprint(r)', out="111")
T(C, "02_prazdny_prvni_seznam", '''
extension (v: Int)
  def f()(b: Int): Int = v * 10 + b
''', 'val x = 4\nval r = x.f()(5)\nprint(r)', out="45")
T(C, "03_castecna_aplikace_je_chyba", '''
extension (v: Int)
  def add(a: Int)(b: Int): Int = v + a + b
''', 'val x = 1\nval r = x.add(10)\nprint("x")', crc=5)
T(C, "04_chybny_pocet_seznamu_navic", '''
extension (v: Int)
  def add(a: Int): Int = v + a
''', 'val x = 1\nval r = x.add(10)(20)\nprint("x")', crc=(3, 5, 6))

C = "99_kombinace_interpolateds"
T(C, "01_metoda_v_interpolaci", TWICE,
  'val x = 6\nprint(s"${x.twice}")', out="12")
T(C, "02_metoda_vraci_interpolaci", '''
extension (n: Int)
  def tag: String = s"<$n>"
''', 'val x = 5\nval r = x.tag\nprint(r)', out="<5>")


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
