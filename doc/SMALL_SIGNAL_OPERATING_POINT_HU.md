# Kisjelű számítás meglévő munkapontból

Az optimalizált solver opcionális kiegészítése:

```text
.OPTIONAL HEXMG_OPT 1
SMALL_SIGNAL_OPERATING_POINT circuit
.END OPTIONAL
```

A megnevezett áramkör AC- vagy időállandó-számítása a már memóriában lévő
valós csomóponti állapot körül linearizál. Ha előtte nem volt DC-számítás,
elmarad az egyébként automatikus DC-egyensúlyszámítás. A betöltött `.SET`
értékek teljességéért és fizikai jelentéséért a bemenet előállítója felel.
Az utasítás önmagában nem olvas fájlt, és nem bizonyítja a munkapont egyensúlyát.
Időtartománybeli pillanatnyi állapotnak nem is kell DC-egyensúlyban lennie.

Ha ugyanebben a futásban közvetlenül DC vagy egy időlépés előzi meg az AC-t,
akkor annak aktuális állapota használódik, köztes mentés nélkül. Ez az
opcionális sor nélkül is megmarad. Az inicializálás most áramkörönként
nyilvántartott: egy másik áramkör korábbi DC-futása nem helyettesíti a sajátot.

AC előtt a vezérlők, anyagfüggvények és fizikai áramok frissülnek az aktuális
valós állapotból. Nincs Newton-lépés, csomóponti visszaállítás vagy időlépés-
elfogadás. A kondenzátor DC-árama és időlépési előzménye megmarad; az AC
beiktatható két időlépés közé. A FUNCTION_GUARD/FUNCTION_RANGE a valós
munkapontot ellenőrzi, nem a komplex amplitúdót. Hibás függvény vagy derivált
esetén a kisjelű eredmény nem fogadható el.

Ismeretlen áramkör, ismételt vagy hibás argumentumszámú sor elutasított.
Multigridhez ez az inicializálás még nincs bekötve. Az alap solver a blokkot
kihagyja; ott a kiegészítés működése nem állítható.

## Rövid példa

```text
; A modell és a függvények definíciói után:
.CREATE circuit M GND=R0
.SET circuit.N0 2
.RUN circuit AC F=1
```

Az opcionális sorral ez a 2 V-os állapot körüli kisjelet számolja akkor is,
ha a DC-gerjesztések más egyensúlyi feszültséget adnának. Élő futásnál:

```text
.RUN circuit DC INITIAL ERR=1e-11
.RUN circuit AC F=1
.RUN circuit TIMESTEP DT=0.1 ERR=1e-11
.RUN circuit AC F=1
.RUN circuit TIMESTEP DT=0.1 ERR=1e-11
```

Az első AC a DC-pontot, a második az első időlépés végállapotát használja.
Az utolsó időlépés az előző időlépés fizikai állapotából folytatódik.

## Célzott ellenőrzés

`tests/small_signal_state_runtime.py`: 23 eset / 79 számszerű ellenőrzés.
Közvetlen és SUNRED, betöltött és élő DC, változtatott pont, RC-időlépés
közé iktatott AC, független második áramkör, érvényesség és hibás derivált.
Független referencia: I=V+V², C=0,2 F, zárt alakú DC/időlépési gyök és
`1/(1+2V+jωC)` kisjel. A korábbi nemlineáris AC-próba 12 eset / 2388 érték.

2026-11-14: a csak betöltött munkapont mellett a globális iterációszám nulla
maradt. Az eredeti konverter és a GUI Ernő/illesztett junction-függvényei
ilyenkor az első Newton-frissítéshez tartozó mesterséges szelővezetést
választották, így hibás volt a kisjelű meredekség. A `runAC()` a teljes
előkészítés és linearizálás idejére a nulla számlálót egyre állítja, majd
siker és kivétel esetén is visszaállítja. Élő DC/időlépés nem nulla számlálóját
nem módosítja; a fizikai csomópontok, áramok és kapacitáselőzmények megmaradnak.
Ez nem új HMG-utasítás és nem új DC-iteráció.

A két új indítóközelítéses eset a korábbi 786438f binárissal hibás volt,
a javítással közvetlen és SUNRED redukciónál is pontos. A teljes 23/79 sor és
a nemlineáris AC 12/2388 sor sikeres. A GUI-adapter termoelektromos, Ernő-,
illesztett és dióda-ellenőrzése 790 feltétel / 32 futás; a kisjel a külön
DC-perturbációkból számított meredekséggel egyezik. A hivatalos alapváltozat
nem változott; a betöltött munkapont opciója csak az optimalizált ág része.
