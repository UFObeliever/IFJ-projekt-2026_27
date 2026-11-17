# Testy celého překladače a generátoru kódu IFJ26 (bez rozšíření)

    ./general_tests.py   --compiler ./ifj26 --interp ./ic26int     # všechno (695 testů)
    ./generator_tests.py --compiler ./ifj26 --interp ./ic26int     # jen generátor (192 testů)

Potřebuje jen Python 3 (žádné balíčky navíc).

## Co se kontroluje

Pro každý program: překladač dostane zdroj na **stdin** a IFJcode26 vypíše na **stdout**.
1. Návratový kód překladače musí odpovídat názvu testu (`.rc0` = přijmout, `.rc6` = chyba 6, `.rc2or6` = obojí je v pořádku).
2. U kódu 0 musí výstup začínat řádkem `.IFJcode26`.
3. Kód se spustí v `ic26int` (soubor s kódem je poslední argument, stdin z `NN_nazev.in`, jinak prázdný).
4. Návratový kód interpretu musí sedět (`.run57` = očekává se 57, jinak 0) a stdout musí být přesně `NN_nazev.out`.

| skript | skupiny | k čemu |
|---|---|---|
| `general_tests.py` | `tests/generator` + `tests/compiler` | celý překladač: chybové kódy 1–10 i běh platných programů |
| `generator_tests.py` | `tests/generator` | jen generování kódu: výstup programů, běhové chyby, vstup ze stdin |

`tests/compiler` je korpus z testů parseru (503 programů). U platných programů je navíc očekávaný
výstup, takže se ověří i jejich běh. `tests/generator` jsou programy psané pro generátor
(aritmetika, Double, řetězce, porovnání, cykly, rozsahy proměnných, funkce a rekurze, vstup, běhové chyby, algoritmy, PARAMLISTS).

## Volby

    -k TEXT        jen testy, jejichž cesta obsahuje TEXT (lze víckrát), např. -k 07_funkce  nebo  -k faktorial
    -x TEXT        vynechá takové testy
    -v             podrobnosti u neúspěchů (očekávaný × skutečný výstup, stderr)
    --save DIR     uloží IFJcode26 neúspěšných testů (dobré pro ruční ladění generátoru)
    --no-run       bez ic26int: jen kód překladače a hlavička
    -j N           souběžnost;  --timeout S  limit na překlad i běh (výchozí 10 s)
    --list         jen vypíše vybrané testy

Cesty lze zadat i proměnnými `IFJ_COMPILER` a `IFJ_INTERP`. Když `ic26int` skončí neočekávaným kódem 51–58,
skript vypíše, co ten kód znamená a co ve vygenerovaném kódu bývá špatně (např. 52 = `DEFVAR` v cyklu nebo
duplicitní návěští, 53 = chybí `INT2FLOAT`, 56 = čtení neinicializované proměnné).

## Odkud jsou očekávané výstupy (důležité)

`ic26int` ani Scala nejsou po ruce, proto je výstup každého programu spočítaný referenčním interpretem
`tools/ref_interp.py` (psaným podle zadání) a u 191 ze 192 testů navíc porovnaný s **ručně / nezávisle spočítanou
hodnotou** – `tools/make_tests.py` spadne, když se neshodují. Přesto nejde o výstup skutečného `ic26int`.
Když test selže, ale podle zadání je váš výstup správně, chyba může být v očekávání. Pro pochybnosti lze
program porovnat se Scalou (`scala run ifj26.scala prog.scala`; Scala ale vypíše Double jinak než `%a`).

Rozhodnutí podle zadání, na kterých očekávání stojí:
* `print(Double)` vypisuje hexadecimálně (`%a`, jako `WRITE`): `print(1.5)` → `0x1.8p+0`; `print(())` → `()`.
* `toStr(Double)` je `%.2f`. U Double bez desetinné části je zadání nejednoznačné, proto se netestuje.
* Dělení Int zaokrouhluje **k nule** (−7 / 2 = −3), kdežto `IDIV` zaokrouhluje k −∞. Dělení nulou (Int i Double) → kód 57.
* `subStr` mimo rozsah → 40 (včetně `start == strLength`); `chr` mimo 0–127 → 58 (`INT2CHAR`); `ord` mimo rozsah → −1.
* `readInt`/`readDouble`: bílé znaky se neignorují, neplatný vstup nebo EOF → 0; `readLine` vrací řádek včetně LF.
* Netestuje se: přetečení Int, Int předaný jako argument typu Double, `toStr` celého Double, `+5` a `007` ve vstupu.

## Kontrola samotné sady

    ./general_tests.py --compiler tools/fake_ifj26.py --interp tools/fake_ic26int.py

Falešný překladač a interpret spustí referenční interpret. Musí projít všech 695 testů; proměnná
`FAKE_MUTATE=out|run|header|rc|hang` záměrně něco pokazí, aby se ověřilo, že spouštěč selhání hlásí.

## Přidání testů

Do `tools/make_tests.py` přidejte řádek `gen(kategorie, "nazev", zdroj, stdin=..., out=<ruční hodnota>, run=<kód>)`
a spusťte `python3 tools/make_tests.py` (složku `tests/generator` smaže a vytvoří znovu; v `tests/compiler`
jen přepíše soubory `.out`). Test lze přidat i ručně: stačí soubory `NN_nazev.rc0.scala` (+ `.in`, `.out`).
