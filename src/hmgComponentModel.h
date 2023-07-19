//***********************************************************************
// HexMG component descriptor classes
// Creation date:  2023. 01. 27.
// Creator:        Pohl L�szl�
//***********************************************************************


//***********************************************************************
#ifndef HMG_COMPONENT_MODEL_HEADER
#define	HMG_COMPONENT_MODEL_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgInstructionStream.h"
#include "hmgCircuitNode.hpp"
#include "hmgFunction.hpp"
#include "hmgSunred.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class ComponentAndControllerBase;
class ComponentDefinition;
//***********************************************************************


//***********************************************************************
class ComponentAndControllerModelBase {
// Type definition of the component, this is instantiated as Component
// e.g. a subcircuit description or built in types (R, C, etc.)
//***********************************************************************
public:
    //***********************************************************************
    // number of nodes
    // these cannot be changed, this is the interface of a component
    const ExternalConnectionSizePack externalNs;
    const ComponentAndControllerModelType modelType;
    //***********************************************************************

    ComponentAndControllerModelBase(ExternalConnectionSizePack Ns_, ComponentAndControllerModelType modelType_)
        : externalNs{ Ns_ }, modelType{ modelType_ } {}
    virtual ~ComponentAndControllerModelBase() = default;
    virtual ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const = 0;
    uns getN_IO_Nodes()const noexcept { return externalNs.nIONodes; }
    uns getN_Normal_I_Nodes()const noexcept { return externalNs.nNormalINodes; }
    uns getN_Control_I_Nodes()const noexcept { return externalNs.nControlINodes; }
    uns getN_Normal_O_Nodes()const noexcept { return externalNs.nNormalONodes; }
    uns getN_Forwarded_O_Nodes()const noexcept { return externalNs.nForwardedONodes; }
    uns getN_O_Nodes()const noexcept { return externalNs.nNormalONodes + externalNs.nForwardedONodes; }
    uns getN_Start_Of_O_Nodes()const noexcept { return externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes; }
    uns getN_ExternalNodes()const noexcept { return externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes + externalNs.nNormalONodes + externalNs.nForwardedONodes; }
    uns getN_Params()const noexcept { return externalNs.nParams; }
    virtual uns getN_NormalInternalNodes()const noexcept { return 0; }
    virtual uns getN_InternalNodes()const noexcept { return 0; }
    bool isController()const noexcept { return modelType == ccmt_Controller; }
    virtual bool canBeNonlinear()const noexcept = 0;
    bool SimpleInterfaceNodeIDToCDNode(CDNode& dest, const SimpleInterfaceNodeID& src)const noexcept;
};


class ModelSubCircuit;


 //***********************************************************************
class ComponentDefinition final {
// Instantiation instruction for a Component
//***********************************************************************
public:
    bool isDefaultRail = false;
    componentModelType modelType = cmtCustom;
    uns modelIndex = 0; // in CircuitStorage::models or CircuitStorage::builtInModels
    uns defaultValueRailIndex = 0;
    std::vector<CDNode> nodesConnectedTo;
    std::vector<CDParam> params;
    std::vector<ComponentIndex> componentParams;

    void setDefaultValueRailIndex(uns defaultValueRailIndex_) noexcept { defaultValueRailIndex = defaultValueRailIndex_; isDefaultRail = true; }
    void processInstructions(IsInstruction*& first, const ModelSubCircuit& container);
};


//***********************************************************************
class ModelConstR_1 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstR_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 1, 0 }, ccmt_ConstR_1 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstR_2 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstR_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 2, 0 }, ccmt_ConstR_2 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstG_1 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstG_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 1, 0 }, ccmt_ConstG_1 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstG_2 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstG_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 2, 0 }, ccmt_ConstG_2 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstC_1 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstC_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 1, 0 }, ccmt_ConstC_1 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstC_2 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstC_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 2, 0 }, ccmt_ConstC_2 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstI_1 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstI_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 4, 0 }, ccmt_ConstI_1 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstI_2 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstI_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 5, 0 }, ccmt_ConstI_2 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConst_V_Controlled_I_1 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConst_V_Controlled_I_1() :ComponentAndControllerModelBase{ { 2, 2, 0, 0, 0, 4, 0 }, ccmt_Const_V_Controlled_I_1 } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConst_Controlled_I final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConst_Controlled_I() :ComponentAndControllerModelBase{ { 2, 0, 1, 0, 0, 1, 0 }, ccmt_ConstIC } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelGirator final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelGirator() :ComponentAndControllerModelBase{ { 4, 0, 0, 0, 0, 2, 0 }, ccmt_Girator } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelConstVI final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstVI() :ComponentAndControllerModelBase{ { 2, 0, 0, 1, 0, 5, 0 }, ccmt_ConstVI } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    uns getN_NormalInternalNodes()const noexcept final override { return 1; }
    uns getN_InternalNodes()const noexcept final override { return 1; }
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class Model_Function_Controlled_I_with_const_G final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    NodeConnectionInstructions functionSources;
    std::vector<uns> functionComponentParams;   // unsMax means _THIS
    HmgFunction* controlFunction = nullptr;
    std::vector<uns> indexField;
    std::vector<uns> nodeToFunctionParam; // for Yij => workField[index[nodeToFunctionParam[i]]] += dx;
    Model_Function_Controlled_I_with_const_G(uns nNormalINodes_, uns nControlINodes_, uns nParams_,
        NodeConnectionInstructions functionSources_, std::vector<uns>&& functionComponentParams_, HmgFunction* controlFunction_)
        :ComponentAndControllerModelBase{ { 2, nNormalINodes_, nControlINodes_, 0, 0, nParams_, 0 }, ccmt_Function_Controlled_I_with_const_G },
            functionSources{ std::move(functionSources_) }, functionComponentParams{ std::move(functionComponentParams_) }, controlFunction{ controlFunction_ } {
            indexField.resize(controlFunction->getN_IndexField());
            indexField[0] = 0;
            indexField[1] = controlFunction->getN_Param() + 1;
            for (uns i = 0; i < controlFunction->getN_Param(); i++)
                indexField[i + 2] = i + 1;
            controlFunction->fillIndexField(&indexField[0]);
            nodeToFunctionParam.resize(getN_IO_Nodes() + getN_Normal_I_Nodes()); // ! different in controller
            for (auto& val : nodeToFunctionParam)
                val = unsMax;
            for (uns i = 0; i < functionSources.sources.size(); i++) {
                const auto& src = functionSources.sources[i];
                if (src.sourceType == NodeConnectionInstructions::SourceType::sExternalNodeValue && src.sourceIndex < nodeToFunctionParam.size())
                    nodeToFunctionParam[src.sourceIndex] = i + 2;
            }
    }
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return true; }
};


//***********************************************************************
class Controller;
//***********************************************************************


//***********************************************************************
class ModelController final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    NodeConnectionInstructions functionSources;
    HmgFunction* controlFunction = nullptr;
    std::vector<uns> indexField;
    uns nMVars;
    ModelController(uns nControlINodes_, uns nControlONodes_, uns nMVars_, uns nParams_,
        NodeConnectionInstructions functionSources_, HmgFunction* controlFunction_)
        :ComponentAndControllerModelBase{ { 0, 0, nControlINodes_, nControlONodes_, 0, nParams_, 0 }, ccmt_Controller }, // O nodes of the Controller are defined as normal O nodes
        nMVars{ nMVars_ }, functionSources { std::move(functionSources_) }, controlFunction{ controlFunction_ } {
            indexField.resize(controlFunction->getN_IndexField());
            indexField[0] = 0;
            indexField[1] = controlFunction->getN_Param() + 1;
            for (uns i = 0; i < controlFunction->getN_Param(); i++)
                indexField[i + 2] = i + 1;
            controlFunction->fillIndexField(&indexField[0]);
        }
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    bool canBeNonlinear()const noexcept override { return false; }
};


//***********************************************************************
class ModelSubCircuit final : public ComponentAndControllerModelBase {
//***********************************************************************
    friend class ComponentAndControllerModelBase;
    friend class ComponentSubCircuit;
    friend class CircuitStorage;
    friend class ComponentDefinition;
    friend struct FineCoarseConnectionDescription;
    //***********************************************************************

    //***********************************************************************
    // number of internal nodes and nInternalVars can be changed in the replace procedure
    InternalNodeVarSizePack internalNs;
    //***********************************************************************
    uns version = 1; // version is increased with each change in the structure of the subcircuit => the component instance can check that it is up to date
    //***********************************************************************
    std::vector<bool> internalNodeIsConcurrent; // if true, the defect type will be atomic (slower)
    std::vector<ForcedNodeDef> forcedNodes; // internal nodes where the default value index is defined
    std::vector<std::unique_ptr<ComponentDefinition>> components;
    std::vector<std::unique_ptr<ComponentDefinition>> controllers;
    hmgSunred::ReductionTreeInstructions* srTreeInstructions;
    //***********************************************************************

private:
    //***********************************************************************
    SolutionType solutionType;
    //***********************************************************************
public:
    //***********************************************************************
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    //***********************************************************************

    //***********************************************************************
    ModelSubCircuit(ExternalConnectionSizePack Ns_, InternalNodeVarSizePack internalNs_, bool defaultInternalNodeIsConcurrent_, SolutionType solutionType_, hmgSunred::ReductionTreeInstructions* srTree = nullptr)
        : ComponentAndControllerModelBase{ Ns_, ccmt_SubCircuit }, internalNs{ internalNs_ }, solutionType{ solutionType_ }, srTreeInstructions{ srTree } {
    //***********************************************************************
        internalNodeIsConcurrent.resize(internalNs_.nNormalInternalNodes, defaultInternalNodeIsConcurrent_);
    }
    //***********************************************************************
    uns getN_NormalInternalNodes()const noexcept final override{ return internalNs.nNormalInternalNodes; }
    uns getN_InternalNodes()const noexcept final override{ return internalNs.nNormalInternalNodes + internalNs.nControlInternalNodes; }
    uns getN_Control_Internal_Nodes()const noexcept { return internalNs.nControlInternalNodes; }
    //***********************************************************************
    void setNInternalVars(uns n) noexcept { version++; internalNs.nInternalVars = n; }
    void setNInternalNodes(uns nAll, uns nNormal, bool defaultInternalNodeIsConcurrent) { version++; internalNs.nControlInternalNodes = nAll - nNormal; internalNs.nNormalInternalNodes = nNormal; internalNodeIsConcurrent.resize(nNormal, defaultInternalNodeIsConcurrent); }
    //***********************************************************************
    uns push_back_component(std::unique_ptr<ComponentDefinition> ptr) { version++; components.push_back(std::move(ptr)); return uns(components.size()); }
    void setComponent(uns index, std::unique_ptr<ComponentDefinition> ptr) noexcept { version++; components[index] = std::move(ptr); }
    void resizeComponents(uns size) { version++; components.resize(size); }
    void clearComponensts() noexcept { version++; components.clear(); }
    //***********************************************************************
    uns push_back_controller(std::unique_ptr<ComponentDefinition> ptr) { version++; controllers.push_back(std::move(ptr)); return uns(controllers.size()); }
    void setController(uns index, std::unique_ptr<ComponentDefinition> ptr) noexcept { version++; controllers[index] = std::move(ptr); }
    void resizeControllers(uns size) { version++; controllers.resize(size); }
    void clearControllers() noexcept { version++; controllers.clear(); }
    //***********************************************************************
    uns getVersion()const noexcept { return version; }
    void processInstructions(IsInstruction*& first);
    //***********************************************************************
    bool canBeNonlinear()const noexcept override { return false; } // can contain nonlinear components but this is just a container
    //***********************************************************************
};


//***********************************************************************
inline bool ComponentAndControllerModelBase::SimpleInterfaceNodeIDToCDNode(CDNode& dest, const SimpleInterfaceNodeID& src)const noexcept{
//***********************************************************************
    uns delta = 0;
    switch (src.type) {
        case nvtIO:
            dest.type = CDNodeType::cdntExternal;
            break;
        case nvtIN:
            dest.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes;
            break;
        case nvtCIN:
            dest.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes + externalNs.nNormalINodes;
            break;
        case nvtOUT:
            dest.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes;
            break;
        case nvtFWOUT:
            dest.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes + externalNs.nNormalONodes;
            break;
        case nvtNInternal:
            if (modelType != ccmt_SubCircuit)
                return false;
            dest.type = CDNodeType::cdntInternal;
            break;
        case nvtCInternal:
            if (modelType != ccmt_SubCircuit)
                return false;
            dest.type = CDNodeType::cdntInternal;
            delta = static_cast<const ModelSubCircuit*>(this)->internalNs.nNormalInternalNodes;
            break;
        case nvtVarInternal:
            if (modelType != ccmt_SubCircuit && modelType != ccmt_Controller)
                return false;
            dest.type = CDNodeType::cdntVar;
            if (modelType == ccmt_SubCircuit)
                delta = static_cast<const ModelSubCircuit*>(this)->internalNs.nNormalInternalNodes + static_cast<const ModelSubCircuit*>(this)->internalNs.nControlInternalNodes;
            // controllers have only internal vars, no internal nodes => delta = 0.
            break;
        case nvtRail:
            dest.type = CDNodeType::cdntRail;
            break;
        case nvtGND:
            dest.type = CDNodeType::cdntGnd;
            break;
        case nvtUnconnected:
            dest.type = CDNodeType::cdntUnconnected;
            break;
        default:
            return false;
    }
    dest.index = src.index + delta;
    return true;
}



//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
