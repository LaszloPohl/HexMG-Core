# OPTIONAL blokkok kihagyása az alap solverben

2026-10-27. Kézzel beilleszthető, az optimalizált ágon lefordított és tesztelt
kód. Kiinduló hivatalos forrás: `631f316f1bf095eabf1413bbb332c75924c30086`.
Az alapág forrását és binárisát nem módosítottuk. A numerikus algoritmusokat
ez a változtatás nem érinti.

## A jóváhagyott működés

Az első oszlopban kezdődő `.OPTIONAL` kulcsszótól az **első `.END OPTIONAL`
sorig** mindent kihagyunk, a két jelölősort is. A nyitó sor további része
tetszőleges; az alap solver nem ellenőrzi a bővítés nevét/verzióját vagy
a beállításait. Egy újabb `.OPTIONAL` a belsejében közönséges kihagyott
tartalom: nincs egymásba ágyazási számláló. A blokk bárhol állhat a fájlban,
modellben, függvényben, rácsleírásban és két sorfolytatás között is.

A kezdés és lezárás ugyanabban a fizikai fájlban kell legyen. EOF esetén
a hibaüzenet megadja a fájlt és a nyitás sorát. A kihagyott `.INCLUDE` nem
nyit fájlt, `.RUN` és `.SAVE` nem hajtódik végre, `.END MODEL` nem zárja le
a környező modellt. A kihagyás a `+` sorfolytatás feldolgozása előtt történik.

A HMG meglévő szokása szerint a nyitó utasítás az első oszlopban kezdődik;
a behúzott HMG-sor továbbra megjegyzés. Kis-/nagybetű nem számít.
A nyitó kulcsszó határa szóköz, tab, sorvége vagy `;`/`//` megjegyzés:
`.OPTIONALLY` nem azonos vele. A zárósorban a `.END` és `OPTIONAL` között
szóköz/tab lehet, utána üres rész vagy `;`/`//` megjegyzés. Behúzott lezárás
is felismerhető. `.END OPTIONAL extra` és idézőjelbe tett jelölő nem lezárás.
LF, CRLF és a régi olvasó LF–CR sorvége is megmarad.

## 1. A fejlécben a getChar helyére

Fájl: `src/hmgHMGFileReader.h`, a `ReadALine` privát részében.
A kiinduló változat **439. sorában kezdődő teljes `getChar()` függvényt**
cseréld az alábbira. A következő `peekChar()` és `peekChar2()` maradjon
változatlan. Az új `getRawChar()` ugyanazt a két puffert használja; nem
kever külön fájlolvasást a már előreolvasott adatokhoz. A normál `getChar()`
megőrzi a számokban lévő aposztrófok eltávolítását, a kihagyó viszont
nyers karakterekkel keres, így például `.E'N'D OPTIONAL` nem zárja le.

```cpp
    int getRawChar() {
    //***********************************************************************
        if (buffer1->index == buffer1->elementNum) {
            fClose();
            return EOF;
        }
        char ret = buffer1->buffer[buffer1->index];
        buffer1->index++;
        if (buffer1->index == buffer1->elementNum)
            readNextBlock();
        return static_cast<unsigned char>(ret);
    }
    // Raw access is used only for OPTIONAL framing; ordinary input keeps its
    // historical apostrophe handling.
    int getChar() {
        const int ch = getRawChar();
        return ch == '\'' ? getChar() : ch;
    }
    int peekRawChar(unsigned offset) const {
        const unsigned available = buffer1->elementNum - buffer1->index;
        if (offset < available)
            return static_cast<unsigned char>(buffer1->buffer[buffer1->index + offset]);
        offset -= available;
        return offset < buffer2->elementNum
            ? static_cast<unsigned char>(buffer2->buffer[offset]) : EOF;
    }
    bool skipOptionalBlock();
```

## 2. A cpp fájlba az új függvény

Fájl: `src/hmgHMGFileReader.cpp`. A `ReadALine::appendLine` elé
(a kiinduló forrásban **291. sor**) illeszd be ezt a teljes függvényt.
A meglévő include-ok elegendők. A kihagyott sorok hossza nem növeli
korlátlanul a memóriát, és a rendes HMG-sor 4096-os korlátja nem vonatkozik
az értelmezés nélkül eldobott tartalomra.

```cpp
// OPTIONAL is a physical-file filter, before INCLUDE and '+' processing.
// actLine already denotes the physical line at the current cursor.
bool ReadALine::skipOptionalBlock() {
    constexpr char opening[] = ".OPTIONAL";
    constexpr char closing[] = ".END OPTIONAL";
    constexpr unsigned openingSize = sizeof(opening) - 1;
    for (unsigned i = 0; i < openingSize; ++i) {
        const int ch = peekRawChar(i);
        if (ch == EOF || toupper(static_cast<unsigned char>(ch)) != opening[i])
            return false;
    }
    const int after = peekRawChar(openingSize);
    if (!(after == EOF || isspace(static_cast<unsigned char>(after)) ||
          after == ';' || (after == '/' && peekRawChar(openingSize + 1) == '/')))
        return false; // .OPTIONALLY is not an OPTIONAL directive.

    const unsigned openingLine = actLine;
    const auto missingEnd = [&]() {
        throw hmgExcept("ReadALine::skipOptionalBlock",
            "missing .END OPTIONAL in the same file as .OPTIONAL in %s, line %u",
            names[fileNameIndex].name.c_str(), openingLine);
    };
    int ch;
    while ((ch = getRawChar()) != EOF && ch != '\n') {} // opaque header
    if (ch == EOF) missingEnd();
    if (peekRawChar(0) == '\r') getRawChar(); // historical LF-CR support
    ++actLine;

    for (;;) {
        // A bounded, whitespace-normalized prefix is enough to recognize the
        // end marker. Arbitrarily long skipped lines do not grow memory.
        std::string prefix;
        bool comment = false;
        while ((ch = getRawChar()) != EOF && ch != '\n') {
            if (comment) continue;
            if (ch == ';' || (ch == '/' && peekRawChar(0) == '/')) {
                comment = true;
                continue;
            }
            if (isspace(static_cast<unsigned char>(ch))) {
                if (!prefix.empty() && prefix.back() != ' ' && prefix.size() <= sizeof(closing))
                    prefix.push_back(' ');
            }
            else if (prefix.size() <= sizeof(closing))
                prefix.push_back(static_cast<char>(toupper(static_cast<unsigned char>(ch))));
        }
        if (!prefix.empty() && prefix.back() == ' ') prefix.pop_back();
        if (ch == '\n') {
            if (peekRawChar(0) == '\r') getRawChar();
            ++actLine;
        }
        if (prefix == closing) return true; // first closing marker, no nesting
        if (ch == EOF) missingEnd(); // never continue in a parent/include file
    }
}
```

## 3. Két ciklus elején egy-egy sor

Az `appendLine()` belsejében, a kiinduló cpp **308. sorában** kezdődő
ciklus eleje ez legyen:

```cpp
    while (isNotFinished) {
        if (st == stStart && skipOptionalBlock()) continue;
        int ch = isPeek ? peekChar() : getChar();
```

A `getLine()` belsejében, a kiinduló cpp **397. sorában** kezdődő
ciklus eleje pedig ez:

```cpp
    while (isNotFinished) {
        if (st == stStart && skipOptionalBlock()) continue;
        int ch = getChar();
```

Mindkét hely szükséges: az `appendLine()` előreolvas a következő sorra,
miközben a `+` folytatást keresi. Csak a globális parancsértelmezőben
végzett kihagyás nem működne modellben/függvényben, és addigra egy INCLUDE
már megnyithatott volna egy fájlt. Más ciklusba nem kell új sor.
A sorszámok a megadott commitra vonatkoznak; a függvénynevek a biztos
illesztési pontok, ha közben módosítottad a fájlt.

## Ellenőrzés

- MSVC x64 Release, `/W4 /WX`: teljes solver és olvasóteszt lefordult.
- `tests/optional_reader_tests.cpp`: **42 eset sikeres**. Nyitás és
  lezárás a 512 KiB-os pufferhatárokon; 1,5 MB-os kihagyott sor; üres,
  ismételt, ismeretlen fejlécű és beágyazottnak látszó blokkok; három
  sorvég; megjegyzések; sorfolytatás; modell/függvény; EOF és fájlhatár.
- A GUI által előállított 18 cellás termikus HMG: hivatalos solver,
  optimalizált solver változatlan bemenettel, majd **minden fizikai sor
  elé tett OPTIONAL blokkal**: a három HXIM fájl bájtra azonos.
- A GUI adapter–folyamat–újranyitás 18 cellás próbája is sikeres.
  Ez kompatibilitási ellenőrzés, nem gyorsulási mérés.

Az INCLUDE utáni szülőfájl LF-sorszámát a hivatalos olvasó már eleve
eggyel korábbinak jelzi; ez külön reprodukálva, itt változatlan maradt.
Az INCLUDE képességét nem bővítettük. A blokkon kívüli, pár nélküli
`.END OPTIONAL` kezeléséhez sem adtunk új szabályt: a normál parser kezeli.

Az optimalizált ág ebben a lépésben **szintén kihagyja a tartalmat**.
A `REDUCTION_PRECISION`/`THREADS` értelmezése, a double út, a párhuzamosítás
és a lineáris rövid út még nincs implementálva. A GUI továbbra blokk nélküli,
az érintetlen alap solverrel is olvasható HMG-t állít elő.

Az optimalizációk külön [élő feladatlistában](OPTIMIZATION_BACKLOG_HU.md)
szerepelnek; az alap- és optimalizált ág összevonása később sincs tervben.
