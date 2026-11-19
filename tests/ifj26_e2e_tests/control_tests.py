#!/usr/bin/env python3
"""
control_tests.py - testy rozšíření CONTROL (if jako výraz, jednořádková těla
větví, cyklus for) pro překladač IFJ26. CONTROL vyžaduje BOOLTHEN.

Vygenerováno AI (Claude, Anthropic).
Prompt: "pojďme první udělat testovací soubory (stejně jak jsme udělali
         s funexp) každý bude mít spouštění ./extensionname_tests.py prosím
         A chci aby každý testy měly solo složku"

Co se testuje (zadání, kapitola 5.3 CONTROL):
  - if jako výraz (větev else povinná, shodné typy větví, implicitní Int ->
    Double), všechny tři zápisy ze zadání (abs, abs2, abs3), else if řetězce,
    if jako poslední výraz bloku funkce, bloky s příkazy ve větvích,
  - jednořádková těla všech větví if (then/else, else if, kombinace s bloky),
  - cyklus for s jedním generátorem: from to to, from until until,
    from to to by step (i záporný krok), libovolný počet stráží (if),
    jednořádkový i víceřádkový zápis (for / generátor / stráže / do),
  - meze a krok se vyhodnotí právě jednou před první iterací, řídicí proměnná
    je nová nemodifikovatelná Int platná jen v těle, nulový krok: literál je
    chyba překladu 10, jinak chyba za běhu 40,
  - chyby: chybějící else ve výrazu, různé typy větví (6), meze/krok/stráž
    špatného typu (6), zápis do řídicí proměnné (7), proměnná mimo tělo (3),
    nepodporované konstrukce (více generátorů, yield, definice v hlavičce: 2).

Testy 01 až 07 nepoužívají FUNEXP: ve volání jsou jen termy a hodnoty se
nejdřív ukládají do proměnných (BOOLTHEN se používá, CONTROL ho vyžaduje).
Kategorie 99_kombinace_* kombinují CONTROL s dalšími rozšířeními (FUNEXP,
VALMEM, INTERPOLATEDS, EXTKW, PARAMLISTS); lze je vynechat přepínačem
--skip 99_kombinace.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup. U nejednoznačných chyb je v testu uvedena n-tice
povolených kódů (viz poznámka u testu).

Příklady:
  ./control_tests.py                                 # všechny testy
  ./control_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./control_tests.py -f straze                       # jen testy se "straze" v názvu
  ./control_tests.py --skip 99_kombinace             # bez kombinací s jinými rozšířeními
  ./control_tests.py --list                          # jen vypíše názvy testů
  ./control_tests.py --keep out_dir                  # uloží .ifj26 a .code
  ./control_tests.py --export scala_out              # uloží .scala/.in/.out pro
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
SIGN1 = "def sign(x: Int): Int = if x < 0 then -1 else if x == 0 then 0 else 1"
SIGN_BLOCK = '''
def sign(x: Int): Int =
  if x < 0 then
    -1
  else if x == 0 then
    0
  else
    1
'''
TR = """
def tr(t: String, v: Int): Int =
  print(t)
  v
"""

# ===========================================================================
# 01 if jako výraz
# ===========================================================================
C = "01_if_vyraz"
T(C, "01_jednoradkovy_ze_zadani", "",
  'var x = -5\nval absolute = if x < 0 then 0 - x else x\nprint(absolute)',
  out="5")
T(C, "02_vetev_else", "",
  'var x = 4\nval absolute = if x < 0 then 0 - x else x\nprint(absolute)',
  out="4")
T(C, "03_viceradkovy_abs2_ze_zadani", "", '''
  var x = -5
  val abs2 = if x < 0 then
    0 - x
  else
    x
  print(abs2)
  ''', out="5")
T(C, "04_abs3_ze_zadani_else_a_pokracovani_na_dalsim_radku", "", '''
  var x = -5
  val abs3 = if x < 0 then 0 - x else
      x
  print(abs3)
  ''', out="5")
T(C, "05_vsechny_tri_zapisy_ze_zadani_pohromade", "", '''
  var x = -5
  val absolute = if x < 0 then 0 - x else x // v 'absolute' je hodnota 5
  // ukázky různých kombinací jednořádkových těl a bloků
  val abs2 = if x < 0 then
    0 - x
  else
    x
  //
  val abs3 = if x < 0 then 0 - x else
      x
  print(absolute)
  print(abs2)
  print(abs3)
  ''', out="555")
T(C, "06_viceradkove_zapisy_s_kladnym_cislem", "", '''
  var x = 4
  val abs2 = if x < 0 then
    0 - x
  else
    x
  val abs3 = if x < 0 then 0 - x else
      x
  print(abs2)
  print(abs3)
  ''', out="44")
T(C, "07_then_blok_else_jednoradkove", "", '''
  var x = 5
  val m = if x > 0 then
    x * 2
  else 0
  x = -1
  val n = if x > 0 then
    x * 2
  else 0
  print(m)
  print(" ")
  print(n)
  ''', out="10 0")
T(C, "08_then_jednoradkove_else_na_dalsim_radku", "", '''
  val x = -3
  val m = if x > 0 then x else
    0 - x
  print(m)
  ''', out="3")
T(C, "09_else_if_retezec_jednoradkovy", "", '''
  val a = -4
  val b = 0
  val c = 9
  val r1 = if a < 0 then -1 else if a == 0 then 0 else 1
  val r2 = if b < 0 then -1 else if b == 0 then 0 else 1
  val r3 = if c < 0 then -1 else if c == 0 then 0 else 1
  print(r1)
  print(" ")
  print(r2)
  print(" ")
  print(r3)
  ''', out="-1 0 1")
T(C, "10_else_if_retezec_viceradkovy", "", '''
  val x = 0
  val r = if x < 0 then
    -1
  else if x == 0 then
    0
  else
    1
  print(r)
  ''', out="0")
T(C, "11_zanoreny_if_vyraz_v_zavorkach", "", '''
  val a = true
  val b = false
  val r = if a then (if b then 1 else 2) else 3
  print(r)
  ''', out="2")
T(C, "12_vetve_s_bloky_prikazu", "", '''
  val x = 5
  val r = if x > 0 then
    val t = x * 2
    t + 1
  else
    0
  print(r)
  ''', out="11")
T(C, "13_vyhodnoti_se_jen_provedena_vetev", "", '''
  val d = 0
  val r = if d != 0 then 10 / d else 0
  val q = if d == 0 then 0 else 10 / d
  print(r)
  print(q)
  ''', out="00",
  note="neprovedená větev by dělila nulou (chyba 57)")
T(C, "14_if_vyraz_v_zavorkach_uvnitr_aritmetiky", "", '''
  val x = 1
  val y = -1
  val r1 = 1 + (if x > 0 then 2 else 3)
  val r2 = 1 + (if y > 0 then 2 else 3)
  print(r1)
  print(r2)
  ''', out="34")
T(C, "15_prirazeni_if_vyrazu", "", '''
  val x = 1
  var r = 0
  r = if x > 0 then 1 else 2
  print(r)
  ''', out="1")
T(C, "16_vetve_typu_string", "", '''
  val x = 3
  val s = if x > 0 then "pos" else "neg"
  print(s)
  ''', out="pos")
T(C, "17_vetve_typu_boolean", "", '''
  val x = 3
  val b = if x > 0 then true else false
  print(b)
  ''', out="true")
T(C, "18_vetve_typu_double", "", '''
  val c = false
  val d = if c then 1.5 else 2.5
  val s = toStr(d)
  print(s)
  ''', out="2.50")
T(C, "19_int_a_double_vetev_vysledek_je_double_else", "", '''
  val c = false
  val d = if c then 1 else 2.5
  val s = toStr(d)
  print(s)
  ''', out="2.50", note="Int větev se implicitně převede na Double")
T(C, "20_int_a_double_vetev_vysledek_je_double_then", "", '''
  val c = true
  val d = if c then 1 else 2.5
  if d == 1.0 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "21_deklarovany_typ_double_s_int_vetvemi", "", '''
  val c = true
  val d: Double = if c then 1 else 2
  if d == 1.0 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "22_deklarovany_typ_int", "", '''
  val x = -2
  val r: Int = if x > 0 then x else 0 - x
  print(r)
  ''', out="2")
T(C, "23_slozena_podminka", "", '''
  val a = 1
  val b = 2
  val r = if a > 0 && b > 0 then 1 else 0
  print(r)
  ''', out="1")
T(C, "24_telo_funkce_je_if_vyraz",
  "def max(a: Int, b: Int): Int = if a > b then a else b", '''
  val r1 = max(3, 9)
  val r2 = max(10, 2)
  print(r1)
  print(" ")
  print(r2)
  ''', out="9 10")
T(C, "25_if_jako_posledni_vyraz_bloku_funkce", SIGN_BLOCK, '''
  val a = sign(-5)
  val b = sign(0)
  val c = sign(7)
  print(a)
  print(" ")
  print(b)
  print(" ")
  print(c)
  ''', out="-1 0 1")
T(C, "26_if_vyraz_v_prikazu_return", '''
def absv(n: Int): Int =
  return if n > 0 then n else 0 - n
''', 'val r = absv(-4)\nprint(r)', out="4")
T(C, "27_funkce_vracejici_string_pres_if_vyraz", '''
def kind(n: Int): String =
  if n < 0 then
    "neg"
  else if n == 0 then
    "zero"
  else
    "pos"
''', '''
  val a = kind(-3)
  val b = kind(0)
  val c = kind(3)
  print(a)
  print(b)
  print(c)
  ''', out="negzeropos")
T(C, "28_vnorene_if_vyrazy_v_blocich", "", '''
  val x = 5
  val r = if x > 0 then
    if x > 3 then
      2
    else
      1
  else
    0
  print(r)
  ''', out="2")
T(C, "29_vedlejsi_efekty_jen_v_provedene_vetvi", "", '''
  var n = 0
  val r = if n == 0 then
    n = n + 1
    10
  else
    n = n + 100
    20
  print(n)
  print(",")
  print(r)
  ''', out="1,10")
T(C, "30_funkce_s_odvozenym_typem_z_if_vyrazu",
  "def pick(c: Boolean) = if c then 1 else 2", '''
  val a = pick(true)
  val b = pick(false)
  print(a)
  print(b)
  ''', out="12")
T(C, "31_if_vyraz_s_funkcemi_ve_vetvich_bez_funexp", D(
  "def one(): Int = 1", "def two(): Int = 2"), '''
  val a = one()
  val b = two()
  val r = if a > b then a else b
  print(r)
  ''', out="2")
T(C, "32_if_vyraz_vraci_hodnotu_do_podminky_dalsiho_if", "", '''
  val x = 4
  val big = if x > 3 then true else false
  if big then
    print("big")
  else
    print("small")
  ''', out="big")

# ===========================================================================
# 02 jednořádková těla větví (příkazy)
# ===========================================================================
C = "02_jednoradkova_tela"
T(C, "01_if_else_na_jednom_radku", "",
  'val x = 1\nif x > 0 then print("p") else print("n")', out="p")
T(C, "02_zkraceny_if_na_jednom_radku", "",
  'val x = 1\nif x > 0 then print("p")\nprint("end")', out="pend")
T(C, "03_zkraceny_if_neprovedeny", "",
  'val x = -1\nif x > 0 then print("p")\nprint("end")', out="end")
T(C, "04_else_na_dalsim_radku_s_jednoradkovymi_teli", "", '''
  val x = -1
  if x > 0 then print("p")
  else print("n")
  ''', out="n")
T(C, "05_then_blok_else_jednoradkove", "", '''
  val x = -1
  if x > 0 then
    print("p")
    print("!")
  else print("n")
  ''', out="n")
T(C, "06_then_jednoradkove_else_blok", "", '''
  val x = -1
  if x > 0 then print("p")
  else
    print("n")
    print("!")
  ''', out="n!")
T(C, "07_else_if_s_jednoradkovymi_teli", "", '''
  val x = 0
  if x < 0 then print("neg")
  else if x == 0 then print("zero")
  else print("pos")
  ''', out="zero")
T(C, "08_prirazeni_v_jednoradkovem_tele", "", '''
  val x = 1
  var r = 0
  if x > 0 then r = 1 else r = 2
  print(r)
  ''', out="1")
T(C, "09_jednoradkova_funkce_s_if_vyrazem", SIGN1, '''
  val a = sign(-5)
  val b = sign(0)
  val c = sign(3)
  print(a)
  print(" ")
  print(b)
  print(" ")
  print(c)
  ''', out="-1 0 1")
T(C, "10_jednoradkovy_if_v_jednoradkovem_for", "",
  'for i <- 1 to 5 do if i > 3 then print(i)', out="45")
T(C, "11_vnorene_jednoradkove_if_else", "", '''
  val x = 3
  if x > 0 then
    if x > 5 then print("big") else print("small")
  else print("neg")
  ''', out="small")
T(C, "12_jednoradkovy_if_else_ve_while", "", '''
  var i = 0
  while i < 5 do
    if i == 2 then print("two") else print(i)
    i = i + 1
  ''', out="01two34")
T(C, "13_jednoradkove_vetve_v_zanorenem_bloku_funkce", '''
def show(n: Int): Unit =
  if n > 0 then print("+") else print("-")
''', 'show(1)\nshow(-1)\nshow(2)', out="+-+")
T(C, "14_zkraceny_jednoradkovy_if_ve_funkci_unit", '''
def maybe(n: Int): Unit =
  if n > 0 then print("p")
''', 'maybe(1)\nmaybe(-1)\nmaybe(2)', out="pp")

# ===========================================================================
# 03 cyklus for: rozsahy
# ===========================================================================
C = "03_for_rozsahy"
T(C, "01_to_jednoradkovy", "", 'for i <- 1 to 5 do print(i)', out="12345")
T(C, "02_until_nezahrnuje_pravy_okraj", "",
  'for i <- 1 until 5 do print(i)', out="1234")
T(C, "03_to_by_ze_zadani", "", 'for i <- 1 to 10 by 2 do print(i)',
  out="13579")
T(C, "04_blokove_telo", "", '''
  for i <- 1 to 3 do
    print(i)
    print(",")
  ''', out="1,2,3,")
T(C, "05_zaporny_krok", "", '''
  for i <- 10 to 1 by -3 do
    print(i)
    print(" ")
  ''', out="10 7 4 1 ")
T(C, "06_prazdny_rozsah_to", "",
  'for i <- 5 to 1 do print(i)\nprint("end")', out="end")
T(C, "07_prazdny_rozsah_until", "",
  'for i <- 3 until 3 do print(i)\nprint("end")', out="end")
T(C, "08_jeden_prvek", "", 'for i <- 1 to 1 do print(i)', out="1")
T(C, "09_krok_vetsi_nez_rozsah", "",
  'for i <- 1 to 3 by 10 do print(i)', out="1")
T(C, "10_krok_presne_trefi_hranici", "", '''
  for i <- 0 to 10 by 5 do
    print(i)
    print(" ")
  ''', out="0 5 10 ")
T(C, "11_zaporne_meze", "", '''
  for i <- -3 to 3 do
    print(i)
    print(" ")
  ''', out="-3 -2 -1 0 1 2 3 ")
T(C, "12_ridici_promenna_v_aritmetice", "", '''
  var sum = 0
  for i <- 1 to 4 do
    sum = sum + i * i
  print(sum)
  ''', out="30")
T(C, "13_faktorial", "", '''
  var f = 1
  for i <- 1 to 5 do
    f = f * i
  print(f)
  ''', out="120")
T(C, "14_meze_jsou_promenne", "", '''
  val lo = 2
  val hi = 6
  for i <- lo to hi do print(i)
  ''', out="23456")
T(C, "15_meze_jsou_vyrazy", "", '''
  val a = 1
  val b = 3
  for i <- a + 1 to b * 2 do print(i)
  ''', out="23456")
T(C, "16_horni_mez_se_vyhodnoti_jednou", "", '''
  var n = 3
  for i <- 1 to n do
    n = n + 1
    print(i)
  print(",")
  print(n)
  ''', out="123,6")
T(C, "17_krok_se_vyhodnoti_jednou", "", '''
  var s = 1
  for i <- 1 to 5 by s do
    s = s + 1
    print(i)
  ''', out="12345")
T(C, "18_dolni_mez_se_vyhodnoti_jednou", "", '''
  var a = 1
  for i <- a to 3 do
    a = 100
    print(i)
  ''', out="123")
T(C, "19_ridici_promenna_zastini_vnejsi", "", '''
  val i = 100
  for i <- 1 to 2 do print(i)
  print(i)
  ''', out="12100")
T(C, "20_vnorene_cykly", "", '''
  for i <- 1 to 3 do
    for j <- 1 to 3 do
      print(i * j)
    print(",")
  ''', out="123,246,369,")
T(C, "21_vnorena_promenna_zastini_vnejsi_ridici", "", '''
  for i <- 1 to 2 do
    for i <- 5 to 6 do
      print(i)
  ''', out="5656")
T(C, "22_for_ve_while", "", '''
  var k = 0
  while k < 2 do
    for i <- 1 to 3 do print(i)
    print("|")
    k = k + 1
  ''', out="123|123|")
T(C, "23_while_ve_for", "", '''
  for i <- 1 to 2 do
    var j = 0
    while j < i do
      print(j)
      j = j + 1
    print("|")
  ''', out="0|01|")
T(C, "24_return_z_cyklu", '''
def firstOver(n: Int): Int =
  for i <- 1 to 100 do
    if i * i > n then
      return i
  return 0
''', 'val a = firstOver(50)\nval b = firstOver(100000)\nprint(a)\nprint(",")\nprint(b)',
  out="8,0")
T(C, "25_for_jako_posledni_prikaz_funkce_unit", '''
def count(n: Int): Unit =
  for i <- 1 to n do
    print(i)
''', 'count(3)', out="123")
T(C, "26_velky_rozsah", "", '''
  var total = 0
  for i <- 1 to 1000 do
    total = total + i
  print(total)
  ''', out="500500")
T(C, "27_krok_je_promenna", "", '''
  val st = 3
  for i <- 0 to 9 by st do
    print(i)
    print(" ")
  ''', out="0 3 6 9 ")
T(C, "28_klesajici_rozsah_krokem_minus_jedna", "",
  'for i <- 3 to 1 by -1 do print(i)', out="321")
T(C, "29_zaporny_krok_u_rostouciho_rozsahu_nic_nevykona", "",
  'for i <- 1 to 5 by -1 do print(i)\nprint("end")', out="end")
T(C, "30_retezec_ze_ridici_promenne", "", '''
  var s = ""
  for i <- 1 to 3 do
    val t = toStr(i)
    s = s + t
  print(s)
  ''', out="123")
T(C, "31_for_volany_z_funkce_s_parametrem", '''
def sumTo(n: Int): Int =
  var s = 0
  for i <- 1 to n do
    s = s + i
  s
''', 'val r = sumTo(10)\nprint(r)', out="55")
T(C, "32_dve_funkce_s_cykly_za_sebou", '''
def sumTo(n: Int): Int =
  var s = 0
  for i <- 1 to n do
    s = s + i
  s

def prodTo(n: Int): Int =
  var p = 1
  for i <- 1 to n do
    p = p * i
  p
''', 'val a = sumTo(4)\nval b = prodTo(4)\nprint(a)\nprint(",")\nprint(b)',
  out="10,24")
T(C, "33_jednoradkovy_for_s_prirazenim", "", '''
  var s = 0
  for i <- 1 to 4 do s = s + i
  print(s)
  ''', out="10")
T(C, "34_komentar_za_zahlavim", "", '''
  for i <- 1 to 3 do // projdi 1..3
    print(i) // vypiš
  ''', out="123")

# ===========================================================================
# 04 stráže a víceřádkový zápis
# ===========================================================================
C = "04_straze_a_viceradkovy_zapis"
T(C, "01_priklad_ze_zadani", "", '''
  val limit = 10
  for
    i <- 0 until limit
    if i != 3
    if i < 8
  do
    print(i)
  ''', out="0124567")
T(C, "02_jedna_straz_na_jednom_radku", "",
  'for i <- 1 to 10 if i > 7 do print(i)', out="8910")
T(C, "03_dve_straze_na_jednom_radku", "",
  'for i <- 1 to 10 if i > 3 if i < 6 do print(i)', out="45")
T(C, "04_straz_na_sudost_pres_celociselne_deleni", "",
  'for i <- 1 to 10 if i / 2 * 2 == i do print(i)', out="246810")
T(C, "05_straz_odkazuje_na_vnejsi_promennou", "", '''
  val skip = 3
  for i <- 1 to 5 if i != skip do print(i)
  ''', out="1245")
T(C, "06_straz_se_vyhodnocuje_pri_kazde_iteraci", "", '''
  var count = 0
  for i <- 1 to 5 if count < 2 do
    count = count + 1
    print(i)
  ''', out="12")
T(C, "07_straz_nikdy_nesplnena", "",
  'for i <- 1 to 5 if i > 10 do print(i)\nprint("end")', out="end")
T(C, "08_straze_se_vyhodnocuji_postupne_zleva_doprava", "",
  'for i <- 0 to 3 if i != 0 if 10 / i > 3 do print(i)', out="12",
  note="pro i = 0 se druhá stráž nesmí vyhodnotit (dělení nulou)")
T(C, "09_viceradkovy_zapis_s_jednoradkovym_telem_za_do", "", '''
  for
    i <- 1 to 3
  do print(i)
  ''', out="123")
T(C, "10_viceradkovy_zapis_bez_straze_s_blokem", "", '''
  for
    i <- 1 to 3
  do
    print(i)
    print(";")
  ''', out="1;2;3;")
T(C, "11_viceradkovy_zapis_se_slozitou_strazi", "", '''
  for
    i <- 1 to 6
    if i > 2 && i < 5
  do
    print(i)
  ''', out="34")
T(C, "12_jednoradkovy_zapis_s_krokem_a_strazi", "",
  'for i <- 1 to 10 by 3 if i != 4 do print(i)', out="1710")
T(C, "13_viceradkovy_zapis_s_krokem_a_strazi", "", '''
  for
    i <- 10 to 1 by -3
    if i != 7
  do
    print(i)
    print(" ")
  ''', out="10 4 1 ")
T(C, "14_komentare_v_hlavicce", "", '''
  for
    i <- 1 to 3 // generátor
    if i != 2   // stráž
  do
    print(i)
  ''', out="13")
T(C, "15_straz_s_boolean_promennou", "", '''
  val flag = true
  for i <- 1 to 3 if flag do print(i)
  ''', out="123")
T(C, "16_straz_s_negaci", "",
  'for i <- 1 to 4 if !(i == 2) do print(i)', out="134")
T(C, "17_vnorene_cykly_se_strazemi", "", '''
  for i <- 1 to 3 if i != 2 do
    for j <- 1 to 3 if j != i do
      print(i)
      print(j)
      print(" ")
  ''', out="12 13 31 32 ")
T(C, "18_strazi_s_or", "",
  'for i <- 1 to 6 if i < 2 || i > 5 do print(i)', out="16")
T(C, "19_tri_straze", "", '''
  for
    i <- 1 to 20
    if i > 5
    if i < 15
    if i / 3 * 3 == i
  do
    print(i)
    print(" ")
  ''', out="6 9 12 ")
T(C, "20_viceradkovy_zapis_s_until_a_strazi", "", '''
  val n = 6
  for
    i <- 0 until n
    if i != 0
  do
    print(i)
  ''', out="12345")
T(C, "21_viceradkovy_zapis_uvnitr_vnoreneho_bloku", '''
def run(n: Int): Unit =
  if n > 0 then
    for
      i <- 1 to n
      if i != 2
    do
      print(i)
''', 'run(4)', out="134")
T(C, "22_jednoradkova_tela_i_u_viceradkoveho_zapisu_a_vnoreneho_if", "", '''
  for
    i <- 1 to 4
  do
    if i / 2 * 2 == i then print("e") else print("o")
  ''', out="oeoe")

# ===========================================================================
# 05 nulový krok
# ===========================================================================
C = "05_nulovy_krok"
T(C, "01_krok_nula_za_behu_ma_kod_40", "", '''
  print("a")
  val s = readInt()
  for i <- 1 to 5 by s do print(i)
  print("b")
  ''', inp="0\n", out="a", rc=40,
  note="krok není literál: kontrola až za běhu, návratový kód 40")
T(C, "02_nenulovy_krok_ze_vstupu", "", '''
  val s = readInt()
  for i <- 1 to 5 by s do print(i)
  ''', inp="3\n", out="14")
T(C, "03_zaporny_krok_ze_vstupu", "", '''
  val s = readInt()
  for i <- 5 to 1 by s do print(i)
  ''', inp="-2\n", out="531")
T(C, "04_prazdny_rozsah_s_nulovym_krokem_za_behu", "", '''
  val s = readInt()
  for i <- 5 to 1 by s do print(i)
  print("b")
  ''', inp="0\n", out="", rc=40,
  note="nulový krok je chyba i tehdy, když by byl rozsah prázdný")

# ===========================================================================
# 06 typické programy s for a if výrazy
# ===========================================================================
C = "06_typicke_programy"
T(C, "01_if_vyraz_v_tele_cyklu", "", '''
  for i <- 1 to 5 do
    val t = if i / 2 * 2 == i then 1 else 0
    print(t)
  ''', out="01010")
T(C, "02_for_ve_vetvi_if_vyrazu", "", '''
  val x = 4
  val r = if x > 0 then
    var s = 0
    for i <- 1 to x do s = s + i
    s
  else
    0
  print(r)
  ''', out="10")
T(C, "03_fizzbuzz", "", '''
  for i <- 1 to 15 do
    if i / 15 * 15 == i then print("FizzBuzz")
    else if i / 3 * 3 == i then print("Fizz")
    else if i / 5 * 5 == i then print("Buzz")
    else print(i)
    print(" ")
  ''', out="1 2 Fizz 4 Buzz Fizz 7 8 Fizz Buzz 11 Fizz 13 14 FizzBuzz ")
T(C, "04_prvocisla_pod_dvacet", "", '''
  for n <- 2 to 20 do
    var isPrime = true
    for d <- 2 until n do
      if n / d * d == n then
        isPrime = false
    if isPrime then
      print(n)
      print(" ")
  ''', out="2 3 5 7 11 13 17 19 ")
T(C, "05_fibonacci_pomoci_for", "", '''
  var a = 0
  var b = 1
  for i <- 1 to 10 do
    val t = a + b
    a = b
    b = t
  print(a)
  ''', out="55")
T(C, "06_nejvetsi_prvek_pomoci_if_vyrazu", "", '''
  var best = 0
  for i <- 1 to 10 do
    val v = i * (11 - i)
    best = if v > best then v else best
  print(best)
  ''', out="30")
T(C, "07_soucet_licheho_a_sudeho", "", '''
  var odd = 0
  var even = 0
  for i <- 1 to 10 do
    if i / 2 * 2 == i then even = even + i else odd = odd + i
  print(odd)
  print(" ")
  print(even)
  ''', out="25 30")
T(C, "08_trojuhelnik_z_hvezdicek", "", '''
  for i <- 1 to 3 do
    for j <- 1 to i do print("*")
    print("\\n")
  '''.replace("\\\\n", "\\n"), out="*\n**\n***\n")
T(C, "09_obracene_poradi_pomoci_zaporneho_kroku", "", '''
  var s = ""
  for i <- 5 to 1 by -1 do
    val t = toStr(i)
    s = s + t
  print(s)
  ''', out="54321")
T(C, "10_vyhledani_prvniho_deliteleho_tremi_se_strazi", "", '''
  var found = 0
  for i <- 10 to 30 if i / 3 * 3 == i if found == 0 do found = i
  print(found)
  ''', out="12")
T(C, "11_while_a_for_dohromady_pocitadlo", "", '''
  var rounds = 0
  var total = 0
  while rounds < 3 do
    for i <- 1 to rounds + 1 do total = total + i
    rounds = rounds + 1
  print(total)
  ''', out="10")

# ===========================================================================
# 07 chyby
# ===========================================================================
C = "07_chyby"
T(C, "01_if_vyraz_bez_else", "",
  'val x = 1\nval r = if x > 0 then 1\nprint("x")', crc=(2, 6),
  note="u if jako výrazu je else povinné")
T(C, "02_vetve_ruznych_typu_int_string", "",
  'val x = 1\nval r = if x > 0 then 1 else "a"\nprint("x")', crc=6)
T(C, "03_vetve_ruznych_typu_int_boolean", "",
  'val x = 1\nval r = if x > 0 then 1 else true\nprint("x")', crc=6)
T(C, "04_vetve_ruznych_typu_string_double", "",
  'val x = 1\nval r = if x > 0 then "a" else 1.5\nprint("x")', crc=6)
T(C, "05_podminka_if_vyrazu_neni_boolean", "",
  'val r = if 1 then 2 else 3\nprint("x")', crc=6)
T(C, "06_double_vetve_do_int_promenne", "",
  'val c = true\nval n: Int = if c then 1.5 else 2.5\nprint("x")', crc=6)
T(C, "07_vysledek_do_spatneho_typu", "",
  'val x = 1\nval s: String = if x > 0 then 1 else 2\nprint("x")', crc=6)
T(C, "08_unit_vetev_a_int_vetev", "",
  'val x = 1\nval r = if x > 0 then print("a") else 1\nprint("x")', crc=6)
T(C, "09_else_if_retezec_bez_posledniho_else_ve_vyrazu", "",
  'val x = 1\nval r = if x > 0 then 1 else if x < 0 then 2\nprint("x")',
  crc=(2, 6))
T(C, "10_else_odsazene_o_mezeru_ve_viceradkovem_vyrazu", "", '''
  val x = 1
  val r = if x > 0 then
    1
   else
    2
  print("x")
  ''', crc=(1, 2))
T(C, "11_else_odsazene_do_bloku_then_vetve", "", '''
  val x = 1
  val r = if x > 0 then
    1
    else
    2
  print("x")
  ''', crc=(1, 2))
T(C, "12_funkce_s_if_vyrazem_a_spatnym_typem_vetve",
  'def f(x: Int): Int = if x > 0 then 1 else "a"', 'print("x")', crc=6)
T(C, "13_funkce_vracejici_string_z_int_vetvi", '''
def f(x: Int): String =
  if x > 0 then
    1
  else
    2
''', 'print("x")', crc=6)
T(C, "14_for_bez_do", "", 'for i <- 1 to 5\n  print(i)', crc=2)
T(C, "15_for_s_in_misto_sipky", "", 'for i in 1 to 5 do print(i)', crc=2)
T(C, "16_double_horni_mez", "", 'for i <- 1 to 2.5 do print(i)', crc=6)
T(C, "17_double_dolni_mez", "", 'for i <- 1.0 to 3 do print(i)', crc=6)
T(C, "18_retezcova_mez", "", 'for i <- 1 to "a" do print(i)', crc=6)
T(C, "19_boolean_mez", "", 'for i <- 1 to true do print(i)', crc=6)
T(C, "20_double_krok", "", 'for i <- 1 to 5 by 0.5 do print(i)', crc=6)
T(C, "21_retezcove_meze", "", 'for i <- "a" to "c" do print(i)', crc=6)
T(C, "22_zapis_do_ridici_promenne", "",
  'for i <- 1 to 3 do\n  i = 5', crc=7)
T(C, "23_ridici_promenna_mimo_telo", "",
  'for i <- 1 to 3 do print(i)\nprint(i)', crc=3)
T(C, "24_nedefinovana_promenna_v_mezi", "",
  'for i <- 1 to nope do print(i)', crc=3)
T(C, "25_straz_neni_boolean", "",
  'for i <- 1 to 3 if i do print(i)', crc=6)
T(C, "26_straz_je_retezec", "",
  'for i <- 1 to 3 if "a" do print(i)', crc=6)
T(C, "27_nulovy_literal_jako_krok", "",
  'for i <- 1 to 5 by 0 do print(i)', crc=10,
  note="překladač kontroluje nulový krok alespoň u literálu (kód 10)")
T(C, "28_vice_generatoru", "", '''
  for
    i <- 1 to 2
    j <- 1 to 2
  do
    print(i)
  ''', crc=2, note="více generátorů není podporováno")
T(C, "29_yield_neni_podporovan", "",
  'val r = for i <- 1 to 3 yield i\nprint("x")', crc=2)
T(C, "30_straz_neni_zarovnana_s_generatorem", "", '''
  for
    i <- 1 to 3
     if i != 2
  do
    print(i)
  ''', crc=(1, 2))
T(C, "31_generator_neni_odsazen_vice_nez_for", "", '''
  for
  i <- 1 to 3
  do
    print(i)
  ''', crc=2)
T(C, "32_do_neni_zarovnano_s_for", "", '''
  for
    i <- 1 to 3
    do
    print(i)
  ''', crc=(1, 2))
T(C, "33_definice_v_hlavicce", "", '''
  for
    i <- 1 to 3
    x = i * 2
  do
    print(x)
  ''', crc=2, note="definice uvnitř hlavičky nejsou podporovány")
T(C, "34_telo_cyklu_chybi", "", 'for i <- 1 to 3 do', crc=2)
T(C, "35_chybi_sipka", "", 'for i = 1 to 3 do print(i)', crc=2)
T(C, "36_zdrojem_je_jedno_cislo", "", 'for i <- 5 do print(i)',
  crc=(2, 6), note="generátor musí být rozsah (nebo List s rozšířením LISTS)")
T(C, "37_telo_cyklu_pouziva_nedefinovanou_funkci", "",
  'for i <- 1 to 3 do nope(i)', crc=3)
T(C, "38_if_vyraz_bez_then", "", 'val x = 1\nval r = if x > 0 1 else 2', crc=2)
T(C, "39_tabulator_v_odsazeni_hlavicky", "",
  'for\n\ti <- 1 to 3\ndo\n  print(i)', crc=(1, 2))
T(C, "40_for_jako_nazev_promenne", "", 'val for = 1\nprint("x")', crc=2)
T(C, "41_by_bez_hodnoty", "", 'for i <- 1 to 5 by do print(i)', crc=2)
T(C, "42_krok_u_until_neni_podporovan_ale_nekonci_chybou_typu_ani_nahodne",
  "", 'for i <- 1 until 5 by 2 do print(i)', crc=(2, 6, 0),
  note="zadání uvádí by jen u to; překladač smí buď odmítnout (2) nebo "
       "program přeložit, test jen hlídá, že nespadne")

# ===========================================================================
# 99 kombinace s dalšími rozšířeními
# ===========================================================================
C = "99_kombinace_funexp"
T(C, "01_meze_s_volanim_se_vyhodnoti_jednou", TR,
  'for i <- 1 to tr("n", 3) do print(i)', out="n123")
T(C, "02_poradi_dolni_horni_krok", TR,
  'for i <- tr("a", 1) to tr("b", 3) by tr("c", 1) do print(i)',
  out="abc123")
T(C, "03_straz_s_volanim_se_vyhodnocuje_pri_kazde_iteraci", TR,
  'for i <- 1 to 3 if tr("g", 1) == 1 do print(i)', out="g1g2g3")
T(C, "04_if_vyraz_jako_argument_print", "",
  'val x = 1\nprint(if x > 0 then "pos" else "neg")', out="pos")
T(C, "05_if_vyraz_jako_argument_toStr", "",
  'val x = 1\nprint(toStr(if x > 0 then 10 else 20))', out="10")
T(C, "06_vyhodnoti_se_jen_provedena_vetev_s_volanim", TR, '''
  val x = 1
  val r = if x > 0 then tr("t", 1) else tr("e", 2)
  print("|")
  print(r)
  ''', out="t|1")
T(C, "07_if_vyraz_s_volanim_v_podmince", '''
def isEven(n: Int): Boolean = n / 2 * 2 == n
''', '''
  val a = if isEven(4) then "even" else "odd"
  val b = if isEven(3) then "even" else "odd"
  print(a)
  print(",")
  print(b)
  ''', out="even,odd")
T(C, "08_if_vyraz_jako_argument_uzivatelske_funkce",
  "def add(a: Int, b: Int): Int = a + b",
  'val c = true\nprint(toStr(add(if c then 1 else 2, 3)))', out="4")
T(C, "09_horni_mez_je_readInt", "",
  'for i <- 1 to readInt() do print(i)', inp="3\n", out="123")
T(C, "10_telo_cyklu_s_volanim_funkce", "def dbl(x: Int): Int = x * 2",
  'for i <- 1 to 3 do print(toStr(dbl(i)))', out="246")
T(C, "11_rekurzivni_funkce_s_if_vyrazem_bez_return", '''
def fact(n: Int): Int =
  if n <= 1 then
    1
  else
    n * fact(n - 1)
''', 'print(toStr(fact(5)))', out="120")
T(C, "12_funkce_s_for_volana_v_mezi", '''
def three(): Int = 3
''', 'for i <- 1 to three() do print(i)', out="123")

C = "99_kombinace_valmem"
T(C, "01_length_v_mezi", "",
  'val s1 = "abc"\nfor i <- 1 to s1.length do print(i)', out="123")
T(C, "02_toString_v_if_vyrazu", "", '''
  val n = 10
  val t = if n > 5 then n.toString else "small"
  print(t)
  ''', out="10")
T(C, "03_toString_ve_strazi", "",
  'for i <- 1 until 15 if i.toString.length == 1 do print(i)',
  out="123456789")
T(C, "04_floor_toInt_v_mezi", "",
  'val d = 3.7\nfor i <- 1 to d.floor.toInt do print(i)', out="123")

C = "99_kombinace_interpolateds"
T(C, "01_ridici_promenna_v_interpolaci", "",
  'for i <- 1 to 3 do print(s"[$i]")', out="[1][2][3]")
T(C, "02_if_vyraz_v_interpolaci", "",
  'val x = 5\nprint(s"${if x > 0 then 1 else 2}")', out="1")
T(C, "03_interpolace_ve_vetvich_if_vyrazu", "", '''
  val n = 3
  val m = if n > 2 then s"big $n" else s"small $n"
  print(m)
  ''', out="big 3")

C = "99_kombinace_extkw"
T(C, "01_if_vyraz_v_tele_metody", '''
extension (v: Int)
  def abs2: Int = if v < 0 then 0 - v else v
''', 'val x = 0 - 7\nval r = x.abs2\nprint(r)', out="7")
T(C, "02_metoda_v_mezi_cyklu", '''
extension (v: Int)
  def twice: Int = v * 2
''', 'val x = 2\nfor i <- 1 to x.twice do print(i)', out="1234")

C = "99_kombinace_paramlists"
T(C, "01_bezparametricka_funkce_jako_mez", "def n: Int = 3",
  'for i <- 1 to n do print(i)', out="123")
T(C, "02_if_vyraz_ve_funkci_s_vice_seznamy",
  "def pick(c: Boolean)(a: Int)(b: Int): Int = if c then a else b",
  'val r = pick(false)(1)(2)\nprint(r)', out="2")
T(C, "03_mez_cyklu_ma_vedlejsi_efekt_a_vyhodnoti_se_jednou", '''
def tickN: Int =
  print("n")
  3
''', 'for i <- 1 to tickN do print(i)', out="n123")


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
