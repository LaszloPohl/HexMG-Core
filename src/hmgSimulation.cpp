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
void Simulation::ananlysis(IsInstruction* instruction) {
//***********************************************************************
    ///ControlAnalysisInstruction* analInstr = dynamic_cast<ControlAnalysisInstruction*>(instruction);
    //if (analInstr == nullptr)
    //    throw hmgExcept("Simulation::ananlysis", "bug: bad parameter");
    //if (gc->isStructuralChange) {
    //    most("xxxxx");
    //    gc->networkModel.initAndRefresh(gc, false);
    //    most("initAndRefresh");
    //    gc->isStructuralChange = false;
    //}
    //switch (analInstr->type) {
    //    case atDC: runOp(); break;
    //    case atTimeStep: runTimeStep(analInstr->value); break;
        //case atIterate: iterate(); break;
    //    default:
    //        throw hmgExcept("Simulation::ananlysis", "unknown analysis type (%u)", analInstr->type);
    //}
}


//***********************************************************************
void Simulation::iterate() {
//***********************************************************************

}


//***********************************************************************
void Simulation::runDC() {
//***********************************************************************

    // TODO: SimControl::minIter


    //ComponentInstance::_dtime.setAct(0);

    //most("start");
    //gc->networkModel.updateControllers();
    //most("updateControllers");
    //gc->networkModel.updateComponents(ComponentInstance::_dtime.getBase());
    //most("updateComponents");
    //double err = gc->networkModel.currentCalculation();
    //most("currentCalculation");
    //gc->networkModel.forwSubs();
    //most("forwsubs");

    //gc->isInitialOpNeeded = false;
}


//***********************************************************************
void Simulation::runTimeStep() {
//***********************************************************************
    //if (gc->isInitialOpNeeded) {
    //    ComponentInstance::_time.setAll(0);
    //    ComponentInstance::_dtime.setAll(0);
    //    runOp();
    //}
    //ComponentInstance::_dtime.setAct(dt);
}


//***********************************************************************
void Simulation::runAC() {
//***********************************************************************
    //ComponentInstance::_dtime.setAct(0);

    //most("start");
    //gc->networkModel.updateControllers();
    //most("updateControllers");
    //gc->networkModel.updateComponents(ComponentInstance::_dtime.getBase());
    //most("updateComponents");
    //double err = gc->networkModel.currentCalculation();
    //most("currentCalculation");
    //gc->networkModel.forwSubs();
    //most("forwsubs");

    //gc->isInitialOpNeeded = false;
}


//***********************************************************************
void Simulation::runTimeConst() {
//***********************************************************************
    //ComponentInstance::_dtime.setAct(0);

    //most("start");
    //gc->networkModel.updateControllers();
    //most("updateControllers");
    //gc->networkModel.updateComponents(ComponentInstance::_dtime.getBase());
    //most("updateComponents");
    //double err = gc->networkModel.currentCalculation();
    //most("currentCalculation");
    //gc->networkModel.forwSubs();
    //most("forwsubs");

    //gc->isInitialOpNeeded = false;
}


//***********************************************************************
void Simulation::run(const RunData& runData) {
//***********************************************************************
    analysisType = runData.analysisType;
    err = runData.err;
    switch (analysisType) {
        case atDC: {
            if(runData.isInitial)
                SimControl::setInitialDC();
            else
                SimControl::setFinalDC();
            // TODO: runData.isPre
            timeFreqValue = rvt0;
            dtValue = rvt0;
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
            runTimeStep();
        }
        break;
        case atAC: {
            SimControl::setComplexFrequencyForAC(runData.fTauDtT);
            timeFreqValue = SimControl::getFrequency();
            dtValue = rvt0;
            runAC();
        }
        break;
        case atTimeConst: {
            if (runData.isTau)
                SimControl::setComplexFrequencyForTimeConst(rvt1 / (2 * hmgPi * runData.fTauDtT), runData.iterNumSPD);
            else
                SimControl::setComplexFrequencyForTimeConst(runData.fTauDtT, runData.iterNumSPD);
            timeFreqValue = SimControl::getFrequency();
            dtValue = rvt0;
            runTimeConst();
        }
        break;
        default:
            throw hmgExcept("Simulation::run", "unknown analysis Type: %u", (uns)analysisType);
    }
    //printf("*");
    //while (!gc->controlInstructions.empty()) {
        //printf("-");
        //ControlInstructionBase* instruction = gc->controlInstructions.front();
        //gc->controlInstructions.pop_front();
        //switch (instruction->getType()) {
        //    case citNothing: break;
        //    case citAnalysis: ananlysis(instruction); break;
        //    case citRepaceComponetType: break;
        //    case citSave: break;
        //    default:
        //        throw hmgExcept("Simulation::run", "bug: unknown instruction type");
        //}
        //delete instruction;
    //}
    //printf("*");
}


}
