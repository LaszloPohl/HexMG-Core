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
        throw ConvergenceFailure("requireConverged", "Did not converge: error=%g, ERR=%g", e, limit);
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
        // One block-Picard startup update avoids a negative linearized
        // Joule source when a voltage drop first collapses from its initial
        // guess. Only the Jacobian is lagged; actual power, residuals and
        // all material/boundary guards are evaluated without modification.
        struct DissipationStartup {
            explicit DissipationStartup(bool use) { SimControl::lagDissipationJacobian=use; }
            ~DissipationStartup() { SimControl::lagDissipationJacobian=false; }
        };
        {
            DissipationStartup startup(curretIterN==0 && requestedIterations==0);
            CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        }
        stateError=measure(true);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        ++curretIterN;
        refresh(); // Includes capacitor currents before accepting the step history.
        balanceError=measure(false);
        std::cout << "Relative iter " << curretIterN << " state=" << stateError << " balance=" << balanceError << std::endl;
    } while(curretIterN<limit && (requestedIterations || stateError>1 || balanceError>1));
    if(!requestedIterations && !(stateError<=1 && balanceError<=1)) {
        const NodeVariable* worst=nullptr;double worstError=0;
        for(const auto* node:nodes){const double value=node->relativeBalanceError(relative);if(value>worstError){worst=node;worstError=value;}}
        if(worst){const auto profile=worst->effectiveConvergenceProfile();
            std::cout<<"Convergence balance node="<<worst->nid[3]<<'/'<<worst->nid[2]<<'/'<<worst->nid[1]<<'/'<<worst->nid[0]
                <<" residual="<<worst->getDDC()<<" magnitude="<<worst->extension->physicalMagnitude.loadRvt(worst->isConcurrent)
                <<" absolute="<<profile.balanceAbsolute<<" role="<<static_cast<int>(worst->convergenceRole)<<"\n";}
    }
    if(!requestedIterations && !(stateError<=1 && balanceError<=1))
        throw ConvergenceFailure("convergence","Circuit %u did not converge in %u updates: normalized state=%g, balance=%g (both must be <=1)",
            fullCircuitID,curretIterN,stateError,balanceError);
    CircuitStorage::AcceptStepDC(fullCircuitID);
    circuitsWithDCState.insert(fullCircuitID);
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
    circuitsWithDCState.insert(fullCircuitID);
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

void Simulation::runAdaptiveTimeStep(const RunData& runData, const optimization::AdaptiveVoltage& control) {
    const auto previousPrecision=std::cout.precision(17);
    struct RestorePrecision { std::streamsize value; ~RestorePrecision(){std::cout.precision(value);} } precision{previousPrecision};
    if(runData.iterNumSPD)throw hmgExcept("ADAPTIVE_VOLTAGE","ITER/ITERS cannot certify convergence of an adaptive step");
    requireValidErr(err);
    auto& storage=CircuitStorage::getInstance();
    auto* component=storage.fullCircuitInstances[fullCircuitID].component.get();
    TimeState checkpoint;
    component->collectTimeState(checkpoint);
    checkpoint.add(&SimControl::timeStepStart);checkpoint.add(&SimControl::timeStepStop);
    checkpoint.add(&SimControl::dt);checkpoint.add(&SimControl::iter);checkpoint.add(&SimControl::stepError);
    checkpoint.add(timeFreqValue);checkpoint.add(dtValue);
    checkpoint.prepare();
    unsigned acceptedIteration=curretIterN;
    const auto restore=[&] {checkpoint.restore();curretIterN=acceptedIteration;};
    std::vector<const NodeVariable*> monitored;
    for(const auto& node:optimization::adaptiveVoltageNodes)
        if(node.circuitId==fullCircuitID) monitored.push_back(&CircuitStorage::limitNode(fullCircuitID,node.id));
    const rvt target=runData.isDT?SimControl::timeStepStop.getValueDC()+runData.fTauDtT:runData.fTauDtT;
    rvt acceptedTime=SimControl::timeStepStop.getValueDC();
    if(!std::isfinite(target)||target<=acceptedTime)throw hmgExcept("ADAPTIVE_VOLTAGE","Target time must be finite and later than accepted time");
    rvt step=target-acceptedTime;
    unsigned intermediate=0;
    while(acceptedTime<target) {
        const bool countLimit=control.maximumIntermediateSteps && intermediate>=control.maximumIntermediateSteps;
        if(countLimit)step=target-acceptedTime;
        std::vector<rvt> previous;previous.reserve(monitored.size());
        for(const auto* node:monitored)previous.push_back(node->getAcceptedValueDC());
        while(true) {
            step=std::min(step,target-acceptedTime);
            const rvt candidate=acceptedTime+step;
            const rvt rounding=8*std::numeric_limits<rvt>::epsilon()*std::max(std::abs(target),std::abs(candidate));
            const rvt trialEnd=step>=target-acceptedTime || target-candidate<=rounding?target:candidate;
            if(!(trialEnd>acceptedTime))throw hmgExcept("ADAPTIVE_VOLTAGE","Time step is below representable resolution at %g s",acceptedTime);
            SimControl::stepTransientWithTStop(trialEnd);
            timeFreqValue=trialEnd;dtValue=SimControl::dt.getValueDC();
            SimControl::stepError.setValueDC(rvt0);
            bool nonconverged=false;
            try {
                try { runTimeStep(); }
                catch(const ConvergenceFailure& error) {
                    if(step>control.minimumStep) {
                        restore();++optimization::adaptiveRejectedSteps;
                        std::cout<<"ADAPTIVE_VOLTAGE: rejected nonconverged t="<<trialEnd<<" dt="<<step<<" s\n";
                        step=std::max(control.minimumStep,step/2);continue;
                    }
                    if(!control.acceptNonconverged)throw;
                    // Only a finite, material-valid last iterate is admissible.
                    // The residual is refreshed with OLD histories before accept.
                    CircuitStorage::CalculateControllersDC(fullCircuitID);
                    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
                    checkpoint.requireFinite();
                    const rvt defect=defectError(component->collectCurrentDefectDC());
                    CircuitStorage::AcceptStepDC(fullCircuitID);nonconverged=true;
                    std::cout<<"HEXMG_WARNING NONCONVERGED_STEP circuit="<<control.circuit<<" t="<<trialEnd
                        <<" dt="<<dtValue<<" defect="<<defect<<" "<<error.what();
                }
                checkpoint.requireFinite();
                rvt maximumChange=0;
                for(size_t i=0;i<monitored.size();++i) {
                    const rvt value=monitored[i]->getAcceptedValueDC();
                    const rvt change=std::abs(value-previous[i])/(std::abs(previous[i])+rvt(1e-6));
                    if(!std::isfinite(change))throw hmgExcept("ADAPTIVE_VOLTAGE","Non-finite monitored voltage");
                    maximumChange=std::max(maximumChange,change);
                }
                const bool tooLarge=maximumChange>control.relativeLimit;
                if(tooLarge && step>control.minimumStep && !countLimit) {
                    restore();++optimization::adaptiveRejectedSteps;
                    std::cout<<"ADAPTIVE_VOLTAGE: rejected change="<<maximumChange<<" dt="<<step<<" s\n";
                    step=control.minimumStep;continue;
                }
                acceptedTime=SimControl::timeStepStop.getValueDC();
                if(tooLarge) {
                    ++optimization::adaptiveForcedSteps;
                    std::cout<<"ADAPTIVE_VOLTAGE: change limit exceeded at "<<(countLimit?"intermediate-step limit":"minimum step")<<"; ";
                }
                std::cout<<"ADAPTIVE_VOLTAGE: accepted t="<<acceptedTime<<" dt="<<dtValue<<" change="<<maximumChange
                    <<" converged="<<(!nonconverged)<<"\n";
                ++optimization::adaptiveAcceptedSteps;
                if(acceptedTime<target) {
                    ++intermediate;
                    for(auto& save:optimization::adaptiveSaveProbes)if(save.circuitId==fullCircuitID) {
                        const std::string file="adaptive_"+save.circuit+".res";
                        storage.save(false,false,true,save.written,unsMax,file.c_str(),save.probeIds);
                        save.written=true;
                    }
                }
                checkpoint.capture();acceptedIteration=curretIterN;
                step=std::min(target-acceptedTime,step*8);
                break;
            } catch(...) {restore();throw;}
        }
    }
}

void Simulation::runAC() {

//***********************************************************************
    CircuitStorage& gc = CircuitStorage::getInstance();
    auto* component=gc.fullCircuitInstances[fullCircuitID].component.get();
    // Zero denotes the artificial first DC/Newton update in legacy junction
    // functions. A supplied operating point is already a physical state.
    // Keep the caller's counter intact, including when validation throws.
    struct PhysicalIteration {
        uns previous=curretIterN;
        PhysicalIteration() { if(curretIterN==0)curretIterN=1; }
        ~PhysicalIteration() { curretIterN=previous; }
    } physicalIteration;
    // Refresh functions at the supplied/current real state, without a Newton
    // solve, node reset or timestep acceptance. Capacitor DC current/history
    // must survive AC inserted between two time steps.
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    struct PreparingSmallSignal {
        PreparingSmallSignal() { SimControl::preparingSmallSignal=true; }
        ~PreparingSmallSignal() { SimControl::preparingSmallSignal=false; }
    };
    {
        PreparingSmallSignal scope;
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    }
    std::vector<NodeVariable*> state;
    component->collectConvergenceNodes(state);
    for(const auto* node:state) if(!std::isfinite(node->getValueDC()))
        throw hmgExcept("SMALL_SIGNAL_OPERATING_POINT","Non-finite state in circuit %u",fullCircuitID);
    (void)defectError(component->collectCurrentDefectDC());
    if (!circuitsBuiltForAC.contains(fullCircuitID)) {
        component->buildForAC();
        circuitsBuiltForAC.insert(fullCircuitID);
    }
    gc.fullCircuitInstances[fullCircuitID].component->deleteD(false);
    gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(false);
    CircuitStorage::RequireValidFunctionState(fullCircuitID);
    gc.fullCircuitInstances[fullCircuitID].component->forwsubs(false);
    CircuitStorage::RequireValidFunctionState(fullCircuitID);
    gc.fullCircuitInstances[fullCircuitID].component->backsubs(false);
    CircuitStorage::RequireValidFunctionState(fullCircuitID);
    gc.fullCircuitInstances[fullCircuitID].component->acceptIterationAndStepAC();
    circuitsWithDCState.insert(fullCircuitID);
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
        if(!optimization::adaptiveVoltage.empty())throw hmgExcept("ADAPTIVE_VOLTAGE","Multigrid adaptive execution is not implemented");
        if(!optimization::smallSignalOperatingPoints.empty())
            throw hmgExcept("SMALL_SIGNAL_OPERATING_POINT","Operating-point initialization is not implemented for multigrid runs");
        if(!optimization::transientInitialTemperatures.empty())
            throw hmgExcept("TRANSIENT_INITIAL_TEMPERATURE","Thermal initialization is not implemented for multigrid runs");
        if(!optimization::functionGuards.empty()||!optimization::functionRanges.empty())throw hmgExcept("FUNCTION_GUARD","Guarded functions are not implemented for multigrid runs");
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

    bool thermalInitialized = false;
    if(analysisType==atTimeStep) {
        for(auto& option:optimization::transientInitialTemperatures) {
            if(option.circuitId!=fullCircuitID || (option.started && !runData.isInitial)) continue;
            SimControl::setInitialDC();
            timeFreqValue=rvt0; dtValue=rvt0;
            SimControl::stepError.setValueDC(rvt0);
            auto* component=CircuitStorage::getInstance().fullCircuitInstances[fullCircuitID].component.get();
            component->resetNodes(true);
            if(!option.defaultRails) component->initializeThermalNodes(option.temperature);
            CircuitStorage::CalculateControllersDC(fullCircuitID);
            CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
            CircuitStorage::AcceptStepDC(fullCircuitID);
            option.started=true; circuitsWithDCState.insert(fullCircuitID); thermalInitialized=true;
            if(option.defaultRails) std::cout << "HEXMG_OPT: transient initial state DEFAULT_RAILS; no DC solve\n";
            else std::cout << "HEXMG_OPT: transient initial temperature " << option.temperature << " C; no DC solve\n";
        }
    }
    const bool suppliedOperatingPoint=(analysisType==atAC || analysisType==atTimeConst) &&
        std::any_of(optimization::smallSignalOperatingPoints.begin(),optimization::smallSignalOperatingPoints.end(),
            [&](const auto& option) { return option.circuitId==fullCircuitID; });
    if(suppliedOperatingPoint && !circuitsWithDCState.contains(fullCircuitID))
        std::cout << "HEXMG_OPT: small signal from supplied/current state; no DC solve\n";
    if (!thermalInitialized && !suppliedOperatingPoint && ((!circuitsWithDCState.contains(fullCircuitID) && (analysisType == atTimeStep || analysisType == atAC || analysisType == atTimeConst)) || (analysisType == atTimeStep && runData.isInitial))) {
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
            const auto adaptive=std::find_if(optimization::adaptiveVoltage.begin(),optimization::adaptiveVoltage.end(),
                [&](const auto& control){return control.circuitId==fullCircuitID;});
            if(adaptive!=optimization::adaptiveVoltage.end()) {runAdaptiveTimeStep(runData,*adaptive);break;}

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
