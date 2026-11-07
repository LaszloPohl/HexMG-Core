# SUNRED: az első mátrixkiértékelés kötelező inicializálása

2026-11-07. Kézi javítás az alap Core **9ccaa6f5712f2d59914ddbf430318e1c373b3899**
forrásához. Az alap forrását nem módosítottam. Az optimalizált ágban a javítás
elkészült. Ez újonnan feltárt hiba, nem valamely korábbi kézi javítás hiánya.

## A hiba és következménye

A `vektor::set_size` Release-ben inicializálatlan tárolót foglal. A SUNRED
levél az első futáskor is `refresh_unsafe`-fal hasonlította a friss értéket
ehhez a még nem érvényes tartalomhoz. Ha egy részfa csak ideális áramforrásokat
vagy nulla áramú megfigyelőket tartalmaz, a teljes admittanciamátrixa nulla.
Ilyenkor az „egyik gyerek mátrixa sem változott” feltétel már az első lépésben
igaz lehetett, és a szülő nem készítette el a saját mátrixát/redukcióját.

A hibás viselkedés a lefoglalt memória előző tartalmától függ: ugyanaz a
bemenet egymás után sikerülhet és hibázhat. Ezért nem megfelelő javítás a
futás megismétlése, a konvergenciakorlát emelése vagy a tűrés lazítása.

A javítás külön DC- és AC-jelzővel követi, hogy elkészült-e már az adott
mátrix. Az első alkalommal közvetlenül felírja a levél értékeit, és kötelezően
elvégzi a szülő redukcióját. A további lépésekben megmarad a korábbi
változásfigyelés és az újrafelhasználás. Új allokáláskor/törléskor a jelző
visszaáll. Nincs új HMG-sor vagy megváltozott fizikai egyenlet.

## 1. `src/hmgSunred.h`

A sorszámok mindenhol a **módosítás előtti** fenti commitra vonatkoznak.
Saját köztes módosítás esetén a függvénynév és az idézett sor azonosítja a helyet.

**86. sor**, a `SunredTreeNode` tagváltozóinál, ez után:

```cpp
    bool isChangedDC = false, isChangedAC = false;
```

szúrd be:

```cpp
    bool initializedDC = false, initializedAC = false;
```

**102. sor**, az `allocDC()` függvényben, közvetlenül a
`dc = std::make_unique<SunredReductorDC>();` elé:

```cpp
            initializedDC = false;
```

**135. sor**, az `allocAC()` függvényben, közvetlenül az
`ac = std::make_unique<SunredReductorAC>();` elé:

```cpp
            initializedAC = false;
```

**160–161. sor**, a két egysoros törlőfüggvényt cseréld erre:

```cpp
    void clearDC() { dc.reset(); initializedDC = false; }
    void clearAC() { ac.reset(); initializedAC = false; }
```

## 2. `src/hmgSunred.cpp` — `SunredTreeNode::forwsubsDC`

**158. sor**, a levélág elején:

```cpp
        bool isChanged = !initializedDC;
```

**162. sor**, az egymástól különböző kapcsokhoz tartozó levélmátrix
`refresh_unsafe` sorát cseréld erre a három sorra:

```cpp
                    const auto value = srcComponent->getYDC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex);
                    if (!initializedDC) dc->YRED.get_elem(y, x) = value;
                    else isChanged = dc->YRED.refresh_unsafe(y, x, value) || isChanged;
```

**174. sor**, az összekötött kapcsokat tartalmazó levél `YA` mátrixának
`refresh_unsafe` sorát hasonlóan:

```cpp
                    const auto value = srcComponent->getYDC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex);
                    if (!initializedDC) dc->leaf->YA.get_elem(y, x) = value;
                    else isChanged = dc->leaf->YA.refresh_unsafe(y, x, value) || isChanged;
```

**207. sor**, a nem levél ág mátrix-összevonását vezérlő feltétel:

```cpp
        if (!initializedDC || srcCell1->isChangedDC || srcCell2->isChangedDC) {
```

**633. sor után**, a függvény utolsó `}` jele elé, az `empty node` megjegyzéses
zárójel **után**, tehát mindkét ág közös végére:

```cpp
    initializedDC = true;
```

## 3. Ugyanez az AC-ágban

Ugyanebben a fájlban, a `SunredTreeNode::forwsubsAC` függvényben:

**646. sor:**

```cpp
        bool isChanged = !initializedAC;
```

**650. sor:**

```cpp
                    const auto value = srcComponent->getYAC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex);
                    if (!initializedAC) ac->YRED.get_elem(y, x) = value;
                    else isChanged = ac->YRED.refresh_unsafe(y, x, value) || isChanged;
```

**662. sor:**

```cpp
                    const auto value = srcComponent->getYAC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex);
                    if (!initializedAC) ac->leaf->YA.get_elem(y, x) = value;
                    else isChanged = ac->leaf->YA.refresh_unsafe(y, x, value) || isChanged;
```

**695. sor:**

```cpp
        if (!initializedAC || srcCell1->isChangedAC || srcCell2->isChangedAC) {
```

**1121. sor után**, az `empty node` megjegyzéses zárójel után,
a függvény utolsó `}` jele elé:

```cpp
    initializedAC = true;
```

Nem a `backsubsDC`/`backsubsAC` függvényeket kell módosítani. A régi
kapocstérkép- és FCB-javítás változatlanul szükséges; ez nem helyettesíti őket.

## Ellenőrzés

- Régi optimalizált solver: ugyanazon sugárzásos bemenet **3/80** futása hibás.
- Külön reprodukáló sor, elektrotermikus sugárzás és nulla mátrixú részfa,
  DC/AC-váltások, három pontosság: régi optimalizált **8/240** hibás,
  hivatalos alap **3/240** hibás. A hibák nem szükségszerűen azonos sorszámúak.
- Javított optimalizált: ugyanez **240/240 PASS**, a külön sugárzásos ismétlés
  **500/500 PASS**. A referencia nem pusztán az exitkódot, hanem a kiszámított
  értékeket is ellenőrzi a 240-es sorban.
- Egyesített DC-rács: **3462 ellenőrzés / 173 futás PASS**, független
  áram-, teljesítmény- és sugárzási hőmérleggel.
- Regresszió: AC **27/27, 1768 érték**, relatív konvergencia **51/51**,
  SUNRED vezérlőport **24/24, 96 érték PASS**.

Reprodukáló program az optimalizált Core-ban:
`tests/sunred_initialization_runtime.py --solver <exe> --output <új_mappa>`.
Az alap solver használata itt csak futtatás, a forrását nem módosítja.
Ez célzott numerikus ellenőrzés, nem teljes Core-tesztsor vagy teljesítménymérés.
