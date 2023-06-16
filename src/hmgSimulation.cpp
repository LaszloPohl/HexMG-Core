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
        CircuitStorage& gc = CircuitStorage::getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->calculateValueDC();
        gc.fullCircuitInstances[fullCircuitID].component->deleteD(true);
        gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(true);
        gc.fullCircuitInstances[fullCircuitID].component->forwsubs(true);
        gc.fullCircuitInstances[fullCircuitID].component->backsubs(true);
        gc.fullCircuitInstances[fullCircuitID].component->acceptIterationDC(true);
        gc.fullCircuitInstances[fullCircuitID].component->acceptStepDC();
        gc.fullCircuitInstances[fullCircuitID].component->calculateValueDC();
        gc.fullCircuitInstances[fullCircuitID].component->deleteD(true);
        gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(true);
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
    }
    else {

    }
    wasDC = true;
}


//***********************************************************************
void Simulation::runTimeStep() {
//***********************************************************************
    if (SimControl::nNonlinComponents == 0) {
        CircuitStorage& gc = CircuitStorage::getInstance();
        gc.fullCircuitInstances[fullCircuitID].component->calculateValueDC();
        gc.fullCircuitInstances[fullCircuitID].component->deleteD(true);
        gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(true);
        gc.fullCircuitInstances[fullCircuitID].component->forwsubs(true);
        gc.fullCircuitInstances[fullCircuitID].component->backsubs(true);
        gc.fullCircuitInstances[fullCircuitID].component->acceptIterationDC(true);
        gc.fullCircuitInstances[fullCircuitID].component->acceptStepDC();
        gc.fullCircuitInstances[fullCircuitID].component->calculateValueDC();
        gc.fullCircuitInstances[fullCircuitID].component->deleteD(true);
        gc.fullCircuitInstances[fullCircuitID].component->calculateCurrent(true);
        gc.fullCircuitInstances[fullCircuitID].component->printNodeValueDC(0);
    }
    else {

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
    gc.fullCircuitInstances[fullCircuitID].component->printNodeValueAC(0);
}


//***********************************************************************
void Simulation::run(const RunData& runData) {
//***********************************************************************
    analysisType = runData.analysisType;
    err = runData.err;
    fullCircuitID = runData.fullCircuitID;
    dtValue = rvt0;
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
            runAC();
        }
        break;
        default:
            throw hmgExcept("Simulation::run", "unknown analysis Type: %u", (uns)analysisType);
    }
}


}
