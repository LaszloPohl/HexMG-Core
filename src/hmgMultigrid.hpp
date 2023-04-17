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
    uns nPreSmoothings = 2; // 2 because of Jacobi iteration instead of Gauss-Seidel
    uns nPostSmoothings = 2;
    rvt errorRate = 1.0 / 3.0;

    std::vector<LocalRestrictionInstructions>  localNodeRestrictionTypes;
    std::vector<LocalProlongationInstructions> localNodeProlongationTypes;
    std::vector<FineCoarseConnectionDescription> levels; // 0 is the coarsest multigrid level, sunred level is not included because these are the destination levels

    //***********************************************************************
    void solveDC() {
    //***********************************************************************
        CircuitStorage& gc = CircuitStorage::getInstance();
        gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->deleteF(true);
        gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->solveDC();
        for (uns iDestLevel = 0; iDestLevel < levels.size(); iDestLevel++) {
            const FineCoarseConnectionDescription& destLevel = levels[iDestLevel];

            gc.fullCircuitInstances[destLevel.indexFineFullCircuit].component->prolongateUDC(destLevel);

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

                    // Lh

                    hGrid.calculateValueDC();
                    hGrid.loadFtoD(true);
                    hGrid.calculateCurrent(true); // d + f

                    // R(uh)

                    hGrid.restrictUDC(hLevel);

                    // LH

                    HGrid.calculateValueDC();
                    HGrid.loadFtoD(true);
                    HGrid.calculateCurrent(true); // d + f

                    // fH

                    rvt truncErr = hGrid.restrictFDDC(hLevel); // fH = R(fh) + dH – R(dh)
                    if (iDown == iDestLevel)
                        truncationError = truncErr;

                } while (iDown != 0);

                // solve the coarsest grid (= level -1)

                gc.fullCircuitInstances[levels[0].indexCoarseFullCircuit].component->solveDC(); // d0 += f0 kell!

                // upward line of the V

                for (uns iUp = 0; iUp <= iDestLevel; iUp++) {
                    
                    const FineCoarseConnectionDescription& hLevel = levels[iUp];
                    ComponentSubCircuit& hGrid = *gc.fullCircuitInstances[hLevel.indexFineFullCircuit].component;

                    // uh

                    hGrid.uHMinusRestrictUhToDHNCDC(hLevel); // dH_NonConcurent = uH – R(uh)
                    hGrid.prolongateDHNCAddToUhDC(hLevel);   // uh += P(dH_NonConcurent)

                    // relax

                    hGrid.relaxDC(nPostSmoothings);
                }

                // residual

                ComponentSubCircuit& destGrid = *gc.fullCircuitInstances[destLevel.indexFineFullCircuit].component;
                destGrid.calculateValueDC();
                destGrid.calculateCurrent(true); // no loadFtoD => only d
                rvt residual = destGrid.calculateResidualDC(); // the residual and the truncationError would be (sqrt(this thing) / nodenum) but not needed
                if (residual < errorRate * truncationError)
                    iV = nVcycles; // break
            }
        }
    }
};


}

#endif