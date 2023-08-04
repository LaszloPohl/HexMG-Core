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
void Simulation::runDC() {
//***********************************************************************

    // TODO: SimControl::minIter

    if (SimControl::nNonlinComponents == 0) {
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::AcceptStepDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        
        CircuitStorage& gc = CircuitStorage::getInstance();
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
    }
    else {
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        
        CircuitStorage& gc = CircuitStorage::getInstance();
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);

        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);

        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);

        for (uns i = 0; i < 10; i++) {
            CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
            CircuitStorage::AcceptIterationDC(fullCircuitID);
            CircuitStorage::CalculateControllersDC(fullCircuitID);
            CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

            std::cout << std::endl;
            gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
        }

        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::AcceptStepDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);

        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
    }
    wasDC = true;
}


//***********************************************************************
void Simulation::runTimeStep() {
//***********************************************************************
    if (SimControl::nNonlinComponents == 0) {
        CircuitStorage& gc = CircuitStorage::getInstance();
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
        CircuitStorage::AcceptIterationDC(fullCircuitID);
        CircuitStorage::AcceptStepDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
    }
    else {
        CircuitStorage& gc = CircuitStorage::getInstance();
        for (uns i = 0; i < 10; i++) {
            CircuitStorage::CalculateControllersDC(fullCircuitID);
            CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
            CircuitStorage::ForwsubsBacksubsDC(fullCircuitID);
            CircuitStorage::AcceptIterationDC(fullCircuitID);
        }
        CircuitStorage::AcceptStepDC(fullCircuitID);
        CircuitStorage::CalculateControllersDC(fullCircuitID);
        CircuitStorage::CalculateValuesAndCurrentsDC(fullCircuitID);
        std::cout << std::endl;
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
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
    std::cout << std::endl;
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueAC(0);
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
        std::cout << std::endl;

        gc.fullCircuitInstances[1].component->printNodeValueDC(0);
        std::cout << std::endl;
        gc.fullCircuitInstances[0].component->printNodeValueDC(0);
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
