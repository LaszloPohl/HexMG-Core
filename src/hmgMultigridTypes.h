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
        uns srcIndex = 0;        // index Prolongation: in ComponentGroup::coarseCells (isFine == false) or ComponentGroup::fineCells (isFine == true), Restriction: in ComponentGroup::fineCells
        CDNode nodeID;           // cdntInternal and cdntExternal allowed
        rvt weight = 0.5;
    };
    struct LocalNodeInstruction {
        uns destIndex = 0;       // index Prolongation: in ComponentGroup::fineCells, Restriction: in ComponentGroup::coarseCells
        CDNode nodeID;           // cdntInternal and cdntExternal allowed
        std::vector<OneLocalInstruction> instr; // sum weight should be 1
    };
    struct OneRecursiveInstruction {
        bool isDestLevel = false;// using an already calculated external node in the destination level
        DeepCDNodeID nodeID;
        rvt weight = 0.5;
    };
    struct RecursiveInstruction {
        DeepCDNodeID nodeID;
        std::vector<OneRecursiveInstruction> instr; // sum weight should be 1
    };
    std::vector<LocalNodeInstruction> destComponentsNodes;
    std::vector<RecursiveInstruction> deepDestComponentNodes; // subckt => subckt => ... => subckt => component => internal node
};


//***********************************************************************
struct InterfaceLocalProlongationOrRestrictionInstructions {
//***********************************************************************
    struct OneLocalInstruction {
        bool isDestLevel = false;       // using an already calculated external node in the destination level
        uns srcIndex = 0;               // index Prolongation: in ComponentGroup::coarseCells (isFine == false) or ComponentGroup::fineCells (isFine == true), Restriction: in ComponentGroup::fineCells
        SimpleInterfaceNodeID nodeID;   // cdntInternal and cdntExternal allowed
        rvt weight = 0.5;
    };
    struct LocalNodeInstruction {
        uns destIndex = 0;              // index Prolongation: in ComponentGroup::fineCells, Restriction: in ComponentGroup::coarseCells
        SimpleInterfaceNodeID nodeID;   // cdntInternal and cdntExternal allowed
        std::vector<OneLocalInstruction> instr; // sum weight should be 1
    };
    struct OneRecursiveInstruction {
        bool isDestLevel = false;       // using an already calculated external node in the destination level
        DeepInterfaceNodeID nodeID;
        rvt weight = 0.5;
    };
    struct RecursiveInstruction {
        DeepInterfaceNodeID nodeID;
        std::vector<OneRecursiveInstruction> instr; // sum weight should be 1
    };
    std::vector<LocalNodeInstruction> destComponentsNodes;
    std::vector<RecursiveInstruction> deepDestComponentNodes; // subckt => subckt => ... => subckt => component => internal node
};


//***********************************************************************
struct NodeInstruction {
//***********************************************************************
    struct OneInstruction {
        uns srcComponentIndex = unsMax; // if unsMax => global node
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
    bool isCopy = true;             // if true, coarseCells[i] == fineCells[i] required; prolongation and restriction is the copiing of the values
    uns localRestrictionIndex = 0;  // in localNodeRestrictionTypes
    uns localProlongationIndex = 0; // in localNodeProlongationTypes
};


//***********************************************************************
struct FineCoarseConnectionDescription {
//***********************************************************************
    uns indexFineFullCircuit = unsMax;
    uns indexCoarseFullCircuit = unsMax;
    std::vector<ComponentGroup> componentGroups;
    std::vector<NodeInstruction> globalNodeRestrictions;
    std::vector<NodeInstruction> globalNodeProlongations;
};


//***********************************************************************
struct hmgMultigrid;
//***********************************************************************


}

#endif