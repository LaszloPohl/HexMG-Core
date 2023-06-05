//***********************************************************************
// HexMG component descriptor classes
// Creation date:  2023. 01. 27.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_COMPONENT_MODEL_HEADER
#define	HMG_COMPONENT_MODEL_HEADER
//***********************************************************************


//***********************************************************************
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
    //***********************************************************************
    // number of nodes
    // these cannot be changed, this is the interface of a component
    const ExternalConnectionSizePack Ns;
    //***********************************************************************
public:
    ComponentAndControllerModelBase(ExternalConnectionSizePack Ns_)
        : Ns{ Ns_ } {}
    virtual ~ComponentAndControllerModelBase() = default;
    virtual ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const = 0;
    uns getN_IO_Nodes()const noexcept { return Ns.nIONodes; }
    uns getN_Normal_I_Nodes()const noexcept { return Ns.nNormalINodes; }
    uns getN_Control_I_Nodes()const noexcept { return Ns.nControlINodes; }
    uns getN_Normal_O_Nodes()const noexcept { return Ns.nNormalONodes; }
    uns getN_Forwarded_O_Nodes()const noexcept { return Ns.nForwardedONodes; }
    uns getN_O_Nodes()const noexcept { return Ns.nNormalONodes + Ns.nForwardedONodes; }
    uns getN_Start_Of_O_Nodes()const noexcept { return Ns.nIONodes + Ns.nNormalINodes + Ns.nControlINodes; }
    uns getN_ExternalNodes()const noexcept { return Ns.nIONodes + Ns.nNormalINodes + Ns.nControlINodes + Ns.nNormalONodes + Ns.nForwardedONodes; }
    uns getN_Params()const noexcept { return Ns.nParams; }
    virtual uns getN_NormalInternalNodes()const noexcept { return 0; }
    virtual uns getN_InternalNodes()const noexcept { return 0; }
    bool isController()const noexcept { return Ns.nIONodes + Ns.nNormalINodes == 0; }
};


 //***********************************************************************
class ComponentDefinition final {
// Instantiation instruction for a Component
//***********************************************************************
public:

    enum CDNodeType{ internal, external, ground, unconnected }; // unconnected: only for ONodes
    struct CDNode { CDNodeType type = external; uns index = 0; };
    enum CDParamType{ value, globalVariable, localVariable, param, internalNode, externalNode };
    struct CDParam { CDParamType type = CDParamType::value; uns index = 0; rvt value = rvt0; };

    bool isBuiltIn = false, isForcedDefNodeValueIndex = false;
    uns componentModelIndex = 0; // in CircuitStorage::models or CircuitStorage::builtInModels
    uns nodesDefValueIndex = 0;
    std::vector<CDNode> nodesConnectedTo;
    std::vector<CDParam> params;

    void setNodesDefValueIndex(uns nodesDefValueIndex_) noexcept { nodesDefValueIndex = nodesDefValueIndex_; isForcedDefNodeValueIndex = true; }
};


//***********************************************************************
class ModelConstR_1 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstR_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 1 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstR_2 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstR_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 2 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstG_1 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstG_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 1 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstG_2 final : public ComponentAndControllerModelBase {
// Resistor value is G!
//***********************************************************************
public:
    ModelConstG_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 2 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstC_1 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstC_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 1 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstC_2 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstC_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 2 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstI_1 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstI_1() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 4 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstI_2 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstI_2() :ComponentAndControllerModelBase{ { 2, 0, 0, 0, 0, 5 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConst_V_Controlled_I_1 final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConst_V_Controlled_I_1() :ComponentAndControllerModelBase{ { 2, 2, 0, 0, 0, 4 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelGirator final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelGirator() :ComponentAndControllerModelBase{ { 4, 0, 0, 0, 0, 2 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelConstV final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    ModelConstV() :ComponentAndControllerModelBase{ { 2, 0, 0, 1, 0, 5 } } {}
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
    uns getN_NormalInternalNodes()const noexcept final override { return 1; }
    uns getN_InternalNodes()const noexcept final override { return 1; }
};


//***********************************************************************
class Model_Function_Controlled_I_with_const_G final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    NodeConnectionInstructions functionSources;
    std::unique_ptr<HmgFunction> controlFunction;
    std::vector<uns> indexField;
    std::vector<uns> nodeToFunctionParam; // for Yij => workField[index[nodeToFunctionParam[i]]] += dx;
    Model_Function_Controlled_I_with_const_G(uns nNormalINodes_, uns nControlINodes_, uns nParams_,
        NodeConnectionInstructions functionSources_, std::unique_ptr<HmgFunction> controlFunction_)
        :ComponentAndControllerModelBase{ { 2, nNormalINodes_, nControlINodes_, 0, 0, nParams_ } },
        functionSources{ std::move(functionSources_) }, controlFunction{ std::move(controlFunction_) } {
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
};


//***********************************************************************
class Controller;
//***********************************************************************


//***********************************************************************
class ModelController final : public ComponentAndControllerModelBase {
//***********************************************************************
public:
    NodeConnectionInstructions functionSources;
    std::unique_ptr<HmgFunction> controlFunction;
    std::vector<uns> indexField;
    uns nMVars;
    ModelController(uns nControlINodes_, uns nControlONodes_, uns nMVars_, uns nParams_,
        NodeConnectionInstructions functionSources_, std::unique_ptr<HmgFunction> controlFunction_)
        :ComponentAndControllerModelBase{ { 0, 0, nControlINodes_, nControlONodes_, 0, nParams_ } }, // O nodes of the Controller are defined as normal O nodes
        nMVars{ nMVars_ }, functionSources {
        std::move(functionSources_)
    }, controlFunction{ std::move(controlFunction_) } {
            indexField.resize(controlFunction->getN_IndexField());
            indexField[0] = 0;
            indexField[1] = controlFunction->getN_Param() + 1;
            for (uns i = 0; i < controlFunction->getN_Param(); i++)
                indexField[i + 2] = i + 1;
            controlFunction->fillIndexField(&indexField[0]);
        }
    ComponentAndControllerBase* makeComponent(const ComponentDefinition*, uns defaultNodeValueIndex) const override; // definition in hmgComponent.h
};


//***********************************************************************
class ModelSubCircuit final : public ComponentAndControllerModelBase {
//***********************************************************************
    friend class ComponentSubCircuit;
//***********************************************************************

    //***********************************************************************
    // number of internal nodes and nInternalVars can be changed in the replace procedure
    InternalNodeVarSizePack internalNs;
    //***********************************************************************
    uns version = 1; // version is increased with each change in the structure of the subcircuit => the component instance can check that it is up to date
    //***********************************************************************
    std::vector<bool> internalNodeIsConcurrent; // if true, the defect type will be atomic (slower)
    struct ForcedNodeDef { uns nodeIndex = 0, defaultValueIndex = 0; bool isExternal = false; };
    std::vector<ForcedNodeDef> forcedNodes; // internal nodes where the default value index is defined
    std::vector<std::unique_ptr<ComponentDefinition>> components;
    std::vector<std::unique_ptr<ComponentDefinition>> controllers;
    hmgSunred::ReductionTreeInstructions srTreeInstructions;
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
        : ComponentAndControllerModelBase{ Ns_ }, internalNs{ internalNs_ }, solutionType{solutionType_} {
    //***********************************************************************
       internalNodeIsConcurrent.resize(internalNs_.nNormalInternalNodes, defaultInternalNodeIsConcurrent_);
       if (srTree)
           srTreeInstructions = std::move(*srTree);
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
    //***********************************************************************
};


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
