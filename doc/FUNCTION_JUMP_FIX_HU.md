# FUNCTION: feltétel nélküli ugrás hiányzó indexének javítása

2026-11-03. Az alap Core **02a79a8** változatához tartozó kézi útmutató.
Az alapforrást nem módosítottam; az optimalizált ágban javítva.

## Pontos sorcsere

`src/hmgHMGFileReader.cpp`, **2739. sor**, `HMGFileFunction::Read`:

```cpp
                    case bift_JMP:      ReadLabel(func, lineToken, reader, lineInfo); break;
```

helyett:

```cpp
                    case bift_JMP:      ReadLabel(func, lineToken, reader, lineInfo);
                                        ReadParams(func, 0, lineToken, reader, line, lineInfo, true);
                                        break;
```

Más részt nem kell átírni. A függvénynév és a `case bift_JMP` akkor is
megtalálja a helyet, ha a saját módosítások elmozdították a sorszámot.
Az optimalizált, korábbi `eaec5bf` forrásban ugyanez a 2790. sor.

## A hiba oka

Minden függvényutasítás belső indexmezejében van visszatérési és
munkaterület-index, akkor is, ha az utasítás nem ad vissza értéket.
A `ReadParams(..., 0, ..., true)` ennek megfelelően egy használaton
kívüli RET-hivatkozást készít; nem olvas új tokent a HMG-sorból.

A `_JMP` ága ezt eddig kihagyta. Emiatt a `fillIndexField` üres
`line.parameters` vektornál is lekérdezte a 0. elemet. A reprodukció
Windows alatt 0xC0000005 hozzáférési hibával leállt, még a számítás előtt.
A feltételes ugrások és a `_JMPR` már előkészítették ezt az indexet.
A javítás azonos belső szerződést biztosít a feltétel nélküli ugrásnak is.
Nem változik a címkék jelentése, az ugrás iránya vagy a HMG-szintaxis.

## Ellenőrzés

Az optimalizált Core `tests/function_jump_runtime.py` próbája 18 kis
netlistát tartalmaz: három pontosságmód, előre- és visszaugrás, negatív/
nulla/pozitív paraméter. Van átugrott nullával osztás és négyfordulós
összegző ciklus is. A régi alap és optimalizált bináris mind a 18
bemeneten összeomlott; a javítás után 18/18 eset, 36/36 érték helyes.
A teszt első ellenőrzésében a külön I áramforrás referenciaelőjelét
helyesbíteni kellett −1 V-ra; az ugrás eredménye már előtte is helyes volt.

A Churchill-C valódi hőáramkörében a normál és a nagyon keskeny
cellaközponti intervallum két ágát külön ellenőriztük. A GUI indítás
előtti binárispróbája mindkét ágat használja; egy régi solverrel nem
indítja el az érintett konvekciós futást.

Kimenetek a GUI-projektben: `diagnostics/runs/solver_gaps_20261102/`
alatt `jump-optimized-before`, `jump-baseline-before`, `jump-fixed-2`
és `vertical-mean-native-2`. Ez külön hiba a SUNRED vezérlőportok
indexelésétől; mindkét javítás szükséges az új teljesfal-peremekhez.
