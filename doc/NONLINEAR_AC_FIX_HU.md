# Nemlineáris elemek kisjelű AC-linearizálása

2026-11-13. FCI/FCIM/FCID/FCIMD és GD/RD/GMD javítás.

A kisjelű admittanciamátrix a munkapontban értékelt fizikai DC-egyenlet
deriváltja. Minden vezérlő bemenet, a mérőcsomópont és mindkét hőkimenet
szerepel benne. A DC indításkor alkalmazható disszipációs késleltetés nem
érvényes az AC-mátrixra. A függvény DC-értéke nem lehet AC-forrás.

A disszipáció kisjelű változása `dP = Idc*dV + Vdc*dI`. Nincs
amplitúdusnégyzetes tag. A konstans vezetéses hőkimenetek miatt az AC-mátrix
sem szimmetrikus. Az összevont portok diagonális tagjai összeadódnak.

A SUNRED nemszimmetrikus AC-redukciójának `Bsiz==1` ágában `XB` helyett
`XAT` adja a megfelelő fordított csatolási blokkot. A szimmetrikus ág és
a már helyes DC-ág változatlan.

Célzott ellenőrzések:

- `ac_jacobian_tests`: 1338 fizikai derivált, komplex áram, nulla és skálázott
  amplitúdó, DC-késleltetéstől való függetlenség és portösszevonás PASS.
  A régi elemek 1314 korábbi ellenőrzéséből 930 hibás volt.
- `nonlinear_ac_runtime.py`: 12/12 hálózat, 2388 számszerű összehasonlítás;
  hat elem, közvetlen/SUNRED, három DC-munkapont, ismételt frekvenciák,
  termikus visszacsatolás és komplex gerjesztés PASS.
- `gmd_jacobian_tests`: korábbi DC 3564 PASS.
- `ac_runtime.py`: korábbi 27/27 futás, 1768 érték PASS.
- Érintetlen alap `7de0355`: FCI/GD négy esete egyaránt hibás; az alap
  forrása nem változott. A GUI tárolóban pontos sorszámos kézi útmutató:
  `docs/solver/CORE_NONLINEAR_AC_FIX_HU.md`.

Nem új HMG-szintaxis. A függvénytartomány-védelemmel közös AC-indítás és
a mentett FVM-munkapont GUI-adatútja külön bekötési feladat marad.
