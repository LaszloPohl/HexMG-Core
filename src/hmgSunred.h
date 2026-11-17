//***********************************************************************
// HexMG Successive Network Reduction Engine
// Creation date:  2023. 03. 15.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_SUNRED_HEADER
#define	HMG_SUNRED_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgCommon.h"
#include "hmgMatrix.hpp"
#include "hmgOptimization.h"
#include <type_traits>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
template<class Real>
struct SunredReductorDC {
//***********************************************************************
    struct CalcPack {
        matrix<Real> XAT, XB, YB_NZB;
        matrix<Real> NZBXA, NZBXAT;
        vektor<Real> JAUA, JBUB, NZBJB;
    };
    struct LeafPack {
        matrix<Real> YA;
        vektor<Real> JA;
    };
    matrix<Real> YRED;
    vektor<Real> JRED;
    std::unique_ptr<CalcPack> calc;
    std::unique_ptr<LeafPack> leaf; // only if there are common (connected) nodes, so the YRED and JRED is not the same as the YRED and JRED of the source component
};


//***********************************************************************
struct SunredReductorAC {
//***********************************************************************
    struct CalcPack {
        matrix<cplx> XAT, XB, YB_NZB;
        matrix<cplx> NZBXA, NZBXAT;
        vektor<cplx> JAUA, JBUB, NZBJB;
    };
    struct LeafPack {
        matrix<cplx> YA;
        vektor<cplx> JA;
    };
    matrix<cplx> YRED;
    vektor<cplx> JRED;
    std::unique_ptr<CalcPack> calc;
    std::unique_ptr<LeafPack> leaf; // only if there are common (connected) nodes, so the YRED and JRED is not the same as the YRED and JRED of the source component
};


class ComponentBase;
class ComponentSubCircuit;
class hmgSunred;
//***********************************************************************
class SunredTreeNode {
//***********************************************************************
    struct IndexPair {
        uns componentTerminalIndex = 0;
        uns nodeIndex = 0;
    };
    struct Connections {
        uns nodeIndex = 0;
        ush sumMergers = 0;
        ush mergersSoFar = 0;
    };
    //hmgSunred* owner = nullptr;
    ComponentBase* srcComponent = nullptr;
    SunredTreeNode* srcCell1 = nullptr;
    SunredTreeNode* srcCell2 = nullptr;
    std::vector<Connections> ANodeIndex; // node in the subcircuit; if (ANodeIndex & XNodeFlag) == 0 => internal node, else external; ANodeIndex.size() = Arowcol
    std::vector<uns> BNodeIndex; // node in the subcircuit; only internal nodes can be in B block !; BNodeIndex.size() = Browcol
    std::vector<IndexPair> CNodeIndex; // used in leafs only
    bool isSymmDC = false, isSymmAC = false;
    bool isChangedDC = false, isChangedAC = false;
    bool initializedDC = false, initializedAC = false;
    std::unique_ptr<SunredReductorDC<rvt>> dcDouble;
    std::unique_ptr<SunredReductorDC<xxt>> dcWide;
    bool wideDC = false;
    template<class Real> auto& dcData() {
        if constexpr(std::is_same_v<Real,rvt>) return dcDouble;
        else return dcWide;
    }
    std::unique_ptr<SunredReductorAC> ac;
public:
    //***********************************************************************
    void loadLeafDataFromSubcircuit(ComponentBase* src, ComponentSubCircuit* pSubckt);
    void loadNodeDataFromTwoNodes(SunredTreeNode* src1, SunredTreeNode* src2);
    //***********************************************************************
    void allocDC() {
        wideDC=optimization::enabled &&
            optimization::sunredPrecision==optimization::Precision::Extended;
        dcDouble.reset();dcWide.reset();
        if(wideDC)allocDCTyped<xxt>();else allocDCTyped<rvt>();
    }
    template<class Real> void allocDCTyped() {
        auto& dc=dcData<Real>();
    //***********************************************************************
        if (srcComponent != nullptr || srcCell1 != nullptr) {

            cuns Asiz = (uns)ANodeIndex.size();
            cuns Bsiz = (uns)BNodeIndex.size();
            cuns Csiz = (uns)CNodeIndex.size();

            initializedDC = false;
            dc = std::make_unique<SunredReductorDC<Real>>();
            dc->YRED.resize_if_needed(Asiz, Asiz, isSymmDC);
            dc->JRED.resize_if_needed(Asiz);

            if (srcComponent != nullptr) { // leaf
                if (Asiz != Csiz) {
                    dc->leaf = std::make_unique<typename SunredReductorDC<Real>::LeafPack>();
                    dc->leaf->YA.resize_if_needed(Csiz, Csiz, isSymmDC);
                    dc->leaf->JA.resize_if_needed(Csiz);
                }
            }
            else { // nonleaf
                dc->calc = std::make_unique<typename SunredReductorDC<Real>::CalcPack>();
                if (!isSymmDC) dc->calc->XAT.resize_if_needed(Asiz, Bsiz, false);
                dc->calc->XB.resize_if_needed(Asiz, Bsiz, false);
                dc->calc->YB_NZB.resize_if_needed(Bsiz, Bsiz, false); // never symmetrical!
                dc->calc->NZBXA.resize_if_needed(Bsiz, Asiz, false);
                dc->calc->NZBXAT.resize_if_needed(Asiz, Bsiz, false);
                dc->calc->JAUA.resize_if_needed(Asiz);
                dc->calc->JBUB.resize_if_needed(Bsiz);
                dc->calc->NZBJB.resize_if_needed(Bsiz);
            }
        } // else: empty node, belsongs to a disabled component, nothing to do
    }
    //***********************************************************************
    void allocAC() {
    //***********************************************************************
        if (srcComponent != nullptr || srcCell1 != nullptr) {

            cuns Asiz = (uns)ANodeIndex.size();
            cuns Bsiz = (uns)BNodeIndex.size();
            cuns Csiz = (uns)CNodeIndex.size();

            initializedAC = false;
            ac = std::make_unique<SunredReductorAC>();
            ac->YRED.resize_if_needed(Asiz, Asiz, isSymmAC);
            ac->JRED.resize_if_needed(Asiz);

            if (srcComponent != nullptr) { // leaf
                if (Asiz != Csiz) {
                    ac->leaf = std::make_unique<SunredReductorAC::LeafPack>();
                    ac->leaf->YA.resize_if_needed(Csiz, Csiz, isSymmAC);
                    ac->leaf->JA.resize_if_needed(Csiz);
                }
            }
            else { // nonleaf
                ac->calc = std::make_unique<SunredReductorAC::CalcPack>();
                if (!isSymmAC) ac->calc->XAT.resize_if_needed(Asiz, Bsiz, false);
                ac->calc->XB.resize_if_needed(Asiz, Bsiz, false);
                ac->calc->YB_NZB.resize_if_needed(Bsiz, Bsiz, false); // never symmetrical!
                ac->calc->NZBXA.resize_if_needed(Bsiz, Asiz, false);
                ac->calc->NZBXAT.resize_if_needed(Asiz, Bsiz, false);
                ac->calc->JAUA.resize_if_needed(Asiz);
                ac->calc->JBUB.resize_if_needed(Bsiz);
                ac->calc->NZBJB.resize_if_needed(Bsiz);
            }
        } // else: empty node, belsongs to a disabled component, nothing to do
    }
    //***********************************************************************
    void clearDC() { dcDouble.reset(); dcWide.reset(); initializedDC = false; }
    void clearAC() { ac.reset(); initializedAC = false; }
    template<class Real> void forwsubsDCTyped(ComponentSubCircuit* pSrc);
    void forwsubsDC(ComponentSubCircuit* pSrc);
    void forwsubsAC(ComponentSubCircuit* pSrc);
    template<class Real> void backsubsDCTyped(ComponentSubCircuit* pSrc);
    void backsubsDC(ComponentSubCircuit* pSrc);
    void backsubsAC(ComponentSubCircuit* pSrc);
    //***********************************************************************
    // Levél cellánál normál esetben YRED és JRED a kapcsolódó komponens megfelelõ értékeinek másolata.
    // Ha viszont van összekötött kapocs, akkor redukció kell. Ebben az esetben CNodeIndex az eredeti ANodeIndex, BNodeIndex a leképezés
    // Ekkor YA-ba és JAUA-ba kerül az eredeti beolvasott érték (ezzel van az összehasonlítás is a redukció szükségessége miatt)
    // YRED-be és JRED-be a redukált kerül, de ekkor a redukció csak a megfelelõ elemek összeadását jelenti, mert csomópont nem esik ki.
    // 
    // Ha vannak a levélen olyan node-ok, amelyek nem kapcsolódnak másik cellához (alapból nem kapcsolódó kapocs, vagy összekapcsolt kapcsok, amik mással nem 
    // kapcsolódnak, akkor az elsõ cellapár összevonásakor ejtjük ki ezeket, tehát nem a levél redukciójakor.
    // 
    // Az external node-ra kapcsolódó node-okat sosem redukáljuk !!!!!
    // 
    // A node-ok legyenek rendezve index szerint
};


//***********************************************************************
class hmgSunred {
//***********************************************************************
public:
    struct ReductionTreeInstructions {
        std::vector<std::vector<ReductionInstruction>> data; // data[0] = Level 1, data[1] = Level 2, etc. level[0][i] = pSrc->components[i]
    };
private:
    std::vector<std::vector<SunredTreeNode>> levels;
    //std::vector<std::vector<uns>> nodeConnectingComponents; // nodeConnectingComponents[i][j] => i: node, j: component
    ComponentSubCircuit* pSrc = nullptr;
public:
    //***********************************************************************
    void buildTree(const ReductionTreeInstructions& instr, ComponentSubCircuit* pSrc_);
    //***********************************************************************
    void allocDC() {
    //***********************************************************************
        for (auto& level : levels)
            for (auto& node : level)
                node.allocDC();
    }
    //***********************************************************************
    void clearDC() {
    //***********************************************************************
        for (auto& level : levels)
            for (auto& node : level)
                node.clearDC();
    }
    //***********************************************************************
    void allocAC() {
    //***********************************************************************
        for (auto& level : levels)
            for (auto& node : level)
                node.allocAC();
    }
    //***********************************************************************
    void clearAC() {
    //***********************************************************************
        for (auto& level : levels)
            for (auto& node : level)
                node.clearAC();
    }
    //***********************************************************************
    void forwsubsDC() {
    // TO PARALLEL
    //***********************************************************************
        for (auto& level : levels)
            for (auto& node : level)
                node.forwsubsDC(pSrc);
    }
    //***********************************************************************
    void backsubsDC() {
    // TO PARALLEL
    //***********************************************************************
        for (size_t i = levels.size(); i != 0; i--) {
            auto& level = levels[i - 1];
            for (auto& node : level)
                node.backsubsDC(pSrc);
        }
    }
    //***********************************************************************
    void forwsubsAC() {
    // TO PARALLEL
    //***********************************************************************
        for (auto& level : levels)
            for (auto& node : level)
                node.forwsubsAC(pSrc);
    }
    //***********************************************************************
    void backsubsAC() {
    // TO PARALLEL
    //***********************************************************************
        for (size_t i = levels.size(); i != 0; i--) {
            auto& level = levels[i - 1];
            for (auto& node : level)
                node.backsubsAC(pSrc);
        }
    }
    //***********************************************************************
};


}

#endif