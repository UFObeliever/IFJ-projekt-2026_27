#!/usr/bin/env python3
"""
ic_mini.py - NEOFICIÁLNÍ minimální interpret IFJcode26 (jen to, co generuje
generator.c). Slouží jako záloha, když nemáte po ruce ic26int.
Rozhodující je vždy oficiální ic26int!

Použití:  ic_mini.py [--strict] [--max-steps N] prog.code < in > out

--strict : na konci programu (EXIT 0) musí být prázdný datový zásobník
           a zásobník rámců, jinak končí s kódem 97 (únik zásobníku).
Návratové kódy odpovídají ic26int (52-58); 97 = strict, 98 = limit kroků.
"""
import re
import sys

UNINIT = object()


class Err(Exception):
    def __init__(self, code, msg=""):
        super().__init__(msg)
        self.code = code


def c_hex(x):
    """printf("%a") jako v glibc."""
    if x != x:
        return "nan"
    if x in (float("inf"), float("-inf")):
        return "inf" if x > 0 else "-inf"
    m, e = x.hex().split("p")
    if "." in m:
        m = m.rstrip("0").rstrip(".")
    return m + "p" + e


def decode_str(s):
    return re.sub(r"\\(\d{3})", lambda m: chr(int(m.group(1))), s)


def is_var(tok):
    return tok[:3] in ("GF@", "LF@", "TF@")


def parse_const(tok):
    t, _, v = tok.partition("@")
    if t == "int":
        return int(v)
    if t == "float":
        return float.fromhex(v)
    if t == "bool":
        return v == "true"
    if t == "nil":
        return None
    if t == "string":
        return decode_str(v)
    raise Err(51, "bad constant " + tok)


class VM:
    def __init__(self, lines, stdin, strict, max_steps):
        self.prog = []
        self.labels = {}
        self.strict = strict
        self.max_steps = max_steps
        self.stdin = stdin
        self.out = []
        for ln in lines:
            ln = ln.split("#", 1)[0].strip()
            if not ln or ln == ".IFJcode26":
                continue
            parts = ln.split()
            op = parts[0].upper()
            if op == "LABEL":
                if parts[1] in self.labels:
                    raise Err(52, "label redefined " + parts[1])
                self.labels[parts[1]] = len(self.prog)
            self.prog.append((op, parts[1:]))
        self.gf = {}
        self.frames = []
        self.tf = None
        self.stack = []
        self.calls = []

    # --- proměnné ---
    def frame(self, name):
        if name == "GF":
            return self.gf
        if name == "LF":
            if not self.frames:
                raise Err(55, "no LF")
            return self.frames[-1]
        if self.tf is None:
            raise Err(55, "no TF")
        return self.tf

    def get(self, tok):
        if not is_var(tok):
            return parse_const(tok)
        f, _, n = tok.partition("@")
        fr = self.frame(f)
        if n not in fr:
            raise Err(54, "undefined var " + tok)
        v = fr[n]
        if v is UNINIT:
            raise Err(56, "uninitialized " + tok)
        return v

    def set(self, tok, val):
        f, _, n = tok.partition("@")
        fr = self.frame(f)
        if n not in fr:
            raise Err(54, "undefined var " + tok)
        fr[n] = val

    def pop(self):
        if not self.stack:
            raise Err(56, "empty data stack")
        return self.stack.pop()

    def jump(self, lab):
        if lab not in self.labels:
            raise Err(52, "no label " + lab)
        return self.labels[lab]

    # --- pomocné typové kontroly ---
    @staticmethod
    def tn(v):
        if v is None:
            return "nil"
        if type(v) is bool:
            return "bool"
        if type(v) is int:
            return "int"
        if type(v) is float:
            return "float"
        return "string"

    def arith(self, op, a, b):
        ta, tb = self.tn(a), self.tn(b)
        if ta != tb or ta not in ("int", "float"):
            raise Err(53, "arith types %s %s" % (ta, tb))
        if op == "ADD":
            return a + b
        if op == "SUB":
            return a - b
        if op == "MUL":
            return a * b
        if op == "IDIV":
            if ta != "int":
                raise Err(53)
            if b == 0:
                raise Err(57, "idiv by zero")
            return a // b  # zaokrouhlení k -inf (jako IDIV)
        if op == "DIV":
            if ta != "float":
                raise Err(53)
            if b == 0:
                raise Err(57, "div by zero")
            return a / b
        raise Err(53)

    def rel(self, op, a, b):
        ta, tb = self.tn(a), self.tn(b)
        if op == "EQ":
            if ta != tb and "nil" not in (ta, tb):
                raise Err(53, "EQ types")
            return a == b
        if ta != tb or ta == "nil":
            raise Err(53, "rel types %s %s" % (ta, tb))
        return a < b if op == "LT" else a > b

    def logic(self, op, a, b=None):
        if self.tn(a) != "bool" or (op != "NOT" and self.tn(b) != "bool"):
            raise Err(53)
        if op == "AND":
            return a and b
        if op == "OR":
            return a or b
        return not a

    def read(self, typ):
        line = self.stdin.readline()
        if typ == "string":
            return line  # včetně LF, na EOF ""
        if line.endswith("\n"):
            line = line[:-1]
        if typ == "int":
            return int(line) if re.fullmatch(r"-?\d+", line) else 0
        if typ == "float":
            ok = re.fullmatch(
                r"[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?", line)
            return float(line) if ok else 0.0
        if typ == "bool":
            return line.lower() == "true"
        raise Err(53)

    def write(self, v):
        t = self.tn(v)
        if t == "int":
            self.out.append(str(v))
        elif t == "float":
            self.out.append(c_hex(v))
        elif t == "bool":
            self.out.append("true" if v else "false")
        elif t == "string":
            self.out.append(v)
        else:
            self.out.append("")

    # --- hlavní smyčka ---
    def run(self):
        pc, steps = 0, 0
        prog = self.prog
        while pc < len(prog):
            steps += 1
            if steps > self.max_steps:
                raise Err(98, "step limit")
            op, a = prog[pc]
            pc += 1
            if op == "LABEL":
                continue
            elif op == "DEFVAR":
                f, _, n = a[0].partition("@")
                fr = self.frame(f)
                if n in fr:
                    raise Err(52, "redefinition of " + a[0])
                fr[n] = UNINIT
            elif op == "MOVE":
                self.set(a[0], self.get(a[1]))
            elif op == "CREATEFRAME":
                self.tf = {}
            elif op == "PUSHFRAME":
                if self.tf is None:
                    raise Err(55, "PUSHFRAME without TF")
                self.frames.append(self.tf)
                self.tf = None
            elif op == "POPFRAME":
                if not self.frames:
                    raise Err(55, "POPFRAME without LF")
                self.tf = self.frames.pop()
            elif op == "CALL":
                self.calls.append(pc)
                pc = self.jump(a[0])
            elif op == "RETURN":
                if not self.calls:
                    raise Err(56, "RETURN without CALL")
                pc = self.calls.pop()
            elif op == "PUSHS":
                self.stack.append(self.get(a[0]))
            elif op == "POPS":
                self.set(a[0], self.pop())
            elif op == "CLEARS":
                self.stack.clear()
            elif op in ("ADD", "SUB", "MUL", "DIV", "IDIV"):
                self.set(a[0], self.arith(op, self.get(a[1]), self.get(a[2])))
            elif op in ("ADDS", "SUBS", "MULS", "DIVS", "IDIVS"):
                y, x = self.pop(), self.pop()
                self.stack.append(self.arith(op[:-1], x, y))
            elif op in ("LT", "GT", "EQ"):
                self.set(a[0], self.rel(op, self.get(a[1]), self.get(a[2])))
            elif op in ("LTS", "GTS", "EQS"):
                y, x = self.pop(), self.pop()
                self.stack.append(self.rel(op[:-1], x, y))
            elif op in ("AND", "OR"):
                self.set(a[0], self.logic(op, self.get(a[1]), self.get(a[2])))
            elif op == "NOT":
                self.set(a[0], self.logic("NOT", self.get(a[1])))
            elif op in ("ANDS", "ORS"):
                y, x = self.pop(), self.pop()
                self.stack.append(self.logic(op[:-1], x, y))
            elif op == "NOTS":
                self.stack.append(self.logic("NOT", self.pop()))
            elif op in ("INT2FLOAT", "INT2FLOATS"):
                v = self.get(a[1]) if op == "INT2FLOAT" else self.pop()
                if self.tn(v) != "int":
                    raise Err(53)
                self.put(op, a, float(v))
            elif op in ("FLOAT2INT", "FLOAT2INTS"):
                v = self.get(a[1]) if op == "FLOAT2INT" else self.pop()
                if self.tn(v) != "float":
                    raise Err(53)
                self.put(op, a, int(v))
            elif op in ("INT2CHAR", "INT2CHARS"):
                v = self.get(a[1]) if op == "INT2CHAR" else self.pop()
                if self.tn(v) != "int":
                    raise Err(53)
                if not 0 <= v <= 127:
                    raise Err(58, "INT2CHAR range")
                self.put(op, a, chr(v))
            elif op in ("INT2STR", "INT2STRS"):
                v = self.get(a[1]) if op == "INT2STR" else self.pop()
                if self.tn(v) != "int":
                    raise Err(53)
                self.put(op, a, str(v))
            elif op in ("FLOAT2STR", "FLOAT2STRS"):
                v = self.get(a[1]) if op == "FLOAT2STR" else self.pop()
                if self.tn(v) != "float":
                    raise Err(53)
                self.put(op, a, "%.2f" % v)
            elif op == "CONCAT":
                x, y = self.get(a[1]), self.get(a[2])
                if self.tn(x) != "string" or self.tn(y) != "string":
                    raise Err(53)
                self.set(a[0], x + y)
            elif op == "STRLEN":
                x = self.get(a[1])
                if self.tn(x) != "string":
                    raise Err(53)
                self.set(a[0], len(x))
            elif op in ("GETCHAR", "STRI2INT"):
                s, i = self.get(a[1]), self.get(a[2])
                if self.tn(s) != "string" or self.tn(i) != "int":
                    raise Err(53)
                if not 0 <= i < len(s):
                    raise Err(58, op + " index")
                self.set(a[0], s[i] if op == "GETCHAR" else ord(s[i]))
            elif op == "READ":
                self.set(a[0], self.read(a[1]))
            elif op == "WRITE":
                self.write(self.get(a[0]))
            elif op == "JUMP":
                pc = self.jump(a[0])
            elif op in ("JUMPIFEQ", "JUMPIFNEQ"):
                x, y = self.get(a[1]), self.get(a[2])
                tx, ty = self.tn(x), self.tn(y)
                if tx != ty and "nil" not in (tx, ty):
                    raise Err(53, "JUMPIF types %s %s" % (tx, ty))
                if (x == y) == (op == "JUMPIFEQ"):
                    pc = self.jump(a[0])
            elif op in ("JUMPIFEQS", "JUMPIFNEQS"):
                y, x = self.pop(), self.pop()
                tx, ty = self.tn(x), self.tn(y)
                if tx != ty and "nil" not in (tx, ty):
                    raise Err(53, "JUMPIFS types %s %s" % (tx, ty))
                if (x == y) == (op == "JUMPIFEQS"):
                    pc = self.jump(a[0])
            elif op == "EXIT":
                v = self.get(a[0])
                if self.tn(v) != "int":
                    raise Err(53)
                if not 0 <= v <= 49:
                    raise Err(57, "bad EXIT code")
                if v == 0 and self.strict:
                    if self.stack:
                        raise Err(97, "data stack not empty at exit: %r"
                                  % (self.stack[-5:],))
                    if self.frames:
                        raise Err(97, "frame stack not empty at exit")
                return v
            elif op in ("BREAK", "DPRINT", "TYPE"):
                pass
            else:
                raise Err(51, "unsupported instruction " + op)
        return 0

    def put(self, op, a, val):
        if op.endswith("S"):
            self.stack.append(val)
        else:
            self.set(a[0], val)


def main():
    args = sys.argv[1:]
    strict, max_steps, path = False, 30_000_000, None
    i = 0
    while i < len(args):
        if args[i] == "--strict":
            strict = True
        elif args[i] == "--max-steps":
            i += 1
            max_steps = int(args[i])
        else:
            path = args[i]
        i += 1
    if path is None:
        print(__doc__, file=sys.stderr)
        return 50
    with open(path, encoding="latin-1") as f:
        lines = f.read().split("\n")
    if not lines or lines[0].strip() != ".IFJcode26":
        print("missing .IFJcode26 header", file=sys.stderr)
        return 51
    stdin = open(sys.stdin.fileno(), encoding="latin-1", newline="",
                 closefd=False)
    rc = 0
    vm = None
    try:
        vm = VM(lines, stdin, strict, max_steps)
        rc = vm.run()
    except Err as e:
        rc = e.code
        print("ic_mini: error %d: %s" % (e.code, e), file=sys.stderr)
    except (ValueError, IndexError) as e:
        rc = 51
        print("ic_mini: parse error: %s" % e, file=sys.stderr)
    if vm is not None:
        sys.stdout.buffer.write("".join(vm.out).encode("latin-1"))
        sys.stdout.flush()
    return rc


if __name__ == "__main__":
    sys.exit(main())
