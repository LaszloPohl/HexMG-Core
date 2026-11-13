# Pontos függvényderivált megadása

2026-11-13. Jóváhagyott opcionális kiegészítés az optimalizált solverhez.

```text
.OPTIONAL HEXMG_OPT 1
FUNCTION_DERIVATIVE F P0 DF
.END OPTIONAL
```

Az `F` egyéni függvény `P0` bemenete szerinti parciális deriváltját a `DF`
egyéni függvény adja vissza. Ugyanazokat a skalár- és komponensbemeneteket
kapja, mint F. A bemenetek száma és sorrendje azonos; a lokális változók
száma és a belső számítás eltérhet. A másik függvény később is definiálható,
de az első RUN előtt mindkettőnek léteznie kell. Saját deriváltként is
megadható egy függvény, például az exponenciális.

Egy függvény több bemenetére külön sor írható; egy bemenetre csak egy.
A nem megadott deriváltaknál a korábbi numerikus differenciálás marad.
Az alap solver a teljes OPTIONAL blokkot kihagyja; a fizikai egyenlet
ugyanaz marad, de a konvergencia nem feltétlenül azonos.

A kiértékelés külön, egyszer előre lefoglalt munkaterületen történik,
és nem írja át az F bemeneteit, RET értékét vagy belső változóit. A solver
numerikus végrehajtása jelenleg soros. A nem véges vagy be nem állított
derivált szabályos futási hibát okoz. A megadott képlet matematikai
helyességét a solver nem bizonyítja; azt a netlista készítőjének kell
ellenőriznie. A deriváltkiértékelésnél a szintetikus függvényvizsgálatra
vonatkozó meglévő tartományellenőrzési kivétel érvényes.

Példa: `F(x)=-x*x` esetén `DF(x)=-2*x`. A paraméterfüggő termékben és
összetett függvényben a teljes láncszabály szerinti derivált szükséges.
Az utasítás nem módosítja a fizikai modellt és nem jelent általános
konvergenciagaranciát. Az FCI/FCID/FCIM/FCIMD meglévő DC Jacobiját szolgálja;
nem javítja automatikusan a régi elemek minden AC-linearizálási hibáját.

Ellenőrzés: 552 közvetlen derivált-, beágyazás- és állapotmegőrzési kapu;
23 parser/futási eset; 112 soros dióda és 27 híd/ellenpárhuzamos áramkör.
Regresszió: 3564 disszipáció-Jacobi, 31 függvénytartomány, 51 konvergencia,
27 AC-futás / 1768 numerikus érték. A külön, szélsőséges 66 diódás
kísérletsor még nem teljesen sikeres; ezért a kombinált dióda GUI-bekötése
nincs késznek minősítve.

A beágyazott egyéni függvényhívás célváltozója korábban nem számított bele
az olvasott argumentumok számába. Ezt a változat javítja; a GUI-tárolóban
az alap 7de0355 változathoz külön, sorszámos kézi útmutató készült:
`docs/solver/CORE_NESTED_FUNCTION_ARGUMENT_FIX_HU.md`.
