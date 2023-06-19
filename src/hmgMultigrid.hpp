//***********************************************************************
// HexMG Multigrid Engine
// Creation date:  2023. 04. 12.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_MULTIGRID_HEADER
#define	HMG_MULTIGRID_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgCommon.h"
#include "hmgMatrix.hpp"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
struct hmgMultigrid {
//***********************************************************************
    uns nVcycles = 2;
    uns nPreSmoothings = 1000;
    uns nPostSmoothings = 1000;
    rvt errorRate = 1.0 / 3.0;

    std::vector<LocalProlongationOrRestrictionInstructions> localNodeRestrictionTypes;
    std::vector<LocalProlongationOrRestrictionInstructions> localNodeProlongationTypes;
    std::vector<FineCoarseConnectionDescription> levels; // 0 is the coarsest multigrid level, sunred level is not included because these are the destination levels

    //***********************************************************************
    void solveDC();
    //***********************************************************************
};


}

#endif