#!/usr/bin/env python3
"""
interpolateds_tests.py - testy rozšíření INTERPOLATEDS (interpolované řetězce
s"...") pro překladač IFJ26.

Vygenerováno AI (Claude, Anthropic).
Prompt: "pojďme první udělat testovací soubory (stejně jak jsme udělali
         s funexp) každý bude mít spouštění ./extensionname_tests.py prosím
         A chci aby každý testy měly solo složku"

Co se testuje (zadání, kapitola 5.6 INTERPOLATEDS):
  - interpolátor s: $id vloží hodnotu proměnné, ${výraz} libovolný výraz,
    $$ vloží jeden znak dolaru, \" je uvozovka v běžném interpolovaném řetězci,
  - víceřádkové interpolované řetězce (s a trojice uvozovek; uvozovka se píše
    doslova),
  - každá vložená hodnota se převádí stejně jako ve funkci toStr
    (Double s desetinnou částí jako %.2f, ostatní jako print),
  - chyby: nedefinovaná proměnná (3), typová chyba ve výrazu (6), neukončený
    řetězec nebo ${ (1/2), chybný escape (1), $ bez identifikátoru (1/2),
    jiný interpolátor než s (1/2).

Testy 01 až 06 nevyužívají FUNEXP: uvnitř ${...} jsou jen proměnné a operátory
a výsledek interpolace se předává dál až přes proměnnou. Kategorie
99_kombinace_* kombinují INTERPOLATEDS s dalšími rozšířeními (FUNEXP, BOOLTHEN,
CONTROL, VALMEM, EXTKW, PARAMLISTS); lze je vynechat přepínačem
--skip 99_kombinace.

Pozor: u Double dává Scala jiný text než IFJ26 (1.2345 -> "1.2345" vs "1.23"),
proto --export (programy pro skutečnou Scalu) u těchto testů nesedí.
Víceřádkové literály se do testů zapisují od začátku řádku, aby bylo přesně
vidět, které mezery jsou součástí řetězce.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup. U nejednoznačných chyb je v testu uvedena n-tice
povolených kódů (viz poznámka u testu).

Příklady:
  ./interpolateds_tests.py                           # všechny testy
  ./interpolateds_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./interpolateds_tests.py -f escape                 # jen testy s "escape" v názvu
  ./interpolateds_tests.py --skip 99_kombinace       # bez kombinací s jinými rozšířeními
  ./interpolateds_tests.py --list                    # jen vypíše názvy testů
  ./interpolateds_tests.py --keep out_dir            # uloží .ifj26 a .code
  ./interpolateds_tests.py --export scala_out        # uloží .scala/.in/.out pro
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
# funkce vypisující vlastní značku (v interpolaci se nepoužívají vnořené uvozovky)
TAGS = """
def pa(v: Int): Int =
  print("a")
  v

def pb(v: Int): Int =
  print("b")
  v

def pc(v: Int): Int =
  print("c")
  v
"""

# ===========================================================================
# 01 základní použití
# ===========================================================================
C = "01_zakladni"
T(C, "01_dolar_id_string", "",
  'val name = "IFJ"\nprint(s"Hello $name!")', out="Hello IFJ!")
T(C, "02_dolar_id_int", "", 'val n = 26\nprint(s"n=$n")', out="n=26")
T(C, "03_vyraz_ve_slozenych_zavorkach", "", 'print(s"${1 + 2}")', out="3")
T(C, "04_priklad_ze_zadani", "", '''
  val name = "IFJ"
  val n = 26
  print(s"$name${n + 1} costs $$0")
  ''', out="IFJ27 costs $0")
RAW(C, "05_viceradkovy_priklad_ze_zadani", '''
@main
def main(): Unit =
  val name = "IFJ"
  print(s"""He said "hello", $name.
Second line.""")
''', out='He said "hello", IFJ.\nSecond line.')
T(C, "06_bez_placeholderu", "", 'print(s"plain text")', out="plain text")
T(C, "07_prazdny_retezec", "", 'val e = s""\nprint(e)\nprint("|")', out="|")
T(C, "08_jen_placeholder", "", 'val n = 26\nprint(s"$n")', out="26")
T(C, "09_vice_za_sebou_bez_mezer", "",
  'val a = 1\nval b = 2\nval c = 3\nprint(s"$a$b$c")', out="123")
T(C, "10_dva_dolary_jsou_jeden_znak", "", 'print(s"$$")', out="$")
T(C, "11_ctyri_dolary_jsou_dva_znaky", "", 'print(s"$$$$")', out="$$")
T(C, "12_dolar_dolar_a_pak_promenna", "",
  'val name = "IFJ"\nprint(s"$$$name")', out="$IFJ")
T(C, "13_text_hned_za_slozenymi_zavorkami", "",
  'val name = "IFJ"\nprint(s"${name}X")', out="IFJX")
T(C, "14_znaky_za_identifikatorem_ho_ukonci", "", '''
  val name = "IFJ"
  print(s"$name-X $name. $name, ($name)")
  ''', out="IFJ-X IFJ. IFJ, (IFJ)")
T(C, "15_identifikator_se_cte_cely", "", '''
  val name = "A"
  val nameX = "!"
  print(s"$nameX ${name}X")
  ''', out="! AX")
T(C, "16_podtrzitko_a_cislice_v_identifikatoru", "",
  'val my_var2 = 5\nval _x = 6\nprint(s"$my_var2 $_x")', out="5 6")
T(C, "17_zaporne_cislo", "", 'val n = -5\nprint(s"n=$n")', out="n=-5")
T(C, "18_hodnota_s_uvozovkami_se_neupravuje", "",
  r'val q = "a\"b"' + '\nprint(s"[$q]")', out='[a"b]')
T(C, "19_lomitka_nejsou_komentar", "", '''
  val a = 1
  print(s"url://x/$a /* $a */")
  ''', out="url://x/1 /* 1 */")
T(C, "20_prazdny_retezec_v_hodnote", "",
  'val e = ""\nprint(s"[$e]")', out="[]")
T(C, "21_bez_mezer_okolo_vseho", "", 'print(s"${1}${2}${3}")', out="123")

# ===========================================================================
# 02 výrazy ve složených závorkách
# ===========================================================================
C = "02_vyrazy"
T(C, "01_priorita_operatoru", "", 'print(s"${2 + 3 * 4}")', out="14")
T(C, "02_promenne_ve_vyrazu", "",
  'val a = 6\nval b = 7\nprint(s"${a * b}")', out="42")
T(C, "03_zavorky_ve_vyrazu", "",
  'val a = 3\nval b = 4\nprint(s"${(a + b) * 2}")', out="14")
T(C, "04_konkatenace_retezcu", "",
  'val a = "ab"\nval b = "cd"\nprint(s"${a + b}")', out="abcd")
T(C, "05_unarni_minus", "", 'val a = 5\nprint(s"${-a}")', out="-5")
T(C, "06_celociselne_deleni", "",
  'val a = 7\nval b = 2\nprint(s"${a / b}")', out="3")
T(C, "07_vice_vyrazu_v_jednom_retezci", "",
  'val a = 3\nval b = 4\nprint(s"${a}+${b}=${a + b}")', out="3+4=7")
T(C, "08_mezery_uvnitr_zavorek", "",
  'val a = 3\nval b = 4\nprint(s"${ a + b }")', out="7")
T(C, "09_double_ve_vyrazu", "",
  'val x = 1.0\nprint(s"${x + 0.5}")', out="1.50")
T(C, "10_v_cyklu_while", "", '''
  var i = 0
  while i < 3 do
    print(s"i=$i;")
    i = i + 1
  ''', out="i=0;i=1;i=2;")
T(C, "11_stejne_jako_dolar_id", "", '''
  val n = 9
  val a = s"$n"
  val b = s"${n}"
  if a == b then
    print("same")
  else
    print("diff")
  ''', out="same")
T(C, "12_vyraz_z_vice_promennych", "", '''
  val a = 1
  val b = 2
  val c = 3
  val d = 4
  print(s"${a + b + c + d}")
  ''', out="10")
T(C, "13_vyraz_se_zavorkami_uvnitr_zavorek", "", '''
  val a = 2
  print(s"${((a + 1) * (a + 2)) - 1}")
  ''', out="11")
T(C, "14_hodnota_zmenene_promenne", "", '''
  var n = 1
  val before = s"n=$n"
  n = n + 1
  val after = s"n=$n"
  print(before)
  print(",")
  print(after)
  ''', out="n=1,n=2")
T(C, "15_typ_double_s_celym_cislem_pres_aritmetiku", "",
  'val x = 2.0\nprint(s"${x * 1.25}")', out="2.50")
T(C, "16_stejny_identifikator_vicekrat", "",
  'val n = 4\nprint(s"$n$n$n ${n * n}")', out="444 16")

# ===========================================================================
# 03 escape sekvence v běžném interpolovaném řetězci
# ===========================================================================
C = "03_escape_sekvence"
RAW(C, "01_uvozovky", r'''
@main
def main(): Unit =
  val name = "IFJ"
  print(s"say \"hi\" $name")
''', out='say "hi" IFJ')
RAW(C, "02_novy_radek", r'''
@main
def main(): Unit =
  print(s"a\nb")
''', out="a\nb")
RAW(C, "03_tabulator", r'''
@main
def main(): Unit =
  print(s"a\tb")
''', out="a\tb")
RAW(C, "04_zpetne_lomitko", r'''
@main
def main(): Unit =
  print(s"a\\b")
''', out="a\\b")
RAW(C, "05_dolar_mezi_uvozovkami", r'''
@main
def main(): Unit =
  print(s"\"$$\"")
''', out='"$"')
RAW(C, "06_escape_vedle_placeholderu", r'''
@main
def main(): Unit =
  val name = "IFJ"
  print(s"\n$name\n")
''', out="\nIFJ\n")
RAW(C, "07_zpetne_lomitko_pred_placeholderem", r'''
@main
def main(): Unit =
  val n = 5
  print(s"\\$n")
''', out="\\5")
RAW(C, "08_carriage_return", r'''
@main
def main(): Unit =
  print(s"a\rb")
''', out="a\rb")
RAW(C, "09_vice_escape_sekvenci", r'''
@main
def main(): Unit =
  val a = 1
  print(s"[\t$a\n\\\"]")
''', out='[\t1\n\\"]')

# ===========================================================================
# 04 víceřádkové interpolované řetězce
# ===========================================================================
C = "04_viceradkove"
RAW(C, "01_uvozovky_doslova", '''
@main
def main(): Unit =
  val name = "IFJ"
  print(s"""say "hi" $name""")
''', out='say "hi" IFJ')
RAW(C, "02_placeholdery_na_vice_radcich", '''
@main
def main(): Unit =
  val a = 1
  val b = 2
  print(s"""a=$a
b=$b
sum=${a + b}""")
''', out="a=1\nb=2\nsum=3")
RAW(C, "03_dolar_dolar", '''
@main
def main(): Unit =
  print(s"""cost $$5""")
''', out="cost $5")
RAW(C, "04_prazdny_radek_uvnitr", '''
@main
def main(): Unit =
  val x = 1
  print(s"""x
$x

y""")
''', out="x\n1\n\ny")
RAW(C, "05_odsazeni_na_dalsim_radku_se_zachova", '''
@main
def main(): Unit =
  val x = 7
  print(s"""first
    indented $x
  less""")
''', out="first\n    indented 7\n  less")
RAW(C, "06_jednoradkovy_zapis_trojitych_uvozovek", '''
@main
def main(): Unit =
  val a = 1
  print(s"""one line $a""")
''', out="one line 1")
RAW(C, "07_ulozeny_do_promenne_a_delka", '''
@main
def main(): Unit =
  val a = 12
  val m = s"""x
$a"""
  val n = strLength(m)
  print(n)
''', out="4", note="'x' LF '12' = 4 znaky")
RAW(C, "08_viceradkovy_v_jednom_vyrazu_s_dalsim_prikazem", '''
@main
def main(): Unit =
  val a = 5
  val m = s"""a
b $a"""
  print(m)
  print("|")
''', out="a\nb 5|")

# ===========================================================================
# 05 typy hodnot a převod jako toStr
# ===========================================================================
C = "05_typy_a_prevod"
T(C, "01_double_priklad_ze_zadani", "", 'print(s"${1.2345}")', out="1.23",
  note="Scala by dala 1.2345, IFJ26 používá formát toStr (%.2f)")
T(C, "02_double_promenna", "", 'val d = 0.5\nprint(s"$d")', out="0.50")
T(C, "03_zaporny_double", "", 'val d = -3.756\nprint(s"$d")', out="-3.76")
T(C, "04_stejne_jako_toStr", "", '''
  val d = 2.675
  val a = s"$d"
  val b = toStr(d)
  if a == b then
    print("same")
  else
    print("diff")
  ''', out="same")
T(C, "05_int_stejne_jako_toStr", "", '''
  val n = -42
  val a = s"$n"
  val b = toStr(n)
  if a == b then
    print("same")
  else
    print("diff")
  ''', out="same")
T(C, "06_vysledek_je_string", "", '''
  val n = 5
  val m: String = s"abc$n"
  val l = strLength(m)
  print(l)
  ''', out="4")
T(C, "07_konkatenace_s_literalem", "",
  'val n = 5\nval m = "x" + s"$n"\nprint(m)', out="x5")
T(C, "08_konkatenace_dvou_interpolaci", "", '''
  val n = 5
  val a = s"x$n"
  val b = a + s"y$n"
  print(b)
  ''', out="x5y5")
T(C, "09_porovnani_s_literalem", "", '''
  val n = 5
  val a = s"n=$n"
  if a == "n=5" then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "10_porovnani_dvou_interpolaci", "", '''
  val a = 1
  val b = 1
  if s"v$a" == s"v$b" then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "11_jako_navratova_hodnota_funkce", "def f(n: Int): String = s\"<$n>\"",
  'val r = f(5)\nprint(r)', out="<5>")
T(C, "12_jako_parametr_a_navrat", '''
def greet(name: String): String = s"Hi $name"
''', 'val g = greet("Bob")\nprint(g)', out="Hi Bob")
T(C, "13_v_prikazu_return", '''
def f(n: Int): String =
  return s"[$n]"
''', 'val r = f(7)\nprint(r)', out="[7]")
T(C, "14_pouzity_znaky_v_hodnote_neovlivni_vysledek", "", '''
  val t = "$x ${y}"
  print(s"<$t>")
  ''', out="<$x ${y}>",
  note="obsah proměnné se už znovu neinterpoluje")
T(C, "15_prirazeni_do_var", "", '''
  val n = 5
  var m = s"a$n"
  m = s"b$n"
  print(m)
  ''', out="b5")
T(C, "16_stin_promenne_v_podbloku", "", '''
  val x = 1
  if x == 1 then
    val x = 2
    print(s"$x")
  else
    print("no")
  print(s"$x")
  ''', out="21")
T(C, "17_zpracovani_vstupu", "", '''
  val n = readInt()
  print(s"got $n")
  ''', inp="42\n", out="got 42")
T(C, "18_readLine_obsahuje_konec_radku", "", '''
  val l = readLine()
  print(s"[$l]")
  ''', inp="abc\n", out="[abc\n]")
T(C, "19_pomocna_funkce_pouziva_interpolaci", '''
def label(n: Int, name: String): String = s"$name#$n"
''', 'val a = label(1, "x")\nval b = label(22, "y")\nprint(a)\nprint(b)',
  out="x#1y#22")

# ===========================================================================
# 07 chyby
# ===========================================================================
C = "06_chyby"
T(C, "01_nedefinovana_promenna", "", 'print(s"$nope")', crc=3)
T(C, "02_nedefinovana_promenna_ve_vyrazu", "", 'print(s"${nope + 1}")',
  crc=3)
T(C, "03_typova_chyba_ve_vyrazu", "",
  'val a = "x"\nprint(s"${1 + a}")', crc=6)
T(C, "04_neukonceny_retezec", "", 'val m = s"abc\nprint("x")', crc=1)
T(C, "05_neukoncene_slozene_zavorky", "",
  'val m = s"${1 + 2"\nprint("x")', crc=(1, 2))
T(C, "06_prazdne_slozene_zavorky", "", 'val m = s"${}"\nprint("x")',
  crc=(1, 2))
T(C, "07_dolar_a_mezera", "", 'val m = s"cost $ 5"\nprint("x")',
  crc=(1, 2), note="$ musí být následován identifikátorem, { nebo $")
T(C, "08_dolar_a_cislice", "", 'val m = s"$5"\nprint("x")', crc=(1, 2))
T(C, "09_dolar_na_konci", "", 'val m = s"cost $"\nprint("x")', crc=(1, 2))
T(C, "10_tri_dolary_na_konci", "", 'val m = s"$$$"\nprint("x")',
  crc=(1, 2))
T(C, "11_jiny_interpolator_f", "", 'val n = 1\nval m = f"a $n"\nprint("x")',
  crc=(1, 2, 3), note="podporován je jen interpolátor s")
T(C, "12_jiny_interpolator_raw", "",
  'val n = 1\nval m = raw"a $n"\nprint("x")', crc=(1, 2, 3))
T(C, "13_nespravny_escape", "", r'val m = s"a\qb"' + '\nprint("x")', crc=1)
RAW(C, "14_konec_radku_v_jednoradkovem_retezci", '''
@main
def main(): Unit =
  val m = s"abc
def"
  print("x")
''', crc=1)
T(C, "15_nevyvazene_zavorky_ve_vyrazu", "",
  'val m = s"${(1 + 2}"\nprint("x")', crc=(1, 2))
T(C, "16_interpolace_do_int_promenne", "",
  'val n: Int = s"1"\nprint("x")', crc=6)
T(C, "17_interpolace_plus_int", "", 'val m = s"a" + 1\nprint("x")', crc=6)
T(C, "18_promenna_definovana_pozdeji", "",
  'val m = s"$later"\nval later = 1\nprint("x")', crc=3)
T(C, "19_mezera_mezi_s_a_uvozovkou", "",
  'val m = s "abc"\nprint("x")', crc=(2, 3))
T(C, "20_dva_identifikatory_ve_vyrazu", "",
  'val a = 1\nval b = 2\nval m = s"${a b}"\nprint("x")', crc=2)
T(C, "21_chybi_operand_ve_vyrazu", "",
  'val a = 1\nval m = s"${a +}"\nprint("x")', crc=2)
RAW(C, "22_neukonceny_viceradkovy_retezec", '''
@main
def main(): Unit =
  val m = s"""abc
def
  print("x")
''', crc=1)
T(C, "23_promenna_mimo_rozsah_bloku", "", '''
  if 1 < 2 then
    val inner = 1
    print("a")
  else
    print("b")
  print(s"$inner")
  ''', crc=3)
T(C, "24_neplatne_cislo_ve_vyrazu", "",
  'val m = s"${1 +* 2}"\nprint("x")', crc=2)

# ===========================================================================
# 99 kombinace s dalšími rozšířeními
# ===========================================================================
C = "99_kombinace_funexp"
T(C, "01_volani_uzivatelske_funkce", "def twice(x: Int): Int = x * 2",
  'print(s"${twice(3)}")', out="6")
T(C, "02_vestavena_funkce_v_interpolaci", "",
  'val w = "hello"\nprint(s"len=${strLength(w)}")', out="len=5")
T(C, "03_vnorena_volani", '''
def twice(x: Int): Int = x * 2

def add(a: Int, b: Int): Int = a + b
''', 'print(s"${add(twice(1), 3)}")', out="5")
T(C, "04_toStr_v_interpolaci", "",
  'val a = 7\nprint(s"${toStr(a)}!")', out="7!")
T(C, "05_interpolace_jako_argument_vestavene_funkce", "",
  'val n = 12\nprint(toStr(strLength(s"ab$n")))', out="4")
T(C, "06_interpolace_jako_argument_uzivatelske_funkce",
  "def len(s: String): Int = strLength(s)",
  'val n = 5\nprint(toStr(len(s"abc$n")))', out="4")
T(C, "07_poradi_vyhodnoceni_zleva_doprava", TAGS, '''
  val m = s"${pa(1)}${pb(2)}${pc(3)}"
  print("|")
  print(m)
  ''', out="abc|123")
T(C, "08_readInt_v_interpolaci", "", 'print(s"got ${readInt() + 1}")',
  inp="4\n", out="got 5")
T(C, "09_interpolace_ve_vyrazu_s_volanim", "def twice(x: Int): Int = x * 2",
  'val n = 4\nprint(toStr(twice(strLength(s"$n$n"))))', out="4")

C = "99_kombinace_boolthen"
T(C, "01_boolean_promenna", "",
  'val flag = true\nprint(s"flag=$flag")', out="flag=true")
T(C, "02_booleanovsky_vyraz", "",
  'val a = true\nval b = false\nprint(s"${a && b}/${a || b}")',
  out="false/true")
T(C, "03_porovnani_v_interpolaci", "",
  'val x = 5\nprint(s"${x > 3}")', out="true")
T(C, "04_negace", "", 'val b = false\nprint(s"${!b}")', out="true")

C = "99_kombinace_control"
T(C, "01_if_vyraz_v_interpolaci", "",
  'val x = 5\nprint(s"${if x > 0 then 1 else 2}")', out="1")
T(C, "02_ridici_promenna_cyklu_for", "",
  'for i <- 1 to 3 do print(s"[$i]")', out="[1][2][3]")
T(C, "03_interpolace_ulozena_z_if_vyrazu", "", '''
  val n = 3
  val m = if n > 2 then s"big $n" else s"small $n"
  print(m)
  ''', out="big 3")
T(C, "04_for_se_strazi", "",
  'for i <- 1 to 6 if i > 4 do print(s"$i;")', out="5;6;")

C = "99_kombinace_valmem"
T(C, "01_length_v_interpolaci", "",
  'val s1 = "abc"\nprint(s"len=${s1.length}")', out="len=3")
T(C, "02_toInt_a_toString_v_interpolaci", "", '''
  val d = 2.7
  val n = 12
  print(s"${d.toInt}/${n.toString}")
  ''', out="2/12")
T(C, "03_vysledek_interpolace_ma_length", "",
  'val n = 12\nval l = s"ab$n".length\nprint(l)', out="4")

C = "99_kombinace_extkw"
T(C, "01_rozsirujici_metoda_v_interpolaci", '''
extension (v: Int)
  def twice: Int = v * 2
''', 'val x = 6\nprint(s"${x.twice}")', out="12")
T(C, "02_rozsirujici_metoda_na_interpolaci", '''
extension (s: String)
  def shout: String = s + "!"
''', 'val n = 5\nval m = s"n=$n".shout\nprint(m)', out="n=5!")

C = "99_kombinace_paramlists"
T(C, "01_bezparametricka_funkce_v_interpolaci", "def answer: Int = 42",
  'print(s"${answer}")', out="42")
T(C, "02_vice_seznamu_v_interpolaci",
  "def add(a: Int)(b: Int): Int = a + b", 'print(s"${add(1)(2)}")', out="3")


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
