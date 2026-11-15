# Tömbös kontrollerek — BATCHCONTROLLER

2026-11-15. A felhasználó által jóváhagyott kötelező HMG-utasítás;
az optimalizált `codex/optimized-solver` ág kiegészítése.

```text
.CREATE circuit MODEL GND=R0
.BATCHCONTROLLER optika circuit TYPE=OPTICAL_RAYS CTRL_LEVEL=1 FILE="optika.hxray"
```

Az első név a kontrolleré, a második a már létrehozott teljes áramköré.
A TYPE, CTRL_LEVEL és FILE mind kötelező, egyszer adható meg. A paraméterek
sorrendje szabad; a fájlnév idézőjeles UTF-8 útvonal. Relatív út esetén az
utasítást tartalmazó HMG-fájl könyvtára a kiindulópont. Az utasításnak az
érintett futás előtt kell szerepelnie. Egy áramkörön belül a tömbös
kontrollerek neve nem ismétlődhet.

Ismeretlen típus, hiányzó/sérült fájl vagy hibás csomóponti kapcsolat hiba.
Ez nem OPTIONAL-adat: kihagyása megváltoztatná a fizikát. Az alap solver
számára a GUI a meglévő CONTROLLER/FCI elemekkel írja le ugyanazt a modellt.

## Miért általános az utasítás neve?

A közös működés: hálózati értékek beolvasása, sok hasonló elemen végzett
tömbös számítás, majd az eredmény visszaadása a hálózatnak. Ez nem kizárólag
optikai feladat. A neutrontranszport karakterisztikus útjai, a vízhálózati
anyagcsomagok és a részecske–rács számítások is tartalmaznak rokon mintákat.
Ez szerkezeti analógia; nem jelenti azt, hogy az optikai számítómodul
ezeket a fizikákat is megoldja.

Elsődleges források:

- [OpenMC: Random Ray és karakterisztikus utak](https://docs.openmc.org/en/stable/methods/random_ray.html)
- [US EPA EPANET: vízminőségi anyagcsomagok](https://github.com/USEPA/EPANET2.2/blob/master/User_Manual/docs/12_analysis_algorithms.rst)
- [WarpX: részecske–rács számítás](https://warpx.readthedocs.io/en/25.11/theory/pic.html)

A közös `BatchController` életciklus bővíthető, de jelenleg kizárólag
`TYPE=OPTICAL_RAYS` létezik. Nincs általános felhasználói programnyelv vagy
tetszőleges dinamikus könyvtár betöltése.

## Ütemezés és csomópontok

Minden szinten előbb valamennyi hagyományos és tömbös kontroller beolvassa
a bemeneteit; csak ezt követik az adott szint írásai. A következő szint már
a friss értékeket olvassa. A GUI exportjában:

1. 0. szint: a hőmérsékletfüggő optikai anyagfüggvények.
2. 1. szint: sugárutak, elnyelt hő, új sárga fény és kilépő teljesítmény.
3. Magasabb szintek: szükséges összegzések.

A fájl csomópontcímei a megadott teljes áramkörhöz viszonyított szokásos
mély címek. Bemenet lehet véges konstans vagy hálózati érték; kimenet csak
B munkaváltozó. Kontrollercím és időlépéskezdő cím jelenleg nem támogatott.
Ugyanaz a kimeneti cím egy fájlon belül nem szerepelhet kétszer, akkor sem,
ha eltérő szöveges alak ugyanarra a csomópontra mutat.

A kontroller a tényleges DC-értéket olvassa, nem a Newton-korrekciót.
A sugárprogram egyszer töltődik be, majd változatlan memóriából fut;
az ismételt kiértékelés újrahasználja a munkatömböket. Nem tárol tartós
nyers mutatót a hálózat csomópontjára. Az optikai függvények a Core
meglévő `rvt=double` függvény-/kontroller-pontosságát használják.

DC és időtartomány támogatott. A tömbös kontrollernek nincs saját fizikai
időtárolása. A külön sárga fény visszacsatolását a GUI hálózati ismeretlennel
adja meg, így arra is vonatkozik a konvergencia. AC, időállandó-spektrum és
multigrid ezzel a típussal szabályos hibát ad; ezekre nem teszünk hallgatólagos
közelítést. A GUI közvetlen optikai AC-exportja külön mátrixos adatút.

## HXRAY001 bináris elrendezés

Little-endian, explicit szélességű mezők; a C++ struktúrák paddingje nem
része a fájlnak. Valós szám: IEEE-754 binary64. A fájl nem tömörített.

| Sorrend | Tartalom |
|---|---|
| 1 | 8 bájt ASCII `HXRAY001` |
| 2 | uint64 cellaszám, double kék/sárga úthosszszorzó, uint32 külön-sárga jelző |
| 3 | uint64 bekötésszám; bekötésenként uint32 UTF-8 címhossz, cím, double konstans |
| 4 | uint64 cellabekötésszám; cellánként négy uint32 bemenet és három uint32 kimenet |
| 5 | uint64 kékforrásszám + uint32 bekötésindexek, majd ugyanígy a sárga források |
| 6 | 14 uint32 kimeneti bekötésindex: oldalanként kék/sárga, végül két levágási kimenet |
| 7 | uint64 sugárszám; sugaranként uint64 kezdet, négy uint32 (darabszám, forrás, ágak, színsáv), double súly |
| 8 | uint64 utasításszám; utasításonként uint32 művelet és index, double érték |
| 9 | uint64 FNV-1a ellenőrző összeg az összes megelőző bájtra |

Üres bekötési cím esetén a konstans érvényes. Hiányzó kimenet: `UINT32_MAX`.
A cella négy együtthatója: kék és sárga elnyelés [1/m], kék átalakítási
és sárga újrakibocsátási energiaarány. A három kimenet: összes elnyelt hő,
ennek falelnyelési része, valamint új sárga fény [W]. A falhőt nem szabad
még egyszer az összes hőhöz adni.

Műveletkódok: 0 Branch, 1 Segment, 2 Split, 3 Wall, 4 Escape, 5 Cutoff.
A Segment indexe a cella, értéke az úthossz. A Split indexe a későbbi
átengedett ág, értéke a visszavert hányad. A Wall indexe az elnyelő cella,
értéke a megmaradó visszavert hányad. Az Escape indexe 0–5 (X−, X+, Y−,
Y+, Z−, Z+). A Branch és a termináló műveletek értéke nulla.

Az olvasó kiosztás előtt ellenőrzi, hogy a darabszám belefér-e a maradék
fájltartalomba, majd ellenőrzi a tartományokat, ágak sorrendjét és lezárását,
az indexeket és a véges értékeket. A címhossz legfeljebb 4096 bájt. Az
ellenőrző összeg sérülésészlelésre szolgál, nem hitelesítésre.

## Ellenőrzés és teljesítmény

- Közös kontrollerszint, beolvasás/írás sorrend, dekódolt adat élettartama:
  `batch_controller_tests`, 20 ellenőrzés PASS.
- Hibás típus, kötelező mező, szint, fájl, checksum, levágott tartalom,
  index, kimenettípus, ismétlődés, futás közbeni anyaghiba és AC-elutasítás:
  GUI-tároló `tests/batch_controller_input_checks.py`, 17 eset PASS.
- Termelési optikai hálózat: 1938 ellenőrzés / 14 natív futás PASS.
- Hőmérsékletfüggő elnyelés és visszacsatolás: 46 ellenőrzés / 4 futás
  PASS, külön implicit hőegyenlet referenciájával.
- Egymillió, nyolc cellaszakaszos sugár: 69,8–73,7 ms/kiértékelés,
  192 000 000 bájt programadat. Ez a számítómodul mérése, nem teljes
  hálózatfutás: az előkészítés, beolvasás és hálózatmegoldás nincs benne.

A nagy CoB-modell teljes sugártervének erőforrásigénye külön nyitott munka.
A szintetikus mérés önmagában nem igazolja azt a modellt.
