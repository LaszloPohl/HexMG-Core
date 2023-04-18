//***********************************************************************
// HexMG Multigrid Types
// Creation date:  2023. 04. 12.
// Creator:        Pohl L�szl�
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
struct LocalProlongationOrRestrictionInstructions {
//***********************************************************************
    struct OneLocalInstruction {
        bool isFine = false;     // Prolongation only: using an already calculated external node in the fine level
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
        bool isFine = false;     // Prolongation only: using an already calculated external node in the fine level
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
    std::vector<LocalNodeInstruction> components;
    std::vector<RecursiveInstruction> deepComponents; // subckt => subckt => ... => subckt => component => internal node
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
    std::vector<NodeInstruction> globalNodeRestrictions;
    std::vector<NodeInstruction> globalNodeProlongations;
};


//***********************************************************************
struct hmgMultigrid;
//***********************************************************************


}

#endif