# Szöveges eredmény: sorszámos kézi javítás az alap Core-hoz

2026-11-03. Alapforrás: **02a79a8022d4288663c0b112e0bb5bf26acfc388**,
`src/hmgSaver.h`, SHA-256:
`F96C3CA5E95084DF3E8BA053F26E8D004E9F616C49C74EA4D499AEEDD29E7D9C`.
Az alapforrást nem módosítottam. Az optimalizált solverben a javítás kész.

## A módosítás

Kizárólag a `saveTxtCore` függvényben (38–154. sor) cseréld a
formátumsztringek összes `%g` elemét `%.17g`-re, a `%+g` elemet pedig
`%+.17g`-re. Ne cseréld át a teljes fájlban vagy más függvényben.

| Eredeti sor | Tartalom | Csere |
| --- | --- | --- |
| 56 | Fejléces TIMESTEP: idő és dt, fejlécben és adatsorban | Négy `%g` → `%.17g` |
| 61 | Fejléces AC: frekvencia | Két `%g` → `%.17g` |
| 67, 69 | Fejléces TIMECONST két ága | Soronként négy `%g` → `%.17g` |
| 78 | Fejléces DC/tranziens mérőérték | `%g` → `%.17g` |
| 86 | Fejléces AC komplex érték | `%g%+gi` → `%.17g%+.17gi` |
| 94 | Fejléces időállandó érték | `%g` → `%.17g` |
| 110 | RAW TIMESTEP: idő és dt | Két `%g` → `%.17g` |
| 115 | RAW AC: frekvencia | `%g` → `%.17g` |
| 120 | RAW TIMECONST: két koordináta | Két `%g` → `%.17g` |
| 129 | RAW DC/tranziens érték | `%g` → `%.17g` |
| 137 | RAW AC: valós és képzetes rész | Két `%g` → `%.17g` |
| 145 | RAW időállandó érték | `%g` → `%.17g` |

Összesen **13 sor**, **26 `%g` és egy `%+g`**. Példa a 86. sorra:

```cpp
fprintf_s(fp, "%.17g%+.17gi", currentSave->saveValuesAC[i].real(), currentSave->saveValuesAC[i].imag());
```

A beolvasó, a solver számítása, az időlépési sorrend, a bináris HXIM és
a sorok/oszlopok elrendezése változatlan. A szöveg hosszabb lesz, mert
a double adatok visszaolvasásához elegendő számjegyet őriz meg. A korábbi
hatjegyű kiírásból már elvesztett számjegyek utólag nem állíthatók helyre.

## Ellenőrzés

Az optimalizált ág `tests/text_precision_runtime.py` programja 9 valódi
solverfutásban 27 DC/TIMESTEP/AC eset RAW és fejléces eredményét hasonlítja
a HXIM-hez: **198 számpár bitazonos**. Előtte mind a 27 esetben
reprodukálódott a kerekítés. Az AC/TIMECONST meglévő regressziója
**27 eset / 1768 numerikus ellenőrzés**, sikeres. A bitazonos új teszt
nem külön TIMECONST-spektrumteszt.

A teszt az optimalizált OPTIONAL-sort használja a három redukciós módhoz;
az alap solver ezt kihagyja. Az alapváltozat javítása után ugyanígy
futtatható, ott a három kör azonos alap-solveres pontosságot ellenőriz.
Parancssor: `text_precision_runtime.py --solver <exe> --output <új-mappa>`.
