# SUNRED: a vezérlőportok kizárása a redukció mátrixából

2026-11-03. Kézi javítás az alap Core **02a79a8** forrásához.
Az alapváltozat forrását nem módosítottam. Az optimalizált változatban
a javítás elkészült, külön numerikus ellenőrzéssel.

## Pontos módosítás

Fájl: `src/hmgSunred.cpp`, **25. sor** a fenti commitban.
Függvény: `SunredTreeNode::loadLeafDataFromSubcircuit`.
A `CNodeIndex.clear();` után ezt az egy sort:

```cpp
    cuns ncSiz = (uns)srcComponent->def->nodesConnectedTo.size();
```

cseréld erre:

```cpp
    cuns ncSiz = srcComponent->getModel().getN_X_Nodes()
        + srcComponent->getModel().getN_Y_Nodes();
```

Az ezt követő `for (uns i = 0; i < ncSiz; i++)` ciklus és a függvény
további része változatlan. A sorszám későbbi saját módosítások után
eltérhet; a függvénynév és a régi sor egyértelműen azonosítja a helyet.

## Miért kell?

A `nodesConnectedTo` nem kizárólag a redukált admittanciamátrix portjait
tartalmazza. Az X/Y portok után A vezérlőportok és O belső csomópontok
is lehetnek. A SUNRED levélmátrixának külső mérete viszont X+Y.

Egy A port például a szülő áramkör B változójára csatlakozhat. A régi
ciklus ennek indexét is a szülő fizikai csomópontjai között kereste,
majd a gyermek mátrixának nem létező portját is lekérdezhette. A
reprodukcióban ez hibás kapcsolatszámhoz, illetve futásleálláshoz vezetett.
A javítás az indexelés okát szünteti meg; nem kivétellel kerüli meg.

Az Y portok továbbra is benne maradnak: ezek a fizikai mátrixhoz tartozó
vezérlőfeszültség-oszlopok is lehetnek. Az A portok adatátadása és a
kontrollerek futtatása változatlan. Az O csomópont a gyermek belsejében
redukált csomópont, nem a levélmátrix további külső portja. Ez külön
hiba a korábbi O/NONE állapotelfogadástól; annak javítását nem váltja ki.

## Ellenőrzés és reprodukció

Az optimalizált Core új `tests/sunred_control_ports_runtime.py` próbája
24 esetet futtat: EXTENDED/DOUBLE/AUTO, lapos és SUNRED-es hálózat,
B0 vagy B7 címen átadott jelentés, kétféle vezérlőeltolás. A négy
kiolvasott értéket független másodfokú skaláregyenlethez hasonlítja:
**24/24 eset, 96/96 érték PASS**. Az alap 02a79a8 egy változatlan
SUNRED-es bemeneten a javítás előtt hibával leáll; az alapforrás érintetlen.

További ellenőrzések az új optimalizált binárissal: relatív konvergencia
51/51, SUNRED/FCB 12/12, AC 27/27 futás és 1768 számszerű ellenőrzés.
A GUI teljes falra átlagoló felső/alsó pereme 36 natív futásban és
22 teljes GUI-futási körútban is működött. A nyers kimenetek:
`diagnostics/runs/solver_gaps_20261102/core-control-ports`,
`core-control-baseline-before`, `core-mean-*`, `mean-native-3`, `mean-gui-1`.

A GUI az érintett perem indítása előtt tényleges kis mintafeladatot
futtat a kiválasztott solverrel. A hibás régi bináris használatát így
nem pusztán a fájlnév vagy egy verziószám alapján próbálja felismerni.
