# FUNCTION_GUARD – futás közbeni függvényvédelem

2026-10-29. Felhasználó által jóváhagyott opcionális kiegészítés.

```text
.OPTIONAL HEXMG_OPT 1
FUNCTION_GUARD THK0 P0 V2 -273.15 *
.END OPTIONAL
```

A THK0 P0 bemenete véges és a zárt alsó/felső határon belüli; a `*`
azon az oldalon korlátlan. A deklarált V2 helyi változó véges és >0.
A függvénynek minden visszatérési útján ki kell töltenie V2-t; kezdetben
NaN. Például a lambda kiszámítása után `_ADDC V2 RET 0` megőrzi azt,
mielőtt RET árammá alakul. A FUNCTION fejlécben ehhez legalább V=3 kell.
A függvény eredményének is végesnek kell lennie.

A nevek/indexek a teljes beolvasás után ellenőrzöttek. Egy függvényhez
csak egy őrsor tartozhat; belső parser-konstans nem deklarált változó.
Hibás/ismételt sor, ismeretlen függvény, hibás index vagy határ elutasított.
Az alap solver az OPTIONAL blokkot kihagyhatja; a függvény normál HMG.
Ez az alap solverbe semmilyen automatikus módosítást nem visz.

A noexcept kiértékelő az első hibát rögzíti. A külső DC-vezérlés dob
kivételt, CLI kilépőkód 1; hibás állapot nem kerül elfogadásra/mentésre.
Fizikai állapot hibája redukció előtt, redukció közbeni nem véges derivált
visszahelyettesítés előtt állít meg. A numerikus deriválás segédpontja
nem fizikai állapot: önmagában a tartomány túllépése nem hiba ott.
A nem véges derivált ettől még elutasított. Nincs automatikus clamp,
Newton-csillapítás vagy új extrapoláció.

A példányút nullától számozott szerkezeti út (circuit/component/controller),
nem eredeti név. Deriválási hibánál a bemenet a derivált változó értéke.
Jelenleg soros DC és időlépés támogatott. AC és multigrid őrzött
függvénnyel elutasított. Párhuzamosítás előtt a hibagyűjtés bővítendő.

Ellenőrzés: `tests/function_guard_runtime.py --solver <exe> --output <új könyvtár>`.
46/46 runtime eset PASS, külön a GUI Qt Creator-telepítésével is.
Korábbi relatív konvergencia 51/51, matematika 39/39, OPTIONAL olvasó 42/42 PASS.
GUI: 918 ellenőrzés/217 futás és 830 végrehajtási/csomagolási ellenőrzés PASS.
Az alap referencia fc8bcc55342a7823d9ebb06ff1933c22e9e2d3a0 érintetlen.
