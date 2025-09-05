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
void Simulation::iterate() {
//***********************************************************************

}

//***********************************************************************
uns curretIterN = 0;
//***********************************************************************


//***********************************************************************
void Simulation::runDC() {
//***********************************************************************

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
    rvt max_error = 1000.0;
    for (uns i = 0; /*max_error > 1.0e-006 &&*/ i < 5; i++) {
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

        rvt cErr = c.sumDefect / c.nodeNum + c.maxDefect;
        rvt vErr = v.sumDefect / v.nodeNum + v.maxDefect;
        max_error = cErr + vErr;
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
    wasDC = true;
}


//***********************************************************************
void Simulation::runTimeStep() {
//***********************************************************************
    if (SimControl::nNonlinComponents == 10) {
        CircuitStorage& gc = CircuitStorage::getInstance();
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::AcceptStepDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
#ifdef HMG_DEBUGPRINT
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif
    }
    else {
        rvt max_error = 1000.0;
        CircuitStorage& gc = CircuitStorage::getInstance();
        for (uns i = 0; max_error > 1.0e-004 && i < 100; i++) {
            CircuitStorage::CalculateControllersDC(fullCircuitID);
            CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
            ComponentBase::DefectCollector c = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
            CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
            ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
            CircuitStorage::AcceptIterationDC(fullCircuitID);
            max_error = c.sumDefect / c.nodeNum + c.maxDefect + v.sumDefect / v.nodeNum + v.maxDefect;
            std::cout << "    error = " << max_error << std::endl;
        }
        CircuitStorage::AcceptStepDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
#ifdef HMG_DEBUGPRINT
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
#endif
    }
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
        runDC();
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
            runDC();
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
            runTimeStep();
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
