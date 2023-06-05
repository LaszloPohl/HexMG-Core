//***********************************************************************
// HexMG classes for the nodes of the circuits
// Creation date:  2023. 01. 24.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_CIRCUIT_NODE_HEADER
#define	HMG_CIRCUIT_NODE_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgCommon.h"
#include "hmgException.h"
#include <atomic>
#include <memory>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
template<typename T>
class ConcurentValue {
//***********************************************************************
    T nonConcurrent;
    std::atomic<T> concurrent;
    bool isConcurrent = true;
    bool isGnd = false;

    //***********************************************************************
    void fetch_add_core(const T& value) noexcept {
    //***********************************************************************
        if (!isConcurrent) {
            nonConcurrent += value;
            return;
        }
        T temp{ concurrent.load(std::memory_order_relaxed) };
        while (!concurrent.compare_exchange_strong(temp, temp + value, std::memory_order_relaxed))
            ;
    }

public:
    //***********************************************************************
    explicit ConcurentValue(T def = T()) noexcept: concurrent{ def }, nonConcurrent{ def } {}
    void setIsConcurrent(bool is) noexcept { isConcurrent = is; }
    bool getIsConcurrent() const noexcept { return isConcurrent; }
    void setIsGnd(bool is)noexcept { isGnd = is; }
    bool getIsGnd() const noexcept { return isGnd; }
    void store(const T& value)noexcept { if (isGnd) return; if (isConcurrent)concurrent.store(value); nonConcurrent = value; } // ! because of storeToNonConcurent, nonConcurrent is always set (deleteD and deleteF)
    void storeToNonConcurent(const T& value)noexcept { nonConcurrent = value; }
    T load()const noexcept { return isConcurrent ? concurrent.load() : nonConcurrent; }
    T loadNonConcurent()const noexcept { return nonConcurrent; }
    const T& operator=(const T& value) noexcept { if (isGnd) return value; if (isConcurrent)concurrent.store(value); nonConcurrent = value; return value; } // ! because of storeToNonConcurent, nonConcurrent is always set (deleteD and deleteF)
    //***********************************************************************

    //***********************************************************************
    void fetch_add(const T& value) noexcept {
    //***********************************************************************
        if (isGnd) // defect of gnd is not needed for the calculations => not calculated
            return;
        fetch_add_core(value);
    }

    //***********************************************************************
    void fetch_sub(const T& value) noexcept { fetch_add(-value); }
    //***********************************************************************
};


//***********************************************************************
struct CircuitNodeDataAC {
//***********************************************************************
    cplx value = cplx0;             // voltage/temperature/... of the node
    cplx toSave = cplx0;
    ConcurentValue<cplx> d;         // defect (current)
    cplx v = 0;                     // error (voltage)
    //***********************************************************************
    cplx f = cplx0;                 // multigrid defect
    ConcurentValue<cplx> yii;       // admitance for multigrid
    //***********************************************************************
    void reset() noexcept {
    //***********************************************************************
        value = cplx0;
        toSave = cplx0;
        d.store(cplx0);
        v = cplx0;
        f = cplx0;
        yii.store(cplx0);
    }
};


//***********************************************************************
struct NodeData {
//***********************************************************************
    //***********************************************************************
    ConcurentValue<rvt> d;  // defect (current)
    rvt v = rvt0;           // error (voltage)
    rvt toSave = rvt0;      // stored for saving
    std::unique_ptr<CircuitNodeDataAC> acNodePtr;
    uns defaultValueIndex;  // in FixVoltages::V
    //***********************************************************************
    rvt f = rvt0; // multigrid defect
    ConcurentValue<rvt> yii;  // admitance for multigrid
    //***********************************************************************
    inline static rvt alpha = 1; // multiplicator for getValue
    //***********************************************************************
    explicit NodeData(uns defValueIndex) noexcept;
    rvt reset() noexcept;
    //***********************************************************************
};


//***********************************************************************
struct ParVarNodeType {
//***********************************************************************
    // no separate type for DC and AC node because AC analysis is less speed sensitive than time domain
    enum PVNTType { pvntParameter, pvntSimpleVariable, pvntStateVariable, pvntNode };
    PVNTType type;
    ParVarNodeType(PVNTType type_) : type{ type_ } {}
};


//***********************************************************************
struct VariableNodeBase final : public ParVarNodeType {
//***********************************************************************
    //***********************************************************************
    rvt value = rvt0;          // voltage/temperature/... of the node
    rvt stepStart = rvt0;      // starting value of a step in the time domain
    std::unique_ptr<NodeData> nodePtr;
    //***********************************************************************

    //***********************************************************************
    VariableNodeBase() : ParVarNodeType{ pvntSimpleVariable } {}
    bool isSimple()const noexcept { return type == pvntSimpleVariable; }
    bool isNode()const noexcept { return type == pvntNode; }
    bool isState()const noexcept { return type == pvntStateVariable; }
    void turnIntoStateVariable(uns defValueIndex) {  if (isSimple()) type = pvntStateVariable; } // if this is a node, no change!
    //***********************************************************************
    void turnIntoNode(uns defValueIndex, bool isDefValueIndexForced) {
    //***********************************************************************
        if (!isNode()) {
            nodePtr = std::make_unique<NodeData>(defValueIndex);
            type = pvntNode;
        }
        else if (isDefValueIndexForced)
            nodePtr->defaultValueIndex = defValueIndex;
    }
    //***********************************************************************
    void reset() noexcept; // DC + AC
    void setIsConcurrentDC(bool is) noexcept { if (isNode()) { nodePtr->d.setIsConcurrent(is); nodePtr->yii.setIsConcurrent(is); } }
    void setIsGndDC(bool is) noexcept { if (isNode()) { nodePtr->d.setIsGnd(is); nodePtr->yii.setIsGnd(is); } }
    rvt getValueDC()const noexcept {  return isNode() ? (value + nodePtr->v * NodeData::alpha) : value; } // value + v*alpha
    rvt getValue0DC()const noexcept { return value; }
    rvt getStepStartDC()const noexcept { return stepStart; }
    rvt getAcceptedValueDC()const noexcept { return value; }
    //***********************************************************************
    void setValueAcceptedDC() noexcept {
    // accept the result of the current iteration with the current alpha
    //***********************************************************************
        if (isNode()) {
            rvt& v = nodePtr->v;
            value = value + v * NodeData::alpha;
            v = rvt0;
        }
    }
    //***********************************************************************
    void setValueAcceptedNoAlphaDC() noexcept {
    // accept the result of the current iteration with the current alpha
    //***********************************************************************
        if (isNode()) {
            rvt& v = nodePtr->v;
            value = value + v;
            v = rvt0;
        }
    }
    //***********************************************************************
    void setValueAcceptedAndSaveDC() noexcept {
    // accept the result of the current iteration with the current alpha
    //***********************************************************************
        if (isNode()) {
            NodeData& nd = *nodePtr.get();
            value = value + nd.v * NodeData::alpha;
            nd.v = rvt0;
            nd.toSave = value;
        }
    }
    //***********************************************************************
    void setToSaveFromValueDC() noexcept { if (isNode()) nodePtr->toSave = value; } // v is not cleared!
    void setStepStartFromAcceptedDC() noexcept { stepStart = value; }
    void setValueFromStepStartDC()noexcept { if (!isSimple()) value = stepStart; } // v is not cleared!
    //***********************************************************************
    void setValueFromStepStartAndClearVDC()noexcept {
    //***********************************************************************
        if (isNode()) {
            value = stepStart;
            nodePtr->v = rvt0;
        }
        else if (isState())
            value = stepStart;
    }
    //***********************************************************************
    void setValueDC(rvt val)noexcept {
    //***********************************************************************
        value = val;
        if (isNode()) {
            nodePtr->v = rvt0;
        }
    }
    //***********************************************************************
    void setValue0DC(rvt val)noexcept { value = val; }
    void incValue0DC(rvt val)noexcept { value += val; }
    void setDDC(rvt d)noexcept { nodePtr->d.store(d); }
    void setFDC(rvt f)noexcept { nodePtr->f = f; }
    void setYiiDC(rvt y)noexcept { nodePtr->yii.store(y); }
    void setDNonConcurentDC(rvt d)noexcept { nodePtr->d.storeToNonConcurent(d); }
    void setVDC(rvt v)noexcept { nodePtr->v = v; }
    void incDDC(rvt d)noexcept { nodePtr->d.fetch_add(d); }
    void incYiiDC(rvt y)noexcept { nodePtr->yii.fetch_add(y); }
    void deleteDDC()noexcept { if (isNode()) nodePtr->d.store(rvt0); }
    void deleteYiiDC()noexcept { if (isNode()) nodePtr->yii.store(rvt0); }
    void deleteFDC()noexcept { if (isNode()) nodePtr->f = rvt0; }
    void loadFtoDDC()noexcept { if (isNode()) nodePtr->d.store(nodePtr->f); }
    rvt getDDC()const noexcept { return nodePtr->d.load(); }
    rvt getDNonConcurentDC()const noexcept { return nodePtr->d.loadNonConcurent(); }
    rvt getFDC()const noexcept { return nodePtr->f; }
    rvt getVDC()const noexcept { return nodePtr->v; }
    rvt getYiiDC()const noexcept { return nodePtr->yii.load(); }
    //************************** AC functions *******************************
private:
    //***********************************************************************
    void make_AC() {
    // This must be a node, not checking!
    //***********************************************************************
        NodeData& nd = *nodePtr.get();
        if (!nd.acNodePtr) {
            nd.acNodePtr = std::make_unique<CircuitNodeDataAC>();
            nd.acNodePtr->d.setIsConcurrent(nd.d.getIsConcurrent());
            nd.acNodePtr->d.setIsGnd(nd.d.getIsGnd());
            nd.acNodePtr->yii.setIsConcurrent(nd.yii.getIsConcurrent());
            nd.acNodePtr->yii.setIsGnd(nd.yii.getIsGnd());
        }
    }
public:
    //***********************************************************************
    void createAC() { if (isNode()) make_AC(); }
    void deleteDAC() { if (isNode()) { make_AC(); nodePtr->acNodePtr->d.store(cplx0); } }
    void deleteYiiAC() { if (isNode()) { make_AC(); nodePtr->acNodePtr->yii.store(cplx0); } }
    void deleteFAC() { if (isNode()) { make_AC(); nodePtr->acNodePtr->f = cplx0; } }
    void loadFtoDAC() { if (isNode()) { nodePtr->acNodePtr->d.store(nodePtr->acNodePtr->f); } }
    //***********************************************************************
    cplx getValueAC()const {
    //***********************************************************************
        if (!isNode())
            throw hmgExcept("VariableNodeBase::getValueAC", "called for variable or contorl node");
        const NodeData& nd = *nodePtr.get();
        return nd.acNodePtr->value + nd.acNodePtr->v;
    }
    //***********************************************************************
    cplx getDAC()const noexcept { return nodePtr->acNodePtr->d.load(); }
    cplx getFAC()const noexcept { return nodePtr->acNodePtr->f; }
    cplx getDNonConcurentAC()const noexcept { return nodePtr->acNodePtr->d.loadNonConcurent(); }
    cplx getVAC()const noexcept(!hmgVErrorCheck) { return nodePtr->acNodePtr->v; }
    cplx getYiiAC()const noexcept { return nodePtr->acNodePtr->yii.load(); }
    void incDAC(ccplx& d)noexcept { nodePtr->acNodePtr->d.fetch_add(d); }
    void incYiiAC(ccplx& y)noexcept { nodePtr->acNodePtr->yii.fetch_add(y); }
    void resetAC() noexcept { if(isNode()) nodePtr->acNodePtr->reset(); }
    void setValue0AC(ccplx& val)noexcept { nodePtr->acNodePtr->value = val; }
    void incValue0AC(ccplx& val)noexcept { nodePtr->acNodePtr->value += val; }
    cplx getValue0AC()const noexcept { return nodePtr->acNodePtr->value; }
    //***********************************************************************
    void setValueAcceptedAC()noexcept {
    // value += v
    //***********************************************************************
        if (isNode()) {
            CircuitNodeDataAC& nd = *nodePtr->acNodePtr.get();
            nd.value += nd.v;
            nd.v = cplx0;
        }
    }
    //***********************************************************************
    void setDAC(const cplx& d)noexcept { nodePtr->acNodePtr->d.store(d); }
    void setFAC(const cplx& f)noexcept { nodePtr->acNodePtr->f = f; }
    void setDNonConcurentAC(const cplx& d)noexcept { nodePtr->acNodePtr->d.storeToNonConcurent(d); }
    void setVAC(ccplx& v)noexcept { nodePtr->acNodePtr->v = v; }
    void setYiiAC(const cplx& y)noexcept { nodePtr->acNodePtr->yii.store(y); }
    //***********************************************************************
    void printNode()const noexcept {
    //***********************************************************************
        std::cout << "Value = " << value << "     stepStart = " << stepStart << std::endl;
        if (nodePtr) {
            std::cout << "v = " << nodePtr->v << "     toSave = " << nodePtr->toSave << "     f = " << nodePtr->f << "     yii = " << nodePtr->yii.load() << std::endl;
            std::cout << "d = " << nodePtr->d.load() << "     d.nonConcurrent = " << nodePtr->d.loadNonConcurent() << "\n"  << std::endl;
        }
    }
};


//***********************************************************************
struct Param final : public ParVarNodeType {
//***********************************************************************
    VariableNodeBase* var = nullptr;
    rvt value = rvt0;
    rvt get()const noexcept { return var ? var->value : value; }
    Param() : ParVarNodeType{ pvntParameter } {}
};


//***********************************************************************
struct Rails {
//***********************************************************************

    //***********************************************************************
    class OneRail {
    //***********************************************************************
        friend struct Rails;
        rvt defaultNodeValue = rvt0;
    public:
        VariableNodeBase fixNode;
        explicit OneRail(uns defValueIndex) noexcept { fixNode.turnIntoNode(defValueIndex, true); }
        rvt getDefaultNodeValue()const noexcept { return defaultNodeValue; }
    };

    //***********************************************************************
    inline static std::vector<std::unique_ptr<OneRail>> V;
    //***********************************************************************

    //***********************************************************************
    static void resize(uns newSize) {
    //***********************************************************************
        while (V.size() < newSize) {
            V.push_back(std::make_unique<OneRail>(uns(V.size())));
        }
    }

    //***********************************************************************
    static void reset() {
    //***********************************************************************
        for (auto& v : V) {
            v->fixNode.reset();
            v->fixNode.setIsGndDC(true);
            v->fixNode.createAC();
        }
    }

    //***********************************************************************
    static void SetVoltage(uns nodeIndex, rvt value) {
    //***********************************************************************
        if (nodeIndex > 0)
            const_cast<rvt&>(V[nodeIndex]->defaultNodeValue) = value;
    }

};


//***********************************************************************
inline rvt NodeData::reset() noexcept {
//***********************************************************************
    toSave = Rails::V[defaultValueIndex]->getDefaultNodeValue();
    d.store(rvt0);
    v = rvt0;
    if (acNodePtr)
        acNodePtr->reset();
    f = rvt0;
    yii.store(rvt0);
    return toSave;
}


//***********************************************************************
inline void VariableNodeBase::reset() noexcept {
//***********************************************************************
    stepStart = value = isNode() ? nodePtr->reset() : rvt0;
}


//***********************************************************************
inline NodeData::NodeData(uns defValueIndex) noexcept
    : defaultValueIndex{ defValueIndex } {
//***********************************************************************
    if (Rails::V.size() > defValueIndex)
        reset();
}


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
