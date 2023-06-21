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
#include "hmgInstructionStream.h"
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

    void processInstructions(IsInstruction*& first);
};


//***********************************************************************
struct InterfaceLocalProlongationOrRestrictionInstructions {
//***********************************************************************
    //***********************************************************************
    struct OneLocalInstruction {
    //***********************************************************************
        bool isDestLevel = false;       // using an already calculated external node in the destination level
        uns srcIndex = 0;               // index Prolongation: in ComponentGroup::coarseCells (isFine == false) or ComponentGroup::fineCells (isFine == true), Restriction: in ComponentGroup::fineCells
        SimpleInterfaceNodeID nodeID;   // cdntInternal and cdntExternal allowed
        rvt weight = 0.5;
    };
    //***********************************************************************
    struct LocalNodeInstruction {
    //***********************************************************************
        uns destIndex = 0;              // index Prolongation: in ComponentGroup::fineCells, Restriction: in ComponentGroup::coarseCells
        SimpleInterfaceNodeID nodeID;   // cdntInternal and cdntExternal allowed
        std::vector<OneLocalInstruction> instr; // sum weight should be 1

        //***********************************************************************
        void toInstructionStream(InstructionStream& iStream) const {
        //***********************************************************************
            iStream.add(new IsDefMgLocalNodeInstruction(destIndex, nodeID, (uns)instr.size()));
            for(const auto& inst : instr)
                iStream.add(new IsDefMgOneLocalNodeInstruction(inst.isDestLevel, inst.srcIndex, inst.nodeID, inst.weight));
            iStream.add(new IsEndDefInstruction(sitMgLocalSimple, 0));
        }
    };
    //***********************************************************************
    struct OneRecursiveInstruction {
    //***********************************************************************
        bool isDestLevel = false;       // using an already calculated external node in the destination level
        DeepInterfaceNodeID nodeID;
        rvt weight = 0.5;
    };
    //***********************************************************************
    struct RecursiveInstruction {
    //***********************************************************************
        DeepInterfaceNodeID nodeID;
        std::vector<OneRecursiveInstruction> instr; // sum weight should be 1

        //***********************************************************************
        void toInstructionStream(InstructionStream& iStream) const {
        //***********************************************************************
            iStream.add(new IsDefMgRecursiveInstruction(nodeID.nodeID, (uns)nodeID.componentID.size(), (uns)instr.size()));
            for (uns compID : nodeID.componentID)
                iStream.add(new IsUnsInstruction(compID));
            for (const auto& inst : instr) {
                iStream.add(new IsDefMgOneRecursiveInstruction(inst.isDestLevel, inst.nodeID.nodeID, (uns)inst.nodeID.componentID.size(), inst.weight));
                for (uns compID : inst.nodeID.componentID)
                    iStream.add(new IsUnsInstruction(compID));
            }
            iStream.add(new IsEndDefInstruction(sitMgLocalSimple, 0));
        }
    };
    //***********************************************************************
    std::vector<LocalNodeInstruction> destComponentsNodes;
    std::vector<RecursiveInstruction> deepDestComponentNodes; // subckt => subckt => ... => subckt => component => internal node
    //***********************************************************************

    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream, bool isRestrictionType, uns index) const {
    //***********************************************************************
        iStream.add(new IsDefMultigridLocalProlongationOrRestrictionInstructionsInstruction(index, isRestrictionType, (uns)destComponentsNodes.size(), (uns)deepDestComponentNodes.size()));
        for (const auto& nodes : destComponentsNodes)
            nodes.toInstructionStream(iStream);
        for (const auto& nodes : deepDestComponentNodes)
            nodes.toInstructionStream(iStream);
        iStream.add(new IsEndDefInstruction(sitMgLocals, 0));
    }
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
struct InterfaceNodeInstruction {
//***********************************************************************
    //***********************************************************************
    struct OneInstruction {
    //***********************************************************************
        uns srcComponentIndex = unsMax; // if unsMax => global node
        SimpleInterfaceNodeID nodeID;
        rvt weight = 0.5;
    };

    //***********************************************************************
    SimpleInterfaceNodeID nodeID;
    std::vector<OneInstruction> instr; // sum weight should be 1
    //***********************************************************************

    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream) const {
    //***********************************************************************
        iStream.add(new IsDefMgNodeInstruction(nodeID, (uns)instr.size()));
        for (const auto& inst : instr) {
            iStream.add(new IsDefMgOneInstruction(inst.srcComponentIndex, inst.nodeID, inst.weight));
        }
        iStream.add(new IsEndDefInstruction(sitMgNodeInstruction, 0));
    }
};


//***********************************************************************
struct ComponentGroup {
//***********************************************************************
    //***********************************************************************
    std::vector<uns> fineCells;     // index in the fine->components vector
    std::vector<uns> coarseCells;   // index in the coarse->components vector
    bool isCopy = true;             // if true, coarseCells[i] == fineCells[i] required; prolongation and restriction is the copiing of the values
    uns localRestrictionIndex = 0;  // in localNodeRestrictionTypes
    uns localProlongationIndex = 0; // in localNodeProlongationTypes
    //***********************************************************************

    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream) const {
    //***********************************************************************
        iStream.add(new IsDefMultigridComponentGroupInstruction(isCopy, localRestrictionIndex, localProlongationIndex, (uns)fineCells.size(), (uns)coarseCells.size()));
        for (uns cell : fineCells)
            iStream.add(new IsUnsInstruction(cell));
        for (uns cell : coarseCells)
            iStream.add(new IsUnsInstruction(cell));
        iStream.add(new IsEndDefInstruction(sitMgComponentGroup, 0));
    }
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
struct InterfaceFineCoarseConnectionDescription {
//***********************************************************************
    //***********************************************************************
    uns indexFineFullCircuit = unsMax;
    uns indexCoarseFullCircuit = unsMax;
    std::vector<ComponentGroup> componentGroups;
    std::vector<InterfaceNodeInstruction> globalNodeRestrictions;
    std::vector<InterfaceNodeInstruction> globalNodeProlongations;
    //***********************************************************************

    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream) const {
    //***********************************************************************
        iStream.add(new IsDefMultigridFineCoarseConnectionInstruction(indexFineFullCircuit,
            indexCoarseFullCircuit, (uns)globalNodeRestrictions.size(), (uns)globalNodeProlongations.size(), (uns)componentGroups.size()));
        for (const auto& nodes : globalNodeRestrictions)
            nodes.toInstructionStream(iStream);
        for (const auto& nodes : globalNodeProlongations)
            nodes.toInstructionStream(iStream);
        for (const auto& group : componentGroups)
            group.toInstructionStream(iStream);
        iStream.add(new IsEndDefInstruction(sitMgFineCoarse, 0));
    }
};


//***********************************************************************
struct hmgMultigrid;
//***********************************************************************


}

#endif