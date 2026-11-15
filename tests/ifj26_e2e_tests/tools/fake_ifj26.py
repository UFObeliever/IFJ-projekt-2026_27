#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Falešný "překladač" pro kontrolu samotné sady testů (ne pro testování vašeho překladače).

Návratový kód zjistí z názvu testu (podle otisku zdroje) a u platných programů místo
IFJcode26 vypíše hlavičku a zdroj v komentáři; fake_ic26int.py ho pak spustí referenčním
interpretem. Když `general_tests.py` s touhle dvojicí projde celý, sada je vnitřně
konzistentní a spouštěč funguje:

  ./general_tests.py --compiler tools/fake_ifj26.py --interp tools/fake_ic26int.py

Proměnná FAKE_MUTATE=header zkazí hlavičku, FAKE_MUTATE=rc způsobí chybný kód překladače
(slouží k ověření, že spouštěč selhání opravdu hlásí).
"""
import base64
import hashlib
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAME_RE = re.compile(r"\.rc(\d+(?:or\d+)*)(?:\.run\d+)?\.scala$")


def table():
    t = {}
    for dirpath, _d, files in os.walk(os.path.join(ROOT, "tests")):
        for fn in files:
            m = NAME_RE.search(fn)
            if not m:
                continue
            codes = [int(x) for x in m.group(1).split("or")]
            rc = 0 if 0 in codes else codes[0]
            with open(os.path.join(dirpath, fn), "rb") as fh:
                t[hashlib.sha1(fh.read()).hexdigest()] = rc
    return t


src = sys.stdin.buffer.read()
rc = table().get(hashlib.sha1(src).hexdigest(), 99)
if os.environ.get("FAKE_MUTATE") == "rc" and rc == 0:
    sys.exit(6)
if rc != 0:
    sys.exit(rc)
head = b".IFJcode26" if os.environ.get("FAKE_MUTATE") != "header" else b"IFJcode26"
sys.stdout.buffer.write(head + b"\n# SRC " + base64.b64encode(src) + b"\n")
