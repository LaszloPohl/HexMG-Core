# Optimalizált solver – ötletek és feladatok

2026-10-27. Élő lista, új ötletekkel bővítendő. Az alábbi képességek
állapota soronként jelölt; kész gyorsulást nem következtetünk puszta típusválasztásból. Külön solverágon fejlesztjük őket;
az alap- és optimalizált változat későbbi összevonása sincs tervben.
A hivatalos ág fejlesztéseit ettől függetlenül követjük, és szükség szerint
egyirányúan átvesszük az optimalizált ágba. A közös HMG/fizika/kimenet szerződés
megmarad, a GUI mindkét változattal használható marad.

| ID | Részfeladat | Következő konkrét eredmény / feltétel | Állapot |
|---|---|---|---|
| OPT-01 | 0. szintű DC-redukció dd_real/double választása | Típusos helyi EXTENDED/DOUBLE/AUTO kész, referenciaellenőrzéssel. Következő: a kettős kiértékelés költségének igazolt kiváltása, majd sebességmérés. | Első funkcionális változat kész; gyorsulás még nem igazolt. [Részletek](LEVEL0_PRECISION_HU.md) |
| OPT-02 | Specializált cellamodell-családok | Külön modell rögzített és időben átváltható peremhez, szükséges fizikai mezők és csomópontok szerint. Minden modellhez teljes hagyományos HMG-leírás és független referencia. | Tervezett |
| OPT-03 | Teljesen lineáris modell: egy megoldás | Bizonyított linearitásnál DC-ben egy megoldás, időtartományban egy megoldás/megkísérelt időlépés. Az állapotelfogadás és mérő-/áramfrissítés megmarad. | Felhasználói új ötlet |
| OPT-04 | Egyirányúan csatolt elektrotermikus modell szétválasztása | Elektromos megoldás → hőforrások → termikus megoldás; külön kisebb hálózatok, megőrzött cím- és eredményhozzárendelés. | Felhasználói új ötlet |
| OPT-05 | Lineáris mátrix/faktorizáció újrahasználata | Azonos mátrix mellett csak új jobb oldal; pontos érvénytelenítés paraméter-, dt-, topológia-, perem- vagy pontosságváltáskor. | Tervezett |
| OPT-06 | Párhuzamosítás | Független cellák/részfeladatok ütemezése, korlátozott munkaszálszám, tisztázott memória- és redukciós sorrend. | Tervezett |
| OPT-07 | Konvergencia és végrehajtási módok rendezése | S10 ERR/ITER/ITERS, hibás értékek, tényleges konvergencia és lineáris rövid út külön szerződése. | Korábbi auditból, újraellenőrzendő |
| OPT-08 | Folyamat- és eredménymentés hibakezelése | S03 quick_exit helyett rendezett kilépés/mentőszál; fwrite/fclose hibák, megszakítás, ép részleges képek megőrzése. | Korábban halasztott, most engedélyezett |
| OPT-09 | Többáramkörös metaadatok | S07 és futás-/rács-/mérőazonosítók helyes áramkörhöz kötése. A GUI ezt jelenleg nem igényli. | Alacsonyabb prioritás |
| OPT-10 | Üres tárolók és élettartam | A korábban megjelölt &vector[0] helyek auditja, szükséges data()/üreság-kezelés; aktuális kód ellenőrzése javítás előtt. | Korábbi auditból |
| OPT-11 | Multigrid helyesség, majd teljesítmény | HGrid, korrekciós forráselőjel, cikluskilépés, független DC/AC referenciák; az előző helyi 24/24 nem általános multigrid-igazolás. | Későbbi nagyobb feladat |

## OPT-01 – miért külön megvalósítás?

A jelenlegi `SubCircuitFullMatrixReductorDC` (`hmgComponent.h`) valamennyi
munkamátrixa/vektora `xxt`, amely `hmgCommon.h` alapján jelenleg `dd_real`.
A közös `HMG_PRECISE` átállítása nem csak egy alapcellára vonatkozna.
A `SubCircuitFullMatrixReductorAC` már most `cplx`, azaz komplex double:
ennek a pontossági útja külön vizsgálandó, nem szabad nem létező AC-gyorsulást
ígérni a DC-típusváltástól.

Következő terv: reduktor műveletek közös típusparaméteres megvalósítása,
az alapcelláknál helyi típusválasztás, a magasabb szintek változatlan útja.
Az alapcella azonosítása strukturális tulajdonságból történjen, ne név vagy
konkrét mintahálózat alapján. A cache és a visszahelyettesítés típushoz és
aktuális mátrixhoz kötött legyen. Először EXTENDED és ellenőrzött DOUBLE;
AUTO csak külön igazolt alkalmassági kritériummal. HMG-kapcsolót működés
nélkül nem jelentünk késznek. A GUI-bekötés során már most megőrzendő,
hogy az optimalizálási kérés a fizikai hálózattól elkülönülő adat.

A vezetésarány önmagában nem elégséges: kiejtés, pivotok, dinamikus C/dt,
aktív elemek és munkapont is számít. Szélsőséges, de fizikailag érvényes
referenciákkal kell összevetni a nagy pontosságú út eredményét. A GUI
maradjon az alapverzióval használható, az opcionális kérés kihagyható.

## OPT-03 – linearitás és az egyetlen iteráció

A teljes összeállított egyenletrendszernek kell lineárisnak lennie az adott
lépés ismeretlenjeiben. Állandó anyagadat önmagában nem bizonyítja ezt:
vezérlő, kapcsoló, hőtermelés vagy állapotfüggő gerjesztés is visszacsatolhat.
Ismeretlen függvény linearitását nem feltételezzük. Az ellenőrzött
lineáris út nem a nemlineáris megállási küszöb lazítása.

Időtartományban a korábbi elfogadott állapot és a választott dt rögzíti
a lépés egyenletét. Egy megoldás elegendő lehet, de adaptív időléptetésnél
egy elutasított lépés kisebb dt-vel új megoldást igényelhet. Az inicializálás,
szakadási esemény, elfogadás, kontrollerek/áramok és mérők frissítése nem
hagyható el; S01/S08 tanulságait meg kell tartani.

## OPT-04 – elektromos, majd termikus

A felhasználó által javasolt sorrend akkor pontos, ha a csatolás egyirányú:

1. Az elektromos hálózat állapota kiszámítható az ismeretlen új termikus
   állapot nélkül, és az elektromos egyenlet lineáris.
2. Az elektromos megoldásból kiszámítjuk az összes szükséges termikus
   gerjesztést, helyes térfogati/felületi elosztással és energiamérleggel.
3. A termikus egyenlet az így rögzített gerjesztéssel lineáris, ezért
   külön egyetlen megoldással meghatározható.

A Joule-hő az elektromos ismeretlenekben négyzetes is lehet: ettől az egész
közös probléma nem lineáris, de egyirányú függés mellett a két egymást követő
lineáris megoldás mégis helyes lehet. A hőmérséklettől függő vezetés,
Seebeck-visszahatás vagy más elektromos–termikus visszacsatolás esetén
nem alkalmazható automatikusan ez a sorrend. A Peltier-/felületi/vezérlő-
kapcsolatokat tényleges függésük szerint kell értékelni.

Időlépésenként mindkét alrendszer saját előző elfogadott állapota megmarad.
Csak azonos lépéshez tartozó, elfogadható állapotpár publikálható;
elutasításkor mindkettő visszaáll. Az eredeti solver közös HMG-hálózata
továbbra teljes marad; az optimalizált solver belső szétválasztása a stabil
komponens-/mérőcímeken és HXIM-sorrenden nem változtathat.

## Javítások és nyilvántartás

A felhasználó külön engedélyezte a korábban egyszerűsítéssel megkerült vagy
félretett hibák rendes javítását az optimalizált ágon. A régi auditok az
okfeltárás és reprodukció kiindulópontjai, nem automatikusan a mai kód hibái.
Javítás előtt ellenőrizni kell, mi került már be az aktuális hivatalos ágba.
Alapoldali változtatás továbbra kézi felhasználói feladat.

Új sorhoz: ID, konkrét feltétel/hatókör, prioritás, mért referencia,
állapot és az elvégzett ellenőrzés tartozzon. Gyorsulás vagy memórianyereség
csak azonos fizikájú, azonos bemenetű összevetésből állítható.
