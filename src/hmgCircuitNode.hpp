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
class ConcurrentValue {
//***********************************************************************
    cplx nonConcurrent;
    std::atomic<rvt> concurrentRe;
    std::atomic<rvt> concurrentIm;

    //***********************************************************************
    void fetch_add_core_concurrent(const cplx& value) noexcept {
    //***********************************************************************
        rvt tempRe{ concurrentRe.load(std::memory_order_relaxed) };
        rvt tempIm{ concurrentIm.load(std::memory_order_relaxed) };
        while (!concurrentRe.compare_exchange_strong(tempRe, tempRe + value.real(), std::memory_order_relaxed))
            ;
        while (!concurrentIm.compare_exchange_strong(tempIm, tempIm + value.imag(), std::memory_order_relaxed))
            ;
    }
    //***********************************************************************
    void fetch_add_core_nonconcurrent(const cplx& value) noexcept { nonConcurrent += value; }
    //***********************************************************************

    //***********************************************************************
    void fetch_add_core_concurrent(rvt value) noexcept {
    //***********************************************************************
        rvt temp{ concurrentRe.load(std::memory_order_relaxed) };
        while (!concurrentRe.compare_exchange_strong(temp, temp + value, std::memory_order_relaxed))
            ;
    }
    //***********************************************************************
    void fetch_add_core_nonconcurrent(rvt value) noexcept { nonConcurrent += value; }
    //***********************************************************************

public:
    //***********************************************************************
    explicit ConcurrentValue(cplx def = cplx0) noexcept : concurrentRe{ def.real() }, concurrentIm{ def.imag() }, nonConcurrent{ def } {}
    void store(const cplx& value, bool isConcurrent, bool isGnd)noexcept { if (isGnd) return; if (isConcurrent) { concurrentRe.store(value.real()); concurrentIm.store(value.imag()); } nonConcurrent = value; } // ! because of storeToNonConcurrent, nonConcurrent is always set (deleteD and deleteF)
    void store(rvt value, bool isConcurrent, bool isGnd)noexcept { if (isGnd) return; if (isConcurrent) concurrentRe.store(value); nonConcurrent = value; } // ! because of storeToNonConcurrent, nonConcurrent is always set (deleteD and deleteF)
    void storeToNonConcurrent(const cplx& value)noexcept { nonConcurrent = value; }
    void storeToNonConcurrent(rvt value)noexcept { nonConcurrent = value; }
    cplx loadCplx(bool isConcurrent)const noexcept { return isConcurrent ? cplx{ concurrentRe.load(), concurrentIm.load() } : nonConcurrent; }
    rvt loadRvt(bool isConcurrent)const noexcept { return isConcurrent ? concurrentRe.load() : nonConcurrent.real(); }
    cplx loadNonConcurrentCplx()const noexcept { return nonConcurrent; }
    rvt loadNonConcurrentRvt()const noexcept { return nonConcurrent.real(); }
    //const cplx& operator=(const cplx& value) = delete;
    //rvt operator=(rvt value) noexcept = delete;
    //***********************************************************************

    //***********************************************************************
    void fetch_add(const cplx& value, bool isConcurrent, bool isGnd) noexcept {
    //***********************************************************************
        if (isGnd) // defect of gnd is not needed for the calculations => not calculated
            return;
        if(isConcurrent)
            fetch_add_core_concurrent(value);
        else 
            fetch_add_core_nonconcurrent(value);
    }

    //***********************************************************************
    void fetch_add(rvt value, bool isConcurrent, bool isGnd) noexcept {
    //***********************************************************************
        if (isGnd) // defect of gnd is not needed for the calculations => not calculated
            return;
        if (isConcurrent)
            fetch_add_core_concurrent(value);
        else
            fetch_add_core_nonconcurrent(value);
    }

    //***********************************************************************
    void fetch_sub(const cplx& value, bool isConcurrent, bool isGnd) noexcept { fetch_add(-value, isConcurrent, isGnd); }
    void fetch_sub(rvt value, bool isConcurrent, bool isGnd) noexcept { fetch_add(-value, isConcurrent, isGnd); }
    //***********************************************************************
};


//***********************************************************************
struct NodeVariable {
//***********************************************************************
    //***********************************************************************
    inline static rvt alpha = 1; // multiplicator for getValue
    //***********************************************************************
    // Variables that must retain their value after a DC-AC switch:
    rvt valueDC = rvt0;          // voltage/temperature/... of the node
    rvt stepStartDC = rvt0;      // starting value of a step in the time domain
    cplx valueAC = cplx0;        // voltage/temperature/... of the node
    uns defaultValueIndex = unsMax;   // in Rails::V
    bool isConcurrent = true;
    bool isGnd = false;
    //***********************************************************************
    // Variables used alternately for DC and AC:
    ConcurrentValue d;           // defect (current)
    cplx v = cplx0;              // error (voltage)
    cplx f = cplx0;              // multigrid defect
    ConcurrentValue yii;         // admitance for multigrid
    //***********************************************************************
    // 
    //***********************************************************************
    NodeVariable() = default;
    void reset() noexcept; // DC + AC
    void setIsConcurrent(bool is) noexcept { isConcurrent = is; }
    void setIsGnd(bool is) noexcept { isGnd = is; }
    void setDefaultValueIndex(uns dvi, bool isForced) noexcept { if (isForced || defaultValueIndex == unsMax) defaultValueIndex = dvi; }
    rvt getValueDC()const noexcept {  return valueDC + v.real() * alpha; } // value + v*alpha
    rvt getValue0DC()const noexcept { return valueDC; }
    rvt getStepStartDC()const noexcept { return stepStartDC; }
    rvt getAcceptedValueDC()const noexcept { return valueDC; }
    void setValueAcceptedDC() noexcept { valueDC += v.real() * alpha; v = cplx0; } // accept the result of the current iteration with the current alpha
    void setValueAcceptedNoAlphaDC() noexcept { valueDC += v.real(); v = rvt0; } // accept the result of the current iteration with the current alpha
    void setStepStartFromAcceptedDC() noexcept { stepStartDC = valueDC; }
    void setValueFromStepStartDC()noexcept { valueDC = stepStartDC; } // v is not cleared!
    void setValueFromStepStartAndClearVDC()noexcept { valueDC = stepStartDC; v = rvt0; }
    void setValueDC(rvt val)noexcept { valueDC = val; v = rvt0; }
    void setValue0DC(rvt val)noexcept { valueDC = val; }
    void incValue0DC(rvt val)noexcept { valueDC += val; }
    void setStepStartDC(rvt val)noexcept { stepStartDC = val; }
    void setDDC(rvt d_)noexcept { d.store(d_, isConcurrent, isGnd); }
    void setFDC(rvt f_)noexcept { f = f_; }
    void setYiiDC(rvt y)noexcept { yii.store(y, isConcurrent, isGnd); }
    void setDNonConcurrentDC(rvt d_)noexcept { d.storeToNonConcurrent(d_); }
    void setVDC(rvt v_)noexcept { v = v_; }
    void incDDC(rvt d_)noexcept { d.fetch_add(d_, isConcurrent, isGnd); }
    void incYiiDC(rvt y)noexcept { yii.fetch_add(y, isConcurrent, isGnd); }
    void deleteDDC()noexcept { d.store(rvt0, isConcurrent, isGnd); }
    void deleteYiiDC()noexcept { yii.store(rvt0, isConcurrent, isGnd); }
    void deleteFDC()noexcept { f = rvt0; }
    void loadFtoDDC()noexcept { d.store(f, isConcurrent, isGnd); }
    rvt getDDC()const noexcept { return d.loadRvt(isConcurrent); }
    rvt getDNonConcurrentDC()const noexcept { return d.loadNonConcurrentRvt(); }
    rvt getFDC()const noexcept { return f.real(); }
    rvt getVDC()const noexcept { return v.real(); }
    rvt getYiiDC()const noexcept { return yii.loadRvt(isConcurrent); }
    //************************** AC functions *******************************
    void deleteDAC() { d.store(cplx0, isConcurrent, isGnd); }
    void deleteYiiAC() { yii.store(cplx0, isConcurrent, isGnd); }
    void deleteFAC() { f = cplx0; }
    void loadFtoDAC() { d.store(f, isConcurrent, isGnd); }
    cplx getValueAC()const { return valueAC + v; }
    cplx getDAC()const noexcept { return d.loadCplx(isConcurrent); }
    cplx getFAC()const noexcept { return f; }
    cplx getDNonConcurrentAC()const noexcept { return d.loadNonConcurrentCplx(); }
    cplx getVAC()const noexcept { return v; }
    cplx getYiiAC()const noexcept { return yii.loadCplx(isConcurrent); }
    void incDAC(ccplx& d_)noexcept { d.fetch_add(d_, isConcurrent, isGnd); }
    void incYiiAC(ccplx& y)noexcept { yii.fetch_add(y, isConcurrent, isGnd); }
    void resetAC() noexcept { reset(); }
    void setValue0AC(ccplx& val)noexcept { valueAC = val; }
    void incValue0AC(ccplx& val)noexcept { valueAC += val; }
    cplx getValue0AC()const noexcept { return valueAC; }
    void setValueAcceptedAC()noexcept { valueAC += v; v = cplx0; }
    void setDAC(const cplx& d_)noexcept { d.store(d_, isConcurrent, isGnd); }
    void setFAC(const cplx& f_)noexcept { f = f_; }
    void setDNonConcurrentAC(const cplx& d_)noexcept { d.storeToNonConcurrent(d_); }
    void setVAC(ccplx& v_)noexcept { v = v_; }
    void setYiiAC(const cplx& y)noexcept { yii.store(y, isConcurrent, isGnd); }
    //***********************************************************************
    void printNode()const noexcept {
    //***********************************************************************
        std::cout << "ValueDC = " << valueDC << "     stepStartDC = " << stepStartDC << std::endl;
        std::cout << "v = " << v << "     f = " << f << "     yii = " << yii.loadCplx(isConcurrent) << std::endl;
        std::cout << "d = " << d.loadCplx(isConcurrent) << "     d.nonConcurrent = " << d.loadNonConcurrentCplx() << "\n" << std::endl;
    }
};


//***********************************************************************
struct Param final {
//***********************************************************************
    NodeVariable* var = nullptr;
    rvt value = rvt0;
    rvt get()const noexcept { return var ? var->valueDC : value; }
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
        NodeVariable rail;
        explicit OneRail(uns defValueIndex) noexcept { rail.setDefaultValueIndex(defValueIndex, true); }
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
            v->rail.reset();
            v->rail.setIsGnd(true);
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
inline void NodeVariable::reset() noexcept {
//***********************************************************************
    d.store(rvt0, isConcurrent, isGnd);
    v = cplx0;
    f = cplx0;
    yii.store(cplx0, isConcurrent, isGnd);
    valueAC = cplx0;
    stepStartDC = valueDC = Rails::V[defaultValueIndex]->getDefaultNodeValue();
}


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
