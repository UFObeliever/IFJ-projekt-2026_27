#!/usr/bin/env python3
"""
ifj_tests.py - testy generátoru IFJcode26 (základní zadání, bez rozšíření).

Každý test je zdrojový program IFJ26 + vstup + očekávaný stdout a návratový
kód interpretu. Runner:
  1. přeloží program vaším překladačem ( ./compiler < prog > prog.code ),
  2. provede STATICKÉ kontroly vygenerovaného kódu (hlavička, unikátní návěští,
     existující cíle skoků, DEFVAR jen na začátku funkce / v hlavičce),
  3. spustí interpret (ic26int, nebo záložní ic_mini.py) a porovná stdout + rc.

Testy chyb kompilace kontrolují návratový kód překladače a to, že na stdout
nevznikl ŽÁDNÝ výstup.

Příklady:
  ./ifj_tests.py                          # všechny testy
  ./ifj_tests.py -f fib -v                # jen testy s "fib" v názvu, podrobně
  ./ifj_tests.py --interp ic26int         # oficiální interpret
  ./ifj_tests.py --mini                   # záložní interpret (--strict)
  ./ifj_tests.py --skip-tags forward      # vynechat testy s tagem
  ./ifj_tests.py --export scala_out       # uloží .scala + .in pro ověření Scalou
  ./ifj_tests.py --keep out_dir           # uloží vygenerované .code

Tagy:  forward = volání funkce před její definicí (zadání to vyžaduje,
                 parser to dnes nepodporuje -> očekávaně padá)
       probe   = chování závisí na detailech interpretu (viz komentář testu)
       ext     = konstrukce nad rámec základu (volání ve výrazech); spouští se
                 jen s --with-ext
       frontend= testuje hlavně parser/sémantiku, ne generátor
"""
import argparse
import difflib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap

HERE = os.path.dirname(os.path.abspath(__file__))
PROLOG = "import ifj26.*\n\n"


class Case:
    def __init__(self, name, src, out="", inp="", rc=0, crc=0, tags=(),
                 note=""):
        self.name = name
        self.src = PROLOG + textwrap.dedent(src).lstrip("\n")
        self.out = out
        self.inp = inp
        self.rc = rc          # očekávaný rc interpretu
        self.crc = crc        # očekávaný rc překladače
        self.tags = set(tags)
        self.note = note


CASES = []


def T(*a, **k):
    CASES.append(Case(*a, **k))


def main_body(*lines):
    """Pomocník: @main funkce s odsazeným tělem (každý prvek = 1 řádek)."""
    return "@main def main(): Unit =\n" + "".join("  " + l + "\n" for l in lines)


def pl(x):
    """print(x) + newline jako dva řádky."""
    return ['print(%s)' % x, 'print("\\n")']


# =========================================================================
# 1. Výstup a literály
# =========================================================================
T("01_hello", """
    @main def main(): Unit =
      print("Hello, IFJ26!\\n")
    """, out="Hello, IFJ26!\n")

T("02_print_literals", """
    @main def main(): Unit =
      print(42)
      print("\\n")
      print(0)
      print("\\n")
      print(1_000_000)
      print("\\n")
      print("a\\tb\\\\c\\"d")
      print("\\n")
      print("")
      print(())
      print("\\n")
    """, out='42\n0\n1000000\na\tb\\c"d\n()\n')

T("03_unit_values", """
    def nothing(): Unit = ()

    @main def main(): Unit =
      val u = ()
      val r = nothing()
      print(u)
      print(r)
      val s = toStr(r)
      print(s)
      print("\\n")
    """, out="()()()\n")

T("04_identifiers_and_names", """
    def idiv(a: Int, b: Int): Int = a / b
    def substr(s: String): String = s
    def print2(n: Int): Int = n + 1

    @main def main(): Unit =
      val _x = 1
      val a_b1 = 2
      val Abc = 3
      val values = 4
      val iff = 5
      val _ret = 6
      val _t1 = 7
      val r = _x + a_b1 + Abc + values + iff
      val q = idiv(9, 2)
      val s = substr("ok")
      val p = print2(_t1)
      print(r)
      print("\\n")
      print(q)
      print("\\n")
      print(s)
      print("\\n")
      print(p)
      print("\\n")
      print(_ret)
      print("\\n")
    """, out="15\n4\nok\n8\n6\n",
  note="názvy proměnných/funkcí nesmí kolidovat s interními návěštími/registry")

# =========================================================================
# 2. Aritmetika
# =========================================================================
T("10_int_arith", """
    @main def main(): Unit =
      val a = 17
      val b = 5
      val sum = a + b
      val diff = a - b
      val prod = a * b
      val quo = a / b
      print(sum)
      print("\\n")
      print(diff)
      print("\\n")
      print(prod)
      print("\\n")
      print(quo)
      print("\\n")
    """, out="22\n12\n85\n3\n")

T("11_precedence_assoc", """
    @main def main(): Unit =
      val r1 = 2 + 3 * 4 - (1 + 1) * 2
      val r2 = 10 - 2 - 3
      val r3 = 100 / 10 / 5
      val r4 = -(2 + 3) * 4
      val r5 = 2 * (3 + 4) * 5
      val r6 = (((7)))
      val r7 = 20 - 3 * 2 + 8 / 4 * 3
      print(r1)
      print("\\n")
      print(r2)
      print("\\n")
      print(r3)
      print("\\n")
      print(r4)
      print("\\n")
      print(r5)
      print("\\n")
      print(r6)
      print("\\n")
      print(r7)
      print("\\n")
    """, out="10\n5\n2\n-20\n70\n7\n20\n")

T("12_int_div_truncates_to_zero", """
    @main def main(): Unit =
      val a = 7
      val na = -7
      val b = 2
      val nb = -2
      val z = 0
      val one = 1
      val m1 = -1
      val six = 6
      val nsix = -6
      val three = 3
      val nthree = -3
      val big = 1000000
      val seven = 7
      val q1 = a / b
      val q2 = na / b
      val q3 = a / nb
      val q4 = na / nb
      val q5 = z / nb
      val q6 = m1 / b
      val q7 = one / nb
      val q8 = m1 / nb
      val q9 = nsix / three
      val q10 = six / nthree
      val q11 = big / seven
      print(q1)
      print("\\n")
      print(q2)
      print("\\n")
      print(q3)
      print("\\n")
      print(q4)
      print("\\n")
      print(q5)
      print("\\n")
      print(q6)
      print("\\n")
      print(q7)
      print("\\n")
      print(q8)
      print("\\n")
      print(q9)
      print("\\n")
      print(q10)
      print("\\n")
      print(q11)
      print("\\n")
    """, out="3\n-3\n-3\n3\n0\n0\n0\n0\n-2\n-2\n142857\n",
  note="IDIV v IFJcode26 zaokrouhluje k -inf, IFJ26 dělí k nule ($$idiv)")

T("13_double_arith", """
    @main def main(): Unit =
      val a = 1.5
      val b = 2
      val s1 = a + b
      val s2 = a - b
      val s3 = a * 3
      val s4 = b / 4.0
      val s5 = 7.0 / 2
      val s6 = 10 / 4.0
      val s7 = 3.14159
      val s8 = -a
      val t1 = toStr(s1)
      val t2 = toStr(s2)
      val t3 = toStr(s3)
      val t4 = toStr(s4)
      val t5 = toStr(s5)
      val t6 = toStr(s6)
      val t7 = toStr(s7)
      val t8 = toStr(s8)
      print(t1)
      print("\\n")
      print(t2)
      print("\\n")
      print(t3)
      print("\\n")
      print(t4)
      print("\\n")
      print(t5)
      print("\\n")
      print(t6)
      print("\\n")
      print(t7)
      print("\\n")
      print(t8)
      print("\\n")
    """, out="3.50\n-0.50\n4.50\n0.50\n3.50\n2.50\n3.14\n-1.50\n",
  note="toStr(Double) = %.2f; záměrně se vyhýbám celým hodnotám (3.0)")

T("14_double_literal_forms", """
    @main def main(): Unit =
      val a = 2.5e-1
      val b = 1_0.2_5
      val c = 1e-1
      val d = 0.75d
      val e = 1.25E1
      val f = 5.5D
      val g = 1_000.5
      val ta = toStr(a)
      val tb = toStr(b)
      val tc = toStr(c)
      val td = toStr(d)
      val te = toStr(e)
      val tf = toStr(f)
      val tg = toStr(g)
      print(ta)
      print("\\n")
      print(tb)
      print("\\n")
      print(tc)
      print("\\n")
      print(td)
      print("\\n")
      print(te)
      print("\\n")
      print(tf)
      print("\\n")
      print(tg)
      print("\\n")
    """, out="0.25\n10.25\n0.10\n0.75\n12.50\n5.50\n1000.50\n")

T("15_implicit_int_to_double", """
    def half(x: Double): Double = x / 2
    def three(): Double = 7

    @main def main(): Unit =
      val d: Double = 5
      val a = d / 2
      var x: Double = 1
      x = 8
      val b = x / 16
      val c = half(9)
      val e = three()
      val f = e / 2
      val i = 3
      val g = i + 0.25
      val h = 0.5 + i
      var y: Double = 0.5
      y = 3
      val k = y / 2
      val ta = toStr(a)
      val tb = toStr(b)
      val tc = toStr(c)
      val tf = toStr(f)
      val tg = toStr(g)
      val th = toStr(h)
      val tk = toStr(k)
      print(ta)
      print("\\n")
      print(tb)
      print("\\n")
      print(tc)
      print("\\n")
      print(tf)
      print("\\n")
      print(tg)
      print("\\n")
      print(th)
      print("\\n")
      print(tk)
      print("\\n")
    """, out="2.50\n0.50\n4.50\n3.50\n3.25\n3.50\n1.50\n",
  note="konverze při inicializaci, přiřazení, parametru i návratové hodnotě")

T("16_expression_stress", """
    @main def main(): Unit =
      val n = 7
      val a = ((1 + 2) * (3 + 4) - 5) / 2
      val b = (1.5 + 2) * (3 + 0.5)
      val c = 2 * 3 + 4 * 5 - 6 / 2 + 7
      val d = 1 + 2 * 3 * 4 - 5
      val e = -n * -n
      val z = -(0 - 3)
      val g = 8 / 3 * 3
      val h = (n + 1) / 2 * 2 + n / 2
      val s = "a" + "b" + "c"
      val i = (1 + 2) / 2.0
      val j = 1 / 2 + 0.5
      val k = 3 * 0.5
      val tb = toStr(b)
      val ti = toStr(i)
      val tj = toStr(j)
      val tk = toStr(k)
      print(a)
      print("\\n")
      print(tb)
      print("\\n")
      print(c)
      print("\\n")
      print(d)
      print("\\n")
      print(e)
      print("\\n")
      print(z)
      print("\\n")
      print(g)
      print("\\n")
      print(h)
      print("\\n")
      print(s)
      print("\\n")
      print(ti)
      print("\\n")
      print(tj)
      print("\\n")
      print(tk)
      print("\\n")
    """, out="8\n12.25\n30\n20\n49\n3\n6\n11\nabc\n1.50\n0.50\n1.50\n",
  note="'1 / 2 + 0.5' = 0 + 0.5: celočíselné dělení se provede PŘED převodem")

# =========================================================================
# 3. Řetězce
# =========================================================================
T("20_strings_builtins", """
    @main def main(): Unit =
      val a = "Hello"
      val b = " "
      val c = "World"
      val ab = a + b
      val t = ab + c
      val len = strLength(t)
      val w = subStr(t, 6, 11)
      val h = subStr(t, 0, 5)
      val e = subStr(t, 3, 3)
      val o1 = ord(t, 0)
      val o2 = ord(t, 4)
      val oob = ord(t, 100)
      val emp = ""
      val o3 = ord(emp, 0)
      val neg = 0 - 1
      val o4 = ord(t, neg)
      val ch = chr(65)
      val n = toStr(len)
      val msg = "len=" + n
      val z0 = strLength(emp)
      print(len)
      print("\\n")
      print(w)
      print("\\n")
      print(h)
      print("\\n")
      print("[")
      print(e)
      print("]\\n")
      print(o1)
      print("\\n")
      print(o2)
      print("\\n")
      print(oob)
      print("\\n")
      print(o3)
      print("\\n")
      print(o4)
      print("\\n")
      print(ch)
      print("\\n")
      print(msg)
      print("\\n")
      print(z0)
      print("\\n")
    """, out="11\nWorld\nHello\n[]\n72\n111\n-1\n-1\n-1\nA\nlen=11\n0\n")

T("21_string_escapes_special_chars", """
    @main def main(): Unit =
      val s = "a b#c\\\\d\\te"
      val sl = strLength(s)
      val q = "x\\"y"
      val nl = "line1\\nline2"
      val cr = "a\\rb"
      val h = ord("#", 0)
      val bs = ord("\\\\", 0)
      val sp = ord(" ", 0)
      print(s)
      print("\\n")
      print(sl)
      print("\\n")
      print(q)
      print("\\n")
      print(nl)
      print("\\n")
      print(cr)
      print("\\n")
      print(h)
      print("\\n")
      print(bs)
      print("\\n")
      print(sp)
      print("\\n")
    """, out='a b#c\\d\te\n9\nx"y\nline1\nline2\na\rb\n35\n92\n32\n',
  note="znaky 0-32, 35 (#) a 92 (\\) se v IFJcode26 zapisují jako \\xyz")

T("22_multiline_string", """
    @main def main(): Unit =
      val m = \"\"\"line1
    line2 \\n stays\"\"\"
      print(m)
      print("|\\n")
      val ml = strLength(m)
      print(ml)
      print("\\n")
      val q = \"\"\"say "hi" # ok\"\"\"
      print(q)
      print("\\n")
    """,
  out='line1\nline2 \\n stays|\n20\nsay "hi" # ok\n',
  note="obsah víceřádkového literálu je doslovný (escape se nevyhodnocují)")

T("23_toStr_variants", """
    @main def main(): Unit =
      val i = 123
      val n = -12
      val s = "abc"
      val u = ()
      val ti = toStr(i)
      val tn = toStr(n)
      val ts = toStr(s)
      val tu = toStr(u)
      val tl = toStr("lit")
      print(ti)
      print("\\n")
      print(tn)
      print("\\n")
      print(ts)
      print("\\n")
      print(tu)
      print("\\n")
      print(tl)
      print("\\n")
    """, out="123\n-12\nabc\n()\nlit\n")

T("24_toInt_truncates", """
    @main def main(): Unit =
      val a = 3.99
      val b = 0.0 - 2.7
      val c = 0.5
      val ia = toInt(a)
      val ib = toInt(b)
      val ic = toInt(c)
      print(ia)
      print("\\n")
      print(ib)
      print("\\n")
      print(ic)
      print("\\n")
    """, out="3\n-2\n0\n")


def _cmp_fn(name, t1, t2):
    ops = ["<", "<=", ">", ">=", "==", "!="]
    body = ""
    for op in ops:
        body += ('  if a %s b then\n    print("T")\n  else\n    print("F")\n'
                 % op)
    body += '  print("\\n")\n'
    return "def %s(a: %s, b: %s): Unit =\n%s\n" % (name, t1, t2, body)


_LT, _EQ, _GT = "TTFFFT", "FTFTTF", "FFTTFT"
T("30_comparisons", (
    _cmp_fn("cmpI", "Int", "Int") + _cmp_fn("cmpD", "Double", "Double") +
    _cmp_fn("cmpM", "Int", "Double") + _cmp_fn("cmpN", "Double", "Int") +
    _cmp_fn("cmpS", "String", "String") + """
@main def main(): Unit =
  cmpI(3, 5)
  cmpI(5, 5)
  cmpI(5, 3)
  cmpD(1.5, 2.5)
  cmpD(2.5, 2.5)
  cmpD(2.5, 1.5)
  cmpM(2, 2.5)
  cmpM(3, 2.5)
  cmpM(2, 2.0)
  cmpN(2.5, 2)
  cmpN(2.0, 2)
  cmpN(1.5, 2)
  cmpS("a", "b")
  cmpS("b", "b")
  cmpS("b", "a")
  cmpS("abc", "abd")
  cmpS("", "a")
  cmpS("a", "")
"""),
  out="\n".join([_LT, _EQ, _GT,            # Int
                 _LT, _EQ, _GT,            # Double
                 _LT, _GT, _EQ,            # Int vs Double
                 _GT, _EQ, _LT,            # Double vs Int
                 _LT, _EQ, _GT, _LT, _LT, _GT]) + "\n",
  note="pořadí operátorů: < <= > >= == !=")

# =========================================================================
# 4. Řízení toku, proměnné, rozsahy
# =========================================================================
T("40_if_else", """
    def classify(n: Int): String =
      if n < 0 then
        return "neg"
      else
        if n == 0 then
          return "zero"
        else
          return "pos"

    @main def main(): Unit =
      val m = -5
      val z = 0
      val p = 9
      val r1 = classify(m)
      val r2 = classify(z)
      val r3 = classify(p)
      print(r1)
      print("\\n")
      print(r2)
      print("\\n")
      print(r3)
      print("\\n")
      val x = 3
      if x < 5 then
        print("small")
      else
        ()
      if x > 5 then
        print("big")
      else
        ()
      print("\\n")
    """, out="neg\nzero\npos\nsmall\n")

T("41_while", """
    @main def main(): Unit =
      var i = 1
      var sum = 0
      while i <= 10 do
        sum = sum + i
        i = i + 1
      print(sum)
      print("\\n")
      var n = 0
      while n < 3 do
        val sq = n * n
        var t = sq + 1
        print(t)
        print("\\n")
        n = n + 1
      var a = 0
      var cnt = 0
      while a < 4 do
        var b = 0
        while b < 3 do
          cnt = cnt + 1
          b = b + 1
        a = a + 1
      print(cnt)
      print("\\n")
      while a < 0 do
        print("never")
      print("done\\n")
    """, out="55\n1\n2\n5\n12\ndone\n",
  note="proměnné deklarované v těle cyklu: DEFVAR musí být před cyklem")

T("42_shadowing", """
    @main def main(): Unit =
      val x = 1
      val v = 5
      if x == 1 then
        val x = 2
        val v = "str"
        print(x)
        print(v)
        print("\\n")
      else
        ()
      print(x)
      print(v)
      print("\\n")
      var i = 0
      while i < 2 do
        val x = 10
        print(x)
        i = i + 1
      print(x)
      print("\\n")
    """, out="2str\n15\n10101\n")

T("43_scopes_many_locals", """
    def many(p: Int, q: Int): Int =
      val a = p + 1
      val b = q + 2
      var c = a + b
      if c > 5 then
        val d = c * 2
        var e = d + 1
        c = e
      else
        val d = 0
        c = d + 1
      var f = c
      while f < 100 do
        val g = f * 2
        f = g
      val h = f + a
      val i = h + b
      i

    def other(s: String): String =
      val a = s + "!"
      var b = a + "?"
      b

    @main def main(): Unit =
      val r1 = many(1, 2)
      val r2 = many(0, 0)
      val r3 = other("hi")
      print(r1)
      print("\\n")
      print(r2)
      print("\\n")
      print(r3)
      print("\\n")
      val k = 1
      if k == 1 then
        val t = "then"
        print(t)
      else
        val t = 99
        print(t)
      print("\\n")
    """, out="110\n131\nhi!?\nthen\n",
  note="stejná jména v sourozeneckých blocích i v různých funkcích")

T("44_semicolons", """
    @main def main(): Unit =
      val a = 1; val b = 2
      val s = a + b; print(s); print("\\n")
    """, out="3\n")

T("45_comments_and_layout", """
    /* block
       comment */
    // line comment
    @main
    def main(): Unit =   // trailing
      /* nested /* inner */ outer */ print("ok")

      print("\\n") // end
    """, out="ok\n", tags=["frontend"])

# =========================================================================
# 5. Funkce
# =========================================================================
T("50_function_params_and_calls", """
    def sub3(a: Int, b: Int, c: Int): Int = a - b * c
    def five(): Int = 5
    def add1(n: Int): Int = n + 1
    def add3(n: Int): Int =
      val a = add1(n)
      val b = add1(a)
      add1(b)
    def concat3(a: String, b: String, c: String): String =
      val ab = a + b
      ab + c
    def mix(a: Int, b: String, c: Double, d: Int): String =
      val s = toStr(a)
      val t = toStr(d)
      val u = toStr(c)
      s + b + u + t
    def viaReturn(n: Int): Int =
      return add1(n)

    @main def main(): Unit =
      val x = sub3(10, 3, 2)
      val y = five()
      val z = add3(y)
      val w = concat3("a", "b", "c")
      val m = mix(1, "-", 2.5, 3)
      val r = viaReturn(41)
      val o = sub3(x, y, z)
      add1(5)
      print(x)
      print("\\n")
      print(y)
      print("\\n")
      print(z)
      print("\\n")
      print(w)
      print("\\n")
      print(m)
      print("\\n")
      print(r)
      print("\\n")
      print(o)
      print("\\n")
    """, out="4\n5\n8\nabc\n1-2.503\n42\n-36\n",
  note="pořadí parametrů, řetězení volání, zahozený výsledek volání")

T("51_return_rules", """
    def firstDiv7(n: Int): Int =
      var i = n
      while i < 1000 do
        if i / 7 * 7 == i then
          return i
        else
          ()
        i = i + 1
      0 - 1

    def early(n: Int): Unit =
      if n < 0 then
        print("neg\\n")
        return
      else
        ()
      print("nonneg\\n")

    def discard(n: Int): Unit =
      n

    def noType() =
      print("inferred unit\\n")

    def twice(n: Int) = n * 2

    def lastAssign(n: Int) =
      var k = n
      k = k + 1

    @main def main(): Unit =
      val a = firstDiv7(50)
      val big = 2000
      val b = firstDiv7(big)
      val neg = -4
      val m = 3
      print(a)
      print("\\n")
      print(b)
      print("\\n")
      early(neg)
      early(m)
      val d = discard(5)
      print(d)
      print("\\n")
      noType()
      val t = twice(21)
      print(t)
      print("\\n")
      val u = lastAssign(1)
      print(u)
      print("\\n")
    """, out="56\n-1\nneg\nnonneg\n()\ninferred unit\n42\n()\n",
  note="return z cyklu, return bez hodnoty, Unit zahazuje hodnotu, "
       "odvození návratového typu")

T("52_recursion", """
    def fib(n: Int): Int =
      if n < 2 then
        return n
      else
        val a = n - 1
        val b = n - 2
        val x = fib(a)
        val y = fib(b)
        return x + y

    def sumTo(n: Int): Int =
      if n == 0 then
        return 0
      else
        val m = n - 1
        val r = sumTo(m)
        return n + r

    @main def main(): Unit =
      val f10 = fib(10)
      val f15 = fib(15)
      val s = sumTo(5000)
      print(f10)
      print("\\n")
      print(f15)
      print("\\n")
      print(s)
      print("\\n")
    """, out="55\n610\n12502500\n",
  note="každá instance rekurze má vlastní lokální rámec; hloubka 5000")

T("53_factorial_with_input", """
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
    """, inp="10\n", out="3628800\n")

T("54_forward_call_and_mutual_recursion", """
    @main def main(): Unit =
      val a = isEven(10)
      val b = isOdd(7)
      val c = isEven(7)
      print(a)
      print("\\n")
      print(b)
      print("\\n")
      print(c)
      print("\\n")

    def isEven(n: Int): Int =
      if n == 0 then
        return 1
      else
        val m = n - 1
        return isOdd(m)

    def isOdd(n: Int): Int =
      if n == 0 then
        return 0
      else
        val m = n - 1
        return isEven(m)
    """, out="1\n1\n0\n", tags=["forward"],
  note="zadání dovoluje volat funkci před definicí a vzájemnou rekurzi")

T("55_loop_with_calls_stack_balance", """
    def inc(n: Int): Int = n + 1

    @main def main(): Unit =
      var i = 0
      var s = 0
      while i < 50000 do
        s = inc(s)
        i = i + 1
      print(s)
      print("\\n")
    """, out="50000\n",
  note="kontroluje vyváženost zásobníku rámců a datového zásobníku")

# =========================================================================
# 6. Vstup / výstup
# =========================================================================
T("60_read_builtins", """
    @main def main(): Unit =
      val n = readInt()
      val d = readDouble()
      val l1 = readLine()
      val l2 = readLine()
      val l3 = readLine()
      val s = toStr(d)
      val k = n * 2
      print(n)
      print("\\n")
      print(s)
      print("\\n")
      print(l1)
      print(l2)
      print("[")
      print(l3)
      print("]\\n")
      print(k)
      print("\\n")
    """, inp="42\n3.25\nhello world\nsecond\n",
  out="42\n3.25\nhello world\nsecond\n[]\n84\n",
  note="readLine vrací řádek včetně LF; na EOF prázdný řetězec")

T("61_read_invalid_input", """
    @main def main(): Unit =
      val a = readInt()
      val b = readInt()
      val c = readInt()
      val d = readInt()
      val e = readInt()
      val z = readDouble()
      print(a)
      print("\\n")
      print(b)
      print("\\n")
      print(c)
      print("\\n")
      print(d)
      print("\\n")
      print(e)
      print("\\n")
      if z == 0.0 then
        print("zero\\n")
      else
        print("nonzero\\n")
    """, inp="-5\n 7\n12x\n\n", out="-5\n0\n0\n0\n0\nzero\n", tags=["probe"],
  note="špatný formát/EOF => 0. Padá-li, vrací READ pro chybu nil a "
       "generátor musí nil převést na 0 (TYPE + MOVE)")

T("62_print_double_hex", """
    @main def main(): Unit =
      val a = 1.5
      val b = 0.1
      val c = 2.25
      print(a)
      print("\\n")
      print(b)
      print("\\n")
      print(c)
      print("\\n")
    """, out="0x1.8p+0\n0x1.999999999999ap-4\n0x1.2p+1\n", tags=["probe"],
  note="print(Double) = WRITE = printf('%a'); očekávání platí pro glibc")

# =========================================================================
# 7. Běhové chyby (návratový kód interpretu + částečný výstup)
# =========================================================================


def _err(name, stmts, rc, note=""):
    body = ['print("before\\n")'] + stmts + ['print("after\\n")']
    T(name, main_body(*body), out="before\n", rc=rc, note=note)


_err("70_err_subStr_start_gt_end", ['val r = subStr("abc", 2, 1)'], 40)
_err("71_err_subStr_end_over_len", ['val r = subStr("abc", 0, 4)'], 40)
_err("72_err_subStr_start_eq_len", ['val r = subStr("abc", 3, 3)'], 40,
     "start >= strLength => chyba 40")
_err("73_err_subStr_empty_string", ['val r = subStr("", 0, 0)'], 40)
_err("74_err_subStr_negative_start",
     ['val n = -1', 'val r = subStr("abc", n, 2)'], 40)
_err("75_err_subStr_negative_end",
     ['val n = -1', 'val r = subStr("abc", 0, n)'], 40)
_err("76_err_int_div_zero", ['val z = 0', 'val r = 10 / z'], 57)
_err("77_err_double_div_zero", ['val z = 0.0', 'val r = 1.0 / z'], 57,
     "instrukce DIV dělení nulou hlásí chybu 57")
_err("78_err_chr_out_of_range", ['val c = 128', 'val r = chr(c)'], 58)
_err("79_err_chr_negative", ['val c = -1', 'val r = chr(c)'], 58)

T("80_subStr_chr_valid_bounds", """
    @main def main(): Unit =
      val s = "abc"
      val a = subStr(s, 0, 0)
      val b = subStr(s, 2, 3)
      val c = subStr(s, 0, 3)
      val c0 = chr(0)
      val l = strLength(c0)
      val c127 = chr(127)
      val o = ord(c127, 0)
      print("[")
      print(a)
      print("]")
      print(b)
      print(c)
      print("\\n")
      print(l)
      print("\\n")
      print(o)
      print("\\n")
    """, out="[]cabc\n1\n127\n")

# =========================================================================
# 8. Chyby kompilace: rc překladače + prázdný stdout
# =========================================================================


def _ce(name, src, crc, note=""):
    T(name, src, crc=crc, tags=["frontend"], note=note)


_ce("90_lex_unknown_char",
    "@main def main(): Unit =\n  val x = 5 # 3\n", 1)
_ce("91_lex_tab_indent",
    "@main def main(): Unit =\n\tprint(\"x\")\n", 1)
_ce("92_syn_if_without_else", """
    @main def main(): Unit =
      val x = 1
      if x < 2 then
        print("a")
    """, 2)
_ce("93_sem_no_main", """
    def foo(): Unit = ()
    """, 3)
_ce("94_sem_undefined_var", """
    @main def main(): Unit =
      print(x)
    """, 3)
_ce("95_sem_undefined_func", """
    @main def main(): Unit =
      foo()
    """, 3)
_ce("96_sem_var_out_of_scope", """
    @main def main(): Unit =
      val k = 1
      if k == 1 then
        val t = 2
        print(t)
      else
        ()
      print(t)
    """, 3)
_ce("97_sem_redefinition_var", """
    @main def main(): Unit =
      val a = 1
      val a = 2
    """, 4)
_ce("98_sem_redefinition_func", """
    def f(): Int = 1
    def f(): Int = 2
    @main def main(): Unit = ()
    """, 4)
_ce("99_sem_two_mains", """
    @main def a(): Unit = ()
    @main def b(): Unit = ()
    """, 4)
_ce("A0_sem_wrong_arg_count", """
    def f(a: Int): Int = a
    @main def main(): Unit =
      val x = f(1, 2)
    """, 5)
_ce("A1_sem_wrong_arg_type", """
    def f(a: Int): Int = a
    @main def main(): Unit =
      val x = f("s")
    """, 5)
_ce("A2_sem_builtin_wrong_type", """
    @main def main(): Unit =
      val x = strLength(5)
    """, 5)
_ce("A3_sem_main_with_param", """
    @main def main(a: Int): Unit = ()
    """, 5)
_ce("A4_sem_main_wrong_return", """
    @main def main(): Int = 1
    """, 6)
_ce("A5_type_string_to_int", """
    @main def main(): Unit =
      var x = 1
      x = "s"
    """, 6)
_ce("A6_type_double_to_int_init", """
    @main def main(): Unit =
      val x: Int = 1.5
    """, 6)
_ce("A7_type_string_plus_int", """
    @main def main(): Unit =
      val s = "a" + 1
    """, 6)
_ce("A8_type_condition_not_bool", """
    @main def main(): Unit =
      if 1 then
        ()
      else
        ()
    """, 6)
_ce("A9_type_return_without_explicit_type", """
    def f(n: Int) =
      return n
    @main def main(): Unit = ()
    """, 6)
_ce("B0_type_bare_return_in_int_func", """
    def f(n: Int): Int =
      return
    @main def main(): Unit = ()
    """, 6)
_ce("B1_write_to_val", """
    @main def main(): Unit =
      val x = 1
      x = 2
    """, 7)
_ce("B2_write_to_param", """
    def f(a: Int): Int =
      a = 5
      a
    @main def main(): Unit = ()
    """, 7)

# =========================================================================
# 9. Rozšíření-lite: volání ve výrazech (parser to už umí, base nevyžaduje)
# =========================================================================
T("X1_nested_calls_in_args", """
    def add1(n: Int): Int = n + 1

    @main def main(): Unit =
      val r = add1(add1(add1(5)))
      print(r)
      print("\\n")
    """, out="8\n", tags=["ext"],
  note="argumenty se vyhodnotí na zásobník dřív než CREATEFRAME")

T("X2_call_in_expression", """
    def sq(n: Int): Int = n * n

    @main def main(): Unit =
      val a = sq(3) + sq(4)
      val s = toStr(2 * sq(5))
      print(a)
      print("\\n")
      print(s)
      print("\\n")
    """, out="25\n50\n", tags=["ext"])


# =========================================================================
# Statické kontroly vygenerovaného kódu
# =========================================================================
JUMPS = {"CALL", "JUMP", "JUMPIFEQ", "JUMPIFNEQ", "JUMPIFEQS", "JUMPIFNEQS"}


def static_check(code):
    errs = []
    lines = [l.split("#", 1)[0].strip() for l in code.split("\n")]
    lines = [l for l in lines if l]
    if not lines or lines[0] != ".IFJcode26":
        errs.append("první řádek musí být .IFJcode26")
        return errs
    ins = [l.split() for l in lines[1:]]
    labels = [i[1] for i in ins if i[0].upper() == "LABEL"]
    dup = {l for l in labels if labels.count(l) > 1}
    for d in sorted(dup):
        errs.append("návěští definováno víckrát: " + d)
    ls = set(labels)
    for i in ins:
        if i[0].upper() in JUMPS and i[1] not in ls:
            errs.append("skok na neexistující návěští: " + " ".join(i))
    # DEFVAR GF jen v hlavičce (před prvním CREATEFRAME / LABEL / CALL)
    in_header = True
    in_fn, body_started = False, False
    for i in ins:
        op = i[0].upper()
        if in_header and op in ("CREATEFRAME", "LABEL", "CALL", "EXIT"):
            in_header = False
        if op == "DEFVAR" and i[1].startswith("GF@") and not in_header:
            errs.append("DEFVAR GF mimo hlavičku: " + i[1])
        if op == "LABEL" and i[1].startswith("$fn_"):
            in_fn, body_started = True, False
            continue
        if op == "LABEL" and not i[1].startswith("$fn_") and in_fn:
            pass
        if in_fn:
            if op == "DEFVAR" and i[1].startswith("LF@"):
                if body_started:
                    errs.append("DEFVAR %s uvnitř těla funkce (hrozí "
                                "redefinice v cyklu)" % i[1])
            elif op != "PUSHFRAME":
                body_started = True
    if not any(i[0].upper() == "EXIT" for i in ins):
        errs.append("chybí EXIT")
    return errs


# =========================================================================
# Runner
# =========================================================================
def find_interp(args):
    if args.mini:
        return [sys.executable, os.path.join(HERE, "ic_mini.py"), "--strict"]
    if args.interp:
        return [args.interp]
    p = shutil.which("ic26int")
    if p:
        return [p]
    print("(ic26int nenalezen v PATH -> používám záložní ic_mini.py)",
          file=sys.stderr)
    return [sys.executable, os.path.join(HERE, "ic_mini.py"), "--strict"]


def show_diff(exp, got):
    d = difflib.unified_diff(exp.splitlines(True), got.splitlines(True),
                             "očekáváno", "získáno", n=1)
    return "".join(d)


def run_case(c, args, interp, tmp):
    base = os.path.join(tmp, c.name)
    with open(base + ".ifj26", "w") as f:
        f.write(c.src)
    try:
        p = subprocess.run([args.compiler], input=c.src.encode(),
                           capture_output=True, timeout=args.timeout)
    except subprocess.TimeoutExpired:
        return False, "překladač: timeout"
    code = p.stdout.decode("latin-1")
    if c.crc != 0:
        if p.returncode != c.crc:
            return False, "rc překladače %d, očekáváno %d" % (p.returncode,
                                                              c.crc)
        if code:
            return False, "při chybě kompilace nesmí vzniknout žádný výstup"
        return True, ""
    if p.returncode != 0:
        err = p.stderr.decode(errors="replace").strip().splitlines()[-3:]
        return False, "překladač vrátil %d (čekáno 0)\n    %s" % (
            p.returncode, "\n    ".join(err))
    with open(base + ".code", "w") as f:
        f.write(code)
    if args.keep:
        os.makedirs(args.keep, exist_ok=True)
        shutil.copy(base + ".code", os.path.join(args.keep, c.name + ".code"))
    serr = static_check(code)
    if serr:
        return False, "statická kontrola:\n    " + "\n    ".join(serr)
    try:
        r = subprocess.run(interp + [base + ".code"], input=c.inp.encode(),
                           capture_output=True, timeout=args.timeout)
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


def export_scala(args):
    os.makedirs(args.export, exist_ok=True)
    n = 0
    for c in CASES:
        if c.crc != 0:
            continue
        with open(os.path.join(args.export, c.name + ".scala"), "w") as f:
            f.write(c.src)
        if c.inp:
            with open(os.path.join(args.export, c.name + ".in"), "w") as f:
                f.write(c.inp)
        n += 1
    print("exportováno %d programů do %s (scala run ifj26.scala X.scala)"
          % (n, args.export))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--compiler", default="./compiler")
    ap.add_argument("--interp", help="cesta k ic26int")
    ap.add_argument("--mini", action="store_true",
                    help="použít záložní ic_mini.py")
    ap.add_argument("-f", "--filter", default="",
                    help="jen testy s tímto podřetězcem v názvu")
    ap.add_argument("--skip-tags", default="",
                    help="čárkou oddělené tagy k vynechání")
    ap.add_argument("--only-tags", default="")
    ap.add_argument("--with-ext", action="store_true",
                    help="spustit i testy s tagem ext")
    ap.add_argument("--keep", help="adresář pro vygenerované .code")
    ap.add_argument("--export", help="uložit programy jako .scala + .in")
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if args.export:
        export_scala(args)
        return 0
    skip = {t for t in args.skip_tags.split(",") if t}
    if not args.with_ext:
        skip.add("ext")
    only = {t for t in args.only_tags.split(",") if t}
    sel = [c for c in CASES if args.filter in c.name and not (c.tags & skip)
           and (not only or c.tags & only)]
    if args.list:
        for c in sel:
            print("%-42s %s" % (c.name, ",".join(sorted(c.tags))))
        return 0
    interp = find_interp(args)
    tmp = tempfile.mkdtemp(prefix="ifjtest_")
    ok = fail = 0
    failed = []
    try:
        for c in sel:
            good, msg = run_case(c, args, interp, tmp)
            tag = (" [%s]" % ",".join(sorted(c.tags))) if c.tags else ""
            if good:
                ok += 1
                if args.verbose:
                    print("PASS  %s%s" % (c.name, tag))
            else:
                fail += 1
                failed.append(c.name)
                print("FAIL  %s%s\n    %s" % (c.name, tag, msg))
                if c.note:
                    print("    poznámka: " + c.note)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("\n%d prošlo, %d selhalo (z %d)" % (ok, fail, ok + fail))
    if failed:
        print("selhalo: " + ", ".join(failed))
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
