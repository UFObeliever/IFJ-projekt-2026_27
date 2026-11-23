#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Referenční interpret základního jazyka IFJ26 (bez rozšíření).

Slouží jen k tomu, aby se z programu v IFJ26 spočítal očekávaný výstup a návratový
kód ("co by měl vypsat správně přeložený program"). Není to překladač a netestuje
statické kontroly – počítá s tím, že program je platný.

Použití jako knihovna:   from ref_interp import run_program
                         stdout, rc = run_program(zdroj, stdin)
Použití z příkazové řádky:  python3 ref_interp.py prog.scala < vstup

Sémantika (rozhodnutí podle zadání):
  * print:  Int jako %d, Double jako %a (jako instrukce WRITE), String doslova, Unit jako "()"
  * toStr:  Double jako %.2f (viz zadání); u Double bez desetinné části je chování
            ve zadání nejednoznačné, proto interpret v tom případě hlásí chybu testu
  * Int:    dělení zaokrouhluje k nule, dělení nulou -> kód 57 (chyba interpretu IFJcode26)
            Přetečení 32 bitů interpret hlásí jako chybu testu (zadání ho netestuje)
  * Double: dělení nulou -> kód 57 (chování instrukce DIV)
  * subStr: chyba -> kód 40;  chr mimo 0..127 -> kód 58 (INT2CHAR);  ord mimo rozsah -> -1
  * readInt/readDouble/readLine podle zadání (bílé znaky se neignorují, chyba -> 0)
"""
import re
import sys
import threading


class IFJError(Exception):
    """Chyba v programu/testu (nemělo by nastat u platných programů)."""


class RunError(Exception):
    """Běhová chyba programu v IFJ26 -> návratový kód interpretu."""

    def __init__(self, code, msg=""):
        super().__init__(msg)
        self.code = code


class ReturnEx(Exception):
    def __init__(self, value):
        super().__init__()
        self.value = value


class _Unit:
    def __repr__(self):
        return "()"


UNIT = _Unit()

KEYWORDS = {"def", "do", "else", "if", "import", "return", "then", "val", "var", "while",
            "Int", "Double", "String", "Unit"}
TYPES = {"Int", "Double", "String", "Unit"}

NUM_RE = re.compile(
    r"(?P<dbl>(?:\d[\d_]*)?\.\d[\d_]*(?:[eE][+-]?\d[\d_]*)?[dD]?"
    r"|\d[\d_]*[eE][+-]?\d[\d_]*[dD]?)"
    r"|(?P<int>\d[\d_]*)")
ID_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
OPS2 = ("==", "!=", "<=", ">=")
OPS1 = "+-*/<>=:,.;()"
ESC = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"'}


# ------------------------------------------------------------------ lexer
def tokenize(src):
    """Vrátí seznam logických řádků [(odsazení, [tokeny])]."""
    n = len(src)
    i = 0
    depth = 0
    lines = []
    toks = []
    indent = 0
    pending = 0
    at_start = True

    def flush():
        nonlocal toks
        if toks:
            lines.append((indent, toks))
            toks = []

    def add(t):
        nonlocal indent
        if not toks:
            indent = pending
        toks.append(t)

    while i < n:
        c = src[i]
        if at_start and depth == 0:
            j = i
            while j < n and src[j] == " ":
                j += 1
            pending = j - i
            i = j
            at_start = False
            continue
        if c == "\n":
            i += 1
            if depth == 0:
                flush()
                at_start = True
            continue
        if c in " \t\r":
            i += 1
            continue
        if src.startswith("//", i):
            while i < n and src[i] != "\n":
                i += 1
            continue
        if src.startswith("/*", i):
            d = 1
            j = i + 2
            had_nl = False
            while j < n and d > 0:
                if src.startswith("/*", j):
                    d += 1
                    j += 2
                elif src.startswith("*/", j):
                    d -= 1
                    j += 2
                else:
                    if src[j] == "\n":
                        had_nl = True
                    j += 1
            if d > 0:
                raise IFJError("neukončený komentář")
            i = j
            if had_nl and depth == 0:
                flush()
            continue
        if c == '"':
            if src.startswith('"""', i):
                j = src.find('"""', i + 3)
                if j < 0:
                    raise IFJError("neukončený víceřádkový řetězec")
                add(("STR", src[i + 3:j]))
                i = j + 3
                continue
            j = i + 1
            buf = []
            while True:
                if j >= n or src[j] == "\n":
                    raise IFJError("neukončený řetězec")
                ch = src[j]
                if ch == '"':
                    break
                if ch == "\\":
                    j += 1
                    if j >= n or src[j] not in ESC:
                        raise IFJError("neplatná escape sekvence")
                    buf.append(ESC[src[j]])
                else:
                    buf.append(ch)
                j += 1
            add(("STR", "".join(buf)))
            i = j + 1
            continue
        if c.isdigit() or (c == "." and i + 1 < n and src[i + 1].isdigit()):
            m = NUM_RE.match(src, i)
            txt = m.group(0)
            if m.group("dbl"):
                add(("DBL", float(txt.replace("_", "").rstrip("dD"))))
            else:
                add(("INT", int(txt.replace("_", ""))))
            i = m.end()
            continue
        if c.isalpha() or c == "_":
            m = ID_RE.match(src, i)
            w = m.group(0)
            i = m.end()
            add(("KW" if w in KEYWORDS else "ID", w))
            continue
        if c == "@":
            if src.startswith("@main", i):
                add(("MAIN", "@main"))
                i += 5
                continue
            raise IFJError("neznámá anotace")
        if src[i:i + 2] in OPS2:
            add(("OP", src[i:i + 2]))
            i += 2
            continue
        if c in OPS1:
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            add(("OP", c))
            i += 1
            continue
        raise IFJError("neznámý znak %r" % c)
    flush()
    return lines


# ------------------------------------------------------------------ parser
class Cur:
    """Kurzor nad seznamem tokenů jednoho logického řádku."""

    def __init__(self, toks):
        self.t = toks
        self.p = 0

    def peek(self, k=0):
        return self.t[self.p + k] if self.p + k < len(self.t) else (None, None)

    def next(self):
        tok = self.peek()
        if tok[0] is None:
            raise IFJError("neočekávaný konec řádku")
        self.p += 1
        return tok

    def is_op(self, v, k=0):
        return self.peek(k) == ("OP", v)

    def is_kw(self, v):
        return self.peek() == ("KW", v)

    def expect_op(self, v):
        if not self.is_op(v):
            raise IFJError("očekáváno %r, nalezeno %r" % (v, self.peek()))
        self.p += 1

    def expect_kw(self, v):
        if not self.is_kw(v):
            raise IFJError("očekáváno klíčové slovo %r, nalezeno %r" % (v, self.peek()))
        self.p += 1

    def ident(self):
        k, v = self.next()
        if k != "ID":
            raise IFJError("očekáván identifikátor, nalezeno %r" % (v,))
        return v

    def type(self):
        k, v = self.next()
        if (k, v) == ("ID", "Boolean"):
            return v
        if k != "KW" or v not in TYPES:
            raise IFJError("očekáván typ, nalezeno %r" % (v,))
        return v

    def at_end(self):
        return self.p >= len(self.t)

    def rest(self):
        r = self.t[self.p:]
        self.p = len(self.t)
        return r


class ExprParser:
    def __init__(self, toks):
        self.c = Cur(toks)

    def parse(self):
        e = self.equality()
        if not self.c.at_end():
            raise IFJError("nadbytečné tokeny ve výrazu: %r" % (self.c.peek(),))
        return e

    def _binary(self, sub, ops):
        left = sub()
        while self.c.peek()[0] == "OP" and self.c.peek()[1] in ops:
            op = self.c.next()[1]
            left = ("bin", op, left, sub())
        return left

    def equality(self):
        return self._binary(self.relational, ("==", "!="))

    def relational(self):
        return self._binary(self.additive, ("<", "<=", ">", ">="))

    def additive(self):
        return self._binary(self.multiplicative, ("+", "-"))

    def multiplicative(self):
        return self._binary(self.unary, ("*", "/"))

    def unary(self):
        if self.c.is_op("-"):
            self.c.next()
            return ("neg", self.unary())
        if self.c.is_op("+"):
            self.c.next()
            return self.unary()
        return self.primary()

    def primary(self):
        k, v = self.c.next()
        if k in ("INT", "DBL", "STR"):
            return ("lit", v)
        if k == "ID":
            if self.c.is_op("("):
                args = []
                while self.c.is_op("("):
                    self.c.next()
                    while not self.c.is_op(")"):
                        args.append(self.equality())
                        if self.c.is_op(","):
                            self.c.next()
                    self.c.expect_op(")")
                return ("call", v, args)
            return ("var", v)
        if (k, v) == ("OP", "("):
            if self.c.is_op(")"):
                self.c.next()
                return ("lit", UNIT)
            e = self.equality()
            self.c.expect_op(")")
            return e
        raise IFJError("neočekávaný token ve výrazu: %r" % ((k, v),))


def parse_expr(toks):
    return ExprParser(toks).parse()


class Func:
    def __init__(self, name, params, ret, body, is_main):
        self.name, self.params, self.ret, self.body, self.is_main = name, params, ret, body, is_main


class Parser:
    def __init__(self, lines):
        self.lines = lines
        self.i = 0

    def program(self):
        if not self.lines or self.lines[0][1][:2] != [("KW", "import"), ("ID", "ifj26")]:
            raise IFJError("chybí prolog")
        self.i = 1
        funcs = {}
        main = None
        pending_main = False
        while self.i < len(self.lines):
            ind, toks = self.lines[self.i]
            if ind != 0:
                raise IFJError("odsazení na nejvyšší úrovni")
            if toks[0][0] == "MAIN":
                pending_main = True
                toks = toks[1:]
                if not toks:
                    self.i += 1
                    continue
            f = self.func(toks, pending_main)
            if f.name in funcs:
                raise IFJError("redefinice funkce " + f.name)
            funcs[f.name] = f
            if pending_main:
                if main is not None:
                    raise IFJError("více funkcí @main")
                main = f.name
                pending_main = False
        if main is None:
            raise IFJError("chybí @main")
        return funcs, main

    def func(self, toks, is_main):
        self.i += 1
        c = Cur(toks)
        c.expect_kw("def")
        name = c.ident()
        params = []
        while c.is_op("("):
            c.next()
            while not c.is_op(")"):
                pn = c.ident()
                c.expect_op(":")
                pt = c.type()
                params.append((pn, pt))
                if c.is_op(","):
                    c.next()
            c.expect_op(")")
        ret = None
        if c.is_op(":"):
            c.next()
            ret = c.type()
        c.expect_op("=")
        rest = c.rest()
        if rest:
            body = ("expr", parse_expr(rest))
        else:
            body = ("block", self.block(0))
        return Func(name, params, ret, body, is_main)

    def block(self, parent_indent):
        if self.i >= len(self.lines):
            raise IFJError("očekáván blok")
        ind = self.lines[self.i][0]
        if ind <= parent_indent:
            raise IFJError("blok musí být odsazen")
        stmts = []
        while self.i < len(self.lines) and self.lines[self.i][0] == ind:
            toks = self.lines[self.i][1]
            self.i += 1
            stmts.extend(self.line(toks, ind))
        if self.i < len(self.lines) and self.lines[self.i][0] > ind:
            raise IFJError("neočekávané odsazení")
        return stmts

    def line(self, toks, ind):
        if toks[0] == ("KW", "if"):
            return [self.if_stmt(toks, ind)]
        if toks[0] == ("KW", "while"):
            return [self.while_stmt(toks, ind)]
        parts, cur, depth = [], [], 0
        for t in toks:
            if t == ("OP", "("):
                depth += 1
            elif t == ("OP", ")"):
                depth -= 1
            if t == ("OP", ";") and depth == 0:
                parts.append(cur)
                cur = []
            else:
                cur.append(t)
        parts.append(cur)
        return [self.simple(p) for p in parts if p]

    def if_stmt(self, toks, ind):
        idx = toks.index(("KW", "then"))
        if idx != len(toks) - 1:
            raise IFJError("za then nesmí nic následovat (základní zadání)")
        cond = parse_expr(toks[1:idx])
        then_b = self.block(ind)
        else_b = []
        if (self.i < len(self.lines) and self.lines[self.i][0] == ind
                and self.lines[self.i][1][:1] == [("KW", "else")]):
            rest = self.lines[self.i][1][1:]
            self.i += 1
            if not rest:
                else_b = self.block(ind)
            elif rest[0] == ("KW", "if"):
                else_b = [self.if_stmt(rest, ind)]
            else:
                raise IFJError("za else nesmí nic následovat")
        return ("if", cond, then_b, else_b)

    def while_stmt(self, toks, ind):
        idx = toks.index(("KW", "do"))
        if idx != len(toks) - 1:
            raise IFJError("za do nesmí nic následovat (základní zadání)")
        return ("while", parse_expr(toks[1:idx]), self.block(ind))

    def simple(self, toks):
        c = Cur(toks)
        if c.is_kw("val") or c.is_kw("var"):
            mutable = c.next()[1] == "var"
            name = c.ident()
            typ = None
            if c.is_op(":"):
                c.next()
                typ = c.type()
            c.expect_op("=")
            return ("def", name, typ, parse_expr(c.rest()), mutable)
        if c.is_kw("return"):
            c.next()
            rest = c.rest()
            return ("return", parse_expr(rest) if rest else None)
        if c.peek()[0] == "ID" and c.is_op("=", 1):
            name = c.ident()
            c.next()
            return ("assign", name, parse_expr(c.rest()))
        return ("expr", parse_expr(toks))


# ------------------------------------------------------------------ vyhodnocení
def tname(v):
    if isinstance(v, bool):
        return "Boolean"
    if isinstance(v, int):
        return "Int"
    if isinstance(v, float):
        return "Double"
    if isinstance(v, str):
        return "String"
    if v is UNIT:
        return "Unit"
    raise IFJError("neznámý typ hodnoty %r" % (v,))


def coerce(v, t):
    """Jediná implicitní konverze: Int -> Double. Ostatní kombinace jsou chyba testu."""
    if t is None:
        return v
    if t == "Double" and isinstance(v, int) and not isinstance(v, bool):
        return float(v)
    if tname(v) != t:
        raise IFJError("typová chyba: hodnota %r není %s" % (v, t))
    return v


def is_num(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def check32(v):
    if not -2 ** 31 <= v < 2 ** 31:
        raise AssertionError("přetečení Int (zadání ho netestuje): %d" % v)
    return v


def fmt_a(x):
    """Totéž co printf("%a") v glibc."""
    h = x.hex()
    sign = ""
    if h.startswith("-"):
        sign, h = "-", h[1:]
    lead, frac, exp = re.fullmatch(r"0x([01])(?:\.([0-9a-f]+))?p([+-]\d+)", h).groups()
    frac = (frac or "").rstrip("0")
    return "%s0x%s%s" % (sign, lead + ("." + frac if frac else ""), "p" + exp)


class Env:
    def __init__(self, parent=None):
        self.vars = {}
        self.parent = parent

    def define(self, name, value, typ, mutable):
        if name in self.vars:
            raise IFJError("redefinice proměnné " + name)
        self.vars[name] = [value, typ, mutable]

    def lookup(self, name):
        e = self
        while e is not None:
            if name in e.vars:
                return e.vars[name]
            e = e.parent
        raise IFJError("nedefinovaná proměnná " + name)


class Interp:
    MAX_STEPS = 30_000_000

    def __init__(self, funcs, stdin):
        self.funcs = funcs
        self.out = []
        self.inp = stdin
        self.pos = 0
        self.steps = 0

    # ---- vstup
    def read_line(self):
        if self.pos >= len(self.inp):
            return ""
        j = self.inp.find("\n", self.pos)
        if j < 0:
            line, self.pos = self.inp[self.pos:], len(self.inp)
        else:
            line, self.pos = self.inp[self.pos:j + 1], j + 1
        return line

    def read_int(self):
        s = self.read_line()
        if s.endswith("\n"):
            s = s[:-1]
        if not re.fullmatch(r"-?\d+", s):
            if re.fullmatch(r"[+]\d+", s):
                raise AssertionError("nejednoznačný vstup pro readInt: %r" % s)
            return 0
        if re.fullmatch(r"-?0\d+", s):
            raise AssertionError("nejednoznačný vstup pro readInt: %r" % s)
        v = int(s)
        if abs(v) >= 2 ** 63:
            return 0
        if not -2 ** 31 <= v < 2 ** 31:
            raise AssertionError("nejednoznačný vstup pro readInt: %r" % s)
        return v

    def read_double(self):
        s = self.read_line()
        if s.endswith("\n"):
            s = s[:-1]
        if re.fullmatch(r"-?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?", s):
            if s.rstrip("0123456789").endswith(".") and not re.search(r"\.\d", s):
                raise AssertionError("nejednoznačný vstup pro readDouble: %r" % s)
            return float(s)
        return 0.0

    # ---- výrazy
    def eval(self, e, env):
        k = e[0]
        if k == "lit":
            return e[1]
        if k == "var":
            try:
                return env.lookup(e[1])[0]
            except IFJError:
                if e[1] in self.funcs:
                    return self.call(e[1], [])
                raise
        if k == "neg":
            v = self.eval(e[1], env)
            if not is_num(v):
                raise IFJError("unární minus na ne-čísle")
            return check32(-v) if isinstance(v, int) else -v
        if k == "bin":
            return self.binop(e[1], self.eval(e[2], env), self.eval(e[3], env))
        if k == "call":
            args = [self.eval(a, env) for a in e[2]]
            return self.call(e[1], args)
        raise IFJError("neznámý uzel " + k)

    def binop(self, op, a, b):
        if op in ("+", "-", "*", "/"):
            if op == "+" and isinstance(a, str) and isinstance(b, str):
                return a + b
            if not (is_num(a) and is_num(b)):
                raise IFJError("typová chyba v operátoru %s: %r %r" % (op, a, b))
            both_int = isinstance(a, int) and isinstance(b, int)
            if op == "/":
                if both_int:
                    if b == 0:
                        raise RunError(57, "dělení nulou (Int)")
                    q = abs(a) // abs(b)
                    return check32(-q if (a < 0) != (b < 0) else q)
                if float(b) == 0.0:
                    raise RunError(57, "dělení nulou (Double)")
                return float(a) / float(b)
            fn = {"+": lambda x, y: x + y, "-": lambda x, y: x - y, "*": lambda x, y: x * y}[op]
            if both_int:
                return check32(fn(a, b))
            return fn(float(a), float(b))
        if op in ("<", "<=", ">", ">="):
            if not ((is_num(a) and is_num(b)) or (isinstance(a, str) and isinstance(b, str))):
                raise IFJError("typová chyba v porovnání %s: %r %r" % (op, a, b))
            return {"<": a < b, "<=": a <= b, ">": a > b, ">=": a >= b}[op]
        if op in ("==", "!="):
            if not ((is_num(a) and is_num(b)) or (type(a) is type(b))):
                raise IFJError("typová chyba v porovnání %s: %r %r" % (op, a, b))
            return (a == b) if op == "==" else (a != b)
        raise IFJError("neznámý operátor " + op)

    # ---- volání funkcí
    def call(self, name, args):
        self.steps += 1
        if self.steps > self.MAX_STEPS:
            raise IFJError("příliš mnoho kroků (nekonečný cyklus?)")
        if name in self.funcs:
            return self.call_user(self.funcs[name], args)
        return self.builtin(name, args)

    def call_user(self, f, args):
        if len(args) != len(f.params):
            raise IFJError("špatný počet argumentů při volání " + f.name)
        env = Env()
        for (pn, pt), a in zip(f.params, args):
            if pt == "Double" and isinstance(a, int):
                raise IFJError("Int předaný jako Double argument je v testech zakázán (nejednoznačné)")
            env.define(pn, coerce(a, pt), pt, False)
        try:
            if f.body[0] == "expr":
                v = self.eval(f.body[1], env)
            else:
                v = self.run_block(f.body[1], env)
        except ReturnEx as r:
            v = r.value
        if f.ret == "Unit":
            return UNIT
        return coerce(v, f.ret)

    def builtin(self, name, a):
        def need(n, *types):
            if len(a) != n:
                raise IFJError("%s: špatný počet argumentů" % name)
            for v, t in zip(a, types):
                if t != "Any" and tname(v) != t:
                    raise IFJError("%s: špatný typ argumentu %r" % (name, v))

        if name == "print":
            need(1, "Any")
            v = a[0]
            t = tname(v)
            self.out.append(fmt_a(v) if t == "Double" else ("()" if t == "Unit" else
                            str(v).lower() if t == "Boolean" else str(v)))
            return UNIT
        if name == "readLine":
            need(0)
            return self.read_line()
        if name == "readInt":
            need(0)
            return self.read_int()
        if name == "readDouble":
            need(0)
            return self.read_double()
        if name == "toStr":
            need(1, "Any")
            v = a[0]
            t = tname(v)
            if t == "Double":
                if v == int(v):
                    raise AssertionError("toStr celého Double je nejednoznačné: %r" % v)
                return "%.2f" % v
            if t == "Boolean":
                return "true" if v else "false"
            return "()" if t == "Unit" else str(v)
        if name == "toInt":
            need(1, "Double")
            return int(a[0])
        if name == "strLength":
            need(1, "String")
            return len(a[0])
        if name == "subStr":
            need(3, "String", "Int", "Int")
            s, st, en = a
            if st < 0 or en < 0 or st > en or st >= len(s) or en > len(s):
                raise RunError(40, "subStr mimo rozsah")
            return s[st:en]
        if name == "ord":
            need(2, "String", "Int")
            s, i = a
            return ord(s[i]) if 0 <= i < len(s) else -1
        if name == "chr":
            need(1, "Int")
            if not 0 <= a[0] <= 127:
                raise RunError(58, "chr mimo 0..127")
            return chr(a[0])
        raise IFJError("neznámá funkce " + name)

    # ---- příkazy
    def run_block(self, stmts, env):
        scope = Env(env)
        last = UNIT
        for s in stmts:
            last = self.stmt(s, scope)
        return last

    def stmt(self, s, env):
        k = s[0]
        if k == "def":
            _, name, typ, expr, mutable = s
            v = self.eval(expr, env)
            t = typ if typ is not None else tname(v)
            env.define(name, coerce(v, t), t, mutable)
            return UNIT
        if k == "assign":
            var = env.lookup(s[1])
            if not var[2]:
                raise IFJError("přiřazení do nemodifikovatelné proměnné " + s[1])
            var[0] = coerce(self.eval(s[2], env), var[1])
            return UNIT
        if k == "if":
            cond = self.eval(s[1], env)
            if not isinstance(cond, bool):
                raise IFJError("podmínka není Boolean")
            self.run_block(s[2] if cond else s[3], env)
            return UNIT
        if k == "while":
            while True:
                self.steps += 1
                if self.steps > self.MAX_STEPS:
                    raise IFJError("příliš mnoho kroků (nekonečný cyklus?)")
                cond = self.eval(s[1], env)
                if not isinstance(cond, bool):
                    raise IFJError("podmínka není Boolean")
                if not cond:
                    return UNIT
                self.run_block(s[2], env)
        if k == "return":
            raise ReturnEx(UNIT if s[1] is None else self.eval(s[1], env))
        if k == "expr":
            return self.eval(s[1], env)
        raise IFJError("neznámý příkaz " + k)


def _run(src, stdin):
    funcs, main = Parser(tokenize(src)).program()
    it = Interp(funcs, stdin)
    try:
        it.call_user(funcs[main], [])
        rc = 0
    except RunError as e:
        rc = e.code
    return "".join(it.out), rc


def run_program(src, stdin=""):
    """Vrátí (stdout, návratový_kód). Výjimky IFJError/AssertionError znamenají chybu testu."""
    if isinstance(src, bytes):
        src = src.decode("latin-1")
    if isinstance(stdin, bytes):
        stdin = stdin.decode("latin-1")
    box = {}

    def target():
        try:
            box["v"] = _run(src, stdin)
        except BaseException as e:  # noqa: BLE001
            box["e"] = e

    sys.setrecursionlimit(2_000_000)
    for size in (512, 256, 64):
        try:
            threading.stack_size(size * 1024 * 1024)
            break
        except (ValueError, RuntimeError):
            continue
    th = threading.Thread(target=target)
    th.start()
    th.join()
    if "e" in box:
        raise box["e"]
    return box["v"]


if __name__ == "__main__":
    out, rc = run_program(open(sys.argv[1], "rb").read(), sys.stdin.buffer.read())
    sys.stdout.write(out)
    sys.exit(rc)
