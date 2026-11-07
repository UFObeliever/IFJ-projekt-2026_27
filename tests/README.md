# Testy parseru IFJ26 (základní zadání)

471 testů v `tests/<kategorie>/<nn>_<popis>.rc<kód>.scala`. Zdrojáky jsou čisté ASCII.

## Spuštění

    ./run_tests.sh ./ifj26            # všechny testy
    ./run_tests.sh ./ifj26 09_rozsahy # jen cesty obsahující řetězec
    VERBOSE=1 TIMEOUT=10 ./run_tests.sh ./ifj26

Předpoklad: kompilátor čte zdroj ze **stdin** a testuje se jen **návratový kód**
(výstup se zahazuje). Jiný způsob spuštění se mění ve funkci `run_one` na začátku skriptu.
Pády (SIGABRT z `assert`, SIGSEGV) a timeouty se hlásí odděleně.

## Název souboru = očekávaný kód

- `.rc0.scala` – validní program, očekává se 0
- `.rc3.scala` – očekává se chyba 3
- `.rc2or6.scala` – zadání je nejednoznačné, projde 2 i 6 (např. řetězení `a < b < c`,
  uložení Boolean do proměnné, neznámý typ `Foo`, `@main` bez návratového typu)

## Kategorie

| složka | obsah |
|---|---|
| 01_prolog | tvar a umístění `import ifj26.*` |
| 02_main | `@main`: chybí / dva / parametr / návratový typ |
| 03_hlavicky_funkci | parametry, čárky, koncová čárka, duplicity, vnořené def |
| 04_bloky_odsazeni | odsazení, `else` zarovnání, tabulátory, pokračování v závorkách |
| 05_prikazy | val/var/přiřazení, středníky, výrazy jako příkazy, chybné začátky příkazů |
| 06_if_while | podmínky, chybějící then/do/else, typ podmínky |
| 07_vyrazy | priorita, unární minus, literály, syntaktické a typové chyby výrazů |
| 08_typy | Int→Double, zúžení, odvozování, Boolean mimo podmínku |
| 09_rozsahy_platnosti | shadowing, redefinice (4), použití mimo rozsah (3), zápis do val (7) |
| 10_volani_funkci | vestavěné i uživatelské funkce, počet/typ argumentů (5), forward reference |
| 11_navratove_typy | pravidla pro návratový typ, implicitní hodnota bloku, rekurze bez typu |
| 12_lexikalni | lexikální chyby, které projdou parserem (1) |
| 13_komentare_retezce | komentáře (vnořené, víceřádkové), escape sekvence, `"""` řetězce |
| 14_zatez | hluboké závorky (limit `PrecedenceStack` 256), dlouhé výrazy, 300 funkcí… |
| 15_cele_programy | příklady ze zadání a pár reálných programů |

## Co testy nepokrývají

Rozšíření (FUNEXP, BOOLTHEN, CONTROL, …) – testy předpokládají jen základní zadání,
např. `if` bez `else` nebo volání funkce uvnitř výrazu je v nich chyba. Pokud některé
rozšíření děláte, příslušné testy smažte nebo přepište.

## Úpravy

`generate_tests.py` je zdroj všech testů; `python3 generate_tests.py` adresář `tests/`
**smaže a vytvoří znovu**. Vlastní testy buď přidejte do generátoru, nebo ručně
do jiné složky (spouštěč bere všechny `*.scala` pod `tests/`).
