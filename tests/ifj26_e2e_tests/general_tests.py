#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Testy celého překladače (základní zadání, bez rozšíření).

Pro každý program v tests/ spustí překladač a zkontroluje:
  * návratový kód překladače (chyby 1-10 pro neplatné programy, 0 pro platné),
  * u platných programů vygenerovaný IFJcode26: spustí ho v ic26int a porovná
    návratový kód a standardní výstup s očekávaným.

Skupiny testů: tests/generator (programy s ověřeným výstupem, včetně běhových chyb)
               tests/compiler  (korpus z testů parseru: chybové kódy 1-10 + běh platných programů)

Příklady:
  ./general_tests.py --compiler ./ifj26 --interp ./ic26int
  ./general_tests.py -k 07_funkce -v          # jen jedna kategorie, s podrobnostmi
  ./general_tests.py --no-run                 # bez ic26int: jen kódy překladače a hlavička
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e2e_lib  # noqa: E402

if __name__ == "__main__":
    sys.exit(e2e_lib.main(["generator", "compiler"], "general_tests.py", __doc__))
