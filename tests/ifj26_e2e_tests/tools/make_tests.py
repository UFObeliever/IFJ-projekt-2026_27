#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Vytvoří testy pro generátor kódu (tests/generator) a doplní očekávané výstupy
k platným programům v tests/compiler.

Spuštění:  python3 tools/make_tests.py
  * tests/generator se SMAŽE a vytvoří znovu
  * v tests/compiler se jen přepíšou soubory *.out (ke každému platnému programu)

Očekávaný výstup každého testu počítá referenční interpret (tools/ref_interp.py).
U testů s argumentem out=... je navíc zadána ručně spočítaná hodnota a skript
spadne, pokud se s interpretem neshoduje – hodnoty se tak kontrolují dvěma
nezávislými způsoby.

Název testu:  NN_nazev.rc0[.run<kód>].scala  (+ NN_nazev.in, NN_nazev.out)
  rc0     překladač musí skončit s kódem 0
  run<N>  ic26int musí skončit s kódem N (výchozí 0)
"""
import glob
import os
import re
import shutil
import sys
import textwrap

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from ref_interp import run_program  # noqa: E402

GEN_DIR = os.path.join(ROOT, "tests", "generator")
CMP_DIR = os.path.join(ROOT, "tests", "compiler")

G = []


# ----------------------------------------------------------------------------- pomocné
def dd(s):
    s = textwrap.dedent(s).lstrip("\n")
    return s if s.endswith("\n") else s + "\n"


PRO = "import ifj26.*\n\n"


def main(body, name="main"):
    """Prolog + @main, jehož tělem jsou zadané příkazy."""
    return PRO + "@main def %s(): Unit =\n" % name + textwrap.indent(dd(body), "  ")


def prog(funcs, body, name="main"):
    """Prolog + funkce + @main s tělem."""
    return PRO + dd(funcs) + "\n@main def %s(): Unit =\n" % name + textwrap.indent(dd(body), "  ")


NL = 'print("\\n")\n'


def show(*names):
    """Vypíše každou proměnnou a za ní nový řádek (argument print musí být term)."""
    return "".join("print(%s)\n%s" % (n, NL) for n in names)


def gen(cat, name, src, stdin="", out=None, run=0):
    G.append((cat, name, src, stdin, out, run))


def lit(v):
    if isinstance(v, str):
        return '"%s"' % v
    return repr(v)


OPS = ["<", "<=", ">", ">=", "==", "!="]


def pyop(op, a, b):
    return {"<": a < b, "<=": a <= b, ">": a > b, ">=": a >= b, "==": a == b, "!=": a != b}[op]


def cmp_test(cat, name, tx, ty, pairs, ops=OPS):
    """Pro každou dvojici a každý operátor vypíše 1/0 podle if/else; očekávání počítá Python."""
    body = "var x = %s\nvar y = %s\n" % (lit(pairs[0][0]), lit(pairs[0][1]))
    exp = ""
    for a, b in pairs:
        body += "x = %s\ny = %s\n" % (lit(a), lit(b))
        for op in ops:
            body += 'if x %s y then\n  print("1")\nelse\n  print("0")\n' % op
            exp += "1" if pyop(op, a, b) else "0"
        body += NL
        exp += "\n"
    gen(cat, name, main(body), out=exp)


# ============================================================================= 01 aritmetika Int
c = "01_aritmetika_int"
gen(c, "scitani_a_odcitani", main("val a = 7\nval b = 3\nval s = a + b\nval d = a - b\n" + show("s", "d")),
    out="10\n4\n")
gen(c, "nasobeni", main("val a = 6\nval b = 7\nval p = a * b\nval z = 0 * b\nval n = a * -3\n" + show("p", "z", "n")),
    out="42\n0\n-18\n")
gen(c, "priorita_operatoru", main("""\
val a = 1 + 2 * 3
val b = (1 + 2) * 3
val c = 10 - 4 - 3
val d = 100 / 10 / 5
val e = 2 * 3 + 4 * 5 - 6 / 2
val f = 20 - 2 * 3 * 2
val g = (2 + 3) * (4 - 1)
""" + show("a", "b", "c", "d", "e", "f", "g")), out="7\n9\n3\n2\n23\n8\n15\n")
gen(c, "unarni_minus", main("""\
val a = 4
val b = 5
val c = -5
val d = - -5
val e = -(-5)
val f = 3 - -2
val g = -a * -b
val h = -(a + b)
val i = -2 * 3
val j = 10 - -a
""" + show("c", "d", "e", "f", "g", "h", "i", "j")), out="-5\n5\n5\n5\n20\n-9\n-6\n14\n")
gen(c, "deleni_kladnych_cisel", main("""\
val a = 7
val b = 2
val q1 = a / b
val q2 = 8 / b
val q3 = 1 / b
val q4 = 0 / a
val q5 = 100 / a
""" + show("q1", "q2", "q3", "q4", "q5")), out="3\n4\n0\n0\n14\n")
# Past: IDIV v IFJcode26 zaokrouhluje k -nekonečnu, IFJ26 má zaokrouhlovat k nule.
gen(c, "deleni_zapornych_cisel_zaokrouhluje_k_nule", main("""\
val p7 = 7
val m7 = -7
val p2 = 2
val m2 = -2
val q1 = m7 / p2
val q2 = p7 / m2
val q3 = m7 / m2
val q4 = -1 / p2
val q5 = -8 / 3
val q6 = 8 / -3
""" + show("q1", "q2", "q3", "q4", "q5", "q6")), out="-3\n-3\n3\n0\n-2\n-2\n")
gen(c, "zbytek_pres_odcitani_a_deleni", main("""\
val a = 7
val b = 3
val r1 = a - a / b * b
val m = -7
val r2 = m - m / b * b
val n = -3
val r3 = a - a / n * n
val r4 = 10 - 10 / 5 * 5
""" + show("r1", "r2", "r3", "r4")), out="1\n-1\n1\n0\n")
gen(c, "velke_cisla_v_rozsahu_int", main("""\
val a = 46340
val r = a * a
val m = 2147483647
val h = 1_000_000 * 2000
""" + show("r", "m", "h")), out="2147395600\n2147483647\n2000000000\n")
gen(c, "cisla_s_podtrzitky", main("val m = 1_000_000 * 2\nval n = 1_2_3\n" + show("m", "n")), out="2000000\n123\n")
gen(c, "vnorene_zavorky", main("val r = ((1 + 2) * (3 + 4)) - ((5 - 6) * 7)\nval q = (((((10)))))\n" + show("r", "q")),
    out="28\n10\n")
gen(c, "nula_a_zaporne_vysledky", main("val a = 5 - 5\nval b = 3 - 10\nval z = -0\nval y = 0 - 0\n" + show("a", "b", "z", "y")),
    out="0\n-7\n0\n0\n")
gen(c, "postupna_aktualizace_promenne", main("""\
var x = 10
x = x + 5
""" + show("x") + """\
x = x * 2
""" + show("x") + """\
x = x - 3
""" + show("x") + """\
x = x / 4
""" + show("x")), out="15\n30\n27\n6\n")
gen(c, "retezeni_operatoru_stejne_priority", main("""\
val a = 100 - 10 - 20 - 30
val b = 2 * 3 * 4 / 6
val c = 100 / 5 / 2 / 2
val d = 1 + 2 - 3 + 4 - 5
""" + show("a", "b", "c", "d")), out="40\n4\n5\n-1\n")

# ============================================================================= 02 aritmetika Double
c = "02_aritmetika_double"
gen(c, "vypis_double_hexadecimalne", main("""\
val a = 1.5
val b = 0.5
val c = 2.0
val d = 0.1
val e = 100.0
val f = -1.5
val g = 3.14
val h = 0.0
""" + show("a", "b", "c", "d", "e", "f", "g", "h")),
    out="0x1.8p+0\n0x1p-1\n0x1p+1\n0x1.999999999999ap-4\n0x1.9p+6\n-0x1.8p+0\n0x1.91eb851eb851fp+1\n0x0p+0\n")
gen(c, "aritmetika_double", main("""\
val a = 1.5
val b = 2.25
val s = a + b
val p = 2.5 * 4.0
val q = 7.5 / 2.5
val d = 1.0 - 0.75
""" + show("s", "p", "q", "d")), out="0x1.ep+1\n0x1.4p+3\n0x1.8p+1\n0x1p-2\n")
gen(c, "rozsireni_int_na_double", main("""\
val d: Double = 3
val e = 1 + 2.5
val f = 5 / 2.0
val g = 2 * 1.5
val h: Double = 7 / 2
var v: Double = 1.5
v = 2
""" + show("d", "e", "f", "g", "h", "v")),
    out="0x1.8p+1\n0x1.cp+1\n0x1.4p+1\n0x1.8p+1\n0x1.8p+1\n0x1p+1\n")
gen(c, "deleni_double", main("""\
val a = 1.0 / 3.0
val b = 2.0 / 3.0
val c = 10.0 / 4
val d = -7.0 / 2.0
""" + show("a", "b", "c", "d")),
    out="0x1.5555555555555p-2\n0x1.5555555555555p-1\n0x1.4p+1\n-0x1.cp+1\n")
gen(c, "tvary_double_literalu", main("""\
val a = 1e3
val b = 2.5e-1
val c = 1.5E2
val d = 1_000.5
val e = 1.0d
val f = .5
val g = 1e2d
val h = 5e-1
val i = 12.375
""" + show("a", "b", "c", "d", "e", "f", "g", "h", "i")),
    out="0x1.f4p+9\n0x1p-2\n0x1.2cp+7\n0x1.f44p+9\n0x1p+0\n0x1p-1\n0x1.9p+6\n0x1p-1\n0x1.8cp+3\n")
gen(c, "toInt_orezava_desetinnou_cast", main("""\
val a = 2.9
val n = -2.9
val b = 0.5
val c = 100.0
val i1 = toInt(a)
val i2 = toInt(n)
val i3 = toInt(b)
val i4 = toInt(c)
val i5 = toInt(7.0)
""" + show("i1", "i2", "i3", "i4", "i5")), out="2\n-2\n0\n100\n7\n")
gen(c, "toStr_double_dve_desetinna_mista", main("""\
val a = 3.14159
val b = 2.5
val c = -3.7
val d = 0.5
val e = 123.456
val s1 = toStr(a)
val s2 = toStr(b)
val s3 = toStr(c)
val s4 = toStr(d)
val s5 = toStr(e)
""" + show("s1", "s2", "s3", "s4", "s5")), out="3.14\n2.50\n-3.70\n0.50\n123.46\n")
gen(c, "unarni_minus_double", main("""\
val x = 2.5
val y = -x
val z = -(x * 2.0)
""" + show("y", "z")), out="-0x1.4p+1\n-0x1.4p+2\n")
gen(c, "soucet_deseti_polovin", main("""\
var s = 0.0
var i = 0
while i < 10 do
  s = s + 0.5
  i = i + 1
""" + show("s")), out="0x1.4p+2\n")
gen(c, "mix_int_double_a_priorita", main("""\
val a = 1 + 2 * 0.5
val b = (1 + 2) * 0.5
val c = 7 / 2 * 1.0
val d = 7 / 2.0 * 2
""" + show("a", "b", "c", "d")), out="0x1p+1\n0x1.8p+0\n0x1.8p+1\n0x1.cp+2\n")
gen(c, "velka_a_mala_cisla", main("val a = 1e10\nval b = 1e-5\nval c = 123456.789\n" + show("a", "b", "c")))
gen(c, "double_promenna_prirazeni_hodnoty_int", main("""\
var d: Double = 1.5
val i = 4
d = i
""" + show("d")), out="0x1p+2\n")

# ============================================================================= 03 řetězce
c = "03_retezce"
gen(c, "konkatenace", main("""\
val a = "ab"
val b = "cd"
val c = a + b
val d = c + "!" + a
val e = "" + a
val f = a + ""
""" + show("c", "d", "e", "f")), out="abcd\nabcd!ab\nab\nab\n")
gen(c, "escape_sekvence_ve_vypisu", main(r'''
print("a\tb\n")
print("back\\slash\n")
print("quote\"q\n")
'''), out='a\tb\nback\\slash\nquote"q\n')
gen(c, "viceradkovy_retezec_ze_zadani",
    PRO + '@main def main(): Unit =\n  val x = """A\nhoj s\\nvete\n"""\n  print(x)\n',
    out="A\nhoj s\\nvete\n")
gen(c, "viceradkovy_retezec_s_uvozovkami", PRO + '@main def main(): Unit =\n  val x = """say "hi" now"""\n  print(x)\n  print("\\n")\n',
    out='say "hi" now\n')
gen(c, "delka_retezce", main("""\
val a = strLength("hello")
val b = strLength("")
val c = strLength("a\\nb")
val s = "ab" + "cde"
val d = strLength(s)
""" + show("a", "b", "c", "d")), out="5\n0\n3\n5\n")
gen(c, "podretezec_subStr", main("""\
val h = "hello"
val a = subStr(h, 1, 3)
val b = subStr(h, 0, 5)
val c = subStr(h, 4, 5)
val d = subStr(h, 2, 2)
val e = subStr(h, 0, 1)
""" + show("a", "b", "c", "d", "e")), out="el\nhello\no\n\nh\n")
gen(c, "ord_a_chr", main("""\
val a = ord("A", 0)
val b = ord("abc", 2)
val c = ord("z", 0)
val d = chr(65)
val e = chr(97)
val f = chr(126)
val g = chr(32)
""" + show("a", "b", "c", "d", "e", "f", "g")), out="65\n99\n122\nA\na\n~\n \n")
gen(c, "ord_mimo_rozsah_vraci_minus_jedna", main("""\
val neg = -1
val a = ord("abc", 3)
val b = ord("abc", neg)
val c = ord("", 0)
val d = ord("a", 1)
val e = ord("abc", 100)
""" + show("a", "b", "c", "d", "e")), out="-1\n-1\n-1\n-1\n-1\n")
gen(c, "abeceda_pomoci_chr", main("""\
var s = ""
var i = 0
while i < 26 do
  val code = 65 + i
  val ch = chr(code)
  s = s + ch
  i = i + 1
""" + show("s")), out="ABCDEFGHIJKLMNOPQRSTUVWXYZ\n")
gen(c, "zmena_retezcove_promenne", main('var s = "a"\ns = s + "b"\ns = s + s\n' + show("s")), out="abab\n")
gen(c, "toStr_a_konkatenace", main("""\
val n = 42
val s = toStr(n)
val t = "n=" + s
val m = -7
val u = toStr(m)
val v = toStr("x")
""" + show("t", "u", "v")), out="n=42\n-7\nx\n")
gen(c, "vypis_unit", main("val u = ()\nprint(u)\n" + NL + "print(())\n" + NL + "val t = toStr(u)\n" + show("t")),
    out="()\n()\n()\n")
gen(c, "specialni_znaky_v_retezci", main(r'''print("#$%&'()*+,-./:;<=>?@[]^_`{|}~")''' + "\n" + NL),
    out="#$%&'()*+,-./:;<=>?@[]^_`{|}~\n")
gen(c, "prazdny_retezec_a_okraje", main('print("")\nprint("[")\nprint("  x  ")\nprint("]")\n' + NL), out="[  x  ]\n")
gen(c, "dlouhy_retezec", main('val s = "' + "abc" * 100 + '"\nval n = strLength(s)\n' + show("n", "s")),
    out="300\n" + "abc" * 100 + "\n")
gen(c, "opakovani_retezce_v_cyklu", main("""\
var s = ""
var i = 0
while i < 5 do
  s = s + "ab"
  i = i + 1
val n = strLength(s)
""" + show("s", "n")), out="ababababab\n10\n")
gen(c, "cislice_pomoci_toStr_v_cyklu", main("""\
var s = ""
var i = 1
while i <= 5 do
  val d = toStr(i)
  s = s + d
  i = i + 1
""" + show("s")), out="12345\n")

# ============================================================================= 04 porovnání
c = "04_porovnani"
cmp_test(c, "cela_cisla_vsechny_operatory", "Int", "Int", [(1, 2), (2, 2), (3, 2)])
cmp_test(c, "zaporna_cisla", "Int", "Int", [(-1, 0), (0, -1), (-5, -5), (-3, -7)])
cmp_test(c, "double_vsechny_operatory", "Double", "Double", [(1.5, 2.5), (2.5, 2.5), (3.5, 2.5)])
# Int proti Double: x je Int, y je Double
body = "var x = 1\nvar y = 2.5\n"
exp = ""
for a, b in [(1, 2.5), (2, 2.0), (3, 2.5)]:
    body += "x = %d\ny = %r\n" % (a, b)
    for op in OPS:
        body += 'if x %s y then\n  print("1")\nelse\n  print("0")\n' % op
        exp += "1" if pyop(op, a, b) else "0"
    body += NL
    exp += "\n"
gen(c, "int_proti_double", main(body), out=exp)
cmp_test(c, "retezce_lexikograficky", "String", "String",
         [("abc", "abd"), ("abc", "abc"), ("b", "abc"), ("abc", "abcd"), ("", "a"), ("Z", "a"), ("10", "9"), ("a", "B")])
gen(c, "plovouci_presnost_0_1_plus_0_2", main("""\
val a = 0.1
val b = 0.2
val c = a + b
val d = 0.3
if c == d then
  print("eq")
else
  print("ne")
""" + NL + """\
if c > d then
  print("gt")
else
  print("le")
""" + NL), out="ne\ngt\n")
gen(c, "porovnani_aritmetickych_vyrazu", main("""\
val a = 5
val b = 10
if a + 1 <= b - 3 then
  print("1")
else
  print("0")
if a * 2 > b then
  print("1")
else
  print("0")
if a * 2 >= b then
  print("1")
else
  print("0")
if a + b == 15 then
  print("1")
else
  print("0")
if b / a != 2 then
  print("1")
else
  print("0")
""" + NL), out="1011" + "0" + "\n")
gen(c, "podminka_v_zavorkach", main("""\
val a = 1
val b = 2
if (a < b) then
  print("yes")
else
  print("no")
""" + NL), out="yes\n")
gen(c, "while_s_ruznymi_operatory", main("""\
var i = 0
while i != 3 do
  i = i + 1
""" + show("i") + """\
while i < 6 do
  i = i + 2
""" + show("i") + """\
while i >= 0 do
  i = i - 3
""" + show("i") + """\
while i <= 5 do
  i = i + 4
""" + show("i")), out="3\n7\n-2\n6\n")
gen(c, "vysledek_porovnani_neovlivnuje_promenne", main("""\
var a = 1
if a == 1 then
  a = 10
else
  a = 20
""" + show("a") + """\
if a == 1 then
  a = 10
else
  a = 20
""" + show("a")), out="10\n20\n")

# ============================================================================= 05 řízení toku
c = "05_rizeni_toku"
gen(c, "if_else_obe_vetve", main("""\
var x = 3
if x < 5 then
  print("small")
else
  print("big")
x = 10
if x < 5 then
  print("small")
else
  print("big")
""" + NL), out="smallbig\n")
cls = ""
for v in (-5, 0, 7):
    cls += ("v = %d\n" % v) + """\
if v < 0 then
  print("neg\\n")
else
  if v == 0 then
    print("zero\\n")
  else
    print("pos\\n")
"""
gen(c, "vnorene_if_else_klasifikace", main("var v = 0\n" + cls), out="neg\nzero\npos\n")
gen(c, "while_cisla_1_az_10", main("""\
var i = 1
while i <= 10 do
  print(i)
  print(" ")
  i = i + 1
""" + NL), out="1 2 3 4 5 6 7 8 9 10 \n")
gen(c, "soucet_1_az_100", main("""\
var i = 1
var s = 0
while i <= 100 do
  s = s + i
  i = i + 1
""" + show("s")), out="5050\n")
gen(c, "soucet_1_az_10000", main("""\
var i = 1
var s = 0
while i <= 10000 do
  s = s + i
  i = i + 1
""" + show("s")), out="50005000\n")
gen(c, "vnorene_while_nasobilka_3x3", main("""\
var i = 1
while i <= 3 do
  var j = 1
  while j <= 3 do
    val p = i * j
    print(p)
    print(" ")
    j = j + 1
  print("\\n")
  i = i + 1
"""), out="1 2 3 \n2 4 6 \n3 6 9 \n")
gen(c, "while_nikdy_neprobehne", main("""\
var i = 10
while i < 5 do
  print("never")
  i = i + 1
print("done")
""" + NL), out="done\n")
gen(c, "pocet_a_soucet_sudych", main("""\
var i = 1
var cnt = 0
var sum = 0
while i <= 20 do
  val r = i - i / 2 * 2
  if r == 0 then
    cnt = cnt + 1
    sum = sum + i
  else
    cnt = cnt
  i = i + 1
""" + show("cnt", "sum")), out="10\n110\n")
fb = ""
for i in range(1, 16):
    fb += ("FizzBuzz\n" if i % 15 == 0 else "Fizz\n" if i % 3 == 0 else "Buzz\n" if i % 5 == 0 else "%d\n" % i)
gen(c, "fizzbuzz_1_az_15", main("""\
var i = 1
while i <= 15 do
  val m3 = i - i / 3 * 3
  val m5 = i - i / 5 * 5
  if m3 == 0 then
    if m5 == 0 then
      print("FizzBuzz")
    else
      print("Fizz")
  else
    if m5 == 0 then
      print("Buzz")
    else
      print(i)
  print("\\n")
  i = i + 1
"""), out=fb)
gen(c, "collatz_27_ma_111_kroku", main("""\
var n = 27
var steps = 0
var peak = 27
while n != 1 do
  val r = n - n / 2 * 2
  if r == 0 then
    n = n / 2
  else
    n = 3 * n + 1
  if n > peak then
    peak = n
  else
    peak = peak
  steps = steps + 1
""" + show("steps", "peak")), out="111\n9232\n")
gen(c, "odpocet", main("""\
var i = 5
while i > 0 do
  print(i)
  print(" ")
  i = i - 1
print("Go!")
""" + NL), out="5 4 3 2 1 Go!\n")
gen(c, "soucty_lichych_a_sudych", main("""\
var i = 1
var odd = 0
var even = 0
while i <= 10 do
  val r = i - i / 2 * 2
  if r == 1 then
    odd = odd + i
  else
    even = even + i
  i = i + 1
""" + show("odd", "even")), out="25\n30\n")
gen(c, "trojuhelnik_z_hvezdicek", main("""\
var i = 1
while i <= 4 do
  var j = 0
  while j < i do
    print("*")
    j = j + 1
  print("\\n")
  i = i + 1
"""), out="*\n**\n***\n****\n")
gen(c, "znamky_vnoreny_else_if", main("""\
var score = 0
var k = 0
while k < 4 do
  if k == 0 then
    score = 95
  else
    if k == 1 then
      score = 85
    else
      if k == 2 then
        score = 75
      else
        score = 50
  if score >= 90 then
    print("A")
  else
    if score >= 80 then
      print("B")
    else
      if score >= 70 then
        print("C")
      else
        print("F")
  k = k + 1
""" + NL), out="ABCF\n")
gen(c, "faktorial_10_iterativne", main("""\
var i = 1
var f = 1
while i <= 10 do
  f = f * i
  i = i + 1
""" + show("f")), out="3628800\n")
gen(c, "dve_promenne_se_potkaji", main("""\
var i = 0
var j = 10
while i < j do
  i = i + 1
  j = j - 1
""" + show("i", "j")), out="5\n5\n")

# ============================================================================= 06 proměnné a rozsahy
c = "06_promenne_rozsahy"
gen(c, "zasteneni_v_podbloku", main("""\
val x = 1
if x < 2 then
  val x = "inner"
  print(x)
  print("\\n")
else
  print("no\\n")
""" + show("x")), out="inner\n1\n")
gen(c, "zasteneni_tremi_typy", main("""\
val x = 1
if x < 2 then
  val x = 2.5
  if x > 1.0 then
    val x = "deep"
    print(x)
    print("\\n")
  else
    print("no\\n")
  print(x)
  print("\\n")
else
  print("no\\n")
""" + show("x")), out="deep\n0x1.4p+1\n1\n")
gen(c, "stejne_jmeno_ruzne_typy_ve_vetvich", main("""\
var k = 0
while k < 2 do
  if k == 0 then
    val t = 5
    print(t)
    print("\\n")
  else
    val t = "str"
    print(t)
    print("\\n")
  k = k + 1
"""), out="5\nstr\n")
gen(c, "promenna_v_tele_cyklu_se_vytvari_znovu", main("""\
var i = 0
while i < 5 do
  val sq = i * i
  var acc = sq + 1
  acc = acc * 2
  print(acc)
  print(" ")
  i = i + 1
""" + NL), out="2 4 10 20 34 \n")
gen(c, "vnorene_cykly_stejna_jmena", main("""\
var i = 0
var total = 0
while i < 3 do
  val t = i * 10
  var j = 0
  while j < 3 do
    val t = j + 1
    total = total + t
    j = j + 1
  total = total + t
  i = i + 1
""" + show("total")), out="48\n")
gen(c, "vymena_pres_docasnou_promennou", main("""\
var a = 1
var b = 2
val t = a
a = b
b = t
""" + show("a", "b")), out="2\n1\n")
gen(c, "poradi_inicializace", main("""\
val a = 1
val b = a + 1
val c = b * 2
val d = c - a
""" + show("c", "d")), out="4\n3\n")
gen(c, "promenna_typu_unit", main("var u = ()\nu = ()\n" + show("u")), out="()\n")
gen(c, "tricet_promennych", main("".join("val a%d = %d\n" % (i, i) for i in range(1, 31))
                                 + "val s = " + " + ".join("a%d" % i for i in range(1, 31)) + "\n" + show("s")),
    out="465\n")
gen(c, "uprava_vnejsi_promenne_z_vnorenych_bloku", main("""\
var cnt = 0
var i = 0
while i < 4 do
  if i < 2 then
    cnt = cnt + 10
  else
    var j = 0
    while j < 2 do
      cnt = cnt + 1
      j = j + 1
  i = i + 1
""" + show("cnt")), out="24\n")
gen(c, "zasteneni_var_hodnotou_a_zmena_vnejsi", main("""\
var x = 1
if x < 2 then
  val x = 100
  print(x)
  print("\\n")
else
  print("no\\n")
x = x + 1
""" + show("x")), out="100\n2\n")
gen(c, "retezcova_promenna_v_tele_cyklu", main("""\
var i = 0
while i < 3 do
  val s = "item"
  val t = s + "!"
  print(t)
  i = i + 1
""" + NL), out="item!item!item!\n")

# ============================================================================= 07 funkce
c = "07_funkce"
gen(c, "volani_jako_prikaz_s_vypisem", prog("def hello(): Unit = print(\"hi\\n\")\n", "hello()\nhello()\n"), out="hi\nhi\n")
gen(c, "return_posledni_vyraz_jednoradkove_telo", prog("""\
def a(x: Int): Int =
  return x + 1

def b(x: Int): Int =
  val y = x * 2
  y

def c(x: Int): Int = x - 1
""", "val r1 = a(10)\nval r2 = b(10)\nval r3 = c(10)\n" + show("r1", "r2", "r3")), out="11\n20\n9\n")
gen(c, "parametry_se_predavaji_hodnotou", prog("""\
def inc(a: Int): Int =
  val b = a + 1
  return b
""", "val x = 5\nval y = inc(x)\n" + show("x", "y")), out="5\n6\n")
gen(c, "parametry_ruznych_typu", prog("""\
def describe(a: Int, b: Double, c: String): String =
  val sa = toStr(a)
  val sb = toStr(b)
  return sa + ":" + sb + ":" + c
""", 'val r = describe(1, 2.5, "x")\n' + show("r")), out="1:2.50:x\n")
gen(c, "faktorial_rekurzivne_10", prog("""\
def fact(n: Int): Int =
  if n < 2 then
    return 1
  else
    val m = n - 1
    val r = fact(m)
    return n * r
""", "val f = fact(10)\n" + show("f")), out="3628800\n")
gen(c, "fibonacci_rekurzivne_20", prog("""\
def fib(n: Int): Int =
  if n < 2 then
    return n
  else
    val a = n - 1
    val b = n - 2
    val x = fib(a)
    val y = fib(b)
    return x + y
""", "val f = fib(20)\n" + show("f")), out="6765\n")
gen(c, "hluboka_rekurze_2000", prog("""\
def sumTo(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = sumTo(m)
    return n + r
""", "val s = sumTo(2000)\n" + show("s")), out="2001000\n")
gen(c, "mocnina_rekurzivne", prog("""\
def pow(b: Int, e: Int): Int =
  if e == 0 then
    return 1
  else
    val e1 = e - 1
    val r = pow(b, e1)
    return b * r
""", "val p = pow(3, 13)\n" + show("p")), out="1594323\n")
gen(c, "ackermann_2_3", prog("""\
def ack(m: Int, n: Int): Int =
  if m == 0 then
    return n + 1
  else
    if n == 0 then
      val m1 = m - 1
      val r = ack(m1, 1)
      return r
    else
      val n1 = n - 1
      val m1 = m - 1
      val inner = ack(m, n1)
      val r = ack(m1, inner)
      return r
""", "val a = ack(2, 3)\n" + show("a")), out="9\n")
gen(c, "gcd_rekurzivne", prog("""\
def gcd(a: Int, b: Int): Int =
  if b == 0 then
    return a
  else
    val r = a - a / b * b
    val g = gcd(b, r)
    return g
""", "val g = gcd(48, 18)\n" + show("g")), out="6\n")
gen(c, "vzajemna_rekurze_sude_liche", prog("""\
def isEven(n: Int): Int =
  if n == 0 then
    return 1
  else
    val m = n - 1
    val r = isOdd(m)
    return r

def isOdd(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = isEven(m)
    return r
""", "val a = isEven(10)\nval b = isOdd(7)\nval c = isEven(7)\n" + show("a", "b", "c")), out="1\n1\n0\n")
gen(c, "volani_funkce_definovane_az_za_mainem",
    PRO + "@main def main(): Unit =\n"
    + textwrap.indent(dd("val x = twice(4)\nval y = once(x)\n" + show("x", "y")), "  ")
    + """
def twice(a: Int): Int =
  val b = once(a)
  return b + b

def once(a: Int): Int = a + 1
""", out="10\n11\n")
gen(c, "lokalni_promenne_prezijou_rekurzivni_volani", prog("""\
def depthSum(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val local = n * 10
    val rest = depthSum(m)
    return local + rest
""", "val a = depthSum(4)\nval b = depthSum(100)\n" + show("a", "b")), out="100\n50500\n")
gen(c, "navratova_hodnota_double_a_rozsireni", prog("""\
def toD(a: Int): Double =
  return a

def half(a: Int): Double = a / 2.0
""", "val x = toD(3)\nval y = half(5)\n" + show("x", "y")), out="0x1.8p+1\n0x1.4p+1\n")
gen(c, "funkce_vracejici_retezec", prog('def greet(name: String): String = "Hello, " + name\n',
                                        'val g = greet("IFJ")\n' + show("g")), out="Hello, IFJ\n")
gen(c, "unit_funkce_s_predcasnym_return", prog("""\
def early(a: Int): Unit =
  if a < 0 then
    print("neg\\n")
    return
  else
    print("pos\\n")
  print("end\\n")
""", "val m = -1\nearly(m)\nearly(1)\n"), out="neg\npos\nend\n")
gen(c, "unit_funkce_s_return_hodnoty", prog("""\
def f(): Unit =
  print("a\\n")
  return 5
""", "f()\n"), out="a\n")
gen(c, "return_uvnitr_cyklu", prog("""\
def firstAbove(n: Int): Int =
  var i = 0
  while i < 1000 do
    val sq = i * i
    if sq > n then
      return i
    else
      i = i + 1
  return -1
""", "val a = firstAbove(50)\nval b = firstAbove(2000000)\n" + show("a", "b")), out="8\n-1\n")
gen(c, "retezeni_volani_pres_promenne", prog("""\
def f(x: Int): Int = x + 1
def g(x: Int): Int = x * 2
def h(x: Int): Int = x - 3
""", "val a = f(2)\nval b = g(a)\nval c = h(b)\n" + show("c")), out="3\n")
gen(c, "lokalni_promenne_funkci_nekoliduji", prog("""\
def f(): Int =
  val x = 100
  return x + 1
""", "val x = 1\nval r = f()\n" + show("r", "x")), out="101\n1\n")
gen(c, "odvozene_navratove_typy", prog("""\
def sq(a: Int) = a * a
def name() = "ifj"
def nothing() =
  val u = 1
""", "val r = sq(7)\nval n = name()\nnothing()\n" + show("r", "n")), out="49\nifj\n")
gen(c, "vysledek_funkce_se_zahodi", prog("""\
def g(a: Int): Int =
  print("side\\n")
  return a + 5
""", "g(1)\ng(2)\n"), out="side\nside\n")
gen(c, "deset_parametru", prog("""\
def sum10(a: Int, b: Int, c: Int, d: Int, e: Int, f: Int, g: Int, h: Int, i: Int, j: Int): Int =
  return a + b + c + d + e + f + g + h + i + j
""", "val r = sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)\n" + show("r")), out="55\n")
gen(c, "soucet_cislic_ve_funkci", prog("""\
def digitSum(n: Int): Int =
  var m = n
  var s = 0
  while m > 0 do
    val d = m - m / 10 * 10
    s = s + d
    m = m / 10
  return s
""", "val r = digitSum(987654321)\n" + show("r")), out="45\n")
gen(c, "rekurze_nad_retezcem_otoceni", prog("""\
def rev(s: String): String =
  val n = strLength(s)
  if n < 2 then
    return s
  else
    val first = subStr(s, 0, 1)
    val rest = subStr(s, 1, n)
    val r = rev(rest)
    return r + first
""", 'val r = rev("stressed")\n' + show("r")), out="desserts\n")
gen(c, "hanoi_pocet_tahu_10", prog("""\
def moves(n: Int): Int =
  if n == 0 then
    return 0
  else
    val m = n - 1
    val r = moves(m)
    return 2 * r + 1
""", "val r = moves(10)\n" + show("r")), out="1023\n")
gen(c, "argument_typu_unit", prog("def u(a: Unit): Unit = print(a)\n", "u(())\n" + NL), out="()\n")
gen(c, "funkce_volaji_dalsi_funkce", prog("""\
def c(): Unit = print("c")

def b(): Unit =
  print("b")
  c()
  print("b")

def a(): Unit =
  print("a")
  b()
  print("a")
""", "a()\n" + NL), out="abcba\n")
gen(c, "funkce_volana_v_cyklu", prog("""\
def sq(a: Int): Int = a * a
""", """\
var i = 1
var s = 0
while i <= 5 do
  val r = sq(i)
  s = s + r
  i = i + 1
""" + show("s")), out="55\n")
gen(c, "main_s_jinym_nazvem", main('print("ok")\n' + NL, name="runFoo"), out="ok\n")
gen(c, "funkce_se_stejnymi_lokalnimi_jmeny", prog("""\
def f(n: Int): Int =
  val t = n + 1
  return t

def g(n: Int): Int =
  val t = n * 100
  val u = f(n)
  return t + u
""", "val t = 7\nval r = g(t)\n" + show("r", "t")), out="708\n7\n")

# ============================================================================= 08 vstup
c = "08_vstup"
rd = lambda body: main(body)  # noqa: E731
gen(c, "readInt_jednoduche", rd("val a = readInt()\n" + show("a")), stdin="42\n", out="42\n")
gen(c, "readInt_dva_vstupy_soucet", rd("val a = readInt()\nval b = readInt()\nval s = a + b\n" + show("s")),
    stdin="3\n4\n", out="7\n")
gen(c, "readInt_zaporne", rd("val a = readInt()\n" + show("a")), stdin="-5\n", out="-5\n")
gen(c, "readInt_bez_koncoveho_LF", rd("val a = readInt()\n" + show("a")), stdin="42", out="42\n")
for nm, inp in [("pismena", "abc\n"), ("uvodni_mezera", " 5\n"), ("koncova_mezera", "5 \n"),
                ("podtrzitko", "1_000\n"), ("desetinne_cislo", "3.5\n"), ("prazdny_radek", "\n"),
                ("eof", ""), ("mimo_rozsah", "99999999999999999999\n")]:
    gen(c, "readInt_neplatny_vstup_" + nm, rd("val a = readInt()\n" + show("a")), stdin=inp, out="0\n")
for nm, inp, exp in [("1_5", "1.5\n", "0x1.8p+0\n"), ("zaporne", "-0.25\n", "-0x1p-2\n"),
                     ("exponent", "1e3\n", "0x1.f4p+9\n"), ("cele_cislo", "3\n", "0x1.8p+1\n"),
                     ("pismena", "abc\n", "0x0p+0\n"), ("eof", "", "0x0p+0\n"),
                     ("uvodni_mezera", " 1.5\n", "0x0p+0\n")]:
    gen(c, "readDouble_" + nm, rd("val a = readDouble()\n" + show("a")), stdin=inp, out=exp)
gen(c, "readLine_vcetne_konce_radku", rd("val s = readLine()\nval n = strLength(s)\n" + show("n") + "print(s)\n"),
    stdin="abc\n", out="4\nabc\n")
gen(c, "readLine_bez_konce_radku_na_konci", rd("val s = readLine()\nval n = strLength(s)\n" + show("n")),
    stdin="abc", out="3\n")
gen(c, "readLine_po_eof_vraci_prazdny_retezec", rd("val s = readLine()\nval n = strLength(s)\n" + show("n")),
    stdin="", out="0\n")
gen(c, "readLine_dva_radky", rd("val a = readLine()\nval b = readLine()\nprint(a)\nprint(b)\n"),
    stdin="ab\ncd\n", out="ab\ncd\n")
gen(c, "soucet_n_cisel", rd("""\
val n = readInt()
var i = 0
var s = 0
while i < n do
  val x = readInt()
  s = s + x
  i = i + 1
""" + show("s")), stdin="3\n10\n20\n30\n", out="60\n")
gen(c, "cteni_do_nuly", rd("""\
var s = 0
var x = readInt()
while x != 0 do
  s = s + x
  x = readInt()
""" + show("s")), stdin="5\n3\n0\n7\n", out="8\n")
gen(c, "echo_radku_do_konce_vstupu", rd("""\
var line = readLine()
var n = strLength(line)
while n > 0 do
  print(line)
  line = readLine()
  n = strLength(line)
"""), stdin="a\nb\nc\n", out="a\nb\nc\n")
gen(c, "faktorial_ze_zadani_se_vstupem", PRO + """\
def factorial(n: Int): Int =
  var result = 1
  var i: Int = n
  while i > 1 do
    result = result * i
    i = i - 1
  result

@main def runFoo(): Unit =
  val n = readInt()
  val result = factorial(n)
  print(result)
  print("\\n")
""", stdin="5\n", out="120\n")
gen(c, "readInt_a_potom_readLine", rd("val a = readInt()\nval s = readLine()\n" + show("a") + "print(s)\n"),
    stdin="7\nhello\n", out="7\nhello\n")
gen(c, "prumer_tri_cisel_toStr", rd("""\
val n = readInt()
var i = 0
var sum = 0
while i < n do
  val x = readInt()
  sum = sum + x
  i = i + 1
val sd: Double = sum
val nd: Double = n
val avg = sd / nd
val t = toStr(avg)
""" + show("t")), stdin="3\n1\n2\n4\n", out="2.33\n")
gen(c, "readDouble_soucet_dvou", rd("""\
val a = readDouble()
val b = readDouble()
val s = a + b
""" + show("s")), stdin="1.5\n2.25\n", out="0x1.ep+1\n")

# ============================================================================= 09 běhové chyby
c = "09_behove_chyby"
gen(c, "deleni_nulou_int", main('print("before\\n")\nval a = 10\nval b = 0\nval q = a / b\nprint("after\\n")\n'),
    out="before\n", run=57)
gen(c, "deleni_nulou_ve_funkci", prog("""\
def divide(a: Int, b: Int): Int =
  val q = a / b
  return q
""", 'print("start\\n")\nval x = divide(6, 3)\n' + show("x") + "val y = divide(1, 0)\n" + show("y")),
    out="start\n2\n", run=57)
gen(c, "deleni_nulou_v_cyklu_po_nekolika_iteracich", main("""\
var i = 0
while i < 5 do
  val d = 3 - i
  val q = 10 / d
  print(q)
  print("\\n")
  i = i + 1
"""), out="3\n5\n10\n", run=57)
gen(c, "deleni_nulou_double", main('print("before\\n")\nval x = 1.5\nval z = 0.0\nval q = x / z\nprint("after\\n")\n'),
    out="before\n", run=57)
gen(c, "deleni_double_celociselnou_nulou", main('print("before\\n")\nval x = 1.5\nval q = x / 0\nprint("after\\n")\n'),
    out="before\n", run=57)
for nm, st, en in [("zaporny_start", "neg", "2"), ("zaporny_konec", "0", "neg"), ("start_vetsi_nez_konec", "2", "1"),
                   ("start_rovny_delce", "3", "3"), ("start_za_koncem", "5", "6"), ("konec_za_koncem", "0", "4")]:
    gen(c, "subStr_" + nm, main('print("before\\n")\nval neg = -1\nval a = %s\nval b = %s\nval s = subStr("abc", a, b)\n'
                                'print("after\\n")\n' % (st, en)), out="before\n", run=40)
gen(c, "subStr_prazdny_retezec", main('print("before\\n")\nval s = subStr("", 0, 0)\nprint("after\\n")\n'),
    out="before\n", run=40)
gen(c, "chr_128", main('print("before\\n")\nval big = 128\nval c = chr(big)\nprint("after\\n")\n'), out="before\n", run=58)
gen(c, "chr_zaporne", main('print("before\\n")\nval neg = -1\nval c = chr(neg)\nprint("after\\n")\n'), out="before\n", run=58)
gen(c, "chr_200", main('print("before\\n")\nval big = 200\nval c = chr(big)\nprint("after\\n")\n'), out="before\n", run=58)
gen(c, "bez_chyby_hranice_subStr", main("""\
val a = subStr("abc", 0, 3)
val b = subStr("abc", 2, 3)
val c = subStr("abc", 0, 0)
""" + show("a", "b", "c")), out="abc\nc\n\n")
gen(c, "bez_chyby_hranice_chr", main("""\
val a = chr(0)
val b = chr(127)
val c = chr(33)
val n = strLength(a)
val m = strLength(b)
""" + show("n", "m", "c")), out="1\n1\n!\n")
gen(c, "bez_chyby_deleni_nenulovym_zapornym", main("val a = -6\nval b = -3\nval q = a / b\n" + show("q")), out="2\n")
gen(c, "chyba_az_po_hlubokem_volani", prog("""\
def down(n: Int): Int =
  if n == 0 then
    val z = 0
    val q = 1 / z
    return q
  else
    val m = n - 1
    val r = down(m)
    return r
""", 'print("go\\n")\nval x = down(50)\nprint("never\\n")\n'), out="go\n", run=57)

# ============================================================================= 10 algoritmy
c = "10_algoritmy"
gen(c, "fibonacci_iterativne_30", main("""\
var a = 0
var b = 1
var i = 0
while i < 30 do
  val t = a + b
  a = b
  b = t
  i = i + 1
""" + show("a")), out="832040\n")
gen(c, "faktorial_12", main("""\
var f = 1
var i = 2
while i <= 12 do
  f = f * i
  i = i + 1
""" + show("f")), out="479001600\n")
gen(c, "gcd_iterativne_1071_462", main("""\
var a = 1071
var b = 462
while b != 0 do
  val t = b
  b = a - a / b * b
  a = t
""" + show("a")), out="21\n")
gen(c, "pocet_prvocisel_do_100", main("""\
var n = 2
var count = 0
while n < 100 do
  var d = 2
  var isPrime = 1
  while d * d <= n do
    val r = n - n / d * d
    if r == 0 then
      isPrime = 0
    else
      isPrime = isPrime
    d = d + 1
  if isPrime == 1 then
    count = count + 1
  else
    count = count
  n = n + 1
""" + show("count")), out="25\n")
gen(c, "otoceni_cisla", main("""\
var n = 12345
var rev = 0
while n > 0 do
  val d = n - n / 10 * 10
  rev = rev * 10 + d
  n = n / 10
""" + show("rev")), out="54321\n")
gen(c, "binarni_zapis_37", main("""\
var n = 37
var s = ""
while n > 0 do
  val bit = n - n / 2 * 2
  val b = toStr(bit)
  s = b + s
  n = n / 2
""" + show("s")), out="100101\n")
gen(c, "palindrom_racecar", main("""\
val s = "racecar"
val n = strLength(s)
var i = 0
var ok = 1
while i < n do
  val j = n - 1 - i
  val a = ord(s, i)
  val b = ord(s, j)
  if a != b then
    ok = 0
  else
    ok = ok
  i = i + 1
""" + show("ok")), out="1\n")
gen(c, "pocet_samohlasek", main("""\
val s = "programming language"
val n = strLength(s)
var i = 0
var cnt = 0
while i < n do
  val c = ord(s, i)
  if c == 97 then
    cnt = cnt + 1
  else
    cnt = cnt
  if c == 101 then
    cnt = cnt + 1
  else
    cnt = cnt
  if c == 105 then
    cnt = cnt + 1
  else
    cnt = cnt
  if c == 111 then
    cnt = cnt + 1
  else
    cnt = cnt
  if c == 117 then
    cnt = cnt + 1
  else
    cnt = cnt
  i = i + 1
""" + show("cnt")), out="7\n")
gen(c, "caesarova_sifra_posun_3", main("""\
val s = "hello"
val n = strLength(s)
var i = 0
var r = ""
while i < n do
  val c = ord(s, i)
  val shifted = c + 3
  val ch = chr(shifted)
  r = r + ch
  i = i + 1
""" + show("r")), out="khoor\n")
gen(c, "velka_pismena", main("""\
val s = "hello"
val n = strLength(s)
var i = 0
var r = ""
while i < n do
  val c = ord(s, i)
  val up = c - 32
  val ch = chr(up)
  r = r + ch
  i = i + 1
""" + show("r")), out="HELLO\n")
gen(c, "dokonala_cisla_do_500", main("""\
var n = 2
while n < 500 do
  var sum = 0
  var d = 1
  while d < n do
    val r = n - n / d * d
    if r == 0 then
      sum = sum + d
    else
      sum = sum
    d = d + 1
  if sum == n then
    print(n)
    print(" ")
  else
    sum = sum
  n = n + 1
""" + NL), out="6 28 496 \n")
gen(c, "newtonova_metoda_sqrt_2", main("""\
var x = 1.0
var i = 0
while i < 10 do
  x = (x + 2.0 / x) / 2.0
  i = i + 1
val t = toStr(x)
""" + show("t")), out="1.41\n")
gen(c, "pocet_slov_podle_mezer", main("""\
val s = "the quick brown fox"
val n = strLength(s)
var i = 0
var words = 1
while i < n do
  val c = ord(s, i)
  if c == 32 then
    words = words + 1
  else
    words = words
  i = i + 1
""" + show("words")), out="4\n")
gen(c, "linearni_kongruencni_generator", main("""\
var x = 1
var i = 0
while i < 10 do
  x = (x * 5 + 3) - (x * 5 + 3) / 16 * 16
  print(x)
  print(" ")
  i = i + 1
""" + NL), out="8 11 10 5 12 15 14 9 0 3 \n")
gen(c, "soucet_ctvercu_1_az_100", main("""\
var i = 1
var s = 0
while i <= 100 do
  s = s + i * i
  i = i + 1
""" + show("s")), out="338350\n")
gen(c, "nasobilka_5x5", main("""\
var i = 1
while i <= 5 do
  var j = 1
  while j <= 5 do
    val p = i * j
    print(p)
    print(" ")
    j = j + 1
  print("\\n")
  i = i + 1
"""), out="".join("".join("%d " % (i * j) for j in range(1, 6)) + "\n" for i in range(1, 6)))
gen(c, "obraceni_retezce_cyklem", main("""\
val s = "stressed"
val n = strLength(s)
var i = n - 1
var r = ""
while i >= 0 do
  val j = i + 1
  val ch = subStr(s, i, j)
  r = r + ch
  i = i - 1
""" + show("r")), out="desserts\n")
gen(c, "mocnina_iterativne_3_na_13", main("""\
var r = 1
var i = 0
while i < 13 do
  r = r * 3
  i = i + 1
""" + show("r")), out="1594323\n")


# ============================================================================= 11 PARAMLISTS
c = "11_paramlists"
gen(c, "bez_seznamu_jednoduche", prog("def answer: Int = 42", "val x = answer\n" + show("x")), out="42\n")
gen(c, "bez_seznamu_ve_vyrazu", prog("def two: Int = 2", "val x = two * 10 + two\n" + show("x")), out="22\n")
gen(c, "bez_seznamu_telo_se_vyhodnoti_pri_kazdem_pouziti", prog("""\
def line: String = readLine()
""", """\
val a = line
val b = line
print(b)
print(a)
"""), stdin="prvni\ndruhy\n", out="druhy\nprvni\n")
gen(c, "bez_seznamu_vedlejsi_efekt_pri_kazdem_volani", prog("""\
def tick: Int =
  print("t")
  1
""", """\
val x = tick + tick + tick
""" + NL + show("x")), out="ttt\n3\n")
gen(c, "bez_seznamu_jako_prikaz", prog('def hello: Unit = print("ahoj\\n")', "hello\nhello\n"), out="ahoj\nahoj\n")
gen(c, "bez_seznamu_jako_argument", prog("def five: Int = 5", "print(five)\n" + NL), out="5\n")
gen(c, "spec_priklad", PRO + dd("""\
def answer: Int = 42
def answerWithParens(): Int = 42
def add(a: Int)(b: Int)(c: Int): Int = a + b + c
def offset()(value: Int): Int = value + 1

@main def run(): Unit =
  val x = answer
  val y = answerWithParens()
  val sum = add(x)(y)(1)
  val result = offset()(sum)
  print(result)
"""), out="86")
gen(c, "dva_seznamy_poradi_parametru", prog("def sub(a: Int)(b: Int): Int = a - b", "val x = sub(10)(3)\n" + show("x")), out="7\n")
gen(c, "vice_parametru_v_seznamech", prog("""\
def f(a: Int, b: Int)(c: Int, d: Int): Int = a * 1000 + b * 100 + c * 10 + d
""", "val x = f(1, 2)(3, 4)\n" + show("x")), out="1234\n")
gen(c, "ruzne_typy_v_seznamech", prog("""\
def f(n: Int)(d: Double)(s: String): String =
  val a = toStr(n)
  val b = toStr(d)
  s + a + " " + b
""", 'val r = f(7)(1.5)("x=")\n' + show("r")), out="x=7 1.50\n")
gen(c, "poradi_vyhodnoceni_argumentu", prog("""\
def say(n: Int): Int =
  print(n)
  n
def f(a: Int)(b: Int)(c: Int): Int = a + b + c
""", "val x = f(say(1))(say(2))(say(3))\n" + NL + show("x")), out="123\n6\n")
gen(c, "rekurze_akumulator", prog("""\
def fact(n: Int)(acc: Int): Int =
  if n <= 1 then
    return acc
  else
    return fact(n - 1)(acc * n)
""", "val x = fact(10)(1)\n" + show("x")), out="3628800\n")
gen(c, "vnorene_volani", prog("def add(a: Int)(b: Int): Int = a + b",
                              "val x = add(add(1)(2))(add(3)(4))\n" + show("x")), out="10\n")
gen(c, "promenna_prekryje_funkci", prog("def answer: Int = 42", "val answer = 7\n" + show("answer")), out="7\n")


# ============================================================================= zápis
def write_generator():
    if os.path.isdir(GEN_DIR):
        shutil.rmtree(GEN_DIR)
    counters = {}
    for cat, name, src, stdin, out, run in G:
        src.encode("ascii")
        stdin.encode("ascii")
        o, rc = run_program(src, stdin)
        if rc != run:
            raise SystemExit("%s/%s: referenční interpret skončil s kódem %d, čekáno %d" % (cat, name, rc, run))
        if out is not None and o != out:
            raise SystemExit("%s/%s: ruční hodnota se liší od interpretu\n  ruční:     %r\n  interpret: %r"
                             % (cat, name, out, o))
        n = counters.get(cat, 0) + 1
        counters[cat] = n
        d = os.path.join(GEN_DIR, cat)
        os.makedirs(d, exist_ok=True)
        base = os.path.join(d, "%02d_%s.rc0%s" % (n, name, (".run%d" % run) if run else ""))
        with open(base + ".scala", "wb") as fh:
            fh.write(src.encode("ascii"))
        if stdin:
            with open(base + ".in", "wb") as fh:
                fh.write(stdin.encode("ascii"))
        with open(base + ".out", "wb") as fh:
            fh.write(o.encode("latin-1"))
    for cat in sorted(counters):
        print("generator  %-28s %3d" % (cat, counters[cat]))
    print("generator  %-28s %3d" % ("CELKEM", len(G)))


def write_compiler_outputs():
    """K platným programům v tests/compiler dopočítá očekávaný výstup (stdin je prázdný)."""
    n = 0
    for p in sorted(glob.glob(os.path.join(CMP_DIR, "*", "*.scala"))):
        m = re.search(r"\.rc(\d+(?:or\d+)*)(?:\.run(\d+))?\.scala$", p)
        if "0" not in m.group(1).split("or"):
            continue
        o, rc = run_program(open(p, "rb").read(), "")
        if rc != 0:
            raise SystemExit("%s: neočekávaný návratový kód %d" % (p, rc))
        with open(p[:-len(".scala")] + ".out", "wb") as fh:
            fh.write(o.encode("latin-1"))
        n += 1
    print("compiler   platné programy s očekávaným výstupem: %d" % n)


if __name__ == "__main__":
    write_generator()
    write_compiler_outputs()
