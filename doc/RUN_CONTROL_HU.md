# Optimalizált futásvezérlés — 2026-10-28

A `.RUN ... ITER` egy, az `ITERS n` pontosan n teljes frissítést kér.
A rögzített futás nem áll meg ERR alapján, de nem véges hibamértéknél hibával
leáll. n pozitív, teljes egész szám; a hibás, nulla és túlcsorduló érték elutasított.

```
.OPTIONAL HEXMG_OPT 1
MAX_ITERATIONS 40
.END OPTIONAL
```

Ez az optimalizált solverben a konvergenciáig futó DC és időlépés felső
korlátja, a DC első, ciklus előtti frissítésével együtt. Az automatikus
kezdeti DC-re is vonatkozik. Nem vonatkozik ITER/ITERS-re, AC-re vagy
multigrid-ciklusokra. Hiányában megmarad a régi DC 101 / időlépés 100
frissítéses határ. Egyszer adható meg, pozitív unsigned tartományú egész.
A korlát elérése nem jelent sikert: ha ERR nem teljesül, a futás hibával
leáll és az utána következő SAVE nem hajtódik végre.

A rögzített időlépést megelőző automatikus DC teljes számítás marad;
ERR-je a RunData alapértéke (0.0001), mert a rögzített kérés ERR-jét ignoráljuk.
Az elfogadott időlépés előtti áramfrissítés megmarad; kondenzátor-előzmény
elfogadása után nem számítjuk újra az áramokat.

Az ERR továbbra a régi átlag+maximum, áram- és feszültséghibákból képzett
mérték, nem dimenziótlan relatív tolerancia. A GUI relatív hibaszázalékát
nem szabad közvetlenül erre leképezni. Nemlineáris GUI-indítás még nincs.
Az alap solver OPTIONAL-kihagyással ignorálja a maximumot; a GUI nem
állíthatja, hogy azon ez a korlát érvényesült. A közös ITER/ITERS-javítás
pontos kézi útmutatója a GUI docs/solver/CORE_CONVERGENCE_FIXES_20261027_HU.md.

Ellenőrzés: a GUI tools/solver_baseline/audit_convergence_multigrid.py
--suite convergence --iteration-details --optimized-limits tesztje és
az audit_state_and_probes.py ellenőrzi a vezérlést, RC-előzményeket és probe-okat.
