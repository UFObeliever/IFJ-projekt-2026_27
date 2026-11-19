#!/usr/bin/env python3
"""
funexp_tests.py - testy rozšíření FUNEXP (funkce ve výrazech) pro překladač IFJ26.

Vygenerováno AI (Claude, Anthropic).
Prompt: "Dokážeš prosím udělat nějaké testy pouze na funexp rozšíření prosím?
         a ať se spustí jako ./funexp_tests.py"

Co se testuje (zadání, kapitola 5.1 FUNEXP):
  - volání vestavěné i uživatelské funkce může být součástí výrazu,
  - argumentem funkce může být libovolný výraz,
  - volání lze vnořovat,
  - argumenty (a operandy) se vyhodnocují zleva doprava,
  - chyby: typy, počet/typ argumentů, nedefinované funkce, syntaxe, lexika.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup.

Příklady:
  ./funexp_tests.py                                  # všechny testy
  ./funexp_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./funexp_tests.py -f poradi                        # jen testy s "poradi" v názvu
  ./funexp_tests.py --list                           # jen vypíše názvy testů
  ./funexp_tests.py --keep out_dir                   # uloží .ifj26 a .code
  ./funexp_tests.py --export scala_out               # uloží .scala/.in/.out pro
                                                     # ověření skutečnou Scalou

Interpret se bere z --interp (lze i s přepínači v uvozovkách), jinak z ./ic26int
vedle skriptu a nakonec z PATH.
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
        self.crc = crc      # očekávaný návratový kód překladače (0 = přeloží se)
        self.note = note

    @property
    def full(self):
        return "%s/%s" % (self.cat, self.name)


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
    CASES.append(Case(cat, name, prog(defs, body), out, inp, rc, crc, note))


def RAW(cat, name, source, out="", inp="", rc=0, crc=0, note=""):
    src = PROLOG + textwrap.dedent(source).lstrip("\n")
    CASES.append(Case(cat, name, src, out, inp, rc, crc, note))


# ---------------------------------------------------------------------------
# pomocné funkce používané v testech
# ---------------------------------------------------------------------------
DBL = "def dbl(x: Int): Int = x * 2"
ADD = "def add(a: Int, b: Int): Int = a + b"
ADD3 = "def add3(a: Int, b: Int, c: Int): Int = a + b + c"
SUB = "def sub(a: Int, b: Int): Int = a - b"
HD = "def hd(x: Double): Double = x / 2.0"
S2 = "def s2(x: String): String = x + x"
# tr vypíše značku a vrátí hodnotu -> z výstupu je vidět pořadí vyhodnocení
TR = """
def tr(t: String, v: Int): Int =
  print(t)
  v
"""


def D(*parts):
    return "\n\n".join(textwrap.dedent(p).strip("\n") for p in parts)


# ===========================================================================
# 01 volání jako operandy aritmetických výrazů
# ===========================================================================
C = "01_aritmetika"
T(C, "01_volani_jako_pravy_operand", DBL,
  'print(toStr(1 + dbl(4)))', out="9")
T(C, "02_volani_jako_levy_operand", DBL,
  'print(toStr(dbl(4) + 1))', out="9")
T(C, "03_dve_volani_nasobeni", DBL,
  'print(toStr(dbl(2) * dbl(3)))', out="24")
T(C, "04_priorita_nasobeni_pred_scitanim", DBL,
  'print(toStr(2 * dbl(3) + 1) + " " + toStr(1 + 2 * dbl(3)))', out="13 13")
T(C, "05_priorita_pri_odcitani", DBL,
  'print(toStr(10 - dbl(2) * dbl(2)))', out="-6")
T(C, "06_zavorky_kolem_volani", DBL,
  'print(toStr((dbl(1) + dbl(2)) * dbl(3)))', out="36")
T(C, "07_unarni_minus_na_volani", DBL,
  'print(toStr(-dbl(3)) + " " + toStr(-dbl(3) + 10) + " " + '
  'toStr(-(dbl(1) + 1)))', out="-6 4 -3")
T(C, "08_deleni_vysledku_volani", "def half(x: Int): Int = x / 2",
  'print(toStr(half(9) + 100 / half(10)))', out="24")
T(C, "09_dlouhy_retez_operatoru", DBL,
  'print(toStr(dbl(1) + dbl(2) * dbl(3) - dbl(4) / dbl(2)))', out="24")
T(C, "10_volani_bez_argumentu", D("def one(): Int = 1", "def zero(): Int = 0"),
  'print(toStr(zero() + one() * 2 - one()))', out="1")

# ===========================================================================
# 02 argumenty jsou libovolné výrazy, vnořování
# ===========================================================================
C = "02_argumenty"
T(C, "01_argumenty_jsou_vyrazy", ADD,
  'print(toStr(add(1 + 2, 3 * 4)))', out="15")
T(C, "02_promenne_ve_vyrazech_v_argumentech", ADD,
  '''
  val a = 5
  val b = 3
  print(toStr(add(a * 2, b - 1)))
  ''', out="12")
T(C, "03_vnorena_volani_jako_argumenty", D(ADD, DBL),
  'print(toStr(add(dbl(2), dbl(3))))', out="10")
T(C, "04_hluboke_vnoreni", DBL,
  'print(toStr(dbl(dbl(dbl(dbl(1))))))', out="16")
T(C, "05_smisene_vnoreni", ADD,
  'print(toStr(add(add(1, 2), add(3, 4) * 2)))', out="17")
T(C, "06_unarni_minus_v_argumentech", D(ADD, DBL),
  'print(toStr(add(-(1 + 2), -dbl(2))))', out="-7")
T(C, "07_vyraz_s_volanim_uvnitr_argumentu", D(ADD, DBL),
  'print(toStr(add(dbl(add(1, 1)) + 1, 2)))', out="7")
T(C, "08_tri_argumenty", D(ADD3, DBL),
  'print(toStr(add3(1 + 1, dbl(2), 10 - 4)))', out="12")
T(C, "09_retezcove_argumenty",
  "def join(a: String, b: String): String = a + b",
  'print(join("x" + "y", toStr(2 * 2)))', out="xy4")
T(C, "10_argument_je_volani_vestavene_funkce", DBL,
  'print(toStr(dbl(strLength("abc") + 1)))', out="8")
T(C, "11_argumenty_s_odsazenim_a_zavorkami", D(ADD, DBL),
  'print(toStr(add((dbl(1) + 1) * 2, (dbl(2)))))', out="10")

# ===========================================================================
# 03 vestavěné funkce ve výrazech (včetně příkladů ze zadání)
# ===========================================================================
C = "03_vestavene"
FACT_ITER = """
def factorial(n: Int): Int =
  var result = 1
  var i: Int = n
  while i > 1 do
    result = result * i
    i = i - 1
  result
"""
T(C, "01_priklad_ze_zadani_factorial_readInt", FACT_ITER,
  '''
  val result = 2 * factorial(readInt() + 1)
  print(toStr(result))
  ''', inp="4\n", out="240")
T(C, "02_priklad_ze_zadani_strLength_subStr", FACT_ITER,
  '''
  val result = 2 * factorial(readInt() + 1)
  print(toStr(result + strLength(subStr("abcd", 1, 3))))
  ''', inp="4\n", out="242")
T(C, "03_strLength_subStr_ve_vyrazu", "",
  'print(toStr(strLength(subStr("abcdef", 1, 4)) * 2))', out="6")
T(C, "04_ord_ve_vyrazu", "",
  'print(toStr(ord("A", 0) + 1))', out="66")
T(C, "05_chr_s_vyrazem_v_argumentu", "",
  'print(chr(65 + 1) + chr(ord("a", 0) + 1))', out="Bb")
T(C, "06_toInt_s_vyrazem_v_argumentu", "",
  'print(toStr(toInt(1.5 + 2.9) * 2))', out="8")
T(C, "07_konkatenace_s_volanimi", DBL,
  'print("n=" + toStr(dbl(21)) + "!")', out="n=42!")
T(C, "08_strLength_z_konkatenace", "",
  'print(toStr(strLength("ab" + toStr(123))))', out="5")
T(C, "09_print_s_volanim_uzivatelske_funkce", DBL,
  'print(dbl(4))\nprint(dbl(2) + dbl(1))', out="86")
T(C, "10_readDouble_ve_vyrazu", "",
  'print(toStr(toInt(readDouble() * 2.0)))', inp="2.5\n", out="5")
T(C, "11_readInt_ve_vyrazu", "",
  'print(toStr(readInt() + 1))', inp="20\n", out="21")
T(C, "12_toStr_a_volani_v_retezci", D(DBL, ADD),
  'print(toStr(add(1, 2)) + "-" + toStr(dbl(3)) + "-" + toStr(add(dbl(1), 1)))',
  out="3-6-3")

# ===========================================================================
# 04 pořadí vyhodnocení zleva doprava
# ===========================================================================
C = "04_poradi_vyhodnoceni"
T(C, "01_argumenty_zleva_doprava", D(TR, ADD3),
  '''
  val r = add3(tr("a", 1), tr("b", 2), tr("c", 3))
  print("|" + toStr(r))
  ''', out="abc|6")
T(C, "02_operandy_zleva_doprava", TR,
  '''
  val r = tr("a", 1) + tr("b", 2) * tr("c", 3)
  print("|" + toStr(r))
  ''', out="abc|7")
T(C, "03_operandy_porovnani", TR,
  '''
  if tr("x", 1) < tr("y", 2) then
    print("|yes")
  else
    print("|no")
  ''', out="xy|yes")
T(C, "04_vnorene_vlevo", D(TR, ADD),
  'print("|" + toStr(add(add(tr("a", 1), tr("b", 2)), tr("c", 3))))',
  out="abc|6")
T(C, "05_vnorene_vpravo", D(TR, ADD),
  '''
  val r = add(tr("a", 1), add(tr("b", 2), tr("c", 3)))
  print("|" + toStr(r))
  ''', out="abc|6")
T(C, "06_vyrazy_v_argumentech", D(TR, ADD),
  '''
  val r = add(tr("a", 1) + tr("b", 1), tr("c", 1) + tr("d", 1))
  print("|" + toStr(r))
  ''', out="abcd|4")
T(C, "07_argument_se_vyhodnoti_prave_jednou",
  D(TR, "def sq(x: Int): Int = x * x"),
  '''
  val r = sq(tr("a", 3))
  print("|" + toStr(r))
  ''', out="a|9")
T(C, "08_vnitrni_volani_pred_vnejsim",
  D(TR, '''
def outer(x: Int): Int =
  print("o")
  x
'''),
  '''
  val r = outer(tr("i", 1)) + tr("j", 2)
  print("|" + toStr(r))
  ''', out="ioj|3")
T(C, "09_cteni_vstupu_zleva_doprava", "",
  'print(toStr(readInt() - readInt()))', inp="10\n3\n", out="7")
T(C, "10_cteni_vstupu_v_argumentech", SUB,
  'print(toStr(sub(readInt(), readInt())))', inp="10\n3\n", out="7")
T(C, "11_cteni_vstupu_ve_slozitem_vyrazu", "",
  'print(toStr(readInt() * 10 + readInt()))', inp="4\n2\n", out="42")
T(C, "12_cteni_vstupu_vnorene", D(SUB, DBL),
  'print(toStr(sub(dbl(readInt()), readInt())))', inp="5\n7\n", out="3")

# ===========================================================================
# 05 porovnání a podmínky if/while s voláním
# ===========================================================================
C = "05_porovnani_podminky"
T(C, "01_vsechny_relacni_operatory_s_volanim", DBL,
  '''
  if dbl(2) == 4 then
    print("a")
  else
    print("A")
  if dbl(2) != dbl(3) then
    print("b")
  else
    print("B")
  if dbl(3) > dbl(2) then
    print("c")
  else
    print("C")
  if dbl(2) <= dbl(2) then
    print("d")
  else
    print("D")
  if dbl(1) >= dbl(2) then
    print("E")
  else
    print("e")
  if dbl(1) < dbl(2) then
    print("f")
  else
    print("F")
  ''', out="abcdef")
T(C, "02_porovnani_retezcu_z_volani", "",
  '''
  if strLength("abc") < strLength("abcd") then
    print("y")
  else
    print("n")
  if toStr(5) == "5" then
    print("y")
  else
    print("n")
  ''', out="yy")
T(C, "03_porovnani_int_s_double", DBL,
  '''
  if dbl(2) < 4.5 then
    print("y")
  else
    print("n")
  if dbl(2) == 4.0 then
    print("y")
  else
    print("n")
  ''', out="yy")
T(C, "04_aritmetika_v_podmince", DBL,
  '''
  if dbl(2) + 1 > dbl(1) * 2 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "05_while_s_volanim_v_podmince", DBL,
  '''
  var i = 0
  while dbl(i) < 10 do
    print(toStr(i))
    i = i + 1
  ''', out="01234")
T(C, "06_podminka_cyklu_se_vyhodnocuje_opakovane", TR,
  '''
  var i = 0
  while tr("c", i) < 3 do
    i = i + 1
  ''', out="cccc")
T(C, "07_volani_ve_vetvich_if_else", DBL,
  '''
  if dbl(1) < dbl(2) then
    print(toStr(dbl(10)))
  else
    print(toStr(dbl(20)))
  ''', out="20")
T(C, "08_vnorene_volani_v_podmince", D(ADD, DBL),
  '''
  if add(dbl(1), dbl(2)) == 6 then
    print("y")
  else
    print("n")
  ''', out="y")

# ===========================================================================
# 06 volání ve všech místech, kde může být výraz
# ===========================================================================
C = "06_kontexty"
T(C, "01_inicializace_a_prirazeni", DBL,
  '''
  val a = dbl(3) + 1
  var b = dbl(a) - 2
  b = b + dbl(b)
  print(toStr(a) + " " + toStr(b))
  ''', out="7 36")
T(C, "02_return_s_vyrazem_z_volani",
  D(DBL, '''
def f(x: Int): Int =
  return dbl(x) + dbl(x + 1)
'''),
  'print(toStr(f(2)))', out="10")
T(C, "03_posledni_vyraz_tela_funkce",
  D(DBL, '''
def g(x: Int): Int =
  val t = dbl(x)
  t + dbl(t)
'''),
  'print(toStr(g(1)))', out="6")
T(C, "04_jednoradkove_telo_s_volanimi",
  D(DBL, "def h(x: Int): Int = dbl(x) + dbl(x + 1)"),
  'print(toStr(h(2)))', out="10")
T(C, "05_odvozeny_navratovy_typ_z_vyrazu_s_volanim",
  D(DBL, "def m(x: Int) = dbl(x) * 2"),
  'print(toStr(m(3)))', out="12")
T(C, "06_return_ve_vetvich_if",
  D(DBL, '''
def abs2(x: Int): Int =
  if x < 0 then
    return -dbl(x)
  else
    return dbl(x)
'''),
  'print(toStr(abs2(-2)) + " " + toStr(abs2(3)))', out="4 6")
T(C, "07_soucet_v_cyklu", DBL,
  '''
  var s = 0
  var i = 0
  while i < 5 do
    s = s + dbl(i)
    i = i + 1
  print(toStr(s))
  ''', out="20")
T(C, "08_vestavena_funkce_v_inicializaci", DBL,
  '''
  val n = strLength("hello") * dbl(2)
  print(toStr(n))
  ''', out="20")
T(C, "09_prirazeni_s_volanim_na_obou_stranach_operatoru", D(ADD, DBL),
  '''
  var v = 1
  v = add(v, dbl(v)) * add(1, 1)
  print(toStr(v))
  ''', out="6")
T(C, "10_volani_jako_samostatny_prikaz_a_ve_vyrazu", TR,
  '''
  tr("a", 1)
  val r = tr("b", 2) + 1
  print("|" + toStr(r))
  ''', out="ab|3")

# ===========================================================================
# 07 rekurze, vzájemná rekurze a dopředná volání ve výrazech
# ===========================================================================
C = "07_rekurze_dopredna"
T(C, "01_faktorial_rekurzivne",
  '''
def fact(n: Int): Int =
  if n <= 1 then
    return 1
  else
    return n * fact(n - 1)
''',
  'print(toStr(fact(5)))', out="120")
T(C, "02_fibonacci",
  '''
def fib(n: Int): Int =
  if n < 2 then
    return n
  else
    return fib(n - 1) + fib(n - 2)
''',
  'print(toStr(fib(10)))', out="55")
T(C, "03_mccarthy_91_vnorena_rekurze_v_argumentech",
  '''
def mc(n: Int): Int =
  if n > 100 then
    return n - 10
  else
    return mc(mc(n + 11))
''',
  'print(toStr(mc(50)) + " " + toStr(mc(150)))', out="91 140")
T(C, "04_gcd_s_vyrazem_v_argumentu",
  '''
def gcd(a: Int, b: Int): Int =
  if b == 0 then
    return a
  else
    return gcd(b, a - (a / b) * b)
''',
  'print(toStr(gcd(48, 18)))', out="6")
T(C, "05_vzajemna_rekurze",
  '''
def ev(n: Int): Int =
  if n == 0 then
    return 1
  else
    return od(n - 1)

def od(n: Int): Int =
  if n == 0 then
    return 0
  else
    return ev(n - 1)
''',
  'print(toStr(ev(10)) + toStr(od(7)) + toStr(ev(7)))', out="110")
RAW(C, "06_dopredne_volani_ve_vyrazu_main_prvni", '''
@main
def main(): Unit =
  print(toStr(later(3) * 2 + later(1)))

def later(x: Int): Int = x + 100
''', out="307")
RAW(C, "07_dopredne_volani_odvozeny_typ_ve_vyrazu", '''
@main
def main(): Unit =
  print(toStr(twice(4) + 1))

def twice(x: Int) = x * 2
''', out="9",
    note="vyžaduje on-demand analýzu funkce s odvozeným typem v parse_call")
RAW(C, "08_retez_odvozenych_typu_dopredu", '''
def fa(x: Int) = fb(x) + 1

@main
def main(): Unit =
  print(toStr(fa(3)))

def fb(x: Int) = x * 2
''', out="7",
    note="vyžaduje on-demand analýzu funkce s odvozeným typem v parse_call")
RAW(C, "09_dopredne_volani_v_argumentech", '''
@main
def main(): Unit =
  print(toStr(plus(inc(1), inc(2) * 2)))

def plus(a: Int, b: Int): Int = a + b

def inc(x: Int): Int = x + 1
''', out="8")

# ===========================================================================
# 08 typy: Int/Double, řetězce
# ===========================================================================
C = "08_typy"
T(C, "01_int_vyraz_na_double_parametr", HD,
  '''
  if hd(1 + 2) == 1.5 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "02_vnorene_volani_na_double_parametr", D(HD, DBL),
  '''
  if hd(dbl(2)) == 2.0 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "03_int_funkce_v_double_vyrazu", DBL,
  'print(toStr(toInt(dbl(3) * 1.5)) + " " + '
  'toStr(toInt(dbl(3) / 4.0 * 10.0)))', out="9 15")
T(C, "04_soucet_double_vysledku", HD,
  'print(toStr(toInt(hd(3) + hd(5))))', out="4")
T(C, "05_toStr_double_s_desetinnou_casti", HD,
  'print(toStr(hd(3)))', out="1.50",
  note="toStr formátuje Double s nenulovou desetinnou částí jako %.2f")
T(C, "06_retezcove_funkce_ve_vyrazu", S2,
  'print(s2("a") + s2("b"))', out="aabb")
T(C, "07_porovnani_retezcu_z_volani", S2,
  '''
  if s2("a") < s2("b") then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "08_retezec_a_cislo_v_argumentech",
  D(S2, DBL, "def cat(a: String, b: String): String = a + b"),
  'print(cat(s2("x") + "-", toStr(dbl(2))))', out="xx-4")
T(C, "09_int_volani_do_double_promenne", DBL,
  '''
  val d: Double = dbl(2) + 1
  print(toStr(toInt(d * 2.0)))
  ''', out="10")
T(C, "10_double_funkce_s_celociselnym_literalem", HD,
  '''
  val d = hd(5) + 1
  if d == 3.5 then
    print("y")
  else
    print("n")
  ''', out="y")

# ===========================================================================
# 09 chyby (musí skončit chybou překladače a nesmí vzniknout žádný výstup)
# ===========================================================================
C = "09_chyby"
T(C, "01_int_plus_string_z_volani", "",
  'val a = 1 + toStr(2)\nprint("x")', crc=6)
T(C, "02_string_krat_int_z_volani", DBL,
  'val a = "a" * dbl(2)\nprint("x")', crc=6)
T(C, "03_int_volani_do_string_promenne", DBL,
  'val a: String = dbl(2)\nprint("x")', crc=6)
T(C, "04_double_volani_do_int_promenne", HD,
  'val a: Int = hd(2)\nprint("x")', crc=6)
T(C, "05_unit_v_aritmetice", "",
  'val a = 1 + print("x")', crc=6)
T(C, "06_porovnani_int_s_retezcem", DBL,
  '''
  if dbl(2) < "a" then
    print("x")
  else
    print("y")
  ''', crc=6)
T(C, "07_spatny_typ_argumentu_ve_vnorenem_vyrazu", DBL,
  'print(toStr(dbl("a" + "b")))', crc=5)
T(C, "08_unit_jako_argument", DBL,
  'val a = dbl(print("x"))', crc=5)
T(C, "09_malo_argumentu_ve_vyrazu", ADD,
  'val a = 1 + add(2)', crc=5)
T(C, "10_moc_argumentu_ve_vnorenem_volani", D(ADD, DBL),
  'val a = add(1, dbl(2, 3))', crc=5)
T(C, "11_chybi_argument_ve_vyrazu", DBL,
  'val a = 2 * dbl()', crc=5)
T(C, "12_vestavena_funkce_spatny_typ", "",
  'val a = strLength(1 + 2)', crc=5)
T(C, "13_chr_s_retezcem", "",
  'print(chr("a"))', crc=5)
T(C, "14_retezec_misto_cisla_v_argumentu", ADD,
  'val a = add(1, toStr(2))', crc=5)
T(C, "15_nedefinovana_funkce_ve_vyrazu", "",
  'val a = 1 + nope(2)', crc=3)
T(C, "16_nedefinovana_funkce_v_argumentu", DBL,
  'val a = dbl(nope(2))', crc=3)
T(C, "17_nedefinovana_promenna_v_argumentu", DBL,
  'val a = dbl(x + 1)', crc=3)
T(C, "18_chybi_uzaviraci_zavorka", DBL,
  'print(toStr(dbl(1 + 2))', crc=2)
T(C, "19_chybi_operand_za_operatorem", DBL,
  'val a = dbl(2) *', crc=2)
T(C, "20_dve_volani_za_sebou_bez_operatoru", DBL,
  'val a = dbl(1) dbl(2)', crc=2)
T(C, "21_lexikalni_chyba_v_argumentu", DBL,
  'val a = dbl(1 + #)', crc=1)
RAW(C, "22_rekurze_bez_navratoveho_typu_ve_vyrazu", '''
def f(x: Int) = f(x - 1) + 1

@main
def main(): Unit =
  print("x")
''', crc=6)
RAW(C, "23_vzajemna_rekurze_bez_typu_ve_vyrazu", '''
def a(x: Int) = b(x) + 1

def b(x: Int) = a(x) + 1

@main
def main(): Unit =
  print(toStr(a(1)))
''', crc=6)


# ---------------------------------------------------------------------------
# runner
# ---------------------------------------------------------------------------
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
    for cand in (os.path.join(HERE, "..", "..", "compiler"),
                 os.path.join(HERE, "..", "compiler"),
                 os.path.join(HERE, "compiler"),
                 os.path.join(os.getcwd(), "compiler")):
        if os.path.isfile(cand) and os.access(cand, os.X_OK):
            return cand
    sys.exit("překladač nenalezen, použijte --compiler CESTA")


def find_interp(arg):
    if arg:
        parts = shlex.split(arg)
        parts[0] = check_exe(parts[0], "interpret")
        return parts
    local = os.path.join(HERE, "ic26int")
    if os.path.isfile(local) and os.access(local, os.X_OK):
        return [local]
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
    if c.crc != 0:
        if p.returncode != c.crc:
            return False, "rc překladače %d, očekáváno %d" % (p.returncode,
                                                              c.crc)
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


def export_scala(directory):
    os.makedirs(directory, exist_ok=True)
    n = 0
    for c in CASES:
        if c.crc != 0:
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
    ap.add_argument("--compiler", help="cesta k překladači (výchozí ../../compiler)")
    ap.add_argument("--interp", help="cesta k ic26int (lze i s přepínači)")
    ap.add_argument("-f", "--filter", default="",
                    help="jen testy s tímto podřetězcem v názvu")
    ap.add_argument("--keep", help="adresář pro .ifj26 a vygenerované .code")
    ap.add_argument("--export", help="uložit programy jako .scala/.in/.out")
    ap.add_argument("--timeout", type=int, default=10,
                    help="limit v sekundách pro překladač i interpret (10)")
    ap.add_argument("--list", action="store_true", help="jen vypsat testy")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if args.export:
        export_scala(args.export)
        return 0
    sel = [c for c in CASES if args.filter in c.full]
    if args.list:
        for c in sel:
            print(c.full)
        return 0
    if not sel:
        print("žádný test neodpovídá filtru", file=sys.stderr)
        return 1

    compiler = find_compiler(args.compiler)
    interp = find_interp(args.interp)
    tmp = tempfile.mkdtemp(prefix="funexp_")
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
        print("  %-28s %3d/%d" % (cat, good, total))
    print("\nCelkem: %d   OK: %d   selhalo: %d   (%.1f s)"
          % (ok + fail, ok, fail, time.time() - t0))
    if failed:
        print("selhalo: " + ", ".join(failed))
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
