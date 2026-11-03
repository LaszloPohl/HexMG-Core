# CONTROLLER LOAD/STORE listák: pontos kézi védelem

2026-11-03. Alap Core **02a79a8022d4288663c0b112e0bb5bf26acfc388**,
`src/hmgHMGFileReader.cpp`. SHA-256:
`DC18068151E7F7A5672B65886C6D7F059658C0C2FE1DA2D3D436126D6953D6BF`.
Az alapforrás változatlan; az optimalizált ágon elkészült a javítás.

## Mi hibás?

A CONTROLLER `LOAD` és `STORE` listája a **RET, P0, P1, …** sorrendet
jelenti, nem a helyi V-változókat. P=2 függvénynél legfeljebb három bejegyzés
szerepelhet, a `-` helykitöltőket is beleszámítva. A rövidebb lista megengedett.
Ha például `STORE A0 A1 - A1` szerepel, az utolsó elem már nem létező
paramétert címez. Eddig ezt a beolvasó elfogadta, és a kiértékelő más
utasítás belső indexét vagy a tömbön kívüli memóriát használhatta.
A FUNCTION sor hiánya szintén eljutott egy érvénytelen függvénymutatóig.

A javítás a hibás netlistát már beolvasáskor, a modell nevével és a
LOAD/STORE eredeti fájlsorszámával utasítja el. Helyes modell számítási
sorrendjén és értékein nem változtat. Ez valóban bemeneti hiba, nem
numerikus problémát elfedő kivétel.

## A három módosítás

Minden hely a **ReadOrReplaceBodyController** függvényben van, amely az
eredeti forrás **1477. sorában** kezdődik. A sorszámok módosítás előtti értékek.

1. A **1480. sorban** lévő `LineTokenizer lineToken;` után:

```cpp
    LineInfo loadLineInfo{}, storeLineInfo{};
```

2. A **1529. sorban** kezdődő LOAD-ág első soraként, a `while` elé:

```cpp
            loadLineInfo = lineInfo;
```

A **1540. sorban** kezdődő STORE-ágban ugyanott:

```cpp
            storeLineInfo = lineInfo;
```

3. A függvény végén, a **1560. sorban** lévő
`    } while (isModelNotEnded);` **után**, a függvény záró `}` jele elé:

```cpp
    if (functionType == biftInvalid)
        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBodyController", "CONTROLLER %s has no FUNCTION in %s, line %u", fullName.c_str(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    cuns nFunctionParams = functionType == biftCustom
        ? globalNames.functionData[functionCustomIndex]->nParams
        : HgmFunctionStorage::builtInFunctions[functionType]->getN_Param();
    // LOAD/STORE lists address RET followed by P0..P(n-1), never V locals.
    if (functionParamsLoad.size() > static_cast<siz>(nFunctionParams) + 1)
        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBodyController", "CONTROLLER %s LOAD has %u entries; at most RET + %u parameters allowed in %s, line %u", fullName.c_str(), static_cast<uns>(functionParamsLoad.size()), nFunctionParams, reader.getFileName(loadLineInfo).c_str(), loadLineInfo.firstLine);
    if (functionParamsStore.size() > static_cast<siz>(nFunctionParams) + 1)
        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBodyController", "CONTROLLER %s STORE has %u entries; at most RET + %u parameters allowed in %s, line %u", fullName.c_str(), static_cast<uns>(functionParamsStore.size()), nFunctionParams, reader.getFileName(storeLineInfo).c_str(), storeLineInfo.firstLine);
```

Az ellenőrzés szándékosan a teljes modell beolvasása után történik:
nem írja elő, hogy a FUNCTION sornak a LOAD/STORE előtt kell állnia.
Nem kell fejlécet vagy más forrást módosítani. A meglévő CP1250 kódolás
és CRLF sorvég megmarad.

## Próbák

`tests/controller_lists_runtime.py`: 21 eset, három pontossági módban.
Helyes saját/beépített függvény, több kimenet, rövid lista; túl hosszú
LOAD/STORE, túl sok helykitöltő és hiányzó FUNCTION.

A régi optimalizált binárisban 9 helyes eset működött, a 12 hibásból egy
sem kapott megfelelő diagnózist. A STORE példája 6 helyett 8-at írt ki;
a FUNCTION hiánya 0xC0000005 hibával állt le. A javított és a Qt Creator
mellé telepített solver **21/21** esetben helyes, hibás bemenet nem készít képet.
A legelső tesztváz téves `ITER=1` szintaxisát még a reprodukció előtt
`ITER` alakra javítottuk; a mérvadó előtte-sor a `controller-lists-before2`.

Regresszió: JMP 18 eset/36 érték; FUNCTION_RANGE 31 eset; Wei 2931
ellenőrzés/54 valódi futás. Minden kapu sikeres. A teljes geometriai
tesztsor nem releváns a HMG-olvasó módosításához.
