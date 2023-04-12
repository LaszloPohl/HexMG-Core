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
#include "hmgComponent.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
struct hmgMultigrid {
//***********************************************************************
    uns nVcycles = 1;
    uns nPreSmoothings = 1;
    uns nPostSmoothings = 1;

    std::vector<LocalRestrictionInstructions>  localNodeRestrictionTypes;
    std::vector<LocalProlongationInstructions> localNodeProlongationTypes;
    std::vector<FineCoarseConnectionDescription> levels;
};


}

#endif