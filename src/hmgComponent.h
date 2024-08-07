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
#include <thread>
#include "hmgMatrix.hpp"
#include "hmgComponentModel.h"
#include "hmgSunred.h"
#include "hmgMultigridTypes.h"
#include "hmgMultigrid.hpp"
#include "hmgSimulation.h"
#include "hmgSaver.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
// Component node order
//***********************************************************************
// External nodes:
// --------------
//      XNodes
//      YNodes: Y values can be nonzero
//      ANodes: no Y values
//      ONodes: >= 2 component nodes connected, must be reduced
// Internal nodes:
// --------------
//      NNodes: >= 2 component nodes connected, must be reduced
//      BNodes
//      UnconnectedExtNodes
//***********************************************************************


//***********************************************************************
class SimControl {
//***********************************************************************
    inline static cplx complexFrequency = cplx0;    // AC: angular frequency (omega) of the analysis, Timeconst: complex frequency (s) of the analysis
public:
    //***********************************************************************
    // It is possible to run AC during transient:
    // DC: dt == 0, transient: dt != 0
    // dt == 0 && timeStepStop == 0: initial DC
    // dt == 0 && timeStepStop != 0: finishing DC
    //***********************************************************************
    inline static NodeVariable timeStepStart;   // transient: the start time of the step
    inline static NodeVariable timeStepStop;    // transient: the end time of the step (timeStepStop = timeStepStart + dt)
    inline static NodeVariable dt;              // transient: dt of the step
    inline static NodeVariable freq;            // frequency component of the complexFrequency (Hz)
    inline static NodeVariable minIter;         // minimum number of iterations in the current step (e.g. a semiconductor diode is replaced with a resistor for the first 1-2 iterations)
    inline static NodeVariable iter;            // which iteration we are at in the current step
    inline static NodeVariable stepError;       // relative error of the current iteration compared to the previous
    inline static std::atomic<uns> nNonlinComponents = 0; // actual number of nonlinear components in the network; if 0 => no more than 1 DC / timestep iteration needed
    //***********************************************************************
    static void setInitialDC() noexcept { timeStepStart.setValueDC(rvt0); timeStepStop.setValueDC(rvt0); dt.setValueDC(rvt0); }
    static void setFinalDC() noexcept { if (timeStepStart.getValueDC() == rvt0)timeStepStart.setValueDC(1e-20); timeStepStop.setValueDC(timeStepStart.getValueDC()); dt.setValueDC(rvt0); }
    static void stepTransientWithDT(rvt dt_) noexcept { timeStepStart.setValueDC(timeStepStop.getValueDC()); timeStepStop.setValueDC(timeStepStart.getValueDC() + dt_); dt.setValueDC(dt_); }
    static void stepTransientWithTStop(rvt tStop) noexcept { rvt tStart = timeStepStop.getValueDC(); if (tStop < tStart)tStop = tStart; timeStepStart.setValueDC(tStart); timeStepStop.setValueDC(tStop); dt.setValueDC(tStop - tStart); }
    static void setComplexFrequencyForAC(rvt f) noexcept { complexFrequency = { 0, 2 * hmgPi * f }; freq.setValue0DC(f); }
    //***********************************************************************
    static void setComplexFrequencyForTimeConst(rvt f, uns stepPerDecade) noexcept {
    //***********************************************************************
        crvt omega = rvt(2 * hmgPi * f);
        crvt angle = rvt(hmgPi + 1.5 * log(10.0) / stepPerDecade); // 3 samples for the full width at half maximum (FWHM)
        complexFrequency = { omega * cos(angle), omega * sin(angle) };
        freq.setValue0DC(f);
    }
    //***********************************************************************
    static cplx getComplexFrequency() noexcept { return complexFrequency; }
    static rvt  getFrequency() noexcept { return freq.getValue0DC(); }
    //***********************************************************************
};


//***********************************************************************
class ComponentAndControllerBase {
 //***********************************************************************
public:
    //***********************************************************************
    inline static bool isTrapezoid = false;
    //***********************************************************************
    const ComponentDefinition* def;
    const ComponentAndControllerModelBase* pModel;
    bool isEnabled = true;
    uns defaultNodeValueIndex;

    //***********************************************************************
    ComponentAndControllerBase(const ComponentDefinition* def_, uns defaultNodeValueIndex_);
    const ComponentAndControllerModelBase& getModel() const noexcept { return *pModel; }
    virtual void buildOrReplace() = 0; // should only be called after the nodes and params have been set!, buildForAC() do this for AC
    virtual bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept = 0;
    NodeVariable* getNodeVariableSimpleInterfaceNodeID(const SimpleInterfaceNodeID& nodeID) noexcept;
    virtual NodeVariable* getNode(siz nodeIndex) noexcept = 0;
    virtual NodeVariable* getInternalNode(siz nodeIndex) noexcept = 0;
    virtual rvt getCurrentDC(uns y) const noexcept = 0;
    virtual cplx getCurrentAC(uns y) const noexcept = 0;
    //***********************************************************************

    //***********************************************************************
    virtual ~ComponentAndControllerBase() {
    //***********************************************************************
        if (pModel->canBeNonlinear())
            SimControl::nNonlinComponents--;
    }
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
    virtual const NodeVariable& getComponentValue() const noexcept = 0;
    virtual void setNode(siz nodeIndex, NodeVariable* pNode) noexcept = 0;
    virtual void setParam(siz parIndex, const Param& par) noexcept = 0;
    virtual const ComponentBase* getContainedComponent(uns componentIndex)const noexcept = 0;
    //***********************************************************************
    virtual const NodeVariable& getComponentCurrent() const noexcept = 0;
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
    virtual uns calculateControllersDC(uns level, controllerOperationStage stage) { return 0; } // no AC version, return: next level => if <= current, no next level
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
    NodeVariable componentValue;
    NodeVariable componentCurrent;
public:
    using ComponentBase::ComponentBase;
    const NodeVariable& getComponentValue() const noexcept override { return componentValue; }
    const NodeVariable& getComponentCurrent() const noexcept override { return componentCurrent; }
    void buildForAC() override {}
    void buildOrReplace() override {}
    const ComponentBase* getContainedComponent(uns componentIndex)const noexcept override { return nullptr; };
#ifdef HMG_DEBUGPRINT
    void testPrint() const noexcept override {}
#endif
};


//***********************************************************************
class Component_2Node : public RealComponent {
//***********************************************************************
protected:
    NodeVariable*N0 = nullptr, *N1 = nullptr;
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override final {
    //***********************************************************************
        return nodeIndex == 0 ? N0 : N1;
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override final {
    //***********************************************************************
        pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        pNode->extend();
        if (nodeIndex == 0)
            N0 = pNode;
        else
            N1 = pNode;
    }
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
class ComponentConstR : public Component_2Node {
//***********************************************************************
    //***********************************************************************
    // value:  // G! (not R)
    //***********************************************************************
    cplx IAC = cplx0;
public:
    //***********************************************************************
    using Component_2Node::Component_2Node;
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
            IAC = componentValue.getValueDC() * (N0->getValueAC() - N1->getValueAC()); // componentValue.getValueDC() !
            N0->incDAC(-IAC);
            N1->incDAC(IAC);
        }
    }
    //************************** DC functions *******************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    rvt getYDC(uns y, uns x) const noexcept override { return y == x ? componentValue.getValueDC() : -componentValue.getValueDC(); }
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
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
    cplx getCurrentAC(uns y) const noexcept override { return y == 0 ? -IAC : IAC; }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstR_1 final : public ComponentConstR {
//***********************************************************************
    //***********************************************************************
    Param param;
    //***********************************************************************
    // value:  // G! (not R)
    //***********************************************************************
public:
    //***********************************************************************
    using ComponentConstR::ComponentConstR;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param = par; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override { componentValue.setValueDC(rvt1 / param.get()); }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstR_2 final : public ComponentConstR {
//***********************************************************************
    //***********************************************************************
    Param param1, param2;
    //***********************************************************************
    // value:  // G! (not R)
    //***********************************************************************
public:
    //***********************************************************************
    using ComponentConstR::ComponentConstR;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { if (parIndex == 0) param1 = par; else param2 = par; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override { componentValue.setValueDC(rvt1 / (param1.get() * param2.get())); }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstG_1 final : public ComponentConstR {
//***********************************************************************
    //***********************************************************************
    Param param;
    //***********************************************************************
    // value:  // G! (not R)
    //***********************************************************************
public:
    //***********************************************************************
    using ComponentConstR::ComponentConstR;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param = par; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override { componentValue.setValueDC(param.get()); }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstG_2 final : public ComponentConstR {
//***********************************************************************
    //***********************************************************************
    Param param1, param2;
    //***********************************************************************
    // value:  // G! (not R)
    //***********************************************************************
public:
    //***********************************************************************
    using ComponentConstR::ComponentConstR;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { if (parIndex == 0) param1 = par; else param2 = par; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override { componentValue.setValueDC(param1.get() * param2.get()); }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstC : public Component_2Node {
//***********************************************************************
protected:
    rvt Gc = 0, I_stepStart = 0;
    cplx IAC = cplx0;
public:
    //***********************************************************************
    using Component_2Node::Component_2Node;
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
            IAC = componentValue.getValueDC() * SimControl::getComplexFrequency() * (N0->getValueAC() - N1->getValueAC()); // componentValue.getValueDC() !
            N0->incDAC(-IAC);
            N1->incDAC(IAC);
        }
    }
    //************************** DC functions *******************************
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
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
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
    cplx getCurrentAC(uns y) const noexcept override { return y == 0 ? -IAC : IAC; }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstC_1 final : public ComponentConstC {
//***********************************************************************
    //***********************************************************************
    Param param;
    //***********************************************************************
public:
    //***********************************************************************
    using ComponentConstC::ComponentConstC;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param = par; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(param.get());
        crvt dt = SimControl::dt.getValueDC();
        Gc = (dt == 0) ? rvt0 : (componentValue.getValueDC() / dt);
        Gc = isTrapezoid ? 2 * Gc : Gc;
    }
};


//***********************************************************************
class ComponentConstC_2 final : public ComponentConstC {
//***********************************************************************
    //***********************************************************************
    Param param1, param2;
    //***********************************************************************
public:
    //***********************************************************************
    using ComponentConstC::ComponentConstC;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { if (parIndex == 0) param1 = par; else param2 = par; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(param1.get() * param2.get());
        crvt dt = SimControl::dt.getValueDC();
        Gc = (dt == 0) ? rvt0 : (componentValue.getValueDC() / dt);
        Gc = isTrapezoid ? 2 * Gc : Gc;
    }
};


//***********************************************************************
class ComponentConstI : public Component_2Node {
//***********************************************************************
public:
    //***********************************************************************
    using Component_2Node::Component_2Node;
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //************************** DC functions *******************************
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
class ComponentConstI_1 final : public ComponentConstI {
//***********************************************************************
    Param param[4];
    cplx IAC = cplx0;
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC amplitude, par4: AC phase [rad]
    //***********************************************************************
    using ComponentConstI::ComponentConstI;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param[parIndex] = par; }
    //************************** AC / DC functions *******************************
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
            IAC = { A * cos(Phi), A * sin(Phi) };
            N0->incDAC(IAC);
            N1->incDAC(-IAC);
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
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
    //************************** AC functions *******************************
    cplx getCurrentAC(uns y) const noexcept override { return y == 0 ? IAC : -IAC; }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstI_2 final : public ComponentConstI {
//***********************************************************************
    Param param[5];
    cplx IAC = cplx0;
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC amplitude, par4: AC phase [rad], par5: value multiplier
    //***********************************************************************
    using ComponentConstI::ComponentConstI;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param[parIndex] = par; }
    //************************** AC / DC functions *******************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            rvt I = componentValue.getValueDC();
            componentCurrent.setValueDC(-I);
            N0->incDDC(I);
            N1->incDDC(-I);
        }
        else {
            crvt A = param[2].get() * param[4].get();
            crvt Phi = param[3].get();
            IAC = { A * cos(Phi), A * sin(Phi) };
            N0->incDAC(IAC);
            N1->incDAC(-IAC);
        }
    }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(SimControl::timeStepStop.getValueDC() == 0 
            ? param[0].get() * param[4].get()
            : param[1].get() * param[4].get());
    }
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
    //************************** AC functions *******************************
    cplx getCurrentAC(uns y) const noexcept override { return y == 0 ? IAC : -IAC; }
    //***********************************************************************
};


//***********************************************************************
class ComponentConstV : public Component_2Node {
//***********************************************************************
    Param param[5];
    cplx IAC = cplx0;
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC amplitude, par4: AC phase [rad], par5: G
    //***********************************************************************
    using Component_2Node::Component_2Node;
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param[parIndex] = par; }
    //************************** AC / DC functions *******************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt V = componentValue.getValueDC();
            crvt G = param[4].get();
            crvt I = G * (V + N1->getValueDC() - N0->getValueDC());
            componentCurrent.setValueDC(-I);
            N0->incDDC(I);
            N1->incDDC(-I);
        }
        else {
            crvt G = param[4].get();
            crvt A = param[2].get() * G;
            crvt Phi = param[3].get();
            IAC = { A * cos(Phi), A * sin(Phi) };
            IAC += N1->getValueAC() - N0->getValueAC();
            N0->incDAC(IAC);
            N1->incDAC(-IAC);
        }
    }
    //***********************************************************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(SimControl::timeStepStop.getValueDC() == 0 
            ? param[0].get()
            : param[1].get());
    }
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() * param[4].get() : componentCurrent.getValueDC() * param[4].get(); }
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    rvt getYDC(uns y, uns x) const noexcept override { return y == x ? param[4].get() : -param[4].get(); }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        if (N0 == N1)
            return;
        crvt G = param[4].get();
        N0->incYiiDC(G);
        N1->incYiiDC(G);
    }
    //************************** AC functions *******************************
    cplx getCurrentAC(uns y) const noexcept override { return y == 0 ? IAC : -IAC; }
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override { return y == x ? param[4].get() : -param[4].get(); }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        if (N0 == N1)
            return;
        ccplx G = param[4].get();
        N0->incYiiAC(G);
        N1->incYiiAC(G);
    }
    //***********************************************************************
};


//***********************************************************************
class Component_4Node_4Par : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[4] = { nullptr, nullptr, nullptr, nullptr };
    Param param[4];
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override final {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param[parIndex] = par; }
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
    cplx IAC = cplx0;
public:
    //***********************************************************************
    // par1: S DC value before t=0, par2: S DC value after t=0, par3: S AC value, par4: AC phase [rad]
    // componentValue is S !
    //***********************************************************************
    using Component_4Node_4Par::Component_4Node_4Par;
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        if (nodeIndex < 2) {
            pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
            pNode->extend();
        }
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
            IAC = cplx{ A * cos(Phi), A * sin(Phi) }  * (N[2]->getValueAC() - N[3]->getValueAC());
            N[0]->incDAC(IAC);
            N[1]->incDAC(-IAC);
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
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { return y > 1 ? 0 : (y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC()); }
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
    //***********************************************************************
    cplx getCurrentAC(uns y) const noexcept override { return y > 1 ? cplx0 : (y == 0 ? IAC : -IAC); }
    //***********************************************************************
};


//***********************************************************************
class Component_2Node_1Control : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[3] = { nullptr, nullptr, nullptr };
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override final {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override final {
    //***********************************************************************
        pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        if (nodeIndex < 2)
            pNode->extend();
        N[nodeIndex] = pNode;
    }
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
class ComponentConst_Controlled_I final: public Component_2Node_1Control {
//***********************************************************************
    Param param; // multiplier
public:
    //***********************************************************************
    using Component_2Node_1Control::Component_2Node_1Control;
    //************************** AC / DC functions *******************************
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override final { param = par; }
    //************************** AC / DC functions *******************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            rvt I = componentValue.getValueDC();
            componentCurrent.setValueDC(-I);
            N[0]->incDDC(I);
            N[1]->incDDC(-I);
        }
        else {
            rvt I = componentValue.getValueDC();
            N[0]->incDAC(I);
            N[1]->incDAC(-I);
        }
    }
    //************************** DC functions *******************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    rvt getYDC(uns y, uns x) const noexcept override { return rvt0; }
    void calculateYiiDC() noexcept override {}
    //************************** DC functions *******************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(param.get() * N[2]->getValueDC());
    }
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
    //************************** AC functions *******************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override { return cplx0; }
    void calculateYiiAC() noexcept override {}
    cplx getCurrentAC(uns y) const noexcept override { return getCurrentDC(y); }
    //***********************************************************************
};


//***********************************************************************
class ComponentGirator final : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[4] = { nullptr, nullptr, nullptr, nullptr };
    Param param[2];
    rvt IDC1 = rvt0, IDC2 = rvt0;
    cplx IAC1 = cplx0, IAC2 = cplx0;
public:
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param[parIndex] = par; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        pNode->extend();
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
            IDC1 = -param[0].get() * (N[2]->getValueDC() - N[3]->getValueDC());
            IDC2 = param[1].get() * (N[0]->getValueDC() - N[1]->getValueDC());
            N[0]->incDDC(IDC1);
            N[1]->incDDC(-IDC1);
            N[2]->incDDC(IDC2);
            N[3]->incDDC(-IDC2);
        }
        else {
            IAC1 = -param[0].get() * (N[2]->getValueAC() - N[3]->getValueAC());
            IAC2 = param[1].get() * (N[0]->getValueAC() - N[1]->getValueAC());
            N[0]->incDAC(IAC1);
            N[1]->incDAC(-IAC1);
            N[2]->incDAC(IAC2);
            N[3]->incDAC(-IAC2);
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
            else        return y + 2 == x ? -param[0].get() : param[0].get();
        }
        else {
            if (x < 2)  return x + 2 == y ? param[1].get() : -param[1].get();
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
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { switch (y) { case 0: return IDC1; case 1: return -IDC1; case 2: return IDC2; default: return -IDC2; } }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override {}
    //***********************************************************************
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    //***********************************************************************
    cplx getYAC(uns y, uns x) const noexcept override { return getYDC(y,x); }
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
    cplx getCurrentAC(uns y) const noexcept override { switch (y) { case 0: return IAC1; case 1: return -IAC1; case 2: return IAC2; default: return -IAC2; } }
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
class ComponentConstVIB final : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[3] = { nullptr, nullptr, nullptr };
    std::unique_ptr<NodeVariable> possibleCurrentNode;
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
    NodeVariable* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nodeIndex == 0 ? N[2] : nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param[parIndex] = par; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        if(nodeIndex == 2) { // this is an internal node from outside, only this component changes it
            pNode->setDefaultValueIndex(0, true); // the default value is mandatory 0
            pNode->setIsConcurrent(false);
        }
        else
            pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        pNode->extend();
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
            componentCurrent.setValueDC(UB);
            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDDC(UB);
            crvt JB = -IB + V;// - G * UB
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
            ccplx JB = -IB + V;// - G * UB
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
        ComponentConstVIB* coarseVgen = static_cast<ComponentConstVIB*>(coarse);
        rvt sumRet = rvt0;
        if (isDC) {
            switch (type) {
                case rprProlongateU: N[2]->setValue0DC(coarseVgen->N[2]->getValue0DC()); break; // uh = uH
                case rprRestrictU:   coarseVgen->N[2]->setValue0DC(N[2]->getValue0DC()); break; // uH = uh
                case rprRestrictFDD: { // fH = R(fh) + dH  R(dh), ret: sum (dHi  R(dh)i)^2
                        rvt diff = coarseVgen->N[2]->getDDC() - N[2]->getDDC();
                        coarseVgen->N[2]->setFDC(N[2]->getFDC() + diff);
                        sumRet += diff * diff;
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: coarseVgen->N[2]->setDNonConcurrentDC(coarseVgen->N[2]->getValue0DC() - N[2]->getValue0DC()); break; // dH_NonConcurrent = uH  R(uh)
                case rprProlongateDHNCAddToUh:   N[2]->incValue0DC(coarseVgen->N[2]->getDNonConcurrentDC()); break; // uh = uh + P(dH_NonConcurrent)
            }
        }
        else {
            switch (type) {
                case rprProlongateU: N[2]->setValue0AC(coarseVgen->N[2]->getValue0AC()); break; // uh = uH
                case rprRestrictU:   coarseVgen->N[2]->setValue0AC(N[2]->getValue0AC()); break; // uH = uh
                case rprRestrictFDD: { // fH = R(fh) + dH  R(dh), ret: sum (dHi  R(dh)i)^2
                        cplx diff = coarseVgen->N[2]->getDAC() - N[2]->getDAC();
                        coarseVgen->N[2]->setFAC(N[2]->getFAC() + diff);
                        sumRet = absSquare(diff);
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: coarseVgen->N[2]->setDNonConcurrentAC(coarseVgen->N[2]->getValue0AC() - N[2]->getValue0AC()); break; // dH_NonConcurrent = uH  R(uh)
                case rprProlongateDHNCAddToUh:   N[2]->incValue0AC(coarseVgen->N[2]->getDNonConcurrentAC()); break; // uh = uh + P(dH_NonConcurrent)
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
            possibleCurrentNode = std::make_unique<NodeVariable>(); 
            setNode(2, possibleCurrentNode.get());
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
        if (y > 1)
            return rvt0;
        // XB = [-1 1]
        // JRED = XB*NZBJB (and +JA but JA=0)
        return y == 0 ? NZBJB_DC : -NZBJB_DC;
    }
    //***********************************************************************
    rvt getYDC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (x > 1 || y > 1)
            return rvt0;
        return y == x ? param[4].get() : -param[4].get();  // !
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    // the internal node is ignored here
    //***********************************************************************
        if (N[0] == N[1])
            return;
        crvt G = param[4].get(); // !
        N[0]->incYiiDC(G);
        N[1]->incYiiDC(G);
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    rvt getCurrentDC(uns y) const noexcept override { switch (y) { case 0: return -N[2]->getValueDC(); case 1: return N[2]->getValueDC(); default: return rvt0; } }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override { N[2]->setValueAcceptedAC(); }
    //***********************************************************************
    cplx getJreducedAC(uns y) const noexcept override {
    //***********************************************************************
        if (y > 1)
            return cplx0;
        // XB = [-1 1]
        // JRED = XB*NZBJB (and +JA but JA=0)
        return y == 0 ? NZBJB_AC : -NZBJB_AC;
    }
    //***********************************************************************
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (x > 1 || y > 1)
            return cplx0;
        return y == x ? param[4].get() : -param[4].get(); // !
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    // the internal node is ignored here
    //***********************************************************************
        if (N[0] == N[1])
            return;
        ccplx G = param[4].get(); // !
        N[0]->incYiiAC(G);
        N[1]->incYiiAC(G);
    }
    //***********************************************************************
    cplx getCurrentAC(uns y) const noexcept override { switch (y) { case 0: return -N[2]->getValueAC(); case 1: return N[2]->getValueAC(); default: return cplx0; } }
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
class ComponentConstVIN final : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[3] = { nullptr, nullptr, nullptr };
    Param param[5];
    rvt RJB = rvt0;
    cplx CJB = cplx0;
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC value, par4: AC phase [rad],
    // par5: G value of the voltage source, this means a G = 1/G resistor on the B side of the girator
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param[parIndex] = par; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        if(nodeIndex == 2) {
            pNode->setDefaultValueIndex(0, true); // the default value is mandatory 0
        }
        else
            pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        pNode->extend();
        N[nodeIndex] = pNode;
    }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override {}
    void deleteD(bool isDC) noexcept override {}
    void deleteF(bool isDC) noexcept override {}
    void deleteYii(bool isDC) noexcept override {}
    void loadFtoD(bool isDC) noexcept override {}
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt IB = N[1]->getValueDC() - N[0]->getValueDC(); // S2 = -1
            crvt V = componentValue.getValueDC();
            crvt G = 1.0 / param[4].get();
            crvt UB = N[2]->getValueDC();
            componentCurrent.setValueDC(-UB);
            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDDC(UB);
            RJB = IB + V;// - G * UB    => After removing the current of the resistor the voltage of the voltage source is more precise (= the error caused the resistor is added as (-G * UB) + (G * UB) = 0
            N[2]->incDDC(RJB);
        }
        else {
            ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
            crvt A = param[2].get();
            crvt Phi = param[3].get();
            ccplx V = cplx{ A * cos(Phi), A * sin(Phi) };
            crvt G = 1.0 / param[4].get();
            ccplx UB = N[2]->getValueAC();
            N[0]->incDAC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDAC(UB);
            CJB = IB + V;// - G * UB
            N[2]->incDAC(CJB);
        }
    }
    //***********************************************************************
    void forwsubs(bool isDC) override {}
    void backsubs(bool isDC) override {}
    void jacobiIteration(bool isDC) noexcept override {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    //************************** DC functions *******************************
    void acceptIterationDC(bool isNoAlpha) noexcept override { if (isNoAlpha) N[2]->setValueAcceptedNoAlphaDC(); else N[2]->setValueAcceptedDC(); }
    void acceptStepDC() noexcept override { N[2]->setStepStartFromAcceptedDC(); }
    void buildOrReplace() override {}
    //***********************************************************************
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
        switch (y) {
            case 0: 
                if (x == 2) return  1; // S1
                break;
            case 1:
                if (x == 2) return -1; // -S1
                break;
            case 2:
                switch (x) {
                    case 0: return  1; // S2
                    case 1: return -1; // -S2
                    case 2: return  rvt1 / param[4].get(); // G
                }
        }
        return x == y ? gmin : -gmin;
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        N[0]->incYiiDC(gmin);
        N[1]->incYiiDC(gmin);
        crvt G = rvt1 / param[4].get();
        N[2]->incYiiDC(G);
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override {
    //***********************************************************************
        switch (y) {
            case 0: return -componentCurrent.getValueDC();
            case 1: return componentCurrent.getValueDC();
            case 2: return -RJB;
        }
        return rvt0;
    }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override { N[2]->setValueAcceptedAC(); }
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override { return getYDC(y, x); }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        N[0]->incYiiAC(gmin);
        N[1]->incYiiAC(gmin);
        crvt G = rvt1 / param[4].get();
        N[2]->incYiiAC(G);
    }
    //***********************************************************************
    cplx getCurrentAC(uns y) const noexcept override { 
    //***********************************************************************
        switch (y) {
            case 0: return -N[2]->getValueAC();
            case 1: return N[2]->getValueAC();
            case 2: return -CJB;
        }
        return rvt0;
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
class ComponentMIB final : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[3] = { nullptr, nullptr, nullptr };
    std::unique_ptr<NodeVariable> possibleCurrentNode;
    Param param;
    rvt NZBJB_DC = rvt0;
    cplx NZBJB_AC = cplx0;
public:
    //***********************************************************************
    // par: G value of the voltmeter, this means a G = 1/G resistor on the B side of the girator
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nodeIndex == 0 ? N[2] : nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param = par; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        if(nodeIndex == 2) { // this is an internal node from outside, only this component changes it
            pNode->setDefaultValueIndex(0, true); // the default value is mandatory 0
            pNode->setIsConcurrent(false);
        }
        else
            pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        pNode->extend();
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
        crvt G = 1.0 / param.get();
        if (isDC) {
            crvt IB = N[1]->getValueDC() - N[0]->getValueDC(); // S2 = -1
            crvt UB = N[2]->getValueDC();
            componentCurrent.setValueDC(UB);
            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDDC(UB);
            crvt JB = -IB;// - G * UB
            N[2]->setDDC(JB);
        }
        else {
            ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
            ccplx UB = N[2]->getValueAC();
            N[0]->incDAC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDAC(UB);
            ccplx JB = -IB;// - G * UB
            N[2]->setDAC(JB);
        }
    }
    //***********************************************************************
    void forwsubs(bool isDC) override {
    //***********************************************************************
        crvt NZB = -param.get(); // crvt G = 1.0 / param.get(); crvt NZB = -1.0 / G;
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
            crvt UB = (UA1 - UA2) * param.get() + NZBJB_DC;
            N[2]->setVDC(UB);
            componentCurrent.setValueDC(N[2]->getValueDC());
        }
        else {
            ccplx UA1 = N[0]->getVAC();
            ccplx UA2 = N[1]->getVAC();
            ccplx UB = (UA1 - UA2) * param.get() + NZBJB_AC;
            N[2]->setVAC(UB);
        }
    }
    //***********************************************************************
    void jacobiIteration(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            N[2]->setValue0DC(N[2]->getValue0DC() + param.get() * N[2]->getDDC()); // + or - ? I'm not sure
        }
        else {
            N[2]->setValue0AC(N[2]->getValue0AC() + param.get() * N[2]->getDAC()); // + or - ? I'm not sure
        }
    }
    //***********************************************************************
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final {
    //***********************************************************************
        ComponentMIB* coarseVgen = static_cast<ComponentMIB*>(coarse);
        rvt sumRet = rvt0;
        if (isDC) {
            switch (type) {
                case rprProlongateU: N[2]->setValue0DC(coarseVgen->N[2]->getValue0DC()); break; // uh = uH
                case rprRestrictU:   coarseVgen->N[2]->setValue0DC(N[2]->getValue0DC()); break; // uH = uh
                case rprRestrictFDD: { // fH = R(fh) + dH  R(dh), ret: sum (dHi  R(dh)i)^2
                        rvt diff = coarseVgen->N[2]->getDDC() - N[2]->getDDC();
                        coarseVgen->N[2]->setFDC(N[2]->getFDC() + diff);
                        sumRet += diff * diff;
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: coarseVgen->N[2]->setDNonConcurrentDC(coarseVgen->N[2]->getValue0DC() - N[2]->getValue0DC()); break; // dH_NonConcurrent = uH  R(uh)
                case rprProlongateDHNCAddToUh:   N[2]->incValue0DC(coarseVgen->N[2]->getDNonConcurrentDC()); break; // uh = uh + P(dH_NonConcurrent)
            }
        }
        else {
            switch (type) {
                case rprProlongateU: N[2]->setValue0AC(coarseVgen->N[2]->getValue0AC()); break; // uh = uH
                case rprRestrictU:   coarseVgen->N[2]->setValue0AC(N[2]->getValue0AC()); break; // uH = uh
                case rprRestrictFDD: { // fH = R(fh) + dH  R(dh), ret: sum (dHi  R(dh)i)^2
                        cplx diff = coarseVgen->N[2]->getDAC() - N[2]->getDAC();
                        coarseVgen->N[2]->setFAC(N[2]->getFAC() + diff);
                        sumRet = absSquare(diff);
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: coarseVgen->N[2]->setDNonConcurrentAC(coarseVgen->N[2]->getValue0AC() - N[2]->getValue0AC()); break; // dH_NonConcurrent = uH  R(uh)
                case rprProlongateDHNCAddToUh:   N[2]->incValue0AC(coarseVgen->N[2]->getDNonConcurrentAC()); break; // uh = uh + P(dH_NonConcurrent)
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
            possibleCurrentNode = std::make_unique<NodeVariable>(); 
            setNode(2, possibleCurrentNode.get());
        } 
    }
    //***********************************************************************
    void calculateValueDC() noexcept override {}
    rvt getJreducedDC(uns y) const noexcept override {
        if (y > 1)
            return rvt0;
        return y == 0 ? NZBJB_DC : -NZBJB_DC; 
    }
    rvt getYDC(uns y, uns x) const noexcept override { 
        if (x > 1 || y > 1)
            return rvt0;
        return y == x ? param.get() : -param.get();
    }  // !
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    // the internal node is ignored here
    //***********************************************************************
        if (N[0] == N[1])
            return;
        crvt G = param.get(); // !
        N[0]->incYiiDC(G);
        N[1]->incYiiDC(G);
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    rvt getCurrentDC(uns y) const noexcept override { switch (y) { case 0: return -N[2]->getValueDC(); case 1: return N[2]->getValueDC(); default: return rvt0; } }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override { N[2]->setValueAcceptedAC(); }
    //***********************************************************************
    cplx getJreducedAC(uns y) const noexcept override {
    //***********************************************************************
        if (y > 1)
            return cplx0;
        // XB = [-1 1]
        // JRED = XB*NZBJB (and +JA but JA=0)
        return y == 0 ? NZBJB_AC : -NZBJB_AC;
    }
    //***********************************************************************
    cplx getYAC(uns y, uns x) const noexcept override {
    //***********************************************************************
        if (x > 1 || y > 1)
            return cplx0;
        return y == x ? param.get() : -param.get(); // !
    }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    // the internal node is ignored here
    //***********************************************************************
        if (N[0] == N[1])
            return;
        ccplx G = param.get(); // !
        N[0]->incYiiAC(G);
        N[1]->incYiiAC(G);
    }
    //***********************************************************************
    cplx getCurrentAC(uns y) const noexcept override { switch (y) { case 0: return -N[2]->getValueAC(); case 1: return N[2]->getValueAC(); default: return cplx0; } }
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
class ComponentMIN final : public RealComponent {
//***********************************************************************
protected:
    NodeVariable* N[3] = { nullptr, nullptr, nullptr };
    Param param;
    rvt RJB = rvt0;
    cplx CJB = cplx0;
public:
    //***********************************************************************
    // par1: DC value before t=0, par2: DC value after t=0, par3: AC value, par4: AC phase [rad],
    // par5: G value of the voltage source, this means a G = 1/G resistor on the B side of the girator
    //***********************************************************************
    using RealComponent::RealComponent;
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return N[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { return false; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { param = par; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        if(nodeIndex == 2) {
            pNode->setDefaultValueIndex(0, true); // the default value is mandatory 0
        }
        else
            pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
        pNode->extend();
        N[nodeIndex] = pNode;
    }
    //************************** AC / DC functions *******************************
    void resetNodes(bool isDC) noexcept override {}
    void deleteD(bool isDC) noexcept override {}
    void deleteF(bool isDC) noexcept override {}
    void deleteYii(bool isDC) noexcept override {}
    void loadFtoD(bool isDC) noexcept override {}
    bool isJacobianMXSymmetrical(bool isDC)const noexcept override { return true; }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    //***********************************************************************
        if (isDC) {
            crvt IB = N[1]->getValueDC() - N[0]->getValueDC(); // S2 = -1
            crvt G = 1.0 / param.get();
            crvt UB = N[2]->getValueDC();
            componentCurrent.setValueDC(-UB);
            N[0]->incDDC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDDC(UB);
            RJB = IB;// - G * UB
            N[2]->incDDC(RJB);
        }
        else {
            ccplx IB = N[1]->getValueAC() - N[0]->getValueAC(); // S2 = -1
            crvt G = 1.0 / param.get();
            ccplx UB = N[2]->getValueAC();
            N[0]->incDAC(-UB); // S1 = -1 => I1 = -1 * UB
            N[1]->incDAC(UB);
            CJB = IB; // - G * UB
            N[2]->incDAC(CJB);
        }
    }
    //***********************************************************************
    void forwsubs(bool isDC) override {}
    void backsubs(bool isDC) override {}
    void jacobiIteration(bool isDC) noexcept override {}
    rvt recursiveProlongRestrictCopy(bool isDC, RecursiveProlongRestrictType type, ComponentBase* coarse) noexcept override final { return rvt0; }
    rvt calculateResidual(bool isDC) const noexcept override final { return rvt0; }
    //************************** DC functions *******************************
    void acceptIterationDC(bool isNoAlpha) noexcept override { if (isNoAlpha) N[2]->setValueAcceptedNoAlphaDC(); else N[2]->setValueAcceptedDC(); }
    void acceptStepDC() noexcept override { N[2]->setStepStartFromAcceptedDC(); }
    void buildOrReplace() override {}
    //***********************************************************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        componentValue.setValueDC(0);
    }
    //***********************************************************************
    rvt getJreducedDC(uns y) const noexcept override { return rvt0; }
    //***********************************************************************
    rvt getYDC(uns y, uns x) const noexcept override {
    //***********************************************************************
        switch (y) {
            case 0: 
                if (x == 2) return  1; // S1
                break;
            case 1:
                if (x == 2) return -1; // -S1
                break;
            case 2:
                switch (x) {
                    case 0: return  1; // S2
                    case 1: return -1; // -S2
                    case 2: return  rvt1 / param.get(); // G
                }
        }
        return x == y ? gmin : -gmin;
    }
    //***********************************************************************
    void calculateYiiDC() noexcept override {
    //***********************************************************************
        N[0]->incYiiDC(gmin);
        N[1]->incYiiDC(gmin);
        crvt G = rvt1 / param.get();
        N[2]->incYiiDC(G);
    }
    //***********************************************************************
    DefectCollector collectCurrentDefectDC() const noexcept override { return DefectCollector{}; }
    DefectCollector collectVoltageDefectDC() const noexcept override { return DefectCollector{}; }
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override {
    //***********************************************************************
        switch (y) {
            case 0: return -componentCurrent.getValueDC();
            case 1: return componentCurrent.getValueDC();
            case 2: return -RJB;
        }
        return rvt0;
    }
    //************************** AC functions *******************************
    void acceptIterationAndStepAC() noexcept override { N[2]->setValueAcceptedAC(); }
    cplx getJreducedAC(uns y) const noexcept override { return cplx0; }
    cplx getYAC(uns y, uns x) const noexcept override { return getYDC(y, x); }
    //***********************************************************************
    void calculateYiiAC() noexcept override {
    //***********************************************************************
        N[0]->incYiiAC(gmin);
        N[1]->incYiiAC(gmin);
        crvt G = rvt1 / param.get();
        N[2]->incYiiAC(G);
    }
    //***********************************************************************
    cplx getCurrentAC(uns y) const noexcept override { 
    //***********************************************************************
        switch (y) {
            case 0: return -N[2]->getValueAC();
            case 1: return N[2]->getValueAC();
            case 2: return -CJB;
        }
        return rvt0;
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
    std::vector<NodeVariable*> externalNodes;
    std::vector<Param> pars; // pars[0] is G
    std::vector<ComponentAndControllerBase*> componentParams;
    std::vector<ComponentAndControllerBase*> functionComponentParams;
    std::vector<rvt> workField;
    cplx IAC = cplx0;
public:
    //***********************************************************************
    Component_Function_Controlled_I_with_const_G(const ComponentDefinition* def_, uns defaultNodeValueIndex_) :RealComponent{ def_, defaultNodeValueIndex_ } {
    // workField size: 
    //***********************************************************************
        is_equal_error<siz>(def->nodesConnectedTo.size(), pModel->getN_ExternalNodes(), "Component_Function_Controlled_I_with_const_G::Component_Function_Controlled_I_with_const_G");
        externalNodes.resize(def->nodesConnectedTo.size());
        pars.resize(def->params.size());
        componentParams.resize(def->componentParams.size());
        const Model_Function_Controlled_I_with_const_G* pM = dynamic_cast<const Model_Function_Controlled_I_with_const_G*>(pModel);
        workField.resize(pM->controlFunction->getN_WorkingField() + pM->controlFunction->getN_Param() + 1);
    }
    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override {
    //***********************************************************************
        return externalNodes[nodeIndex];
    }
    //***********************************************************************
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return nullptr; }
    //***********************************************************************
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { componentParams[parIndex] = ct; return true; }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { pars[parIndex] = par; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        if (nodeIndex < pModel->getN_X_Nodes() + pModel->getN_Y_Nodes()) { // There are 2 IO nodes.
            pNode->setDefaultValueIndex(defaultNodeValueIndex, false);
            pNode->extend();
        }
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
            componentCurrent.setValueDC(-Ifull);
            externalNodes[0]->incDDC(Ifull);
            externalNodes[1]->incDDC(-Ifull);
        }
        else {
            crvt G = pars[0].get();
            ccplx IG = G * (externalNodes[0]->getValueAC() - externalNodes[1]->getValueAC());
            IAC = componentValue.getValueDC() - IG; // getValueDC !
            externalNodes[0]->incDAC(IAC);
            externalNodes[1]->incDAC(-IAC);
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
    //***********************************************************************
    void buildOrReplace() override {
    //***********************************************************************
        const Model_Function_Controlled_I_with_const_G* pMod = static_cast<const Model_Function_Controlled_I_with_const_G*>(pModel);
        csiz siz = pMod->functionComponentParams.size();
        functionComponentParams.resize(siz);
        for (uns i = 0; i < siz; i++) {
            cuns index = pMod->functionComponentParams[i];
            functionComponentParams[i] = index == unsMax ? this : componentParams[index];
        }
    }
    //***********************************************************************
    void calculateValueDC() noexcept override {
    //***********************************************************************
        const Model_Function_Controlled_I_with_const_G& model = static_cast<const Model_Function_Controlled_I_with_const_G&>(*pModel);
        const std::vector<NodeConnectionInstructions::ConnectionInstruction>& load = model.functionSources.load;
        for (uns i = 0; i < load.size(); i++) {
            switch (load[i].nodeOrVarType) {
                case NodeConnectionInstructions::sExternalNodeValue:
                    workField[model.indexField[load[i].functionParamIndex]] = externalNodes[load[i].nodeOrVarIndex]->getValueDC();
                    break;
                case NodeConnectionInstructions::sExternalNodeStepstart:
                    workField[model.indexField[load[i].functionParamIndex]] = externalNodes[load[i].nodeOrVarIndex]->getStepStartDC();
                    break;
                case NodeConnectionInstructions::sParam:
                    workField[model.indexField[load[i].functionParamIndex]] = pars[load[i].nodeOrVarIndex].get();
                    break;
            }
        }
        model.controlFunction->evaluate(&model.indexField[0], &workField[0], this, LineDescription(), functionComponentParams.size() == 0 ? nullptr : &functionComponentParams.front());
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
        rvt dFv_per_dUi = x < model.nodeToFunctionParam.size() && model.nodeToFunctionParam[x] != unsMax
            ? model.controlFunction->devive(&model.indexField[0], &workField[0], const_cast<Component_Function_Controlled_I_with_const_G*>(this), 
                model.nodeToFunctionParam[x], LineDescription(), functionComponentParams.size() == 0 ? nullptr : const_cast<ComponentAndControllerBase**>(&functionComponentParams.front()))
            : rvt0;
        //printf("[%u %u] dFv_per_dUi = %g\n", y, x, dFv_per_dUi);
        //dFv_per_dUi = 0;
        if (dFv_per_dUi > gmax)
            dFv_per_dUi = gmax;
        if (dFv_per_dUi < -gmax)
            dFv_per_dUi = -gmax;
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
    rvt getCurrentDC(uns y) const noexcept override { return y == 0 ? -componentCurrent.getValueDC() : componentCurrent.getValueDC(); }
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
    cplx getCurrentAC(uns y) const noexcept override { return y == 0 ? IAC : -IAC; }
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
class Controller final : public ComponentAndControllerBase {
//***********************************************************************
    friend class HmgF_Load_Controller_Node_StepStart;
    friend class HmgF_Load_Controller_mVar_StepStart;
    friend class HmgF_Load_Controller_mVar_Value;
    friend class HmgF_Set_Controller_mVar_Value;
    friend class HmgF_Set_Controller_mVar_ValueFromStepStart;
    friend class CircuitStorage;
    std::vector<NodeVariable> mVars;
    uns nmVars = 0;
    std::vector<NodeVariable*> externalNodes;
    std::vector<Param> pars;
    std::vector<ComponentAndControllerBase*> componentParams;
    std::vector<ComponentAndControllerBase*> functionComponentParams;
    std::vector<rvt> workField;
public:
    //***********************************************************************
    Controller(const ComponentDefinition* def_, uns defaultNodeValueIndex_) :ComponentAndControllerBase{ def_, defaultNodeValueIndex_ } {
    // workField size: 
    //***********************************************************************
        is_equal_error<siz>(def->nodesConnectedTo.size(), pModel->getN_ExternalNodes(), "Controller::Controller");
        externalNodes.resize(def->nodesConnectedTo.size());
        pars.resize(def->params.size());
        componentParams.resize(def->componentParams.size());
        const ModelController* pM = dynamic_cast<const ModelController*>(pModel);
        cuns wfs = pM->controlFunction->getN_WorkingField() + pM->controlFunction->getN_Param() + 1;
        workField.resize(wfs);
        for (uns i = 0; i < wfs; i++)
            workField[i] = rvt0;
    }
    //***********************************************************************

    //***********************************************************************
    void buildOrReplace() override {
    //***********************************************************************
        if (mVars.size() != 0)
            return;

        const ModelController& model = static_cast<const ModelController&>(*pModel);

        uns nUnconnectedExtNode = 0;
        cuns nEndExtNodes = model.getN_ExternalNodes();
        for (uns i = 0; i < nEndExtNodes; i++) {
            if (externalNodes[i] == nullptr)
                nUnconnectedExtNode++;
        }

        uns nUnconnectedOnodeIndex = model.nMVars;
        nmVars = nUnconnectedOnodeIndex + nUnconnectedExtNode;
        mVars.resize(nmVars);

        for (uns i = 0; i < model.nMVars; i++)
            mVars[i].setDefaultValueIndex(0, true);

        for (uns i = 0; i < nEndExtNodes; i++) {
            if (externalNodes[i] == nullptr)
                externalNodes[i] = &mVars[nUnconnectedOnodeIndex++];
        }

        csiz siz = model.functionComponentParams.size();
        functionComponentParams.resize(siz);
        for (uns i = 0; i < siz; i++) {
            cuns index = model.functionComponentParams[i];
            functionComponentParams[i] = index == unsMax ? this : componentParams[index];
        }
    }

    //***********************************************************************
    NodeVariable* getNode(siz nodeIndex) noexcept override { return externalNodes[nodeIndex]; }
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return &mVars[nodeIndex]; }
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept { externalNodes[nodeIndex] = pNode; }
    void setParam(siz parIndex, const Param& par)noexcept { pars[parIndex] = par; }
    rvt getCurrentDC(uns y) const noexcept override { return rvt0; }
    cplx getCurrentAC(uns y) const noexcept override { return cplx0; }
    //***********************************************************************
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { componentParams[parIndex] = ct; return true; }
    //***********************************************************************
    Param& getParam(siz parIndex) noexcept { return pars[parIndex]; }
    //***********************************************************************
    void loadNodesAndParamsToFunction() noexcept {
    //***********************************************************************
        const ModelController& model = static_cast<const ModelController&>(*pModel);
        const std::vector<NodeConnectionInstructions::ConnectionInstruction>& load = model.functionSources.load;
        for (uns i = 0; i < load.size(); i++) {
            switch (load[i].nodeOrVarType) {
                case NodeConnectionInstructions::sExternalNodeValue:
                    workField[model.indexField[load[i].functionParamIndex]] = externalNodes[load[i].nodeOrVarIndex]->getValueDC();
                    break;
                case NodeConnectionInstructions::sExternalNodeStepstart:
                    workField[model.indexField[load[i].functionParamIndex]] = externalNodes[load[i].nodeOrVarIndex]->getStepStartDC();
                    break;
                case NodeConnectionInstructions::sParam:
                    workField[model.indexField[load[i].functionParamIndex]] = pars[load[i].nodeOrVarIndex].get();
                    break;
                case NodeConnectionInstructions::sMVarValue:
                    workField[model.indexField[load[i].functionParamIndex]] = mVars[load[i].nodeOrVarIndex].getValueDC();
                    break;
                case NodeConnectionInstructions::sMVarStepstart:
                    workField[model.indexField[load[i].functionParamIndex]] = mVars[load[i].nodeOrVarIndex].getStepStartDC();
                    break;
            }
        }
    }
    //***********************************************************************
    void evaluate_and_storeNodes() {
    //***********************************************************************
        const ModelController& model = static_cast<const ModelController&>(*pModel);

        model.controlFunction->evaluate(&model.indexField[0], &workField[0], this, LineDescription(), functionComponentParams.size() == 0 ? nullptr : &functionComponentParams.front());

        for (uns i = 0; i < model.functionSources.store.size(); i++) {
            const NodeConnectionInstructions::ConnectionInstruction& dest = model.functionSources.store[i];

            // functionParamIndex == 0 => return, >0 => par

            if (dest.nodeOrVarType == NodeConnectionInstructions::sExternalNodeValue)
                externalNodes[dest.nodeOrVarIndex]->setValueDC(workField[model.indexField[dest.functionParamIndex]]);
            else if (dest.nodeOrVarType == NodeConnectionInstructions::sMVarValue)
                mVars[dest.nodeOrVarIndex].setValueDC(workField[model.indexField[dest.functionParamIndex]]);
            else
                throw hmgExcept("Controller::evaluate_and_storeNodes", "inappropriate STORE type: only OUT node and Var allowed, %u arrived", dest.nodeOrVarType);
        }
    }
    //***********************************************************************
    void resetMVars() noexcept {
    // !!! External nodes of disabled controllers will be reseted!
    // !!! This cannot be avoided because at the place of the creation 
    //     it is impossible to see wether a node will be used in a disabled
    //     component. 
    //***********************************************************************
        for (uns i = 0; i < nmVars; i++)
            mVars[i].reset();
        const ModelController& model = static_cast<const ModelController&>(*pModel);
        for (uns i = 0; i < model.defaultNodeValues.size(); i++) {
            const DefaultNodeParameter& np = model.defaultNodeValues[i];
            if (np.nodeID.type == nvtA) {
                externalNodes[np.nodeID.index]->setValueDC(np.defaultValue);
                externalNodes[np.nodeID.index]->setStepStartDC(np.defaultValue);
            }
            else if (np.nodeID.type == nvtO) {
                externalNodes[np.nodeID.index + model.externalNs.nANodes]->setValueDC(np.defaultValue);
                externalNodes[np.nodeID.index + model.externalNs.nANodes]->setStepStartDC(np.defaultValue);
            }
            else if (np.nodeID.type == nvtB) {
                mVars[np.nodeID.index].setValueDC(np.defaultValue);
                mVars[np.nodeID.index].setStepStartDC(np.defaultValue);
            }
            else
                printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"); // impossible
        }
    }
    //***********************************************************************
    void setStepStartFromValue() noexcept {
    //***********************************************************************
        for (uns i = 0; i < nmVars; i++)
            mVars[i].setStepStartFromAcceptedDC();
    }
};


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
    friend class CircuitStorage;
    friend struct FineCoarseConnectionDescription;
    //***********************************************************************
    std::vector<std::unique_ptr<ComponentBase>> components;
    std::vector<std::unique_ptr<Controller>> controllers;
    std::vector<NodeVariable*> externalNodes;
    std::vector<cplx> externalCurrents;
    std::vector<NodeVariable> internalNodesAndVars;
    std::vector<Param> pars;
    std::vector<ComponentAndControllerBase*> componentParams;
    std::unique_ptr<SubCircuitFullMatrixReductorDC> sfmrDC;
    std::unique_ptr<SubCircuitFullMatrixReductorAC> sfmrAC;
    uns nInternalNodesAndVars = 0;
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
    ComponentSubCircuit(const ComponentDefinition* def_, uns defaultNodeValueIndex_) :ComponentBase{ def_, defaultNodeValueIndex_ } {
    //***********************************************************************
        is_equal_error<siz>(def->nodesConnectedTo.size(), pModel->getN_ExternalNodes(), "ComponentSubCircuit::ComponentSubCircuit");
        externalNodes.resize(def->nodesConnectedTo.size());
        externalCurrents.resize(pModel->getN_X_Nodes());
        pars.resize(def->params.size());
        componentParams.resize(def->componentParams.size());
    }
    //***********************************************************************
    const NodeVariable& getComponentValue() const noexcept override { return getNContainedComponents() == 0 ? Rails::V[0]->rail : getContainedComponent(0)->getComponentValue(); }
    const NodeVariable& getComponentCurrent() const noexcept override { return getNContainedComponents() == 0 ? Rails::V[0]->rail : getContainedComponent(0)->getComponentCurrent(); }
    NodeVariable* getNode(siz nodeIndex) noexcept override { return externalNodes[nodeIndex]; }
    NodeVariable* getInternalNode(siz nodeIndex) noexcept override final { return &internalNodesAndVars[nodeIndex]; }
    //***********************************************************************
    void setNode(siz nodeIndex, NodeVariable* pNode)noexcept override {
    //***********************************************************************
        externalNodes[nodeIndex] = pNode;
    }
    //***********************************************************************
    void setParam(siz parIndex, const Param& par)noexcept override { pars[parIndex] = par; }
    //***********************************************************************
    bool setComponentParam(siz parIndex, ComponentAndControllerBase* ct) noexcept override { componentParams[parIndex] = ct; return true; }
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
        externalNodesToComponents.resize(pModel->getN_ExternalNodes()); // ONode is possible
        internalNodesToComponents.clear();
        internalNodesToComponents.resize(static_cast<const ModelSubCircuit*>(pModel)->getN_N_Nodes()); // ?? What about ONodes?
        for (uns i = 0; i < components.size(); i++) {
            const auto& comp = *components[i];
            if (comp.isEnabled) {
                for (const auto& node : comp.def->nodesConnectedTo) {
                    if (node.type == CDNodeType::cdntInternal) {
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
                    else if (node.type == CDNodeType::cdntExternal) {
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
                    // ground, var and unconnected nodes ignored
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
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].reset();
            for (auto& comp : components)
                comp->resetNodes(true);
            for (auto& ctrl : controllers)
                ctrl.get()->resetMVars();
        }
        else {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].resetAC();
            for (auto& comp : components)
                comp->resetNodes(false);
        }
    }
    //***********************************************************************
    void deleteD(bool isDC) noexcept override { 
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].deleteDDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteD(true);
        }
        else {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].deleteDAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteD(false);
        }
    }
    //***********************************************************************
    void calculateCurrent(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            cuns currSiz = (uns)externalCurrents.size();
            for (uns i = 0; i < currSiz; i++)
                getRe(externalCurrents[i]) = rvt0;
            for (auto& comp : components) {
                if (comp->isEnabled) comp->calculateCurrent(true);
                
                // calculating the external node currents
                
                const std::vector<CDNode>& nodesConnectedTo = comp->def->nodesConnectedTo;
                for (uns i = 0; i < nodesConnectedTo.size();i++) {
                    const CDNode& node = nodesConnectedTo[i];
                    if (node.type == cdntExternal && node.index < currSiz)
                        getRe(externalCurrents[node.index]) += comp->getCurrentDC(i);
                }
            }
        }
        else {
            cuns currSiz = (uns)externalCurrents.size();
            for (uns i = 0; i < currSiz; i++)
                externalCurrents[i] = cplx0;
            for (auto& comp : components) {
                if (comp->isEnabled) comp->calculateCurrent(false);

                // calculating the external node currents

                const std::vector<CDNode>& nodesConnectedTo = comp->def->nodesConnectedTo;
                for (uns i = 0; i < nodesConnectedTo.size(); i++) {
                    const CDNode& node = nodesConnectedTo[i];
                    if (node.type == cdntExternal && node.index < currSiz)
                        externalCurrents[node.index] += comp->getCurrentAC(i);
                }
            }
        }
    }
    //***********************************************************************
    uns calculateControllersDC(uns level, controllerOperationStage stage) override {
    // TO PARALLEL
    //***********************************************************************
        uns nextLevel = level;
        for (auto& comp : components)
            if (comp->isEnabled) {
                uns retLevel = comp->calculateControllersDC(level, stage);
                if (retLevel > level && (nextLevel == level || retLevel < nextLevel))
                    nextLevel = retLevel;
            }

        if (stage == cosLoad) {
            for (auto& ctrl : controllers)
                if (ctrl.get()->isEnabled) {
                    uns ctrlLevel = ctrl.get()->def->ctrlLevel;
                    if (ctrlLevel == level)
                        ctrl.get()->loadNodesAndParamsToFunction();
                    else if (ctrlLevel > level && (nextLevel == level || ctrlLevel < nextLevel)) // choosing the smallest of the greaters
                        nextLevel = ctrlLevel;
                }
        }
        else { // cosEvaluateAndStore
            for (auto& ctrl : controllers)
                if (ctrl.get()->isEnabled && ctrl.get()->def->ctrlLevel == level) 
                    ctrl.get()->evaluate_and_storeNodes();
        }
        return nextLevel;
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
            for (uns i = 0; i < model.getN_N_Nodes(); i++) { // ?? What about ONodes?
                crvt y = internalNodesAndVars[i].getYiiDC();
                crvt d = internalNodesAndVars[i].getDDC();
                if (y != rvt0)
                    internalNodesAndVars[i].incValue0DC(d / y);
            }
        }
        else {
            for (uns i = 0; i < model.getN_N_Nodes(); i++) { // ?? What about ONodes?
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
        cuns NNormalInternal = pModel->getN_N_Nodes(); // ?? What about ONodes?
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
                case rprRestrictFDD: // fH = R(fh) + dH  R(dh), ret: sum (dHi  R(dh)i)^2
                    for (uns i = 0; i < NInternal; i++) {
                        rvt diff = coarseSubckt->internalNodesAndVars[i].getDDC() - internalNodesAndVars[i].getDDC();
                        coarseSubckt->internalNodesAndVars[i].setFDC(internalNodesAndVars[i].getFDC() + diff);
                        sumRet += diff * diff;
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: // dH_NonConcurrent = uH  R(uh)
                    for (uns i = 0; i < NInternal; i++)
                        coarseSubckt->internalNodesAndVars[i].setDNonConcurrentDC(coarseSubckt->internalNodesAndVars[i].getValue0DC() - internalNodesAndVars[i].getValue0DC());
                    break;
                case rprProlongateDHNCAddToUh: // uh = uh + P(dH_NonConcurrent)
                    for (uns i = 0; i < NInternal; i++)
                        internalNodesAndVars[i].setValue0DC(internalNodesAndVars[i].getValue0DC() + coarseSubckt->internalNodesAndVars[i].getDNonConcurrentDC());
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
                case rprRestrictFDD: // fH = R(fh) + dH  R(dh), ret: sum (dHi  R(dh)i)^2
                    for (uns i = 0; i < NInternal; i++) {
                        cplx diff = coarseSubckt->internalNodesAndVars[i].getDAC() - internalNodesAndVars[i].getDAC();
                        coarseSubckt->internalNodesAndVars[i].setFAC(internalNodesAndVars[i].getFAC() + diff);
                        sumRet += absSquare(diff);
                    }
                    break;
                case rpruHMinusRestrictUhToDHNC: // dH_NonConcurrent = uH  R(uh)
                    for (uns i = 0; i < NInternal; i++)
                        coarseSubckt->internalNodesAndVars[i].setDNonConcurrentAC(coarseSubckt->internalNodesAndVars[i].getValue0AC() - internalNodesAndVars[i].getValue0AC());
                    break;
                case rprProlongateDHNCAddToUh: // uh = uh + P(dH_NonConcurrent)
                    for (uns i = 0; i < NInternal; i++)
                        internalNodesAndVars[i].setValue0AC(internalNodesAndVars[i].getValue0AC() + coarseSubckt->internalNodesAndVars[i].getDNonConcurrentAC());
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
        cuns NInodes = pModel->getN_N_Nodes(); // ?? What about ONodes?

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
        for (uns i = 0; i < model.getN_InternalNodes(); i++) {
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
        for (uns i = 0; i < model.getN_InternalNodes(); i++) {
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
            for (uns i = 0; i < model.getN_InternalNodes(); i++) // the ONodes come from outside
                internalNodesAndVars[i].setValueAcceptedNoAlphaDC();
        else {
            for (uns i = 0; i < model.getN_InternalNodes(); i++) // the ONodes come from outside
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
        for (uns i = 0; i < nInternalNodesAndVars; i++)
            internalNodesAndVars[i].setStepStartFromAcceptedDC();
        for (auto& comp : components)
            if (comp->isEnabled) comp->acceptStepDC();
        for (auto& ctrl : controllers)
            if (ctrl.get()->isEnabled) ctrl.get()->setStepStartFromValue();
    }
    //***********************************************************************
    rvt getCurrentDC(uns y) const noexcept override { return y < externalCurrents.size() ? externalCurrents[y].real() : rvt0; }
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
        for (uns i = 0; i < model.getN_InternalNodes(); i++) // the ONodes come from outside
            internalNodesAndVars[i].setValueAcceptedAC();
        for (auto& comp : components)
            if (comp->isEnabled) comp->acceptIterationAndStepAC();
    }
    //************************  Multigrid Functions  ************************
    void deleteF(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].deleteFDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteF(true);
        }
        else {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].deleteFAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteF(false);
        }
    }
    //***********************************************************************
    void deleteYii(bool isDC) noexcept override { 
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].deleteYiiDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteYii(true);
        }
        else {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].deleteYiiAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->deleteYii(false);
        }
    }
    //***********************************************************************
    void loadFtoD(bool isDC) noexcept override {
    // TO PARALLEL
    //***********************************************************************
        if (isDC) {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].loadFtoDDC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->loadFtoD(true);
        }
        else {
            for (uns i = 0; i < nInternalNodesAndVars; i++)
                internalNodesAndVars[i].loadFtoDAC();
            for (auto& comp : components)
                if (comp->isEnabled) comp->loadFtoD(false);
        }
    }
    //***********************  DC Multigrid Functions  **********************
    void solveDC(); // d0 += f0 kell!
    void relaxDC(uns nRelax); // f-et is figyelembe kell venni!
    void prolongateUDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);
    void restrictUDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);
    rvt restrictFDDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);                 // fH = R(fh) + dH  R(dh), ret: truncation error
    void uHMinusRestrictUhToDHNCDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);   // dH_NonConcurrent = uH  R(uh)
    void prolongateDHNCAddToUhDC(const FineCoarseConnectionDescription&, const hmgMultigrid&);     // uh = uh + P(dH_NonConcurrent)
    //***********************  AC Multigrid Functions  **********************
    void solveAC() {} // d0 += f0 kell!
    void relaxAC(uns nRelax) {} // f-et is figyelembe kell venni!
    void prolongeteUAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}
    void restrictUAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}
    rvt restrictFDDAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) { return rvt0; }  // fH = R(fh) + dH  R(dh), ret: truncation error => saját fv kell a re*re+im*im-hez
    void uHMinusRestrictUhToDHNCAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}   // dH_NonConcurrent = uH  R(uh)
    void prolongateDHNCAddToUhAC(const FineCoarseConnectionDescription&, const hmgMultigrid&) {}     // uh = uh + P(dH_NonConcurrent)
    //***********************************************************************
    cplx getCurrentAC(uns y) const noexcept override { return y < externalCurrents.size() ? externalCurrents[y] : cplx0; }
    //***********************************************************************
#ifdef HMG_DEBUGPRINT
    //***********************************************************************
    void printNodeValueDC(uns n) const noexcept override {
    //***********************************************************************
        const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
        for (uns i = 0; i < model.getN_InternalNodes(); i++)
            std::cout << "V (" << n << ")\t[" << i << "] = " << std::setprecision(15) << cutToPrint(internalNodesAndVars[i].getValueDC()) << std::endl;
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
        for (uns i = 0; i < nInternalNodesAndVars; i++) {
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
        cuns Arow = model.getN_X_Nodes();
        cuns Acol = Arow + (isSymm ? 0 : model.getN_Y_Nodes());
        cuns Browcol = model.getN_N_Nodes() + model.getN_O_Nodes();
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
        cuns Arow = model.getN_X_Nodes();
        cuns Acol = Arow + (isSymm ? 0 : model.getN_Y_Nodes());
        cuns Browcol = model.getN_N_Nodes() + model.getN_O_Nodes();
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
    //rvt I = static_cast<ComponentSubCircuit*>(components[compindex2].get())->components[3]->getNode(2)->getValueDC();
    //rvt T = static_cast<ComponentSubCircuit*>(components[compindex2].get())->internalNodesAndVars[0].getValueDC();
    //printf("\n\nI = %.15f\tT = %.15f\n\n", I, T);
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
    if (model.solutionType == SolutionType::stFullMatrix)
        return y < sfmrDC->JRED.size() ? sfmrDC->JRED[y] : rvt0;
    return rvt0;
}
inline rvt ComponentSubCircuit::getYDC(uns y, uns x) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix)
        return y < sfmrDC->YRED.get_row() && x < sfmrDC->YRED.get_col() ? sfmrDC->YRED.get_elem(y, x) : rvt0;
    return rvt0;
}
inline cplx ComponentSubCircuit::getJreducedAC(uns y) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix)
        return y < sfmrAC->JRED.size() ? sfmrAC->JRED[y] : cplx0;
    return cplx0;
}
inline cplx ComponentSubCircuit::getYAC(uns y, uns x) const noexcept {
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix)
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
    if (model.solutionType == SolutionType::stFullMatrix)
        sfmrDC->forwsubs();
    else if (model.solutionType == SolutionType::stSunRed)
        sunred.forwsubsDC();
}


//***********************************************************************
inline void ComponentSubCircuit::backsubsDC() {
// TO PARALLEL
// UA is input in subcircuits, it must be set with setV before calling its backsubs
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix)
        sfmrDC->backsubs();
    else if (model.solutionType == SolutionType::stSunRed)
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
    if (model.solutionType == SolutionType::stFullMatrix)
        sfmrAC->forwsubs();
    else if (model.solutionType == SolutionType::stSunRed)
        sunred.forwsubsAC();
}


//***********************************************************************
inline void ComponentSubCircuit::backsubsAC() {
// TO PARALLEL
// UA is input in subcircuits, it must be set with setV before calling its backsubs
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix)
        sfmrAC->backsubs();
    else if (model.solutionType == SolutionType::stSunRed)
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
inline ComponentAndControllerBase* ModelConstR_1::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstR_1(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstR_2::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstR_2(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstG_1::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstG_1(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstG_2::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstG_2(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstC_1::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstC_1(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstC_2::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstC_2(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstI_1::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstI_1(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstI_2::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstI_2(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstV::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstV(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConst_V_Controlled_I_1::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConst_V_Controlled_I_1(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConst_Controlled_I::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConst_Controlled_I(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelGirator::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentGirator(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstVIB::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstVIB(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelConstVIN::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentConstVIN(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelMIB::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentMIB(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelMIN::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentMIN(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* Model_Function_Controlled_I_with_const_G::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new Component_Function_Controlled_I_with_const_G(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelController::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new Controller(def, defaultNodeValueIndex);
}


//***********************************************************************
inline ComponentAndControllerBase* ModelSubCircuit::makeComponent(const ComponentDefinition* def, uns defaultNodeValueIndex) const {
//***********************************************************************
    return new ComponentSubCircuit(def, defaultNodeValueIndex);
}


//***********************************************************************
class CircuitStorage {
//***********************************************************************
    friend class Simulation;
    friend class ModelSubCircuit;

    //***********************************************************************
    struct Probe {
    //***********************************************************************
        ProbeType probeType = ptV;
        uns fullCircuitID = 0;
        std::vector<DeepCDNodeID> nodes;
    };


    //***********************************************************************
    std::vector<std::unique_ptr<Probe>> probes;
    std::vector<std::unique_ptr<hmgSunred::ReductionTreeInstructions>> sunredTrees;
    std::vector<std::unique_ptr<hmgMultigrid>> multiGrids;
    std::vector<std::unique_ptr<HmgFunction>> functions;
    Simulation sim;
    hmgSaver saver;
    std::thread saverThread;
    //***********************************************************************
public:
    //***********************************************************************
    std::vector<std::unique_ptr<NodeVariable>> globalVariables;
    std::vector<std::unique_ptr<ComponentAndControllerModelBase>> models; // all models are stored globally, controller models also included
    std::vector<std::unique_ptr<ComponentAndControllerModelBase>> builtInModels;
    std::vector<std::unique_ptr<ComponentAndControllerModelBase>> functionControlledBuiltInModels; // core gives the ID number and not the client application / file reader as in case of the custom models
    std::vector<std::unique_ptr<HmgFunction>> internalCustomFunctions;
    //***********************************************************************
    struct FullCircuit { std::unique_ptr<ComponentDefinition> def; std::unique_ptr<ComponentSubCircuit> component; };
    std::vector<FullCircuit> fullCircuitInstances;
    //***********************************************************************

private:

    //***********************************************************************
    void Create_bimtConstL_1(ComponentAndControllerModelBase* dest);
    HmgFunction* Create_function_XDiodeEq();
    NodeConnectionInstructions Create_ConnectionInstructions_XDiode();
    NodeConnectionInstructions Create_ConnectionInstructions_HYS_1();
    std::vector<DefaultNodeParameter> Create_DefaultNodeParameter_HYS_1();
    //***********************************************************************

 
    //***********************************************************************
    CircuitStorage() {
    //***********************************************************************
        HgmFunctionStorage::getInstance(); // HgmFunctionStorage constructor runs
        builtInModels.resize(builtInModelType::bimtSize);
        builtInModels[builtInModelType::bimtConstR_1] = std::make_unique<ModelConstR_1>();
        builtInModels[builtInModelType::bimtConstR_2] = std::make_unique<ModelConstR_2>();
        builtInModels[builtInModelType::bimtConstG_1] = std::make_unique<ModelConstG_1>();
        builtInModels[builtInModelType::bimtConstG_2] = std::make_unique<ModelConstG_2>();
        builtInModels[builtInModelType::bimtConstC_1] = std::make_unique<ModelConstC_1>();
        builtInModels[builtInModelType::bimtConstC_2] = std::make_unique<ModelConstC_2>();
        builtInModels[builtInModelType::bimtConstI_1] = std::make_unique<ModelConstI_1>();
        builtInModels[builtInModelType::bimtConstI_2] = std::make_unique<ModelConstI_2>();
        builtInModels[builtInModelType::bimtConstV] = std::make_unique<ModelConstV>();
        builtInModels[builtInModelType::bimtConst_V_Controlled_I] = std::make_unique<ModelConst_V_Controlled_I_1>();
        builtInModels[builtInModelType::bimtConst_Controlled_I] = std::make_unique<ModelConst_Controlled_I>();
        builtInModels[builtInModelType::bimtGirator] = std::make_unique<ModelGirator>();
        builtInModels[builtInModelType::bimtConstVIB] = std::make_unique<ModelConstVIB>();
        builtInModels[builtInModelType::bimtConstVIN] = std::make_unique<ModelConstVIN>();
        builtInModels[builtInModelType::bimtMIB] = std::make_unique<ModelMIB>();
        builtInModels[builtInModelType::bimtMIN] = std::make_unique<ModelMIN>();
        builtInModels[builtInModelType::bimFunc_Controlled_IG] = std::unique_ptr<Model_Function_Controlled_I_with_const_G>(nullptr);

    /*  ExternalConnectionSizePack
    uns nXNodes = 0;
    uns nYNodes = 0;
    uns nANodes = 0;
    uns nONodes = 0;
    uns nParams = 0;
    uns nComponentT = 0;
        InternalNodeSizePack
    uns nNNodes = 0;
    uns nBNodes = 0;
    */
        
        builtInModels[builtInModelType::bimtConstL_1] = std::make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 2, 0, 0, 0, 1, 0 }, InternalNodeSizePack{ 1, 0 }, false, stFullMatrix);
        Create_bimtConstL_1(builtInModels[builtInModelType::bimtConstL_1].get());

        builtInModels[builtInModelType::bimtXDiode] = std::make_unique<Model_Function_Controlled_I_with_const_G>(1, 0, 2, Create_ConnectionInstructions_XDiode(), std::vector<uns>(), Create_function_XDiodeEq());

        builtInModels[builtInModelType::bimtHYS_1] = std::make_unique<ModelController>(ExternalConnectionSizePack{ 0, 0, 2, 0, 3, 0 }, 0, Create_DefaultNodeParameter_HYS_1(), Create_ConnectionInstructions_HYS_1(), std::vector<uns>(), HgmFunctionStorage::builtInFunctions[bift_HYS_1].get());

        saverThread = std::thread{ hmgSaver::waitToFinish, &saver };
    }
    //***********************************************************************
    const ComponentBase& getComponent(uns fullCircuitID, const DeepCDNodeID& nodeID) const {
    //***********************************************************************
        if (nodeID.isController)
            throw hmgExcept("CircuitStorage::getComponent", "nodeID asks controller");
        const ComponentBase* component = fullCircuitInstances[fullCircuitID].component.get();
        for (uns i = 0; i < nodeID.componentID.size(); i++) {
            const ComponentSubCircuit* subckt = static_cast<const ComponentSubCircuit*>(component);
            component = subckt->components[nodeID.componentID[i]].get();
        }
        return *component;
    }

    //***********************************************************************
    const Controller& getController(uns fullCircuitID, const DeepCDNodeID& nodeID) const {
    //***********************************************************************
        if (!nodeID.isController)
            throw hmgExcept("CircuitStorage::getController", "nodeID asks component");
        const ComponentBase* component = fullCircuitInstances[fullCircuitID].component.get();
        const Controller* controller = nullptr;
        for (uns i = 0; i < nodeID.componentID.size(); i++) {
            const ComponentSubCircuit* subckt = static_cast<const ComponentSubCircuit*>(component);
            if (i + 1 < nodeID.componentID.size())
                component = subckt->components[nodeID.componentID[i]].get();
            else
                controller = subckt->controllers[nodeID.componentID[i]].get();
        }
        return *controller;
    }

    //***********************************************************************
    const NodeVariable& getNode(uns fullCircuitID, const DeepCDNodeID& nodeID) const {
    //***********************************************************************
        if (nodeID.isController) {
            const Controller& controller = getController(fullCircuitID, nodeID);
            switch (nodeID.nodeID.type) {
                case cdntInternal: return controller.mVars[nodeID.nodeID.index];
                case cdntExternal: return *const_cast<Controller&>(controller).getNode(nodeID.nodeID.index);
                default:
                    throw hmgExcept("CircuitStorage::getNode", "impossible probe node type: %u", (uns)nodeID.nodeID.type);
            }
        }
        else {
            const ComponentBase& component = getComponent(fullCircuitID, nodeID);
            switch (nodeID.nodeID.type) {
                case cdntInternal: return static_cast<const ComponentSubCircuit&>(component).internalNodesAndVars[nodeID.nodeID.index];
                case cdntExternal: return *const_cast<ComponentBase&>(component).getNode(nodeID.nodeID.index);
                case cdntRail: return Rails::V[nodeID.nodeID.index].get()->rail;
                default:
                    throw hmgExcept("CircuitStorage::getNode", "impossible probe node type: %u", (uns)nodeID.nodeID.type);
            }
        }
    }

    //***********************************************************************
    rvt getCurrentDC(uns fullCircuitID, const DeepCDNodeID& nodeID) const {
    //***********************************************************************
        if(nodeID.nodeID.type != cdntExternal)
            throw hmgExcept("CircuitStorage::getCurrentDC", "not allowed I probe node type: %u, only X enabled", (uns)nodeID.nodeID.type);
        const ComponentBase& component = getComponent(fullCircuitID, nodeID);
        return component.getCurrentDC(nodeID.nodeID.index);
    }

    //***********************************************************************
    cplx getCurrentAC(uns fullCircuitID, const DeepCDNodeID& nodeID) const {
    //***********************************************************************
        if(nodeID.nodeID.type != cdntExternal)
            throw hmgExcept("CircuitStorage::getCurrentDC", "not allowed I probe node type: %u, only X enabled", (uns)nodeID.nodeID.type);
        const ComponentBase& component = getComponent(fullCircuitID, nodeID);
        return component.getCurrentAC(nodeID.nodeID.index);
    }

    //***********************************************************************
    void getProbeValuesDC(const Probe& probe, std::vector<rvt>& saveValuesDC) const {
    //***********************************************************************
        switch (probe.probeType) {
            case ptV: {
                saveValuesDC.reserve(saveValuesDC.size() + probe.nodes.size());
                for (const auto& nodeID : probe.nodes)
                    saveValuesDC.push_back(getNode(probe.fullCircuitID, nodeID).getValueDC());
            }
            break;
            case ptI: {
                saveValuesDC.reserve(saveValuesDC.size() + probe.nodes.size());
                for (const auto& nodeID : probe.nodes)
                    saveValuesDC.push_back(getCurrentDC(probe.fullCircuitID, nodeID));
            }
            break;
            case ptVSum: {
                rvt sum = rvt0;
                for (const auto& nodeID : probe.nodes)
                    sum += getNode(probe.fullCircuitID, nodeID).getValueDC();
                saveValuesDC.push_back(sum);
            }
            break;
            case ptVAverage: {
                rvt sum = rvt0;
                for (const auto& nodeID : probe.nodes)
                    sum += getNode(probe.fullCircuitID, nodeID).getValueDC();
                saveValuesDC.push_back(sum / probe.nodes.size());
            }
            break;
            case ptISum: {
                rvt sum = rvt0;
                for (const auto& nodeID : probe.nodes)
                    sum += getCurrentDC(probe.fullCircuitID, nodeID);
                saveValuesDC.push_back(sum);
            }
            break;
            case ptIAverage: {
                rvt sum = rvt0;
                for (const auto& nodeID : probe.nodes)
                    sum += getCurrentDC(probe.fullCircuitID, nodeID);
                saveValuesDC.push_back(sum / probe.nodes.size());
            }
            break;
            default:
                throw hmgExcept("CircuitStorage::getProbeValuesDC", "unknown probe Type: %u", (uns)probe.probeType);
        }
    }

    //***********************************************************************
    void getProbeValuesAC(const Probe& probe, std::vector<cplx>& saveValuesAC) const {
    //***********************************************************************
        switch (probe.probeType) {
            case ptV: {
                saveValuesAC.reserve(saveValuesAC.size() + probe.nodes.size());
                for (const auto& nodeID : probe.nodes)
                    saveValuesAC.push_back(getNode(probe.fullCircuitID, nodeID).getValueAC());
            }
            break;
            case ptI: {
                saveValuesAC.reserve(saveValuesAC.size() + probe.nodes.size());
                for (const auto& nodeID : probe.nodes)
                    saveValuesAC.push_back(getCurrentAC(probe.fullCircuitID, nodeID));
            }
            break;
            case ptVSum: {
                cplx sum = cplx0;
                for (const auto& nodeID : probe.nodes)
                    sum += getNode(probe.fullCircuitID, nodeID).getValueAC();
                saveValuesAC.push_back(sum);
            }
            break;
            case ptVAverage: {
                cplx sum = cplx0;
                for (const auto& nodeID : probe.nodes)
                    sum += getNode(probe.fullCircuitID, nodeID).getValueAC();
                saveValuesAC.push_back(sum / (rvt)probe.nodes.size());
            }
            break;
            case ptISum: {
                cplx sum = cplx0;
                for (const auto& nodeID : probe.nodes)
                    sum += getCurrentAC(probe.fullCircuitID, nodeID);
                saveValuesAC.push_back(sum);
            }
            break;
            case ptIAverage: {
                cplx sum = cplx0;
                for (const auto& nodeID : probe.nodes)
                    sum += getCurrentAC(probe.fullCircuitID, nodeID);
                saveValuesAC.push_back(sum / (rvt)probe.nodes.size());
            }
            break;
            default:
                throw hmgExcept("CircuitStorage::getProbeValuesAC", "unknown probe Type: %u", (uns)probe.probeType);
        }
    }

    //***********************************************************************
    void save(bool isRaw, bool isAppend, uns maxResultsPerRow, const char* fileName, const std::vector<uns>& probeIndex) {
    //***********************************************************************
        SimulationToSaveData* src = new SimulationToSaveData;
        src->isRaw = isRaw;
        src->isAppend = isAppend;
        src->maxResultsPerRow = maxResultsPerRow;
        src->fileName = fileName;
        sim.fillSaveData(src);
        switch (src->analysisType) {
            case atDC:
            case atTimeStep: {
                for (auto pi : probeIndex)
                    getProbeValuesDC(*probes[pi].get(), src->saveValuesDC);
            }
            break;
            case atAC:
            case atTimeConst: {
                for (auto pi : probeIndex)
                    getProbeValuesAC(*probes[pi].get(), src->saveValuesAC);
            }
            break;
            default:
                throw hmgExcept("CircuitStorage::save", "unknown analysis Type: %u", (uns)src->analysisType);
        }
        saver.addSimulationToSaveData(src);
    }

    //***********************************************************************
    void processSunredTreeInstructions(IsInstruction*& first, uns currentTree);
    void processMultigridInstructions(IsInstruction*& first, uns currentMg);
    void processRailsInstructions(IsInstruction*& first);
    void processProbesInstructions(IsInstruction*& first, uns currentProbe);
    void processSaveInstructions(IsInstruction*& first, std::vector<uns>& probeIndex);
    void processFunctionInstructions(IsInstruction*& first, uns functionIndex, uns nComponentParams, uns nParams, uns nVars, uns nCallInstructions);
    //***********************************************************************

public:
    CircuitStorage(const CircuitStorage&) = delete;
    CircuitStorage& operator=(const CircuitStorage&) = delete;

    //***********************************************************************
    ~CircuitStorage() {
    //***********************************************************************
        saver.finish();
        saverThread.join();
    }

    //***********************************************************************
    static CircuitStorage& getInstance() { // singleton
    //***********************************************************************
        static CircuitStorage instance;
        return instance;
    }
    //***********************************************************************
    void createFullCircuit(uns componentModelIndex, uns nodesDefaultValueIndex, uns fullCircuitIndexToCheck) {
    //***********************************************************************
        if(fullCircuitInstances.size() != fullCircuitIndexToCheck)
            throw hmgExcept("CircuitStorage::createFullCircuit", "Full circuits must be defined in order: fullCircuitIndex = %u expected, %u arrived.", (uns)fullCircuitInstances.size(), fullCircuitIndexToCheck);
        FullCircuit fc;
        fc.def = std::make_unique<ComponentDefinition>();
        fc.def->modelIndex = componentModelIndex;
        fc.component = std::unique_ptr<ComponentSubCircuit>(dynamic_cast<ComponentSubCircuit*>(static_cast<ComponentBase*>(models[componentModelIndex]->makeComponent(fc.def.get(), nodesDefaultValueIndex))));
        fullCircuitInstances.push_back(std::move(fc));
        fullCircuitInstances.back().component->buildOrReplace();
    }
    //***********************************************************************
    bool processInstructions(IsInstruction*& first);
    //***********************************************************************
 
    //***********************************************************************
    static void CalculateControllersDC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        uns currentLevel = 0;
        uns nextLevel = 0;
        do {
            currentLevel = nextLevel;
            nextLevel = gc.fullCircuitInstances[fullCircuitID].component->calculateControllersDC(currentLevel, cosLoad);
            gc.fullCircuitInstances[fullCircuitID].component->calculateControllersDC(currentLevel, cosEvaluateAndStore);
        } while (nextLevel > currentLevel);
    }

    //***********************************************************************
    static void CalculateValuesAndCurrentsDC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->calculateValueDC();
        gc.fullCircuitInstances[fullCircuitID].component->deleteD(true);
        gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(true);
    }
    //***********************************************************************
    static void ForwsubsBacksubsDC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->forwsubs(true);
        gc.fullCircuitInstances[fullCircuitID].component->backsubs(true);
    }
    //***********************************************************************
    static void AcceptIterationDC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->acceptIterationDC(true);
    }
    //***********************************************************************
    static void AcceptStepDC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->acceptStepDC();
    }

    //***********************************************************************
    static void CalculateValuesAndCurrentsAC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->deleteD(false);
        gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(false);
    }
    //***********************************************************************
    static void ForwsubsBacksubsAC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->forwsubs(false);
        gc.fullCircuitInstances[fullCircuitID].component->backsubs(false);
    }
    //***********************************************************************
    static void AcceptIterationAC(uns fullCircuitID) {
    // no iterations in AC => do nothing
    //***********************************************************************

    }
    //***********************************************************************
    static void AcceptStepAC(uns fullCircuitID) {
    //***********************************************************************
        CircuitStorage& gc = getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->acceptIterationAndStepAC();
    }
};


//***********************************************************************
inline ComponentAndControllerBase::ComponentAndControllerBase(const ComponentDefinition* def_, uns defaultNodeValueIndex_) :def{ def_ },
    pModel{ static_cast<ComponentAndControllerModelBase*>(
        def_->modelType == cmtBuiltIn  
            ? CircuitStorage::getInstance().builtInModels[def_->modelIndex].get()
            : (
                def_->modelType == cmtCustom 
                    ? CircuitStorage::getInstance().models[def_->modelIndex].get()
                    : CircuitStorage::getInstance().functionControlledBuiltInModels[def_->modelIndex].get()
            )
        ) }, defaultNodeValueIndex{ defaultNodeValueIndex_ } {
//***********************************************************************
    if (pModel->canBeNonlinear())
        SimControl::nNonlinComponents++;
}
//***********************************************************************




}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
