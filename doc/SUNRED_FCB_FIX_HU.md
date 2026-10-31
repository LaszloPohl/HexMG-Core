# FCB a SUNRED-ben: pontos kézi javítás az alap solverhez

2026-10-31. Alapforrás: e724e0d. A sorszámok erre a változatra vonatkoznak;
eltérő forrásban a megadott függvényt és eredeti sort kell megkeresni.
Az alapváltozatot nem módosítottam.

## Ok és következmény

Az FCB a `components` listában van, `ccmt_Function_Controlled_Node` típussal.
Nem vezetést vagy áramot ad a hálózathoz, hanem kiszámít és beállít egy értéket.
A SUNRED mégis betöltötte levélként, és a kapcsolatszámlálásba is beleszámította.
Ha kimenete B-változó, annak indexe már a fizikai N-csomóponttömbön kívül van:
a levél építése érvénytelen memóriaolvasáshoz és félrevezető „Too many
components” hibához vezethet. Egy fizikai csomópontra kötött FCB-bemenet
pedig a kapcsolatszámot növeli úgy, hogy nincs hozzá hálózati egyenlet.

Nem a G/I paraméterbeolvasását kell átírni. Az FCB-t kizárólag a redukció
és a redukció kapcsolatszámlálása hagyja ki. A szokásos iterációs
kiértékelését és állapotelfogadását nem változtatjuk meg.

## 1. Kapcsolatszámlálás

`src/hmgComponent.h`, `setNodesToComponents()`, 3988. sor:

```cpp
if (comp.isEnabled) {
```

helyett:

```cpp
if (comp.isEnabled &&
    comp.pModel->modelType != ccmt_Function_Controlled_Node) {
```

Csak ebben a függvényben cserélendő. A belső csomópontbejárás változatlan.

## 2. SUNRED-levelek betöltése

`src/hmgSunred.cpp`, `hmgSunred::buildTree()`, 1245. sor:

```cpp
if (comp[i]->isEnabled) // only the enabled componets are loaded !
```

helyett:

```cpp
if (comp[i]->isEnabled &&
    comp[i]->pModel->modelType != ccmt_Function_Controlled_Node)
```

A komponensindexeket nem számozzuk újra. A SUNRED `RED` sorai a valódi
hálózati elemek eredeti indexeire hivatkozzanak; FCB-re ne hivatkozzanak.
A GUI-export a cellák és a G/I párok indexét így adja meg.

## 3. Üres levél ciklushatára

Ugyanebben a fájlban, `SunredTreeNode::loadLeafDataFromSubcircuit()`, 46. sor:

```cpp
for (uns i = 0; i < siz - 1; i++)
```

helyett:

```cpp
for (uns i = 0; i + 1 < siz; i++)
```

Így nulla méretnél nincs unsigned alulcsordulás. Ez önmagában nem helyettesíti
az első két javítást, és nem állít általános nulla-portos hálózattámogatást.

## Ellenőrzés

`tests/sunred_fcb_runtime.py`: két G elemmel 10 V felezése, FCB által
számított 2 S vezetés, a kiszámított érték N0-ból olvasva. B0/B3 kimenet,
FCB az elemlista elején/közepén/végén, sűrű és SUNRED út: 12/12 PASS.
A korábbi e1e6507 binárison a hat sűrű próba sikeres, mind a hat SUNRED
próba hibás; a javított binárison mindkettő 5 V.
Lee teljes hálózat: 2319 ellenőrzés / 14 solverindítás, benne két 180-cellás,
60-sávos, 477 közös csomópontos referencia és tranziens energiamérleg PASS.
FUNCTION_RANGE 31, FUNCTION_GUARD 46, relatív konvergencia 51 ellenőrzés PASS.

```powershell
python tests/sunred_fcb_runtime.py --solver <solver.exe> --output <uj-konyvtar>
```

Az optimalizált változat részletesebb hibaszöveget is ad az eredeti
kapcsolatszám-védelmi kivételnél; ez a fenti kézi javításhoz nem szükséges.
