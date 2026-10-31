# AC amplitúdó és feszültségforrásáram javítása

2026-10-31. Az optimalizált változatban megvalósítva; az alapváltozatba
kézzel építendő be. A sorszámok a hivatalos `e724e0d` változatra vonatkoznak,
**a módosítások előtti számozással**. Eltérő forrásnál a megadott osztályt,
függvényt és régi sort keresd. Nincs új HMG-utasítás vagy OPTIONAL-beállítás.

## AC01 – hiányzó vezetőképesség a V forrás AC-áramában

**Fájl:** `src/hmgComponent.h`, **1366. sor**.
**Hely:** `ComponentConstV::calculateCurrent(bool isDC)`, az `else` ágban,
közvetlenül az `IAC = { A * cos(Phi), A * sin(Phi) };` után.

Ezt:

```cpp
IAC += N1->getValueAC() - N0->getValueAC();
```

cseréld erre:

```cpp
IAC += G * (N1->getValueAC() - N0->getValueAC());
```

A forrás X0 kapcsán kifelé pozitív ágáram helyesen
`I = G · (E + U1 − U0)`. A komplex gerjesztést már megszorozta G-vel
az `A = param[2].get() * G` sor, a kapocsfeszültség tagját viszont nem.
Így a megoldó a helyes G vezetésmátrixhoz hibás áramegyenleghibát kapott.
Ez mértékegység szerint is hibás: az áramhoz feszültséget adott hozzá.

A nulláról indított első AC-pont feszültsége még helyes lehet, de a forrás
jelentett árama már ekkor is hibás. A második AC-számítás a megmaradt
komplex állapotból indul; annak hatását a hibás áramszámítás nem vonja le
helyesen. Nagy G esetén ebből közel 2×, 3×, 4× amplitúdó lesz. G=1 S mellett
a hiba véletlenül eltűnik, ezért ezt az esetet önmagában nem elég tesztelni.

**Nem kell módosítani** a `Simulation::runAC` hívási sorrendjét vagy a
`setValueAcceptedAC` összeadását. A megoldó a korrekciót számolja ki:
`Y · ΔU = J − Y · Urégi`; elfogadáskor `Uúj = Urégi + ΔU` helyes.
A csomópontok teljes nullázása csak elfedné a hibát, és a `resetAC()` jelenleg
a DC munkapontot és az időlépési előzményt is alaphelyzetbe állítaná.

A `TIMECONST` ugyanezt a komplex megoldási utat használja; a javítás ott is
érvényes, a komplex frekvencia megválasztása változatlan.

## AC02 – paraméterváltozó miatti hozzáférési hiba

**Fájl:** `src/hmgCircuitNode.hpp`, **181. és 197. sor**.

A közönséges B paraméternek nincs elektromos egyenlete, így nem feltétlenül
kap `extension` munkaterületet. A közös csomópont–változó tömb AC-törlése és
állapotelfogadása mégis meghívja ezeket a műveleteket. A null mutató
elérése már a számítás elején leállíthatja a folyamatot. Nem kell a B
változóból mesterséges elektromos csomópontot létrehozni.

A 181. sort:

```cpp
void deleteDAC() { if (!isGnd) extension->d.store(cplx0, isConcurrent); }
```

cseréld erre:

```cpp
void deleteDAC() { if (extension && !isGnd) extension->d.store(cplx0, isConcurrent); }
```

A 197. sort:

```cpp
void setValueAcceptedAC()noexcept { extension->valueAC += extension->v; extension->v = cplx0; }
```

cseréld erre:

```cpp
void setValueAcceptedAC()noexcept { if (!extension) return; extension->valueAC += extension->v; extension->v = cplx0; }
```

Ez ugyanaz az elv, mint a már létező `deleteDDC` és `setValueAcceptedDC`
ellenőrzése. A B értéke megmarad, és a valódi elektromos csomópontokon
semmi nem változik. Ez nem teszi megoldhatóvá a lebegő vagy fölöslegesen
deklarált, szinguláris elektromos hálózatot; a próbamodellben az N darabszám
a tényleges elektromos csomópontok száma legyen.

## AC03 – kapcsolódó DC forrásáramhiba

**Fájl:** `src/hmgComponent.h`, **1381. sor**.
**Hely:** `ComponentConstV::getCurrentDC`.

Ezt a teljes sort:

```cpp
rvt getCurrentDC([[maybe_unused]] uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() * param[4].get() : componentCurrent.getValueDC() * param[4].get(); }
```

cseréld erre:

```cpp
rvt getCurrentDC([[maybe_unused]] uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
```

A `calculateCurrent(true)` már kiszámítja és eltárolja a G-vel megszorzott
áramot. A lekérdezés ezt másodszor szorozta meg. Ez a módosítás a DC-/időlépési
kapocsáram-lekérdezést és az abból képzett aláramköri áramot javítja; a
csomóponti DC egyenletbe pecsételt áram és a tárolóelemek időbeli működése
nem változik. Az X0/X1 előjel továbbra az alkatrészből kifelé pozitív.

## Rövid kézi ellenőrzés

Mentsd új könyvtárba, a sorok elé ne kerüljön szóköz:

```text
HEXMG 1.0
.MODEL M SUBCIRCUIT N=1 B=1
VS V N0 GND 3 7 B0 0 4
RL R N0 GND 10
.END MODEL M
.CREATE MAIN M GND=R0
.PROBE U V MAIN N0
.PROBE IP I MAIN VS.X0
.PROBE IM I MAIN VS.X1
.RUN MAIN DC INITIAL ITER
.SAVE RAW FILE="dc.res" U IP IM
.SET MAIN.B0 1
.RUN MAIN AC F=100
.SAVE RAW FILE="ac.res" U IP IM
.RUN MAIN AC F=100
.SAVE RAW APPEND FILE="ac.res" U IP IM
.SET MAIN.B0 0
.RUN MAIN AC F=100
.SAVE RAW APPEND FILE="ac.res" U IP IM
.END
```

`dc.res`: körülbelül `2.92683  0.292683  -0.292683`.
`ac.res` első és második sora azonos:
`100  0.97561  0  0.097561  0  -0.097561  0`.
A harmadik sorban a frekvencia után nulla értékek várhatók, lebegőpontos
kerekítési maradék megengedett. B nélkül is szükséges az AC01 és AC03;
a B változó az AC02 próbáját teszi bele ugyanebbe a kis példába.

## Automatizált ellenőrzés és korlátok

```powershell
python tests/ac_runtime.py --solver <solver.exe> --output <uj-konyvtar>
```

27/27 valódi solverfutás, 1768 számszerű összehasonlítás sikeres. Lefedés:

- RC és RL, sík és aláramköri hálózat; G=0,25 / 1 / 4 / 10⁹ S;
- emelkedő, csökkenő és azonos frekvencia; fázis, amplitúdó és nullázott forrás;
- mindkét V kapocsáram és a tárolóelem árama, önálló áramforrásos kontroll;
- helyi mátrixos és SUNRED-redukció, AC/TIMECONST váltás;
- DC forrásáram, kezdeti DC, nyolc Euler-időlépés közé beszúrt 24 AC-pont.

A referencia a komplex impedanciákból és a hátrafelé vett Euler egyenletből
számol, nem a solver kimenetét másolja. A RAW hat értékes számjegyét és
G=10⁹ S forrásnál a közel azonos nagy áramtagok kivonási hibáját figyelembe
veszi. Az alap e724e0d ugyanezen végleges próbákból 7/27-et teljesít;
a B-s esetek hozzáférési hibával állnak le.

Kapcsolódó regresszió: relatív konvergencia 51/51, anyagvédelem 46/46,
mindkét natív CTest-cél sikeres. A régi pontossági sor 36/39:
`cancellation_None`, `cancellation_EXTENDED`, `cancellation_AUTO` ugyanúgy
hibás a módosítás előtti aaeb820 binárissal is. Ezek korábbi, nagy
kioltásnál fennmaradó konvergenciaproblémák, nem az AC-javítás regressziói.
Nem állítunk általános AC-/multigrid-/nemlineáris tanúsítást.

A javítás nem változtatja meg az AC `TAU=` korábbi korlátját, a HMG formátumot
vagy a GUI elektromos analízis-bekötésének hatókörét.
