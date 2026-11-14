# Adaptív időlépés feszültségváltozás alapján

2026-11-14. Csak a `codex/optimized-solver` ág új képessége; a hivatalos
alapváltozatot nem módosítottuk.

```text
.OPTIONAL HEXMG_OPT 1
ADAPTIVE_VOLTAGE circuit 0.1 1e-9 140
ADAPTIVE_VOLTAGE_NODE circuit CELL.N0
ADAPTIVE_NONCONVERGENCE circuit ACCEPT_LAST
ADAPTIVE_SAVE_PROBES circuit TP0 TP1
.END OPTIONAL
```

Az első sor relatív feszültségváltozás-korlátot, minimumlépést [s] és a két
RUN-végpont közötti elfogadott köztes lépések korlátját adja. Nulla darab
korlátlan. Minden ellenőrzött fizikai potenciálcsomópontra külön NODE sor
kell. A feltétel `abs(Vúj−Vrégi)/(abs(Vrégi)+1e-6) <= határ`.

Nagy változásnál teljes visszaállítás és minimumlépés következik; elfogadás
után legfeljebb nyolcszorozódik a lépés. A kért végpontot nem lépi át.
A minimum vagy darabszámkorlát nagyobb változást megengedhet, naplózva.
A kritérium változáskorlát, nem helyi időintegrálási hibabecslés.

Konvergenciahiba esetén felezett lépéssel próbál újra a minimumig. Alapból
ott is hibával áll le. Az opcionális ACCEPT_LAST csak ekkor enged véges,
anyagérvényesség szempontjából elfogadható, de nem konvergált állapotot.
Az időpont, lépésköz és egyenleghiba `HEXMG_WARNING NONCONVERGED_STEP`
sorként kerül a naplóba. Anyaghiba, nem véges állapot és más kivétel nem
alakulhat megengedett konvergenciahibává. A darabszámkorlát önmagában nem
engedélyez nem konvergált lépést. ITER/ITERS és multigrid együttes használat
elutasított; a futásnak konvergenciakritérium szerint kell működnie.

A SAVE_PROBES az elfogadott köztes időpontokat az `adaptive_CIRCUIT.res`
fájlba írja, RAW időpont/lépésköz/mért érték sorrendben. Csak ugyanahhoz az
áramkörhöz tartozó V/I probe adható meg. A RUN-végpontot a rendes SAVE
utasítás menti, nincs ismételt végpont. Az elvetett próbákból nincs sor.

`TimeState`: a csomópontok, korábbi elfogadott értékek, kapacitáselőzmények,
függvény- és vezérlőmunkaterek, valamint idővezérlés számszerű állapota.
A topológia nem változik; a mátrix és az áramok a visszaállítás után
újraszámolódnak. A checkpoint csak elfogadás után frissül. A végpontnál
néhány gépi kerekítési egység maradványa nem hoz létre hamis plusz lépést.

Célzott referencia a GUI-tároló `tools/solver/adaptive_voltage_reference.py`
eszközével: 67 ellenőrzés / 19 natív futás. Független RC és hiszterézis,
elfogadott időpontok adaptivitás nélküli újrajátszása, felezéses konvergencia,
szigorú/ACCEPT_LAST ág, anyagvédelem, mérésmentés és hibás bemenetek.
A GUI projekt–solver–HXRP próbája további 143 ellenőrzés / 4 futás.
Ez nem minden VO₂-modell konvergenciájának bizonyítása.

A forró csatorna, szimmetriatörés és eltérő csatornahely önmagában nem hiba.
Az egyenletek teljesülése, a mérlegek, a tér- és időfelbontás érzékenysége
és a megoldási ágak külön értékelendők. Az opt-in nem konvergált eredmény
megjelölése akkor is megmarad, ha a kialakult csatornakép fizikailag hihető.
