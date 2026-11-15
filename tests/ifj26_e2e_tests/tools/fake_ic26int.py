#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Falešný ic26int: spustí zdroj uložený v komentáři referenčním interpretem (viz fake_ifj26.py).
FAKE_MUTATE=out  přidá k výstupu znak X,  FAKE_MUTATE=run  změní návratový kód,
FAKE_MUTATE=hang zasekne se (kontrola timeoutu)."""
import base64
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ref_interp import run_program  # noqa: E402

code = open(sys.argv[-1], "rb").read().decode("latin-1")
m = re.search(r"^# SRC (\S+)$", code, re.M)
if not m:
    sys.exit(51)
if os.environ.get("FAKE_MUTATE") == "hang":
    time.sleep(60)
src = base64.b64decode(m.group(1))
try:
    out, rc = run_program(src, sys.stdin.buffer.read())
except BaseException:  # noqa: BLE001
    sys.exit(60)
if os.environ.get("FAKE_MUTATE") == "out":
    out += "X"
if os.environ.get("FAKE_MUTATE") == "run":
    rc = 53
sys.stdout.buffer.write(out.encode("latin-1"))
sys.exit(rc)
