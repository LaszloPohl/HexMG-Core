//***********************************************************************
// HexMG component instance (cell) classes cpp
// Creation date:  2023. 01. 27.
// Creator:        Pohl L�szl�
//***********************************************************************


//***********************************************************************
#include "hmgComponent.h"
#include "hmgMultigrid.hpp"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
void ComponentSubCircuit::allocForReductionDC() {
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix || model.solutionType == ModelSubCircuit::SolutionType::stSunRed) {
        if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix) {
            if (!sfmrDC)
                sfmrDC = std::make_unique<SubCircuitFullMatrixReductorDC>(this);
            sfmrDC->alloc();
        }
        else if (model.solutionType == ModelSubCircuit::SolutionType::stSunRed) {
            sunred.buildTree(model.srTreeInstructions, this);
            sunred.allocDC();
        }
    }
}


//***********************************************************************
void ComponentSubCircuit::allocForReductionAC() {
//***********************************************************************
    const ModelSubCircuit& model = static_cast<const ModelSubCircuit&>(*pModel);
    if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix || model.solutionType == ModelSubCircuit::SolutionType::stSunRed) {
        if (model.solutionType == ModelSubCircuit::SolutionType::stFullMatrix) {
            if (!sfmrAC)
                sfmrAC = std::make_unique<SubCircuitFullMatrixReductorAC>(this);
            sfmrAC->alloc();
        }
        else if (model.solutionType == ModelSubCircuit::SolutionType::stSunRed) {
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

    uns nUnconnectedOnodeIndex = model.nInternalNodes + model.nInternalVars; // = the number of internal nodes and internal vars
    
    internalNodesAndVars.resize(nUnconnectedOnodeIndex + nUnconnectedONode);

    for (uns i = model.getN_Start_Of_O_Nodes(); i < nEndOnodes; i++) {
        if (externalNodes[i] == nullptr)
            externalNodes[i] = &internalNodesAndVars[nUnconnectedOnodeIndex++];
    }

    for (const auto& forced : model.forcedNodes) {
        if (forced.isExternal)
            externalNodes[forced.nodeIndex]->turnIntoNode(forced.defaultValueIndex, true);
        else
            internalNodesAndVars[forced.nodeIndex].turnIntoNode(forced.defaultValueIndex, true);
    }

    // creating components

    //*******************************************************
    // Here we don't examine that the contained components are enabled or not (defalult is enabled)
    //*******************************************************

    csiz nComponent = model.components.size();
    components.resize(nComponent);
    for (siz i = 0; i < nComponent; i++) {
        const ComponentAndControllerModelBase* iModel = model.components[i]->isBuiltIn
            ? CircuitStorage::getInstance().builtInModels[model.components[i]->componentModelIndex].get()
            : CircuitStorage::getInstance().models[model.components[i]->componentModelIndex].get();
        components[i] = std::unique_ptr<ComponentBase>(static_cast<ComponentBase*>(iModel->makeComponent(model.components[i].get())));
    }

    // setting external nodes and parameters of the contained components

    for (siz i = 0; i < nComponent; i++) {
        ComponentBase& comp = *components[i].get();
        for (siz j = 0; j < comp.def->nodesConnectedTo.size(); j++) {
            const ComponentDefinition::CDNode& cdn = comp.def->nodesConnectedTo[j];
            if (cdn.type != ComponentDefinition::CDNodeType::unconnected) {
                comp.setNode(j,
                    cdn.type == ComponentDefinition::CDNodeType::internal ? &internalNodesAndVars[cdn.index]
                    : cdn.type == ComponentDefinition::CDNodeType::external ? externalNodes[cdn.index]
                    : cdn.type == ComponentDefinition::CDNodeType::ground ? &FixVoltages::V[cdn.index].get()->fixNode
                    : nullptr // unconnected node, never gets here
                    , def->nodesDefaultValueIndex
                );
            }
        }
    }

    for (siz i = 0; i < nComponent; i++) {
        ComponentBase& comp = *components[i].get();
        for (siz j = 0; j < comp.def->params.size(); j++) {
            const ComponentDefinition::CDParam& cdp = comp.def->params[j];
            Param par;
            switch (cdp.type) {
                case ComponentDefinition::CDParamType::value:
                    par.value = cdp.value;
                    break;
                case ComponentDefinition::CDParamType::globalVariable:
                    par.var = CircuitStorage::getInstance().globalVariables[cdp.index].get();
                    break;
                case ComponentDefinition::CDParamType::localVariable:
                    par.var = &internalNodesAndVars[model.nInternalNodes + cdp.index];
                    break;
                case ComponentDefinition::CDParamType::param:
                    par = pars[cdp.index];
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


    // set isConcurrent for normal internal nodes (for control internal nodes it does not change! (control internal node can be an internal node of a component))

    for (siz i = 0; i < model.internalNodeIsConcurrent.size(); i++)
        internalNodesAndVars[i].setIsConcurrentDC(model.internalNodeIsConcurrent[i]);

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
    cuns B1_nNInternalNodes = model.getN_Normal_Internal_Nodes();
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

                if (compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::ground || compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::unconnected)
                    continue;

                // what is the destination?
                // if normal ONode among the external nodes, it must be handled as an internal node

                bool isA;
                uns yDest = compDef.nodesConnectedTo[rowSrc].index;
                if (B2_nNONodes != 0) {
                    if (compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::external) {
                        if (yDest >= ONodes_start && yDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                            isA = false; // false: NormalONode
                            yDest += B1_nNInternalNodes - ONodes_start;
                        }
                        else isA = true;
                    }
                    else isA = false; // false: internal
                }
                else isA = compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::external; // false: internal

                uns externalIndex1 = isA ? yDest : 0;
                uns internalIndex1 = isA ? 0 : yDest;

                // loading J

                if (isA)
                    JA[yDest] += compInstance.getJreducedDC(rowSrc);
                else
                    JB[yDest] += compInstance.getJreducedDC(rowSrc);

                // admittances 

                for (uns colSrc = isSymm ? rowSrc : 0; colSrc < nAx; colSrc++) { // in symmetric matrices the admittances should be increased only once(y[i,j] and y[j,i] would be the same)
                    if (compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::ground || compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::unconnected)
                        continue;
                    crvt adm = compInstance.getYDC(rowSrc, colSrc);

                    // what is the destination?
                    // if normal ONode among the external nodes, it must be handled as an internal node

                    bool isUp;
                    uns xDest = compDef.nodesConnectedTo[colSrc].index;
                    if (B2_nNONodes != 0) {
                        if (compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::external) {
                            if (xDest >= ONodes_start && xDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                                isUp = false; // false: NormalONode
                                xDest += B1_nNInternalNodes - ONodes_start;
                            }
                            else isUp = true;
                        }
                        else isUp = false; // false: internal
                    }
                    else isUp = compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::external; // false: internal

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
    cuns B1_nNInternalNodes = model.getN_Normal_Internal_Nodes();
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
    cuns B1_nNInternalNodes = model.getN_Normal_Internal_Nodes();
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

                if (compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::ground || compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::unconnected)
                    continue;

                // what is the destination?
                // if normal ONode among the external nodes, it must be handled as an internal node

                bool isA;
                uns yDest = compDef.nodesConnectedTo[rowSrc].index;
                if (B2_nNONodes != 0) {
                    if (compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::external) {
                        if (yDest >= ONodes_start && yDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                            isA = false; // false: NormalONode
                            yDest += B1_nNInternalNodes - ONodes_start;
                        }
                        else isA = true;
                    }
                    else isA = false; // false: internal
                }
                else isA = compDef.nodesConnectedTo[rowSrc].type == ComponentDefinition::CDNodeType::external; // false: internal

                uns externalIndex1 = isA ? yDest : 0;
                uns internalIndex1 = isA ? 0 : yDest;

                // loading J

                if (isA)
                    JA[yDest] += compInstance.getJreducedAC(rowSrc);
                else
                    JB[yDest] += compInstance.getJreducedAC(rowSrc);

                // admittances 

                for (uns colSrc = isSymm ? rowSrc : 0; colSrc < nAx; colSrc++) { // in symmetric matrices the admittances should be increased only once(y[i,j] and y[j,i] would be the same)
                    if (compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::ground || compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::unconnected)
                        continue;
                    ccplx adm = compInstance.getYAC(rowSrc, colSrc);

                    // what is the destination?
                    // if normal ONode among the external nodes, it must be handled as an internal node

                    bool isUp;
                    uns xDest = compDef.nodesConnectedTo[colSrc].index;
                    if (B2_nNONodes != 0) {
                        if (compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::external) {
                            if (xDest >= ONodes_start && xDest < NONodes_end) { // internal node as normal (=to be reduced) ONode
                                isUp = false; // false: NormalONode
                                xDest += B1_nNInternalNodes - ONodes_start;
                            }
                            else isUp = true;
                        }
                        else isUp = false; // false: internal
                    }
                    else isUp = compDef.nodesConnectedTo[colSrc].type == ComponentDefinition::CDNodeType::external; // false: internal

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
    cuns B1_nNInternalNodes = model.getN_Normal_Internal_Nodes();
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
    for (uns i = 0; i < nRelax; i++) {
        loadFtoD(true);
        calculateCurrent(true);
        jacobiIteration(true);
    }
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
            sumU += coarse.internalNodesAndVars[src.srcNodeIndex].getValue0DC() * src.weight;
        }
        internalNodesAndVars[instruction.destNodeIndex].setValue0DC(sumU);
    }

    // contained components

    for (const auto& componentGroup : connections.componentGroups) {
        
        const LocalProlongationOrRestrictionInstructions& instructions = multigrid.localNodeProlongationTypes[componentGroup.localProlongationIndex];

        // top level components
        
        for (const auto& dest : instructions.components) {
            rvt sumU = rvt0;
            for (const auto& src : dest.instr) {
                ComponentBase* srcComponent = src.isFine ? components[componentGroup.fineCells[src.srcIndex]].get() : coarse.components[componentGroup.coarseCells[src.srcIndex]].get();
                if (src.isExternal) {
                    sumU += srcComponent->getNode(src.nodeIndex)->getValue0DC() * src.weight;
                }
                else {
                    sumU += srcComponent->getInternalNode(src.nodeIndex)->getValue0DC() * src.weight;
                }
            }
            if (dest.isExternal) {
                components[componentGroup.fineCells[dest.destIndex]]->getNode(dest.nodeIndex)->setValue0DC(sumU);
            }
            else {
                components[componentGroup.fineCells[dest.destIndex]]->getInternalNode(dest.nodeIndex)->setValue0DC(sumU);
            }
        }

        // deep components (in most cases there are no deep components)

        for (const auto& dest : instructions.deepComponents) {

            // calculating the voltage
            
            rvt sumU = rvt0;
            
            for (const auto& src : dest.instr) {

                // getting the starting component

                ComponentBase* srcComponent = src.isFine
                    ? components[componentGroup.fineCells[src.srcComponentIndex[0]]].get()
                    : coarse.components[componentGroup.coarseCells[src.srcComponentIndex[0]]].get();

                // going through the chain

                for (uns i = 1; i < src.srcComponentIndex.size(); i++) {
                    srcComponent = static_cast<ComponentSubCircuit*>(srcComponent)->components[src.srcComponentIndex[i]].get();
                }

                // adding the weighted node value

                if (src.isExternal) {
                    sumU += srcComponent->getNode(src.nodeIndex)->getValue0DC() * src.weight;
                }
                else {
                    sumU += srcComponent->getInternalNode(src.nodeIndex)->getValue0DC() * src.weight;
                }
            }

            // finding the destination component: the starting component

            ComponentBase* destComponent = components[componentGroup.fineCells[dest.destComponentIndex[0]]].get();

            // finding the destination component: going through the chain

            for (uns i = 1; i < dest.destComponentIndex.size(); i++) {
                destComponent = static_cast<ComponentSubCircuit*>(destComponent)->components[dest.destComponentIndex[i]].get();
            }

            // setting the voltage

            if (dest.isExternal) {
                destComponent->getNode(dest.nodeIndex)->setValue0DC(sumU);
            }
            else {
                destComponent->getInternalNode(dest.nodeIndex)->setValue0DC(sumU);
            }
        }
    }
}


//***********************************************************************
}
//***********************************************************************