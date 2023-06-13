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
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class Simulation {
//***********************************************************************
    AnalysisType analysisType = atDC;
    rvt timeFreqValue = rvt0;
    rvt dtValue = rvt0;
    rvt err = rvt0;

    void ananlysis(IsInstruction* instruction);
    void iterate(); // always DC
    void runDC();
    void runTimeStep();
    void runAC();
    void runTimeConst();
public:
    Simulation() = default;
    void run(const RunData& runData);
};



}


#endif
