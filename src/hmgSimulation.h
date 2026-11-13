//***********************************************************************
// HexMG Simulation Header
// Creation date:  2021. 09. 14.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HMG_SIMULATION_HEADER
#define HMG_SIMULATION_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgException.h"
#include "hmgCommon.h"
#include <set>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class Simulation {
//***********************************************************************
    uns fullCircuitID = 0;
    AnalysisType analysisType = atDC;
    rvt timeFreqValue = rvt0;
    rvt dtValue = rvt0;
    rvt err = rvt0;
    std::set<uns> circuitsWithDCState;
    std::set<uns> circuitsBuiltForAC;
    bool isTau = false;

    void iterate(); // always DC
    void runRelativeDC(bool timeStep, uns requestedIterations);
    void runDC(uns requestedIterations = 0);
    void runTimeStep(uns requestedIterations = 0);
    void runAC();
public:
    //***********************************************************************
    Simulation() = default;
    void run(const RunData& runData);
    //***********************************************************************

    //***********************************************************************
    void fillSaveData(SimulationToSaveData* dest) const noexcept{
    //***********************************************************************
        dest->analysisType = analysisType;
        dest->timeFreqValue = timeFreqValue;
        dest->dtValue = dtValue;
        dest->isTau = isTau;
    }
};


}


#endif
