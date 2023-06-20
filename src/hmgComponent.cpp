//***********************************************************************
// HexMG component instance (cell) classes cpp
// Creation date:  2023. 01. 27.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#include "hmgComponent.h"
#include "hmgMultigrid.hpp"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
bool CircuitStorage::processInstructions(IsInstruction*& first) {
//***********************************************************************
    bool isNotFinished = true;
    bool isImpossibleInstruction = false;
    while (first != nullptr) {
        IsInstruction* act = first;
        first = first->next;
        switch (act->instruction) {
            case sitNothing:                        break;
            case sitCreate: {
                    IsCreateInstruction* pAct = static_cast<IsCreateInstruction*>(act);
                    createFullCircuit(pAct->modelID, pAct->GND, pAct->fullCircuitIndex);
                    fullCircuitInstances[pAct->fullCircuitIndex].component->resetNodes(true);
                }
                break;
            case sitSave: {
                    IsSaveInstruction* pAct = static_cast<IsSaveInstruction*>(act);
                    std::vector<uns> probeIndex;
                    processSaveInstructions(first, probeIndex);
                    save(pAct->isRaw, pAct->isAppend, pAct->maxResultsPerRow, pAct->fileName, probeIndex);
                }
                break;
            case sitDefModelSubcircuit: {
                    IsDefModelSubcircuitInstruction* pAct = static_cast<IsDefModelSubcircuitInstruction*>(act);
                    
                    if (pAct->isReplace) {
                        ModelSubCircuit* ms = static_cast<ModelSubCircuit*>(models[pAct->index].get());
                        uns version = ms->version + 1;
                        models[pAct->index] = std::make_unique<ModelSubCircuit>(ms->externalNs, ms->internalNs, ms->solutionType != SolutionType::stFullMatrix, ms->solutionType, ms->srTreeInstructions);
                        ms = static_cast<ModelSubCircuit*>(models[pAct->index].get());
                        ms->version = version;
                    }
                    else {
                        if (pAct->index == models.size()) {
                            models.push_back(std::make_unique<ModelSubCircuit>(pAct->externalNs, pAct->internalNs,
                                pAct->solutionType != SolutionType::stFullMatrix, pAct->solutionType, pAct->solutionType == SolutionType::stSunRed ? sunredTrees[pAct->solutionDescriptionIndex].get() : nullptr));
                        }
                        else {
                            if (pAct->index > models.size())
                                models.resize(pAct->index + 1);
                            models[pAct->index] = std::make_unique<ModelSubCircuit>(pAct->externalNs, pAct->internalNs,
                                pAct->solutionType != SolutionType::stFullMatrix, pAct->solutionType, pAct->solutionType == SolutionType::stSunRed ? sunredTrees[pAct->solutionDescriptionIndex].get() : nullptr);
                        }
                    }

                    ModelSubCircuit* ms = static_cast<ModelSubCircuit*>(models[pAct->index].get());

                    ms->processInstructions(first);
                }
                break;
            case sitDefModelController: {
                    
                }
                break;
            case sitComponentInstance:              isImpossibleInstruction = true; break;
            case sitSunredTree: {
                    IsDefSunredInstruction* pAct = static_cast<IsDefSunredInstruction*>(act);

                    // pAct->isReplace can be ignored

                    if (pAct->index == sunredTrees.size()) {
                        sunredTrees.push_back(std::make_unique<hmgSunred::ReductionTreeInstructions>());
                    }
                    else  {
                        if (pAct->index > sunredTrees.size())
                            sunredTrees.resize(pAct->index + 1);
                        sunredTrees[pAct->index] = std::make_unique<hmgSunred::ReductionTreeInstructions>();
                    }

                    sunredTrees[pAct->index]->data.resize(pAct->levels);

                    processSunredTreeInstructions(first, pAct->index);
                }
                break;
            case sitSunredLevel:                    isImpossibleInstruction = true; break;
            case sitSunredReduction:                isImpossibleInstruction = true; break;
            case sitRails: {
                    IsDefRailsInstruction* pAct = static_cast<IsDefRailsInstruction*>(act);
                    Rails::resize(pAct->nRailValues + 1);
                    processRailsInstructions(first);
                    Rails::reset();
                }
                break;
            case sitRailRange:                      isImpossibleInstruction = true; break;
            case sitNodeValue:                      isImpossibleInstruction = true; break;
            case sitParameterValue:                 isImpossibleInstruction = true; break;
            case sitProbe: {
                    IsProbeInstruction* pAct = static_cast<IsProbeInstruction*>(act);
                    if (pAct->probeIndex == probes.size()) {
                        probes.push_back(std::make_unique<Probe>(Probe()));
                    }
                    else {
                        if (pAct->probeIndex > probes.size())
                            probes.resize(pAct->probeIndex + 1);
                        probes[pAct->probeIndex] = std::make_unique<Probe>(Probe());
                    }
                    probes[pAct->probeIndex]->probeType = (ProbeType)pAct->probeType;
                    probes[pAct->probeIndex]->fullCircuitID = pAct->fullCircuitID;
                    probes[pAct->probeIndex]->nodes.reserve(pAct->nNodes);
                    processProbesInstructions(first, pAct->probeIndex);
                }
                break;
            case sitProbeNode:                      isImpossibleInstruction = true; break;
            case sitRun: {
                    IsRunInstruction* pAct = static_cast<IsRunInstruction*>(act);
                    sim.run(pAct->data);
                }
                break;
            case sitFunction: {
                    TODO("sitFunction");
                }
                break;
            case sitExpressionAtom:                 isImpossibleInstruction = true; break;
            case sitUns:                            isImpossibleInstruction = true; break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitNothing)
                        throw hmgExcept("CircuitStorage::processInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("CircuitStorage::processInstructions", "unknown instruction type (%u)", act->instruction);
        }
        if(isImpossibleInstruction)
            throw hmgExcept("CircuitStorage::processInstructions", "%u is not a global instruction", act->instruction);
        delete act;
        if(!isNotFinished && first != nullptr)
            throw hmgExcept("CircuitStorage::processInstructions", "instruction after End Simulation instruction");
    }
    return isNotFinished;
}


//***********************************************************************
void CircuitStorage::processSunredTreeInstructions(IsInstruction*& first, uns currentTree) {
//***********************************************************************
    bool isNotFinished = true;
    if (isNotFinished && first == nullptr)
        throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "The instruction stream has ended during sunred tree definition.");
    uns currentLevel = unsMax, nReductions = 0;
    while (isNotFinished) {
        IsInstruction* act = first;
        first = first->next;
        if (act->instruction != sitSunredReduction && currentLevel != unsMax) {
            if(nReductions != sunredTrees[currentTree]->data[currentLevel].size())
                throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "Not enough or too many reductions in Tree %u Level %u (%u expected, %u arrived)", 
                    currentTree, currentLevel, nReductions, (uns)sunredTrees[currentTree]->data[currentLevel].size());
        }
        switch (act->instruction) {
            case sitNothing: break;
            case sitSunredLevel: {//instr.data[0].resize(2);
                    IsDefSunredLevelInstruction* pAct = static_cast<IsDefSunredLevelInstruction*>(act);
                    if(pAct->level == 0 || pAct->level > sunredTrees[currentTree]->data.size())
                        throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "illegal destination level (%u)", pAct->level);
                    currentLevel = pAct->level - 1;
                    nReductions = pAct->nReductions;
                    sunredTrees[currentTree]->data[currentLevel].reserve(nReductions);
                }
                break;
            case sitSunredReduction: {//instr.data[0][0] = { 0, 1, 0, 2 };
                    IsDefSunredReductionInstruction* pAct = static_cast<IsDefSunredReductionInstruction*>(act);
                    sunredTrees[currentTree]->data[currentLevel].push_back(pAct->reduction);
                }
                break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitSunredTree)
                        throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "%u is not a sunred tree instruction", act->instruction);
        }
        delete act;
        if(isNotFinished && first == nullptr)
            throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "The instruction stream has ended during sunred tree definition.");
    }
}


//***********************************************************************
void CircuitStorage::processRailsInstructions(IsInstruction*& first) {
//***********************************************************************
    bool isNotFinished = true;
    if (isNotFinished && first == nullptr)
        throw hmgExcept("CircuitStorage::processSunredTreeInstructions", "The instruction stream has ended during RAILS definition.");
    while (isNotFinished) {
        IsInstruction* act = first;
        first = first->next;
        switch (act->instruction) {
            case sitNothing: break;
            case sitRailValue: {
                    IsDefRailValueInstruction* pAct = static_cast<IsDefRailValueInstruction*>(act);
                    Rails::SetVoltage(pAct->index, pAct->value);
                }
                break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitRails)
                        throw hmgExcept("CircuitStorage::processRailsInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("CircuitStorage::processRailsInstructions", "%u is not a RAILS instruction", act->instruction);
        }
        delete act;
        if(isNotFinished && first == nullptr)
            throw hmgExcept("CircuitStorage::processRailsInstructions", "The instruction stream has ended during RAILS definition.");
    }
}


//***********************************************************************
void CircuitStorage::processProbesInstructions(IsInstruction*& first, uns currentProbe) {
//***********************************************************************
    bool isNotFinished = true;
    if (isNotFinished && first == nullptr)
        throw hmgExcept("CircuitStorage::processProbesInstructions", "The instruction stream has ended during PROBE node definition.");
    InternalNodeVarSizePack dummyInternalPack;
    while (isNotFinished) {
        IsInstruction* act = first;
        first = first->next;
        switch (act->instruction) {
            case sitNothing: break;
            case sitProbeNode: {
                    IsProbeNodeInstruction* pAct = static_cast<IsProbeNodeInstruction*>(act);

                    const ComponentBase* component = fullCircuitInstances[probes[currentProbe]->fullCircuitID].component.get();
                    for (uns i = 0; i < pAct->nodeID.componentID.size(); i++) {
                        const ComponentSubCircuit* subckt = dynamic_cast<const ComponentSubCircuit*>(component);
                        if (subckt == nullptr)
                            throw hmgExcept("CircuitStorage::processProbesInstructions", "PROBE node definition problem: [[[SUBCIRCUIT.]SUBCIRCUIT.]COMPONENT.]NODE required but COMPONENT found instead of a SUBCIRCUIT");
                        component = subckt->components[pAct->nodeID.componentID[i]].get();
                    }
                    const ComponentSubCircuit* subckt = dynamic_cast<const ComponentSubCircuit*>(component);
                    ;
                    DeepCDNodeID id = {
                        SimpleInterfaceNodeID2CDNode(pAct->nodeID.nodeID, component->pModel->externalNs, subckt == nullptr ? dummyInternalPack : static_cast<const ModelSubCircuit*>(subckt->pModel)->internalNs),
                        pAct->nodeID.componentID
                    };
                    if(subckt == nullptr && id.nodeID.type != cdntExternal)
                        throw hmgExcept("CircuitStorage::processProbesInstructions", "PROBE node definition problem: internal node is allowed only for SUBCIRCUITS");
                    probes[currentProbe]->nodes.push_back(id);
                }
                break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitProbe)
                        throw hmgExcept("CircuitStorage::processProbesInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("CircuitStorage::processProbesInstructions", "%u is not a PROBE instruction", act->instruction);
        }
        delete act;
        if(isNotFinished && first == nullptr)
            throw hmgExcept("CircuitStorage::processProbesInstructions", "The instruction stream has ended during PROBE definition.");
    }
}


//***********************************************************************
void CircuitStorage::processSaveInstructions(IsInstruction*& first, std::vector<uns>& probeIndex) {
//***********************************************************************
    bool isNotFinished = true;
    if (isNotFinished && first == nullptr)
        throw hmgExcept("CircuitStorage::processSaveInstructions", "The instruction stream has ended during SAVE probe definition.");
    while (isNotFinished) {
        IsInstruction* act = first;
        first = first->next;
        switch (act->instruction) {
            case sitNothing: break;
            case sitUns: {
                    IsUnsInstruction* pAct = static_cast<IsUnsInstruction*>(act);
                    probeIndex.push_back(pAct->data);
                }
                break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitSave)
                        throw hmgExcept("CircuitStorage::processSaveInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("CircuitStorage::processSaveInstructions", "%u is not a SAVE instruction", act->instruction);
        }
        delete act;
        if(isNotFinished && first == nullptr)
            throw hmgExcept("CircuitStorage::processSaveInstructions", "The instruction stream has ended during SAVE definition.");
    }
}


//***********************************************************************
void ModelSubCircuit::processInstructions(IsInstruction*& first) {
//***********************************************************************
    bool isNotFinished = true;
    if (isNotFinished && first == nullptr)
        throw hmgExcept("ModelSubCircuit::processInstructions", "The instruction stream has ended during subcircuit definition.");
    while (isNotFinished) {
        IsInstruction* act = first;
        first = first->next;
        switch (act->instruction) {
            case sitNothing: break;
            case sitComponentInstance: {
                    IsComponentInstanceInstruction* pAct = static_cast<IsComponentInstanceInstruction*>(act);

                    std::unique_ptr<ComponentDefinition> cd = std::make_unique<ComponentDefinition>();

                    cd->isBuiltIn = pAct->isBuiltIn;
                    cd->isDefaultRail = pAct->isDefaultRail;
                    cd->defaultValueRailIndex = pAct->defaultValueRailIndex;
                    cd->modelIndex = pAct->modelIndex;
                    cd->nodesConnectedTo.reserve(pAct->nNodes);
                    cd->params.reserve(pAct->nParams);

                    cd->processInstructions(first, *this);

                    std::vector<std::unique_ptr<ComponentDefinition>>& instanceStorage = pAct->isController ? controllers : components;

                    if (pAct->instanceIndex == instanceStorage.size()) {
                        instanceStorage.push_back(std::move(cd));
                    }
                    else {
                        if (pAct->instanceIndex > instanceStorage.size())
                            instanceStorage.resize(pAct->instanceIndex + 1);
                        instanceStorage[pAct->instanceIndex] = std::move(cd);
                    }
                }
                break;
            case sitRailRange: {
                    IsRailNodeRangeInstruction* pAct = static_cast<IsRailNodeRangeInstruction*>(act);
                    forcedNodes.push_back(pAct->forcedNodeRange);
                }
                break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitDefModelSubcircuit)
                        throw hmgExcept("ModelSubCircuit::processInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("ModelSubCircuit::processInstructions", "%u is not a MODEL SUBCIRCUIT instruction", act->instruction);
        }
        delete act;
        if(isNotFinished && first == nullptr)
            throw hmgExcept("ModelSubCircuit::processInstructions", "The instruction stream has ended during MODEL SUBCIRCUIT definition.");
    }
}


//***********************************************************************
void ComponentDefinition::processInstructions(IsInstruction*& first, const ModelSubCircuit& container) {
//***********************************************************************
    bool isNotFinished = true;
    if (isNotFinished && first == nullptr)
        throw hmgExcept("ComponentDefinition::processInstructions", "The instruction stream has ended during component definition.");
    while (isNotFinished) {
        IsInstruction* act = first;
        first = first->next;
        switch (act->instruction) {
            case sitNothing: break;
            case sitNodeValue: {
                    IsNodeValueInstruction* pAct = static_cast<IsNodeValueInstruction*>(act);
                    nodesConnectedTo.push_back(SimpleInterfaceNodeID2CDNode(pAct->nodeID, container.externalNs, container.internalNs));
                }
                break;
            case sitParameterValue: {
                    IsParameterValueInstruction* pAct = static_cast<IsParameterValueInstruction*>(act);
                    params.push_back(ParameterInstance2CDParam(pAct->param, container.externalNs, container.internalNs));
                }
                break;
            case sitEndInstruction: {
                    IsEndDefInstruction* pAct = static_cast<IsEndDefInstruction*>(act);
                    if(pAct->whatEnds != sitComponentInstance)
                        throw hmgExcept("ComponentDefinition::processInstructions", "illegal ending instruction type (%u) in global level", pAct->whatEnds);
                    isNotFinished = false;
                }
                break;
        default:
            throw hmgExcept("ComponentDefinition::processInstructions", "%u is not a component instance instruction", act->instruction);
        }
        delete act;
        if(isNotFinished && first == nullptr)
            throw hmgExcept("ComponentDefinition::processInstructions", "The instruction stream has ended during component instance definition.");
    }
}


//***********************************************************************
void ComponentSubCircuit::allocForReductionDC() {
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix || model.solutionType == SolutionType::stSunRed) {
        if (model.solutionType == SolutionType::stFullMatrix) {
            if (!sfmrDC)
                sfmrDC = std::make_unique<SubCircuitFullMatrixReductorDC>(this);
            sfmrDC->alloc();
        }
        else if (model.solutionType == SolutionType::stSunRed) {
            sunred.buildTree(*model.srTreeInstructions, this);
            sunred.allocDC();
        }
    }
}


//***********************************************************************
void ComponentSubCircuit::allocForReductionAC() {
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == SolutionType::stFullMatrix || model.solutionType == SolutionType::stSunRed) {
        if (model.solutionType == SolutionType::stFullMatrix) {
            if (!sfmrAC)
                sfmrAC = std::make_unique<SubCircuitFullMatrixReductorAC>(this);
            sfmrAC->alloc();
        }
        else if (model.solutionType == SolutionType::stSunRed) {
            sunred.allocAC();
        }
    }
}


//***********************************************************************
void ComponentSubCircuit::buildOrReplace() {
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (version == model.getVersion()) {
        
        // this subcircuit (and the directly contained components) didn't change but deeper (e.g. components[n]->components[k]) could
        // => recursive traversal of the component tree and buildOrReplace
        // => recursive update of isJacobianMXSymmetrical

        // replacing

        csiz nComponent = model.components.size();
        for (auto& comp : components)
            if (comp->isEnabled) comp->buildOrReplace();

        // isJacobianMXSymmetrical

        bool isSymmDC = true;
        bool isSymmAC = true;
        for (siz i = 0; i < nComponent && (isSymmDC || isSymmAC); i++) {
            isSymmDC = isSymmDC && components[i]->isJacobianMXSymmetrical(true);
            isSymmAC = isSymmAC && components[i]->isJacobianMXSymmetrical(false);
        }
        isJacobianMXSymmetricalDC_ = isSymmDC;
        isJacobianMXSymmetricalAC_ = isSymmAC;

        // isContainedComponentWithInternalNode

        bool isInternalNode = false;
        for (siz i = 0; i < nComponent && !isInternalNode; i++) {
            isInternalNode = components[i]->pModel->getN_InternalNodes() > 0;
            if (!isInternalNode) {
                const ComponentSubCircuit* pSubckt = dynamic_cast<ComponentSubCircuit*>(components[i].get());
                if (pSubckt)
                    isInternalNode = pSubckt->isContainedComponentWithInternalNode;
            }
        }
        isContainedComponentWithInternalNode = isInternalNode;

        return;
    }

    // nodes:
    // - allocates memory for internal nodes and vars
    // - if there are unconnected ONodes, allocates memory for then at the end of internalNodesAndVars, and sets the unconnected ONodes to them
    // - sets the forced nodes

    uns nUnconnectedONode = 0;
    cuns nEndOnodes = model.getN_ExternalNodes();
    for (uns i = model.getN_Start_Of_O_Nodes(); i < nEndOnodes; i++) {
        if (externalNodes[i] == nullptr)
            nUnconnectedONode++;
    }

    uns nUnconnectedOnodeIndex = model.internalNs.nNormalInternalNodes + model.internalNs.nControlInternalNodes + model.internalNs.nInternalVars; // = the number of internal nodes and internal vars
    
    delete[] internalNodesAndVars;
    nInternalNodesAndVars = nUnconnectedOnodeIndex + nUnconnectedONode;
    internalNodesAndVars = new NodeVariable[nInternalNodesAndVars];

    for (uns i = model.getN_Start_Of_O_Nodes(); i < nEndOnodes; i++) {
        if (externalNodes[i] == nullptr)
            externalNodes[i] = &internalNodesAndVars[nUnconnectedOnodeIndex++];
    }

    for (const auto& forced : model.forcedNodes) {
        if (forced.isExternal)
            for (uns i = forced.nodeStartIndex; i <= forced.nodeStopIndex; i++)
                externalNodes[i]->setDefaultValueIndex(forced.defaultRailIndex, true);
        else
            for (uns i = forced.nodeStartIndex; i <= forced.nodeStopIndex; i++)
                internalNodesAndVars[i].setDefaultValueIndex(forced.defaultRailIndex, true);
    }

    // creating components

    //*******************************************************
    // Here we don't examine that the contained components are enabled or not (defalult is enabled)
    //*******************************************************

    csiz nComponent = model.components.size();
    components.resize(nComponent);
    for (siz i = 0; i < nComponent; i++) {
        const ComponentDefinition& cDef = *model.components[i];
        uns defNodeIndex = cDef.isDefaultRail ? cDef.defaultValueRailIndex : this->defaultNodeValueIndex;
        const ComponentAndControllerModelBase* iModel = cDef.isBuiltIn
            ? CircuitStorage::getInstance().builtInModels[cDef.modelIndex].get()
            : CircuitStorage::getInstance().models[cDef.modelIndex].get();
        components[i] = std::unique_ptr<ComponentBase>(static_cast<ComponentBase*>(iModel->makeComponent(&cDef, defNodeIndex)));
    }

    // setting external nodes and parameters of the contained components

    for (siz i = 0; i < nComponent; i++) {
        ComponentBase& comp = *components[i].get();
        for (siz j = 0; j < comp.def->nodesConnectedTo.size(); j++) {
            const CDNode& cdn = comp.def->nodesConnectedTo[j];
            if (cdn.type != CDNodeType::cdntUnconnected) {
                comp.setNode(j,
                    cdn.type == CDNodeType::cdntInternal ? &internalNodesAndVars[cdn.index]
                    : cdn.type == CDNodeType::cdntExternal ? externalNodes[cdn.index]
                    : cdn.type == CDNodeType::cdntRail ? &Rails::V[cdn.index].get()->rail
                    : cdn.type == CDNodeType::cdntGnd ? &Rails::V[defaultNodeValueIndex].get()->rail
                    : nullptr // unconnected node, never gets here
                );
            }
        }
    }

    for (siz i = 0; i < nComponent; i++) {
        ComponentBase& comp = *components[i].get();
        for (siz j = 0; j < comp.def->params.size(); j++) {
            const CDParam& cdp = comp.def->params[j];
            Param par;
            switch (cdp.type) {
                case CDParamType::cdptValue:
                    par.value = cdp.value;
                    break;
                case CDParamType::cdptGlobalVariable:
                    par.var = CircuitStorage::getInstance().globalVariables[cdp.index].get();
                    break;
                case CDParamType::cdptLocalVariable:
                    par.var = &internalNodesAndVars[model.internalNs.nNormalInternalNodes + model.internalNs.nControlInternalNodes + cdp.index];
                    break;
                case CDParamType::cdptParam:
                    par = pars[cdp.index];
                    break;
                case CDParamType::cdptInternalNode:
                    par.var = &internalNodesAndVars[cdp.index];
                    break;
                case CDParamType::cdptExternalNode:
                    par.var = externalNodes[cdp.index];
                    break;
            }
            comp.setParam(j,par);
        }
    }

    // building / replacing

    for (siz i = 0; i < nComponent; i++) {
        components[i]->buildOrReplace();
    }

    // isJacobianMXSymmetrical

    bool isSymmDC = true;
    bool isSymmAC = true;
    for (siz i = 0; i < nComponent && (isSymmDC || isSymmAC); i++) {
        isSymmDC = isSymmDC && components[i]->isJacobianMXSymmetrical(true);
        isSymmAC = isSymmAC && components[i]->isJacobianMXSymmetrical(false);
    }
    isJacobianMXSymmetricalDC_ = isSymmDC;
    isJacobianMXSymmetricalAC_ = isSymmAC;

    // isContainedComponentWithInternalNode

    bool isInternalNode = false;
    for (siz i = 0; i < nComponent && !isInternalNode; i++) {
        isInternalNode = components[i]->pModel->getN_InternalNodes() > 0;
        if (!isInternalNode) {
            const ComponentSubCircuit* pSubckt = dynamic_cast<ComponentSubCircuit*>(components[i].get());
            if (pSubckt)
                isInternalNode = pSubckt->isContainedComponentWithInternalNode;
        }
    }
    isContainedComponentWithInternalNode = isInternalNode;

    // set isConcurrent for normal internal nodes (for control internal nodes it does not change! (control internal node can be an internal node of a component))

    for (siz i = 0; i < model.internalNodeIsConcurrent.size(); i++)
        internalNodesAndVars[i].setIsConcurrent(model.internalNodeIsConcurrent[i]);

    // Allocations

    allocForReductionDC();

    // structure is up to date

    version = model.getVersion();
}


//***********************************************************************
void SubCircuitFullMatrixReductorDC::forwsubs() {
//***********************************************************************
    ComponentSubCircuit& subckt = *pSubCircuit;
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*subckt.pModel);
    const bool isSymm = subckt.isJacobianMXSymmetrical(true);

    YAwork.zero_unsafe();
    XATwork.zero_unsafe();
    XBwork.zero_unsafe();
    YBwork.zero_unsafe();
    JA.zero(); // the defect of the external nodes is addad to the defect of the subckt component's JB so here not used
    // JB.zero(); // initialized with the defect of the internal nodes

    cuns A1_nIONodes = model.getN_IO_Nodes();
    cuns A2_nNINodes = model.getN_Normal_I_Nodes();
    cuns B1_nNInternalNodes = model.getN_NormalInternalNodes();
    cuns B2_nNONodes = model.getN_Normal_O_Nodes();
    cuns ONodes_start = model.getN_Start_Of_O_Nodes();
    cuns NONodes_end = ONodes_start + B2_nNONodes; // end index of normal ONodes

    // defect of the internal nodes

    for (uns i = 0; i < B1_nNInternalNodes; i++)
        JB[i] = -subckt.internalNodesAndVars[i].getDDC(); // JB initialized here

    if (B2_nNONodes != 0) { // some internal nodes are routed out (they are stored among the external nodes)
        for (uns i = 0; i < B2_nNONodes; i++)
            JB[B1_nNInternalNodes + i] = -subckt.externalNodes[ONodes_start + i]->getDDC();
    }

    // admittance + defect of the contained components

    for (uns i = 0; i < subckt.getNContainedComponents(); i++) {
        const ComponentBase& compInstance = *subckt.getContainedComponent(i);
        if (compInstance.isEnabled) {
            const ComponentAndControllerModelBase& compModel = compInstance.getModel();
            const ComponentDefinition& compDef = *compInstance.def;
            cuns nIO = compModel.getN_IO_Nodes();
            cuns nAx = nIO + (isSymm ? 0 : compModel.getN_Normal_I_Nodes());
            for (uns rowSrc = 0; rowSrc < nIO; rowSrc++) {

                // ground connections disappear

                if (compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntRail || compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntGnd || compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntUnconnected)
                    continue;

                // what is the destination?
                // if normal ONode among the external nodes, it must be handled as an internal node

                bool isA;
                uns yDest = compDef.nodesConnectedTo[rowSrc].index;
                if (B2_nNONodes != 0) {
                    if (compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntExternal) {
                        if (yDest >= ONodes_start && yDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                            isA = false; // false: NormalONode
                            yDest += B1_nNInternalNodes - ONodes_start;
                        }
                        else isA = true;
                    }
                    else isA = false; // false: internal
                }
                else isA = compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntExternal; // false: internal

                uns externalIndex1 = isA ? yDest : 0;
                uns internalIndex1 = isA ? 0 : yDest;

                // loading J

                if (isA)
                    JA[yDest] += compInstance.getJreducedDC(rowSrc);
                else
                    JB[yDest] += compInstance.getJreducedDC(rowSrc);

                // admittances 

                for (uns colSrc = isSymm ? rowSrc : 0; colSrc < nAx; colSrc++) { // in symmetric matrices the admittances should be increased only once(y[i,j] and y[j,i] would be the same)
                    if (compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntRail || compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntGnd || compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntUnconnected)
                        continue;
                    crvt adm = compInstance.getYDC(rowSrc, colSrc);

                    // what is the destination?
                    // if normal ONode among the external nodes, it must be handled as an internal node

                    bool isUp;
                    uns xDest = compDef.nodesConnectedTo[colSrc].index;
                    if (B2_nNONodes != 0) {
                        if (compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntExternal) {
                            if (xDest >= ONodes_start && xDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                                isUp = false; // false: NormalONode
                                xDest += B1_nNInternalNodes - ONodes_start;
                            }
                            else isUp = true;
                        }
                        else isUp = false; // false: internal
                    }
                    else isUp = compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntExternal; // false: internal

                    uns externalIndex2 = isUp ? xDest : 0;
                    uns internalIndex2 = isUp ? 0 : xDest;

                    // add admittance

                    if (isA) {
                        if (isUp) {  // YA
                            YAwork.get_elem(externalIndex1, externalIndex2) += adm; // get_elem_unsafe cannot be used because yDest > xDest possible
                        }
                        else { // XA
                            if (isSymm) {
                                XBwork[externalIndex1 + externalIndex2][internalIndex1 + internalIndex2] += adm; // (!) in symm case XA is not used, XB is XAT; indices switched;
                            }
                            else {
                                XATwork[externalIndex1 + externalIndex2][internalIndex1 + internalIndex2] += adm;
                            }
                        }
                    }
                    else {
                        if (isUp) { // XB
                            XBwork[externalIndex1 + externalIndex2][internalIndex1 + internalIndex2] += adm;
                        }
                        else { // YB
                            YBwork.get_elem(internalIndex1, internalIndex2) += adm; // get_elem_unsafe cannot be used because yDest > xDest possible
                        }
                    }
                }
            }
        }
    }

    // Refresh matrices

    bool isJacobiChanged = YAcopy.refresh_unsafe(YAwork);
    if (!isSymm) isJacobiChanged = XATcopy.refresh_unsafe(XATwork) || isJacobiChanged;
    isJacobiChanged = XBcopy.refresh_unsafe(XBwork) || isJacobiChanged;
    isJacobiChanged = YBcopy.refresh_unsafe(YBwork) || isJacobiChanged;

    // reduce (=Jacobi)

    if (isJacobiChanged) {
        if (isSymm) {
            NZB.copy_from_symm_to_nonsymm(YBcopy);
            NZB.math_symm_ninv_of_nonsymm();
            NZBXA.math_mul_t_unsafe(NZB, XBcopy);
            NZBXAT.transp(NZBXA);
            YRED.math_add_mul_t_symm(YAcopy, XBcopy, NZBXAT);
        }
        else {
            NZB.copy_unsafe(YBcopy);
            NZB.math_ninv_np();
            NZBXA.math_mul_t_unsafe(NZB, XATcopy);
            NZBXAT.transp(NZBXA);
            YRED.math_add_mul_t_unsafe(YAcopy, XBcopy, NZBXAT);
        }
    }

    // forward (=defects)

    math_mul(NZBJB, NZB, JB);
    math_add_mul(JRED, JA, XBcopy, NZBJB);
}


//***********************************************************************
void SubCircuitFullMatrixReductorDC::backsubs() {
//***********************************************************************
    ComponentSubCircuit& subckt = *pSubCircuit;
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*subckt.pModel);
    const bool isSymm = subckt.isJacobianMXSymmetrical(true);

    // fill UA

    cuns A1_nIONodes = model.getN_IO_Nodes();
    cuns A2_nNINodes = model.getN_Normal_I_Nodes();
    cuns nA = A1_nIONodes + A2_nNINodes;
    cuns B1_nNInternalNodes = model.getN_NormalInternalNodes();
    cuns B2_nNONodes = model.getN_Normal_O_Nodes();

    for (uns i = 0; i < nA; i++) {
        UA[i] = subckt.externalNodes[i]->getVDC();
    }

    // backward

    if (UB.size() == 1)      math_1_add_mul_ub(UB, NZBJB, NZBXA, UA);
    else if (UB.size() == 2) math_2_add_mul_ub(UB, NZBJB, NZBXA, UA);
    else                         math_add_mul(UB, NZBJB, NZBXA, UA);

    // v of the internal nodes

    for (uns i = 0; i < B1_nNInternalNodes; i++)
        subckt.internalNodesAndVars[i].setVDC(UB[i]);

    if (B2_nNONodes != 0) { // some internal nodes are routed out (they are stored among the external nodes)
        cuns ONodes_start = model.getN_Start_Of_O_Nodes();
        for (uns i = 0; i < B2_nNONodes; i++)
            subckt.externalNodes[ONodes_start + i]->setVDC(UB[B1_nNInternalNodes + i]);
    }
}


//***********************************************************************
void SubCircuitFullMatrixReductorAC::forwsubs() {
//***********************************************************************
    ComponentSubCircuit& subckt = *pSubCircuit;
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*subckt.pModel);
    const bool isSymm = subckt.isJacobianMXSymmetrical(false);

    YA.zero_unsafe();
    XAT.zero_unsafe();
    XB.zero_unsafe();
    YB_NZB.zero_unsafe();
    JA.zero(); // the defect of the external nodes is addad to the defect of the subckt component's JB so here not used

    cuns A1_nIONodes = model.getN_IO_Nodes();
    cuns A2_nNINodes = model.getN_Normal_I_Nodes();
    cuns B1_nNInternalNodes = model.getN_NormalInternalNodes();
    cuns B2_nNONodes = model.getN_Normal_O_Nodes();
    cuns ONodes_start = model.getN_Start_Of_O_Nodes();
    cuns NONodes_end = ONodes_start + B2_nNONodes; // end index of normal ONodes

    // defect of the internal nodes

    for (uns i = 0; i < B1_nNInternalNodes; i++)
        JB[i] = -subckt.internalNodesAndVars[i].getDAC(); // JB initialized here

    if (B2_nNONodes != 0) { // some internal nodes are routed out (they are stored among the external nodes)
        for (uns i = 0; i < B2_nNONodes; i++)
            JB[B1_nNInternalNodes + i] = -subckt.externalNodes[ONodes_start + i]->getDAC();
    }

    // admittance + defect of the contained components

    for (uns i = 0; i < subckt.getNContainedComponents(); i++) {
        const ComponentBase& compInstance = *subckt.getContainedComponent(i);
        if (compInstance.isEnabled) {
            const ComponentAndControllerModelBase& compModel = compInstance.getModel();
            const ComponentDefinition& compDef = *compInstance.def;
            cuns nIO = compModel.getN_IO_Nodes();
            cuns nAx = nIO + (isSymm ? 0 : compModel.getN_Normal_I_Nodes());
            for (uns rowSrc = 0; rowSrc < nIO; rowSrc++) {

                // ground connections disappear

                if (compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntRail || compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntGnd || compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntUnconnected)
                    continue;

                // what is the destination?
                // if normal ONode among the external nodes, it must be handled as an internal node

                bool isA;
                uns yDest = compDef.nodesConnectedTo[rowSrc].index;
                if (B2_nNONodes != 0) {
                    if (compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntExternal) {
                        if (yDest >= ONodes_start && yDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                            isA = false; // false: NormalONode
                            yDest += B1_nNInternalNodes - ONodes_start;
                        }
                        else isA = true;
                    }
                    else isA = false; // false: internal
                }
                else isA = compDef.nodesConnectedTo[rowSrc].type == CDNodeType::cdntExternal; // false: internal

                uns externalIndex1 = isA ? yDest : 0;
                uns internalIndex1 = isA ? 0 : yDest;

                // loading J

                if (isA)
                    JA[yDest] += compInstance.getJreducedAC(rowSrc);
                else
                    JB[yDest] += compInstance.getJreducedAC(rowSrc);

                // admittances 

                for (uns colSrc = isSymm ? rowSrc : 0; colSrc < nAx; colSrc++) { // in symmetric matrices the admittances should be increased only once(y[i,j] and y[j,i] would be the same)
                    if (compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntRail || compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntGnd || compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntUnconnected)
                        continue;
                    ccplx adm = compInstance.getYAC(rowSrc, colSrc);

                    // what is the destination?
                    // if normal ONode among the external nodes, it must be handled as an internal node

                    bool isUp;
                    uns xDest = compDef.nodesConnectedTo[colSrc].index;
                    if (B2_nNONodes != 0) {
                        if (compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntExternal) {
                            if (xDest >= ONodes_start && xDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                                isUp = false; // false: NormalONode
                                xDest += B1_nNInternalNodes - ONodes_start;
                            }
                            else isUp = true;
                        }
                        else isUp = false; // false: internal
                    }
                    else isUp = compDef.nodesConnectedTo[colSrc].type == CDNodeType::cdntExternal; // false: internal

                    uns externalIndex2 = isUp ? xDest : 0;
                    uns internalIndex2 = isUp ? 0 : xDest;

                    // add admittance

                    if (isA) {
                        if (isUp) {  // YA
                            YA.get_elem(externalIndex1, externalIndex2) += adm; // get_elem_unsafe cannot be used because yDest > xDest possible
                        }
                        else { // XA
                            if (isSymm) {
                                XB[externalIndex1 + externalIndex2][internalIndex1 + internalIndex2] += adm; // (!) in symm case XA is not used, XB is XAT; indices switched; ??
                            }
                            else {
                                XAT[externalIndex1 + externalIndex2][internalIndex1 + internalIndex2] += adm;
                            }
                        }
                    }
                    else {
                        if (isUp) { // XB
                            XB[externalIndex1 + externalIndex2][internalIndex1 + internalIndex2] += adm;
                        }
                        else { // YB
                            if (isSymm) {
                                if(internalIndex1 > internalIndex2) // always put in the upper triangle
                                    YB_NZB.get_elem(internalIndex2, internalIndex1) += adm;
                                else
                                    YB_NZB.get_elem(internalIndex1, internalIndex2) += adm;
                            }
                            else {
                                YB_NZB.get_elem(internalIndex1, internalIndex2) += adm;
                            }
                        }
                    }
                }
            }
        }
    }


    // reduce (=Jacobi)

    if (isSymm) {
        YB_NZB.symmetrize_from_upper();
        YB_NZB.math_symm_ninv_of_nonsymm();
        NZBXA.math_mul_t_unsafe(YB_NZB, XB);
        NZBXAT.transp(NZBXA);
        YRED.math_add_mul_t_symm(YA, XB, NZBXAT);
    }
    else {
        YB_NZB.math_ninv_np();
        NZBXA.math_mul_t_unsafe(YB_NZB, XAT);
        NZBXAT.transp(NZBXA);
        YRED.math_add_mul_t_unsafe(YA, XB, NZBXAT);
    }

    // forward (=defects)

    math_mul(NZBJB, YB_NZB, JB);
    math_add_mul(JRED, JA, XB, NZBJB);
}


//***********************************************************************
void SubCircuitFullMatrixReductorAC::backsubs() {
//***********************************************************************
    ComponentSubCircuit& subckt = *pSubCircuit;
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*subckt.pModel);
    const bool isSymm = subckt.isJacobianMXSymmetrical(false);

    // fill UA

    cuns A1_nIONodes = model.getN_IO_Nodes();
    cuns A2_nNINodes = model.getN_Normal_I_Nodes();
    cuns nA = A1_nIONodes + A2_nNINodes;
    cuns B1_nNInternalNodes = model.getN_NormalInternalNodes();
    cuns B2_nNONodes = model.getN_Normal_O_Nodes();

    for (uns i = 0; i < nA; i++) {
        UA[i] = subckt.externalNodes[i]->getVAC();
    }

    // backward

    if (UB.size() == 1)      math_1_add_mul_ub(UB, NZBJB, NZBXA, UA);
    else if (UB.size() == 2) math_2_add_mul_ub(UB, NZBJB, NZBXA, UA);
    else                         math_add_mul(UB, NZBJB, NZBXA, UA);

    // v of the internal nodes

    for (uns i = 0; i < B1_nNInternalNodes; i++)
        subckt.internalNodesAndVars[i].setVAC(UB[i]);

    if (B2_nNONodes != 0) { // some internal nodes are routed out (they are stored among the external nodes)
        cuns ONodes_start = model.getN_Start_Of_O_Nodes();
        for (uns i = 0; i < B2_nNONodes; i++)
            subckt.externalNodes[ONodes_start + i]->setVAC(UB[B1_nNInternalNodes + i]);
    }
}


//***********************************************************************
void ComponentSubCircuit::solveDC() {
//***********************************************************************
    calculateValueDC();
    loadFtoD(true); // ! this is a multigrid-specific instruction, in normal case (e.g. sunred), D is deleted
    calculateCurrent(true);
    //ComponentBase::DefectCollector d = collectCurrentDefectDC();
    //ComponentBase::DefectCollector v = collectVoltageDefectDC();
    forwsubs(true);
    backsubs(true);
    acceptIterationDC(true); // value += v
    acceptStepDC(); // stepstart = value
}


//***********************************************************************
void ComponentSubCircuit::relaxDC(uns nRelax) {
//***********************************************************************
    calculateValueDC();
    deleteYii(true);
    calculateYiiDC();
    //std::cout << "\nRelax Start\n" << std::endl;
    //printNodeValueDC(0);
    for (uns i = 0; i < nRelax; i++) {
        loadFtoD(true);
        calculateCurrent(true);
        GaussSeidelRed(true);
        loadFtoD(true);
        calculateCurrent(true);
        GaussSeidelBlack(true);
        //std::cout << "\nRelax " << (i + 1) << std::endl;
        //printNodeValueDC(0);
        //std::cout << std::endl;
    }
    //std::cout << "\nRelax Stop\n" << std::endl;
    //printNodeValueDC(0);
}


//***********************************************************************
void ComponentSubCircuit::prolongateUDC(const FineCoarseConnectionDescription& connections, const hmgMultigrid& multigrid) {
//***********************************************************************

    CircuitStorage& gc = CircuitStorage::getInstance();
    ComponentSubCircuit& coarse = *gc.fullCircuitInstances[connections.indexCoarseFullCircuit].component;

    // internal nodes

    for (const auto& instruction : connections.globalNodeProlongations) {
        rvt sumU = rvt0;
        for (const auto& src : instruction.instr) {
            const NodeVariable& srcNode = (src.srcComponentIndex == unsMax) ? coarse.internalNodesAndVars[src.srcNodeIndex] : *coarse.components[src.srcComponentIndex]->getInternalNode(src.srcNodeIndex);
            sumU += srcNode.getValue0DC() * src.weight;
        }
        NodeVariable& destNode = internalNodesAndVars[instruction.destNodeIndex];
        destNode.setValue0DC(sumU);
    }

    // contained components

    for (const auto& componentGroup : connections.componentGroups) {

        // the same cell in both levels

        if (componentGroup.isCopy) {
            for (uns i = 0; i < componentGroup.fineCells.size(); i++) { // size of fineCells and coarseCells must be the same
                auto& comp = components[componentGroup.fineCells[i]];
                if(comp->isEnabled)
                    comp->recursiveProlongRestrictCopy(true, rprProlongateU, coarse.components[componentGroup.coarseCells[i]].get());
            }
            continue;
        }
        
        const LocalProlongationOrRestrictionInstructions& instructions = multigrid.localNodeProlongationTypes[componentGroup.localProlongationIndex];

        // top level components
        
        for (const auto& dest : instructions.destComponentsNodes) {
            
            rvt sumU = rvt0;
            
            for (const auto& src : dest.instr) {
                
                ComponentBase* srcComponent = src.isDestLevel ? components[componentGroup.fineCells[src.srcIndex]].get() : coarse.components[componentGroup.coarseCells[src.srcIndex]].get();
                
                const NodeVariable& srcNode = src.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.index);
                sumU += srcNode.getValue0DC() * src.weight;
            }

            NodeVariable& destNode = dest.nodeID.type == cdntExternal
                ? *components[componentGroup.fineCells[dest.destIndex]]->getNode(dest.nodeID.index)
                : *components[componentGroup.fineCells[dest.destIndex]]->getInternalNode(dest.nodeID.index);

            destNode.setValue0DC(sumU);
        }

        // deep components (in most cases there are no deep components)

        for (const auto& dest : instructions.deepDestComponentNodes) {

            // calculating the voltage
            
            rvt sumU = rvt0;
            
            for (const auto& src : dest.instr) {

                // getting the starting component

                ComponentBase* srcComponent = src.isDestLevel
                    ? components[componentGroup.fineCells[src.nodeID.componentID[0]]].get()
                    : coarse.components[componentGroup.coarseCells[src.nodeID.componentID[0]]].get();

                // going through the chain

                for (uns i = 1; i < src.nodeID.componentID.size(); i++) {
                    srcComponent = static_cast<ComponentSubCircuit*>(srcComponent)->components[src.nodeID.componentID[i]].get();
                }

                // adding the weighted node value

                const NodeVariable& srcNode = src.nodeID.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.nodeID.index);
                sumU += srcNode.getValue0DC() * src.weight;
            }

            // finding the destination component: the starting component

            ComponentBase* destComponent = components[componentGroup.fineCells[dest.nodeID.componentID[0]]].get();

            // finding the destination component: going through the chain

            for (uns i = 1; i < dest.nodeID.componentID.size(); i++) {
                destComponent = static_cast<ComponentSubCircuit*>(destComponent)->components[dest.nodeID.componentID[i]].get();
            }

            // setting the voltage

            NodeVariable& destNode = dest.nodeID.nodeID.type == cdntExternal
                ? *destComponent->getNode(dest.nodeID.nodeID.index)
                : *destComponent->getInternalNode(dest.nodeID.nodeID.index);

            destNode.setValue0DC(sumU);
        }
    }
}


//***********************************************************************
void ComponentSubCircuit::restrictUDC(const FineCoarseConnectionDescription& connections, const hmgMultigrid& multigrid) {
//***********************************************************************

    CircuitStorage& gc = CircuitStorage::getInstance();
    ComponentSubCircuit& coarse = *gc.fullCircuitInstances[connections.indexCoarseFullCircuit].component;

    // internal nodes

    for (const auto& instruction : connections.globalNodeRestrictions) {
        rvt sumU = rvt0;
        for (const auto& src : instruction.instr) {
            const NodeVariable& srcNode = (src.srcComponentIndex == unsMax) ? internalNodesAndVars[src.srcNodeIndex] : *components[src.srcComponentIndex]->getInternalNode(src.srcNodeIndex);
            sumU += srcNode.getValue0DC() * src.weight;
        }
        NodeVariable& destNode = coarse.internalNodesAndVars[instruction.destNodeIndex];
        destNode.setValue0DC(sumU);
    }

    // contained components

    for (const auto& componentGroup : connections.componentGroups) {

        // the same cell in both levels

        if (componentGroup.isCopy) {
            for (uns i = 0; i < componentGroup.fineCells.size(); i++) { // size of fineCells and coarseCells must be the same
                auto& comp = components[componentGroup.fineCells[i]];
                if (comp->isEnabled)
                    comp->recursiveProlongRestrictCopy(true, rprRestrictU, coarse.components[componentGroup.coarseCells[i]].get());
            }
            continue;
        }

        const LocalProlongationOrRestrictionInstructions& instructions = multigrid.localNodeRestrictionTypes[componentGroup.localRestrictionIndex];

        // top level components

        for (const auto& dest : instructions.destComponentsNodes) {

            rvt sumU = rvt0;

            for (const auto& src : dest.instr) {

                ComponentBase* srcComponent = src.isDestLevel ? coarse.components[componentGroup.coarseCells[src.srcIndex]].get() : components[componentGroup.fineCells[src.srcIndex]].get();

                const NodeVariable& srcNode = src.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.index);
                sumU += srcNode.getValue0DC() * src.weight;
            }

            NodeVariable& destNode = dest.nodeID.type == cdntExternal
                ? *coarse.components[componentGroup.coarseCells[dest.destIndex]]->getNode(dest.nodeID.index)
                : *coarse.components[componentGroup.coarseCells[dest.destIndex]]->getInternalNode(dest.nodeID.index);

            destNode.setValue0DC(sumU);
        }

        // deep components (in most cases there are no deep components)

        for (const auto& dest : instructions.deepDestComponentNodes) {

            // calculating the voltage

            rvt sumU = rvt0;

            for (const auto& src : dest.instr) {

                // getting the starting component

                ComponentBase* srcComponent = src.isDestLevel
                    ? coarse.components[componentGroup.coarseCells[src.nodeID.componentID[0]]].get()
                    : components[componentGroup.fineCells[src.nodeID.componentID[0]]].get();

                // going through the chain

                for (uns i = 1; i < src.nodeID.componentID.size(); i++) {
                    srcComponent = static_cast<ComponentSubCircuit*>(srcComponent)->components[src.nodeID.componentID[i]].get();
                }

                // adding the weighted node value

                const NodeVariable& srcNode = src.nodeID.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.nodeID.index);
                sumU += srcNode.getValue0DC() * src.weight;
            }

            // finding the destination component: the starting component

            ComponentBase* destComponent = coarse.components[componentGroup.coarseCells[dest.nodeID.componentID[0]]].get();

            // finding the destination component: going through the chain

            for (uns i = 1; i < dest.nodeID.componentID.size(); i++) {
                destComponent = static_cast<ComponentSubCircuit*>(destComponent)->components[dest.nodeID.componentID[i]].get();
            }

            // setting the voltage

            NodeVariable& destNode = dest.nodeID.nodeID.type == cdntExternal
                ? *destComponent->getNode(dest.nodeID.nodeID.index)
                : *destComponent->getInternalNode(dest.nodeID.nodeID.index);

            destNode.setValue0DC(sumU);
        }
    }
}


//***********************************************************************
rvt ComponentSubCircuit::restrictFDDC(const FineCoarseConnectionDescription& connections, const hmgMultigrid& multigrid) {
// fH = R(fh) + dH – R(dh), ret: sum (dHi – R(dh)i)^2
//***********************************************************************

    CircuitStorage& gc = CircuitStorage::getInstance();
    ComponentSubCircuit& coarse = *gc.fullCircuitInstances[connections.indexCoarseFullCircuit].component;

    rvt truncationError = rvt0;

    // internal nodes

    for (const auto& instruction : connections.globalNodeRestrictions) {
        rvt sumRfh = rvt0;
        rvt sumRdh = rvt0;
        for (const auto& src : instruction.instr) {
            const NodeVariable& srcNode = (src.srcComponentIndex == unsMax) ? internalNodesAndVars[src.srcNodeIndex] : *components[src.srcComponentIndex]->getInternalNode(src.srcNodeIndex);
            sumRfh += srcNode.getFDC() * src.weight;
            sumRdh += srcNode.getDDC() * src.weight;
        }
        NodeVariable& destNode = coarse.internalNodesAndVars[instruction.destNodeIndex];
        crvt dH = destNode.getDDC();
        truncationError += square(dH - sumRdh);
        destNode.setFDC(sumRfh + dH - sumRdh);
    }

    // contained components

    for (const auto& componentGroup : connections.componentGroups) {

        // the same cell in both levels

        if (componentGroup.isCopy) {
            for (uns i = 0; i < componentGroup.fineCells.size(); i++) { // size of fineCells and coarseCells must be the same
                auto& comp = components[componentGroup.fineCells[i]];
                if (comp->isEnabled)
                    truncationError += comp->recursiveProlongRestrictCopy(true, rprRestrictFDD, coarse.components[componentGroup.coarseCells[i]].get());
            }
            continue;
        }

        const LocalProlongationOrRestrictionInstructions& instructions = multigrid.localNodeRestrictionTypes[componentGroup.localRestrictionIndex];

        // top level components

        for (const auto& dest : instructions.destComponentsNodes) {

            rvt sumRfh = rvt0;
            rvt sumRdh = rvt0;

            for (const auto& src : dest.instr) {

                ComponentBase* srcComponent = src.isDestLevel ? coarse.components[componentGroup.coarseCells[src.srcIndex]].get() : components[componentGroup.fineCells[src.srcIndex]].get();

                const NodeVariable& srcNode = src.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.index);
                sumRfh += srcNode.getFDC() * src.weight;
                sumRdh += srcNode.getDDC() * src.weight;

            }

            NodeVariable& destNode = dest.nodeID.type == cdntExternal
                ? *coarse.components[componentGroup.coarseCells[dest.destIndex]]->getNode(dest.nodeID.index)
                : *coarse.components[componentGroup.coarseCells[dest.destIndex]]->getInternalNode(dest.nodeID.index);
                
            crvt dH = destNode.getDDC();
            truncationError += square(dH - sumRdh);
            destNode.setFDC(sumRfh + dH - sumRdh);
        }

        // deep components (in most cases there are no deep components)

        for (const auto& dest : instructions.deepDestComponentNodes) {

            // calculating the voltage

            rvt sumRfh = rvt0;
            rvt sumRdh = rvt0;

            for (const auto& src : dest.instr) {

                // getting the starting component

                ComponentBase* srcComponent = src.isDestLevel
                    ? coarse.components[componentGroup.coarseCells[src.nodeID.componentID[0]]].get()
                    : components[componentGroup.fineCells[src.nodeID.componentID[0]]].get();

                // going through the chain

                for (uns i = 1; i < src.nodeID.componentID.size(); i++) {
                    srcComponent = static_cast<ComponentSubCircuit*>(srcComponent)->components[src.nodeID.componentID[i]].get();
                }

                // adding the weighted node value

                const NodeVariable& srcNode = src.nodeID.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.nodeID.index);
                sumRfh += srcNode.getFDC() * src.weight;
                sumRdh += srcNode.getDDC() * src.weight;
            }

            // finding the destination component: the starting component

            ComponentBase* destComponent = coarse.components[componentGroup.coarseCells[dest.nodeID.componentID[0]]].get();

            // finding the destination component: going through the chain

            for (uns i = 1; i < dest.nodeID.componentID.size(); i++) {
                destComponent = static_cast<ComponentSubCircuit*>(destComponent)->components[dest.nodeID.componentID[i]].get();
            }

            // setting the voltage

            NodeVariable& destNode = dest.nodeID.nodeID.type == cdntExternal
                ? *destComponent->getNode(dest.nodeID.nodeID.index)
                : *destComponent->getInternalNode(dest.nodeID.nodeID.index);

            crvt dH = destNode.getDDC();
            truncationError += square(dH - sumRdh);
            destNode.setFDC(sumRfh + dH - sumRdh);
        }
    }
    return truncationError;
}


//***********************************************************************
void ComponentSubCircuit::uHMinusRestrictUhToDHNCDC(const FineCoarseConnectionDescription& connections, const hmgMultigrid& multigrid) {
// dH_NonConcurrent = uH – R(uh)
//***********************************************************************

    CircuitStorage& gc = CircuitStorage::getInstance();
    ComponentSubCircuit& coarse = *gc.fullCircuitInstances[connections.indexCoarseFullCircuit].component;

    // internal nodes

    for (const auto& instruction : connections.globalNodeRestrictions) {
        rvt sumU = rvt0;
        for (const auto& src : instruction.instr) {
            const NodeVariable& srcNode = (src.srcComponentIndex == unsMax) ? internalNodesAndVars[src.srcNodeIndex] : *components[src.srcComponentIndex]->getInternalNode(src.srcNodeIndex);
            sumU += srcNode.getValue0DC() * src.weight;
        }
        NodeVariable& destNode = coarse.internalNodesAndVars[instruction.destNodeIndex];
        crvt uH = destNode.getValue0DC();
        destNode.setDNonConcurrentDC(uH - sumU);
    }

    // contained components

    for (const auto& componentGroup : connections.componentGroups) {

        // the same cell in both levels

        if (componentGroup.isCopy) {
            for (uns i = 0; i < componentGroup.fineCells.size(); i++) { // size of fineCells and coarseCells must be the same
                auto& comp = components[componentGroup.fineCells[i]];
                if (comp->isEnabled)
                    comp->recursiveProlongRestrictCopy(true, rpruHMinusRestrictUhToDHNC, coarse.components[componentGroup.coarseCells[i]].get());
            }
            continue;
        }

        const LocalProlongationOrRestrictionInstructions& instructions = multigrid.localNodeRestrictionTypes[componentGroup.localRestrictionIndex];

        // top level components

        for (const auto& dest : instructions.destComponentsNodes) {

            rvt sumU = rvt0;

            for (const auto& src : dest.instr) {

                ComponentBase* srcComponent = src.isDestLevel ? coarse.components[componentGroup.coarseCells[src.srcIndex]].get() : components[componentGroup.fineCells[src.srcIndex]].get();

                const NodeVariable& srcNode = src.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.index);
                sumU += srcNode.getValue0DC() * src.weight;

            }

            NodeVariable& destNode = dest.nodeID.type == cdntExternal
                ? *coarse.components[componentGroup.coarseCells[dest.destIndex]]->getNode(dest.nodeID.index)
                : *coarse.components[componentGroup.coarseCells[dest.destIndex]]->getInternalNode(dest.nodeID.index);

            crvt uH = destNode.getValue0DC();
            destNode.setDNonConcurrentDC(uH - sumU);
        }

        // deep components (in most cases there are no deep components)

        for (const auto& dest : instructions.deepDestComponentNodes) {

            // calculating the voltage

            rvt sumU = rvt0;

            for (const auto& src : dest.instr) {

                // getting the starting component

                ComponentBase* srcComponent = src.isDestLevel
                    ? coarse.components[componentGroup.coarseCells[src.nodeID.componentID[0]]].get()
                    : components[componentGroup.fineCells[src.nodeID.componentID[0]]].get();

                // going through the chain

                for (uns i = 1; i < src.nodeID.componentID.size(); i++) {
                    srcComponent = static_cast<ComponentSubCircuit*>(srcComponent)->components[src.nodeID.componentID[i]].get();
                }

                // adding the weighted node value

                const NodeVariable& srcNode = src.nodeID.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.nodeID.index);
                sumU += srcNode.getValue0DC() * src.weight;
            }

            // finding the destination component: the starting component

            ComponentBase* destComponent = coarse.components[componentGroup.coarseCells[dest.nodeID.componentID[0]]].get();

            // finding the destination component: going through the chain

            for (uns i = 1; i < dest.nodeID.componentID.size(); i++) {
                destComponent = static_cast<ComponentSubCircuit*>(destComponent)->components[dest.nodeID.componentID[i]].get();
            }

            // setting the voltage

            NodeVariable& destNode = dest.nodeID.nodeID.type == cdntExternal
                ? *destComponent->getNode(dest.nodeID.nodeID.index)
                : *destComponent->getInternalNode(dest.nodeID.nodeID.index);

            crvt uH = destNode.getValue0DC();
            destNode.setDNonConcurrentDC(uH - sumU);
        }
    }
}


//***********************************************************************
void ComponentSubCircuit::prolongateDHNCAddToUhDC(const FineCoarseConnectionDescription& connections, const hmgMultigrid& multigrid) {
// uh = uh + P(dH_NonConcurrent)
//***********************************************************************

    CircuitStorage& gc = CircuitStorage::getInstance();
    ComponentSubCircuit& coarse = *gc.fullCircuitInstances[connections.indexCoarseFullCircuit].component;

    // internal nodes

    for (const auto& instruction : connections.globalNodeProlongations) {
        rvt sumTemp = rvt0;
        for (const auto& src : instruction.instr) {
            const NodeVariable& srcNode = (src.srcComponentIndex == unsMax) ? coarse.internalNodesAndVars[src.srcNodeIndex] : *coarse.components[src.srcComponentIndex]->getInternalNode(src.srcNodeIndex);
            sumTemp += srcNode.getDNonConcurrentDC() * src.weight;
        }
        NodeVariable& destNode = internalNodesAndVars[instruction.destNodeIndex];
        destNode.incValue0DC(sumTemp);
    }

    // contained components

    for (const auto& componentGroup : connections.componentGroups) {

        // the same cell in both levels

        if (componentGroup.isCopy) {
            for (uns i = 0; i < componentGroup.fineCells.size(); i++) { // size of fineCells and coarseCells must be the same
                auto& comp = components[componentGroup.fineCells[i]];
                if (comp->isEnabled)
                    comp->recursiveProlongRestrictCopy(true, rprProlongateDHNCAddToUh, coarse.components[componentGroup.coarseCells[i]].get());
            }
            continue;
        }

        const LocalProlongationOrRestrictionInstructions& instructions = multigrid.localNodeProlongationTypes[componentGroup.localProlongationIndex];

        // top level components

        for (const auto& dest : instructions.destComponentsNodes) {

            rvt sumTemp = rvt0;

            for (const auto& src : dest.instr) {

                ComponentBase* srcComponent = src.isDestLevel ? components[componentGroup.fineCells[src.srcIndex]].get() : coarse.components[componentGroup.coarseCells[src.srcIndex]].get();

                const NodeVariable& srcNode = src.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.index);
                sumTemp += srcNode.getDNonConcurrentDC() * src.weight;
            }

            NodeVariable& destNode = dest.nodeID.type == cdntExternal
                ? *components[componentGroup.fineCells[dest.destIndex]]->getNode(dest.nodeID.index)
                : *components[componentGroup.fineCells[dest.destIndex]]->getInternalNode(dest.nodeID.index);

            destNode.incValue0DC(sumTemp);
        }

        // deep components (in most cases there are no deep components)

        for (const auto& dest : instructions.deepDestComponentNodes) {

            // calculating the voltage

            rvt sumTemp = rvt0;

            for (const auto& src : dest.instr) {

                // getting the starting component

                ComponentBase* srcComponent = src.isDestLevel
                    ? components[componentGroup.fineCells[src.nodeID.componentID[0]]].get()
                    : coarse.components[componentGroup.coarseCells[src.nodeID.componentID[0]]].get();

                // going through the chain

                for (uns i = 1; i < src.nodeID.componentID.size(); i++) {
                    srcComponent = static_cast<ComponentSubCircuit*>(srcComponent)->components[src.nodeID.componentID[i]].get();
                }

                // adding the weighted node value

                const NodeVariable& srcNode = src.nodeID.nodeID.type == cdntExternal ? *srcComponent->getNode(src.nodeID.nodeID.index) : *srcComponent->getInternalNode(src.nodeID.nodeID.index);
                sumTemp += srcNode.getDNonConcurrentDC() * src.weight;
            }

            // finding the destination component: the starting component

            ComponentBase* destComponent = components[componentGroup.fineCells[dest.nodeID.componentID[0]]].get();

            // finding the destination component: going through the chain

            for (uns i = 1; i < dest.nodeID.componentID.size(); i++) {
                destComponent = static_cast<ComponentSubCircuit*>(destComponent)->components[dest.nodeID.componentID[i]].get();
            }

            // setting the voltage

            NodeVariable& destNode = dest.nodeID.nodeID.type == cdntExternal
                ? *destComponent->getNode(dest.nodeID.nodeID.index)
                : *destComponent->getInternalNode(dest.nodeID.nodeID.index);

            destNode.incValue0DC(sumTemp);
        }
    }
}


//***********************************************************************
void hmgMultigrid::solveDC() {
//***********************************************************************
    CircuitStorage& gc = CircuitStorage::getInstance();
    gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->deleteF(true);
    gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->solveDC();
    for (uns iDestLevel = 0; iDestLevel < levels.size(); iDestLevel++) {
        const FineCoarseConnectionDescription& destLevel = levels[iDestLevel];

        gc.fullCircuitInstances[destLevel.indexFineFullCircuit].component->prolongateUDC(destLevel, *this);

        for (uns iV = 0; iV < nVcycles; iV++) {
            rvt truncationError = rvt0;

            // downward line of the V

            uns iDown = iDestLevel + 1;
            do {
                iDown--; // iDestLevel to 0, iDestLevel and 0 included (0 is the coarsest destination(!) level)
                    
                const FineCoarseConnectionDescription& hLevel = levels[iDown];
                ComponentSubCircuit& hGrid = *gc.fullCircuitInstances[hLevel.indexFineFullCircuit].component;
                ComponentSubCircuit& HGrid = *gc.fullCircuitInstances[hLevel.indexCoarseFullCircuit].component;

                // TODO: controllers; where?

                // relax

                hGrid.relaxDC(nPreSmoothings);
                //hGrid.printNodesDC();
                //HGrid.printNodesDC();

                // Lh

                hGrid.calculateValueDC();
                hGrid.deleteD(true);
                hGrid.calculateCurrent(true);

                // R(uh)

                hGrid.restrictUDC(hLevel, *this);

                // LH

                HGrid.calculateValueDC();
                HGrid.deleteD(true);
                HGrid.calculateCurrent(true);

                // fH

                rvt truncErr = hGrid.restrictFDDC(hLevel, *this); // fH = R(fh) + dH – R(dh)
                if (iDown == iDestLevel)
                    truncationError = truncErr;

            } while (iDown != 0);

            // solve the coarsest grid (= level -1)

            gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->solveDC(); // d0 += f0 kell!

            // upward line of the V

            for (uns iUp = 0; iUp <= iDestLevel; iUp++) {
                    
                const FineCoarseConnectionDescription& hLevel = levels[iUp];
                ComponentSubCircuit& hGrid = *gc.fullCircuitInstances[hLevel.indexFineFullCircuit].component;
                ComponentSubCircuit& HGrid = *gc.fullCircuitInstances[hLevel.indexCoarseFullCircuit].component;

                // uh

                hGrid.uHMinusRestrictUhToDHNCDC(hLevel, *this); // dH_NonConcurrent = uH – R(uh)
                hGrid.prolongateDHNCAddToUhDC(hLevel, *this);   // uh += P(dH_NonConcurrent)

                // relax

                hGrid.relaxDC(nPostSmoothings);
            }

            // residual

            ComponentSubCircuit& destGrid = *gc.fullCircuitInstances[destLevel.indexFineFullCircuit].component;
            destGrid.calculateValueDC();
            destGrid.deleteD(true);
            destGrid.calculateCurrent(true);
            rvt residual = destGrid.calculateResidual(true); // the residual and the truncationError would be (sqrt(this thing) / nodenum) but not needed
            //if (residual < errorRate * truncationError)
                iV = nVcycles; // break
        }
    }
}


//***********************************************************************
}
//***********************************************************************
