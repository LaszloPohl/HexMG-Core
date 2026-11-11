# Opcionális bináris HMG-bemenet

2026-11-11, kizárólag az optimalizált ágon. Az alap solver változatlan.

A CLI ugyanúgy egy bemeneti fájlnevet kap. A tartalom alapján felismeri
a szöveges HMG-t és az új `.hmgb` fájlt; a kiterjesztés önmagában nem dönt.

Formátum: 8 ASCII bájt `HXHB0001`, utána little-endian uint64 a kibontott
adat bájthosszával, majd egyetlen zlib adatfolyam. A belső tartalom az eredeti
HMG pontos bájtsorozata. Nem IEEE double-ra kerekített numerikus tokenek;
a nagy pontosságú konstansok, szóközök, idézőjelek, megjegyzések és sortörések
is megmaradnak. Az INCLUDE és OPTIONAL feldolgozás a régi olvasóban történik.

A kibontás memóriában történik, ideiglenes szövegfájl nélkül. A régi lexer
és parser továbbra is fut: ez nem előre feldolgozott hálózati formátum,
és jelenleg nem állítunk parser- vagy teljes solvergyorsulást.

Legfeljebb 2 GiB kibontott bemenet engedett. A kibontott tár fokozatosan nő;
nem foglaljuk le ellenőrzés nélkül a fejlécben megadott teljes méretet.
Verzió-, méret-, zlib/Adler-ellenőrzési és lezárási hiba, valamint extra
adat esetén szabályos hmgExcept hiba keletkezik. A teljes olvasásnak meg
kell történnie, mielőtt a solver a betöltött utasításokat végrehajtja.

Függőség: MIT-licencű miniz, a `third_party/miniz/LICENSE` alatt, CMake-ből
fordítva. A GUI ugyanennek a zlib-formátumnak megfelelő tömörítést készít.

Célteszt: `tests/binary_input.py <solver.exe> <új-kimeneti-mappa>`.
15 ellenőrzés sikeres: RC DC/időtartomány/AC és termikus/SUNRED képek
szöveges/bináris bájtpontos egyezése; CRLF, hibás verzió, túl nagy vagy
csonka fejléc, sérült zlib, extra lezáró adat. A GUI külön ellenőrzi a
tényleges bináris futtatást és a választható szöveges módot.
