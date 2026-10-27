# Helyi DC-pontosság – első, referenciaellenőrzött változat

2026-10-27. Csak az optimalizált ág; hivatalos alap: `631f316`.

```text
.OPTIONAL HEXMG_OPT 1
REDUCTION_PRECISION LEVEL0 EXTENDED
.END OPTIONAL
```

A mód EXTENDED, DOUBLE vagy AUTO. Blokk nélkül EXTENDED. A blokk a sorolvasó
által elért bármely helyen lehet, akár a HEXMG fejléc előtt vagy modellben.
Az egész bemenet feldolgozása megelőzi az utasítások végrehajtását:
ez **egész futtatásra szóló opció**, elhelyezése nem jelent helyi hatókört
vagy időbeli üzemmódváltást. Ismételt pontossági beállítás hiba, azonos értékkel
is. Az ismeretlen bővítés/verzió kihagyott, egyszeri figyelmeztetéssel.
Ismert v1 hibás fejléc/sor, idegen kulcs vagy ismétlés forráshelyes hiba.
A v1 legfeljebb 128 sort és soronként 4096 bájtot fogad; ismeretlen blokknál
a korábbi állandó memóriájú kihagyás megmarad. THREADS AUTO/1 dokumentált
soros mód, más érték nem támogatott hiba; párhuzamosítás még nincs.

## A numerikus út

`TypedFullMatrixReductorDC<Scalar>`: ugyanaz a megőrzött összeállítás,
redukció, jobb oldal és visszahelyettesítés `double` vagy `xxt` skalárral.
A jelen fordításban `xxt=dd_real`. Csak full-matrix strukturális levélkör
(nincs benne további ComponentSubCircuit) használhat double-t. Nem
multigridszint; a szülők, SUNRED-mátrixok és komplex AC változatlanok.

EXTENDED a korábbi út. DOUBLE és AUTO a nagy pontosságú referenciát is
kiszámítja minden kiértékeléskor. A YRED, NZB és NZBXA elemenkénti eltérése
legfeljebb 10⁻¹¹·|referencia|; nulla referencia mellett pontos nulla kell.
A JRED esetén ugyanez a helyi JA/JB maximumával bővített skála, NZBJB-nél
a referenciavektor maximuma, UB-nél UA és NZBJB maximuma a skála.
A különbség képzése dd_real-ben történik, a véges double referenciaérték
kötelező. Sem NaN, sem végtelen nem minősül sikeres egyezésnek.

Csak ellenőrzött double mátrix/vektor jut a szülőbe, és csak ellenőrzött
visszahelyettesítés írja az állapotot. DOUBLE esetén bármely eltérés hiba,
AUTO esetén a megfelelő extended eredmény használatos. Ha a double
előrehelyettesítő kivételt dob, de az extended sikerült, AUTO új double
munkamátrixokat készít, és a következő kiértékeléskor ismét próbálkozik.
A meglévő faktorizációcache mindkét típusnál megmarad; a double alkalmasság
nem külön, elavulható cache-bejegyzés.

Ez helyi referencia-összevetés, **nem globális hibabizonyítás**. Az extended
referencia maga is a jelenlegi solver algoritmusa, nem tetszőleges pontosságú
matematikai etalon. A jobb oldal és az állapot skálázott ellenőrzése kis,
közel nulla tagokra nem ad önálló relatív hibakorlátot.

## Költség és további munka

Valóban double mátrixokon történik a kért út, azonban a referencia miatt
két munkakészlet és többletszámítás van. **Nincs igazolt gyorsulás**, ez nem
a végleges gyorsított üzem. A következő lépés igazolt alkalmassági korlát és
érvénytelenítési szabály alapján ritkítani/kiváltani a referenciát,
független numerikus próbákkal. Az alapértelmezés ezért továbbra EXTENDED.

Napló: kért mód, sikeres helyi double előrehelyettesítések és extended
visszaesések száma. Ez kiértékelésszám, nem egyedi cellaszám. A hibázó
DOUBLE futásnak nem kell végső összesítést adnia; hibadiagnosztikája megmarad.

## Reprodukció

```text
python tests/check_precision.py --baseline <alap.exe> --optimized <optimalizalt.exe> --output <uj-mappa>
python tests/check_optional_solver.py --baseline <alap.exe> --optimized <optimalizalt.exe> --output <masik-uj-mappa>
ctest --test-dir <build> -C Release --output-on-failure
```

A precision teszt független DC-, implicit Euler RC- és girátoros egyenleteket
használ, időlépés- és frekvenciaváltással. Külön szélsőséges Schur-kivonásnál
DOUBLE elutasít, AUTO az extended eredményt őrzi. A korábbi 42 sorolvasóeset
és a dekorált/eredeti 18 cellás bemenet bájtszintű összevetése megmarad.
Nem változott alkatrészfizika, állapotelfogadási sorrend vagy HXIM-formátum.
