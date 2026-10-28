# Relatív konvergencia: matematikai alap és futási bekötés

2026-10-28. Az optimalizált solverben elkészült az explicit módon
bekapcsolható DC/időlépési futási út. A [részletes leírás és alapváltozathoz
szóló kézi útmutató](BASELINE_RELATIVE_CONVERGENCE_HU.md) az aktuális szerződés.

`src/hmgConvergenceMetric.h` fizikai alapértékek nélkül számol:

```
állapothiba = abs(tényleges változás) / (abszolút állapotküszöb + relatív * referencia)
egyenleghiba = abs(fizikai egyenleg) / (abszolút egyenlegküszöb + relatív * abszolút ágáramösszeg)
csomópont hibája = max(állapothiba, egyenleghiba)
hálózat hibája = max(minden csomópont hibája)
```

A konvergencia feltétele legfeljebb 1. A referencia és az abszolút küszöb
azonos egységű. A segéd nem ismer Celsius/Kelvin, V/A/K/W típusokat;
azokat a hívó adja meg. Nincs rejtett küszöb vagy skála.
A nem véges bemenet, negatív referencia, nem pozitív tolerancia és
nem ábrázolható küszöb elutasított. Osztási túlcsordulás végtelen hibát ad,
amely nem konvergens. A nulla elemszámú gyűjtő matematikailag konvergens;
a futási réteg ettől külön köteles a lefedettséget ellenőrizni.

39 natív /W4 /WX próba: pontos küszöb, előjel, nulla referencia,
1e-9/1/1e9 egységváltás, egymást kioltó ágáramok, két feltétel ÉS-kapcsolata,
egy rossz csomópont egymillió pontos mellett, NaN/Inf/hibás tolerancia,
túlcsordulás. CMake BUILD_TESTING mellett `convergence_metric_tests`,
CTest-név: `convergence_metric`.

## Megvalósult futási bekötés

- A `NodeVariable` tényleges változása `getValueDC()-getAcceptedValueDC()`;
  a nyers `getVDC()` nem veszi figyelembe az alpha csillapítást.
- A `CalculateValuesAndCurrentsDC` sorrendje: kontrollerérték,
  komponensérték, deleteD, calculateCurrent. Előtte külön nullázódik az
  új fizikai referenciaösszeget; redukció vagy multigrid nem nullázhatja.
- A `calculateCurrent` fizikai bélyegzését műszerezése készült el, nem az általános
  incDDC-t. A komponensfejléc 54 incDDC-előfordulása nem jelent 54 egyforma
  ágat: többkapus elemek és segédegyenletek is vannak. A feladat nem
  oldható meg vak szövegcserével.
- A gyűjtők most noexcept függvények. A dobó új segédet nem szabad
  közvetlenül ezekbe hívni, mert az std::terminate-hez vezetne. Külön,
  nem noexcept validálási út dob a Simulation határán,
  csomópont-/komponensazonosítóval.
- Az új egyenleghiba az új állapot fizikai áramfrissítése UTÁN értendő.
  Időlépésben a kondenzátor előző állapotát AcceptStepDC-ig meg kell őrizni.
- A generikus Core nem tudja a csomópont fizikai típusát. Metadata nélküli
  hálózaton nem lehet automatikusan K/W-ként értelmezni a számokat.
- A fent felsorolt feltételek teljesülnek a külön `runRelativeDC` útban.
  A régi `DefectCollector` és a relatív mód nélküli ERR megmaradt.
  `CONVERGENCE RELATIVE` adja a globális profilt, `CONVERGENCE_NODE` az
  egyedi csomópontprofilt. A potenciálváltozás referenciája az előírt
  környezethez / 0 V-hoz viszonyított új és régi érték abszolút maximuma.
  A gyűjtés kiterjed a kivezetett O csomópontokra és a VIB/MIB saját
  áramváltozóira; a megosztott csomópontokat egyszer értékeljük.
- 51 valódi CLI/HXIM célpróba és a korábbi 42 állapot/probe próba az új
  módban is sikeres. A kézi recept külön alapmásolatán ugyanezek is sikeresek.
  Az FCI meglévő numerikus deriváltja miatt a Newton-pálya nem bitazonos
  egységváltáskor. Multigrid relatív megállása nem támogatott.

## Anyagérvényesség

A GUI exportja a megadott minimum/maximum hőmérsékletű törvényeket ma
elutasítja; ez a védelem megmarad. A későbbi futási ellenőrzésnek a
cellaközépponti hőmérséklet mellett a tényleges lambda(T)>0, véges értéket
is ellenőriznie kell. A nulla hőmérsékletkülönbségből adódó nulla ágáram
nem bizonyítja a vezetőképesség helyességét. Például egy negatív, de véges
lambda is adhat véges áramot; a NaN-védelem ezt nem helyettesíti.
A reciprok függvény nevezője, a folytatott exponenciális előjele, a
kezdeti állapot és minden későbbi iteráció külön vizsgálandó. A validálás
nem helyettesítheti a hibás értéket nullával és nem clampelhet észrevétlenül.
A numerikus deriválás próbaállapotának kezelését az elfogadott fizikai
állapot ellenőrzésétől meg kell különböztetni a bekötéskor.
