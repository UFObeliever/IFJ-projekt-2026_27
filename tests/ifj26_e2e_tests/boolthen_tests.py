#!/usr/bin/env python3
"""
boolthen_tests.py - testy rozšíření BOOLTHEN (pravdivostní hodnoty) pro
překladač IFJ26.

Vygenerováno AI (Claude, Anthropic).
Prompt: "pojďme první udělat testovací soubory (stejně jak jsme udělali
         s funexp) každý bude mít spouštění ./extensionname_tests.py prosím
         A chci aby každý testy měly solo složku"

Co se testuje (zadání, kapitola 5.2 BOOLTHEN):
  - typ Boolean, literály true a false, ! (vyšší priorita než násobení),
    && (nižší než relační operátory) a || (nižší než &&),
  - zkrácené vyhodnocení && a || zleva doprava (bez FUNEXP se ověřuje dělením
    nulou: pravý operand se nesmí vyhodnotit, jinak program skončí chybou 57),
  - ukládání, předávání, vracení a porovnávání (==, !=) hodnot typu Boolean,
    toStr/print pro Boolean ("true"/"false"), vestavěná funkce readBoolean,
  - libovolný výraz typu Boolean jako podmínka if a while,
  - zkrácený if (bez else) a else if (zarovnané s if),
  - chyby: typová chyba 6 u podmínek, operátorů i inicializace, chybné
    zarovnání else if (syntaxe), argument funkce špatného typu (5).

Testy 01 až 09 nepoužívají FUNEXP: výsledky výrazů se vždy nejdřív uloží do
proměnné. readBoolean se testuje jen na vstupech "true" a "false" (jiné vstupy
interpret ic26int vrací jako nil a zadání jejich význam neurčuje). Kategorie
99_kombinace_* kombinují BOOLTHEN s dalšími rozšířeními (FUNEXP, CONTROL,
INTERPOLATEDS, EXTKW); lze je vynechat přepínačem --skip 99_kombinace.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup. U nejednoznačných chyb je v testu uvedena n-tice
povolených kódů (viz poznámka u testu).

Příklady:
  ./boolthen_tests.py                                # všechny testy
  ./boolthen_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./boolthen_tests.py -f zkrat                       # jen testy se "zkrat" v názvu
  ./boolthen_tests.py --skip 99_kombinace            # bez kombinací s jinými rozšířeními
  ./boolthen_tests.py --list                         # jen vypíše názvy testů
  ./boolthen_tests.py --keep out_dir                 # uloží .ifj26 a .code
  ./boolthen_tests.py --export scala_out             # uloží .scala/.in/.out pro
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
TRP = """
def t(tag: String, v: Boolean): Boolean =
  print(tag)
  v
"""
CLASSIFY = '''
def classify(x: Int): Unit =
  if x < 0 then
    print("negative")
  else if x == 0 then
    print("zero")
  else
    print("positive")
'''

# ===========================================================================
# 01 literály, výpis a ukládání
# ===========================================================================
C = "01_literaly_a_vypis"
T(C, "01_print_literalu", "", 'print(true)\nprint(false)', out="truefalse")
T(C, "02_toStr_literalu", "",
  'val a = toStr(true)\nval b = toStr(false)\nprint(a)\nprint(b)',
  out="truefalse")
T(C, "03_val_boolean", "", 'val b = true\nprint(b)', out="true")
T(C, "04_val_s_typem", "", 'val b: Boolean = false\nprint(b)', out="false")
T(C, "05_var_a_prirazeni", "",
  'var b = false\nb = true\nprint(b)', out="true")
T(C, "06_var_s_typem_a_prirazeni", "",
  'var b: Boolean = true\nb = false\nprint(b)', out="false")
T(C, "07_vysledek_porovnani_se_uklada", "",
  'val r = 3 < 4\nval s = 3 > 4\nprint(r)\nprint(s)', out="truefalse")
T(C, "08_toStr_promenne", "",
  'val b = false\nval s = toStr(b)\nprint(s)', out="false")
T(C, "09_boolean_v_konkatenaci_pres_toStr", "",
  'val b = true\nval s = "is " + toStr(b)\nprint(s)', out="is true")
T(C, "10_vsechna_relacni_porovnani_do_promennych", "", '''
  val a = 1 < 2
  val b = 1 <= 1
  val c = 2 > 1
  val d = 2 >= 3
  val e = 1 == 1
  val f = 1 != 1
  print(a)
  print(b)
  print(c)
  print(d)
  print(e)
  print(f)
  ''', out="truetruetruefalsetruefalse")
T(C, "11_porovnani_retezcu_a_realnych_cisel", "", '''
  val a = "a" < "b"
  val b = "x" == "x"
  val c = 1.5 > 2.5
  val d = 1 < 1.5
  print(a)
  print(b)
  print(c)
  print(d)
  ''', out="truetruefalsetrue")

# ===========================================================================
# 02 negace
# ===========================================================================
C = "02_negace"
T(C, "01_negace_true", "", 'val b = !true\nprint(b)', out="false")
T(C, "02_negace_false", "", 'val b = !false\nprint(b)', out="true")
T(C, "03_dvojita_negace", "", 'val b = !!true\nprint(b)', out="true")
T(C, "04_negace_negace_v_zavorkach", "",
  'val a = false\nval b = !(!a)\nprint(b)', out="false")
T(C, "05_negace_promenne", "", 'val a = true\nval b = !a\nprint(b)',
  out="false")
T(C, "06_negace_porovnani_v_zavorkach", "",
  'val b = !(3 < 4)\nprint(b)', out="false")
T(C, "07_negace_ma_prednost_pred_and", "",
  'val a = true\nval b = false\nval r = !a && b\nprint(r)', out="false",
  note="(!a) && b = false; špatně by bylo !(a && b) = true")
T(C, "08_negace_ma_prednost_pred_or", "",
  'val a = true\nval b = true\nval r = !a || b\nprint(r)', out="true",
  note="(!a) || b = true; špatně by bylo !(a || b) = false")
T(C, "09_negace_v_podmince", "", '''
  val flag = false
  if !flag then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "10_negace_na_levem_operandu_porovnani_boolean", "", '''
  val a = true
  val b = false
  val r = (!a) == b
  print(r)
  ''', out="true")
T(C, "11_negace_a_aritmetika_v_jednom_vyrazu", "", '''
  val x = 5
  val r = !(x > 3) || x * 2 == 10
  print(r)
  ''', out="true")

# ===========================================================================
# 03 && a ||, priority
# ===========================================================================
C = "03_and_or_priority"
T(C, "01_tabulka_and", "", '''
  val t = true
  val f = false
  val r1 = t && t
  val r2 = t && f
  val r3 = f && t
  val r4 = f && f
  print(r1)
  print(r2)
  print(r3)
  print(r4)
  ''', out="truefalsefalsefalse")
T(C, "02_tabulka_or", "", '''
  val t = true
  val f = false
  val r1 = t || t
  val r2 = t || f
  val r3 = f || t
  val r4 = f || f
  print(r1)
  print(r2)
  print(r3)
  print(r4)
  ''', out="truetruetruefalse")
T(C, "03_retezeni_and", "",
  'val a = true\nval b = true\nval c = true\nval r = a && b && c\nprint(r)',
  out="true")
T(C, "04_retezeni_or_same_false", "",
  'val a = false\nval b = false\nval c = false\nval r = a || b || c\nprint(r)',
  out="false")
T(C, "05_and_ma_prednost_pred_or_vpravo", "", '''
  val a = true
  val b = false
  val c = false
  val r = a || b && c
  print(r)
  ''', out="true", note="a || (b && c); (a || b) && c by dalo false")
T(C, "06_and_ma_prednost_pred_or_vlevo", "", '''
  val a = false
  val b = true
  val c = true
  val r = a && b || c
  print(r)
  ''', out="true", note="(a && b) || c; a && (b || c) by dalo false")
T(C, "07_slozita_priorita", "", '''
  val a = true
  val b = false
  val c = false
  val d = false
  val r = a || b && c || d
  print(r)
  ''', out="true")
T(C, "08_zavorky_meni_prioritu", "", '''
  val a = true
  val b = false
  val c = false
  val r = (a || b) && c
  print(r)
  ''', out="false")
T(C, "09_relacni_operatory_maji_prednost_pred_and", "", '''
  val a = 1 < 2 && 3 < 4
  val b = 1 < 2 && 4 < 3
  print(a)
  print(b)
  ''', out="truefalse")
T(C, "10_aritmetika_a_relace_pred_and", "",
  'val r = 1 + 2 < 4 && 3 * 2 == 6\nprint(r)', out="true")
T(C, "11_or_dvou_relaci", "",
  'val x = 5\nval r = x < 0 || x > 10\nprint(r)', out="false")
T(C, "12_rovnost_a_nerovnost_v_and", "",
  'val x = 1\nval y = 3\nval r = x == 1 && y != 2\nprint(r)', out="true")
T(C, "13_nerovnost_a_rovnost_boolean_se_zavorkami", "", '''
  val a = 1 < 2
  val b = 3 < 4
  val r = (a && b) == (a || b)
  print(r)
  ''', out="true")
T(C, "14_rozsah_hodnot", "", '''
  val x = 5
  val r = x >= 1 && x <= 10
  print(r)
  ''', out="true")
T(C, "15_vsechny_kombinace_xor", "", '''
  val t = true
  val f = false
  val r1 = (f || f) && !(f && f)
  val r2 = (f || t) && !(f && t)
  val r3 = (t || f) && !(t && f)
  val r4 = (t || t) && !(t && t)
  print(r1)
  print(r2)
  print(r3)
  print(r4)
  ''', out="falsetruetruefalse")
T(C, "16_de_morgan", "", '''
  val t = true
  val f = false
  val a1 = (!(t && f)) == (!t || !f)
  val a2 = (!(f || f)) == (!f && !f)
  val a3 = (!(t || f)) == (!t && !f)
  print(a1)
  print(a2)
  print(a3)
  ''', out="truetruetrue")

# ===========================================================================
# 04 zkrácené vyhodnocení (sonda: dělení nulou -> chyba 57)
# ===========================================================================
C = "04_zkracene_vyhodnoceni"
T(C, "01_and_s_levym_false_nevyhodnoti_pravy", "",
  'val z = 0\nval r = z != 0 && 10 / z > 1\nprint(r)', out="false",
  note="pravý operand se nesmí vyhodnotit (dělení nulou by skončilo chybou 57)")
T(C, "02_or_s_levym_true_nevyhodnoti_pravy", "",
  'val z = 0\nval r = z == 0 || 10 / z > 1\nprint(r)', out="true")
T(C, "03_and_s_levym_true_vyhodnoti_pravy", "",
  'val x = 2\nval r = x != 0 && 10 / x > 1\nprint(r)', out="true")
T(C, "04_or_s_levym_false_vyhodnoti_pravy", "",
  'val x = 5\nval r = x < 0 || 10 / x > 1\nprint(r)', out="true")
T(C, "05_levy_true_pravy_se_vyhodnoti_a_pada", "",
  'print("a")\nval z = 0\nval r = z == 0 && 10 / z > 1\nprint("b")',
  out="a", rc=57,
  note="levý operand je true, vyhodnotí se pravý a dělení nulou skončí kódem 57")
T(C, "06_or_levy_false_pravy_se_vyhodnoti_a_pada", "",
  'print("a")\nval z = 0\nval r = z != 0 || 10 / z > 1\nprint("b")',
  out="a", rc=57)
T(C, "07_retezec_and_zastavi_na_prvnim_false", "", '''
  val z = 0
  val a = false
  val r = a && z == 0 && 10 / z > 1
  print(r)
  ''', out="false")
T(C, "08_retezec_or_zastavi_na_prvnim_true", "", '''
  val z = 0
  val a = true
  val r = a || z != 0 || 10 / z > 1
  print(r)
  ''', out="true")
T(C, "09_v_podmince_if", "", '''
  val z = 0
  if z != 0 && 10 / z > 1 then
    print("a")
  else
    print("b")
  ''', out="b")
T(C, "10_v_podmince_while", "", '''
  var i = 0
  while i < 3 && 10 / (3 - i) > 0 do
    print(i)
    i = i + 1
  ''', out="012",
  note="při i = 3 by pravý operand dělil nulou, ale levý je již false")
T(C, "11_zavorkovane_vnorene_vyrazy", "", '''
  val x = 0
  val r = (x != 0 && 10 / x > 1) || x == 0
  print(r)
  ''', out="true")
T(C, "12_vnoreny_and_or", "", '''
  val x = 0
  val r = x != 0 && (10 / x > 1 || 20 / x > 1)
  print(r)
  ''', out="false")
T(C, "13_zkracene_vyhodnoceni_je_zleva_doprava", "", '''
  val z = 0
  val r = z == 0 || 10 / z > 1 || 20 / z > 1
  print(r)
  ''', out="true")
T(C, "14_and_zprava_nevyhodnoceny_vyraz_s_vedlejsim_efektem_nehrozi", "", '''
  var n = 0
  val a = false
  if a && n == 0 then
    n = 1
  print(n)
  ''', out="0")

# ===========================================================================
# 05 ukládání, předávání, vracení a porovnávání
# ===========================================================================
C = "05_predavani_a_vraceni"
T(C, "01_boolean_parametr", '''
def pick(b: Boolean): Int =
  if b then
    return 1
  else
    return 0
''', 'val a = pick(true)\nval b = pick(false)\nprint(a)\nprint(b)', out="10")
T(C, "02_vraci_boolean", "def isPositive(n: Int): Boolean = n > 0",
  'val r = isPositive(5)\nval s = isPositive(-5)\nprint(r)\nprint(s)',
  out="truefalse")
T(C, "03_odvozeny_navratovy_typ_boolean", "def isZero(n: Int) = n == 0",
  'val r = isZero(0)\nprint(r)', out="true")
T(C, "04_tri_boolean_parametry",
  "def and3(a: Boolean, b: Boolean, c: Boolean): Boolean = a && b && c",
  'val r = and3(true, true, false)\nval s = and3(true, true, true)\n'
  'print(r)\nprint(s)', out="falsetrue")
T(C, "05_return_boolean", '''
def isBig(n: Int): Boolean =
  if n > 100 then
    return true
  else
    return false
''', 'val a = isBig(200)\nval b = isBig(5)\nprint(a)\nprint(b)', out="truefalse")
T(C, "06_rovnost_boolean_hodnot", "", '''
  val a = true
  val b = false
  val e = a == b
  val n = a != b
  print(e)
  print(n)
  ''', out="falsetrue")
T(C, "07_rovnost_vysledku_porovnani", "",
  'val r = (1 < 2) == (3 < 4)\nprint(r)', out="true")
T(C, "08_negace_jako_funkce", "def not(b: Boolean): Boolean = !b",
  'val r = not(true)\nval s = not(false)\nprint(r)\nprint(s)',
  out="falsetrue")
T(C, "09_boolean_jako_posledni_vyraz_bloku", '''
def f(): Boolean =
  val x = 5
  x > 3
''', 'val r = f()\nprint(r)', out="true")
T(C, "10_boolean_promenna_jako_priznak_cyklu", "", '''
  var running = true
  var i = 0
  while running do
    i = i + 1
    if i == 3 then
      running = false
  print(i)
  ''', out="3")
T(C, "11_prirazeni_ve_funkci", '''
def flip(b: Boolean): Boolean =
  var r = b
  r = !r
  r
''', 'val a = flip(true)\nval b = flip(false)\nprint(a)\nprint(b)',
  out="falsetrue")
T(C, "12_boolean_parametr_zavisly_na_druhem", '''
def check(x: Int, strict: Boolean): Boolean =
  if strict then
    return x > 10
  else
    return x > 0
''', '''
  val a = check(5, true)
  val b = check(5, false)
  print(a)
  print(b)
  ''', out="falsetrue")
T(C, "13_jednoradkove_boolean_telo_s_logikou",
  "def between(x: Int, lo: Int, hi: Int): Boolean = x >= lo && x <= hi",
  'val a = between(5, 1, 10)\nval b = between(0, 1, 10)\nprint(a)\nprint(b)',
  out="truefalse")
T(C, "14_boolean_vracen_z_vnoreneho_if", '''
def sgnPositive(x: Int): Boolean =
  if x > 0 then
    return true
  else
    if x < 0 then
      return false
    else
      return false
''', 'val a = sgnPositive(3)\nval b = sgnPositive(-3)\nval c = sgnPositive(0)\n'
    'print(a)\nprint(b)\nprint(c)', out="truefalsefalse")

# ===========================================================================
# 06 zkrácený if a else if
# ===========================================================================
C = "06_if_bez_else_a_else_if"
T(C, "01_zkraceny_if_proveden", "", '''
  val x = 1
  if x > 0 then
    print("p")
  ''', out="p")
T(C, "02_zkraceny_if_preskocen", "", '''
  val x = -1
  if x > 0 then
    print("p")
  print("end")
  ''', out="end")
T(C, "03_else_if_tri_vetve", CLASSIFY,
  'classify(-5)\nclassify(0)\nclassify(7)', out="negativezeropositive")
T(C, "04_else_if_bez_posledniho_else", '''
def sign(x: Int): Unit =
  if x < 0 then
    print("negative")
  else if x == 0 then
    print("zero")
''', 'sign(-1)\nsign(0)\nsign(5)\nprint("|")', out="negativezero|")
T(C, "05_dlouhy_retezec_else_if", '''
def grade(g: Int): Unit =
  if g >= 90 then
    print("A")
  else if g >= 80 then
    print("B")
  else if g >= 70 then
    print("C")
  else if g >= 60 then
    print("D")
  else
    print("F")
''', 'grade(95)\ngrade(85)\ngrade(75)\ngrade(65)\ngrade(10)', out="ABCDF")
T(C, "06_zanoreny_if_ve_else_if", "", '''
  val x = 15
  if x < 10 then
    print("small")
  else if x < 20 then
    if x < 15 then
      print("mid-low")
    else
      print("mid-high")
  else
    print("big")
  ''', out="mid-high")
T(C, "07_zkraceny_if_ve_while", "", '''
  var i = 0
  var evens = 0
  while i < 10 do
    if i / 2 * 2 == i then
      evens = evens + 1
    i = i + 1
  print(evens)
  ''', out="5")
T(C, "08_zkraceny_if_s_and", "", '''
  val a = 3
  val b = 4
  if a > 0 && b > 0 then
    print("both")
  ''', out="both")
T(C, "09_vnoreny_zkraceny_if", "", '''
  val x = 5
  if x > 0 then
    if x > 3 then
      print("big")
    print("pos")
  ''', out="bigpos")
T(C, "10_zkraceny_if_jako_posledni_prikaz_unit_funkce", '''
def maybe(x: Int): Unit =
  if x > 0 then
    print("p")
''', 'maybe(1)\nmaybe(-1)\nmaybe(2)', out="pp")
T(C, "11_zkraceny_if_v_funkci_s_odvozenym_typem_unit", '''
def maybe2(x: Int) =
  if x > 0 then
    print("p")
''', 'maybe2(1)\nmaybe2(-1)', out="p")
T(C, "12_else_if_s_prirazenim", "", '''
  val x = 0
  var r = 0
  if x < 0 then
    r = -1
  else if x == 0 then
    r = 0
  else
    r = 1
  print(r)
  ''', out="0")
T(C, "13_pouze_jedna_vetev_else_if_se_provede", "", '''
  val x = 5
  if x > 0 then
    print("a")
  else if x > 1 then
    print("b")
  else if x > 2 then
    print("c")
  ''', out="a")
T(C, "14_else_if_se_vyhodnoti_az_kdyz_predchozi_selze", "", '''
  val z = 0
  if z == 0 then
    print("zero")
  else if 10 / z > 1 then
    print("never")
  ''', out="zero")

# ===========================================================================
# 07 podmínky: libovolný výraz typu Boolean
# ===========================================================================
C = "07_podminky"
T(C, "01_if_s_promennou", "", '''
  val flag = true
  if flag then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "02_while_s_and", "", '''
  var i = 0
  var s = 0
  while i < 10 && s < 20 do
    s = s + i
    i = i + 1
  print(i)
  print(",")
  print(s)
  ''', out="7,21")
T(C, "03_while_s_or", "", '''
  var i = 0
  var j = 10
  while i < 3 || j > 8 do
    i = i + 1
    j = j - 1
  print(i)
  print(",")
  print(j)
  ''', out="3,7",
  note="smyčka běží dokud je alespoň jedna z podmínek pravdivá")
T(C, "04_if_s_rovnosti_boolean_hodnot", "", '''
  val a = 1 < 2
  val b = 3 < 4
  if a == b then
    print("same")
  else
    print("diff")
  ''', out="same")
T(C, "05_while_s_negaci_priznaku", "", '''
  var done = false
  var i = 0
  while !done do
    i = i + 1
    if i >= 4 then
      done = true
  print(i)
  ''', out="4")
T(C, "06_rozsah_hodnot_v_podmince", "", '''
  val a = 0
  val b = 5
  val c = 11
  if a >= 1 && a <= 10 then
    print("y")
  else
    print("n")
  if b >= 1 && b <= 10 then
    print("y")
  else
    print("n")
  if c >= 1 && c <= 10 then
    print("y")
  else
    print("n")
  ''', out="nyn")
T(C, "07_podminka_s_rovnosti_a_nerovnosti", "", '''
  val x = 3
  if x == 3 && x != 4 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "08_podminka_ulozena_do_promenne", "", '''
  val x = 7
  val ok = x > 5 && x < 10
  if ok then
    print("in")
  else
    print("out")
  ''', out="in")
T(C, "09_slozita_podminka_se_zavorkami", "", '''
  val a = true
  val b = false
  val c = true
  if (a || b) && !(b && c) then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "10_while_se_slozitou_podminkou_a_break_pres_priznak", "", '''
  var i = 1
  var found = false
  while i <= 10 && !found do
    if i * i > 50 then
      found = true
    else
      i = i + 1
  print(i)
  ''', out="8")

# ===========================================================================
# 08 vstup: readBoolean
# ===========================================================================
C = "08_readBoolean"
T(C, "01_true", "", 'val b = readBoolean()\nprint(b)', inp="true\n",
  out="true")
T(C, "02_false", "", 'val b = readBoolean()\nprint(b)', inp="false\n",
  out="false")
T(C, "03_dve_hodnoty", "", '''
  val a = readBoolean()
  val b = readBoolean()
  val r = a && b
  print(r)
  ''', inp="true\nfalse\n", out="false")
T(C, "04_v_podmince_pres_promennou", "", '''
  val b = readBoolean()
  if b then
    print("yes")
  else
    print("no")
  ''', inp="true\n", out="yes")
T(C, "05_toStr_nactene_hodnoty", "", '''
  val b = readBoolean()
  val s = toStr(b)
  print(s)
  ''', inp="false\n", out="false")
T(C, "06_smisene_s_readInt", "", '''
  val n = readInt()
  val flag = readBoolean()
  if flag then
    print(n)
  else
    print(0)
  ''', inp="5\ntrue\n", out="5")
T(C, "07_negace_nactene_hodnoty", "", '''
  val b = readBoolean()
  val c = !b
  print(c)
  ''', inp="true\n", out="false")

# ===========================================================================
# 09 chyby
# ===========================================================================
C = "09_chyby"
T(C, "01_podminka_if_je_int", "",
  'if 1 then\n  print("a")\nelse\n  print("b")', crc=6)
T(C, "02_podminka_if_je_string", "",
  'if "a" then\n  print("a")\nelse\n  print("b")', crc=6)
T(C, "03_podminka_if_je_int_promenna", "",
  'val x = 1\nif x then\n  print("a")\nelse\n  print("b")', crc=6)
T(C, "04_podminka_while_je_int", "",
  'var i = 1\nwhile i do\n  i = i - 1', crc=6)
T(C, "05_negace_cisla", "", 'val b = !5\nprint("x")', crc=6)
T(C, "06_negace_retezce", "", 'val b = !"a"\nprint("x")', crc=6)
T(C, "07_and_s_int_vlevo", "", 'val b = 1 && true\nprint("x")', crc=6)
T(C, "08_and_s_int_vpravo", "", 'val b = true && 1\nprint("x")', crc=6)
T(C, "09_or_s_retezcem", "", 'val b = true || "a"\nprint("x")', crc=6)
T(C, "10_soucet_boolean_a_int", "", 'val b = true + 1\nprint("x")', crc=6)
T(C, "11_soucet_dvou_boolean", "", 'val b = true + false\nprint("x")', crc=6)
T(C, "12_nasobeni_boolean", "", 'val b = true * 2\nprint("x")', crc=6)
T(C, "13_unarni_minus_na_boolean", "", 'val b = -true\nprint("x")', crc=6)
T(C, "14_boolean_do_int_promenne", "", 'val n: Int = true\nprint("x")', crc=6)
T(C, "15_int_do_boolean_promenne", "", 'val b: Boolean = 1\nprint("x")',
  crc=6)
T(C, "16_boolean_do_string_promenne", "",
  'val s: String = false\nprint("x")', crc=6)
T(C, "17_rovnost_boolean_a_int", "", 'val b = true == 1\nprint("x")', crc=6)
T(C, "18_nerovnost_boolean_a_retezce", "",
  'val b = true != "a"\nprint("x")', crc=6)
T(C, "19_relacni_operator_na_boolean", "",
  'val b = true < false\nprint("x")', crc=6,
  note="<, <=, >, >= jen pro čísla a řetězce")
T(C, "20_prirazeni_int_do_boolean_var", "",
  'var b = true\nb = 1\nprint("x")', crc=6)
T(C, "21_prirazeni_boolean_do_int_var", "",
  'var n = 1\nn = true\nprint("x")', crc=6)
T(C, "22_int_argument_na_boolean_parametr",
  "def f(b: Boolean): Boolean = b", 'val r = f(1)\nprint("x")', crc=5)
T(C, "23_boolean_argument_na_int_parametr",
  "def f(n: Int): Int = n", 'val r = f(true)\nprint("x")', crc=5)
T(C, "24_navratova_hodnota_boolean_pro_int",
  "def f(): Int = true", 'print("x")', crc=6)
T(C, "25_navratova_hodnota_int_pro_boolean",
  "def f(): Boolean = 1", 'print("x")', crc=6)
T(C, "26_zkraceny_if_v_funkci_s_navratovym_typem_int", '''
def f(x: Int): Int =
  if x > 0 then
    return 1
''', 'print("x")', crc=6,
  note="chybějící else se chová jako větev s (), takže neodpovídá typu Int")
T(C, "27_else_if_odsazeny_o_mezeru", "", '''
  val x = 0
  if x < 0 then
    print("a")
   else if x == 0 then
    print("b")
  ''', crc=(1, 2), note="else if musí být zarovnáno s if")
T(C, "28_else_if_odsazeny_hloub", "", '''
  val x = 0
  if x < 0 then
    print("a")
    else if x == 0 then
      print("b")
  ''', crc=(1, 2))
T(C, "29_else_bez_if", "", 'else\n  print("x")', crc=2)
T(C, "30_chybi_pravy_operand_and", "", 'val b = true &&\nprint("x")',
  crc=2)
RAW(C, "31_radek_zacina_operatorem", '''
@main
def main(): Unit =
  val b = true
    || false
  print("x")
''', crc=(1, 2), note="logický řádek nemůže začínat operátorem")
T(C, "32_true_jako_identifikator", "", 'val true = 1\nprint("x")', crc=2)
T(C, "33_neexistujici_literal_True", "", 'val b = True\nprint("x")', crc=3)
T(C, "34_readBoolean_s_argumentem", "", 'val b = readBoolean(1)\nprint("x")',
  crc=5)
T(C, "35_retezeni_relacnich_operatoru", "",
  'val b = 1 < 2 < 3\nprint("x")', crc=(2, 6),
  note="řetězení relačních operátorů není povoleno")
T(C, "36_retezeni_rovnosti_boolean", "",
  'val b = true == true == true\nprint("x")', crc=(2, 6))
T(C, "37_toStr_boolean_plus_int", "",
  'val s = toStr(true) + 1\nprint("x")', crc=6)
T(C, "38_if_bez_then", "", 'val x = 1\nif x > 0\n  print("a")', crc=2)
T(C, "39_nedefinovana_promenna_v_boolean_vyrazu", "",
  'val b = nope && true\nprint("x")', crc=3)
T(C, "40_boolean_v_aritmeticke_pozici_zavorky", "",
  'val n = 1 + (2 < 3)\nprint("x")', crc=6)

# ===========================================================================
# 99 kombinace s dalšími rozšířeními
# ===========================================================================
C = "99_kombinace_funexp"
T(C, "01_and_zkracene_s_volanim", TRP, '''
  val r = t("a", true) && t("b", false) && t("c", true)
  print("|")
  print(r)
  ''', out="ab|false")
T(C, "02_or_zkracene_s_volanim", TRP, '''
  val r = t("a", false) || t("b", true) || t("c", false)
  print("|")
  print(r)
  ''', out="ab|true")
T(C, "03_priorita_a_zkraceni_or_pred_and", TRP, '''
  val r = t("a", true) || t("b", true) && t("c", true)
  print("|")
  print(r)
  ''', out="a|true")
T(C, "04_priorita_a_zkraceni_and_pred_or", TRP, '''
  val r = t("a", false) && t("b", true) || t("c", true)
  print("|")
  print(r)
  ''', out="ac|true")
T(C, "05_rovnost_vyhodnocuje_zleva_doprava", TRP, '''
  val r = t("a", true) == t("b", true)
  print("|")
  print(r)
  ''', out="ab|true")
T(C, "06_funkce_vracejici_boolean_v_podmince", '''
def isEven(n: Int): Boolean = n / 2 * 2 == n
''', '''
  if isEven(4) then
    print("y")
  else
    print("n")
  if isEven(3) then
    print("y")
  else
    print("n")
  ''', out="yn")
T(C, "07_volani_v_booleanovskem_vyrazu", '''
def isEven(n: Int): Boolean = n / 2 * 2 == n
''', 'print(toStr(isEven(4) && !isEven(3)))', out="true")
T(C, "08_readBoolean_v_podmince", "", '''
  if readBoolean() then
    print("yes")
  else
    print("no")
  ''', inp="false\n", out="no")
T(C, "09_vnorena_volani_s_boolean_argumenty", '''
def isEven(n: Int): Boolean = n / 2 * 2 == n

def both(a: Boolean, b: Boolean): Boolean = a && b
''', 'print(toStr(both(isEven(2), isEven(3) || true)))', out="true")
T(C, "10_boolean_vysledek_volani_do_toStr_a_konkatenace", '''
def isOdd(n: Int): Boolean = n / 2 * 2 != n
''', 'print("odd=" + toStr(isOdd(7)) + "," + toStr(isOdd(8)))',
  out="odd=true,false")
T(C, "11_while_s_volanim_v_podmince", '''
def small(n: Int): Boolean = n < 4
''', '''
  var i = 0
  while small(i) do
    print(i)
    i = i + 1
  ''', out="0123")
T(C, "12_negace_vysledku_volani", '''
def isPos(n: Int): Boolean = n > 0
''', 'val r = !isPos(5)\nprint(r)', out="false")
T(C, "13_zkraceni_chrani_pred_chybou_ve_volani", '''
def safeDiv(a: Int, b: Int): Int = a / b
''', '''
  val z = 0
  val r = z != 0 && safeDiv(10, z) > 1
  print(r)
  ''', out="false")

C = "99_kombinace_control"
T(C, "01_if_vyraz_vraci_boolean", "", '''
  val x = 5
  val b = if x > 0 then true else false
  print(b)
  ''', out="true")
T(C, "02_for_se_strazi_s_and", "",
  'for i <- 1 to 10 if i > 3 && i < 7 do print(i)', out="456")
T(C, "03_for_se_strazi_s_or", "",
  'for i <- 1 to 10 if i < 3 || i > 8 do print(i)', out="12910")
T(C, "04_for_se_strazi_s_negaci", "",
  'for i <- 1 to 4 if !(i == 2) do print(i)', out="134")
T(C, "05_if_vyraz_s_slozenou_podminkou", "", '''
  val x = 5
  val y = 6
  val r = if x > 0 && y > 0 then 1 else 0
  print(r)
  ''', out="1")
T(C, "06_if_vyraz_s_vetvemi_boolean_a_negaci", "", '''
  val a = true
  val b = if a then !a else a
  print(b)
  ''', out="false")
T(C, "07_while_a_for_s_priznakem", "", '''
  var stop = false
  var total = 0
  for i <- 1 to 10 do
    if !stop then
      total = total + i
      if total > 10 then
        stop = true
  print(total)
  ''', out="15")

C = "99_kombinace_interpolateds"
T(C, "01_boolean_promenna_v_interpolaci", "",
  'val flag = true\nval m = s"flag=$flag"\nprint(m)', out="flag=true")
T(C, "02_booleanovsky_vyraz_v_interpolaci", "",
  'val a = true\nval b = false\nval m = s"${a && b}/${a || b}"\nprint(m)',
  out="false/true")
T(C, "03_porovnani_v_interpolaci", "",
  'val x = 5\nval m = s"${x > 3}"\nprint(m)', out="true")

C = "99_kombinace_extkw"
T(C, "01_rozsirujici_metoda_na_boolean", '''
extension (b: Boolean)
  def num: Int =
    if b then
      return 1
    else
      return 0
''', 'val x = true\nval y = false\nval a = x.num\nval c = y.num\n'
    'print(a)\nprint(c)', out="10")
T(C, "02_rozsirujici_metoda_vraci_boolean", '''
extension (n: Int)
  def isPositive: Boolean = n > 0
''', 'val a = 5\nval r = a.isPositive\nprint(r)', out="true")


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
