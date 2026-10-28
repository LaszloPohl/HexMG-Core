# Relatív konvergencia — pontos kézi beépítés az alap solverbe

2026-10-28. Kiindulás: `fc8bcc55342a7823d9ebb06ff1933c22e9e2d3a0`.
Az érintetlen alapágat nem módosítottuk. A kézi sorcserékből külön helyi,
kísérleti példány készül ellenőrzésre; az nem kerül a GUI mellé.

## Mit kell átvenni?

A [fájlonkénti sorcserejegyzék](BASELINE_RELATIVE_LINE_EDITS_HU.md)
az összes szükséges változtatást tartalmazza, eredeti sorszámmal és
környezettel. A két új fejléc teljes szövege is ott van. A `+` és `-`
jelölések a kézi útmutató részei, nem kerülnek a C++ fájlokba.
Nem kell más optimalizációt vagy ágat átvenni.

Sorrend:

1. Új `src/hmgConvergenceMetric.h`: dimenzió nélküli hibaarány és maximum.
2. Új `src/hmgOptimization.h`: **a sorcserejegyzékben szereplő alapváltozatot**
   használd. Ez csak a konvergencia és iterációkorlát beállításait kezeli;
   a pontosság- és szálkezelési sorokat kihagyja. A név az azonos hívási
   felület miatt maradt meg; nincs benne új redukció vagy párhuzamosítás.
3. `hmgCircuitNode.hpp`: fizikai referenciaösszeg, profil és két mérési
   függvény. A régi `incDDC` változatlan marad. Ne abból gyűjtsd az
   abszolút áramösszeget, mert az elimináció is használja.
4. `hmgComponentModel.h`: az `InternalNodeSizePack` csak olvasható lekérdezése.
5. `hmgComponent.h`: a jegyzékben felsorolt fizikai áramhozzáadások cseréje,
   külön forrás-/mérőegyenletek, a megoldott csomópontok összegyűjtése.
   A `VIB` és `MIB` saját áramváltozója és a részáramkör `O` kivezetései
   is szerepelnek. A `B` vezérlőváltozók nem KCL-egyenletek, nem kerülnek bele.
   A végén a három állapotelfogadó ciklus is változik: az összes helyben
   birtokolt csomópontot kell elfogadni, a `NONE`-hoz allokált `O` portot is.
6. `hmgHMGFileReader.cpp`: az opcionális blokk gyűjtése és értelmezése,
   csomópontnevek ellenőrzése a teljes beolvasás után, szigorú `ITERS` olvasás.
   Az első, ugyanabban a fizikai fájlban lévő `.END OPTIONAL` zár.
7. `hmgSimulation.h` és `.cpp`: a jegyzék teljes sorcseréi. Ezek **tartalmazzák
   a korábbi S10c javítást is**, tehát azt nem kell még egyszer rájuk alkalmazni.
   A DC/időlépés pozitív `ITERS` esetén pontosan a kért számú frissítést végzi;
   normál futásban a két normalizált maximum külön-külön legfeljebb 1 lehet.
   A hibás/nem véges állapot a fix iterációs futásban sem elfogadható.

A `main`, a reduktorok, a fájlmentés és az AC-megoldó változatlan marad.
A hibát a meglévő, 1-es kilépőkódot adó kivételkezelés jelenti.
Ha saját forrásod már eltér, a függvénynevek/környezeti sorok alapján illeszd;
a megadott sorszámok kizárólag a fenti commitra érvényesek.

## A megállási feltétel és a sorrend

Minden megoldott potenciálcsomópontnál:

```text
állapothiba = abs(új - régi) /
             (állapot_abs + ERR * max(abs(új - referencia), abs(régi - referencia)))
egyensúlyhiba = abs(új állapot csomóponti áramegyenlege) /
               (egyensúly_abs + ERR * sum(abs(új állapot fizikai ágáramai)))
```

Mindkettőből a hálózat maximumát vesszük. Nem átlagolunk és nem adunk össze
V²/A² vagy K²/W² mennyiségeket. Termikus referencia a környezet
hőmérséklete, elektromos referencia 0 V. A tolerancia kezdőértéke
1e-6 K/V és 1e-12 W/A; a HMG ezeket explicit adja meg.

Egy iteráció sorrendje: fizikai kiértékelés → lineáris megoldás →
állapotváltozás mérése → `AcceptIterationDC` → kontrollerek/alkatrészek/
áramok újraszámítása → egyensúlyhiba mérése. A `ForwsubsBacksubsDC`
előtti első fizikai kiértékelés egyszer történik, utána az előző iteráció
végi kiértékelést használjuk. `AcceptStepDC` csak a végén történik.
Így a kondenzátor még a korábbi időpont állapotából számol áramot.

Az ideális feszültségforrás és árammérő segédváltozója áram: ott az
állapottolerancia A/W, a kényszeregyenlet toleranciája V/K. A két abszolút
küszöb ezért felcserélődik; a referencia 0. A `GM/GMD/FCIM/FCIMD`
árammérő változójánál mindkét abszolút küszöb az áramküszöb.
A kényszeregyenlet referenciája a terminálkülönbség és az előírt érték
abszolút összegéből képződik, nem a közel nulla maradékból.

## HMG-példa és használat

25 °C környezetű termikus hálózat, 0,5% relatív küszöb:

```text
.OPTIONAL HEXMG_OPT 1
CONVERGENCE RELATIVE 25 1e-6 1e-12
MAX_ITERATIONS 100
.END OPTIONAL
.RUN circuit DC INITIAL ERR=0.005
```

A `CONVERGENCE RELATIVE` után: referencia, állapot abszolút toleranciája,
egyensúly abszolút toleranciája. Véges számok, pozitív toleranciák;
ponttal írt decimális vagy tudományos alak, SPICE-utótag nélkül.
A beállítások a teljes bemenetre érvényesek, nem időben végrehajtott utasítások.
Az `ERR` futásonként adható meg, **arány**, nem százalékszám.

Vegyes hálózatban például a globális referencia 0, és a termikus csomópontok:

```text
CONVERGENCE_NODE circuit CELL12.N0 25 1e-6 1e-12
```

A név ugyanúgy mély csomópontnév, mint a feszültség-probe-nál.
Egy fizikai csomópontra csak egy profil adható, különböző aliasokon sem
ismételhető. Sín, GND, vezérlőváltozó, STEPSTART nem megengedett.
Az automatikus segédegyenletprofil az első potenciálkapu explicit profilját,
ennek hiányában a globális profilt használja. Ha ez vegyes hálózatban nem
megfelelő, a segédváltozóra külön profil kell, annak tényleges egységeiben.
Legfeljebb 1 000 000 sor és 64 MiB tartalom lehet egy értelmezett blokkban.

Relatív mód nélkül az `ERR` régi jelentése marad. A hivatalos, még nem
módosított alap solver továbbra kihagyja az opcionális blokkot; ezért nála
a GUI nem állíthatja, hogy relatív kritérium vagy maximum érvényesült.
Multigrid relatív megállása nincs megvalósítva: ilyen kérés világos hibával
leáll. Az AC közvetlen komplex megoldását ez nem alakítja iteratívvá;
az előtte szükséges DC munkapont már az új feltételt használja.

## Ellenőrzés és korlátok

A valódi CLI/HXIM próbák a `tests/relative_runtime.py` fájlban vannak.
Független köbös egyenlet, 1e-9/1/1e9 egységskála, 25/3000 környezeti
referencia, több csomópont, vegyes profilok, `O` portok, ideális források,
fix lépésszám és hibás bemenet szerepel bennük. Az RC/induktivitás/girátor
és nemlineáris időlépés ellenőrzése a GUI saját állapot/probe próbájával történik.

Az FCI numerikus deriváltjában meglévő fix `1e-10` eltolás miatt az
egységváltás nem ad bitazonos Newton-pályát; ezt a változtatás nem módosítja.
A teszt azonos megállási lépésszámot, helyes végeredményt és mindkét
normalizált feltételt ellenőrzi. Az anyagtörvény érvényességi tartományának
ellenőrzése külön feladat; a véges, de fizikailag érvénytelen értéket a
konvergenciakritérium önmagában nem ismeri fel.
