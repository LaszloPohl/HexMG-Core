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
    uns nVcycles = 2;
    uns nPreSmoothings = 1000;
    uns nPostSmoothings = 1000;
    rvt errorRate = 1.0 / 3.0;

    std::vector<LocalProlongationOrRestrictionInstructions> localNodeRestrictionTypes;
    std::vector<LocalProlongationOrRestrictionInstructions> localNodeProlongationTypes;
    std::vector<FineCoarseConnectionDescription> levels; // 0 is the coarsest multigrid level, sunred level is not included because these are the destination levels

    //***********************************************************************
    void solveDC() {
    //***********************************************************************
        CircuitStorage& gc = CircuitStorage::getInstance();
        gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->deleteF(true);
        gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->solveDC();
        for (uns iDestLevel = 0; iDestLevel < levels.size(); iDestLevel++) {
            const FineCoarseConnectionDescription& destLevel = levels[iDestLevel];

            gc.fullCircuitInstances[destLevel.indexFineFullCircuit].component->prolongateUDC(destLevel, *this);

            for (uns iV = 0; iV < nVcycles; iV++) {
                rvt truncationError = rvt0;

                // downward line of the V

                uns iDown = iDestLevel + 1;
                do {
                    iDown--; // iDestLevel to 0, iDestLevel and 0 included (0 is the coarsest destination(!) level)
                    
                    const FineCoarseConnectionDescription& hLevel = levels[iDown];
                    ComponentSubCircuit& hGrid = *gc.fullCircuitInstances[hLevel.indexFineFullCircuit].component;
                    ComponentSubCircuit& HGrid = *gc.fullCircuitInstances[hLevel.indexCoarseFullCircuit].component;

                    // TODO: controllers; where?

                    // relax

                    hGrid.relaxDC(nPreSmoothings);
                    //hGrid.printNodesDC();
                    //HGrid.printNodesDC();

                    // Lh

                    hGrid.calculateValueDC();
                    hGrid.deleteD(true);
                    hGrid.calculateCurrent(true);

                    // R(uh)

                    hGrid.restrictUDC(hLevel, *this);

                    // LH

                    HGrid.calculateValueDC();
                    HGrid.deleteD(true);
                    HGrid.calculateCurrent(true);

                    // fH

                    rvt truncErr = hGrid.restrictFDDC(hLevel, *this); // fH = R(fh) + dH – R(dh)
                    if (iDown == iDestLevel)
                        truncationError = truncErr;

                } while (iDown != 0);

                // solve the coarsest grid (= level -1)

                gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->solveDC(); // d0 += f0 kell!

                // upward line of the V

                for (uns iUp = 0; iUp <= iDestLevel; iUp++) {
                    
                    const FineCoarseConnectionDescription& hLevel = levels[iUp];
                    ComponentSubCircuit& hGrid = *gc.fullCircuitInstances[hLevel.indexFineFullCircuit].component;
                    ComponentSubCircuit& HGrid = *gc.fullCircuitInstances[hLevel.indexCoarseFullCircuit].component;

                    // uh

                    hGrid.uHMinusRestrictUhToDHNCDC(hLevel, *this); // dH_NonConcurrent = uH – R(uh)
                    hGrid.prolongateDHNCAddToUhDC(hLevel, *this);   // uh += P(dH_NonConcurrent)

                    // relax

                    hGrid.relaxDC(nPostSmoothings);
                }

                // residual

                ComponentSubCircuit& destGrid = *gc.fullCircuitInstances[destLevel.indexFineFullCircuit].component;
                destGrid.calculateValueDC();
                destGrid.deleteD(true);
                destGrid.calculateCurrent(true);
                rvt residual = destGrid.calculateResidual(true); // the residual and the truncationError would be (sqrt(this thing) / nodenum) but not needed
                //if (residual < errorRate * truncationError)
                    iV = nVcycles; // break
            }
        }
    }
};


}

#endif