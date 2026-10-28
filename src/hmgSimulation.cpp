//***********************************************************************
// HexMG Simulation Source
// Creation date:  2021. 09. 14.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#include "hmgComponent.h"
#include "hmgSimulation.h"
#include <cmath>
#include "hmgOptimization.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
void Simulation::iterate() {
//***********************************************************************

}

//***********************************************************************
uns curretIterN = 0;
//***********************************************************************


//***********************************************************************
// The legacy defect metric is intentionally retained: it is not relative.
static rvt defectError(const ComponentBase::DefectCollector& d) {
    const rvt e = (d.nodeNum == 0 ? rvt0 : d.sumDefect / d.nodeNum) + d.maxDefect;
    if (!(std::isfinite(e) && e >= rvt0))
        throw hmgExcept("defectError", "Non-finite or invalid iteration defect");
    return e;
}

static void requireValidErr(rvt e) {
    if (!(std::isfinite(e) && e > rvt0))
        throw hmgExcept("requireValidErr", "ERR must be finite and positive: %g", e);
}

static void requireConverged(rvt e, rvt limit) {
    if (!(std::isfinite(e) && e <= limit))
        throw hmgExcept("requireConverged", "Did not converge: error=%g, ERR=%g", e, limit);
}

// A separate path leaves legacy decks and multigrid stopping rules unchanged.
void Simulation::runRelativeDC(bool timeStep, uns requestedIterations) {
    if(requestedIterations==0) requireValidErr(err);
    const double relative=requestedIterations ? RunData{}.err : err;
    auto& storage=CircuitStorage::getInstance();
    std::vector<NodeVariable*> nodes;
    storage.fullCircuitInstances[fullCircuitID].component->collectConvergenceNodes(nodes);
    std::sort(nodes.begin(),nodes.end(),std::less<NodeVariable*>{});
    nodes.erase(std::unique(nodes.begin(),nodes.end()),nodes.end());
    for(auto* node:nodes) node->convergenceProfile.reset();
    std::set<NodeVariable*> configured;
    for(const auto& option:optimization::convergenceNodes) {
        if(option.circuitId!=fullCircuitID) continue;
        const ComponentBase* component=storage.fullCircuitInstances[fullCircuitID].component.get();
        for(auto index:option.id.componentID) {
            const auto* sub=dynamic_cast<const ComponentSubCircuit*>(component);
            if(!sub || index>=sub->getNContainedComponents()) throw hmgExcept("convergence","Invalid component path %s",option.node.c_str());
            component=sub->getContainedComponent(index);
        }
        const auto* sub=dynamic_cast<const ComponentSubCircuit*>(component);
        DeepCDNodeID id{SimpleInterfaceNodeID2CDNode(option.id.nodeID,component->pModel->externalNs,
            sub?static_cast<const ModelSubCircuit*>(sub->pModel)->getInternalNodeSizes():InternalNodeSizePack{}),option.id.componentID,false};
        if(id.nodeID.type!=cdntInternal && id.nodeID.type!=cdntExternal)
            throw hmgExcept("convergence","Not a solved node: %s",option.node.c_str());
        auto* node=const_cast<NodeVariable*>(&storage.getNode(fullCircuitID,id));
        if(!std::binary_search(nodes.begin(),nodes.end(),node,std::less<NodeVariable*>{}) || !configured.insert(node).second)
            throw hmgExcept("convergence","Unsolved or aliased repeated node: %s",option.node.c_str());
        node->convergenceProfile=option.profile;
    }
    const auto refresh=[&] {
        for(auto* node:nodes) node->clearPhysicalMagnitude();
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    };
    const auto measure=[&](bool change) {
        convergence::Collector result;
        for(auto* node:nodes) {
            try { result.add(change?node->relativeChangeError(relative):node->relativeBalanceError(relative)); }
            catch(const std::exception& e) {
                throw hmgExcept("convergence","Circuit %u node %u/%u/%u/%u %s: %s",fullCircuitID,
                    node->nid[3],node->nid[2],node->nid[1],node->nid[0],change?"state":"balance",e.what());
            }
        }
        return result.maximum();
    };
    curretIterN=0;
    double stateError=0, balanceError=0;
    const unsigned limit=requestedIterations?requestedIterations:
        optimization::maxIterations?optimization::maxIterations:(timeStep?100u:101u);
    refresh();
    (void)measure(false); // Reject an invalid starting physical state before reduction.
    do {
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        stateError=measure(true);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        ++curretIterN;
        refresh(); // Includes capacitor currents before accepting the step history.
        balanceError=measure(false);
        std::cout << "Relative iter " << curretIterN << " state=" << stateError << " balance=" << balanceError << std::endl;
    } while(curretIterN<limit && (requestedIterations || stateError>1 || balanceError>1));
    if(!requestedIterations && !(stateError<=1 && balanceError<=1))
        throw hmgExcept("convergence","Circuit %u did not converge in %u updates: normalized state=%g, balance=%g (both must be <=1)",
            fullCircuitID,curretIterN,stateError,balanceError);
    CircuitStorage::AcceptStepDC(fullCircuitID);
    if(!timeStep) wasDC=true;
}

void Simulation::runDC(uns requestedIterations) {
    if(optimization::relativeConvergence) { runRelativeDC(false,requestedIterations); return; }
    if (requestedIterations == 0) requireValidErr(err);
    curretIterN = 0;
    auto* component = CircuitStorage::getInstance().fullCircuitInstances[fullCircuitID].component.get();
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
    // Corrections must be sampled before AcceptIterationDC clears them.
    const rvt firstVoltageError = defectError(component->collectVoltageDefectDC());
    CircuitStorage::AcceptIterationDC(fullCircuitID);
    ++curretIterN;
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    rvt max_error = firstVoltageError + defectError(component->collectCurrentDefectDC());
    if (!std::isfinite(max_error)) throw hmgExcept("Simulation::runDC", "Non-finite first iteration");
    // Preserve the legacy complete-solve bound: one initial + 100 loop updates.
    // A positive request instead means exactly that many TOTAL updates.
    for (uns i = 0; requestedIterations != 0
        ? i < requestedIterations - 1
        : (i == 0 || max_error > err) && curretIterN < (optimization::maxIterations ? optimization::maxIterations : 101u); ++i) {
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        const rvt vErr = defectError(component->collectVoltageDefectDC());
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        ++curretIterN;
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        const rvt cErr = defectError(component->collectCurrentDefectDC());
        max_error = cErr + vErr;
        if (!std::isfinite(max_error)) throw hmgExcept("Simulation::runDC", "Non-finite iteration");
        std::cout << "Iter " << (i + 1) << "    error = " << max_error
                  << "    I error = " << cErr << "    V error = " << vErr << std::endl;
    }
    if (requestedIterations == 0) requireConverged(max_error, err);
    CircuitStorage::AcceptStepDC(fullCircuitID);
    wasDC = true;
}

void Simulation::runTimeStep(uns requestedIterations) {
    if(optimization::relativeConvergence) { runRelativeDC(true,requestedIterations); return; }
    if (requestedIterations == 0) requireValidErr(err);
    curretIterN = 0;
    rvt max_error = 0;
    auto* component = CircuitStorage::getInstance().fullCircuitInstances[fullCircuitID].component.get();
    for (uns i = 0; requestedIterations != 0
        ? i < requestedIterations
        : (i == 0 || max_error > err) && i < (optimization::maxIterations ? optimization::maxIterations : 100u); ++i) {
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        const rvt cErr = defectError(component->collectCurrentDefectDC());
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        const rvt vErr = defectError(component->collectVoltageDefectDC());
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        ++curretIterN;
        max_error = cErr + vErr;
        if (!std::isfinite(max_error)) throw hmgExcept("Simulation::runTimeStep", "Non-finite iteration");
        std::cout << "    error = " << max_error << std::endl;
    }
    if (requestedIterations == 0) requireConverged(max_error, err);
    // Refresh currents while capacitor history still refers to the old step.
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    (void)defectError(component->collectCurrentDefectDC());
    CircuitStorage::AcceptStepDC(fullCircuitID);
}

void Simulation::runAC() {
//***********************************************************************
    CircuitStorage& gc = CircuitStorage::getInstance();
    if (!isBuiltForAC) {
        for (auto& fullCkt : gc.fullCircuitInstances)
            fullCkt.component->buildForAC();
        isBuiltForAC = true;
    }
    gc.fullCircuitInstances[fullCircuitID].component->deleteD(false);
    gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(false);
    gc.fullCircuitInstances[fullCircuitID].component->forwsubs(false);
    gc.fullCircuitInstances[fullCircuitID].component->backsubs(false);
    gc.fullCircuitInstances[fullCircuitID].component->acceptIterationAndStepAC();
    gc.fullCircuitInstances[fullCircuitID].component->deleteD(false);
    gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(false);
#ifdef HMG_DEBUGPRINT
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueAC(0);
#endif
}


//***********************************************************************
void Simulation::run(const RunData& runData) {
//***********************************************************************

    analysisType = runData.analysisType;
    err = runData.err;
    fullCircuitID = runData.fullCircuitID;
    dtValue = rvt0;

    // ******************************************************************
    // MULTIGRID
    // ******************************************************************

    if (runData.isMultigrid) {
        if(optimization::relativeConvergence) throw hmgExcept("convergence","Relative convergence is not implemented for multigrid runs");

        if (runData.isInitial)
            SimControl::setInitialDC();
        else
            SimControl::setFinalDC();
        // TODO: runData.isPre
        timeFreqValue = rvt0;
        dtValue = rvt0;

        CircuitStorage& gc = CircuitStorage::getInstance();
        gc.multiGrids[runData.fullCircuitID]->solveDC(); // fullCircuitID used as choosing the multigrid (in most cases only one multigrid is defined => fullCircuitID == 0
#ifdef HMG_DEBUGPRINT
        std::cout << std::endl;

        gc.fullCircuitInstances[1].component->printNodeValueDC(0);
        std::cout << std::endl;
        gc.fullCircuitInstances[0].component->printNodeValueDC(0);
#endif
        return;
    }

    // ******************************************************************
    // END MULTIGRID
    // ******************************************************************

    if ((!wasDC && (analysisType == atTimeStep || analysisType == atAC || analysisType == atTimeConst)) || (analysisType == atTimeStep && runData.isInitial)) {
        SimControl::setInitialDC();
        // TODO: runData.isPre
        timeFreqValue = rvt0;
        dtValue = rvt0;
        SimControl::stepError.setValueDC(rvt0);
        // ITER/ITERS on a timestep does not limit its automatic initial DC.
        // ERR is ignored for a fixed-step request, including this hidden solve.
        const rvt requestedErr = err;
        if (analysisType == atTimeStep && runData.iterNumSPD != 0) err = RunData{}.err;
        runDC();
        err = requestedErr;
    }
    switch (analysisType) {
        case atDC: {
            if(runData.isInitial)
                SimControl::setInitialDC();
            else
                SimControl::setFinalDC();
            // TODO: runData.isPre
            timeFreqValue = rvt0;
            dtValue = rvt0;
            SimControl::stepError.setValueDC(rvt0);
            runDC(runData.iterNumSPD);
        }
        break;
        case atTimeStep: {
            if (runData.isDT)
                SimControl::stepTransientWithDT(runData.fTauDtT);
            else
                SimControl::stepTransientWithTStop(runData.fTauDtT);
            // TODO: runData.isPre
            timeFreqValue = SimControl::timeStepStop.getValueDC();
            dtValue = SimControl::dt.getValueDC();
            SimControl::stepError.setValueDC(rvt0);
            std::cout << "Time = " << timeFreqValue << " s\n";
            runTimeStep(runData.iterNumSPD);
        }
        break;
        case atAC: {
            SimControl::setComplexFrequencyForAC(runData.fTauDtT);
            timeFreqValue = SimControl::getFrequency();
            runAC();
        }
        break;
        case atTimeConst: {
            if (runData.isTau)
                SimControl::setComplexFrequencyForTimeConst(rvt1 / (2 * hmgPi * runData.fTauDtT), runData.iterNumSPD);
            else
                SimControl::setComplexFrequencyForTimeConst(runData.fTauDtT, runData.iterNumSPD);
            timeFreqValue = SimControl::getFrequency();
            isTau = runData.isTau;
            runAC();
        }
        break;
        default:
            throw hmgExcept("Simulation::run", "unknown analysis Type: %u", (uns)analysisType);
    }
}


}
