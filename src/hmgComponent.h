//***********************************************************************
// HexMG component instance (cell) classes
// Creation date:  2023. 01. 27.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_COMPONENT_INSTANCE_HEADER
#define	HMG_COMPONENT_INSTANCE_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgMatrix.hpp"
#include "hmgComponentModel.h"
#include "hmgSunred.h"
#include "hmgMultigridTypes.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
// Component node order
//***********************************************************************
// External nodes:
// --------------
//      IONodes
//      NormalINodes: Y values can be nonzero
//      ControlInodes: no Y values
//      NormalONodes: >= 2 component nodes connected, must be reduced
//      ForwardedONodes
// Internal nodes:
// --------------
//      NormalInternalNodes: >= 2 component nodes connected, must be reduced
//      ControlInternalNodes
//      InternalVars
//***********************************************************************
// When a component is created, all external nodes must be connected
// because it gets the external nodes from out.
// If nothing uses ONodes, then an internal node must be created in the 
// storage component for each ONode and must be attached to it.
//***********************************************************************


//***********************************************************************
class SimControl {
//***********************************************************************
    inline static cplx complexFrequency = cplx0;    // AC: angular frequency (omega) of the analysis, Timeconst: complex frequency (s) of the analysis
    inline static rvt  freq = rvt0;                 // frequency component of the complexFrequency (Hz)
public:
    //***********************************************************************
    // It is possible to run AC during transient:
    // DC: dt == 0, transient: dt != 0
    // dt == 0 && timeStepStop == 0: initial DC
    // dt == 0 && timeStepStop != 0: finishing DC
    //***********************************************************************
    inline static VariableNodeBase timeStepStart;   // transient: the start time of the step
    inline static VariableNodeBase timeStepStop;    // transient: the end time of the step (timeStepStop = timeStepStart + dt)
    inline static VariableNodeBase dt;              // transient: dt of the step
    inline static VariableNodeBase minIter;         // minimum number of iterations in the current step (e.g. a semiconductor diode is replaced with a resistor for the first 1-2 iterations)
    inline static VariableNodeBase iter;            // which iteration we are at in the current step
    //***********************************************************************
    static void setFinalDC() noexcept { if (timeStepStart.getValueDC() == rvt0)timeStepStart.setValueDC(1e-10); timeStepStop.setValueDC(timeStepStart.getValueDC()); dt.setValueDC(rvt0); }
    static void stepTransient(rvt dt_) noexcept { timeStepStart.setValueDC(timeStepStop.getValueDC()); timeStepStop.setValueDC(timeStepStart.getValueDC() + dt_); dt.setValueDC(dt_); }
    static void setComplexFrequencyForAC(rvt f) noexcept { complexFrequency = { 0, 2 * hmgPi * f }; freq = f; }
    //***********************************************************************
    static void setComplexFrequencyForTimeConst(rvt f, uns stepPerDecade) noexcept {
    //***********************************************************************
        crvt omega = rvt(2 * hmgPi * f);
        crvt angle = rvt(hmgPi + 1.5 * log(10.0) / stepPerDecade); // 3 samples for the full width at half maximum (FWHM)
        complexFrequency = { omega * cos(angle), omega * sin(angle) };
        freq = f;
    }
    //***********************************************************************
    static cplx getComplexFrequency() noexcept { return complexFrequency; }
    static rvt  getFrequency() noexcept { return freq; }
    //***********************************************************************
};


//***********************************************************************
class ComponentAndControllerBase {
 //***********************************************************************
public:
    const ComponentDefinition* def;
    const ComponentAndControllerModelBase* pModel;
    bool isEnabled = true;

    //***********************************************************************
    ComponentAndControllerBase(const ComponentDefinition* def_);
    const ComponentAndControllerModelBase& getModel() const noexcept { return *pModel; }
    //***********************************************************************
};


 //***********************************************************************
class ComponentBase : public ComponentAndControllerBase {
//***********************************************************************
public:
    //***********************************************************************
    struct DefectCollector {
    //***********************************************************************
        rvt sumDefect = 0; // sum of squares!
        rvt maxDefect = 0; // square!
        uns nodeNum = 0;
        DefectCollector()noexcept {}
        DefectCollector(rvt sum, rvt max, uns num)noexcept :sumDefect{ sum }, maxDefect{ max }, nodeNum{ num } {}
        void addCollector(const DefectCollector& toAdd)noexcept { sumDefect += toAdd.sumDefect; if (toAdd.maxDefect > maxDefect)maxDefect = toAdd.maxDefect; nodeNum += toAdd.nodeNum; }
        void addDefectNonSquare(rvt nonSquareDefect)noexcept { rvt sqr = nonSquareDefect * nonSquareDefect; sumDefect += sqr; if (sqr > maxDefect)maxDefect = sqr; nodeNum++; }
    };

    //***********************************************************************
    using ComponentAndControllerBase::ComponentAndControllerBase;
    //***********************************************************************

    //***********************************************************************
    virtual ~ComponentBase() = default;
    virtual const VariableNodeBase& getComponentValue() const noexcept = 0;
    virtual VariableNodeBase* getNode(siz nodeIndex) noexcept = 0;
    virtual VariableNodeBase* getInternalNode(siz nodeIndex) noexcept = 0;
    virtual void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex) noexcept = 0;
    virtual void setParam(siz parIndex, const Param& par) noexcept = 0;
    virtual Param& getParam(siz parIndex) noexcept = 0;
    virtual const ComponentBase* getContainedComponent(uns componentIndex)const noexcept = 0;
    //***********************************************************************
    virtual const VariableNodeBase& getComponentCurrent() const noexcept = 0;
    virtual void buildOrReplace() = 0; // should only be called after the nodes and params have been set!, buildForAC() do this for AC
    //************************** AC / DC functions *******************************
    virtual bool isJacobianMXSymmetrical(bool isDC)const noexcept = 0; // e.g. controlled source is alway asymmetrical, in AC, too
    virtual void resetNodes(bool isDC) noexcept = 0;
    virtual void deleteD(bool isDC) noexcept = 0;
    virtual void deleteF(bool isDC) noexcept = 0;
    virtual void deleteYii(bool isDC) noexcept = 0;
    virtual void loadFtoD(bool isDC) noexcept = 0;
    virtual void calculateCurrent(bool isDC) noexcept = 0;
    virtual void forwsubs(bool isDC) = 0;
    virtual void backsubs(bool isDC) = 0;
    virtual void jacobiIteration(bool isDC) noexcept = 0;
    virtual rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept = 0;
    virtual rvt calculateResidual(bool isDC) const noexcept = 0;
    //************************** DC functions *******************************
    virtual void acceptIterationDC(bool isNoAlpha) noexcept = 0; // Vnode = Vnode + v*alpha; isNoAlpha => Vnode = Vnode + v
    virtual void acceptStepDC() noexcept = 0; // VstepStart = Vnode // no AC version
    virtual void calculateValueDC() noexcept = 0; // no AC version
    virtual void calculateControllersDC(uns step) noexcept {} // no AC version, step 1 and 2: to reduce the number of virtual functions
    virtual DefectCollector collectCurrentDefectDC() const noexcept = 0; // no AC version
    virtual DefectCollector collectVoltageDefectDC() const noexcept = 0; // no AC version
    virtual rvt getJreducedDC(uns y) const noexcept = 0;
    virtual rvt getYDC(uns y, uns x) const noexcept = 0;
    virtual void calculateYiiDC() noexcept = 0;
    //************************** AC functions *******************************
    virtual void acceptIterationAndStepAC() noexcept = 0; // Vnode = Vnode + v
    virtual void buildForAC() = 0; // buildOrReplace() do this for DC
    virtual cplx getJreducedAC(uns y) const noexcept = 0;
    virtual cplx getYAC(uns y, uns x) const noexcept = 0;
    virtual void calculateYiiAC() noexcept = 0;
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    virtual void printNodeValueDC(uns) const noexcept = 0;
    virtual void printNodeErrorDC(uns) const noexcept = 0;
    virtual void printNodeDefectDC(uns) const noexcept = 0;
    virtual void printNodeValueAC(uns) const noexcept = 0;
    virtual void printNodeErrorAC(uns) const noexcept = 0;
    virtual void printNodeDefectAC(uns) const noexcept = 0;
    virtual void testPrint() const noexcept = 0;
#endif
};


//***********************************************************************
class RealComponent : public ComponentBase { // real = not a container
//***********************************************************************
protected:
    VariableNodeBase componentValue;
    VariableNodeBase componentCurrent;
public:
    using ComponentBase::ComponentBase;
    const VariableNodeBase& getComponentValue() const noexcept override { return componentValue; }
    const VariableNodeBase& getComponentCurrent() const noexcept override { return componentCurrent; }
    void buildForAC() override {}
    void buildOrReplace() override {}
#ifdef HMG_DEBUGPRINT
    const ComponentBase* getContainedComponent(uns componentIndex)const noexcept override { return nullptr; };
    void testPrint() const noexcept override {}
#endif
};


//***********************************************************************
class Component_2Node_1Par : public RealComponent {
//***********************************************************************
protected:
    VariableNodeBase*N0 = nullptr, *N1 = nullptr;
    Param param;
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    VariableNodeBase* getNode(siz nodeIndex) noexcept override final {
    //***********************************************************************
        return nodeIndex == 0 ? N0 : N1;
    }
    //***********************************************************************
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override final {
    //***********************************************************************
        pNode->turnIntoNode(defValueIndex, false);
        if (nodeIndex == 0)
            N0 = pNode;
        else
            N1 = pNode;
    }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param = par; }
    Param& getParam(siz parIndex) noexcept override final { return param; }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override final {}
    void deleteD(bool isDC) noexcept override final {}
    void deleteF(bool isDC) noexcept override final {}
    void deleteYii(bool isDC) noexcept override final {}
    void loadFtoD(bool isDC) noexcept override final {}
    void forwsubs(bool isDC) override final {}
    void backsubs(bool isDC) override final {}
    void jacobiIteration(bool isDC) noexcept override final {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    //************************** DC functions *******************************
    DefectCollector collectCurrentDefectDC() const noexcept override final { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    void acceptIterationDC(bool isNoAlpha) noexcept override final {}
    void acceptStepDC() noexcept override {}
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override final {}
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    void printNodeValueDC(uns) const noexcept override final {}
    void printNodeErrorDC(uns) const noexcept override final {}
    void printNodeDefectDC(uns) const noexcept override final {}
    void printNodeValueAC(uns) const noexcept override final {}
    void printNodeErrorAC(uns) const noexcept override final {}
    void printNodeDefectAC(uns) const noexcept override final {}
#endif
};


//***********************************************************************
class ComponentConstR_1 final : public Component_2Node_1Par {
//***********************************************************************
    //***********************************************************************
    // value:  // G! (not R)
    //***********************************************************************
public:
    //***********************************************************************
    using Component_2Node_1Par::Component_2Node_1Par;
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if(isDC) {
            rvt I = componentValue.getValueDC() * (N0->getValueDC() - N1->getValueDC());
            componentCurrent.setValueDC(I);
            N0->incDDC(-I);
            N1->incDDC(I);
        }
        else {
            cplx I = componentValue.getValueDC() * (N0->getValueAC() - N1->getValueAC()); // componentValue.getValueDC() !
            N0->incDAC(-I);
            N1->incDAC(I);
        }
    }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override { componentValue.setValueDC(param.get()); }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    rvt getYDC(uns y, uns x) const noexcept override { return y == x ? componentValue.getValueDC() : -componentValue.getValueDC(); }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        if (N0 == N1)
            return;
        crvt G = componentValue.getValueDC();
        N0->incYiiDC(G);
        N1->incYiiDC(G);
    }
    //************************** AC functions *******************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override { return y == x ? componentValue.getValueDC() : -componentValue.getValueDC(); } // componentValue.getValueDC() !
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        if (N0 == N1)
            return;
        ccplx G = componentValue.getValueDC(); // componentValue.getValueDC() !
        N0->incYiiAC(G);
        N1->incYiiAC(G);
    }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstC_1 final : public Component_2Node_1Par {
//***********************************************************************
    rvt Gc = 0, I_stepStart = 0;
public:
    //***********************************************************************
    inline static bool isTrapezoid = false;
    //***********************************************************************
    using Component_2Node_1Par::Component_2Node_1Par;
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt V_start = N0->getStepStartDC() - N1->getStepStartDC();
            crvt V = N0->getValueDC() - N1->getValueDC();
            crvt dV = V - V_start;
            crvt I = isTrapezoid ? (dV * Gc - (SimControl::dt.getValueDC() == 0 ? rvt0 : I_stepStart)) : (dV * Gc);
            componentCurrent.setValueDC(I);
            N0->incDDC(-I);
            N1->incDDC(I);
        }
        else {
            cplx I = componentValue.getValueDC() * SimControl::getComplexFrequency() * (N0->getValueAC() - N1->getValueAC()); // componentValue.getValueDC() !
            N0->incDAC(-I);
            N1->incDAC(I);
        }
    }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(param.get());
        crvt dt = SimControl::dt.getValueDC();
        Gc = (dt == 0) ? rvt0 : (componentValue.getValueDC() / dt);
        Gc = isTrapezoid ? 2 * Gc : Gc;
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    rvt getYDC(uns y, uns x) const noexcept override { return y == x ? Gc : -Gc; }
    void acceptStepDC() noexcept override { I_stepStart = componentCurrent.getValueDC(); }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        if (N0 == N1)
            return;
        N0->incYiiDC(Gc);
        N1->incYiiDC(Gc);
    }
    //************************** AC functions *******************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    //***********************************************************************
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        return y == x  // componentValue.getValueDC() !
            ?  componentValue.getValueDC() * SimControl::getComplexFrequency()
            : -componentValue.getValueDC() * SimControl::getComplexFrequency();
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        if (N0 == N1)
            return;
        ccplx G = componentValue.getValueDC() * SimControl::getComplexFrequency();
        N0->incYiiAC(G);
        N1->incYiiAC(G);
    }
    //***********************************************************************
};


//***********************************************************************
class Component_2Node_4Par : public RealComponent {
//***********************************************************************
protected:
    VariableNodeBase *N0 = nullptr, *N1 = nullptr;
    Param param[4];
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    VariableNodeBase* getNode(siz nodeIndex) noexcept override final {
    //***********************************************************************
        return nodeIndex == 0 ? N0 : N1;
    }
    //***********************************************************************
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override final {
    //***********************************************************************
        pNode->turnIntoNode(defValueIndex, false);
        if (nodeIndex == 0)
            N0 = pNode;
        else
            N1 = pNode;
    }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept override final { return param[parIndex]; }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override final {}
    void deleteD(bool isDC) noexcept override final {}
    void deleteF(bool isDC) noexcept override final {}
    void deleteYii(bool isDC) noexcept override final {}
    void loadFtoD(bool isDC) noexcept override final {}
    void forwsubs(bool isDC) override final {}
    void backsubs(bool isDC) override final {}
    void jacobiIteration(bool isDC) noexcept override final {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    //************************** DC functions *******************************
    DefectCollector collectCurrentDefectDC() const noexcept override final { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override final { return DefectCollector{}; }
    void acceptIterationDC(bool isNoAlpha) noexcept override final {}
    void acceptStepDC() noexcept override final {}
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override final {}
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    void printNodeValueDC(uns) const noexcept override final {}
    void printNodeErrorDC(uns) const noexcept override final {}
    void printNodeDefectDC(uns) const noexcept override final {}
    void printNodeValueAC(uns) const noexcept override final {}
    void printNodeErrorAC(uns) const noexcept override final {}
    void printNodeDefectAC(uns) const noexcept override final {}
#endif
};


//***********************************************************************
class ComponentConstI_1 final : public Component_2Node_4Par {
//***********************************************************************
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC amplitude, par4: AC phase [rad]
    //***********************************************************************
    using Component_2Node_4Par::Component_2Node_4Par;
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            rvt I = componentValue.getValueDC();
            componentCurrent.setValueDC(-I);
            N0->incDDC(I);
            N1->incDDC(-I);
        }
        else {
            crvt A = param[2].get();
            crvt Phi = param[3].get();
            cplx I = { A * cos(Phi), A * sin(Phi) };
            N0->incDAC(I);
            N1->incDAC(-I);
        }
    }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(SimControl::timeStepStop.getValueDC() == 0 
            ? param[0].get() 
            : param[1].get());
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    rvt getYDC(uns y, uns x) const noexcept override { return rvt0; }
    void calculateYiiDC() noexcept override {}
    //************************** AC functions *******************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override { return cplx0; }
    void calculateYiiAC() noexcept override {}
    //***********************************************************************
};


//***********************************************************************
class Component_4Node_4Par : public RealComponent {
//***********************************************************************
protected:
    VariableNodeBase* N[4] = { nullptr, nullptr, nullptr, nullptr };
    Param param[4];
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    VariableNodeBase* getNode(siz nodeIndex) noexcept override final {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept override final { return param[parIndex]; }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override final {}
    void deleteD(bool isDC) noexcept override final {}
    void deleteF(bool isDC) noexcept override final {}
    void deleteYii(bool isDC) noexcept override final {}
    void loadFtoD(bool isDC) noexcept override final {}
    void forwsubs(bool isDC) override final {}
    void backsubs(bool isDC) override final {}
    void jacobiIteration(bool isDC) noexcept override final {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    //************************** DC functions *******************************
    DefectCollector collectCurrentDefectDC() const noexcept override final { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override final { return DefectCollector{}; }
    void acceptIterationDC(bool isNoAlpha) noexcept override final {}
    void acceptStepDC() noexcept override final {}
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override final {}
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    void printNodeValueDC(uns) const noexcept override final {}
    void printNodeErrorDC(uns) const noexcept override final {}
    void printNodeDefectDC(uns) const noexcept override final {}
    void printNodeValueAC(uns) const noexcept override final {}
    void printNodeErrorAC(uns) const noexcept override final {}
    void printNodeDefectAC(uns) const noexcept override final {}
#endif
};


//***********************************************************************
class ComponentConst_V_Controlled_I_1 final : public Component_4Node_4Par {
//***********************************************************************
public:
    //***********************************************************************
    // par1: S DC value before t=0, par2: S DC value after t=0, par3: S AC value, par4: AC phase [rad]
    // componentValue is S !
    //***********************************************************************
    using Component_4Node_4Par::Component_4Node_4Par;
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override {
    //***********************************************************************
        if(nodeIndex < 2)
            pNode->turnIntoNode(defValueIndex, false);
        N[nodeIndex] = pNode;
    }
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return false; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            rvt I = componentValue.getValueDC() * (N[2]->getValueDC() - N[3]->getValueDC());
            componentCurrent.setValueDC(-I);
            N[0]->incDDC(I);
            N[1]->incDDC(-I);
        }
        else {
            crvt A = param[2].get();
            crvt Phi = param[3].get();
            cplx I = cplx{ A * cos(Phi), A * sin(Phi) }  * (N[2]->getValueAC() - N[3]->getValueAC());
            N[0]->incDAC(I);
            N[1]->incDAC(-I);
        }
    }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(SimControl::timeStepStop.getValueDC() == 0 
            ? param[0].get()
            : param[1].get());
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    //***********************************************************************
    rvt getYDC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (y > 1 || x < 2)
            return rvt0;
        return (x == y + 2) ? componentValue.getValueDC() : -componentValue.getValueDC();
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        if (N[0] == N[1])
            return;
        if (N[2] == N[3])
            return;
        if (N[0] == N[2]) N[0]->incYiiDC( componentValue.getValueDC());
        if (N[0] == N[3]) N[0]->incYiiDC(-componentValue.getValueDC());
        if (N[1] == N[2]) N[1]->incYiiDC(-componentValue.getValueDC());
        if (N[1] == N[3]) N[1]->incYiiDC( componentValue.getValueDC());
    }
    //************************** AC functions *******************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (y > 1 || x < 2)
            return cplx0;
        crvt A = param[2].get();
        crvt Phi = param[3].get();
        ccplx S = cplx{ A * cos(Phi), A * sin(Phi) };
        return (x == y + 2) ? S : -S;
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        if (N[0] == N[1])
            return;
        if (N[2] == N[3])
            return;
        crvt A = param[2].get();
        crvt Phi = param[3].get();
        ccplx S = cplx{ A * cos(Phi), A * sin(Phi) };
        if (N[0] == N[2]) N[0]->incYiiAC( S);
        if (N[0] == N[3]) N[0]->incYiiAC(-S);
        if (N[1] == N[2]) N[1]->incYiiAC(-S);
        if (N[1] == N[3]) N[1]->incYiiAC( S);
    }
};


//***********************************************************************
class ComponentGirator final : public RealComponent {
//***********************************************************************
protected:
    VariableNodeBase* N[4] = { nullptr, nullptr, nullptr, nullptr };
    Param param[2];
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    VariableNodeBase* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept override { return param[parIndex]; }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override {
    //***********************************************************************
        pNode->turnIntoNode(defValueIndex, false);
        N[nodeIndex] = pNode;
    }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override {}
    void deleteD(bool isDC) noexcept override {}
    void deleteF(bool isDC) noexcept override {}
    void deleteYii(bool isDC) noexcept override {}
    void loadFtoD(bool isDC) noexcept override {}
    void forwsubs(bool isDC) override {}
    void backsubs(bool isDC) override {}
    void jacobiIteration(bool isDC) noexcept override {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return false; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt IA = param[0].get() * (N[2]->getValueDC() - N[3]->getValueDC());
            crvt IB = param[1].get() * (N[0]->getValueDC() - N[1]->getValueDC());
            //componentCurrent.setValueDC(rvt0);
            N[0]->incDDC(IA);
            N[1]->incDDC(-IA);
            N[2]->incDDC(IB);
            N[3]->incDDC(-IB);
        }
        else {
            ccplx IA = param[0].get() * (N[2]->getValueAC() - N[3]->getValueAC());
            ccplx IB = param[1].get() * (N[0]->getValueAC() - N[1]->getValueAC());
            N[0]->incDAC(IA);
            N[1]->incDAC(-IA);
            N[2]->incDAC(IB);
            N[3]->incDAC(-IB);
        }
    }
    //************************** DC functions *******************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    void acceptIterationDC(bool isNoAlpha) noexcept override {}
    void acceptStepDC() noexcept override {}
    //***********************************************************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        //componentValue.setValueDC(rvt0); // value of a girator is pointless
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    //***********************************************************************
    rvt getYDC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (y < 2) {
            if (x < 2)  return rvt0;
            else        return y + 2 == x ? -param[0].get() : param[0].get(); // I don't know why -S
        }
        else {
            if (x < 2)  return x + 2 == y ? -param[1].get() : param[1].get();
            else        return rvt0;
        }
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        if (N[0] == N[1])
            return;
        if (N[2] == N[3])
            return;
        crvt s = -(param[0].get() + param[1].get()); // I set -S because in getYDC -S is used but I don't know why
        if (N[0] == N[2]) N[0]->incYiiDC( s);
        if (N[0] == N[3]) N[0]->incYiiDC(-s);
        if (N[1] == N[2]) N[1]->incYiiDC(-s);
        if (N[1] == N[3]) N[1]->incYiiDC( s);
    }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override {}
    //***********************************************************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (y < 2) {
            if (x < 2)  return cplx0;
            else        return y + 2 == x ? -param[0].get() : param[0].get();
        }
        else {
            if (x < 2)  return x + 2 == y ? -param[1].get() : param[1].get();
            else        return cplx0;
        }
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        if (N[0] == N[1])
            return;
        if (N[2] == N[3])
            return;
        ccplx s = -(param[0].get() + param[1].get()); // I set -S because in getYAC -S is used but I don't know why
        if (N[0] == N[2]) N[0]->incYiiAC( s);
        if (N[0] == N[3]) N[0]->incYiiAC(-s);
        if (N[1] == N[2]) N[1]->incYiiAC(-s);
        if (N[1] == N[3]) N[1]->incYiiAC( s);
    }
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    void printNodeValueDC(uns) const noexcept override {}
    void printNodeErrorDC(uns) const noexcept override {}
    void printNodeDefectDC(uns) const noexcept override {}
    void printNodeValueAC(uns) const noexcept override {}
    void printNodeErrorAC(uns) const noexcept override {}
    void printNodeDefectAC(uns) const noexcept override {}
#endif
};


//***********************************************************************
class ComponentConstV final : public RealComponent {
//***********************************************************************
protected:
    VariableNodeBase* N[3] = { nullptr, nullptr, nullptr };
    std::unique_ptr<VariableNodeBase> possibleCurrentNode;
    Param param[5];
    rvt NZBJB_DC = rvt0;
    cplx NZBJB_AC = cplx0;
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC value, par4: AC phase [rad],
    // par5: G value of the voltage source, this means a G = 1/G resistor on the B side of the girator
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    VariableNodeBase* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return nodeIndex == 0 ? N[2] : nullptr; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept override { return param[parIndex]; }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override {
    //***********************************************************************
        if(nodeIndex == 2) { // this is an internal node from outside, only this component changes it
            pNode->turnIntoNode(0, true); // the default value is mandatory 0
            pNode->setIsConcurrentDC(false);
        }
        else
            pNode->turnIntoNode(defValueIndex, false);
        N[nodeIndex] = pNode;
    }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override { if (!possibleCurrentNode) return; if (isDC) possibleCurrentNode->reset(); else possibleCurrentNode->resetAC(); }
    void deleteD(bool isDC) noexcept override { if (!possibleCurrentNode) return; if (isDC) possibleCurrentNode->deleteDDC(); else possibleCurrentNode->deleteDAC(); }
    void deleteF(bool isDC) noexcept override { if (!possibleCurrentNode) return; if (isDC) possibleCurrentNode->deleteFDC(); else possibleCurrentNode->deleteFAC(); }
    void deleteYii(bool isDC) noexcept override { if (!possibleCurrentNode) return; if (isDC) possibleCurrentNode->deleteYiiDC(); else possibleCurrentNode->deleteYiiAC(); }
    void loadFtoD(bool isDC) noexcept override { if (!possibleCurrentNode) return; if (isDC) possibleCurrentNode->loadFtoDDC(); else possibleCurrentNode->loadFtoDAC(); }
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt IB = N[1]->getValueDC() - N[0]->getValueDC(); // S2 = -1
            crvt V = -componentValue.getValueDC();
            crvt G = 1.0 / param[4].get();
            crvt UB = N[2]->getValueDC();
            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDDC(UB);
            crvt JB = -IB + V - G * UB;
            N[2]->setDDC(JB);
        }
        else {
            ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
            crvt A = -param[2].get();
            crvt Phi = param[3].get();
            ccplx V = cplx{ A * cos(Phi), A * sin(Phi) };
            crvt G = 1.0 / param[4].get();
            ccplx UB = N[2]->getValueAC();
            N[0]->incDAC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDAC(UB);
            ccplx JB = -IB + V - G * UB;
            N[2]->setDAC(JB);
        }
    }
    //***********************************************************************
    void forwsubs(bool isDC) override {
    //***********************************************************************
        crvt G = 1.0 / param[4].get();
        crvt NZB = -1.0 / G;
        if (isDC) {
            crvt JB = -N[2]->getDDC();
            NZBJB_DC = NZB * JB;
        }
        else {
            ccplx JB = -N[2]->getDAC();
            NZBJB_AC = NZB * JB;
        }
    }
    //***********************************************************************
    void backsubs(bool isDC) override {
    //***********************************************************************
        if (isDC) {
            crvt UA1 = N[0]->getVDC();
            crvt UA2 = N[1]->getVDC();
            crvt G = 1.0 / param[4].get();
            // XA = [1 -1]
            // NZBXA = [-1/G 1/G]
            // UB = NZBXA*UA+NZBJB
            crvt UB = (UA1 - UA2) / G + NZBJB_DC;
            N[2]->setVDC(UB);
            componentCurrent.setValueDC(N[2]->getValueDC());
        }
        else {
            ccplx UA1 = N[0]->getVAC();
            ccplx UA2 = N[1]->getVAC();
            crvt G = 1.0 / param[4].get();
            // XA = [1 -1]
            // NZBXA = [-1/G 1/G]
            // UB = NZBXA*UA+NZBJB
            ccplx UB = (UA1 - UA2) / G + NZBJB_AC;
            N[2]->setVAC(UB);
        }
    }
    //***********************************************************************
    void jacobiIteration(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            N[2]->setValue0DC(N[2]->getValue0DC() + param[4].get() * N[2]->getDDC()); // + or - ? I'm not sure
        }
        else {
            N[2]->setValue0AC(N[2]->getValue0AC() + param[4].get() * N[2]->getDAC()); // + or - ? I'm not sure
        }
    }
    //***********************************************************************
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final {
    //***********************************************************************
        ComponentConstV* coarseVgen = static_cast<ComponentConstV*>(coarse);
        rvt sumRet = rvt0;
        if (isDC) {
            switch (type) {
                case rprProlongateU: N[2]->setValue0DC(coarseVgen->N[2]->getValue0DC()); break; // uh = uH
                case rprRestrictU:   coarseVgen->N[2]->setValue0DC(N[2]->getValue0DC()); break; // uH = uh
                case rprRestrictFDD: { // fH = R(fh) + dH – R(dh), ret: sum (dHi – R(dh)i)^2
                        rvt diff = coarseVgen->N[2]->getDDC() - N[2]->getDDC();
                        coarseVgen->N[2]->setFDC(N[2]->getFDC() + diff);
                        sumRet += diff * diff;
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: coarseVgen->N[2]->setDNonConcurentDC(coarseVgen->N[2]->getValue0DC() - N[2]->getValue0DC()); break; // dH_NonConcurent = uH – R(uh)
                case rprProlongateDHNCAddToUh:   N[2]->incValue0DC(coarseVgen->N[2]->getDNonConcurentDC()); break; // uh = uh + P(dH_NonConcurent)
            }
        }
        else {
            switch (type) {
                case rprProlongateU: N[2]->setValue0AC(coarseVgen->N[2]->getValue0AC()); break; // uh = uH
                case rprRestrictU:   coarseVgen->N[2]->setValue0AC(N[2]->getValue0AC()); break; // uH = uh
                case rprRestrictFDD: { // fH = R(fh) + dH – R(dh), ret: sum (dHi – R(dh)i)^2
                        cplx diff = coarseVgen->N[2]->getDAC() - N[2]->getDAC();
                        coarseVgen->N[2]->setFAC(N[2]->getFAC() + diff);
                        sumRet = absSquare(diff);
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: coarseVgen->N[2]->setDNonConcurentAC(coarseVgen->N[2]->getValue0AC() - N[2]->getValue0AC()); break; // dH_NonConcurent = uH – R(uh)
                case rprProlongateDHNCAddToUh:   N[2]->incValue0AC(coarseVgen->N[2]->getDNonConcurentAC()); break; // uh = uh + P(dH_NonConcurent)
            }
        }
        return sumRet;
    }
    //***********************************************************************
    rvt calculateResidual(bool isDC) const noexcept override final { return isDC ? square(N[2]->getDDC()) : absSquare(N[2]->getDAC()); }
    //************************** DC functions *******************************
    void acceptIterationDC(bool isNoAlpha) noexcept override { if (isNoAlpha) N[2]->setValueAcceptedNoAlphaDC(); else N[2]->setValueAcceptedDC(); }
    void acceptStepDC() noexcept override { N[2]->setStepStartFromAcceptedDC(); }
    //***********************************************************************
    // If the current node is not connected, buildOrReplace creates an internal node and connect the current node to it.
    void buildOrReplace() override { 
    //***********************************************************************
        if (N[2] == nullptr) { 
            possibleCurrentNode = std::make_unique<VariableNodeBase>(); 
            setNode(2, possibleCurrentNode.get(), 0);
        } 
    }
    //***********************************************************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(SimControl::timeStepStop.getValueDC() == 0 
            ? param[0].get() 
            : param[1].get());
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { 
    //***********************************************************************
        // XB = [-1 1]
        // JRED = XB*NZBJB (and +JA but JA=0)
        return y == 0 ? NZBJB_DC : -NZBJB_DC;
    }
    //***********************************************************************
    rvt getYDC(uns y, uns x) const noexcept override {
    //***********************************************************************
        return y == x ? param[4].get() : -param[4].get();
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    // the internal node is ignored here
    //***********************************************************************
        if (N[0] == N[1])
            return;
        crvt G = param[4].get();
        N[0]->incYiiDC(G);
        N[1]->incYiiDC(G);
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override { N[2]->setValueAcceptedAC(); }
    //***********************************************************************
    cplx getJreducedAC(uns y) const noexcept override {
    //***********************************************************************
        // XB = [-1 1]
        // JRED = XB*NZBJB (and +JA but JA=0)
        return y == 0 ? NZBJB_AC : -NZBJB_AC;
    }
    //***********************************************************************
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        return y == x ? param[4].get() : -param[4].get();
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    // the internal node is ignored here
    //***********************************************************************
        if (N[0] == N[1])
            return;
        ccplx G = param[4].get();
        N[0]->incYiiAC(G);
        N[1]->incYiiAC(G);
    }
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    void printNodeValueDC(uns) const noexcept override {}
    void printNodeErrorDC(uns) const noexcept override {}
    void printNodeDefectDC(uns) const noexcept override {}
    void printNodeValueAC(uns) const noexcept override {}
    void printNodeErrorAC(uns) const noexcept override {}
    void printNodeDefectAC(uns) const noexcept override {}
#endif
};


//***********************************************************************
class Component_Function_Controlled_I_with_const_G final : public RealComponent {
//***********************************************************************
    friend class HmgF_Load_ControlledI_Node_StepStart;
    std::vector<VariableNodeBase*> externalNodes;
    std::vector<Param> pars; // pars[0] is G
    std::vector<rvt> workField;
public:
    //***********************************************************************
    Component_Function_Controlled_I_with_const_G(const ComponentDefinition* def_) :RealComponent{ def_ } {
    // workField size: 
    //***********************************************************************
        is_equal_error<siz>(def->nodesConnectedTo.size(), pModel->getN_ExternalNodes(), "Component_Function_Controlled_I_with_const_G::Component_Function_Controlled_I_with_const_G");
        externalNodes.resize(def->nodesConnectedTo.size());
        pars.resize(def->params.size());
        const Model_Function_Controlled_I_with_const_G* pM = dynamic_cast<const Model_Function_Controlled_I_with_const_G*>(pModel);
        workField.resize(pM->controlFunction->getN_WorkingField() + pM->controlFunction->getN_Param() + 1);
    }
    //***********************************************************************
    VariableNodeBase* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return externalNodes[nodeIndex];
    }
    //***********************************************************************
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { pars[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept override { return pars[parIndex]; }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override {
    //***********************************************************************
        if (nodeIndex < pModel->getN_IO_Nodes() + pModel->getN_Normal_I_Nodes()) // There are 2 IO nodes.
            pNode->turnIntoNode(defValueIndex, false);
        externalNodes[nodeIndex] = pNode;
    }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override {}
    void deleteD(bool isDC) noexcept override {}
    void deleteF(bool isDC) noexcept override {}
    void deleteYii(bool isDC) noexcept override {}
    void loadFtoD(bool isDC) noexcept override {}
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return false; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt G = pars[0].get();
            crvt IG = G * (externalNodes[0]->getValueDC() - externalNodes[1]->getValueDC());
            crvt Ifull = componentValue.getValueDC() - IG;
            externalNodes[0]->incDDC(Ifull);
            externalNodes[1]->incDDC(-Ifull);
        }
        else {
            crvt G = pars[0].get();
            ccplx IG = G * (externalNodes[0]->getValueAC() - externalNodes[1]->getValueAC());
            ccplx Ifull = componentValue.getValueDC() - IG; // getValueDC !
            externalNodes[0]->incDAC(Ifull);
            externalNodes[1]->incDAC(-Ifull);
        }
    }
    //***********************************************************************
    void forwsubs(bool isDC) override {}
    void backsubs(bool isDC) override {}
    void jacobiIteration(bool isDC) noexcept override {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    //************************** DC functions *******************************
    void acceptIterationDC(bool isNoAlpha) noexcept override {}
    void acceptStepDC() noexcept override {}
    void buildOrReplace() override {}
    //***********************************************************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        const Model_Function_Controlled_I_with_const_G& model = static_cast<const Model_Function_Controlled_I_with_const_G&>(*pModel);
        for (uns i = 0; i < model.functionSources.sources.size(); i++) {
            switch (model.functionSources.sources[i].sourceType) {
                case NodeConnectionInstructions::SourceType::sExternalNodeValue:
                    workField[i + 1] = externalNodes[model.functionSources.sources[i].sourceIndex]->getValueDC();
                    break;
                case NodeConnectionInstructions::SourceType::sExternalNodeStepstart:
                    workField[i + 1] = externalNodes[model.functionSources.sources[i].sourceIndex]->getStepStartDC();
                    break;
                case NodeConnectionInstructions::SourceType::sParam:
                    workField[i + 1] = pars[model.functionSources.sources[i].sourceIndex].get();
                    break;
                case NodeConnectionInstructions::SourceType::sReturn:
                    // do nothing
                    break;
            }
        }
        model.controlFunction->evaluate(&model.indexField[0], &workField[0], this);
        componentValue.setValueDC(workField[0]);
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    //***********************************************************************
    rvt getYDC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (y >= 2)
            return rvt0;
        crvt G = x == 0 ? pars[0].get() : x == 1 ? -pars[0].get() : rvt0;
        const Model_Function_Controlled_I_with_const_G& model = static_cast<const Model_Function_Controlled_I_with_const_G&>(*pModel);
        crvt dFv_per_dUi = x < model.nodeToFunctionParam.size() && model.nodeToFunctionParam[x] != unsMax
            ? model.controlFunction->devive(&model.indexField[0], &workField[0], const_cast<Component_Function_Controlled_I_with_const_G*>(this), model.nodeToFunctionParam[x])
            : rvt0;
        return y == 0 ? G - dFv_per_dUi : -G + dFv_per_dUi;
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        if (externalNodes[0] == externalNodes[1])
            return;

        rvt Y = rvt0;
        for (uns i = 0; i < externalNodes.size(); i++)
            if (externalNodes[i] == externalNodes[0])
                Y += getYDC(0, i);
        Y = rvt0;

        externalNodes[0]->incYiiDC(Y);
        for (uns i = 1; i < externalNodes.size(); i++)
            if (externalNodes[i] == externalNodes[1])
                Y += getYDC(0, i);
        externalNodes[1]->incYiiDC(Y);
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override {}
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    //***********************************************************************
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (y >= 2)
            return cplx0;
        crvt G = x == 0 ? pars[0].get() : x == 1 ? -pars[0].get() : rvt0;
        return y == 0 ? G : -G;
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        if (externalNodes[0] == externalNodes[1])
            return;
        ccplx G = pars[0].get();
        externalNodes[0]->incYiiAC(G);
        externalNodes[1]->incYiiAC(G);
    }
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    void printNodeValueDC(uns) const noexcept override {}
    void printNodeErrorDC(uns) const noexcept override {}
    void printNodeDefectDC(uns) const noexcept override {}
    void printNodeValueAC(uns) const noexcept override {}
    void printNodeErrorAC(uns) const noexcept override {}
    void printNodeDefectAC(uns) const noexcept override {}
#endif
};


//***********************************************************************
inline int HmgF_Load_ControlledI_Node_StepStart::evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept {
//***********************************************************************
    workField[index[0]] = static_cast<const Component_Function_Controlled_I_with_const_G*>(owner)->externalNodes[nodeIndex]->getStepStartDC();
    return 0;
}


//***********************************************************************
class Controller final : public ComponentAndControllerBase {
//***********************************************************************
    friend class HmgF_Load_Controller_Node_StepStart;
    friend class HmgF_Load_Controller_mVar_StepStart;
    friend class HmgF_Load_Controller_mVar_Value;
    friend class HmgF_Set_Controller_mVar_Value;
    friend class HmgF_Set_Controller_mVar_ValueFromStepStart;
    std::vector<VariableNodeBase*> externalNodes;
    std::vector<VariableNodeBase> mVars;
    std::vector<Param> pars; // pars[0] is G
    std::vector<rvt> workField;
public:
    //***********************************************************************
    Controller(const ComponentDefinition* def_) :ComponentAndControllerBase{ def_ } {
    // workField size: 
    //***********************************************************************
        is_equal_error<siz>(def->nodesConnectedTo.size(), pModel->getN_ExternalNodes(), "Controller::Controller");
        externalNodes.resize(def->nodesConnectedTo.size());
        pars.resize(def->params.size());
        const ModelController* pM = dynamic_cast<const ModelController*>(pModel);
        workField.resize(pM->controlFunction->getN_WorkingField() + pM->controlFunction->getN_Param() + 1);
        mVars.resize(pM->nMVars);
        for (auto& mVar : mVars)
            mVar.turnIntoStateVariable(0);
    }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode)noexcept { externalNodes[nodeIndex] = pNode; }
    VariableNodeBase* getNode(siz nodeIndex) noexcept { return externalNodes[nodeIndex]; }
    void setParam(siz parIndex, const Param& par)noexcept { pars[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept { return pars[parIndex]; }
    //***********************************************************************
    void loadNodesAndParamsToFunction() noexcept {
    //***********************************************************************
        const ModelController& model = static_cast<const ModelController&>(*pModel);
        for (uns i = 0; i < model.functionSources.sources.size(); i++) {
            switch (model.functionSources.sources[i].sourceType) {
                case NodeConnectionInstructions::SourceType::sExternalNodeValue:
                    workField[i + 1] = externalNodes[model.functionSources.sources[i].sourceIndex]->getValueDC();
                    break;
                case NodeConnectionInstructions::SourceType::sExternalNodeStepstart:
                    workField[i + 1] = externalNodes[model.functionSources.sources[i].sourceIndex]->getStepStartDC();
                    break;
                case NodeConnectionInstructions::SourceType::sParam:
                    workField[i + 1] = pars[model.functionSources.sources[i].sourceIndex].get();
                    break;
                case NodeConnectionInstructions::SourceType::sReturn:
                    // do nothing
                    break;
                // mVars are not copied, instead HmgF_Load_Controller_mVar_Value / HmgF_Set_Controller_mVar_Value functions are used
            }
        }
    }
    //***********************************************************************
    void evaluate_and_storeNodes() noexcept {
    //***********************************************************************
        const ModelController& model = static_cast<const ModelController&>(*pModel);
        model.controlFunction->evaluate(&model.indexField[0], &workField[0], this);
        for (uns i = 0; i < model.functionSources.destinations.size(); i++) {
            const NodeConnectionInstructions::Destination& dest = model.functionSources.destinations[i];
            externalNodes[dest.destNodeIndex]->setValueDC(workField[dest.srcParamIndex]); // srcParamIndex == 0 => return, >0 => par
        }
    }
    //***********************************************************************
    void resetMVars() noexcept {
    // !!! External nodes of disabled controllers will be reseted!
    // !!! This cannot be avoided because at the place of the creation 
    //     it is impossible to see wether a node will be used in a disabled
    //     component. 
    //***********************************************************************
        for (auto& var : mVars)
            var.reset();
    }
    //***********************************************************************
    void setStepStartFromValue() noexcept {
    //***********************************************************************
        for (uns i = 0; i < mVars.size(); i++)
            mVars[i].setStepStartFromAcceptedDC();
    }
};


//***********************************************************************
inline int HmgF_Load_Controller_Node_StepStart::evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept {
//***********************************************************************
    workField[index[0]] = static_cast<const Controller*>(owner)->externalNodes[nodeIndex]->getStepStartDC();
    return 0;
}


//***********************************************************************
inline int HmgF_Load_Controller_mVar_StepStart::evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept {
//***********************************************************************
    workField[index[0]] = static_cast<const Controller*>(owner)->mVars[varIndex].getStepStartDC();
    return 0;
}


//***********************************************************************
inline int HmgF_Load_Controller_mVar_Value::evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept {
//***********************************************************************
    workField[index[0]] = static_cast<const Controller*>(owner)->mVars[varIndex].getValueDC();
    return 0;
}


//***********************************************************************
inline int HmgF_Set_Controller_mVar_Value::evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept {
//***********************************************************************
    static_cast<Controller*>(owner)->mVars[varIndex].setValueDC(workField[index[0]]);
    return 0;
}


//***********************************************************************
inline int HmgF_Set_Controller_mVar_ValueFromStepStart::evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept {
//***********************************************************************
    static_cast<Controller*>(owner)->mVars[varIndex].setValueFromStepStartDC();
    return 0;
}



//***********************************************************************
class SubCircuitFullMatrixReductorDC;
class SubCircuitFullMatrixReductorAC;
struct CellReductionDescription;
//***********************************************************************


//***********************************************************************
class ComponentSubCircuit final : public ComponentBase {
//***********************************************************************
    //***********************************************************************
    friend class SubCircuitFullMatrixReductorDC;
    friend class SubCircuitFullMatrixReductorAC;
    friend class hmgSunred;
    friend class SunredTreeNode;
    //***********************************************************************
    std::vector<std::unique_ptr<ComponentBase>> components;
    std::vector<std::unique_ptr<Controller>> controllers;
    std::vector<VariableNodeBase*> externalNodes;
    std::vector<VariableNodeBase> internalNodesAndVars;
    std::vector<Param> pars;
    std::unique_ptr<SubCircuitFullMatrixReductorDC> sfmrDC;
    std::unique_ptr<SubCircuitFullMatrixReductorAC> sfmrAC;
    uns version = 0; // buildOrReplace must be run if this->version != model->version
    bool isJacobianMXSymmetricalDC_ = false;
    bool isJacobianMXSymmetricalAC_ = false;
    bool isContainedComponentWithInternalNode = false;
    //*******     SUNRED     ************************************************
    hmgSunred sunred;
    std::vector<std::vector<uns>> externalNodesToComponents; // IONodes + NormalINodes: which components are connected to this node
    std::vector<std::vector<uns>> internalNodesToComponents; // NormalInternalNodes: which components are connected to this node
    //***********************************************************************
    void forwsubsDC();
    void backsubsDC();
    void forwsubsAC();
    void backsubsAC();
    //***********************************************************************
public:
    //***********************************************************************
    ComponentSubCircuit(const ComponentDefinition* def_) :ComponentBase{ def_ } {
    //***********************************************************************
        is_equal_error<siz>(def->nodesConnectedTo.size(), pModel->getN_ExternalNodes(), "ComponentSubCircuit::ComponentSubCircuit");
        externalNodes.resize(def->nodesConnectedTo.size());
        pars.resize(def->params.size());
    }
    //***********************************************************************
    const VariableNodeBase& getComponentValue() const noexcept override { return getNContainedComponents() == 0 ? FixVoltages::V[0]->fixNode : getContainedComponent(0)->getComponentValue(); }
    const VariableNodeBase& getComponentCurrent() const noexcept override { return getNContainedComponents() == 0 ? FixVoltages::V[0]->fixNode : getContainedComponent(0)->getComponentCurrent(); }
    VariableNodeBase* getNode(siz nodeIndex) noexcept override { return externalNodes[nodeIndex]; }
    VariableNodeBase* getInternalNode(siz nodeIndex) noexcept override final { return &internalNodesAndVars[nodeIndex]; }
    //***********************************************************************
    void setNode(siz nodeIndex, VariableNodeBase* pNode, uns defValueIndex)noexcept override {
    //***********************************************************************
        externalNodes[nodeIndex] = pNode;
    }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { pars[parIndex] = par; }
    Param& getParam(siz parIndex) noexcept override { return pars[parIndex]; }
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override {
    //***********************************************************************
        return isDC ? isJacobianMXSymmetricalDC_ : isJacobianMXSymmetricalAC_;
    }
    //***********************************************************************
    uns getNContainedComponents() const noexcept { return (uns)components.size(); }
    const ComponentBase* getContainedComponent(uns i) const noexcept override { return components[i].get(); }
    void buildOrReplace()override;
    //***********************************************************************
    void setNodesToComponents() {
    // how many components are connecting to a node (multiple connection from the same component is counted as 1)
    //***********************************************************************
        externalNodesToComponents.clear();
        externalNodesToComponents.resize(pModel->getN_ExternalNodes()); // NormalONode is possible
        internalNodesToComponents.clear();
        internalNodesToComponents.resize(static_cast<const ModelSubCircuit*>(pModel)->getN_NormalInternalNodes());
        for (uns i = 0; i < components.size(); i++) {
            const auto& comp = *components[i];
            if (comp.isEnabled) {
                for (const auto& node : comp.def->nodesConnectedTo) {
                    if (node.type == ComponentDefinition::CDNodeType::internal) {
                        if (node.index < internalNodesToComponents.size()) {
                            bool isNotIn = true;
                            auto& ntc = internalNodesToComponents[node.index];
                            for (uns j = 0; j < ntc.size() && isNotIn; j++)
                                if (ntc[j] == i)
                                    isNotIn = false;
                            if (isNotIn)
                                ntc.push_back(i);
                        }
                    }
                    else if (node.type == ComponentDefinition::CDNodeType::external) {
                        if (node.index < externalNodesToComponents.size()) {
                            bool isNotIn = true;
                            auto& ntc = externalNodesToComponents[node.index];
                            for (uns j = 0; j < ntc.size() && isNotIn; j++)
                                if (ntc[j] == i)
                                    isNotIn = false;
                            if (isNotIn)
                                ntc.push_back(i);
                        }
                    }
                    // ground and unconnected nodes ignored
                }
            }
        }
    }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override {
    // TO PARALLEL
    // !!! External nodes of disabled components will be reseted! 
    // !!! This cannot be avoided because at the place of the creation 
    //     it is impossible to see wether a node will be used in a disabled
    //     component. 
    //***********************************************************************
        if (isDC) {
            for (auto& node : internalNodesAndVars)
                node.reset();
            for (auto& comp : components)
                comp->resetNodes(true);
            for (auto& ctrl : controllers)
                ctrl.get()->resetMVars();
        }
        else {
            for (auto& node : internalNodesAndVars)
                node.resetAC();
            for (auto& comp : components)
                comp->resetNodes(false);
        }
    }
    //***********************************************************************
    void deleteD(bool isDC) noexcept override { 
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (auto& node : internalNodesAndVars)
                node.deleteDDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteD(true);
        }
        else {
            for (auto& node : internalNodesAndVars)
                node.deleteDAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteD(false);
        }
    }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (auto& comp : components)
                if (comp->isEnabled) comp->calculateCurrent(true);
            // setting own current should be here
        }
        else {
            for (auto& comp : components)
                if (comp->isEnabled) comp->calculateCurrent(false);
            // setting own current should be here
        }
    }
    //***********************************************************************
    void calculateControllersDC(uns step) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        for (auto& comp : components)
            if (comp->isEnabled) comp->calculateControllersDC(step);

        if (step == 1) {
            for (auto& ctrl : controllers)
                if (ctrl.get()->isEnabled) ctrl.get()->loadNodesAndParamsToFunction();
        }
        else { // 2
            for (auto& ctrl : controllers)
                if (ctrl.get()->isEnabled) ctrl.get()->evaluate_and_storeNodes();
        }
    }
    //***********************************************************************
    void forwsubs(bool isDC)override { if (isDC) forwsubsDC(); else forwsubsAC(); }
    void backsubs(bool isDC)override { if (isDC) backsubsDC(); else backsubsAC(); }
    //***********************************************************************
    void jacobiIteration(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        GaussSeidelRed(isDC);
        GaussSeidelBlack(isDC);
    }
    //***********************************************************************
    void GaussSeidelRed(bool isDC) {
    // TO PARALLEL
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        
        for (auto& comp : components)
            if (comp->isEnabled) comp->jacobiIteration(isDC);
    }
    //***********************************************************************
    void GaussSeidelBlack(bool isDC) {
    // TO PARALLEL
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        
        if (isDC) {
            for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) {
                crvt y = internalNodesAndVars[i].getYiiDC();
                crvt d = internalNodesAndVars[i].getDDC();
                if (y != rvt0)
                    internalNodesAndVars[i].incValue0DC(d / y);
            }
        }
        else {
            for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) {
                ccplx y = internalNodesAndVars[i].getYiiAC();
                ccplx d = internalNodesAndVars[i].getDAC();
                if (y != cplx0)
                    internalNodesAndVars[i].incValue0AC(d / y);
            }
        }
    }
    //***********************************************************************
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final {
    // coarse is the same type as this
    //***********************************************************************
        ComponentSubCircuit* coarseSubckt = static_cast<ComponentSubCircuit*>(coarse);
        cuns NInternal = pModel->getN_InternalNodes();
        cuns NNormalInternal = pModel->getN_NormalInternalNodes();
        rvt sumRet = rvt0;
        if (isDC) {
            switch (type) {
                case rprProlongateU: // uh = uH
                    for (uns i = 0; i < NInternal; i++)
                        internalNodesAndVars[i].setValue0DC(coarseSubckt->internalNodesAndVars[i].getValue0DC());
                    break;
                case rprRestrictU: // uH = uh
                    for (uns i = 0; i < NInternal; i++)
                        coarseSubckt->internalNodesAndVars[i].setValue0DC(internalNodesAndVars[i].getValue0DC());
                    break;
                case rprRestrictFDD: // fH = R(fh) + dH – R(dh), ret: sum (dHi – R(dh)i)^2
                    for (uns i = 0; i < NInternal; i++) {
                        rvt diff = coarseSubckt->internalNodesAndVars[i].getDDC() - internalNodesAndVars[i].getDDC();
                        coarseSubckt->internalNodesAndVars[i].setFDC(internalNodesAndVars[i].getFDC() + diff);
                        sumRet += diff * diff;
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: // dH_NonConcurent = uH – R(uh)
                    for (uns i = 0; i < NInternal; i++)
                        coarseSubckt->internalNodesAndVars[i].setDNonConcurentDC(coarseSubckt->internalNodesAndVars[i].getValue0DC() - internalNodesAndVars[i].getValue0DC());
                    break;
                case rprProlongateDHNCAddToUh: // uh = uh + P(dH_NonConcurent)
                    for (uns i = 0; i < NInternal; i++)
                        internalNodesAndVars[i].setValue0DC(internalNodesAndVars[i].getValue0DC() + coarseSubckt->internalNodesAndVars[i].getDNonConcurentDC());
                    break;
            }
        }
        else {
            switch (type) {
                case rprProlongateU: // uh = uH
                    for (uns i = 0; i < NInternal; i++)
                        internalNodesAndVars[i].setValue0AC(coarseSubckt->internalNodesAndVars[i].getValue0AC());
                    break;
                case rprRestrictU: // uH = uh
                    for (uns i = 0; i < NInternal; i++)
                        coarseSubckt->internalNodesAndVars[i].setValue0AC(internalNodesAndVars[i].getValue0AC());
                    break;
                case rprRestrictFDD: // fH = R(fh) + dH – R(dh), ret: sum (dHi – R(dh)i)^2
                    for (uns i = 0; i < NInternal; i++) {
                        cplx diff = coarseSubckt->internalNodesAndVars[i].getDAC() - internalNodesAndVars[i].getDAC();
                        coarseSubckt->internalNodesAndVars[i].setFAC(internalNodesAndVars[i].getFAC() + diff);
                        sumRet += absSquare(diff);
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: // dH_NonConcurent = uH – R(uh)
                    for (uns i = 0; i < NInternal; i++)
                        coarseSubckt->internalNodesAndVars[i].setDNonConcurentAC(coarseSubckt->internalNodesAndVars[i].getValue0AC() - internalNodesAndVars[i].getValue0AC());
                    break;
                case rprProlongateDHNCAddToUh: // uh = uh + P(dH_NonConcurent)
                    for (uns i = 0; i < NInternal; i++)
                        internalNodesAndVars[i].setValue0AC(internalNodesAndVars[i].getValue0AC() + coarseSubckt->internalNodesAndVars[i].getDNonConcurentAC());
                    break;
            }
        }
        if (isContainedComponentWithInternalNode)
            for (uns i = 0; i < components.size(); i++)
                if (components[i]->isEnabled)
                    sumRet += components[i]->recursiveProlongRestrictCopy(isDC, type, coarseSubckt->components[i].get());
        return sumRet;
    }
    //***********************************************************************
    rvt calculateResidual(bool isDC) const noexcept override final { 
    //***********************************************************************
        rvt residual = rvt0;
        cuns NInodes = pModel->getN_NormalInternalNodes();

        if (isDC) {
            for (uns i = 0; i < NInodes; i++)
                residual += square(internalNodesAndVars[i].getDDC());
        }
        else {
            for (uns i = 0; i < NInodes; i++)
                residual += absSquare(internalNodesAndVars[i].getDAC());
        }

        if (isContainedComponentWithInternalNode)
            for (uns i = 0; i < components.size(); i++)
                if (components[i]->isEnabled)
                    residual += components[i]->calculateResidual(isDC);

        return residual;
    }
    //************************** DC functions *******************************
    void allocForReductionDC();
    //***********************************************************************
    void calculateValueDC() noexcept override {
    // TO PARALLEL
    //***********************************************************************
        for (auto& comp : components)
            if (comp->isEnabled) comp->calculateValueDC();
        // setting own value should be here
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override {
    // TO PARALLEL
    //***********************************************************************
        DefectCollector d;
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) {
            d.addDefectNonSquare(internalNodesAndVars[i].getDDC());
        }
        for (auto& comp : components)
            if (comp->isEnabled) d.addCollector(comp->collectCurrentDefectDC());
        return d;
    }
    //***********************************************************************
    DefectCollector collectVoltageDefectDC() const noexcept override {
    // TO PARALLEL
    //***********************************************************************
        DefectCollector d;
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) {
            d.addDefectNonSquare(internalNodesAndVars[i].getVDC());
        }
        for (auto& comp : components)
            if (comp->isEnabled) d.addCollector(comp->collectVoltageDefectDC());
        return d;
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override;
    rvt getYDC(uns y, uns x) const noexcept override;
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    // TO PARALLEL
    //***********************************************************************
        for (auto& comp : components)
            if (comp->isEnabled) comp->calculateYiiDC();
    }
    //***********************************************************************

    //***********************************************************************
    void acceptIterationDC(bool isNoAlpha) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        if (isNoAlpha)
            for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) // the normalONodes come from outside, from normalInternalNodes
                internalNodesAndVars[i].setValueAcceptedNoAlphaDC();
        else {
            for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) // the normalONodes come from outside, from normalInternalNodes
                internalNodesAndVars[i].setValueAcceptedDC();
        }
        for (auto& comp : components)
            if (comp->isEnabled) comp->acceptIterationDC(isNoAlpha);
    }
    //***********************************************************************
    void acceptStepDC() noexcept override {
    // TO PARALLEL
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < internalNodesAndVars.size(); i++)
            internalNodesAndVars[i].setStepStartFromAcceptedDC();
        for (auto& comp : components)
            if (comp->isEnabled) comp->acceptStepDC();
        for (auto& ctrl : controllers)
            if (ctrl.get()->isEnabled) ctrl.get()->setStepStartFromValue();
    }
    //************************** AC functions *******************************
    void allocForReductionAC();
    //***********************************************************************
    void buildForAC() override {
    //***********************************************************************
        for (auto& comp : components)
            if (comp->isEnabled) comp->buildForAC();
        allocForReductionAC();
    }
    //***********************************************************************
    cplx getJreducedAC(uns y) const noexcept override;
    cplx getYAC(uns y, uns x) const noexcept override;
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    // TO PARALLEL
    //***********************************************************************
        for (auto& comp : components)
            if (comp->isEnabled) comp->calculateYiiAC();
    }
    //***********************************************************************
    void acceptIterationAndStepAC() noexcept override { // sunred and multigrid
    // TO PARALLEL
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_NormalInternalNodes(); i++) // the normalONodes come from outside, from normalInternalNodes
            internalNodesAndVars[i].setValueAcceptedAC();
        for (auto& comp : components)
            if (comp->isEnabled) comp->acceptIterationAndStepAC();
    }
    //************************  Multigrid Functions  ************************
    void deleteF(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (auto& node : internalNodesAndVars)
                node.deleteFDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteF(true);
        }
        else {
            for (auto& node : internalNodesAndVars)
                node.deleteFAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteF(false);
        }
    }
    //***********************************************************************
    void deleteYii(bool isDC) noexcept override { 
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (auto& node : internalNodesAndVars)
                node.deleteYiiDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteYii(true);
        }
        else {
            for (auto& node : internalNodesAndVars)
                node.deleteYiiAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteYii(false);
        }
    }
    //***********************************************************************
    void loadFtoD(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (auto& node : internalNodesAndVars)
                node.loadFtoDDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->loadFtoD(true);
        }
        else {
            for (auto& node : internalNodesAndVars)
                node.loadFtoDAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->loadFtoD(false);
        }
    }
    //***********************  DC Multigrid Functions  **********************
    void solveDC(); // d0 += f0 kell!
    void relaxDC(uns nRelax); // f-et is figyelembe kell venni!
    void prolongateUDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);
    void restrictUDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);
    rvt restrictFDDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);                 // fH = R(fh) + dH – R(dh), ret: truncation error
    void uHMinusRestrictUhToDHNCDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);   // dH_NonConcurent = uH – R(uh)
    void prolongateDHNCAddToUhDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);     // uh = uh + P(dH_NonConcurent)
    //***********************  AC Multigrid Functions  **********************
    void solveAC() {} // d0 += f0 kell!
    void relaxAC(uns nRelax) {} // f-et is figyelembe kell venni!
    void prolongeteUAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}
    void restrictUAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}
    rvt restrictFDDAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) { return rvt0; }  // fH = R(fh) + dH – R(dh), ret: truncation error => saját fv kell a re*re+im*im-hez
    void uHMinusRestrictUhToDHNCAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}   // dH_NonConcurent = uH – R(uh)
    void prolongateDHNCAddToUhAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}     // uh = uh + P(dH_NonConcurent)
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    //***********************************************************************
    void printNodeValueDC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "V (" << n << ")\t[" << i << "] = " << cutToPrint(internalNodesAndVars[i].getValueDC()) << std::endl;
        for (uns i = 0; i < components.size(); i++) {
            if (components[i]->isEnabled) components[i]->printNodeValueDC(i);
        }
    }
    //***********************************************************************
    void printNodeErrorDC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "E (" << n << ")\t[" << i << "] = " << cutToPrint(internalNodesAndVars[i].getVDC()) << std::endl;
        for (uns i = 0; i < components.size(); i++) {
            if (components[i]->isEnabled) components[i]->printNodeErrorDC(i);
        }
    }
    //***********************************************************************
    void printNodeDefectDC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "D (" << n << ")\t[" << i << "] = " << cutToPrint(internalNodesAndVars[i].getDDC()) << std::endl;
        for (uns i = 0; i < components.size(); i++) {
            if (components[i]->isEnabled) components[i]->printNodeDefectDC(i);
        }
    }
    //***********************************************************************
    void printNodeValueAC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "V (" << n << ")\t[" << i << "] = " << cutToPrint(internalNodesAndVars[i].getValueAC()) << std::endl;
        for (uns i = 0; i < components.size(); i++) {
            if (components[i]->isEnabled) components[i]->printNodeValueAC(i);
        }
    }
    //***********************************************************************
    void printNodeErrorAC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "E (" << n << ")\t[" << i << "] = " << cutToPrint(internalNodesAndVars[i].getVAC()) << std::endl;
        for (uns i = 0; i < components.size(); i++) {
            if (components[i]->isEnabled) components[i]->printNodeErrorAC(i);
        }
    }
    //***********************************************************************
    void printNodeDefectAC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "D (" << n << ")\t[" << i << "] = " << cutToPrint(internalNodesAndVars[i].getDAC()) << std::endl;
        for (uns i = 0; i < components.size(); i++) {
            if (components[i]->isEnabled) components[i]->printNodeDefectAC(i);
        }
    }
    //***********************************************************************
    void testPrint() const noexcept override;
    //***********************************************************************
    void printNodesDC() const noexcept {
    //***********************************************************************
        for (uns i = 0; i < internalNodesAndVars.size(); i++) {
            std::cout << "Internal node " << i << std::endl;
            internalNodesAndVars[i].printNode();
        }
        for (uns i = 0; i < components.size(); i++) {
            const ComponentSubCircuit* subckt = dynamic_cast<const ComponentSubCircuit*>(components[i].get());
            if (subckt != nullptr) {
                std::cout << "***** Component " << i << std::endl;
                subckt->printNodesDC();
            }
        }
    }
#endif
};


//***********************************************************************
class SubCircuitFullMatrixReductorDC {
//***********************************************************************
    friend class ComponentSubCircuit;
    //***********************************************************************
    matrix<rvt> YRED; // forwsubs sets
    vektor<rvt> JRED; // forwsubs sets
    matrix<rvt> YAwork, YAcopy, XATwork, XATcopy, XBwork, XBcopy, YBwork, YBcopy; // step 1: fill "work" YA, XA, XB, YB; step2: update all "copy". If no change, no need for matrix reduction. 
    matrix<rvt> NZB, NZBXA, NZBXAT;
    vektor<rvt> JA, JB, NZBJB, UA, UB;
    //***********************************************************************
    ComponentSubCircuit* pSubCircuit;
    //***********************************************************************
public:
    //***********************************************************************
    SubCircuitFullMatrixReductorDC(ComponentSubCircuit* pOwner) :pSubCircuit{ pOwner } {}
    //***********************************************************************
    void alloc() {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pSubCircuit->pModel);
        const bool isSymm = pSubCircuit->isJacobianMXSymmetrical(true);
        cuns Arow = model.getN_IO_Nodes();
        cuns Acol = Arow + (isSymm ? 0 : model.getN_Normal_I_Nodes());
        cuns Browcol = model.getN_NormalInternalNodes() + model.getN_Normal_O_Nodes();
        YRED.resize_if_needed(Arow, Acol, isSymm);
        JRED.resize_if_needed(Arow);
        YAwork.resize_if_needed(Arow, Acol, isSymm);
        YAcopy.resize_if_needed(Arow, Acol, isSymm);
        if (!isSymm) XATwork.resize_if_needed(Acol, Browcol, false);
        if (!isSymm) XATcopy.resize_if_needed(Acol, Browcol, false);
        XBwork.resize_if_needed(Arow, Browcol, false);
        XBcopy.resize_if_needed(Arow, Browcol, false);
        YBwork.resize_if_needed(Browcol, Browcol, isSymm);
        YBcopy.resize_if_needed(Browcol, Browcol, isSymm);
        NZB.resize_if_needed(Browcol, Browcol, false); // never symmetrical!
        NZBXA.resize_if_needed(Browcol, Acol, false);
        NZBXAT.resize_if_needed(Acol, Browcol, false);
        JA.resize_if_needed(Arow);
        JB.resize_if_needed(Browcol);
        NZBJB.resize_if_needed(Browcol);
        UA.resize_if_needed(Acol);
        UB.resize_if_needed(Browcol);
    }
    //***********************************************************************
    void forwsubs();
    void backsubs();
    //***********************************************************************
};


//***********************************************************************
class SubCircuitFullMatrixReductorAC {
//***********************************************************************
    friend class SubCircuitComponent;
    friend class ComponentSubCircuit;
    //***********************************************************************
    matrix<cplx> YRED; // fullmatrix and sunred forwsubs sets, multigrid not allocates
    vektor<cplx> JRED; // forwsubs sets; fullmatrix and sunred sets, multigrid not allocates
    matrix<cplx> YA, XAT, XB;
    matrix<cplx> YB_NZB, NZBXA, NZBXAT;
    vektor<cplx> JA, JB, NZBJB, UA, UB;
    //***********************************************************************
    ComponentSubCircuit* pSubCircuit;
    //***********************************************************************
public:
    //***********************************************************************
    SubCircuitFullMatrixReductorAC(ComponentSubCircuit* pOwner) :pSubCircuit{ pOwner } {}
    //***********************************************************************
    void alloc() {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pSubCircuit->pModel);
        const bool isSymm = pSubCircuit->isJacobianMXSymmetrical(false);
        cuns Arow = model.getN_IO_Nodes();
        cuns Acol = Arow + (isSymm ? 0 : model.getN_Normal_I_Nodes());
        cuns Browcol = model.getN_NormalInternalNodes() + model.getN_Normal_O_Nodes();
        YRED.resize_if_needed(Arow, Acol, isSymm);
        JRED.resize_if_needed(Arow);
        YA.resize_if_needed(Arow, Acol, isSymm);
        if (!isSymm) XAT.resize_if_needed(Acol, Browcol, false);
        XB.resize_if_needed(Arow, Browcol, false);
        YB_NZB.resize_if_needed(Browcol, Browcol, false); // never symmetrical!
        NZBXA.resize_if_needed(Browcol, Acol, false);
        NZBXAT.resize_if_needed(Acol, Browcol, false);
        JA.resize_if_needed(Arow);
        JB.resize_if_needed(Browcol);
        NZBJB.resize_if_needed(Browcol);
        UA.resize_if_needed(Acol);
        UB.resize_if_needed(Browcol);
    }
    //***********************************************************************
    void forwsubs();
    void backsubs();
    //***********************************************************************
};

#ifdef HMG_DEBUGPRINT

//***********************************************************************
inline void ComponentSubCircuit::testPrint() const noexcept {
//***********************************************************************
    //constexpr uns compindex1 = 12;
    //constexpr uns compindex2 = 7;
    constexpr uns compindex1 = 4;
    constexpr uns compindex2 = 3;
    cplx value = static_cast<ComponentSubCircuit*>(components[compindex1].get())->internalNodesAndVars[0].getValueAC();
    std::cout << "\n+++ [ci] AC:" << value << std::endl;
    std::cout << "+++ [ci] TC:" << (1.0 / (2 * hmgPi * SimControl::getFrequency())) << " sec     R: " << (value.imag() * log(10.0) / hmgPi) << std::endl;
    rvt I = static_cast<ComponentSubCircuit*>(components[compindex2].get())->components[3]->getNode(2)->getValueDC();
    rvt T = static_cast<ComponentSubCircuit*>(components[compindex2].get())->internalNodesAndVars[0].getValueDC();
    printf("\n\nI = %.15f\tT = %.15f\n\n", I, T);
/*
    std::cout << "\nYRED:" << std::endl;
    YREDdc.print_z();
    std::cout << "\nYB:" << std::endl;
    sfmrDC->YBcopy.print_z();
    std::cout << "\nNZB:" << std::endl;
    sfmrDC->NZB.print_z();
    if (sfmrDC->YBcopy.get_col() == 2) {
        sfmrDC->YBcopy.get_elem(0, 1) *= -1.0;
        sfmrDC->YBcopy.get_elem(1, 0) *= -1.0;
        sfmrDC->YBwork.math_mul_t_safe(sfmrDC->NZB, sfmrDC->YBcopy);
        std::cout << "\nE:" << std::endl;
        sfmrDC->YBwork.print_z();
    }
    std::cout << "\nYA:" << std::endl;
    sfmrDC->YAcopy.print_z();
    std::cout << "\nXAT:" << std::endl;
    sfmrDC->XATcopy.print_z();
    std::cout << "\nXB:" << std::endl;
    sfmrDC->XBcopy.print_z();
    std::cout << "\nNZBJB:" << std::endl;
    sfmrDC->NZBJB.print_z();
    std::cout << "\nJRED:" << std::endl;
    JREDdc.print_z();
    std::cout << "\nJA:" << std::endl;
    sfmrDC->JA.print_z();
    std::cout << "\nJB:" << std::endl;
    sfmrDC->JB.print_z();
    std::cout << "\nUA:" << std::endl;
    sfmrDC->UA.print_z();
    std::cout << "\nUB:" << std::endl;
    sfmrDC->UB.print_z();
    std::cout << "\nNZBXA:" << std::endl;
    sfmrDC->NZBXA.print_z();
    for (uns i = 0; i < components.size(); i++) {
        if (dynamic_cast<ComponentSubCircuit*>(components[i].get())) std::cout << "\n***************\n" << i << "\n***************" << std::endl;
        if (components[i].get()->isEnabled) components[i].get()->testPrint();
    }
*/
}
#endif

//***********************************************************************
inline rvt ComponentSubCircuit::getJreducedDC(uns y) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        return y < sfmrDC->JRED.size() ? sfmrDC->JRED[y] : rvt0;
    return rvt0;
}
inline rvt ComponentSubCircuit::getYDC(uns y, uns x) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        return y < sfmrDC->YRED.get_row() && x < sfmrDC->YRED.get_col() ? sfmrDC->YRED.get_elem(y, x) : rvt0;
    return rvt0;
}
inline cplx ComponentSubCircuit::getJreducedAC(uns y) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        return y < sfmrAC->JRED.size() ? sfmrAC->JRED[y] : cplx0;
    return cplx0;
}
inline cplx ComponentSubCircuit::getYAC(uns y, uns x) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        return y < sfmrAC->YRED.get_row() && x < sfmrAC->YRED.get_col() ? sfmrAC->YRED.get_elem(y, x) : cplx0;
    return cplx0;
}
//***********************************************************************


//***********************************************************************
inline void ComponentSubCircuit::forwsubsDC() {
// TO PARALLEL
//***********************************************************************
    for (auto& comp : components)
        if (comp->isEnabled) comp->forwsubs(true);
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        sfmrDC->forwsubs();
    else if (model.solutionType == ModelSubCircuit::SolutionType::stSunRed)
        sunred.forwsubsDC();
}


//***********************************************************************
inline void ComponentSubCircuit::backsubsDC() {
// TO PARALLEL
// UA is input in subcircuits, it must be set with setV before calling its backsubs
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        sfmrDC->backsubs();
    else if (model.solutionType == ModelSubCircuit::SolutionType::stSunRed)
        sunred.backsubsDC();
    for (auto& comp : components)
        if (comp->isEnabled) comp->backsubs(true);
}


//***********************************************************************
inline void ComponentSubCircuit::forwsubsAC() {
// TO PARALLEL
//***********************************************************************
    for (auto& comp : components)
        if (comp->isEnabled) comp->forwsubs(false);
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        sfmrAC->forwsubs();
    else if (model.solutionType == ModelSubCircuit::SolutionType::stSunRed)
        sunred.forwsubsAC();
}


//***********************************************************************
inline void ComponentSubCircuit::backsubsAC() {
// TO PARALLEL
// UA is input in subcircuits, it must be set with setV before calling its backsubs
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix)
        sfmrAC->backsubs();
    else if (model.solutionType == ModelSubCircuit::SolutionType::stSunRed)
        sunred.backsubsAC();
    for (auto& comp : components)
        if (comp->isEnabled) comp->backsubs(false);
}



//***********************************************************************
//***********************************************************************
// ****************** !!! ComponentModel functions !!! ******************
//***********************************************************************
//***********************************************************************




//***********************************************************************
inline ComponentAndControllerBase* ModelConstR_1::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentConstR_1(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstC_1::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentConstC_1(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstI_1::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentConstI_1(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConst_V_Controlled_I_1::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentConst_V_Controlled_I_1(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelGirator::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentGirator(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstV::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentConstV(def);
}


//***********************************************************************
inline ComponentAndControllerBase* Model_Function_Controlled_I_with_const_G::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new Component_Function_Controlled_I_with_const_G(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelController::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new Controller(def);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelSubCircuit::makeComponent(const ComponentDefinition* def) const {
//***********************************************************************
    return new ComponentSubCircuit(def);
}


//***********************************************************************
class CircuitStorage {
//***********************************************************************
 
    //***********************************************************************
    CircuitStorage() {
    //***********************************************************************
        HgmFunctionStorage::getInstance(); // HgmFunctionStorage constructor runs
        builtInModels.resize(builtInModelType::bimtSize);
        builtInModels[builtInModelType::bimtConstR_1] = std::make_unique<ModelConstR_1>();
        builtInModels[builtInModelType::bimtConstC_1] = std::make_unique<ModelConstC_1>();
        builtInModels[builtInModelType::bimtConstI_1] = std::make_unique<ModelConstI_1>();
        builtInModels[builtInModelType::bimtConst_V_Controlled_I] = std::make_unique<ModelConst_V_Controlled_I_1>();
        builtInModels[builtInModelType::bimtGirator] = std::make_unique<ModelGirator>();
        builtInModels[builtInModelType::bimtConstV] = std::make_unique<ModelConstV>();
    }

public:
    CircuitStorage(const CircuitStorage&) = delete;
    CircuitStorage& operator=(const CircuitStorage&) = delete;

    //***********************************************************************
    std::vector<std::unique_ptr<VariableNodeBase>> globalVariables;
    std::vector<std::unique_ptr<ComponentAndControllerModelBase>> models; // all models are stored globally, controller models also included
    std::vector<std::unique_ptr<ComponentAndControllerModelBase>> builtInModels;
    //***********************************************************************
    struct FullCircuit { std::unique_ptr<ComponentDefinition> def; std::unique_ptr<ComponentSubCircuit> component; };
    std::vector<FullCircuit> fullCircuitInstances;
    //***********************************************************************

    //***********************************************************************
    static CircuitStorage& getInstance() { // singleton
    //***********************************************************************
        static CircuitStorage instance;
        return instance;
    }

    //***********************************************************************
    void createFullCircuit(uns componentModelIndex, uns nodesDefaultValueIndex) {
    //***********************************************************************
        FullCircuit fc;
        fc.def = std::make_unique<ComponentDefinition>();
        fc.def->componentModelIndex = componentModelIndex;
        fc.def->nodesDefaultValueIndex = nodesDefaultValueIndex;
        fc.component = std::unique_ptr<ComponentSubCircuit>(dynamic_cast<ComponentSubCircuit*>(static_cast<ComponentBase*>(models[componentModelIndex]->makeComponent(fc.def.get()))));
        fullCircuitInstances.push_back(std::move(fc));
        fullCircuitInstances.back().component->buildOrReplace();
    }
};


//***********************************************************************
inline ComponentAndControllerBase::ComponentAndControllerBase(const ComponentDefinition* def_) :def{ def_ },
    pModel{ static_cast<ComponentAndControllerModelBase*>(
        def_->isBuiltIn 
            ? CircuitStorage::getInstance().builtInModels[def_->componentModelIndex].get()
            : CircuitStorage::getInstance().models[def_->componentModelIndex].get()
        ) } {}
//***********************************************************************




}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
