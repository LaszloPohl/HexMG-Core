# Dióda nyitóirányú Newton-lépésének korlátozása

Jóváhagyott opcionális utasítás, 2026-11-13:

```text
.OPTIONAL HEXMG_OPT 1
NEWTON_FORWARD_LIMIT circuit anode cathode 0.2
.END OPTIONAL
```

A két kapocs feszültségkülönbségének **pozitív korrekcióját** korlátozza
egy DC- vagy időtartománybeli iterációban. A negatív korrekció változatlan.
A korlát véges, pozitív szám voltban. Kapocs lehet belső vagy kivezetett
csomópont, beágyazott komponens portja, illetve rögzített sín.
Állapotváltozó és vezérlőváltozó nem választható. Azonos szöveges
kapocspár ismétlése hibás bemenet; fordított pár külön korlát lehet.

Több korlát esetén a legkisebb szükséges tényező szorozza a teljes áramkör
összes megoldott ismeretlenének korrekcióját, az eliminált belső és
áramcsomópontokat is. Az azonos csomópontra vezető több port csak egyszer
szorzódik. A rögzített értékek, a végleges karakterisztika és az időlépés
elfogadásáig megőrzött előzmények nem változnak. AC-ra nem vonatkozik;
opció nélkül nincs új munka a Newton-lépésben.

Ellenőrzés: `tests/newton_forward_runtime.py`, 25 eset: közös csökkentés,
beágyazott és O/NONE port, több korlát, előjel, rögzített sín, két áramkör,
régi/relatív megállás, hibás bemenet és két RC-időlépés. A meglévő relatív
regresszió 51/51. A GUI külön analitikus ellenpárhuzamos diódapróbájában
66/66 SUNRED és 66/66 belső áramcsomópontos eset megfelelt, változatlan
feszültség- és áramtűréssel. Ez nem általános konvergenciabizonyítás.

Az alap solver az OPTIONAL blokkot kihagyja; az integráló GUI köteles
ellenőrizni a képességet, ha a működéséhez szükséges. A teljes fizikai
egyenlet továbbra is a szokásos HMG-elemekben és függvényekben szerepel.
