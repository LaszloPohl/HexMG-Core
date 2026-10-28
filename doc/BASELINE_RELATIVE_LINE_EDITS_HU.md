# Kézi sorcserék — relatív konvergencia

Kizárólag az alap `fc8bcc55342a7823d9ebb06ff1933c22e9e2d3a0` változathoz.
A `-` sor törlendő, a `+` sor beillesztendő; a jelölést nem kell a C++-ba másolni.
A szóközzel kezdődő sor környezet. Az új fájlok teljes tartalma szerepel.
Az `@@` eredeti és új sorszámokat ad; eltérő saját forrásnál a környezet alapján dolgozz.
Ez kézi útmutató, nem automatikusan alkalmazott javítás.

[Magyarázat és lépéssorrend](BASELINE_RELATIVE_CONVERGENCE_HU.md).

## hmgConvergenceMetric.h

```diff
--- alap/src/hmgConvergenceMetric.h
+++ javított/src/hmgConvergenceMetric.h
@@ -0,0 +1,36 @@
+#pragma once
+#include <algorithm>
+#include <cmath>
+#include <stdexcept>
+
+namespace nsHMG::convergence {
+// Arithmetic foundation only; not wired into Simulation yet.
+// Callers supply physical scales and tolerances explicitly: no implicit units.
+struct Tolerances { double relative, stateAbsolute, balanceAbsolute; };
+inline double normalized(double error, double reference, double absolute, double relative) {
+    if (!std::isfinite(error) || !std::isfinite(reference) || reference < 0 ||
+        !std::isfinite(absolute) || absolute <= 0 || !std::isfinite(relative) || relative <= 0)
+        throw std::invalid_argument("Invalid convergence input or tolerance");
+    const double limit = std::fma(relative, reference, absolute);
+    if (!std::isfinite(limit) || limit <= 0)
+        throw std::invalid_argument("Unrepresentable convergence limit");
+    // Infinity from division is a failed convergence test, never success.
+    return std::abs(error) / limit;
+}
+inline double nodeError(double actualChange, double stateReference,
+                        double physicalBalance, double absoluteBranchSum,
+                        Tolerances tolerance) {
+    return std::max(normalized(actualChange, stateReference, tolerance.stateAbsolute, tolerance.relative),
+                    normalized(physicalBalance, absoluteBranchSum, tolerance.balanceAbsolute, tolerance.relative));
+}
+class Collector {
+    double maximum_ = 0;
+public:
+    void add(double error) {
+        if (std::isnan(error) || error < 0) throw std::invalid_argument("Invalid normalized convergence error");
+        maximum_ = std::max(maximum_, error);
+    }
+    double maximum() const { return maximum_; }
+    bool converged() const { return maximum_ <= 1; }
+};
+}
```

## hmgOptimization.h

```diff
--- alap/src/hmgOptimization.h
+++ javított/src/hmgOptimization.h
@@ -0,0 +1,86 @@
+#pragma once
+#include <atomic>
+#include <charconv>
+#include <cmath>
+#include <set>
+#include <cctype>
+#include <iostream>
+#include <sstream>
+#include <string>
+#include <vector>
+#include "hmgCommon.h"
+#include "hmgException.h"
+
+namespace nsHMG::optimization {
+inline bool enabled = true; // Baseline manual adoption of convergence options.
+inline unsigned maxIterations = 0; // Zero preserves legacy full-solve bounds.
+inline bool maxIterationsSeen = false;
+struct ConvergenceProfile { double reference=0, stateAbsolute=1e-6, balanceAbsolute=1e-12; };
+inline bool relativeConvergence = false;
+inline ConvergenceProfile convergenceDefault;
+struct NodeConvergenceOption {
+    std::string circuit, node;
+    ConvergenceProfile profile;
+    unsigned circuitId = 0;
+    DeepInterfaceNodeID id;
+};
+inline std::vector<NodeConvergenceOption> convergenceNodes;
+inline bool unsupportedWarned = false;
+inline std::vector<std::string> tokens(std::string line) {
+    const auto semicolon = line.find(';'), slash = line.find("//");
+    line.resize(std::min(line.size(), std::min(semicolon, slash)));
+    for (char& c : line) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
+    std::istringstream stream(line); std::vector<std::string> result; std::string token;
+    while (stream >> token) result.push_back(token);
+    return result;
+}
+inline bool knownHeader(const std::string& header, const std::string& file, unsigned line) {
+    if (!enabled) return false;
+    const auto t = tokens(header);
+    if(t.size()>=3 && t[1]=="HEXMG_OPT" && t[2]=="1" && t.size()!=3)
+        throw hmgExcept("HEXMG_OPT","invalid version-1 header in %s, line %u",file.c_str(),line);
+    const bool known = t == std::vector<std::string>{".OPTIONAL", "HEXMG_OPT", "1"};
+    if (!known && !unsupportedWarned) { unsupportedWarned=true;std::cerr << "HEXMG_OPT: unsupported OPTIONAL header; block ignored\n"; }
+    return known;
+}
+inline void parse(const std::vector<std::string>& lines, const std::string& file, unsigned openingLine) {
+    unsigned nextMax = maxIterations; bool seenMax = maxIterationsSeen;
+    bool nextRelative = relativeConvergence;
+    auto nextProfile = convergenceDefault;
+    auto nextNodes = convergenceNodes;
+    std::set<std::pair<std::string,std::string>> nodeKeys;
+    for(const auto& node:nextNodes) nodeKeys.emplace(node.circuit,node.node);
+    for (unsigned i = 0; i < lines.size(); ++i) {
+        const auto t = tokens(lines[i]); if (t.empty()) continue;
+        const auto fail = [&] { throw hmgExcept("HEXMG_OPT", "invalid or repeated option in %s, line %u", file.c_str(), openingLine + i + 1); };
+        // Precision and threading hints have no effect in this baseline variant.
+        if(t[0]=="REDUCTION_PRECISION" || t[0]=="THREADS") continue;
+        if (t.size()==2 && t[0]=="MAX_ITERATIONS") {
+            if (seenMax) fail();
+            const auto parsed = std::from_chars(t[1].data(), t[1].data() + t[1].size(), nextMax);
+            if (parsed.ec != std::errc{} || parsed.ptr != t[1].data() + t[1].size() || nextMax == 0) fail();
+            seenMax = true;
+        } else if ((t.size() == 5 && t[0] == "CONVERGENCE" && t[1] == "RELATIVE") ||
+                   (t.size() == 6 && t[0] == "CONVERGENCE_NODE")) {
+            const bool global = t[0] == "CONVERGENCE";
+            ConvergenceProfile profile;
+            const auto value = [&](size_t at) {
+                double v = 0;
+                const auto parsed = std::from_chars(t[at].data(), t[at].data()+t[at].size(), v);
+                if (parsed.ec != std::errc{} || parsed.ptr != t[at].data()+t[at].size() || !std::isfinite(v)) fail();
+                return v;
+            };
+            const size_t first = global ? 2 : 3;
+            profile.reference=value(first); profile.stateAbsolute=value(first+1); profile.balanceAbsolute=value(first+2);
+            if (profile.stateAbsolute <= 0 || profile.balanceAbsolute <= 0) fail();
+            if (global) { if(nextRelative) fail(); nextRelative=true; nextProfile=profile; }
+            else {
+                if(!nodeKeys.emplace(t[1],t[2]).second) fail();
+                nextNodes.push_back({t[1],t[2],profile,0,{}});
+            }
+        } else fail();
+    }
+    relativeConvergence=nextRelative; convergenceDefault=nextProfile; convergenceNodes=std::move(nextNodes);
+    maxIterations = nextMax; maxIterationsSeen = seenMax;
+}
+}
```

## hmgCircuitNode.hpp

```diff
--- alap/src/hmgCircuitNode.hpp
+++ javított/src/hmgCircuitNode.hpp
@@ -15,8 +15,11 @@
 #include "hmgCommon.h"
 #include "hmgException.h"
 #include <atomic>
 #include <memory>
+#include <optional>
+#include "hmgOptimization.h"
+#include "hmgConvergenceMetric.h"
 //***********************************************************************
 
 
 //***********************************************************************
@@ -105,15 +108,20 @@
         // Variables that must retain their value after a DC-AC switch:
         cplx valueAC = cplx0;        // voltage/temperature/... of the node
         //***********************************************************************
         // Variables used alternately for DC and AC:
+        ConcurrentValue physicalMagnitude; // Reset only before physical current stamping.
         ConcurrentValue d;           // defect (current)
         cplx v = cplx0;              // error (voltage)
         cplx f = cplx0;              // multigrid defect
         ConcurrentValue yii;         // admitance for multigrid
         //***********************************************************************
     };
     //***********************************************************************
+    std::optional<optimization::ConvergenceProfile> convergenceProfile;
+    enum class ConvergenceRole { Potential, FlowConstraint, FlowMonitor };
+    ConvergenceRole convergenceRole = ConvergenceRole::Potential;
+    const NodeVariable* convergencePotential = nullptr;
     inline static rvt alpha = 1; // multiplicator for getValue
     //***********************************************************************
     // Variables that must retain their value after a DC-AC switch:
     rvt valueDC = rvt0;          // voltage/temperature/... of the node
@@ -164,8 +172,47 @@
     void setYiiDC(rvt y)noexcept { if (!isGnd) extension->yii.store(y, isConcurrent); }
     void setDNonConcurrentDC(rvt d_)noexcept { extension->d.storeToNonConcurrent(d_); }
     void setVDC(rvt v_)noexcept { extension->v = v_; }
     void incDDC(rvt d_)noexcept { if (!isGnd) extension->d.fetch_add(d_, isConcurrent); }
+    void clearPhysicalMagnitude() noexcept {
+        if(extension) extension->physicalMagnitude.store(rvt0,isConcurrent);
+        convergenceRole=ConvergenceRole::Potential;
+        convergencePotential=nullptr;
+    }
+    void addPhysicalCurrentDC(rvt current) noexcept {
+        incDDC(current);
+        if(optimization::relativeConvergence && !isGnd && extension)
+            extension->physicalMagnitude.fetch_add(std::abs(current),isConcurrent);
+    }
+    void addPhysicalEquationDC(rvt residual, rvt magnitude, const NodeVariable* potential, ConvergenceRole role, bool replace=false) noexcept {
+        if(replace) setDDC(residual); else incDDC(residual);
+        if(optimization::relativeConvergence && !isGnd && extension) {
+            convergenceRole=role; convergencePotential=potential;
+            if(replace) extension->physicalMagnitude.store(magnitude,isConcurrent);
+            else extension->physicalMagnitude.fetch_add(magnitude,isConcurrent);
+        }
+    }
+    optimization::ConvergenceProfile effectiveConvergenceProfile() const {
+        if(convergenceProfile) return *convergenceProfile;
+        auto profile = convergencePotential && convergencePotential->convergenceProfile
+            ? *convergencePotential->convergenceProfile : optimization::convergenceDefault;
+        if(convergenceRole!=ConvergenceRole::Potential) {
+            profile.reference=0;
+            if(convergenceRole==ConvergenceRole::FlowConstraint) std::swap(profile.stateAbsolute,profile.balanceAbsolute);
+            else profile.stateAbsolute=profile.balanceAbsolute;
+        }
+        return profile;
+    }
+    double relativeChangeError(double relative) const {
+        const auto profile=effectiveConvergenceProfile();
+        return convergence::normalized(getValueDC()-getAcceptedValueDC(),
+            std::max(std::abs(getValueDC()-profile.reference),std::abs(getAcceptedValueDC()-profile.reference)),profile.stateAbsolute,relative);
+    }
+    double relativeBalanceError(double relative) const {
+        const auto profile=effectiveConvergenceProfile();
+        return convergence::normalized(getDDC(),extension->physicalMagnitude.loadRvt(isConcurrent),profile.balanceAbsolute,relative);
+    }
+
     void incYiiDC(rvt y)noexcept { if (!isGnd) extension->yii.fetch_add(y, isConcurrent); }
     void deleteDDC()noexcept { if (!extension) return; if (!isGnd) extension->d.store(rvt0, isConcurrent); }
     void deleteYiiDC()noexcept { if (!isGnd) extension->yii.store(rvt0, isConcurrent); }
     void deleteFDC()noexcept { extension->f = rvt0; }
```

## hmgComponentModel.h

```diff
--- alap/src/hmgComponentModel.h
+++ javított/src/hmgComponentModel.h
@@ -565,8 +565,9 @@
     //***********************************************************************
         internalNodeIsConcurrent.resize(internalNs_.nNNodes, defaultInternalNodeIsConcurrent_);
     }
     //***********************************************************************
+    InternalNodeSizePack getInternalNodeSizes() const noexcept { return internalNs; }
     uns getN_N_Nodes()const noexcept final override{ return internalNs.nNNodes; }
     uns getN_InternalNodes()const noexcept final override{ return internalNs.nNNodes + internalNs.nBNodes; }
     //***********************************************************************
     //void setNInternalNodes(uns nAll, uns nNormal, bool defaultInternalNodeIsConcurrent) { version++; internalNs.nBNodes = nAll - nNormal; internalNs.nNNodes = nNormal; internalNodeIsConcurrent.resize(nNormal, defaultInternalNodeIsConcurrent); }
```

## hmgComponent.h

```diff
--- alap/src/hmgComponent.h
+++ javított/src/hmgComponent.h
@@ -166,8 +166,9 @@
     virtual void acceptStepDC() noexcept = 0; // VstepStart = Vnode // no AC version
     virtual void calculateComponentControllerValueDC() noexcept {} // no AC version, runs before any calculateValueDC is evaluated => the other components can use what this calcaulated (FCB uses)
     virtual void calculateValueDC() noexcept = 0; // no AC version
     virtual uns calculateControllersDC([[maybe_unused]] uns level, [[maybe_unused]] controllerOperationStage stage) { return 0; } // no AC version, return: next level => if <= current, no next level
+    virtual void collectConvergenceNodes(std::vector<NodeVariable*>&) {}
     virtual DefectCollector collectCurrentDefectDC() const noexcept = 0; // no AC version
     virtual DefectCollector collectVoltageDefectDC() const noexcept = 0; // no AC version
     virtual void printNodeValue() const noexcept = 0;
     virtual rvt getJreducedDC([[maybe_unused]] uns y) const noexcept = 0;
@@ -447,10 +448,10 @@
     //***********************************************************************
         if(isDC) {
             rvt I = componentValue.getValueDC() * (N0->getValueDC() - N1->getValueDC());
             componentCurrent.setValueDC(I);
-            N0->incDDC(-I);
-            N1->incDDC(I);
+            N0->addPhysicalCurrentDC(-I);
+            N1->addPhysicalCurrentDC(I);
         }
         else {
             IAC = componentValue.getValueDC() * (N0->getValueAC() - N1->getValueAC()); // componentValue.getValueDC() !
             N0->incDAC(-IAC);
@@ -506,12 +507,12 @@
     //***********************************************************************
         if(isDC) {
             rvt I = componentValue.getValueDC() * (N[0]->getValueDC() - N[1]->getValueDC());
             componentCurrent.setValueDC(I);
-            N[0]->incDDC(-I);
-            N[1]->incDDC(I);
+            N[0]->addPhysicalCurrentDC(-I);
+            N[1]->addPhysicalCurrentDC(I);
             IMD = I - N[2]->getValueDC() * 1.0; // RM = 1 Ohm
-            N[2]->incDDC(IMD);
+            N[2]->addPhysicalEquationDC(IMD, std::abs(I)+std::abs(N[2]->getValueDC()), N[0], NodeVariable::ConvergenceRole::FlowMonitor);
         }
         else {
             IAC = componentValue.getValueDC() * (N[0]->getValueAC() - N[1]->getValueAC()); // componentValue.getValueDC() !
             N[0]->incDAC(-IAC);
@@ -661,13 +662,13 @@
             PDC = I * V;
             rvt P1 = PDC * share.get();
             rvt P2 = PDC - P1;
             componentCurrent.setValueDC(I);
-            N[0]->incDDC(-I);
-            N[1]->incDDC(I);
+            N[0]->addPhysicalCurrentDC(-I);
+            N[1]->addPhysicalCurrentDC(I);
             //if(fabs(P1) > 1e-12)printf("%g %g %g %g\n", N[0]->getValueDC(), N[1]->getValueDC(), fabs(I)<1e-12?0:I, fabs(P1)<1e-12?0:P1);
-            N[2]->incDDC(P1);
-            N[3]->incDDC(P2);
+            N[2]->addPhysicalCurrentDC(P1);
+            N[3]->addPhysicalCurrentDC(P2);
             //printf("%g %g %g %g\n", N[0]->getValueDC(), N[1]->getValueDC(), N[2]->getValueDC(), N[3]->getValueDC());
         }
         else {
             cplx V = N[0]->getValueAC() - N[1]->getValueAC();
@@ -952,16 +953,16 @@
             PDC = I * V;
             rvt P1 = PDC * share.get();
             rvt P2 = PDC - P1;
             componentCurrent.setValueDC(I);
-            N[0]->incDDC(-I);
-            N[1]->incDDC(I);
+            N[0]->addPhysicalCurrentDC(-I);
+            N[1]->addPhysicalCurrentDC(I);
 
             IMD = I - N[2]->getValueDC() * 1.0; // RM = 1 Ohm
-            N[2]->incDDC(IMD);
+            N[2]->addPhysicalEquationDC(IMD, std::abs(I)+std::abs(N[2]->getValueDC()), N[0], NodeVariable::ConvergenceRole::FlowMonitor);
             //if (fabs(P1) > 1e-12)printf("%g %g %g %g\n", N[0]->getValueDC(), N[1]->getValueDC(), fabs(I) < 1e-12 ? 0 : I, fabs(P1) < 1e-12 ? 0 : P1);
-            N[3]->incDDC(P1);
-            N[4]->incDDC(P2);
+            N[3]->addPhysicalCurrentDC(P1);
+            N[4]->addPhysicalCurrentDC(P2);
         }
         else {
             cplx V = N[0]->getValueAC() - N[1]->getValueAC();
             IAC = componentValue.getValueDC() * V; // componentValue.getValueDC() !
@@ -1135,10 +1136,10 @@
             crvt V = N0->getValueDC() - N1->getValueDC();
             crvt dV = V - V_start;
             crvt I = isTrapezoid ? (dV * Gc - (SimControl::dt.getValueDC() == 0 ? rvt0 : I_stepStart)) : (dV * Gc);
             componentCurrent.setValueDC(I);
-            N0->incDDC(-I);
-            N1->incDDC(I);
+            N0->addPhysicalCurrentDC(-I);
+            N1->addPhysicalCurrentDC(I);
         }
         else {
             IAC = componentValue.getValueDC() * SimControl::getComplexFrequency() * (N0->getValueAC() - N1->getValueAC()); // componentValue.getValueDC() !
             N0->incDAC(-IAC);
@@ -1264,10 +1265,10 @@
     //***********************************************************************
         if (isDC) {
             rvt I = componentValue.getValueDC();
             componentCurrent.setValueDC(-I);
-            N0->incDDC(I);
-            N1->incDDC(-I);
+            N0->addPhysicalCurrentDC(I);
+            N1->addPhysicalCurrentDC(-I);
         }
         else {
             crvt A = param[2].get();
             crvt Phi = param[3].get();
@@ -1308,10 +1309,10 @@
     //***********************************************************************
         if (isDC) {
             rvt I = componentValue.getValueDC();
             componentCurrent.setValueDC(-I);
-            N0->incDDC(I);
-            N1->incDDC(-I);
+            N0->addPhysicalCurrentDC(I);
+            N1->addPhysicalCurrentDC(-I);
         }
         else {
             crvt A = param[2].get() * param[4].get();
             crvt Phi = param[3].get();
@@ -1354,10 +1355,10 @@
             crvt V = componentValue.getValueDC();
             crvt G = param[4].get();
             crvt I = G * (V + N1->getValueDC() - N0->getValueDC());
             componentCurrent.setValueDC(-I);
-            N0->incDDC(I);
-            N1->incDDC(-I);
+            N0->addPhysicalCurrentDC(I);
+            N1->addPhysicalCurrentDC(-I);
         }
         else {
             crvt G = param[4].get();
             crvt A = param[2].get() * G;
@@ -1483,10 +1484,10 @@
     //***********************************************************************
         if (isDC) {
             rvt I = componentValue.getValueDC() * (N[2]->getValueDC() - N[3]->getValueDC());
             componentCurrent.setValueDC(-I);
-            N[0]->incDDC(I);
-            N[1]->incDDC(-I);
+            N[0]->addPhysicalCurrentDC(I);
+            N[1]->addPhysicalCurrentDC(-I);
         }
         else {
             crvt A = param[2].get();
             crvt Phi = param[3].get();
@@ -1628,10 +1629,10 @@
     //***********************************************************************
         if (isDC) {
             rvt I = componentValue.getValueDC();
             componentCurrent.setValueDC(-I);
-            N[0]->incDDC(I);
-            N[1]->incDDC(-I);
+            N[0]->addPhysicalCurrentDC(I);
+            N[1]->addPhysicalCurrentDC(-I);
         }
         else {
             rvt I = componentValue.getValueDC();
             N[0]->incDAC(I);
@@ -1703,12 +1704,12 @@
     //***********************************************************************
         if (isDC) {
             IDC1 = -param[0].get() * (N[2]->getValueDC() - N[3]->getValueDC());
             IDC2 = param[1].get() * (N[0]->getValueDC() - N[1]->getValueDC());
-            N[0]->incDDC(IDC1);
-            N[1]->incDDC(-IDC1);
-            N[2]->incDDC(IDC2);
-            N[3]->incDDC(-IDC2);
+            N[0]->addPhysicalCurrentDC(IDC1);
+            N[1]->addPhysicalCurrentDC(-IDC1);
+            N[2]->addPhysicalCurrentDC(IDC2);
+            N[3]->addPhysicalCurrentDC(-IDC2);
         }
         else {
             IAC1 = -param[0].get() * (N[2]->getValueAC() - N[3]->getValueAC());
             IAC2 = param[1].get() * (N[0]->getValueAC() - N[1]->getValueAC());
@@ -1843,12 +1844,12 @@
             crvt V = -componentValue.getValueDC();
             [[maybe_unused]] crvt G = 1.0 / param[4].get();
             crvt UB = N[2]->getValueDC();
             componentCurrent.setValueDC(UB);
-            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
-            N[1]->incDDC(UB);
+            N[0]->addPhysicalCurrentDC(-UB); // S1 = -1 => I1 = -1 * UB
+            N[1]->addPhysicalCurrentDC(UB);
             crvt JB = -IB + V;// - G * UB
-            N[2]->setDDC(JB);
+            N[2]->addPhysicalEquationDC(JB, std::abs(IB) + std::abs(V), N[0], NodeVariable::ConvergenceRole::FlowConstraint, true);
         }
         else {
             ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
             crvt A = -param[2].get();
@@ -1993,8 +1994,11 @@
         N[0]->incYiiDC(G);
         N[1]->incYiiDC(G);
     }
     //***********************************************************************
+    void collectConvergenceNodes(std::vector<NodeVariable*>& nodes) override {
+        if(possibleCurrentNode && !possibleCurrentNode->isGnd) nodes.push_back(possibleCurrentNode.get());
+    }
     DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
     DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
     void printNodeValue() const noexcept override {}
     rvt getCurrentDC([[maybe_unused]] uns y) const noexcept override { switch (y) { case 0: return -N[2]->getValueDC(); case 1: return N[2]->getValueDC(); default: return rvt0; } }
@@ -2090,12 +2094,12 @@
             crvt V = componentValue.getValueDC();
             [[maybe_unused]] crvt G = 1.0 / param[4].get();
             crvt UB = N[2]->getValueDC();
             componentCurrent.setValueDC(-UB);
-            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
-            N[1]->incDDC(UB);
+            N[0]->addPhysicalCurrentDC(-UB); // S1 = -1 => I1 = -1 * UB
+            N[1]->addPhysicalCurrentDC(UB);
             RJB = IB + V;// - G * UB    => After removing the current of the resistor the voltage of the voltage source is more precise (= the error caused the resistor is added as (-G * UB) + (G * UB) = 0
-            N[2]->incDDC(RJB);
+            N[2]->addPhysicalEquationDC(RJB, std::abs(IB) + std::abs(V), N[0], NodeVariable::ConvergenceRole::FlowConstraint);
         }
         else {
             ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
             crvt A = param[2].get();
@@ -2253,12 +2257,12 @@
         if (isDC) {
             crvt IB = N[1]->getValueDC() - N[0]->getValueDC(); // S2 = -1
             crvt UB = N[2]->getValueDC();
             componentCurrent.setValueDC(UB);
-            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
-            N[1]->incDDC(UB);
+            N[0]->addPhysicalCurrentDC(-UB); // S1 = -1 => I1 = -1 * UB
+            N[1]->addPhysicalCurrentDC(UB);
             crvt JB = -IB;// - G * UB
-            N[2]->setDDC(JB);
+            N[2]->addPhysicalEquationDC(JB, std::abs(IB), N[0], NodeVariable::ConvergenceRole::FlowConstraint, true);
         }
         else {
             ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
             ccplx UB = N[2]->getValueAC();
@@ -2379,8 +2383,11 @@
         N[0]->incYiiDC(G);
         N[1]->incYiiDC(G);
     }
     //***********************************************************************
+    void collectConvergenceNodes(std::vector<NodeVariable*>& nodes) override {
+        if(possibleCurrentNode && !possibleCurrentNode->isGnd) nodes.push_back(possibleCurrentNode.get());
+    }
     DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
     DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
     void printNodeValue() const noexcept override {}
     rvt getCurrentDC([[maybe_unused]] uns y) const noexcept override { switch (y) { case 0: return -N[2]->getValueDC(); case 1: return N[2]->getValueDC(); default: return rvt0; } }
@@ -2475,12 +2482,12 @@
             crvt IB = N[1]->getValueDC() - N[0]->getValueDC(); // S2 = -1
             [[maybe_unused]] crvt G = 1.0 / param.get();
             crvt UB = N[2]->getValueDC();
             componentCurrent.setValueDC(-UB);
-            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
-            N[1]->incDDC(UB);
+            N[0]->addPhysicalCurrentDC(-UB); // S1 = -1 => I1 = -1 * UB
+            N[1]->addPhysicalCurrentDC(UB);
             RJB = IB;// - G * UB
-            N[2]->incDDC(RJB);
+            N[2]->addPhysicalEquationDC(RJB, std::abs(IB), N[0], NodeVariable::ConvergenceRole::FlowConstraint);
         }
         else {
             ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
             [[maybe_unused]] crvt G = 1.0 / param.get();
@@ -2640,10 +2647,10 @@
             crvt G = pars[0].get();
             crvt IG = G * (externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC());
             crvt Ifull = componentValue.getValueDC() - IG;
             componentCurrent.setValueDC(-Ifull);
-            externalNodes[0]->incDDC(Ifull);
-            externalNodes[1]->incDDC(-Ifull);
+            externalNodes[0]->addPhysicalCurrentDC(Ifull);
+            externalNodes[1]->addPhysicalCurrentDC(-Ifull);
             // printf("** G = %g\tIG = %g\t Ifull = %g A U = %g**\n", G, IG, Ifull, externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC());
         }
         else {
             crvt G = pars[0].get();
@@ -2852,16 +2859,16 @@
             rvt V = externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC();
             crvt IG = G * V;
             crvt Ifull = componentValue.getValueDC() - IG; // R or G components: I sign is opposite !
             componentCurrent.setValueDC(-Ifull);
-            externalNodes[0]->incDDC(Ifull);
-            externalNodes[1]->incDDC(-Ifull);
+            externalNodes[0]->addPhysicalCurrentDC(Ifull);
+            externalNodes[1]->addPhysicalCurrentDC(-Ifull);
 
             PDC = -Ifull * V;
             rvt P1 = PDC * pars[0].get(); // dissipated power share
             rvt P2 = PDC - P1;
-            externalNodes[2]->incDDC(P1);
-            externalNodes[3]->incDDC(P2);
+            externalNodes[2]->addPhysicalCurrentDC(P1);
+            externalNodes[3]->addPhysicalCurrentDC(P2);
         }
         else {
             crvt G = pars[1].get();
             ccplx V = externalNodes[0]->getValueAC() - externalNodes[1]->getValueAC();
@@ -3124,12 +3131,12 @@
             crvt G = pars[0].get();
             crvt IG = G * (externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC());
             crvt Ifull = componentValue.getValueDC() - IG;
             componentCurrent.setValueDC(-Ifull);
-            externalNodes[0]->incDDC(Ifull);
-            externalNodes[1]->incDDC(-Ifull);
+            externalNodes[0]->addPhysicalCurrentDC(Ifull);
+            externalNodes[1]->addPhysicalCurrentDC(-Ifull);
             IMD = -Ifull - externalNodes[2]->getValueDC() * 1.0; // RM = 1 Ohm
-            externalNodes[2]->incDDC(IMD);
+            externalNodes[2]->addPhysicalEquationDC(IMD, std::abs(Ifull)+std::abs(externalNodes[2]->getValueDC()), externalNodes[0], NodeVariable::ConvergenceRole::FlowMonitor);
             // printf("** G = %g\tIG = %g\t Ifull = %g A U = %g**\n", G, IG, Ifull, externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC());
         }
         else {
             crvt G = pars[0].get();
@@ -3374,19 +3381,19 @@
             rvt V = externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC();
             crvt IG = G * V;
             crvt Ifull = componentValue.getValueDC() - IG; // R or G components: I sign is opposite !
             componentCurrent.setValueDC(-Ifull);
-            externalNodes[0]->incDDC(Ifull);
-            externalNodes[1]->incDDC(-Ifull);
+            externalNodes[0]->addPhysicalCurrentDC(Ifull);
+            externalNodes[1]->addPhysicalCurrentDC(-Ifull);
 
             IMD = -Ifull - externalNodes[2]->getValueDC() * 1.0; // RM = 1 Ohm
-            externalNodes[2]->incDDC(IMD);
+            externalNodes[2]->addPhysicalEquationDC(IMD, std::abs(Ifull)+std::abs(externalNodes[2]->getValueDC()), externalNodes[0], NodeVariable::ConvergenceRole::FlowMonitor);
 
             PDC = -Ifull * V;
             rvt P1 = PDC * pars[0].get(); // dissipated power share
             rvt P2 = PDC - P1;
-            externalNodes[3]->incDDC(P1);
-            externalNodes[4]->incDDC(P2);
+            externalNodes[3]->addPhysicalCurrentDC(P1);
+            externalNodes[4]->addPhysicalCurrentDC(P2);
         }
         else {
             crvt G = pars[1].get();
             ccplx V = externalNodes[0]->getValueAC() - externalNodes[1]->getValueAC();
@@ -4276,8 +4283,19 @@
             if (components[i]->isEnabled) components[i]->calculateValueDC();
         // setting own value should be here
     }
     //***********************************************************************
+    void collectConvergenceNodes(std::vector<NodeVariable*>& nodes) override {
+        const auto& model=static_cast<const ModelSubCircuit&>(*pModel);
+        for(uns i=0;i<model.getN_N_Nodes();++i)
+            if(internalNodesAndVars[i].extension && !internalNodesAndVars[i].isGnd) nodes.push_back(&internalNodesAndVars[i]);
+        // O ports are reduced here, but stored in externalNodes, not internalNodesAndVars.
+        for(uns i=0;i<model.getN_O_Nodes();++i) {
+            auto* node=externalNodes[model.getN_Start_Of_O_Nodes()+i];
+            if(node->extension && !node->isGnd) nodes.push_back(node);
+        }
+        for(auto& component:components) if(component->isEnabled) component->collectConvergenceNodes(nodes);
+    }
     DefectCollector collectCurrentDefectDC() const noexcept override {
     // TO PARALLEL
     //***********************************************************************
         DefectCollector d;
@@ -4345,12 +4363,12 @@
     // TO PARALLEL
     //***********************************************************************
         const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
         if (isNoAlpha)
-            for (uns i = 0; i < model.getN_InternalNodes(); i++) // the ONodes come from outside
+            for (uns i = 0; i < nInternalNodesAndVars; i++) // Includes locally owned, unconnected O ports.
                 internalNodesAndVars[i].setValueAcceptedNoAlphaDC();
         else {
-            for (uns i = 0; i < model.getN_InternalNodes(); i++) // the ONodes come from outside
+            for (uns i = 0; i < nInternalNodesAndVars; i++) // Includes locally owned, unconnected O ports.
                 internalNodesAndVars[i].setValueAcceptedDC();
         }
         for (auto& comp : components)
             if (comp->isEnabled) comp->acceptIterationDC(isNoAlpha);
@@ -4413,10 +4431,9 @@
     //***********************************************************************
     void acceptIterationAndStepAC() noexcept override { // sunred and multigrid
     // TO PARALLEL
     //***********************************************************************
-        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
-        for (uns i = 0; i < model.getN_InternalNodes(); i++) // the ONodes come from outside
+        for (uns i = 0; i < nInternalNodesAndVars; i++) // Includes locally owned, unconnected O ports.
             internalNodesAndVars[i].setValueAcceptedAC();
         for (auto& comp : components)
             if (comp->isEnabled) comp->acceptIterationAndStepAC();
     }
```

## hmgHMGFileReader.cpp

```diff
--- alap/src/hmgHMGFileReader.cpp
+++ javított/src/hmgHMGFileReader.cpp
@@ -8,8 +8,9 @@
 #define _CRT_SECURE_NO_WARNINGS
 //***********************************************************************
 #include <iostream>
 #include "hmgHMGFileReader.h"
+#include "hmgOptimization.h"
 #include <cstdio>
 #include <stack>
 #include <set>
 #include <charconv>
@@ -270,11 +271,8 @@
 
 
 //***********************************************************************
 bool ReadALine::skipOptionalBlock() {
-// OPTIONAL is a physical-file filter, before INCLUDE and '+' processing.
-// actLine already denotes the physical line at the current cursor.
-//***********************************************************************
     constexpr char opening[] = ".OPTIONAL";
     constexpr char closing[] = ".END OPTIONAL";
     constexpr unsigned openingSize = sizeof(opening) - 1;
     for (unsigned i = 0; i < openingSize; ++i) {
@@ -283,9 +281,9 @@
             return false;
     }
     const int after = peekRawChar(openingSize);
     if (!(after == EOF || isspace(static_cast<unsigned char>(after)) ||
-        after == ';' || (after == '/' && peekRawChar(openingSize + 1) == '/')))
+          after == ';' || (after == '/' && peekRawChar(openingSize + 1) == '/')))
         return false; // .OPTIONALLY is not an OPTIONAL directive.
 
     const unsigned openingLine = actLine;
     const auto missingEnd = [&]() {
@@ -293,19 +291,27 @@
             "missing .END OPTIONAL in the same file as .OPTIONAL in %s, line %u",
             names[fileNameIndex].name.c_str(), openingLine);
     };
     int ch;
-    while ((ch = getRawChar()) != EOF && ch != '\n') {} // opaque header
+    std::string header;
+    while ((ch = getRawChar()) != EOF && ch != '\n') { if(header.size()<256)header.push_back(static_cast<char>(ch)); }
+    const bool interpret=optimization::knownHeader(header,names[fileNameIndex].name,openingLine);
+    std::vector<std::string> lines;
+    size_t optionBytes=0;
     if (ch == EOF) missingEnd();
     if (peekRawChar(0) == '\r') getRawChar(); // historical LF-CR support
     ++actLine;
 
     for (;;) {
         // A bounded, whitespace-normalized prefix is enough to recognize the
         // end marker. Arbitrarily long skipped lines do not grow memory.
-        std::string prefix;
+        std::string prefix, raw;
         bool comment = false;
         while ((ch = getRawChar()) != EOF && ch != '\n') {
+            if(interpret) {
+                if(raw.size()>=4096 || lines.size()>=1000000 || ++optionBytes>64u*1024u*1024u)throw hmgExcept("HEXMG_OPT","option block too large in %s, line %u",names[fileNameIndex].name.c_str(),openingLine);
+                raw.push_back(static_cast<char>(ch));
+            }
             if (comment) continue;
             if (ch == ';' || (ch == '/' && peekRawChar(0) == '/')) {
                 comment = true;
                 continue;
@@ -321,9 +327,13 @@
         if (ch == '\n') {
             if (peekRawChar(0) == '\r') getRawChar();
             ++actLine;
         }
-        if (prefix == closing) return true; // first closing marker, no nesting
+        if (prefix == closing) {
+            if(interpret)optimization::parse(lines,names[fileNameIndex].name,openingLine);
+            return true;
+        }
+        if(interpret)lines.push_back(raw); // first closing marker, no nesting
         if (ch == EOF) missingEnd(); // never continue in a parent/include file
     }
 }
 
@@ -825,8 +835,21 @@
 
     // reading the simulation
 
     globalCircuit.Read(reader, line, lineInfo);
+    if (!optimization::convergenceNodes.empty() && !optimization::relativeConvergence)
+        throw hmgExcept("convergence", "CONVERGENCE_NODE requires CONVERGENCE RELATIVE");
+    for(auto& option : optimization::convergenceNodes) {
+        auto& names=HMGFileListItem::globalNames;
+        const auto circuit=names.fullCircuitNames.find(option.circuit);
+        if(circuit==names.fullCircuitNames.end()) throw hmgExcept("convergence", "Unknown circuit %s",option.circuit.c_str());
+        option.circuitId=circuit->second;
+        std::vector<char> node(option.node.begin(),option.node.end()); node.push_back(0);
+        if(!names.textToDeepInterfaceNodeID(node.data(),option.circuitId,option.id) || option.id.isController ||
+           option.id.nodeID.isStepStart || option.id.nodeID.type==nvtBG || option.id.nodeID.type==nvtRail || option.id.nodeID.type==nvtGND)
+            throw hmgExcept("convergence", "Expected a solved circuit node: %s",option.node.c_str());
+    }
+
     //most("Read");
     //globalCircuit.ProcessXLines();
     //most("ProcessXLines");
     //globalCircuit.ProcessExpressionNames(&globalCircuit.itemList);
@@ -1891,10 +1914,14 @@
         if (strcmp(token, "INITIAL") == 0) data.isInitial = true;
         else if (strcmp(token, "ITER") == 0) data.iterNumSPD = 1;
         else if (strcmp(token, "ITERS") == 0) {
             token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
-            if(sscanf_s(token, "%u", &data.iterNumSPD) != 1)
-                throw hmgExcept("HMGFileRun::Read", "unrecognised ITERS number (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
+            const char* end = token + std::strlen(token);
+            const auto parsed = std::from_chars(token, end, data.iterNumSPD);
+            if (parsed.ec != std::errc{} || parsed.ptr != end || data.iterNumSPD == 0)
+                throw hmgExcept("HMGFileRun::Read",
+                    "ITERS must be a positive integer (%s) in %s, line %u: %s",
+                    token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
         }
         else if (strcmp(token, "PRE") == 0) data.isPre = true;
         else if (strcmp(token, "ERR") == 0) {
             token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
```

## hmgSimulation.h

```diff
--- alap/src/hmgSimulation.h
+++ javított/src/hmgSimulation.h
@@ -34,10 +34,11 @@
     bool isBuiltForAC = false;
     bool isTau = false;
 
     void iterate(); // always DC
-    void runDC();
-    void runTimeStep();
+    void runRelativeDC(bool timeStep, uns requestedIterations);
+    void runDC(uns requestedIterations = 0);
+    void runTimeStep(uns requestedIterations = 0);
     void runAC();
 public:
     //***********************************************************************
     Simulation() = default;
```

## hmgSimulation.cpp

```diff
--- alap/src/hmgSimulation.cpp
+++ javított/src/hmgSimulation.cpp
@@ -7,8 +7,10 @@
 
 //***********************************************************************
 #include "hmgComponent.h"
 #include "hmgSimulation.h"
+#include <cmath>
+#include "hmgOptimization.h"
 //***********************************************************************
 
 
 //***********************************************************************
@@ -16,175 +18,170 @@
 //***********************************************************************
 
 
 //***********************************************************************
-static inline rvt defectError(const ComponentBase::DefectCollector& d) {
-//***********************************************************************
-    return (d.nodeNum == 0 ? rvt0 : d.sumDefect / d.nodeNum) + d.maxDefect;
-}
-
-
-//***********************************************************************
-static inline void requireValidErr(rvt e) {
-//***********************************************************************
+void Simulation::iterate() {
+//***********************************************************************
+
+}
+
+//***********************************************************************
+uns curretIterN = 0;
+//***********************************************************************
+
+
+//***********************************************************************
+// The legacy defect metric is intentionally retained: it is not relative.
+static rvt defectError(const ComponentBase::DefectCollector& d) {
+    const rvt e = (d.nodeNum == 0 ? rvt0 : d.sumDefect / d.nodeNum) + d.maxDefect;
+    if (!(std::isfinite(e) && e >= rvt0))
+        throw hmgExcept("defectError", "Non-finite or invalid iteration defect");
+    return e;
+}
+
+static void requireValidErr(rvt e) {
     if (!(std::isfinite(e) && e > rvt0))
         throw hmgExcept("requireValidErr", "ERR must be finite and positive: %g", e);
 }
 
-
-//***********************************************************************
-static inline void requireConverged(rvt e, rvt limit) {
-//***********************************************************************
+static void requireConverged(rvt e, rvt limit) {
     if (!(std::isfinite(e) && e <= limit))
         throw hmgExcept("requireConverged", "Did not converge: error=%g, ERR=%g", e, limit);
 }
 
-
-//***********************************************************************
-void Simulation::iterate() {
-//***********************************************************************
-
-}
-
-//***********************************************************************
-uns curretIterN = 0;
-//***********************************************************************
-
-
-//***********************************************************************
-void Simulation::runDC() {
-//***********************************************************************
-    requireValidErr(err);
-
-    // TODO: SimControl::minIter
-
+// A separate path leaves legacy decks and multigrid stopping rules unchanged.
+void Simulation::runRelativeDC(bool timeStep, uns requestedIterations) {
+    if(requestedIterations==0) requireValidErr(err);
+    const double relative=requestedIterations ? RunData{}.err : err;
+    auto& storage=CircuitStorage::getInstance();
+    std::vector<NodeVariable*> nodes;
+    storage.fullCircuitInstances[fullCircuitID].component->collectConvergenceNodes(nodes);
+    std::sort(nodes.begin(),nodes.end(),std::less<NodeVariable*>{});
+    nodes.erase(std::unique(nodes.begin(),nodes.end()),nodes.end());
+    for(auto* node:nodes) node->convergenceProfile.reset();
+    std::set<NodeVariable*> configured;
+    for(const auto& option:optimization::convergenceNodes) {
+        if(option.circuitId!=fullCircuitID) continue;
+        const ComponentBase* component=storage.fullCircuitInstances[fullCircuitID].component.get();
+        for(auto index:option.id.componentID) {
+            const auto* sub=dynamic_cast<const ComponentSubCircuit*>(component);
+            if(!sub || index>=sub->getNContainedComponents()) throw hmgExcept("convergence","Invalid component path %s",option.node.c_str());
+            component=sub->getContainedComponent(index);
+        }
+        const auto* sub=dynamic_cast<const ComponentSubCircuit*>(component);
+        DeepCDNodeID id{SimpleInterfaceNodeID2CDNode(option.id.nodeID,component->pModel->externalNs,
+            sub?static_cast<const ModelSubCircuit*>(sub->pModel)->getInternalNodeSizes():InternalNodeSizePack{}),option.id.componentID,false};
+        if(id.nodeID.type!=cdntInternal && id.nodeID.type!=cdntExternal)
+            throw hmgExcept("convergence","Not a solved node: %s",option.node.c_str());
+        auto* node=const_cast<NodeVariable*>(&storage.getNode(fullCircuitID,id));
+        if(!std::binary_search(nodes.begin(),nodes.end(),node,std::less<NodeVariable*>{}) || !configured.insert(node).second)
+            throw hmgExcept("convergence","Unsolved or aliased repeated node: %s",option.node.c_str());
+        node->convergenceProfile=option.profile;
+    }
+    const auto refresh=[&] {
+        for(auto* node:nodes) node->clearPhysicalMagnitude();
+        CircuitStorage::CalculateControllersDC(fullCircuitID);
+        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
+    };
+    const auto measure=[&](bool change) {
+        convergence::Collector result;
+        for(auto* node:nodes) {
+            try { result.add(change?node->relativeChangeError(relative):node->relativeBalanceError(relative)); }
+            catch(const std::exception& e) {
+                throw hmgExcept("convergence","Circuit %u node %u/%u/%u/%u %s: %s",fullCircuitID,
+                    node->nid[3],node->nid[2],node->nid[1],node->nid[0],change?"state":"balance",e.what());
+            }
+        }
+        return result.maximum();
+    };
+    curretIterN=0;
+    double stateError=0, balanceError=0;
+    const unsigned limit=requestedIterations?requestedIterations:
+        optimization::maxIterations?optimization::maxIterations:(timeStep?100u:101u);
+    refresh();
+    (void)measure(false); // Reject an invalid starting physical state before reduction.
+    do {
+        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
+        stateError=measure(true);
+        CircuitStorage::AcceptIterationDC(fullCircuitID);
+        ++curretIterN;
+        refresh(); // Includes capacitor currents before accepting the step history.
+        balanceError=measure(false);
+        std::cout << "Relative iter " << curretIterN << " state=" << stateError << " balance=" << balanceError << std::endl;
+    } while(curretIterN<limit && (requestedIterations || stateError>1 || balanceError>1));
+    if(!requestedIterations && !(stateError<=1 && balanceError<=1))
+        throw hmgExcept("convergence","Circuit %u did not converge in %u updates: normalized state=%g, balance=%g (both must be <=1)",
+            fullCircuitID,curretIterN,stateError,balanceError);
+    CircuitStorage::AcceptStepDC(fullCircuitID);
+    if(!timeStep) wasDC=true;
+}
+
+void Simulation::runDC(uns requestedIterations) {
+    if(optimization::relativeConvergence) { runRelativeDC(false,requestedIterations); return; }
+    if (requestedIterations == 0) requireValidErr(err);
     curretIterN = 0;
-    CircuitStorage& gc = CircuitStorage::getInstance();
-
-    bench_now("start DC");
-    CircuitStorage::CalculateControllersDC(fullCircuitID);
-    bench_now("CalculateControllersDC");
-    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-  //printf("CalculateValuesAndCurrents\n");
-  //gc.fullCircuitInstances[0].component->printNodeValue();
-    bench_now("CalculateValuesAndCurrentsDC");
-    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
-  //printf("ForwsubsBacksubs\n");
-  //gc.fullCircuitInstances[0].component->printNodeValue();
-    bench_now("ForwsubsBacksubsDC");
-    CircuitStorage::AcceptIterationDC(fullCircuitID);
-  //printf("AcceptIteration\n");
-  //gc.fullCircuitInstances[0].component->printNodeValue();
-    bench_now("AcceptIterationDC");
-
-    curretIterN++;
-
-    CircuitStorage::CalculateControllersDC(fullCircuitID);
-    bench_now("CalculateControllersDC");
-    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-    bench_now("CalculateValuesAndCurrentsDC");
-  //printf("CalculateValuesAndCurrents\n");
-  //gc.fullCircuitInstances[0].component->printNodeValue();
-
-/*
-#ifdef HMG_DEBUGPRINT
-    std::cout << std::endl;
-    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
-#endif
-
-    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
-    CircuitStorage::AcceptIterationDC(fullCircuitID);
+    auto* component = CircuitStorage::getInstance().fullCircuitInstances[fullCircuitID].component.get();
     CircuitStorage::CalculateControllersDC(fullCircuitID);
     CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-
-#ifdef HMG_DEBUGPRINT
-    std::cout << std::endl;
-    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
-#endif
-
     CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
+    // Corrections must be sampled before AcceptIterationDC clears them.
+    const rvt firstVoltageError = defectError(component->collectVoltageDefectDC());
     CircuitStorage::AcceptIterationDC(fullCircuitID);
+    ++curretIterN;
     CircuitStorage::CalculateControllersDC(fullCircuitID);
     CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-
-#ifdef HMG_DEBUGPRINT
-    std::cout << std::endl;
-    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
-#endif
-    bench_now("first iterations");
-*/
-    rvt max_error = 1000.0;
-    for (uns i = 0; (i == 0 || max_error > err) && i < 100; i++) {
+    rvt max_error = firstVoltageError + defectError(component->collectCurrentDefectDC());
+    if (!std::isfinite(max_error)) throw hmgExcept("Simulation::runDC", "Non-finite first iteration");
+    // Preserve the legacy complete-solve bound: one initial + 100 loop updates.
+    // A positive request instead means exactly that many TOTAL updates.
+    for (uns i = 0; requestedIterations != 0
+        ? i < requestedIterations - 1
+        : (i == 0 || max_error > err) && curretIterN < (optimization::maxIterations ? optimization::maxIterations : 101u); ++i) {
         CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
-      //printf("ForwsubsBacksubs\n");
-      //gc.fullCircuitInstances[0].component->printNodeValue();
-        ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
+        const rvt vErr = defectError(component->collectVoltageDefectDC());
         CircuitStorage::AcceptIterationDC(fullCircuitID);
-      //printf("AcceptIteration\n");
-      //gc.fullCircuitInstances[0].component->printNodeValue();
-        curretIterN++;
+        ++curretIterN;
         CircuitStorage::CalculateControllersDC(fullCircuitID);
         CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-      //printf("CalculateValuesAndCurrents\n");
-      //gc.fullCircuitInstances[0].component->printNodeValue();
-        ComponentBase::DefectCollector c = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
-
-        rvt cErr = defectError(c);
-        rvt vErr = defectError(v);
+        const rvt cErr = defectError(component->collectCurrentDefectDC());
         max_error = cErr + vErr;
-        std::cout << "Iter " << (i + 1) << "    error = " << max_error << "    I error = " << cErr << "    V error = " << vErr << std::endl;
-#ifdef HMG_DEBUGPRINT
-        std::cout << std::endl;
-        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
-#endif
-        bench_now("iteration");
-    }
-/*
-    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
-    CircuitStorage::AcceptIterationDC(fullCircuitID);
+        if (!std::isfinite(max_error)) throw hmgExcept("Simulation::runDC", "Non-finite iteration");
+        std::cout << "Iter " << (i + 1) << "    error = " << max_error
+                  << "    I error = " << cErr << "    V error = " << vErr << std::endl;
+    }
+    if (requestedIterations == 0) requireConverged(max_error, err);
     CircuitStorage::AcceptStepDC(fullCircuitID);
+    wasDC = true;
+}
+
+void Simulation::runTimeStep(uns requestedIterations) {
+    if(optimization::relativeConvergence) { runRelativeDC(true,requestedIterations); return; }
+    if (requestedIterations == 0) requireValidErr(err);
+    curretIterN = 0;
+    rvt max_error = 0;
+    auto* component = CircuitStorage::getInstance().fullCircuitInstances[fullCircuitID].component.get();
+    for (uns i = 0; requestedIterations != 0
+        ? i < requestedIterations
+        : (i == 0 || max_error > err) && i < (optimization::maxIterations ? optimization::maxIterations : 100u); ++i) {
+        CircuitStorage::CalculateControllersDC(fullCircuitID);
+        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
+        const rvt cErr = defectError(component->collectCurrentDefectDC());
+        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
+        const rvt vErr = defectError(component->collectVoltageDefectDC());
+        CircuitStorage::AcceptIterationDC(fullCircuitID);
+        ++curretIterN;
+        max_error = cErr + vErr;
+        if (!std::isfinite(max_error)) throw hmgExcept("Simulation::runTimeStep", "Non-finite iteration");
+        std::cout << "    error = " << max_error << std::endl;
+    }
+    if (requestedIterations == 0) requireConverged(max_error, err);
+    // Refresh currents while capacitor history still refers to the old step.
     CircuitStorage::CalculateControllersDC(fullCircuitID);
     CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-
-#ifdef HMG_DEBUGPRINT
-    std::cout << std::endl;
-    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
-#endif
-*/
-    requireConverged(max_error, err);
+    (void)defectError(component->collectCurrentDefectDC());
     CircuitStorage::AcceptStepDC(fullCircuitID);
-    wasDC = true;
-}
-
-
-//***********************************************************************
-void Simulation::runTimeStep() {
-//***********************************************************************
-    requireValidErr(err);
-    rvt max_error = 1000.0 * err;
-    CircuitStorage& gc = CircuitStorage::getInstance();
-    for (uns i = 0; (i == 0 || max_error > err) && i < 100; i++) {
-        CircuitStorage::CalculateControllersDC(fullCircuitID);
-        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-        ComponentBase::DefectCollector c = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
-        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
-        ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
-        CircuitStorage::AcceptIterationDC(fullCircuitID);
-        max_error = defectError(c) + defectError(v);
-        std::cout << "    error = " << max_error << std::endl;
-    }
-    requireConverged(max_error, err);
-    CircuitStorage::CalculateControllersDC(fullCircuitID);
-    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
-    CircuitStorage::AcceptStepDC(fullCircuitID);
-#ifdef HMG_DEBUGPRINT
-    std::cout << std::endl;
-    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
-#endif
-}
-
-
-//***********************************************************************
+}
+
 void Simulation::runAC() {
 //***********************************************************************
     CircuitStorage& gc = CircuitStorage::getInstance();
     if (!isBuiltForAC) {
@@ -219,8 +216,9 @@
     // MULTIGRID
     // ******************************************************************
 
     if (runData.isMultigrid) {
+        if(optimization::relativeConvergence) throw hmgExcept("convergence","Relative convergence is not implemented for multigrid runs");
 
         if (runData.isInitial)
             SimControl::setInitialDC();
         else
@@ -250,9 +248,14 @@
         // TODO: runData.isPre
         timeFreqValue = rvt0;
         dtValue = rvt0;
         SimControl::stepError.setValueDC(rvt0);
+        // ITER/ITERS on a timestep does not limit its automatic initial DC.
+        // ERR is ignored for a fixed-step request, including this hidden solve.
+        const rvt requestedErr = err;
+        if (analysisType == atTimeStep && runData.iterNumSPD != 0) err = RunData{}.err;
         runDC();
+        err = requestedErr;
     }
     switch (analysisType) {
         case atDC: {
             if(runData.isInitial)
@@ -262,9 +265,9 @@
             // TODO: runData.isPre
             timeFreqValue = rvt0;
             dtValue = rvt0;
             SimControl::stepError.setValueDC(rvt0);
-            runDC();
+            runDC(runData.iterNumSPD);
         }
         break;
         case atTimeStep: {
             if (runData.isDT)
@@ -275,9 +278,9 @@
             timeFreqValue = SimControl::timeStepStop.getValueDC();
             dtValue = SimControl::dt.getValueDC();
             SimControl::stepError.setValueDC(rvt0);
             std::cout << "Time = " << timeFreqValue << " s\n";
-            runTimeStep();
+            runTimeStep(runData.iterNumSPD);
         }
         break;
         case atAC: {
             SimControl::setComplexFrequencyForAC(runData.fTauDtT);
```
