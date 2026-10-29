//***********************************************************************
// HexMG Simulation Source
// Creation date:  2021. 09. 14.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#include "hmgComponent.h"
#include "hmgSimulation.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
static inline rvt defectError(const ComponentBase::DefectCollector& d) {
//***********************************************************************
    const rvt e = (d.nodeNum == 0 ? rvt0 : d.sumDefect / d.nodeNum) + d.maxDefect;
    if (!(std::isfinite(e) && e >= rvt0))
        throw hmgExcept("defectError", "Non-finite or invalid iteration defect");
    return e;
}


//***********************************************************************
static inline void requireValidErr(rvt e) {
//***********************************************************************
    if (!(std::isfinite(e) && e > rvt0))
        throw hmgExcept("requireValidErr", "ERR must be finite and positive: %g", e);
}


//***********************************************************************
static inline void requireConverged(rvt e, rvt limit) {
//***********************************************************************
    if (!(std::isfinite(e) && e <= limit))
        throw hmgExcept("requireConverged", "Did not converge: error=%g, ERR=%g", e, limit);
}


//***********************************************************************
void Simulation::iterate() {
//***********************************************************************

}

//***********************************************************************
uns curretIterN = 0;
//***********************************************************************


//***********************************************************************
void Simulation::runDC(uns requestedIterations) {
//***********************************************************************
    if(requestedIterations == 0) 
        requireValidErr(err);

    // TODO: SimControl::minIter

    curretIterN = 0;
    CircuitStorage& gc = CircuitStorage::getInstance();

    bench_now("start DC");
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    bench_now("CalculateControllersDC");
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
  //printf("CalculateValuesAndCurrents\n");
  //gc.fullCircuitInstances[0].component->printNodeValue();
    bench_now("CalculateValuesAndCurrentsDC");
    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
  //printf("ForwsubsBacksubs\n");
  //gc.fullCircuitInstances[0].component->printNodeValue();
    ComponentBase::DefectCollector firstVoltageDefect =
        gc.fullCircuitInstances[fullCircuitID].component->collectVoltageDefectDC();
    bench_now("ForwsubsBacksubsDC");
    CircuitStorage::AcceptIterationDC(fullCircuitID);
  //printf("AcceptIteration\n");
  //gc.fullCircuitInstances[0].component->printNodeValue();
    bench_now("AcceptIterationDC");

    curretIterN++;

    CircuitStorage::CalculateControllersDC(fullCircuitID);
    bench_now("CalculateControllersDC");
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    bench_now("CalculateValuesAndCurrentsDC");
  //printf("CalculateValuesAndCurrents\n");
  //gc.fullCircuitInstances[0].component->printNodeValue();

/*
#ifdef HMG_DEBUGPRINT
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif

    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
    CircuitStorage::AcceptIterationDC(fullCircuitID);
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

#ifdef HMG_DEBUGPRINT
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif

    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
    CircuitStorage::AcceptIterationDC(fullCircuitID);
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

#ifdef HMG_DEBUGPRINT
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif
    bench_now("first iterations");
*/
    rvt max_error = defectError(firstVoltageDefect) +
        defectError(gc.fullCircuitInstances[fullCircuitID].component->collectCurrentDefectDC());
    if (!std::isfinite(max_error))
        throw hmgExcept("Simulation::runDC", "Non-finite first iteration");

    for (uns i = 0; requestedIterations != 0 ? i < requestedIterations - 1 : (i == 0 || max_error > err) && i < 100; i++) {
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
      //printf("ForwsubsBacksubs\n");
      //gc.fullCircuitInstances[0].component->printNodeValue();
        ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
        CircuitStorage::AcceptIterationDC(fullCircuitID);
      //printf("AcceptIteration\n");
      //gc.fullCircuitInstances[0].component->printNodeValue();
        curretIterN++;
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
      //printf("CalculateValuesAndCurrents\n");
      //gc.fullCircuitInstances[0].component->printNodeValue();
        ComponentBase::DefectCollector c = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();

        rvt cErr = defectError(c);
        rvt vErr = defectError(v);
        max_error = cErr + vErr;
        if (!std::isfinite(max_error))
            throw hmgExcept("Simulation::runDC", "Non-finite iteration");
        std::cout << "Iter " << (i + 1) << "    error = " << max_error << "    I error = " << cErr << "    V error = " << vErr << std::endl;
#ifdef HMG_DEBUGPRINT
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif
        bench_now("iteration");
    }
/*
    CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
    CircuitStorage::AcceptIterationDC(fullCircuitID);
    CircuitStorage::AcceptStepDC(fullCircuitID);
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

#ifdef HMG_DEBUGPRINT
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif
*/

    if (requestedIterations == 0) 
        requireConverged(max_error, err);
    
    CircuitStorage::AcceptStepDC(fullCircuitID);
    wasDC = true;
}


//***********************************************************************
void Simulation::runTimeStep(uns requestedIterations) {
//***********************************************************************
    if (requestedIterations == 0)
        requireValidErr(err);
    curretIterN = 0;
    rvt max_error = 1000.0 * err;
    CircuitStorage& gc = CircuitStorage::getInstance();
    for (uns i = 0; requestedIterations != 0 ? i < requestedIterations : (i == 0 || max_error > err) && i < 100; i++) {
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        ComponentBase::DefectCollector c = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        curretIterN++;
        max_error = defectError(c) + defectError(v);
        if (!std::isfinite(max_error))
            throw hmgExcept("Simulation::runTimeStep", "Non-finite iteration");
        std::cout << "    error = " << max_error << std::endl;
    }
    
    if (requestedIterations == 0) 
        requireConverged(max_error, err);
    
    CircuitStorage::CalculateControllersDC(fullCircuitID);
    CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
    (void)defectError(CircuitStorage::getInstance().fullCircuitInstances[fullCircuitID].component->collectCurrentDefectDC());
    CircuitStorage::AcceptStepDC(fullCircuitID);
#ifdef HMG_DEBUGPRINT
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif
}


//***********************************************************************
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
        const rvt requestedErr = err;
        if (analysisType == atTimeStep && runData.iterNumSPD != 0)
            err = RunData{}.err;
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
