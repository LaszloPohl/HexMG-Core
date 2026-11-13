# Külön villamos és termikus kezdőállapot, disszipációs deriváltak

2026-11-13. A felhasználó által jóváhagyott új opcionális sor:

```text
.OPTIONAL HEXMG_OPT 1
TRANSIENT_INITIAL_STATE circuit DEFAULT_RAILS
.END OPTIONAL
```

Az első TIMESTEP előtt, illetve explicit TIMESTEP INITIAL esetén a megadott
áramkör szabad csomópontjai a hálózat saját kezdeti referenciasínjeiről
indulnak. A `.DEFAULTRAILRANGE` így a termikus csomópontokat környezeti
hőmérsékletre, a villamos ismeretleneket nullára állíthatja. Az előírt
csomópontértékek változatlanok. A vezérlők, komponensértékek, áramok és
időlépési előzmények frissülnek; előzetes DC-egyensúlyszámítás nincs.

Egy korábbi DC-futás sem helyettesíti ezt a kért indulási állapotot.
A következő, INITIAL nélküli időlépések viszont az elfogadott állapotból
folytatódnak. Egy áramkörhöz egyetlen indulási utasítás adható, a régi
`TRANSIENT_INITIAL_TEMPERATURE` sorral együtt sem duplázható.
Ismeretlen áramkör, más módnév és többlet argumentum hiba.

A régi `TRANSIENT_INITIAL_TEMPERATURE circuit 25` változatlan: minden
szabad csomópontnak ugyanazt a hőmérsékleti számértéket adja. Ez tisztán
termikus hálózathoz való, vegyes elektromos–termikus hálózathoz nem.
Multigridre az új indulási mód sem engedélyezett; a meglévő védelem marad.
Az OPTIONAL blokkot átugró alap solver nem valósítja meg ezt a képességet.

## Disszipációs deriváltak

Az FCID és FCIMD hőkimeneteinek súlya `A`, illetve `1−A`. A párhuzamos
vezetéssel együtt `P=(G V−F)V`. A javítás a tényleges árampecsételés
negatív deriváltjához igazítja a két hősort; az FCIMD második hősorát
nem nullázza le korai visszatérés. A fizikai disszipáció nem változik.

A relatív DC-/időlépési iteráció első frissítése rögzített disszipációs
Jacobi-sorokkal indul, majd teljes Newton-deriváltakra vált. A fizikai
áram és egyenlethiba eközben is pontosan számítódik. Ez az indulás a
feszültségtipp nagy változásakor megakadályozza a negatívra linearizált
Joule-forrást. Az explicit ITER/ITERS és a régi konvergenciaút változatlan.
Az ideiglenes jelző kivétel esetén is visszaáll; új HMG-sor nem kell hozzá.

## Célpróbák

- `tests/initial_state_runtime.py`: két független RC-egyenlet, külön
  0/25 indulás, rögzített 45, újraindítás, módosított referenciasín,
  előzetes DC és hibás szintaxis: **9 eset PASS**.
- `tests/gmd_jacobian_tests.cpp`: tényleges áramok differenciahányadosa,
  FCID/FCIMD X/Y-vezérlés, GD/GMD, három feszültség és három megosztás;
  induláskor is változatlan fizikai áram: **3564 ellenőrzés PASS**.
- Meglévő relatív futásvezérlés: **51 eset PASS**; függvénytartományok:
  **31 eset PASS**.
- GUI-exportból készített közös konvekciós hálózat összevetése külön
  termikus hálózattal: **3006 ellenőrzés / 66 natív futás PASS**.

Példa a két új cél futtatására (a kimeneti könyvtárak legyenek újak):

```text
gmd_jacobian_tests.exe fresh-jacobian
python tests/initial_state_runtime.py --solver hexmg_core_optimized.exe --output fresh-initial
```
