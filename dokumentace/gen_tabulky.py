#!/usr/bin/env python3
"""
gen_tabulky.py - vygeneruje LaTeXové tabulky pro dokumentaci z jediné definice.

  gramatika.tex       číslovaná LL gramatika (pravidla 1-70)
  ll_tabulka.tex      LL tabulka (množiny FIRST/FOLLOW se spočítají sami)
  first_follow.tex    množiny FIRST a FOLLOW (volitelné, do dokumentu se nevkládá)
  psa_gramatika.tex   gramatika pro precedenční analýzu
  prec_tabulka.tex    precedenční tabulka
  prec_priority.tex   priority a asociativita operátorů

Spuštění:  python3 gen_tabulky.py
Konflikty LL tabulky (buňky s víc pravidly) se vypíšou na stderr a v tabulce
se označí červeně (makro \\cf). Gramatiku upravte v seznamu RULES tak,
aby odpovídala parseru: čísla pravidel v tabulce se přepočítají sama.

Tato gramatika byla zrekonstruována z LL tabulky týmu a její spočtená tabulka
se shoduje se všemi 131 buňkami původní tabulky (včetně 4 konfliktů).
Neterminály def/extension/else se v kódu jmenují def_head/ext_head/else_part,
aby se nepletly se stejnojmennými terminály.
"""
import sys

EPS = "eps"
R = lambda lhs, *rhs: (lhs, list(rhs) if rhs else [EPS])

# --------------------------------------------------------------------------
# 1) LL gramatika (jazyk IFJ26 včetně rozšíření)
#    INDENT/DEDENT: virtuální terminály (parser vychází z odsazení)
#    expr: neterminál zpracovaný precedenční analýzou (viz EXPR_FIRST)
# --------------------------------------------------------------------------
RULES = [
    R("program",     "eols", "import", "ifj26", ".", "*", "fun_list"),        # 1
    R("eols",        "EOL", "eols"),                                          # 2
    R("eols"),                                                                # 3
    R("fun_list",    "fun_def", "fun_list"),                                  # 4
    R("fun_list",    "ext_head", "fun_list"),                                 # 5
    R("fun_list",    "EOL", "fun_list"),                                      # 6
    R("fun_list"),                                                            # 7
    R("fun_def",     "@main", "eols", "def_head"),                            # 8
    R("fun_def",     "def_head"),                                             # 9
    R("def_head",    "def", "id", "param_lists", "ret_type", "=", "fun_body"),# 10
    R("param_lists", "(", "params", ")", "param_lists"),                      # 11
    R("param_lists"),                                                         # 12
    R("params",      "param", "params_n"),                                    # 13
    R("params"),                                                              # 14
    R("params_n",    ",", "param", "params_n"),                               # 15
    R("params_n"),                                                            # 16
    R("param",       "id", ":", "type"),                                      # 17
    R("ret_type",    ":", "type"),                                            # 18
    R("ret_type"),                                                            # 19
    R("type", "Int"), R("type", "Double"), R("type", "String"),               # 20-22
    R("type", "Unit"), R("type", "Boolean"),                                  # 23-24
    R("fun_body",    "expr"),                                                 # 25
    R("fun_body",    "EOL", "block"),                                         # 26
    R("ext_head",    "extension", "(", "id", ":", "type", ")", "EOL",
                     "INDENT", "ext_defs", "DEDENT"),                         # 27
    R("ext_defs",    "EOL", "ext_defs"),                                      # 28
    R("ext_defs",    "def_head", "ext_defs"),                                 # 29
    R("ext_defs"),                                                            # 30
    R("block",       "EOL", "INDENT", "stmt", "stmt_list", "DEDENT"),         # 31
    R("stmt_list",   "EOL", "stmt_rest"),                                     # 32
    R("stmt_list",   ";", "stmt_rest"),                                       # 33
    R("stmt_list"),                                                           # 34
    R("stmt_rest",   "stmt", "stmt_list"),                                    # 35
    R("stmt_rest",   "EOL", "stmt_rest"),                                     # 36
    R("stmt_rest"),                                                           # 37
    R("stmt",        "val", "id", "opt_type", "=", "expr"),                   # 38
    R("stmt",        "var", "id", "opt_type", "=", "expr"),                   # 39
    R("stmt",        "id", "=", "expr"),                                      # 40
    R("stmt",        "expr"),                                                 # 41
    R("stmt",        "return", "ret_val"),                                    # 42
    R("stmt",        "if", "expr", "then", "body", "else_part"),              # 43
    R("stmt",        "while", "expr", "do", "body"),                          # 44
    R("stmt",        "for", "for_head", "do", "body"),                        # 45
    R("opt_type",    ":", "type"),                                            # 46
    R("opt_type"),                                                            # 47
    R("ret_val",     "expr"),                                                 # 48
    R("ret_val"),                                                             # 49
    R("else_part",   "else", "body"),                                         # 50
    R("else_part",   "EOL", "else", "body"),                                  # 51
    R("else_part"),                                                           # 52
    R("body",        "block"),                                                # 53
    R("body",        "stmt"),                                                 # 54
    R("for_head",    "generator", "step", "guards"),                          # 55
    R("for_head",    "EOL", "INDENT", "generator", "step", "guards_ml",
                     "DEDENT"),                                               # 56
    R("generator",   "id", "<-", "expr", "range", "expr"),                    # 57
    R("range", "to"), R("range", "until"),                                    # 58-59
    R("step",        "by", "expr"),                                           # 60
    R("step"),                                                                # 61
    R("guards",      "if", "expr", "guards"),                                 # 62
    R("guards"),                                                              # 63
    R("guards_ml",   "EOL", "if", "expr", "guards_ml"),                       # 64
    R("guards_ml"),                                                           # 65
    R("if_expr",     "if", "expr", "then", "branch", "else_e"),               # 66
    R("else_e",      "else", "branch"),                                       # 67
    R("else_e",      "EOL", "else", "branch"),                                # 68
    R("branch",      "expr"),                                                 # 69
    R("branch",      "block"),                                                # 70
]

EXPR_FIRST = {"id", "literal", "true", "false", "(", "-", "!", "if"}
START = "program"
NO_ROW = {"expr"}     # neterminál bez řádku v LL tabulce (řeší ho PSA)

TERM_ORDER = ["import", "ifj26", ".", "*", "@main", "def", "extension", "id",
              "(", ")", ",", ":", "=", "<-", ";", "Int", "Double", "String",
              "Unit", "Boolean", "literal", "true", "false", "-", "!", "val",
              "var", "return", "if", "then", "else", "while", "do", "for",
              "to", "until", "by", "INDENT", "DEDENT", "EOL", "EOF"]
DROP_EMPTY_COLUMNS = True   # vynechat sloupce, které nejsou v žádné buňce


def nonterminals():
    nts = []
    for l, _ in RULES:
        if l not in nts:
            nts.append(l)
    if "expr" not in nts:
        nts.append("expr")
    return nts


NTS = nonterminals()


def is_nt(s):
    return s in NTS


def first_of_seq(seq, FIRST):
    out = set()
    for s in seq:
        if s == EPS:
            continue
        if is_nt(s):
            out |= FIRST[s] - {EPS}
            if EPS not in FIRST[s]:
                return out
        else:
            out.add(s)
            return out
    out.add(EPS)
    return out


def compute_first():
    FIRST = {n: set() for n in NTS}
    FIRST["expr"] = set(EXPR_FIRST)
    changed = True
    while changed:
        changed = False
        for l, r in RULES:
            f = first_of_seq(r, FIRST)
            if not f <= FIRST[l]:
                FIRST[l] |= f
                changed = True
    return FIRST


def compute_follow(FIRST):
    FOLLOW = {n: set() for n in NTS}
    FOLLOW[START].add("$")
    changed = True
    while changed:
        changed = False
        for l, r in RULES:
            for i, s in enumerate(r):
                if not is_nt(s):
                    continue
                rest = first_of_seq(r[i + 1:], FIRST)
                add = rest - {EPS}
                if EPS in rest:
                    add |= FOLLOW[l]
                if not add <= FOLLOW[s]:
                    FOLLOW[s] |= add
                    changed = True
    return FOLLOW


# --------------------------------------------------------------------------
# LaTeX pomocné funkce
# --------------------------------------------------------------------------
ITALIC = {"id", "literal", "EOL", "EOF", "INDENT", "DEDENT"}


def esc(s):
    return s.replace("_", r"\_")


def tex_term(s):
    if s == EPS:
        return r"$\varepsilon$"
    if s == "$":
        return r"\texttt{\$}"
    if s in ITALIC:
        return r"\textit{%s}" % s
    if s == "<-":
        return r"\texttt{\textless-}"
    if s == "&&":
        return r"\texttt{\&\&}"
    if s == "||":
        return r"\texttt{\textbar\textbar}"
    return r"\texttt{%s}" % esc(s)


def tex_sym(s):
    if s == EPS:
        return tex_term(s)
    if is_nt(s):
        return r"\nt{%s}" % esc(s)
    return tex_term(s)


def write_grammar(path):
    lines = [r"% Vygenerováno skriptem gen_tabulky.py, needitujte ručně.",
             r"{\renewcommand{\arraystretch}{1.12}\small",
             r"\begin{longtable}{@{}r@{\hspace{8pt}}r@{\hspace{5pt}}c@{\hspace{5pt}}l@{}}"]
    for i, (l, r) in enumerate(RULES, 1):
        rhs = " ".join(tex_sym(s) for s in r)
        lines.append(r"%d. & %s & $\longrightarrow$ & %s \\" % (i, tex_sym(l), rhs))
    lines.append(r"\end{longtable}}")
    open(path, "w", encoding="utf-8").write("\n".join(lines) + "\n")


def build_table(FIRST, FOLLOW):
    table = {}
    for n, (l, r) in enumerate(RULES, 1):
        f = first_of_seq(r, FIRST)
        sel = set(f - {EPS})
        if EPS in f:
            sel |= FOLLOW[l]
        for t in sel:
            t = "EOF" if t == "$" else t
            table.setdefault((l, t), []).append(str(n))
    return table


def write_ll(path, FIRST, FOLLOW):
    table = build_table(FIRST, FOLLOW)
    conflicts = {k: v for k, v in table.items() if len(v) > 1}
    for (nt, t), v in sorted(conflicts.items()):
        print("LL konflikt: M[%s, %s] = %s" % (nt, t, "/".join(v)),
              file=sys.stderr)

    used = {t for (_, t) in table}
    missing = sorted(used - set(TERM_ORDER))
    if missing:
        print("POZOR: terminály chybí v TERM_ORDER: %s" % missing,
              file=sys.stderr)
    cols = [t for t in TERM_ORDER if (t in used or not DROP_EMPTY_COLUMNS)]
    dropped = [t for t in TERM_ORDER if t not in cols]
    if dropped:
        print("vynechané prázdné sloupce: %s" % ", ".join(dropped),
              file=sys.stderr)
    L = [r"% Vygenerováno skriptem gen_tabulky.py, needitujte ručně.",
         r"{\scriptsize\setlength{\tabcolsep}{2.2pt}",
         r"\renewcommand{\arraystretch}{1.08}",
         r"\begin{tabular}{|l|" + "c|" * len(cols) + "}", r"\hline"]
    head = " & ".join(r"\rotatebox{90}{%s\hspace{2pt}}" % tex_term(t)
                      for t in cols)
    L.append(r"& %s \\ \hline" % head)
    for nt in NTS:
        if nt in NO_ROW:
            continue
        cells = []
        for t in cols:
            v = table.get((nt, t))
            if not v:
                cells.append("")
            elif len(v) > 1:
                cells.append(r"\cf{%s}" % "/".join(v))
            else:
                cells.append(v[0])
        L.append(r"%s & %s \\ \hline" % (tex_sym(nt), " & ".join(cells)))
    L.append(r"\end{tabular}}")
    open(path, "w", encoding="utf-8").write("\n".join(L) + "\n")


def write_first_follow(path, FIRST, FOLLOW):
    L = [r"% Vygenerováno skriptem gen_tabulky.py, needitujte ručně.",
         r"{\small\renewcommand{\arraystretch}{1.15}",
         r"\begin{longtable}{@{}l p{5.8cm} p{5.8cm}@{}}",
         r"\toprule Neterminál & FIRST & FOLLOW \\ \midrule \endhead"]
    for nt in NTS:
        def fmt(s):
            items = sorted(s - {"$"}, key=lambda x: (x == EPS, x))
            txt = ", ".join(tex_term(x) for x in items)
            if "$" in s:
                txt += (", " if txt else "") + tex_term("EOF")
            return txt or "--"
        L.append(r"%s & %s & %s \\" % (tex_sym(nt), fmt(FIRST[nt]),
                                       fmt(FOLLOW[nt])))
    L.append(r"\bottomrule\end{longtable}}")
    open(path, "w", encoding="utf-8").write("\n".join(L) + "\n")


# --------------------------------------------------------------------------
# 2) Gramatika a tabulka precedenční analýzy (priority podle zadání, kap. 3
#    a rozšíření BOOLTHEN; 1 = nejsilněji se váže)
# --------------------------------------------------------------------------
PSA_RULES = [
    "! E", "- E", "+ E",
    "E * E", "E / E", "E + E", "E - E",
    "E < E", "E <= E", "E > E", "E >= E", "E == E", "E != E",
    "E && E", "E || E",
    "( E )", "i", "f ( )", "f ( E )", "f ( E , \\ldots )",
]

PREC_CLASSES = [
    # symbol, TeX, úroveň, asociativita ('L','R','N'), prefix?
    ("u",   r"\texttt{+}$_{\mathrm{un}}$ \texttt{-}$_{\mathrm{un}}$ \texttt{!}", 1, "R", True),
    ("mul", r"\texttt{*}\ \texttt{/}",                       2, "L", False),
    ("add", r"\texttt{+}\ \texttt{-}",                       3, "L", False),
    ("rel", r"\texttt{<}\ \texttt{<=}\ \texttt{>}\ \texttt{>=}", 4, "N", False),
    ("eq",  r"\texttt{==}\ \texttt{!=}",                     5, "N", False),
    ("and", r"\texttt{\&\&}",                                6, "L", False),
    ("or",  r"\texttt{\textbar\textbar}",                    7, "L", False),
]


def write_psa_grammar(path):
    L = [r"% Vygenerováno skriptem gen_tabulky.py, needitujte ručně.",
         r"{\renewcommand{\arraystretch}{1.1}\small",
         r"\begin{tabular}{@{}r@{\hspace{8pt}}r@{\hspace{5pt}}c@{\hspace{5pt}}l@{}}"]
    for i, r in enumerate(PSA_RULES, 1):
        toks = []
        for t in r.split():
            if t in ("E",):
                toks.append("$E$")
            elif t in ("i", "f"):
                toks.append(r"\textit{%s}" % t)
            elif t == "\\ldots":
                toks.append(r"\ldots")
            elif t == "&&":
                toks.append(r"\texttt{\&\&}")
            elif t == "||":
                toks.append(r"\texttt{\textbar\textbar}")
            else:
                toks.append(r"\texttt{%s}" % t)
        L.append(r"%d. & $E$ & $\longrightarrow$ & %s \\" % (i, " ".join(toks)))
    L.append(r"\end{tabular}}")
    open(path, "w", encoding="utf-8").write("\n".join(L) + "\n")


def prec_relation(a, b):
    ca = {c[0]: c for c in PREC_CLASSES}
    if a in ca and b in ca:
        la, aa = ca[a][2], ca[a][3]
        lb, pb = ca[b][2], ca[b][4]
        if pb:
            return "<"
        if la < lb:
            return ">"
        if la > lb:
            return "<"
        return ">" if aa == "L" else ("<" if aa == "R" else "")
    return None


def write_prec(path):
    ops = [c[0] for c in PREC_CLASSES]
    cols = ops + ["(", ")", "i", "$"]
    tex = {c[0]: c[1] for c in PREC_CLASSES}
    tex.update({"(": r"\texttt{(}", ")": r"\texttt{)}", "i": r"\textit{i}",
                "$": r"\texttt{\$}"})
    binary = {c[0] for c in PREC_CLASSES if not c[4]}

    def rel(a, b):
        r = prec_relation(a, b)
        if r is not None:
            return r
        if a == "(":
            if b == ")":
                return "="
            return "<" if (b in ops or b in ("(", "i")) else ""
        if a in ("i", ")"):
            return ">" if (b in binary or b in (")", "$")) else ""
        if a == "$":
            if b == "$":
                return "A"
            return "<" if (b in ops or b in ("(", "i")) else ""
        if b in ("(", "i"):         # a je operátor
            return "<"
        return ">" if b in (")", "$") else ""

    sym = {"<": r"$\lessdot$", ">": r"$\gtrdot$", "=": r"$\doteq$",
           "A": "A", "": ""}
    L = [r"% Vygenerováno skriptem gen_tabulky.py, needitujte ručně.",
         r"{\small\setlength{\tabcolsep}{4pt}",
         r"\renewcommand{\arraystretch}{1.3}",
         r"\begin{tabular}{|l||" + "c|" * len(cols) + "}", r"\hline",
         " & " + " & ".join(tex[c] for c in cols) + r" \\ \hline\hline"]
    for a in ops + ["(", ")", "i", "$"]:
        L.append(r"%s & %s \\ \hline" % (tex[a], " & ".join(
            sym[rel(a, b)] for b in cols)))
    L.append(r"\end{tabular}}")
    open(path, "w", encoding="utf-8").write("\n".join(L) + "\n")


def write_prec_levels(path):
    L = [r"% Vygenerováno skriptem gen_tabulky.py, needitujte ručně.",
         r"{\renewcommand{\arraystretch}{1.15}",
         r"\begin{tabular}{@{}cll@{}}",
         r"\toprule Priorita & Operátory & Asociativita \\ \midrule"]
    word = {"L": "levá", "R": "pravá", "N": "žádná"}
    for _, t, lvl, assoc, _p in PREC_CLASSES:
        L.append(r"%d & %s & %s \\" % (lvl, t, word[assoc]))
    L.append(r"\bottomrule\end{tabular}}")
    open(path, "w", encoding="utf-8").write("\n".join(L) + "\n")


if __name__ == "__main__":
    F = compute_first()
    FO = compute_follow(F)
    write_grammar("gramatika.tex")
    write_ll("ll_tabulka.tex", F, FO)
    write_first_follow("first_follow.tex", F, FO)
    write_psa_grammar("psa_gramatika.tex")
    write_prec("prec_tabulka.tex")
    write_prec_levels("prec_priority.tex")
    print("vygenerováno: gramatika.tex ll_tabulka.tex first_follow.tex "
          "psa_gramatika.tex prec_tabulka.tex prec_priority.tex")
