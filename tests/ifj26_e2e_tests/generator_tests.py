#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Testy generátoru kódu (základní zadání, bez rozšíření).

Jen platné programy z tests/generator: překladač musí skončit kódem 0, výstup musí
být platný IFJcode26 a po spuštění v ic26int musí program vypsat očekávaný text
a skončit očekávaným kódem (0, nebo 40 / 57 / 58 u testů běhových chyb).

Při neočekávaném kódu ic26int (51-58) skript vypíše, co ten kód znamená a co ve
vygenerovaném kódu obvykle bývá špatně. Kód neúspěšných testů uloží --save DIR.

Příklady:
  ./generator_tests.py --compiler ./ifj26 --interp ./ic26int
  ./generator_tests.py -k 06_promenne -v --save /tmp/code
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e2e_lib  # noqa: E402

if __name__ == "__main__":
    sys.exit(e2e_lib.main(["generator"], "generator_tests.py", __doc__))
