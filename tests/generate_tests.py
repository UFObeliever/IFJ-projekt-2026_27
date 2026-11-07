#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generátor testů parseru IFJ26 (základní zadání, bez rozšíření).

Spuštění:  python3 generate_tests.py
Vytvoří adresář tests/ (stávající tests/ SMAŽE a vytvoří znovu).

Název souboru:  tests/<kategorie>/<nn>_<popis>.rc<kód>.scala
  rc2        -> kompilátor má skončit s návratovým kódem 2
  rc2or6     -> zadání je nejednoznačné, projde 2 i 6
Zdrojové soubory jsou čistě ASCII (jak říká zadání).
"""
import os
import re
import shutil
import textwrap

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "tests")

PRO = "import ifj26.*\n"
TRIV = "@main def main(): Unit =\n  val m = 0\n"      # triviální main
G = "def g(a: Int): Int = a\n"                          # pomocná funkce
P = "def p(): Unit =\n  val a = 1\n"                    # pomocná funkce vracející Unit


def dd(s):
    s = textwrap.dedent(s)
    return s if s.endswith("\n") else s + "\n"


def B(body):
    """Prolog + @main, jehož tělem jsou zadané příkazy."""
    return PRO + "\n@main def main(): Unit =\n" + textwrap.indent(dd(body), "  ")


def F(funcs):
    """Prolog + zadané funkce + triviální main."""
    return PRO + "\n" + dd(funcs) + "\n" + TRIV


def FB(funcs, body):
    """Prolog + zadané funkce + @main s tělem."""
    return (PRO + "\n" + dd(funcs) + "\n@main def main(): Unit =\n"
            + textwrap.indent(dd(body), "  "))


def R(prog):
    """Prolog + libovolný program (včetně vlastního main)."""
    return PRO + "\n" + dd(prog)


T = []


def add(cat, name, rc, src):
    T.append((cat, name, str(rc), src))


C1 = "01_prolog"
C2 = "02_main"
C3 = "03_hlavicky_funkci"
C4 = "04_bloky_odsazeni"
C5 = "05_prikazy"
C6 = "06_if_while"
C7 = "07_vyrazy"
C8 = "08_typy"
C9 = "09_rozsahy_platnosti"
C10 = "10_volani_funkci"
C11 = "11_navratove_typy"
C12 = "12_lexikalni"
C13 = "13_komentare_retezce"
C14 = "14_zatez"
C15 = "15_cele_programy"

# =============================================================== 01 prolog
c = C1
add(c, "ok_minimalni_program", 0, PRO + "\n" + TRIV)
add(c, "ok_komentare_a_prazdne_radky_pred_prologem", 0,
    "\n\n// komentar\n/* blok */\n   \n" + PRO + "\n" + TRIV)
add(c, "ok_blokovy_komentar_pres_radky_pred_prologem", 0,
    "/* a\n b */\n" + PRO + "\n" + TRIV)
add(c, "ok_mezery_mezi_tokeny", 0, "import   ifj26 . *\n\n" + TRIV)
add(c, "ok_tabulatory_mezi_tokeny", 0, "import\tifj26\t.\t*\n\n" + TRIV)
add(c, "ok_komentar_za_prologem", 0, "import ifj26.* // prolog\n\n" + TRIV)
add(c, "chybi_prolog", 2, TRIV)
add(c, "prazdny_soubor", 2, "")
add(c, "jen_komentare", 2, "// nic\n/* nic */\n")
add(c, "jen_prolog_bez_main", 3, PRO)
add(c, "prolog_dvakrat", 2, PRO + PRO + "\n" + TRIV)
add(c, "spatny_nazev_modulu", 2, "import ifj27.*\n\n" + TRIV)
add(c, "spatna_velikost_pismen_v_modulu", 2, "import IFJ26.*\n\n" + TRIV)
add(c, "bez_hvezdicky", 2, "import ifj26\n\n" + TRIV)
add(c, "misto_hvezdicky_identifikator", 2, "import ifj26.foo\n\n" + TRIV)
add(c, "bez_tecky", 2, "import ifj26*\n\n" + TRIV)
add(c, "podtrzitko_misto_hvezdicky", "1or2", "import ifj26._\n\n" + TRIV)
add(c, "chybi_klicove_slovo_import", 2, "ifj26.*\n\n" + TRIV)
add(c, "prolog_a_funkce_na_jednom_radku", 2,
    "import ifj26.* @main def main(): Unit =\n  val m = 0\n")
add(c, "prolog_az_po_funkci", 2, TRIV + "\n" + PRO)
add(c, "prikaz_pred_prologem", 2, "val x = 1\n" + PRO + "\n" + TRIV)

# =============================================================== 02 main
c = C2
add(c, "ok_jednoradkove_telo_unit", 0, PRO + "\n@main def main(): Unit = ()\n")
add(c, "ok_anotace_na_samostatnem_radku", 0,
    PRO + "\n@main\ndef main(): Unit =\n  val m = 0\n")
add(c, "ok_libovolny_nazev", 0, PRO + "\n@main def runFoo(): Unit =\n  val m = 0\n")
add(c, "ok_vice_mezer_za_anotaci", 0,
    PRO + "\n@main     def main(): Unit =\n  val m = 0\n")
add(c, "ok_komentar_za_anotaci", 0,
    PRO + "\n@main // vstup\ndef main(): Unit =\n  val m = 0\n")
add(c, "ok_main_za_jinou_funkci", 0,
    PRO + "\ndef helper(): Int = 1\n\n@main def main(): Unit =\n  val m = 0\n")
add(c, "ok_main_pred_jinou_funkci", 0,
    PRO + "\n@main def main(): Unit =\n  val m = 0\n\ndef helper(): Int = 1\n")
add(c, "ok_main_mezi_funkcemi", 0,
    PRO + "\ndef a(): Int = 1\n\n@main def main(): Unit =\n  val m = 0\n\ndef b(): Int = 2\n")
add(c, "ok_mezera_v_prazdnych_zavorkach", 0,
    PRO + "\n@main def main( ): Unit =\n  val m = 0\n")
add(c, "chybi_main", 3, PRO + "\ndef f(): Unit =\n  val a = 1\n")
add(c, "def_main_bez_anotace", 3, PRO + "\ndef main(): Unit =\n  val m = 0\n")
add(c, "dva_main_ruzne_nazvy", 4,
    PRO + "\n@main def a(): Unit =\n  val m = 0\n\n@main def b(): Unit =\n  val m = 0\n")
add(c, "dva_main_stejny_nazev", 4,
    PRO + "\n@main def main(): Unit =\n  val m = 0\n\n@main def main(): Unit =\n  val m = 0\n")
add(c, "main_s_parametrem", 5, PRO + "\n@main def main(a: Int): Unit =\n  val m = 0\n")
add(c, "main_vraci_int", 6, PRO + "\n@main def main(): Int =\n  return 1\n")
add(c, "main_vraci_string_jednoradkove", 6, PRO + '\n@main def main(): String = "a"\n')
add(c, "main_bez_navratoveho_typu", "0or6",
    PRO + "\n@main def main() =\n  val m = 0\n")
add(c, "anotace_s_mezerou_za_zavinacem", "1or2",
    PRO + "\n@ main def main(): Unit =\n  val m = 0\n")
add(c, "neznama_anotace", "1or2", PRO + "\n@foo def main(): Unit =\n  val m = 0\n")
add(c, "anotace_velka_pismena", "1or2", PRO + "\n@Main def main(): Unit =\n  val m = 0\n")
add(c, "anotace_bez_def", 2, PRO + "\n@main\n")
add(c, "anotace_pred_prikazem", 2, PRO + "\n@main\nval x = 1\n")
add(c, "anotace_dvakrat", 2, PRO + "\n@main @main def main(): Unit =\n  val m = 0\n")

# =============================================================== 03 hlavičky funkcí
c = C3
add(c, "ok_bez_parametru", 0, F("def f(): Int = 1"))
add(c, "ok_jeden_parametr", 0, F("def f(a: Int): Int = a"))
add(c, "ok_vsechny_typy_parametru", 0,
    F("def f(a: Int, b: Double, c: String, d: Unit): Unit =\n  val x = 1"))
add(c, "ok_mezery_okolo_zavorek_a_dvojtecek", 0, F("def f ( a : Int , b : Int ) : Int = a"))
add(c, "ok_prazdne_zavorky_s_mezerou", 0, F("def f( ): Int = 1"))
add(c, "ok_bez_navratoveho_typu_jednoradkove", 0, F("def f(a: Int) = a + 1"))
add(c, "ok_bez_navratoveho_typu_blok", 0, F("def f() =\n  val a = 1"))
add(c, "ok_parametr_typu_unit", 0, F("def f(u: Unit): Unit = ()"))
add(c, "ok_parametry_na_vice_radcich", 0,
    F("def f(\n    a: Int,\n    b: Int\n  ): Int = a + b"))
add(c, "ok_koncova_carka_s_koncem_radku", 0,
    F("def f(\n    a: Int,\n    b: Int,\n  ): Int = a + b"))
add(c, "ok_funkce_hned_za_sebou", 0, F("def f(): Int = 1\ndef g(): Int = 2"))
add(c, "ok_nazvy_s_podtrzitkem_a_velkymi_pismeny", 0,
    F("def _f(): Int = 1\ndef f_2(): Int = 2\ndef F(): Int = 3"))
add(c, "koncova_carka_na_stejnem_radku", 2, F("def f(a: Int,): Int = a"))
add(c, "carka_v_prazdnem_seznamu", 2, F("def f(,): Int = 1"))
add(c, "dvojita_carka", 2, F("def f(a: Int,, b: Int): Int = a"))
add(c, "carka_na_zacatku", 2, F("def f(, a: Int): Int = a"))
add(c, "chybi_dvojtecka_u_parametru", 2, F("def f(a Int): Int = a"))
add(c, "chybi_typ_parametru", 2, F("def f(a: ): Int = a"))
add(c, "chybi_jmeno_parametru", 2, F("def f(: Int): Int = 1"))
add(c, "chybi_carka_mezi_parametry", 2, F("def f(a: Int b: Int): Int = a"))
add(c, "neznamy_typ_parametru", "2or3", F("def f(a: Foo): Int = 1"))
add(c, "chybi_zaviraci_zavorka", 2, F("def f(a: Int: Int = a"))
add(c, "chybi_oteviraci_zavorka", 2, F("def f a: Int): Int = a"))
add(c, "chybi_rovnitko_jednoradkove", 2, F("def f(): Int 1"))
add(c, "chybi_rovnitko_blok", 2, F("def f(): Int\n  return 1"))
add(c, "chybi_jmeno_funkce", 2, F("def (): Int = 1"))
add(c, "klicove_slovo_jako_jmeno_funkce", 2, F("def if(): Int = 1"))
add(c, "typ_jako_jmeno_funkce", 2, F("def Int(): Int = 1"))
add(c, "klicove_slovo_jako_jmeno_parametru", 2, F("def f(val: Int): Int = 1"))
add(c, "chybi_typ_za_dvojteckou", 2, F("def f(): = 1"))
add(c, "chybi_dvojtecka_pred_navratovym_typem", 2, F("def f() Int = 1"))
add(c, "duplicitni_parametr", 4, F("def f(a: Int, a: Int): Int = a"))
add(c, "duplicitni_funkce", 4, F("def f(): Int = 1\ndef f(): Int = 2"))
add(c, "pretizeni_funkce", 4,
    F("def f(a: Int): Int = a\ndef f(a: String): Int = 1"))
add(c, "vnorena_funkce", 2,
    F("def f(): Unit =\n  val a = 1\n  def g(): Int = 1"))
add(c, "odsazena_funkce_na_nejvyssi_urovni", 2,
    PRO + "\n  def f(): Int = 1\n\n" + TRIV)
add(c, "prikaz_na_nejvyssi_urovni", 2, PRO + "\nval x = 1\n\n" + TRIV)
add(c, "volani_na_nejvyssi_urovni", 2, PRO + '\nprint("a")\n\n' + TRIV)
add(c, "dve_funkce_na_jednom_radku", 2, F("def f(): Int = 1 def g(): Int = 2"))
add(c, "smeti_za_jednoradkovym_telem", 2, F("def f(): Int = 1 2"))

# =============================================================== 04 bloky a odsazení
c = C4
add(c, "ok_odsazeni_1_mezera", 0, PRO + "\n@main def main(): Unit =\n val a = 1\n val b = 2\n")
add(c, "ok_odsazeni_4_mezery", 0,
    PRO + "\n@main def main(): Unit =\n    val a = 1\n    val b = 2\n")
add(c, "ok_odsazeni_8_mezer", 0,
    PRO + "\n@main def main(): Unit =\n        val a = 1\n        val b = 2\n")
add(c, "ok_ruzna_sirka_odsazeni_v_ruznych_blocich", 0,
    PRO + "\ndef f(): Unit =\n  val a = 1\n\n@main def main(): Unit =\n    val b = 2\n"
    "    if b < 3 then\n        val c = 1\n    else\n     val d = 2\n")
add(c, "ok_prazdne_radky_a_radky_s_mezerami", 0,
    PRO + "\n@main def main(): Unit =\n  val a = 1\n\n   \n  val b = 2\n      \n\n  val c = 3\n")
add(c, "ok_radek_jen_s_tabulatorem", 0,
    PRO + "\n@main def main(): Unit =\n  val a = 1\n\t\n  val b = 2\n")
add(c, "ok_tabulator_uvnitr_radku", 0,
    PRO + "\n@main def main(): Unit =\n  val a =\t1\n  val\tb = 2\n")
add(c, "ok_komentar_s_jinym_odsazenim", 0,
    PRO + "\n@main def main(): Unit =\n  val a = 1\n      // komentar\n// dalsi\n  val b = 2\n")
add(c, "ok_koncove_mezery", 0,
    PRO + "\n@main def main(): Unit =   \n  val a = 1   \n  val b = 2  \n")
add(c, "ok_bez_koncoveho_konce_radku", 0,
    PRO + "\n@main def main(): Unit =\n  val a = 1\n  val b = 2")
add(c, "ok_prazdne_radky_na_konci_souboru", 0,
    PRO + "\n@main def main(): Unit =\n  val a = 1\n\n   \n\n")
add(c, "ok_dedent_o_dve_urovne", 0,
    B("var i = 0\nwhile i < 3 do\n  if i < 1 then\n    i = i + 2\n  else\n    i = i + 1\ni = i + 1"))
add(c, "ok_vnorene_if_else_spravne_zarovnane", 0,
    B("var a = 0\nif a < 1 then\n  if a < 0 then\n    a = 1\n  else\n    a = 2\nelse\n  a = 3"))
add(c, "ok_while_v_while_v_if", 0,
    B("var a = 0\nvar b = 0\nif a < 1 then\n  while a < 3 do\n    while b < 3 do\n      b = b + 1\n"
      "    a = a + 1\nelse\n  a = 5"))
add(c, "ok_if_v_else_vetvi", 0,
    B("var a = 0\nif a < 1 then\n  a = 1\nelse\n  if a < 2 then\n    a = 2\n  else\n    a = 3"))
add(c, "ok_pokracovani_v_zavorkach", 0, B("val x = (1 +\n    2)"))
add(c, "prazdny_blok_funkce_eof", 2, PRO + "\ndef f(): Unit =\n")
add(c, "prazdny_blok_funkce_nasleduje_dalsi_funkce", 2, PRO + "\ndef f(): Unit =\n\n" + TRIV)
add(c, "telo_funkce_neodsazene", 2, PRO + "\ndef f(): Unit =\nval a = 1\n\n" + TRIV)
add(c, "telo_if_neodsazene", 2, B("var a = 0\nif a < 1 then\na = 2\nelse\n  a = 3"))
add(c, "neocekavane_odsazeni", 2, B("val a = 1\n  val b = 2"))
add(c, "dedent_na_nesouvisejici_uroven_ve_funkci", 2,
    PRO + "\ndef f(): Int =\n    val a = 1\n  return a\n\n" + TRIV)
add(c, "dedent_na_nesouvisejici_uroven_v_cyklu", 2,
    B("var a = 0\nwhile a < 3 do\n    a = a + 1\n  a = a + 2"))
add(c, "else_zarovnany_s_telem_vetve", 2,
    B("var a = 0\nif a < 1 then\n  a = 1\n  else\n  a = 2"))
add(c, "else_vic_odsazeny_nez_if", 2,
    B("var a = 0\nif a < 1 then\n    a = 1\n  else\n    a = 2"))
add(c, "else_mene_odsazeny_nez_if", 2,
    PRO + "\n@main def main(): Unit =\n  var a = 0\n  if a < 1 then\n    a = 1\nelse\n  a = 2\n")
add(c, "chybi_else_v_zakladnim_zadani", 2, B("var a = 0\nif a < 1 then\n  a = 1"))
add(c, "chybi_else_u_vnitrniho_if", 2,
    B("var a = 0\nif a < 1 then\n  if a < 0 then\n    a = 1\nelse\n  a = 3"))
add(c, "else_bez_if", 2, B("var a = 0\nelse\n  a = 1"))
add(c, "dvojity_else", 2,
    B("var a = 0\nif a < 1 then\n  a = 1\nelse\n  a = 2\nelse\n  a = 3"))
add(c, "tabulator_v_odsazeni", 1, PRO + "\n@main def main(): Unit =\n\tval a = 1\n")
add(c, "tabulator_po_mezerach_v_odsazeni", 1,
    PRO + "\n@main def main(): Unit =\n  val a = 1\n \tval b = 2\n")
add(c, "pokracovani_v_zavorkach_se_stejnym_odsazenim", "1or2", B("val x = (1 +\n2)"))
add(c, "radek_zacina_operatorem_krat", 2, B("val x = 1\n* 2"))
add(c, "radek_zacina_operatorem_deleno", 2, B("val x = 1\n/ 2"))
add(c, "radek_zacina_porovnanim", 2, B("val x = 1\n== 2"))
add(c, "vyraz_pokracuje_na_dalsim_radku_bez_zavorek", 2, B("val x = 1 +\n  2"))
add(c, "dva_prikazy_na_radku_bez_oddelovace", 2, B("val a = 1 val b = 2"))

# =============================================================== 05 příkazy
c = C5
add(c, "ok_val_var_odvozeny_typ", 0, B('val a = 1\nvar b = 2.5\nval c = "s"\nvar d = ()'))
add(c, "ok_vsechny_explicitni_typy", 0,
    B('val a: Int = 1\nval b: Double = 1.5\nval c: String = "x"\nval d: Unit = ()'))
add(c, "ok_mezery_okolo_rovna_se_a_dvojtecky", 0,
    B("val x=1\nvar y:Int=2\nval  z : Int  =  3"))
add(c, "ok_nazvy_identifikatoru", 0,
    B("val _x = 1\nval x_1 = 2\nval X = 3\nval a_b_ = 4\nval abc123 = 5\nval Z9_ = 6"))
add(c, "ok_dvojite_podtrzitko_jako_nazev", 0, B("val __ = 1"))
add(c, "ok_rozliseni_velikosti_pismen", 0, B("val x = 1\nval X = 2\nval xX = 3"))
add(c, "ok_prirazeni", 0, B("var a = 1\na = 2\na = a + 1\na = a * a - 3"))
add(c, "ok_strednik_mezi_prikazy", 0, B("val a = 1; val b = 2; var c = 3\nc = a + b"))
add(c, "ok_strednik_v_tele_cyklu", 0,
    B("var i = 0\nvar j = 0\nwhile i < 3 do\n  i = i + 1; j = j + 1"))
add(c, "ok_vyraz_jako_prikaz_literal", 0, B("val a = 1\n1 + 2\nval b = 2"))
add(c, "ok_vyraz_jako_prikaz_promenna", 0, B("val a = 1\na\nval b = 2"))
add(c, "ok_unit_jako_prikaz", 0, B("val a = 1\n()\nval b = 2"))
add(c, "val_bez_inicializace", 2, B("val x"))
add(c, "var_bez_inicializace_s_typem", 2, B("var x: Int"))
add(c, "val_bez_jmena", 2, B("val = 5"))
add(c, "val_bez_vyrazu", 2, B("val x ="))
add(c, "val_klicove_slovo_jako_jmeno", 2, B("val if = 1"))
add(c, "val_typ_jako_jmeno", 2, B("val Int = 1"))
add(c, "val_jmeno_zacina_cislici", "1or2", B("val 1x = 2"))
add(c, "val_dvojtecka_bez_typu", 2, B("val x: = 1"))
add(c, "val_typ_bez_dvojtecky", 2, B("val x Int = 1"))
add(c, "val_dva_typy", 2, B("val x: Int Int = 1"))
add(c, "val_dvojite_rovna_se", 2, B("val x == 1"))
add(c, "prirazeni_bez_prave_strany", 2, B("var a = 1\na ="))
add(c, "plus_rovna_se", "1or2", B("var a = 1\na += 1"))
add(c, "dvojity_strednik", 2, B("val a = 1;; val b = 2"))
add(c, "strednik_na_zacatku_radku", 2, B("; val a = 1"))
add(c, "prikaz_zacina_zaviraci_zavorkou", 2, B("val a = 1\n)"))
add(c, "prikaz_zacina_then", 2, B("then"))
add(c, "prikaz_zacina_do", 2, B("do"))
add(c, "prikaz_zacina_rovna_se", 2, B("= 5"))
add(c, "prikaz_zacina_dvojteckou", 2, B(": Int"))
add(c, "prikaz_zacina_carkou", 2, B(", a"))
add(c, "prikaz_zacina_typem", 2, B("Int"))
add(c, "import_uvnitr_funkce", 2, B("import ifj26.*"))

# =============================================================== 06 if / while
c = C6
add(c, "ok_if_else", 0, B("var a = 0\nif a < 1 then\n  a = 1\nelse\n  a = 2"))
add(c, "ok_while", 0, B("var a = 0\nwhile a < 10 do\n  a = a + 1"))
ops = ["<", "<=", ">", ">=", "==", "!="]
add(c, "ok_vsechny_relacni_operatory", 0,
    B("var a = 0\n" + "".join("if a %s 1 then\n  a = 1\nelse\n  a = 2\n" % o for o in ops)))
add(c, "ok_podminka_retezce", 0,
    B('var a = 0\nif "a" < "b" then\n  a = 1\nelse\n  a = 2\n'
      'if "a" == "b" then\n  a = 1\nelse\n  a = 2'))
add(c, "ok_podminka_int_a_double", 0,
    B("var a = 0\nif 1 < 2.5 then\n  a = 1\nelse\n  a = 2\nif 1 == 1.0 then\n  a = 1\nelse\n  a = 2"))
add(c, "ok_podminka_v_zavorce", 0,
    B("var a = 0\nif (a < 5) then\n  a = 1\nelse\n  a = 2\nwhile (a < 9) do\n  a = a + 1"))
add(c, "ok_podminka_s_aritmetikou", 0,
    B("var a = 0\nvar b = 5\nif a + 1 * 2 <= b - 3 then\n  a = 1\nelse\n  a = 2"))
add(c, "ok_else_s_unit_literalem", 0,
    B("val x = 3\nif x < 5 then\n  val t = 1\nelse\n  ()"))
add(c, "ok_return_v_cyklu_a_if", 0,
    F("def find(n: Int): Int =\n  var i = 0\n  while i < 100 do\n    if i * i > n then\n"
      "      return i\n    else\n      i = i + 1\n  return -1"))
add(c, "if_bez_then", 2, B("var a = 0\nif a < 1\n  a = 1\nelse\n  a = 2"))
add(c, "if_bez_podminky", 2, B("if then\n  val a = 1\nelse\n  val b = 1"))
add(c, "while_bez_do", 2, B("var a = 0\nwhile a < 1\n  a = 1"))
add(c, "while_bez_podminky", 2, B("while do\n  val a = 1"))
add(c, "then_na_novem_radku", 2, B("var a = 0\nif a < 1\nthen\n  a = 1\nelse\n  a = 2"))
add(c, "podminka_je_prirazeni", 2, B("var a = 0\nif a = 1 then\n  a = 1\nelse\n  a = 2"))
add(c, "podminka_nezavrena_zavorka", 2,
    B("var a = 0\nif (a < 1 then\n  a = 1\nelse\n  a = 2"))
add(c, "podminka_zavorka_navic", 2, B("var a = 0\nif a < 1) then\n  a = 1\nelse\n  a = 2"))
add(c, "while_podminka_je_prirazeni", 2, B("var a = 0\nwhile a = 1 do\n  a = 2"))
add(c, "telo_then_chybi_eof", 2, B("var a = 0\nif a < 1 then"))
add(c, "telo_else_chybi_eof", 2, B("var a = 0\nif a < 1 then\n  a = 1\nelse"))
add(c, "telo_while_chybi_eof", 2, B("var a = 0\nwhile a < 1 do"))
add(c, "podminka_typu_int", 6, B("val a = 1\nif a then\n  val b = 1\nelse\n  val c = 1"))
add(c, "podminka_aritmeticky_vyraz", 6,
    B("val a = 1\nif a + 1 then\n  val b = 1\nelse\n  val c = 1"))
add(c, "podminka_typu_string", 6, B('if "a" then\n  val b = 1\nelse\n  val c = 1'))
add(c, "while_podminka_typu_int", 6, B("var a = 1\nwhile a do\n  a = 0"))
add(c, "podminka_typu_unit", 6, B("if () then\n  val a = 1\nelse\n  val b = 1"))
add(c, "podminka_retezeni_relacnich", "2or6",
    B("if 1 < 2 < 3 then\n  val a = 1\nelse\n  val b = 1"))
add(c, "podminka_retezeni_rovnosti", "2or6",
    B("if 1 == 1 != 2 then\n  val a = 1\nelse\n  val b = 1"))
add(c, "podminka_nesouhlas_typu", 6,
    B('if 1 < "a" then\n  val a = 1\nelse\n  val b = 1'))
add(c, "podminka_retezec_rovno_int", 6,
    B('if "a" == 1 then\n  val a = 1\nelse\n  val b = 1'))
add(c, "podminka_nedefinovana_promenna", 3,
    B("if zz < 1 then\n  val a = 1\nelse\n  val b = 1"))
add(c, "while_nedefinovana_promenna", 3, B("while zz < 1 do\n  val a = 1"))

# =============================================================== 07 výrazy
c = C7
add(c, "ok_priorita_a_asociativita", 0,
    B("val a = 1 + 2 * 3\nval b = (1 + 2) * 3\nval c = 10 - 4 - 3\nval d = 100 / 10 / 5\n"
      "val e = 2 * 3 + 4 * 5 - 6 / 2"))
add(c, "ok_unarni_minus", 0,
    B("val a = 1\nval b = 2\nval c = -1\nval d = - -1\nval e = -(-1)\nval f = 1 - -1\n"
      "val g = 2 * -3\nval h = -a * -b\nval i = -(a + b)"))
add(c, "ok_zavorky_hluboko", 0, B("val a = ((((((((1))))))))"))
add(c, "ok_zavorky_kolem_promenne", 0, B("val a = 1\nval b = (a)\nval c = ((a) + (b))"))
add(c, "ok_bez_mezer", 0, B("val a=1+2*3-4/2"))
add(c, "ok_minus_varianty", 0, B("val a = 1\nval b = a-1\nval c = a - 1\nval d = a- 1\nval e = a-(-1)"))
add(c, "ok_retezeni_retezcu", 0, B('val a = "a" + "b" + "c"\nval b = a + a'))
add(c, "ok_celociselne_deleni", 0, B("val a: Int = 7 / 2\nval b: Int = -7 / 2"))
add(c, "ok_double_aritmetika", 0,
    B("val a: Double = 1 + 2.5\nval b: Double = 5 / 2.0\nval c: Double = 1.5 * 2\n"
      "val d = 2.5 - 1\nval e: Double = d"))
add(c, "ok_int_deleni_do_double", 0, B("val a: Double = 5 / 2"))
for nm, lit in [("tecka", "1.5"), ("bez_cele_casti", ".5"), ("jen_exponent", "1e3"),
                ("velke_E", "1E3"), ("exponent_plus", "1e+3"), ("exponent_minus", "1e-3"),
                ("tecka_a_exponent", "1.5e3"), ("podtrzitka", "1_000.5"),
                ("pripona_d", "1.0d"), ("pripona_D", "1.0D"),
                ("exponent_a_pripona", "1e2d"), ("nula", "0.0"), ("nulovy_exponent", "0e0")]:
    add(c, "ok_double_literal_" + nm, 0, B("val x: Double = " + lit))
for nm, lit in [("nula", "0"), ("jednocifernny", "7"), ("podtrzitka", "1_000_000"),
                ("max_int", "2147483647")]:
    add(c, "ok_int_literal_" + nm, 0, B("val x: Int = " + lit))
add(c, "vyraz_koncovy_operator", 2, B("val x = 1 +"))
add(c, "vyraz_zacina_krat", 2, B("val x = * 1"))
add(c, "vyraz_dva_operatory", 2, B("val x = 1 * * 2"))
add(c, "vyraz_nezavrena_zavorka", 2, B("val x = (1 + 2"))
add(c, "vyraz_zavorka_navic", 2, B("val x = 1 + 2)"))
add(c, "vyraz_prazdna_cast_v_zavorce", 2, B("val x = (1 + )"))
add(c, "vyraz_dva_literaly_za_sebou", 2, B("val x = 1 2"))
add(c, "vyraz_dve_promenne_za_sebou", 2, B("val a = 1\nval b = 2\nval x = a b"))
add(c, "vyraz_retezec_a_cislo_za_sebou", 2, B('val x = "a" 1'))
add(c, "vyraz_jen_operator", 2, B("val x = *"))
add(c, "vyraz_jen_tecka", 2, B("val x = ."))
add(c, "vyraz_carka", 2, B("val x = 1, 2"))
add(c, "typ_int_plus_string", 6, B('val x = 1 + "a"'))
add(c, "typ_string_plus_int", 6, B('val x = "a" + 1'))
add(c, "typ_unarni_minus_string", 6, B('val x = -"a"'))
add(c, "typ_string_krat_string", 6, B('val x = "a" * "b"'))
add(c, "typ_string_minus_string", 6, B('val x = "a" - "b"'))
add(c, "typ_string_deleno_int", 6, B('val x = "a" / 2'))
add(c, "typ_unit_plus_int", 6, B("val x = () + 1"))
add(c, "typ_unit_promenna_v_aritmetice", 6, B("val u = ()\nval x = u + 1"))
add(c, "typ_unarni_minus_unit", 6, B("val x = -()"))
add(c, "typ_deleni_double_do_int", 6, B("val x: Int = 5 / 2.0"))
add(c, "typ_soucet_double_do_int", 6, B("val x: Int = 1 + 1.5"))
add(c, "typ_soucet_int_do_stringu", 6, B("val x: String = 1 + 2"))
add(c, "typ_promenne_ruznych_typu", 6, B('val a = 1\nval b = "x"\nval c = a + b'))
add(c, "nedefinovana_promenna_ve_vyrazu", 3, B("val x = zz + 1"))
add(c, "promenna_ve_vlastni_inicializaci", 3, B("val x = x + 1"))
add(c, "pouziti_pred_definici", 3, B("val a = b\nval b = 1"))
add(c, "nedefinovana_promenna_jako_prikaz", 3, B("zz"))

# =============================================================== 08 typy
c = C8
add(c, "ok_rozsireni_int_na_double_inicializace", 0, B("val d: Double = 1"))
add(c, "ok_rozsireni_int_na_double_prirazeni", 0,
    B("var d: Double = 1.5\nd = 2\nd = d + 1"))
add(c, "ok_odvozeni_typu", 0,
    B('val a = 1\nval b = a + 1\nval c: Int = b\nval d = 1.5\nval e: Double = d\n'
      'val s = "x"\nval t: String = s'))
add(c, "ok_promenna_typu_unit", 0, B("var u: Unit = ()\nu = ()\nval v = u"))
add(c, "zuzeni_double_na_int_inicializace", 6, B("val i: Int = 1.5"))
add(c, "zuzeni_double_promenne_na_int", 6, B("val x = 1.0\nval i: Int = x"))
add(c, "retezec_do_int", 6, B('val i: Int = "a"'))
add(c, "int_do_retezce", 6, B("val s: String = 1"))
add(c, "double_do_retezce", 6, B("val s: String = 1.5"))
add(c, "unit_do_int", 6, B("val i: Int = ()"))
add(c, "int_do_unit", 6, B("val u: Unit = 5"))
add(c, "prirazeni_retezce_do_int", 6, B('var i = 1\ni = "s"'))
add(c, "prirazeni_double_do_int", 6, B("var i = 1\ni = 2.5"))
add(c, "prirazeni_int_do_retezce", 6, B('var s = "a"\ns = 1'))
add(c, "prirazeni_promenne_jineho_typu", 6, B('val a = 1\nvar s = "x"\ns = a'))
add(c, "prirazeni_vyrazu_double_do_int", 6, B("var i = 1\ni = i + 1.5"))
add(c, "pravdivostni_hodnota_do_val", "2or6", B("val b = 1 < 2"))
add(c, "pravdivostni_hodnota_do_var_s_typem", "2or6", B("var b: Int = 1 < 2"))
add(c, "pravdivostni_hodnota_do_prirazeni", "2or6", B("var a = 0\na = 1 < 2"))
add(c, "pravdivostni_hodnota_v_return", "2or6", F("def f(): Int =\n  return 1 < 2"))
add(c, "neznamy_typ_promenne", "2or3", B("val x: Foo = 1"))
add(c, "typ_malymi_pismeny", "2or3", B("val x: int = 1"))

# =============================================================== 09 rozsahy platnosti
c = C9
add(c, "ok_zastineni_v_podbloku", 0,
    B('val x = 1\nif x < 2 then\n  val x = "s"\n  val y: String = x\nelse\n  val z = 0\n'
      'val w: Int = x'))
add(c, "ok_stejne_jmeno_v_obou_vetvich", 0,
    B('var a = 0\nif a < 1 then\n  val t = 1\nelse\n  val t = "s"'))
add(c, "ok_stejne_jmeno_v_ruznych_funkcich", 0,
    F("def f(): Int =\n  val a = 1\n  return a\n\ndef g(): Int =\n  val a = 2\n  return a"))
add(c, "ok_uprava_vnejsi_promenne_v_cyklu", 0,
    B("var sum = 0\nvar i = 0\nwhile i < 5 do\n  sum = sum + i\n  i = i + 1"))
add(c, "ok_promenna_v_tele_cyklu", 0,
    B("var i = 0\nwhile i < 3 do\n  var t = 0\n  t = t + i\n  i = i + 1"))
add(c, "ok_promenna_viditelna_v_podblocich", 0,
    B("val a = 1\nvar b = 0\nif a < 2 then\n  while b < 3 do\n    b = b + a\nelse\n  b = a"))
add(c, "ok_zastineni_parametru_v_tele", "0or4",
    F("def f(a: Int): Int =\n  val a = 2\n  return a"))
add(c, "redefinice_ve_stejnem_bloku", 4, B("val a = 1\nval a = 2"))
add(c, "redefinice_val_a_var", 4, B("var a = 1\nval a = 2"))
add(c, "redefinice_po_vnorenem_bloku", 4,
    B("val a = 1\nif a < 2 then\n  val b = 1\nelse\n  val b = 2\nval a = 3"))
add(c, "redefinice_v_tele_cyklu", 4,
    B("var i = 0\nwhile i < 2 do\n  val t = 1\n  val t = 2\n  i = i + 1"))
add(c, "redefinice_s_jinym_typem", 4, B('val a = 1\nval a = "s"'))
add(c, "pouziti_mimo_if", 3,
    B("var a = 0\nif a < 1 then\n  val t = 1\nelse\n  val u = 2\nval x = t"))
add(c, "pouziti_mimo_while", 3,
    B("var i = 0\nwhile i < 2 do\n  val t = 1\n  i = i + 1\nval x = t"))
add(c, "pouziti_v_else_promenne_z_then", 3,
    B("var a = 0\nif a < 1 then\n  val t = 1\nelse\n  a = t"))
add(c, "parametr_mimo_svou_funkci", 3,
    F("def f(a: Int): Int =\n  return a\n\ndef g(): Int =\n  return a"))
add(c, "lokalni_promenna_jine_funkce", 3,
    F("def f(): Int =\n  val a = 1\n  return a\n\ndef g(): Int =\n  return a"))
add(c, "prirazeni_do_nedefinovane", "3or7", B("zz = 1"))
add(c, "prirazeni_do_val", 7, B("val a = 1\na = 2"))
add(c, "prirazeni_do_parametru", 7, F("def f(a: Int): Unit =\n  a = 5"))
add(c, "prirazeni_do_zastinene_promenne", 7,
    B("var a = 1\nif a < 2 then\n  val a = 5\n  a = 6\nelse\n  a = 7"))

# =============================================================== 10 volání funkcí
c = C10
builtin_ok = [
    ("readInt", "val a: Int = readInt()"),
    ("readDouble", "val a: Double = readDouble()"),
    ("readLine", "val a: String = readLine()"),
    ("readInt_do_double", "val a: Double = readInt()"),
    ("toStr_int", "val a: String = toStr(5)"),
    ("toStr_double", "val a: String = toStr(1.5)"),
    ("toStr_string", 'val a: String = toStr("x")'),
    ("toStr_unit", "val a: String = toStr(())"),
    ("toInt", "val a: Int = toInt(1.5)"),
    ("strLength", 'val a: Int = strLength("abc")'),
    ("subStr", 'val a: String = subStr("abcd", 1, 3)'),
    ("ord", 'val a: Int = ord("abc", 1)'),
    ("chr", "val a: String = chr(65)"),
    ("print_int", "print(1)"),
    ("print_double", "print(1.5)"),
    ("print_string", 'print("x")'),
    ("print_unit", "print(())"),
    ("print_promenna", "val s = 1\nprint(s)"),
    ("argumenty_jako_promenne", 'val s = "abc"\nval n = 1\nval t = subStr(s, n, 3)'),
]
for nm, code in builtin_ok:
    add(c, "ok_vestavena_" + nm, 0, B(code))
add(c, "ok_volani_jako_prikaz", 0, FB(G, "g(1)"))
add(c, "ok_volani_unit_funkce_jako_prikaz", 0, FB(P, "p()"))
add(c, "ok_volani_v_inicializaci", 0, FB(G, "val x = g(1)\nval y: Int = g(2)"))
add(c, "ok_volani_v_prirazeni", 0, FB(G, "var x = 0\nx = g(2)"))
add(c, "ok_volani_v_return", 0, F(G + "\ndef h(): Int =\n  return g(1)"))
add(c, "ok_volani_jako_jednoradkove_telo", 0, F(G + "\ndef h(): Int = g(1)"))
add(c, "ok_volani_pred_definici", 0,
    PRO + "\n@main def main(): Unit =\n  val x = g(1)\n\ndef g(a: Int): Int = a\n")
add(c, "ok_vzajemna_rekurze", 0,
    F("def isEven(n: Int): Int =\n  if n == 0 then\n    return 1\n  else\n    val m = n - 1\n"
      "    return isOdd(m)\n\ndef isOdd(n: Int): Int =\n  if n == 0 then\n    return 0\n"
      "  else\n    val m = n - 1\n    return isEven(m)"))
add(c, "ok_prima_rekurze_s_typem", 0,
    F("def fact(n: Int): Int =\n  if n < 2 then\n    return 1\n  else\n    val m = n - 1\n"
      "    val r = fact(m)\n    return n * r"))
add(c, "ok_volani_bez_argumentu", 0, FB("def one(): Int = 1\n", "val x = one()"))
add(c, "ok_volani_s_mezerami", 0, FB(G, "val x = g ( 1 )\nval y = g( 2 )"))
add(c, "ok_argumenty_na_vice_radcich_s_koncovou_carkou", 0,
    B('val s = subStr(\n    "abcdef",\n    1,\n    3,\n  )'))
add(c, "ok_argument_unit", 0, FB("def u(a: Unit): Unit = ()\n", "u(())"))
add(c, "ok_vysledek_unit_funkce_do_promenne", 0, FB(P, "val u = p()"))
add(c, "ok_vice_parametru_ruznych_typu", 0,
    FB("def h(a: Int, b: Double, c: String): String = c\n", 'val x = h(1, 2.5, "s")'))
add(c, "nedefinovana_funkce_jako_prikaz", 3, B("foo(1)"))
add(c, "nedefinovana_funkce_v_inicializaci", 3, B("val x = foo(1)"))
add(c, "malo_argumentu", 5, FB(G, "g()"))
add(c, "moc_argumentu", 5, FB(G, "g(1, 2)"))
add(c, "spatny_typ_argumentu_string", 5, FB(G, 'g("a")'))
add(c, "spatny_typ_argumentu_double", 5, FB(G, "g(1.5)"))
add(c, "spatny_typ_argumentu_promenna", 5, FB(G, 'val s = "a"\ng(s)'))
add(c, "spatny_pocet_v_inicializaci", 5, FB(G, "val x = g(1, 2)"))
add(c, "spatny_pocet_v_return", 5, F(G + "\ndef h(): Int =\n  return g()"))
for nm, code in [
    ("print_bez_argumentu", "print()"),
    ("print_dva_argumenty", "print(1, 2)"),
    ("readInt_s_argumentem", "val a = readInt(1)"),
    ("readLine_s_argumentem", 'val a = readLine("x")'),
    ("strLength_int", "val a = strLength(5)"),
    ("toInt_string", 'val a = toInt("a")'),
    ("subStr_malo_argumentu", 'val a = subStr("abc", 1)'),
    ("subStr_spatny_typ", 'val a = subStr("abc", "b", 1)'),
    ("chr_string", 'val a = chr("a")'),
    ("ord_chybi_index", 'val a = ord("a")'),
    ("toStr_bez_argumentu", "val a = toStr()"),
    ("toStr_dva_argumenty", "val a = toStr(1, 2)"),
]:
    add(c, "vestavena_" + nm, 5, B(code))
add(c, "vysledek_readInt_do_stringu", 6, B("val s: String = readInt()"))
add(c, "vysledek_readLine_do_int", 6, B("val i: Int = readLine()"))
add(c, "vysledek_readDouble_do_int", 6, B("val i: Int = readDouble()"))
add(c, "vysledek_toInt_do_stringu", 6, B("val s: String = toInt(1.5)"))
add(c, "vysledek_uzivatelske_funkce_do_stringu", 6, FB(G, "val s: String = g(1)"))
add(c, "vysledek_unit_funkce_do_int", 6, FB(P, "val i: Int = p()"))
add(c, "vysledek_volani_do_prirazeni_spatny_typ", 6, FB(G, 'var s = "a"\ns = g(1)'))
add(c, "volani_nezavrene", 2, B('print("a"'))
add(c, "volani_koncova_carka_na_stejnem_radku", 2, FB(G, "g(1,)"))
add(c, "volani_dvojita_carka", 2, B('val a = subStr("abc",, 1)'))
add(c, "volani_chybi_carka", 2, FB(G, "g(1 2)"))
add(c, "dve_volani_na_radku", 2, B('print("a") print("b")'))
add(c, "volani_jen_carka", 2, FB(G, "g(,)"))
add(c, "volani_zaviraci_zavorka_navic", 2, FB(G, "g(1))"))

# =============================================================== 11 návratové typy
c = C11
add(c, "ok_return_odpovidajici_typy", 0,
    F('def a(): Int =\n  return 1\n\ndef b(): Double =\n  return 1.5\n\n'
      'def c(): String =\n  return "s"\n\ndef d(): Unit =\n  return'))
add(c, "ok_return_rozsireni_int_na_double", 0, F("def f(): Double =\n  return 1"))
add(c, "ok_return_hodnoty_ve_funkci_unit", 0, F("def f(): Unit =\n  return 5"))
add(c, "ok_jednoradkove_telo_int_ve_funkci_unit", 0, F("def f(): Unit = 5"))
add(c, "ok_posledni_vyraz_promenna", 0, F("def f(): Int =\n  val a = 1\n  a"))
add(c, "ok_posledni_vyraz_aritmeticky", 0, F("def f(): Int =\n  val a = 1\n  a + 2"))
add(c, "ok_posledni_vyraz_rozsireni_na_double", 0, F("def f(): Double =\n  val a = 1\n  a"))
add(c, "ok_blok_konci_definici_odvozeny_unit", 0, F("def f() =\n  val a = 1"))
add(c, "ok_blok_konci_prirazenim_explicitni_unit", 0,
    F("def f(): Unit =\n  var a = 1\n  a = 2"))
add(c, "ok_blok_konci_cyklem_explicitni_unit", 0,
    F("def f(): Unit =\n  var a = 1\n  while a < 3 do\n    a = a + 1"))
add(c, "ok_if_vsechny_vetve_return", 0,
    F("def f(a: Int): Int =\n  if a < 0 then\n    return 1\n  else\n    return 2"))
add(c, "ok_vnorene_if_vsechny_vetve_return", 0,
    F("def f(a: Int): Int =\n  if a < 0 then\n    if a < 10 then\n      return 1\n    else\n"
      "      return 2\n  else\n    return 3"))
add(c, "ok_if_unit_vetve_odvozeny_unit", 0,
    F("def f() =\n  var a = 1\n  if a < 2 then\n    a = 3\n  else\n    a = 4"))
add(c, "ok_if_unit_vetve_explicitni_unit", 0,
    F("def f(): Unit =\n  var a = 1\n  if a < 2 then\n    a = 3\n  else\n    a = 4"))
add(c, "ok_explicitni_unit_if_vetve_vraci_hodnoty", 0,
    F("def f(a: Int): Unit =\n  if a < 0 then\n    return 1\n  else\n    return 2"))
add(c, "ok_odvozeny_typ_int", 0, FB("def f(a: Int) = a + 1\n", "val x: Int = f(1)"))
add(c, "ok_odvozeny_typ_string", 0, FB('def g() = "s"\n', "val s: String = g()"))
add(c, "jednoradkove_int_vs_string", 6, F('def f(): Int = "a"'))
add(c, "jednoradkove_string_vs_int", 6, F("def f(): String = 1"))
add(c, "jednoradkove_int_vs_double", 6, F("def f(): Int = 1.5"))
add(c, "jednoradkove_int_vs_unit", 6, F("def f(): Int = ()"))
add(c, "return_string_v_int_funkci", 6, F('def f(): Int =\n  return "a"'))
add(c, "return_double_v_int_funkci", 6, F("def f(): Int =\n  return 1.5"))
add(c, "return_string_v_double_funkci", 6, F('def f(): Double =\n  return "a"'))
add(c, "return_v_cyklu_spatny_typ", 6,
    F('def f(): Int =\n  var i = 0\n  while i < 3 do\n    return "a"\n  return 1'))
add(c, "return_bez_hodnoty_v_int_funkci", "6or10", F("def f(): Int =\n  return"))
add(c, "return_bez_explicitniho_typu", 6, F("def f() =\n  return 1"))
add(c, "return_v_if_bez_explicitniho_typu", 6,
    F("def f(a: Int) =\n  if a < 0 then\n    return 1\n  else\n    return 2"))
add(c, "posledni_vyraz_spatneho_typu", 6, F('def f(): Int =\n  val s = "a"\n  s'))
add(c, "blok_konci_definici_explicitni_int", 6, F("def f(): Int =\n  val a = 1"))
add(c, "blok_konci_prirazenim_explicitni_int", 6, F("def f(): Int =\n  var a = 1\n  a = 2"))
add(c, "blok_konci_cyklem_explicitni_int", 6,
    F("def f(): Int =\n  var a = 1\n  while a < 3 do\n    a = a + 1"))
add(c, "if_vetev_bez_return_explicitni_int", 6,
    F("def f(a: Int): Int =\n  var b = 0\n  if a < 0 then\n    return 1\n  else\n    b = 2"))
add(c, "if_vetve_s_hodnotami_explicitni_int", 6,
    F("def f(a: Int): Int =\n  if a < 0 then\n    1\n  else\n    2"))
add(c, "prima_rekurze_bez_typu", 6, F("def f(a: Int) = f(a)"))
add(c, "prima_rekurze_blok_bez_typu", 6, F("def f(a: Int) =\n  val b = f(a)"))
add(c, "neprima_rekurze_bez_typu", 6, F("def f(a: Int) = g(a)\ndef g(a: Int) = f(a)"))
add(c, "odvozeny_typ_string_pouzit_jako_int", 6, FB('def g() = "s"\n', "val n: Int = g()"))
add(c, "odvozeny_typ_int_pouzit_jako_string", 6,
    FB("def f(a: Int) = a + 1\n", "val s: String = f(1)"))

# =============================================================== 12 lexikální chyby (průchod parserem)
c = C12
add(c, "neznamy_znak_mrizka", 1, B("val x = 1 # 2"))
add(c, "neznamy_znak_dolar", 1, B("val x = 1 $ 2"))
add(c, "neznamy_znak_otaznik", 1, B("val x = ?"))
add(c, "zpetne_apostrofy", 1, B("val `x` = 1"))
add(c, "znakovy_literal", 1, B("val c = 'a'"))
add(c, "samotny_ampersand", "1or2", B("val x = 1 & 2"))
add(c, "samotna_svisla_cara", "1or2", B("val x = 1 | 2"))
add(c, "zavinac_ve_vyrazu", "1or2", B("val x = @"))
add(c, "slozene_zavorky", "1or2", F("def f(): Unit = { }"))
add(c, "neukonceny_retezec", 1, B('val s = "abc'))
add(c, "novy_radek_v_retezci", 1, B('val s = "abc\ndef"'))
add(c, "neplatna_escape_x", 1, B(r'val s = "\x"'))
add(c, "neplatna_escape_q", 1, B(r'val s = "a\qb"'))
add(c, "neplatna_escape_nula", 1, B(r'val s = "\0"'))
add(c, "retezec_konci_zpetnym_lomitkem_a_uvozovkou", 1, B(r'val s = "abc\"'))
add(c, "nuly_na_zacatku_literalu", 1, B("val x = 007"))
add(c, "exponent_bez_cislic", "1or2", B("val x = 1e"))
add(c, "exponent_se_znamenkem_bez_cislic", "1or2", B("val x = 1e+"))
add(c, "tecka_bez_desetinne_casti", "1or2", B("val x = 1."))
add(c, "koncove_podtrzitko_v_cisle", "1or2", B("val x = 1_"))
add(c, "hexadecimalni_literal", "1or2", B("val x = 0x10"))
add(c, "sufix_L", "1or2", B("val x = 10L"))
add(c, "sufix_f", "1or2", B("val x = 1.5f"))
add(c, "neukonceny_blokovy_komentar", 1, B("val x = 1 /* nekonci"))
add(c, "neukonceny_vnoreny_komentar", 1, B("val x = 1 /* /* vnitrni */ nekonci"))
add(c, "uzaviraci_komentar_bez_oteviraciho", "1or2", B("val x = 1 */"))
add(c, "token_za_viceradkovym_komentarem", "1or2",
    PRO + "\n@main def main(): Unit =\n  /* a\n  b */ val x = 1\n")

# =============================================================== 13 komentáře a řetězce
c = C13
add(c, "ok_radkovy_komentar_za_prikazem", 0, B("val a = 1 // komentar\nval b = 2"))
add(c, "ok_radkovy_komentar_na_konci_bez_LF", 0, PRO + "\n" + TRIV + "// konec")
add(c, "ok_blokovy_komentar_uvnitr_radku", 0, B("val /* c */ a = /* d */ 1"))
add(c, "ok_prazdny_blokovy_komentar", 0, B("val a = 1 /**/ + 2"))
add(c, "ok_vnorene_komentare", 0, B("/* a /* b /* c */ d */ e */\nval a = 1"))
add(c, "ok_radkovy_komentar_v_blokovem", 0,
    B("/* // */\nval a = 1\n/* a // b\n c */\nval b = 2"))
add(c, "ok_blokovy_komentar_v_radkovem", 0, B("// /* neni zacatek\nval a = 1"))
add(c, "ok_viceradkovy_komentar_jako_radek", 0, B("val a = 1\n/* vice\nradku */\nval b = 2"))
add(c, "ok_viceradkovy_komentar_za_prikazem", 0,
    B("val a = 1 /* vice\nradku */\nval b = 2"))
add(c, "ok_komentar_v_retezci", 0,
    B('val s = "// neni komentar"\nval t = "/* ani tohle */"'))
add(c, "ok_deleni_a_komentar", 0, B("val a = 4 / 2\nval b = 4/2\nval c = 4 /* c */ / 2"))
add(c, "ok_komentar_mezi_if_a_else", 0,
    B("var a = 0\nif a < 1 then\n  a = 1\n// komentar\nelse\n  a = 2"))
add(c, "ok_komentar_za_then_a_else", 0,
    B("var a = 0\nif a < 1 then // c\n  a = 1\nelse // d\n  a = 2"))
add(c, "ok_komentar_za_rovna_se_funkce", 0, F("def f(): Int = // c\n  return 1"))
add(c, "ok_komentar_pred_anotaci", 0,
    PRO + "\n// vstup\n@main def main(): Unit =\n  val m = 0\n")
add(c, "ok_blokovy_komentar_ve_vyrazu_v_zavorce", 0, B("val x = (1 + /* c */ 2)"))
add(c, "ok_radkovy_komentar_v_zavorkach_pres_radky", 0,
    B("val x = (1 + // c\n    2)"))
add(c, "ok_escape_sekvence", 0,
    B("\n".join([r'val a = "\\"', r'val b = "\""', r'val c = "\n"', r'val d = "\r"',
                 r'val e = "\t"', r'val f = "a\\b\"c\nd"'])))
add(c, "ok_prazdny_retezec", 0, B('val s = ""\nval t = "" + ""'))
add(c, "ok_specialni_znaky_v_retezci", 0,
    B('val s = "a b # $ % & \' ` ~ ! ? : ; , . ( ) [ ] { } < > = + - * / @ |"'))
add(c, "ok_viceradkovy_retezec_ze_zadani", 0,
    PRO + '\n@main def main(): Unit =\n  val x = """A\nhoj s\\nvete\n"""\n  val y = 1\n')
add(c, "ok_viceradkovy_retezec_s_uvozovkami", 0, B('val s = """a "q" b"""'))
add(c, "ok_viceradkovy_retezec_zpetna_lomitka", 0, B(r'val s = """s\nvete \" \x"""'))
add(c, "ok_viceradkovy_retezec_s_komentari", 0,
    B('val s = """// ne\n/* ne */"""\nval t = 1'))
add(c, "viceradkovy_retezec_neukonceny", 1, B('val s = """abc\ndef'))


# =============================================================== 14 zátěž
def nested_if(n):
    L = ["var a = 0"]
    for i in range(n):
        L.append("  " * i + "if a < 1 then")
    L.append("  " * n + "a = 1")
    for i in reversed(range(n)):
        L.append("  " * i + "else")
        L.append("  " * (i + 1) + "a = 2")
    return "\n".join(L) + "\n"


def nested_while(n):
    L = ["var a = 0"]
    for i in range(n):
        L.append("  " * i + "while a < 1 do")
    L.append("  " * n + "a = 1")
    return "\n".join(L) + "\n"


c = C14
add(c, "zavorky_100", 0, B("val a = " + "(" * 100 + "1" + ")" * 100))
add(c, "zavorky_300_limit_zasobniku_256", 0, B("val a = " + "(" * 300 + "1" + ")" * 300))
add(c, "unarni_minus_100", 0, B("val a = " + "- " * 100 + "1"))
add(c, "unarni_minus_300_limit_zasobniku_256", 0, B("val a = " + "- " * 300 + "1"))
add(c, "scitani_1000_clenu", 0, B("val a = " + " + ".join(["1"] * 1000)))
add(c, "smisene_operatory_500_clenu", 0, B("val a = " + " + ".join(["2 * 3 - 1"] * 500)))
add(c, "mnoho_funkci_300", 0,
    PRO + "\n" + "".join("def f%d(): Int = %d\n" % (i, i) for i in range(300)) + "\n" + TRIV)
add(c, "mnoho_prikazu_2000", 0, B("var a = 0\n" + "a = a + 1\n" * 2000))
add(c, "mnoho_promennych_v_bloku_1000", 0,
    B("".join("val v%d = %d\n" % (i, i) for i in range(1000))))
add(c, "hluboke_zanoreni_if_40", 0, B(nested_if(40)))
add(c, "hluboke_zanoreni_while_40", 0, B(nested_while(40)))
add(c, "dlouhy_identifikator_1000", 0, B("val " + "a" * 1000 + " = 1"))
add(c, "dlouhy_retezec_20000", 0, B('val s = "' + "x" * 20000 + '"'))
add(c, "mnoho_parametru_50", 0,
    F("def f(" + ", ".join("p%d: Int" % i for i in range(50)) + "): Int = p0"))
add(c, "mnoho_prazdnych_radku_5000", 0, PRO + "\n" * 5000 + TRIV)

# =============================================================== 15 celé programy
c = C15
add(c, "spec_priklad_factorial", 0, R("""\
def factorial(n: Int): Int =
  var result = 1
  var i: Int = n // uvedeni typu je nepovinne
  while i > 1 do
    result = result * i
    i = i - 1
  result // posledni vyraz v tele funkce urcuje navratovou hodnotu

@main def runFoo(): Unit =
  val n = readInt()
  val result = factorial(n)
  print(result)
  print("\\n")
"""))
add(c, "spec_priklad_if_else_s_unit", 0, R("""\
@main def main(): Unit =
  val x = 3
  if x < 5 then
    print("prilis malo")
  else
    ()
"""))
add(c, "prog_fibonacci", 0, R("""\
def fib(n: Int): Int =
  var a = 0
  var b = 1
  var i = 0
  while i < n do
    val t = a + b
    a = b
    b = t
    i = i + 1
  return a

@main def main(): Unit =
  val n = readInt()
  val r = fib(n)
  val s = toStr(r)
  print(s)
  print("\\n")
"""))
add(c, "prog_gcd", 0, R("""\
def gcd(a: Int, b: Int): Int =
  var x = a
  var y = b
  while y != 0 do
    val t = y
    y = x - x / y * y
    x = t
  return x

@main def main(): Unit =
  val a = readInt()
  val b = readInt()
  val g = gcd(a, b)
  print(g)
"""))
add(c, "prog_opakovani_retezce", 0, R("""\
def repeat(s: String, n: Int): String =
  var result = ""
  var i = 0
  while i < n do
    result = result + s
    i = i + 1
  return result

@main def main(): Unit =
  val r = repeat("ab", 3)
  print(r)
"""))
add(c, "prog_klasifikace_a_soucet", 0, R("""\
def classify(n: Int): String =
  if n < 0 then
    return "negative"
  else
    if n == 0 then
      return "zero"
    else
      return "positive"

def sumTo(n: Int): Int =
  var s = 0
  var i = 1
  while i <= n do
    s = s + i
    i = i + 1
  return s

@main def main(): Unit =
  val a = readInt()
  val c = classify(a)
  print(c)
  print("\\n")
  val s = sumTo(a)
  val t = toStr(s)
  print(t)
  print("\\n")
"""))
add(c, "prog_main_na_zacatku_pomocne_funkce_za_nim", 0, R("""\
@main def main(): Unit =
  val x = twice(4)
  print(x)

def twice(a: Int): Int =
  val b = once(a)
  return b + b

def once(a: Int): Int = a
"""))
add(c, "prog_pocitani_znaku", 0, R("""\
def countA(s: String): Int =
  var i = 0
  var c = 0
  val n = strLength(s)
  while i < n do
    val ch = ord(s, i)
    if ch == 97 then
      c = c + 1
    else
      ()
    i = i + 1
  return c

@main def main(): Unit =
  val s = readLine()
  val c = countA(s)
  print(c)
"""))


# =============================================================== zápis na disk
def main():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    counters = {}
    seen = set()
    per_cat = {}
    for cat, name, rc, src in T:
        assert re.fullmatch(r"\d+(or\d+)*", rc), (name, rc)
        assert (cat, name) not in seen, "duplicitni test: %s/%s" % (cat, name)
        seen.add((cat, name))
        data = src.encode("ascii")          # zdrojaky jsou jen ASCII
        n = counters.get(cat, 0) + 1
        counters[cat] = n
        d = os.path.join(OUT, cat)
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, "%02d_%s.rc%s.scala" % (n, name, rc)), "wb") as fh:
            fh.write(data)
        per_cat[cat] = n
    for cat in sorted(per_cat):
        print("%-28s %3d" % (cat, per_cat[cat]))
    print("%-28s %3d" % ("CELKEM", len(T)))


if __name__ == "__main__":
    main()
