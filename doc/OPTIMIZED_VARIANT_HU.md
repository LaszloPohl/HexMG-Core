# Optimalizált HexMG-Core – fejlesztési ág

2026-11-17: a relatív iterációs ciklus az első elfogadott korrekció után már
a fizikai függvényértékekkel ellenőrzi a mérleget, nem a nulla sorszámú
induló közelítéssel. Egy kis hálózat korábban tévesen 0 V-ot fogadott el
a helyes 0,618033989 V helyett. Nyolc új célpróba, 51 relatív konvergencia-
és 87/21 adaptív időlépés-ellenőrzés PASS. Az ITER/ITERS pontos lépésszáma
megmarad. Ez az optimalizált relatív ciklus javítása; az alap saját
konvergenciájának sorrendje már helyes.

Az FCI áramlekérdezése a 2-től kezdődő vezérlőkapcsokra DC/időben és AC-ban
nullát ad. Korábban az X1 áramát jelentette, ezért az aláramkör külső
vezérlőkapcsán nem létező áram látszott. Hat előjeles DC/idő/AC célpróba
PASS. Az alap kézi javítása a GUI-tároló
`docs/solver/CORE_FCI_CONTROL_CURRENT_FIX_HU.md` fájljában, két sorszámos
cserével. A nagy OLED teljes konvergenciája ettől még nincs igazolva.

2026-11-15: a `NEWTON_FORWARD_LIMIT` és `ADAPTIVE_VOLTAGE_NODE` ismétlődésének
ellenőrzése indexelt keresést használ a korábbi teljes listabejárás helyett.
400000 bejegyzés feldolgozása 0,440 s; azonos és különböző áramkör,
fordított csomópontpár, blokkok közötti ismétlődés és hibás blokk utáni
változatlan állapot 11 célzott ellenőrzése PASS. A 25 meglévő Newton-lépés-
korlátozási futás is PASS. A nagy diagnosztikai OLED-bemenet korábban már
az opciók beolvasásánál elérte a 120 s időkorlátot; ez nem solverkonvergencia.
A fizikai modell és a lépéskorlátozás szabálya változatlan.

Az optikai fájlíró egyszer foglal a pontos bináris méretre. Egymillió,
nyolc szakaszos sugár: 192000436 bájt, változatlan ellenőrző összeg;
kódolás 0,383 → 0,324 s, ismételt növekedési másolás nélkül.


2026-11-14: [adaptív feszültségalapú időlépés](ADAPTIVE_TIME_HU.md), teljes
állapot-visszaállítással, választható nem konvergált folytatással és köztes
mérésmentéssel. A felhasználó által jóváhagyott opcionális sorok; alapváltozat
változatlan. 87/21 natív és 143/4 GUI-adatút-ellenőrzés.

2026-11-14: a mátrixműveletek nem nulla, 1e-20-nál kisebb pivotjai is
valódi reciprokot kapnak. Az abszolút küszöb helyett csak a pontos nulla
régi fallbackje marad; szinguláris hálózatok általános megoldása nem készült.
7140 mátrixelem-ellenőrzés és négy analitikus kisvezetésű hálózat PASS.
A Soma VO₂ első 1e-8 s lépése 60 sikertelen iteráció helyett 8-cal konvergál;
a teljes VO₂-futás konvergenciáját ez nem igazolja. Alapforrás változatlan,
a GUI-tároló docs/solver/CORE_SMALL_PIVOT_FIX_HU.md sorszámos útmutatót ad.

2026-10-27. Ág: `codex/optimized-solver`.
Kiindulópont: `631f316f1bf095eabf1413bbb332c75924c30086`, hivatalos `main`.

2026-10-31: a V forrás AC-áramtörvénye, a B paraméterek AC-kezelése és
a V forrás DC-áramlekérdezése javítva. 27/27 célzott futás, 1768 numerikus
ellenőrzés. [Okfeltárás és sorszámos kézi alapoldali javítás](AC_SOURCE_FIX_HU.md).

A felhasználó az optimalizált változat fejlesztését, commitját és pushát
engedélyezte erre a külön ágra. A hivatalos `main` továbbra az ő saját
változata; oda az asszisztens sem commitot, sem merge-et, sem push-t nem végez.

**Aktuális állapot:** a fizikai sorolvasó kihagyja az opcionális blokkokat
az első, ugyanabban a fájlban lévő `.END OPTIONAL` sorig. Ez modellben,
függvényben és sorfolytatások között is működik. A baseline-be kézzel
beépíthető [pontos kód és ellenőrzés](OPTIONAL_SKIP_BASELINE_HU.md) kész.
A v1 LEVEL0 EXTENDED/DOUBLE/AUTO beállítás már értelmezett;
a helyi reduktor két típuson futtatható, nagy pontosságú referenciaellenőrzéssel.
Gyorsulást még nem állítunk. [Pontos hatókör](LEVEL0_PRECISION_HU.md).

A GUI-nak az alap- és az optimalizált solver között csereszabatosnak kell
maradnia. A közös HMG-rész teljesen definiálja a fizikát; az opcionális
utasítások csak a végrehajtás módját változtatják. Az alapváltozat ugyanazt
a modellt nagy pontosságú, soros úton is megoldhatja. A kimenet, azonosítók,
mérősorrend és fizikailag értelmezett eredmény közös szerződés.

Jövőbeli témák és a korábban halasztott hibajavítások a bővítendő
[optimalizációs feladatlistában](OPTIMIZATION_BACKLOG_HU.md) vannak.
A 0. szintű dd_real/double választás első ellenőrzött változata kész;
referenciaköltségének biztonságos csökkentése következhet. A teljesen lineáris és az egyirányúan
csatolt elektrotermikus megoldás külön téma. Ezek még nem kész képességek.

Az opcionális HMG-blokk keretezését a felhasználó jóváhagyta és pontosította.
A GUI-integráció és a későbbi opciók leírása:
[OPTIMIZED_SOLVER_HMG_PROPOSAL_HU.md](https://github.com/LaszloPohl/HexMG-GUI/blob/codex/exact-quad-s4d/docs/solver/OPTIMIZED_SOLVER_HMG_PROPOSAL_HU.md).
Az alapváltozatba a felhasználó építi be a kihagyó kódot. Addig az új blokkot
az alapbináris nem fogadja el; a GUI továbbra blokk nélküli HMG-t exportál.

A hivatalos ág új változásait ebbe az ágba átvesszük és összevetjük;
az optimalizált és alapváltozat későbbi összevonása sincs tervben.
A felhasználó kérésére a GUI 0.352.1-ben az optimalizált az első és
alapértelmezett választás; a pontosság alapból EXTENDED.

## Fordítás és célzott ellenőrzés

```powershell
cmake -S . -B ../solver-optimized -G "Visual Studio 17 2022" -A x64
cmake --build ../solver-optimized --config Release --parallel 2
ctest --test-dir ../solver-optimized -C Release --output-on-failure
```

A CMake célok MSVC alatt `/W4 /WX` beállítással készülnek. A teszt friss
alkönyvtárba ír az `optional-fixtures` alatt, korábbi próbát nem ír felül.
A 42 sorolvasási eset, a GUI 18 cellás körútja és az opcionális blokkokkal
kibővített HMG bájtra azonos HXIM-eredménye sikeres. A numerikus összevetés
újrafuttatható a `tests/check_optional_solver.py` eszközzel, külön
`--baseline`, `--optimized`, `--output` argumentumokkal. Alapból a mellékelt
`tests/fixtures/thermal18.hmg` szintetikus GUI-exportot használja;
a `--deck` kapcsolóval más önálló, relatív HXIM-kimenetű bemenet adható meg.

## 2026-10-29 — anyagérvényesség

Jóváhagyott FUNCTION_GUARD DC/időlépés-védelem elkészült.
[Pontos szerződés, korlátok és tesztek](FUNCTION_GUARD_HU.md).


## 2026-11-17 — nagy optikai kontrolleradat beolvasása

A HXRAY001/002 fájl dekódolása közvetlenül a fájlból, legfeljebb 1 MiB-os
beolvasási pufferrel történik. A teljes kódolt fájl és a dekódolt program
nem marad egyszerre a memóriában. A formátum, az ellenőrzőösszeg és a
kapcsolódási/értéktartomány-ellenőrzések változatlanok. A GUI kódolója a
már elkészült programot másolás nélkül is átadhatja a kódolónak.

Célzott ellenőrzés: 206 program/fájl kapu (két formátum, csonkolás, sérülés,
1 MiB-on átnyúló rekordok és hosszú címek), valamint 20 tényleges
BATCHCONTROLLER betöltési/ütemezési kapu PASS. Ez adatkezelési módosítás;
az optikai egyenleteket és a Core iterációs sorrendjét nem változtatja.
