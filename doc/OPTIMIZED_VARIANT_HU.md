# Optimalizált HexMG-Core – fejlesztési ág

2026-10-27. Ág: `codex/optimized-solver`.
Kiindulópont: `631f316f1bf095eabf1413bbb332c75924c30086`, hivatalos `main`.

A felhasználó az optimalizált változat fejlesztését, commitját és pushát
engedélyezte erre a külön ágra. A hivatalos `main` továbbra az ő saját
változata; oda az asszisztens sem commitot, sem merge-et, sem push-t nem végez.

**Kezdeti állapot:** az összes solver-/konverterforrás változatlan.
Ez még nem gyorsított kiadás: csak az ág és a fejlesztési szabályok készültek.
Nem készült új bináris, parser vagy numerikus algoritmus.

A GUI-nak az alap- és az optimalizált solver között csereszabatosnak kell
maradnia. A közös HMG-rész teljesen definiálja a fizikát; az opcionális
utasítások csak a végrehajtás módját változtatják. Az alapváltozat ugyanazt
a modellt nagy pontosságú, soros úton is megoldhatja. A kimenet, azonosítók,
mérősorrend és fizikailag értelmezett eredmény közös szerződés.

Jövőbeli témák: cellamodell-specializáció, ellenőrzött pontosságválasztás
a 0. szintű redukcióban, párhuzamosítás és más igazolt optimalizációk.
Ezek nem kész képességek. A fix és az időben átváltható peremhez külön
cellamodell is készülhet; mindkettőnek teljes hagyományos HMG-leírás kell.

Az opcionális HMG-blokk jelenleg egyeztetési tervezet. Mérvadó javaslat a
GUI tárolóban:
[OPTIMIZED_SOLVER_HMG_PROPOSAL_HU.md](https://github.com/LaszloPohl/HexMG-GUI/blob/codex/exact-quad-s4d/docs/solver/OPTIMIZED_SOLVER_HMG_PROPOSAL_HU.md).
A jóváhagyás után először az alapváltozatba kézzel beépíthető kihagyó
kódrészlet és célzott teszt készül. A numerikus gyorsítás külön fejlesztés.

A hivatalos ág új változásait ebbe az ágba átvesszük és összevetjük;
visszafelé nincs automatikus beolvasztás. A GUI solverválasztása nem
változik meg attól, hogy ez az ág létezik.
