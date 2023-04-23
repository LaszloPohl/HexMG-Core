//***********************************************************************
// HexMG Multigrid Types
// Creation date:  2023. 04. 12.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_MULTIGRIDTYPES_HEADER
#define	HMG_MULTIGRIDTYPES_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgCommon.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
enum RecursiveProlongRestrictType { rprProlongateU, rprRestrictU, 
    rprRestrictFDD, rpruHMinusRestrictUhToDHNC, rprProlongateDHNCAddToUh
};
//***********************************************************************


//***********************************************************************
struct LocalProlongationOrRestrictionInstructions {
//***********************************************************************
    struct OneLocalInstruction {
        bool isDestLevel = false;// using an already calculated external node in the destination level
        bool isExternal = false; // using an external node of the srcIndex component
        uns srcIndex = 0;        // index Prolongation: in ComponentGroup::coarseCells (isFine == false) or ComponentGroup::fineCells (isFine == true), Restriction: in ComponentGroup::fineCells
        uns nodeIndex = 0;       // externalNodeFlag is not set, isExternal decides
        rvt weight = 0.5;
    };
    struct LocalNodeInstruction {
        bool isExternal = false; // using an external node of the fineIndex component
        uns destIndex = 0;       // index Prolongation: in ComponentGroup::fineCells, Restriction: in ComponentGroup::coarseCells
        uns nodeIndex = 0;       // externalNodeFlag is not set, isExternal decides
        std::vector<OneLocalInstruction> instr; // sum weight should be 1
    };
    struct OneRecursiveInstruction {
        bool isDestLevel = false;// using an already calculated external node in the destination level
        bool isExternal = false; // using an external node of the last srcComponentIndex component
        std::vector<uns> srcComponentIndex; // all are subckts, except the last one, which is a component (of course it can be a subckt, but it doesn't have to be)
        uns nodeIndex = 0;
        rvt weight = 0.5;
    };
    struct RecursiveInstruction {
        bool isExternal = false;             // using an external node of the fineIndex component
        std::vector<uns> destComponentIndex; // all are subckts, except the last one, which is a component (of course it can be a subckt, but it doesn't have to be)
        uns nodeIndex = 0;
        std::vector<OneRecursiveInstruction> instr; // sum weight should be 1
    };
    std::vector<LocalNodeInstruction> destComponentsNodes;
    std::vector<RecursiveInstruction> deepDestComponentNodes; // subckt => subckt => ... => subckt => component => internal node
};


//***********************************************************************
struct NodeInstruction {
//***********************************************************************
    struct OneInstruction {
        uns srcNodeIndex = 0;
        rvt weight = 0.5;
    };
    uns destNodeIndex = 0;
    std::vector<OneInstruction> instr; // sum weight should be 1
};


//***********************************************************************
struct ComponentGroup {
//***********************************************************************
    std::vector<uns> fineCells;     // index in the fine->components vector
    std::vector<uns> coarseCells;   // index in the coarse->components vector
    bool isCopy = true;             // if true, coarseCells[1] == fineCells[i] required; prolongation and restriction is the copiing of the values
    uns localRestrictionIndex = 0;  // in localNodeRestrictionTypes, if isNormalRestriction == false
    uns localProlongationIndex = 0; // in localNodeProlongationTypes, always
};


//***********************************************************************
struct FineCoarseConnectionDescription {
//***********************************************************************
    uns indexFineFullCircuit = 0;
    uns indexCoarseFullCircuit = 0;
    std::vector<ComponentGroup> componentGroups;
    std::vector<NodeInstruction> globalNodeRestrictions;
    std::vector<NodeInstruction> globalNodeProlongations;
};


//***********************************************************************
struct hmgMultigrid;
//***********************************************************************


}

#endif