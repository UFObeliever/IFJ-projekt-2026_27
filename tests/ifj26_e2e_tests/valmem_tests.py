#!/usr/bin/env python3
"""
valmem_tests.py - testy rozšíření VALMEM (členy hodnot: výraz.id) pro překladač
IFJ26.

Vygenerováno AI (Claude, Anthropic).
Prompt: "pojďme první udělat testovací soubory (stejně jak jsme udělali
         s funexp) každý bude mít spouštění ./extensionname_tests.py prosím
         A chci aby každý testy měly solo složku"

Co se testuje (zadání, kapitola 5.4 VALMEM) - právě tyto členy:
  libovolná hodnota   .toString   String
  hodnota typu Double .toInt      Int      (oseknutí desetinné části)
  hodnota typu Double .floor      Double
  hodnota typu String .length     Int
  - členy se zapisují bez prázdného seznamu argumentů, lze je řetězit
    (number.floor.toInt), příjemcem může být proměnná, literál, výraz
    v závorkách nebo výsledek jiného členu,
  - .toString u Double používá stejný formát jako toStr ("1.2345" -> "1.23").

Testy 01 až 08 nepoužívají FUNEXP: členy se vždy nejdřív uloží do proměnné
a teprve ta se předává dál. Kategorie 99_kombinace_* kombinují VALMEM s dalšími
rozšířeními (FUNEXP, BOOLTHEN, CONTROL, INTERPOLATEDS, EXTKW); lze je vynechat
přepínačem --skip 99_kombinace.

Chyby: použití člena na hodnotě nesprávného typu a neexistující členy nejsou
v zadání přesně zařazeny pod jeden kód (typová chyba 6 / nedefinovaný člen 3 /
syntaxe 2), proto u nich test připouští n-tici kódů (viz poznámka u testu).
Neočekávané tvary (např. člen s prázdnými závorkami) vedou na chybu 2 nebo 5.

Test je zdrojový program IFJ26 + vstup + očekávaný stdout (a návratový kód
interpretu), nebo program, který musí překladač odmítnout s daným kódem.
Při chybě kompilace se kontroluje návratový kód překladače a to, že na stdout
nevznikl žádný výstup.

Příklady:
  ./valmem_tests.py                                  # všechny testy
  ./valmem_tests.py --compiler ../../compiler --interp ./ic26int -v
  ./valmem_tests.py -f floor                         # jen testy s "floor" v názvu
  ./valmem_tests.py --skip 99_kombinace              # bez kombinací s jinými rozšířeními
  ./valmem_tests.py --list                           # jen vypíše názvy testů
  ./valmem_tests.py --keep out_dir                   # uloží .ifj26 a .code
  ./valmem_tests.py --export scala_out               # uloží .scala/.in/.out pro
                                                     # ověření skutečnou Scalou
                                                     # (u Double se liší formát)

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
# ===========================================================================
# 01 .toString
# ===========================================================================
C = "01_toString"
T(C, "01_int", "", 'val n = 42\nval s = n.toString\nprint(s)', out="42")
T(C, "02_zaporny_int", "", 'val n = -7\nval s = n.toString\nprint(s)',
  out="-7")
T(C, "03_nula", "", 'val n = 0\nval s = n.toString\nprint(s)', out="0")
T(C, "04_string", "", 'val a = "abc"\nval s = a.toString\nprint(s)',
  out="abc")
T(C, "05_double_priklad_ze_zadani", "",
  'val d = 1.2345\nval s = d.toString\nprint(s)', out="1.23",
  note="Scala by dala 1.2345, IFJ26 používá formát toStr (%.2f)")
T(C, "06_double_mensi_nez_jedna", "",
  'val d = 0.5\nval s = d.toString\nprint(s)', out="0.50")
T(C, "07_double_zaporny", "",
  'val d = -3.756\nval s = d.toString\nprint(s)', out="-3.76")
T(C, "08_vyraz_v_zavorkach", "", '''
  val a = (1 + 2).toString
  val b = ("a" + "b").toString
  print(a)
  print(b)
  ''', out="3ab")
T(C, "09_literaly_jako_prijemce", "", '''
  val a = "abc".toString
  val b = (5).toString
  val c = (2.5).toString
  print(a)
  print(",")
  print(b)
  print(",")
  print(c)
  ''', out="abc,5,2.50")
T(C, "10_v_konkatenaci_ma_clen_prednost", "", '''
  val n = 42
  val s = "n=" + n.toString + "!"
  print(s)
  ''', out="n=42!")
T(C, "11_vysledek_je_string_lze_porovnat", "", '''
  val n = 42
  if n.toString == "42" then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "12_vice_cisel_za_sebou", "", '''
  val a = 1
  val b = 22
  val c = 333
  val s = a.toString + "," + b.toString + "," + c.toString
  print(s)
  ''', out="1,22,333")

# ===========================================================================
# 02 .length
# ===========================================================================
C = "02_length"
T(C, "01_retezcovy_literal", "", 'val n = "abc".length\nprint(n)', out="3")
T(C, "02_promenna", "", 'val s = "hello"\nval n = s.length\nprint(n)',
  out="5")
T(C, "03_prazdny_retezec", "", 'val n = "".length\nprint(n)', out="0")
T(C, "04_konkatenace_v_zavorkach", "",
  'val n = ("ab" + "cde").length\nprint(n)', out="5")
T(C, "05_v_aritmetice", "",
  'val s = "hello"\nval n = s.length * 2 + 1\nprint(n)', out="11")
T(C, "06_v_podmince_if", "", '''
  val s = "hello"
  if s.length > 3 then
    print("long")
  else
    print("short")
  ''', out="long")
T(C, "07_v_podmince_while", "", '''
  val s = "abcd"
  var i = 0
  while i < s.length do
    i = i + 1
  print(i)
  ''', out="4")
RAW(C, "08_escape_sekvence_jsou_jeden_znak", r'''
@main
def main(): Unit =
  val a = "a\nb".length
  val b = "\t".length
  val c = "\\".length
  val d = "\"".length
  print(a)
  print(",")
  print(b)
  print(",")
  print(c)
  print(",")
  print(d)
''', out="3,1,1,1")
RAW(C, "09_viceradkovy_literal_bez_escape_sekvenci", r'''
@main
def main(): Unit =
  val n = """a\nb""".length
  print(n)
''', out="4", note="ve víceřádkovém literálu se escape sekvence nevyhodnocují")
RAW(C, "10_viceradkovy_literal_pres_radky", '''
@main
def main(): Unit =
  val n = """ab
cd""".length
  print(n)
''', out="5")
T(C, "11_mezery_se_pocitaji", "", 'val n = "a b c".length\nprint(n)',
  out="5")
T(C, "12_po_prirazeni", "", '''
  var s = "a"
  s = s + "bc"
  val n = s.length
  print(n)
  ''', out="3")
T(C, "13_kontrola_prazdneho_retezce", "", '''
  val s = ""
  if s.length == 0 then
    print("empty")
  else
    print("full")
  ''', out="empty")

# ===========================================================================
# 03 .toInt
# ===========================================================================
C = "03_toInt"
T(C, "01_desetinna_cast_se_oseka", "",
  'val d = 3.99\nval n = d.toInt\nprint(n)', out="3")
T(C, "02_zaporne_cislo_se_oseka_k_nule", "",
  'val d = -3.99\nval n = d.toInt\nprint(n)', out="-3")
T(C, "03_literal_v_zavorkach", "", 'val n = (2.5).toInt\nprint(n)', out="2")
T(C, "04_cele_cislo_typu_double", "",
  'val d = 5.0\nval n = d.toInt\nprint(n)', out="5")
T(C, "05_v_aritmetice", "",
  'val d = 3.99\nval n = d.toInt * 2 + 1\nprint(n)', out="7")
T(C, "06_vyraz_jako_prijemce", "",
  'val d = 1.75\nval n = (d * 2.0).toInt\nprint(n)', out="3")
T(C, "07_toInt_toString", "",
  'val d = 3.99\nval s = d.toInt.toString\nprint(s)', out="3")
T(C, "08_zapis_s_exponentem", "",
  'val d = 1e3\nval n = d.toInt\nprint(n)', out="1000")
T(C, "09_mensi_nez_jedna", "",
  'val d = 0.9\nval n = d.toInt\nprint(n)', out="0")
T(C, "10_zaporne_mensi_nez_jedna", "",
  'val d = -0.9\nval n = d.toInt\nprint(n)', out="0")

# ===========================================================================
# 04 .floor
# ===========================================================================
C = "04_floor"
T(C, "01_kladne_cislo", "",
  'val d = 2.7\nval n = d.floor.toInt\nprint(n)', out="2")
T(C, "02_zaporne_cislo_zaokrouhluje_dolu", "",
  'val d = -2.5\nval n = d.floor.toInt\nprint(n)', out="-3")
T(C, "03_cele_cislo", "",
  'val d = 4.0\nval n = d.floor.toInt\nprint(n)', out="4")
T(C, "04_toInt_a_floor_se_lisi_u_zapornych", "", '''
  val d = -2.5
  val a = d.toInt
  val b = d.floor.toInt
  print(a)
  print(" ")
  print(b)
  ''', out="-2 -3")
T(C, "05_vysledek_je_double", "", '''
  val d = 2.7
  val f: Double = d.floor
  if f == 2.0 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "06_floor_a_aritmetika_zustava_double", "", '''
  val d = 2.7
  val g = d.floor + 0.5
  val s = g.toString
  print(s)
  ''', out="2.50")
T(C, "07_vyraz_jako_prijemce", "",
  'val d = 2.7\nval n = (d + 0.5).floor.toInt\nprint(n)', out="3")
T(C, "08_literal_v_zavorkach", "",
  'val n = (7.9).floor.toInt\nprint(n)', out="7")
T(C, "09_zaokrouhleni_pres_floor", "", '''
  val a = 2.4
  val b = 2.5
  val r1 = (a + 0.5).floor.toInt
  val r2 = (b + 0.5).floor.toInt
  print(r1)
  print(r2)
  ''', out="23")
T(C, "10_floor_dvakrat", "",
  'val d = 2.7\nval n = d.floor.floor.toInt\nprint(n)', out="2")
T(C, "11_floor_pak_porovnani", "", '''
  val d = 3.9
  if d.floor < d then
    print("y")
  else
    print("n")
  ''', out="y")

# ===========================================================================
# 05 řetězení členů
# ===========================================================================
C = "05_retezeni"
T(C, "01_floor_toInt_ze_zadani", "",
  'val number = 3.7\nval n = number.floor.toInt\nprint(n)', out="3")
T(C, "02_floor_toInt_toString", "",
  'val number = 3.7\nval s = number.floor.toInt.toString\nprint(s)', out="3")
T(C, "03_floor_toInt_toString_length", "", '''
  val number = 123.9
  val n = number.floor.toInt.toString.length
  print(n)
  ''', out="3")
T(C, "04_int_toString_length", "",
  'val n = 1000\nval l = n.toString.length\nprint(l)', out="4")
T(C, "05_double_toString_length", "",
  'val d = 1.2345\nval l = d.toString.length\nprint(l)', out="4")
T(C, "06_length_toString", "",
  'val s = "hello"\nval t = s.length.toString\nprint(t)', out="5")
T(C, "07_length_toString_length", "",
  'val s = "hello"\nval l = s.length.toString.length\nprint(l)', out="1")
T(C, "08_dlouhy_retezec_clenu", "", '''
  val x = 2.5
  val l = (x * 100.0).floor.toInt.toString.length
  print(l)
  ''', out="3")
T(C, "09_toString_na_retezci_a_pak_length", "",
  'val n = 42\nval l = n.toString.toString.length\nprint(l)', out="2")
T(C, "10_zaporne_cislo_v_toString_length", "",
  'val n = -100\nval l = n.toString.length\nprint(l)', out="4",
  note="zaporné znaménko se počítá do délky řetězce")
T(C, "11_toString_floor_toInt_chain_na_literalu", "",
  'val l = (12.5).floor.toInt.toString.length\nprint(l)', out="2")

# ===========================================================================
# 06 členy ve výrazech a příkazech
# ===========================================================================
C = "06_ve_vyrazech"
T(C, "01_v_prikazu_return", '''
def len(s: String): Int =
  return s.length
''', 'val n = len("abcd")\nprint(n)', out="4")
T(C, "02_v_jednoradkovem_tele", "def len2(s: String): Int = s.length",
  'val n = len2("abcd")\nprint(n)', out="4")
T(C, "03_double_parametr", "def trunc(d: Double): Int = d.toInt",
  'val n = trunc(9.99)\nprint(n)', out="9")
T(C, "04_clen_ma_prednost_pred_nasobenim", "",
  'val s = "abc"\nval n = 1 + s.length * 2\nprint(n)', out="7")
T(C, "05_porovnani_dvou_delek", "", '''
  val a = "ab"
  val b = "abc"
  if a.length < b.length then
    print("lt")
  else
    print("ge")
  ''', out="lt")
T(C, "06_toString_v_podmince_s_delkou", "", '''
  val n = 42
  if n.toString.length == 2 then
    print("two")
  else
    print("other")
  ''', out="two")
T(C, "07_odvozeny_typ_promennych", "", '''
  val s = "abc"
  val l = s.length
  val t = l + 1
  val d = 2.7
  val f = d.floor
  val g = f + 1.0
  print(t)
  if g == 3.0 then
    print("y")
  else
    print("n")
  ''', out="4y")
T(C, "08_zaporny_prijemce", "",
  'val d = 2.5\nval n = (-d).toInt\nprint(n)', out="-2")
T(C, "09_vysledek_do_promenne_s_typem", "", '''
  val s = "abc"
  val n: Int = s.length
  val t: String = n.toString
  val d: Double = (2.7).floor
  val m = d.toInt
  print(n)
  print(t)
  print(m)
  ''', out="332")
T(C, "10_prirazeni_do_var", "", '''
  val s = "abc"
  var n = 0
  n = s.length
  print(n)
  ''', out="3")
T(C, "11_v_podmince_s_aritmetikou", "", '''
  val s = "abc"
  if s.length * 2 == 6 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "12_clen_na_parametru_funkce", '''
def cnt(s: String, d: Double): Int =
  val a = s.length
  val b = d.toInt
  a + b
''', 'val r = cnt("abcd", 2.9)\nprint(r)', out="6")
T(C, "13_clen_na_navratove_hodnote_ulozene_do_promenne", '''
def twice(x: Int): Int = x * 2
''', 'val r = twice(21)\nval s = r.toString\nprint(s)', out="42")
T(C, "14_v_cyklu_while", "", '''
  val s = "xyz"
  var i = 0
  var total = 0
  while i < 3 do
    total = total + s.length
    i = i + 1
  print(total)
  ''', out="9")
T(C, "15_porovnani_toString_dvou_cisel", "", '''
  val a = 12
  val b = 12
  if a.toString == b.toString then
    print("same")
  else
    print("diff")
  ''', out="same")

# ===========================================================================
# 07 členy a vstup
# ===========================================================================
C = "07_vstup"
T(C, "01_readLine_obsahuje_konec_radku", "",
  'val l = readLine()\nval n = l.length\nprint(n)', inp="abc\n", out="4",
  note="readLine vrací řádek včetně LF, tedy abc + LF = 4 znaky")
T(C, "02_readDouble_a_toInt", "",
  'val d = readDouble()\nval n = d.toInt\nprint(n)', inp="7.9\n", out="7")
T(C, "03_readInt_toString_length", "",
  'val n = readInt()\nval l = n.toString.length\nprint(l)', inp="12345\n",
  out="5")
T(C, "04_chybny_vstup_je_nula", "",
  'val n = readInt()\nval l = n.toString.length\nprint(l)', inp="abc\n",
  out="1", note="readInt vrátí 0 při chybném formátu, '0' má délku 1")
T(C, "05_prazdny_vstup_readLine", "",
  'val l = readLine()\nval n = l.length\nprint(n)', inp="", out="0")

# ===========================================================================
# 08 chyby
# ===========================================================================
C = "08_chyby"
WRONG = (3, 6)
T(C, "01_length_na_int", "", 'val n = 5\nval l = n.length\nprint(l)',
  crc=WRONG, note="length je jen pro String; 6 (typ) nebo 3 (člen neexistuje)")
T(C, "02_length_na_double", "", 'val d = 1.5\nval l = d.length\nprint(l)',
  crc=WRONG)
T(C, "03_toInt_na_int", "", 'val n = 5\nval m = n.toInt\nprint(m)',
  crc=WRONG, note="ve Scale to platí, v IFJ26 je toInt jen pro Double")
T(C, "04_toInt_na_string", "", 'val s = "5"\nval m = s.toInt\nprint(m)',
  crc=WRONG)
T(C, "05_floor_na_int", "", 'val n = 5\nval m = n.floor\nprint("x")',
  crc=WRONG)
T(C, "06_floor_na_string", "", 'val s = "a"\nval m = s.floor\nprint("x")',
  crc=WRONG)
UNK = (2, 3, 6)
T(C, "07_neznamy_clen_size", "", 'val s = "abc"\nval n = s.size\nprint(n)',
  crc=UNK, note="jiné členy nejsou podporovány; kód není v zadání jednoznačný")
T(C, "08_neznamy_clen_toUpperCase", "",
  'val s = "abc"\nval t = s.toUpperCase\nprint("x")', crc=UNK)
T(C, "09_neznamy_clen_round", "",
  'val d = 1.5\nval n = d.round\nprint("x")', crc=UNK)
T(C, "10_neznamy_clen_abs", "", 'val n = 5\nval m = n.abs\nprint("x")',
  crc=UNK)
T(C, "11_length_s_prazdnymi_zavorkami", "",
  'val n = "abc".length()\nprint(n)', crc=(2, 5),
  note="členy se zapisují bez prázdného seznamu argumentů")
T(C, "12_toString_s_prazdnymi_zavorkami", "",
  'val n = 5\nval s = n.toString()\nprint(s)', crc=(2, 5))
T(C, "13_toInt_s_prazdnymi_zavorkami", "",
  'val d = 5.5\nval n = d.toInt()\nprint(n)', crc=(2, 5))
T(C, "14_prirazeni_do_clenu", "",
  'val s = "abc"\ns.length = 3\nprint("x")', crc=(2, 7))
T(C, "15_chybi_identifikator_za_teckou", "",
  'val s = "abc"\nval n = s.\nprint("x")', crc=2)
T(C, "16_tecka_bez_prijemce", "", 'val n = .length\nprint("x")', crc=2)
T(C, "17_length_do_stringu", "",
  'val s = "abc"\nval x: String = s.length\nprint("x")', crc=6)
T(C, "18_floor_do_int", "",
  'val d = 1.5\nval x: Int = d.floor\nprint("x")', crc=6)
T(C, "19_length_plus_string", "",
  'val s = "abc"\nval x = s.length + "a"\nprint("x")', crc=6)
T(C, "20_toString_do_int", "",
  'val n = 5\nval x: Int = n.toString\nprint("x")', crc=6)
T(C, "21_length_na_vysledku_length", "",
  'val s = "abc"\nval n = s.length.length\nprint("x")', crc=WRONG)
T(C, "22_toInt_na_vysledku_toInt", "",
  'val d = 1.5\nval n = d.toInt.toInt\nprint("x")', crc=WRONG)
T(C, "23_floor_na_vysledku_toInt", "",
  'val d = 1.5\nval n = d.toInt.floor\nprint("x")', crc=WRONG)
T(C, "24_clen_na_nedefinovane_promenne", "",
  'val n = nope.length\nprint("x")', crc=3)
T(C, "25_toString_vysledek_nelze_pricist_k_int", "",
  'val n = 5\nval x = n.toString + 1\nprint("x")', crc=6)

# ===========================================================================
# 99 kombinace s dalšími rozšířeními
# ===========================================================================
C = "99_kombinace_funexp"
T(C, "01_clen_na_vysledku_readInt", "",
  'val r = readInt().toString.length\nprint(r)', inp="1234\n", out="4")
T(C, "02_clen_na_vysledku_uzivatelske_funkce", "def dbl(x: Int): Int = x * 2",
  'print(dbl(21).toString)', out="42")
T(C, "03_clen_jako_argument_uzivatelske_funkce",
  "def f(x: Int): Int = x * 2",
  'print(toStr(f("abc".length)))', out="6")
T(C, "04_length_a_strLength_se_shoduji", "", '''
  if "abc".length == strLength("abc") then
    print("same")
  else
    print("diff")
  ''', out="same")
T(C, "05_clen_na_vysledku_subStr", "",
  'print(toStr(subStr("abcdef", 1, 4).length))', out="3")
T(C, "06_toInt_vestavene_a_clen", "",
  'print(toStr(toInt(2.9) + (2.9).toInt))', out="4")
T(C, "07_clen_v_argumentu_vestavene_funkce", "",
  'print(chr(65 + "ab".length))', out="C")
T(C, "08_clen_primo_v_print", "", 'print("abcd".length)', out="4")
T(C, "09_vnorene_clenove_volani", "def f(x: Int): Int = x + 1",
  'print(f(f("ab".length).toString.length))', out="2")

C = "99_kombinace_boolthen"
T(C, "01_toString_booleanu", "",
  'val b = true\nval s = b.toString\nprint(s)', out="true")
T(C, "02_toString_porovnani", "",
  'val s = (1 < 2).toString\nprint(s)', out="true")
T(C, "03_toString_negace", "",
  'val b = false\nval s = (!b).toString\nprint(s)', out="true")
T(C, "04_clen_v_booleanovskem_vyrazu", "", '''
  val s = "abc"
  val d = 3.5
  if s.length > 2 && d.toInt == 3 then
    print("y")
  else
    print("n")
  ''', out="y")
T(C, "05_delka_toString_booleanu", "",
  'val b = false\nval n = b.toString.length\nprint(n)', out="5")
T(C, "06_porovnani_s_clenem_ulozene_do_boolean", "",
  'val s = "abc"\nval big = s.length > 2\nprint(big)', out="true")

C = "99_kombinace_control"
T(C, "01_clen_v_mezi_cyklu_for", "",
  'val s = "abc"\nfor i <- 1 to s.length do print(i)', out="123")
T(C, "02_if_vyraz_s_clenem", "", '''
  val s = "abc"
  val m = if s.length > 2 then s.length else 0
  print(m)
  ''', out="3")
T(C, "03_toString_ve_strazi", "",
  'for i <- 1 until 15 if i.toString.length == 1 do print(i)',
  out="123456789")
T(C, "04_floor_v_mezi", "",
  'val d = 3.7\nfor i <- 1 to d.floor.toInt do print(i)', out="123")
T(C, "05_if_vyraz_vraci_toString", "", '''
  val n = 10
  val t = if n > 5 then n.toString else "small"
  print(t)
  ''', out="10")

C = "99_kombinace_interpolateds"
T(C, "01_length_v_interpolaci", "",
  'val s = "abc"\nval m = s"len=${s.length}"\nprint(m)', out="len=3")
T(C, "02_toInt_a_floor_v_interpolaci", "",
  'val d = 2.7\nval m = s"${d.toInt}-${d.floor.toInt}"\nprint(m)',
  out="2-2")
T(C, "03_retezeni_v_interpolaci", "",
  'val d = 123.9\nval m = s"${d.floor.toInt.toString.length}"\nprint(m)',
  out="3")
T(C, "04_toString_se_rovna_interpolaci", "", '''
  val d = 1.5
  val a = d.toString
  val b = s"$d"
  if a == b then
    print("same")
  else
    print("diff")
  ''', out="same")

C = "99_kombinace_extkw"
T(C, "01_clen_na_vysledku_rozsirujici_metody", '''
extension (v: Int)
  def twice: Int = v * 2
''', 'val n = 6\nval s = n.twice.toString\nprint(s)', out="12")
T(C, "02_rozsirujici_metoda_na_clenu_literalu", '''
extension (s: String)
  def shout: String = s + "!"
''', 'val n = "abc".shout.length\nprint(n)', out="4")
T(C, "03_rozsirujici_metoda_pouziva_clen", '''
extension (s: String)
  def doubled: Int = s.length * 2
''', 'val n = "abcd".doubled\nprint(n)', out="8")


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
