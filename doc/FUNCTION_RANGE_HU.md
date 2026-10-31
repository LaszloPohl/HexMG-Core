# FUNCTION_RANGE – jóváhagyott opcionális tartományvédelem

2026-10-31, csak az optimalizált solverben. Az alap solver az OPTIONAL
blokkot kihagyja; a közönséges fizikai modell változatlanul szükséges.

```text
.OPTIONAL HEXMG_OPT 1
FUNCTION_RANGE LEE0G P0 25 *
FUNCTION_RANGE LEE0G V2 200 2000
FUNCTION_RANGE LEE0G RET 0 *
FUNCTION_RANGE LEE0S RET * *
.END OPTIONAL
```

Forma: `FUNCTION_RANGE függvény Pn|Vn|RET alsó felső`.
A határok inkluzívak, `*` az adott oldali korlát hiánya. NaN/Inf mindig
hiba. A mértékegység a függvény szerződéséből származik, nincs konverzió.
Ugyanahhoz a függvényhez több külön cél írható; ugyanaz a cél kétszer
elutasított (P0 és P00 is azonos cél). Ismeretlen függvény, nem deklarált
P/V-index, hibás szám és fordított határ már beolvasáskor hiba.

P ellenőrzése a függvénybe lépéskor történik, módosítás előtt; V és RET
ellenőrzése kilépéskor, `_RETURN` esetén is. A figyelt lokális változó
NaN-nal indul, így az elmaradt értékadás nem használhat korábbi állapotot.
A RET is így indul, kivéve a helyben végzett hívást, ahol RET egyben
bemeneti paraméter: annak már definiált értékét nem szabad felülírni.
A numerikus derivált szintetikus próbaállapotait nem tekintjük fizikai
állapotnak; a nem véges derivált meglévő ellenőrzése megmarad.

A noexcept függvénykiértékelő rögzíti a hibát; a külső vezérlő-/érték-
frissítés dob kivételt még redukció vagy időlépés-elfogadás előtt. A hiba
tartalmazza a függvény nevét, célját, értékét, tartományát és a komponens
útvonalát. Hibás P esetén a függvénytörzs nem fut le. Hibás V/RET esetén
a következő redukció nem indulhat el. AC és multigrid jelenleg védetten
elutasított, ahogyan a korábbi FUNCTION_GUARD esetén is.

A régi FUNCTION_GUARD megmaradt; ugyanazon függvényen is használható.
A FUNCTION_RANGE önmagában nem konvergenciakritérium, és nem tesz
fizikailag érvényessé egy nem megfelelő korrelációt.

Ellenőrzés: `tests/function_range_runtime.py --solver <exe> --output <új mappa>`.
31 parser/futási próba PASS a GUI Release és Qt Creator mellett is:
határok, nulla/előjeles érték, NaN/Inf, hiányzó értékadás, korai kilépés,
beágyazott helyben hívás, késői OPTIONAL, régi/új védelem együtt,
DC-frissítés és tranziens lépés során keletkező hiba. A hibás futások
nem írják ki az utána következő HXIM-et. További regresszió:
FUNCTION_GUARD 46/46, relatív konvergencia 51/51, OPTIONAL CTest PASS.
