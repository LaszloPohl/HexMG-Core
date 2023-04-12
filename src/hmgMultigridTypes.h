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
struct LocalRestrictionInstructions {
//***********************************************************************
    struct OneLocalRestrictionInstruction {
        uns fineIndex = 0; // index in ComponentGroup::fineCells
        uns nodeIndex = 0;
        rvt weight = 0.5;
    };
    struct LocalNodeRestrictionInstruction {
        uns coarseIndex = 0; // index in ComponentGroup::coarseCells
        uns nodeIndex = 0;
        std::vector<OneLocalRestrictionInstruction> instr; // sum weight should be 1
    };
    std::vector<LocalNodeRestrictionInstruction> restrictions;
};


//***********************************************************************
struct LocalProlongationInstructions {
//***********************************************************************
    struct OneLocalProlongationInstruction {
        uns coarseIndex = 0; // index in ComponentGroup::coarseCells
        uns nodeIndex = 0;
        rvt weight = 0.5;
    };
    struct LocalNodeProlongationInstruction {
        uns fineIndex = 0; // index in ComponentGroup::fineCells
        uns nodeIndex = 0;
        std::vector<OneLocalProlongationInstruction> instr; // sum weight should be 1
    };
    std::vector<LocalNodeProlongationInstruction> restrictions;
};


//***********************************************************************
struct OneRestrictionInstruction {
//***********************************************************************
    uns fineNodeIndex = 0;
    rvt weight = 0.5;
};


//***********************************************************************
struct NodeRestrictionInstruction {
//***********************************************************************
    uns coarseNodeIndex = 0;
    std::vector<OneRestrictionInstruction> instr; // sum weight should be 1
};


//***********************************************************************
struct OneProlongationInstruction {
//***********************************************************************
    uns coarseNodeIndex = 0;
    rvt weight = 0.5;
};


//***********************************************************************
struct NodeProlongationInstruction {
//***********************************************************************
    uns fineNodeIndex = 0;
    std::vector<OneProlongationInstruction> instr; // sum weight should be 1
};


//***********************************************************************
struct ComponentGroup {
//***********************************************************************
    std::vector<uns> fineCells;     // index in the fine->components vector
    std::vector<uns> coarseCells;   // index in the coarse->components vector
    bool isNormalRestriction = true;// if true, coarseCells.size() must be 1. If true, coarseCells[0].internalNodes[i] = sum (fineCells[j].internalNodes[i]) / fineCells.size()
    uns localRestrictionIndex = 0;  // in localNodeRestrictionTypes, if isNormalRestriction == false
    uns localProlongationIndex = 0; // in localNodeProlongationTypes, always
};


//***********************************************************************
struct FineCoarseConnectionDescription {
//***********************************************************************
    uns indexFineFullCircuit = 0;
    uns indexCoarseFullCircuit = 0;
    std::vector<ComponentGroup> componentGroups;
    std::vector<NodeRestrictionInstruction> globalNodeRestrictions;
    std::vector<NodeProlongationInstruction> globalNodeProlongations;
};


}

#endif