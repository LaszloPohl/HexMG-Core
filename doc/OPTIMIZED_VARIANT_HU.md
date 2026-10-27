# Optimalizált HexMG-Core – fejlesztési ág

2026-10-27. Ág: `codex/optimized-solver`.
Kiindulópont: `631f316f1bf095eabf1413bbb332c75924c30086`, hivatalos `main`.

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
