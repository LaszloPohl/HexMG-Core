//***********************************************************************
// HexMG Instruction Stream Header
// Creation date:  2021. 07. 26.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HMG_INSTRUCTION_STREAM_HEADER
#define HMG_INSTRUCTION_STREAM_HEADER
//***********************************************************************


//***********************************************************************
#include <vector>
#include <list>
#include <string>
#include "hmgException.h"
#include "hmgCommon.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
inline CDNode SimpleInterfaceNodeID2CDNode(const SimpleInterfaceNodeID& src, const ExternalConnectionSizePack& externalNs, const InternalNodeVarSizePack& internalNs) {
//***********************************************************************
    CDNode res;
    uns delta = 0;
    switch (src.type) {
        case nvtIO:
            res.type = CDNodeType::cdntExternal;
            break;
        case nvtIN:
            res.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes;
            break;
        case nvtCIN:
            res.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes + externalNs.nNormalINodes;
            break;
        case nvtOUT:
            res.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes;
            break;
        case nvtFWOUT:
            res.type = CDNodeType::cdntExternal;
            delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes + externalNs.nNormalONodes;
            break;
        case nvtNInternal:
            res.type = CDNodeType::cdntInternal;
            break;
        case nvtCInternal:
            res.type = CDNodeType::cdntInternal;
            delta = internalNs.nNormalInternalNodes;
            break;
        case nvtRail:
            res.type = CDNodeType::cdntRail;
            break;
        case nvtGND:
            res.type = CDNodeType::cdntGnd;
            break;
        case nvtUnconnected:
            res.type = CDNodeType::cdntUnconnected;
            break;
        default:
            throw hmgExcept("SimpleInterfaceNodeID2CDNode", "not a node type (%u)", src.type);
    }
    res.index = src.index + delta;
    return res;
}


//***********************************************************************
inline CDParam ParameterInstance2CDParam(const ParameterInstance& src, const ExternalConnectionSizePack& externalNs, const InternalNodeVarSizePack& internalNs) {
//***********************************************************************
    CDParam res;
    uns delta = 0;
    res.value = src.value;
    switch (src.param.type) {
        case nvtNone:
            res.type = CDParamType::cdptValue;
            break;
        case nvtIO:
            res.type = CDParamType::cdptExternalNode;
            break;
        case nvtIN:
            res.type = CDParamType::cdptExternalNode;
            delta = externalNs.nIONodes;
            break;
        case nvtCIN:
            res.type = CDParamType::cdptExternalNode;
            delta = externalNs.nIONodes + externalNs.nNormalINodes;
            break;
        case nvtOUT:
            res.type = CDParamType::cdptExternalNode;
            delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes;
            break;
        case nvtFWOUT:
            res.type = CDParamType::cdptExternalNode;
            delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes + externalNs.nNormalONodes;
            break;
        case nvtNInternal:
            res.type = CDParamType::cdptInternalNode;
            break;
        case nvtCInternal:
            res.type = CDParamType::cdptInternalNode;
            delta = internalNs.nNormalInternalNodes;
            break;
        case nvtVarInternal:
            res.type = CDParamType::cdptLocalVariable;
            break;
        case nvtVarGlobal:
            res.type = CDParamType::cdptGlobalVariable;
            break;
        case nvtParam:
            res.type = CDParamType::cdptParam;
            break;
        default:
            throw hmgExcept("ParameterInstance2CDParam", "illegal type (%u)", src.param.type);
    }
    res.index = src.param.index + delta;
    return res;
}


//***********************************************************************
struct IsInstruction {
//***********************************************************************
    StreamInstructionType instruction;
    IsInstruction* next;
    IsInstruction(StreamInstructionType it) :instruction{ it }, next{ nullptr }{}
    virtual ~IsInstruction() {}
};


//***********************************************************************
struct IsNothingInstruction: public IsInstruction {
//***********************************************************************
    IsNothingInstruction() :IsInstruction{ sitNothing } {}
};


//***********************************************************************
struct IsDefSunredInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    uns levels = 0;
    bool isReplace = false;
    IsDefSunredInstruction(bool isReplace_, uns indx, uns lvls)
        :IsInstruction{ sitSunredTree }, isReplace{ isReplace_ }, index{ indx }, levels{ lvls } {}
};


//***********************************************************************
struct IsDefSunredLevelInstruction: public IsInstruction {
//***********************************************************************
    uns level = 0;
    uns nReductions = 0;
    IsDefSunredLevelInstruction(uns lvl, uns reds)
        :IsInstruction{ sitSunredLevel }, level{ lvl }, nReductions{ reds } {}
};


//***********************************************************************
struct IsDefSunredReductionInstruction: public IsInstruction {
//***********************************************************************
    ReductionInstruction reduction;
    IsDefSunredReductionInstruction(ReductionInstruction red)
        :IsInstruction{ sitSunredReduction }, reduction{ red } {}
};


//***********************************************************************
struct IsDefMultigridInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    uns nLocalNodeRestrictionTypes = 0;
    uns nLocalNodeProlongationTypes = 0;
    uns nLevels = 0;
    bool isReplace = false;
    IsDefMultigridInstruction(bool isReplace_, uns index_, uns nLocalNodeRestrictionTypes_, uns nLocalNodeProlongationTypes_, uns nLevels_)
        :IsInstruction{ sitMultigrid }, isReplace{ isReplace_ }, index{ index_ }, nLocalNodeRestrictionTypes{ nLocalNodeRestrictionTypes_ }, nLocalNodeProlongationTypes{ nLocalNodeProlongationTypes_ }, nLevels{ nLevels_ } {}
};


//***********************************************************************
struct IsDefMultigridLocalProlongationOrRestrictionInstructionsInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    uns nSimpleNodes = 0;
    uns nDeepNodes = 0;
    bool isRestrict = false;
    IsDefMultigridLocalProlongationOrRestrictionInstructionsInstruction(uns index_, bool isRestrictionType, uns nSimpleNods, uns nDeepNods)
        :IsInstruction{ sitMgLocals }, index{ index_ }, isRestrict{ isRestrictionType }, nSimpleNodes{ nSimpleNods }, nDeepNodes{ nDeepNods } {}
};


//***********************************************************************
struct IsDefMgLocalNodeInstruction : public IsInstruction {
//***********************************************************************
    uns destIndex = 0;
    uns nInstructions = 0;
    CDNode nodeID;
    IsDefMgLocalNodeInstruction(uns destIndx, const CDNode& nodeID_, uns nInstrs)
        :IsInstruction{ sitMgLocalSimple }, destIndex{ destIndx }, nodeID{ nodeID_ }, nInstructions{ nInstrs } {}
};


//***********************************************************************
struct IsDefMgOneLocalNodeInstruction : public IsInstruction {
//***********************************************************************
    bool isDestLevel = false;
    uns srcIndex = 0;
    CDNode nodeID;
    rvt weight = 0.5;
    IsDefMgOneLocalNodeInstruction(bool isDestLevel_, uns srcIndex_, const CDNode& nodeID_, rvt weight_)
        :IsInstruction{ sitMgOneLocalSimple }, isDestLevel{ isDestLevel_ }, srcIndex{ srcIndex_ }, nodeID{ nodeID_ }, weight{ weight_ } {}
};


//***********************************************************************
struct IsDefMgRecursiveInstruction : public IsInstruction {
//***********************************************************************
    CDNode nodeID;
    uns compDepth = 0;
    uns nInstr = 0;
    IsDefMgRecursiveInstruction(const CDNode& nodeID_, uns compDepth_, uns nInstr_)
        :IsInstruction{ sitMgRecursiveInstr }, nodeID{ nodeID_ }, compDepth{ compDepth_ }, nInstr{ nInstr_ } {}
};


//***********************************************************************
struct IsDefMgOneRecursiveInstruction : public IsInstruction {
//***********************************************************************
    bool isDestLevel = false;
    uns compDepth = 0;
    CDNode nodeID;
    rvt weight = 0.5; // inst.isDestLevel, inst.nodeID.nodeID, (uns)inst.nodeID.componentID.size(), inst.weight
    IsDefMgOneRecursiveInstruction(bool isDestLevel_, const CDNode& nodeID_, uns compDepth_, rvt weight_)
        :IsInstruction{ sitMgOneRecursiveInstr }, isDestLevel{ isDestLevel_ }, nodeID{ nodeID_ }, compDepth{ compDepth_ }, weight{ weight_ } {}
};


//***********************************************************************
struct IsDefMultigridFineCoarseConnectionInstruction : public IsInstruction {
//***********************************************************************
    uns indexFineFullCircuit = unsMax;
    uns indexCoarseFullCircuit = unsMax;
    uns nGlobalNodeRestrictions = 0;
    uns nGlobalNodeProlongations = 0;
    uns nComponentGroups = 0;
    IsDefMultigridFineCoarseConnectionInstruction(uns indexFineFullCircuit_, uns indexCoarseFullCircuit_, uns nGlobalNodeRestrictions_, uns nGlobalNodeProlongations_, uns nComponentGroups_)
        :IsInstruction{ sitMgFineCoarse }, indexFineFullCircuit{ indexFineFullCircuit_ }, indexCoarseFullCircuit{ indexCoarseFullCircuit_ }, 
        nGlobalNodeRestrictions{ nGlobalNodeRestrictions_ }, nGlobalNodeProlongations{ nGlobalNodeProlongations_ }, nComponentGroups{ nComponentGroups_ } {}
};


//***********************************************************************
struct IsDefMgNodeInstruction : public IsInstruction {
//***********************************************************************
    bool isRestrict = false;
    SimpleInterfaceNodeID nodeID;
    uns nInstr = 0;
    IsDefMgNodeInstruction(bool isRestrict_, const SimpleInterfaceNodeID& nodeID_, uns nInstr_)
        :IsInstruction{ sitMgNodeInstruction }, isRestrict{ isRestrict_ }, nodeID{ nodeID_ }, nInstr{ nInstr_ } {}
};


//***********************************************************************
struct IsDefMgOneInstruction : public IsInstruction {
//***********************************************************************
    uns srcIndex = 0;
    SimpleInterfaceNodeID nodeID;
    rvt weight = 0.5;
    IsDefMgOneInstruction(uns srcIndex_, const SimpleInterfaceNodeID& nodeID_, rvt weight_)
        :IsInstruction{ sitMgOne }, srcIndex{ srcIndex_ }, nodeID{ nodeID_ }, weight{ weight_ } {}
};


//***********************************************************************
struct IsDefMultigridComponentGroupInstruction : public IsInstruction {
//***********************************************************************
    bool isCopy = true;
    uns localRestrictionIndex = 0;
    uns localProlongationIndex = 0;
    uns nFineCells = 0;
    uns nCoarseCells = 0;
    IsDefMultigridComponentGroupInstruction(bool isCopy_, uns localRestrictionIndex_, uns localProlongationIndex_, uns nFineCells_, uns nCoarseCells_)
        :IsInstruction{ sitMgComponentGroup }, isCopy{ isCopy_ }, localRestrictionIndex{ localRestrictionIndex_ },
        localProlongationIndex{ localProlongationIndex_ }, nFineCells{ nFineCells_ }, nCoarseCells{ nCoarseCells_ } {}
};


//***********************************************************************
struct IsDefRailsInstruction: public IsInstruction {
//***********************************************************************
    uns rails = 0;
    uns nRailValues = 0;
    IsDefRailsInstruction(uns rails_, uns nValues)
        :IsInstruction{ sitRails }, rails{ rails_ }, nRailValues{ nValues } {}
};


//***********************************************************************
struct IsDefRailValueInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    double value = 0.0;
    IsDefRailValueInstruction(uns indx, double val)
        :IsInstruction{ sitRailValue }, index{ indx }, value{ val } {}
};


//***********************************************************************
struct IsDefModelSubcircuitInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    ExternalConnectionSizePack externalNs;
    InternalNodeVarSizePack internalNs;
    SolutionType solutionType = stFullMatrix;
    uns solutionDescriptionIndex = 0; // for sunred and multigrid 
    bool isReplace = false;
    IsDefModelSubcircuitInstruction(bool isReplace_, uns index_, ExternalConnectionSizePack externalNs_, InternalNodeVarSizePack internalNs_, SolutionType solutionType_, uns sdi)
        :IsInstruction{ sitDefModelSubcircuit }, isReplace{ isReplace_ }, index{ index_ }, externalNs{ externalNs_ },
        internalNs{ internalNs_ }, solutionType{ solutionType_ }, solutionDescriptionIndex{ sdi } {}
};


//***********************************************************************
struct IsDefModelControllerInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    bool isReplace = false;
    IsDefModelControllerInstruction(bool isReplace_, uns indx)
        :IsInstruction{ sitDefModelController }, isReplace{ isReplace_ }, index{ indx } {}
};


//***********************************************************************
struct IsEndDefInstruction: public IsInstruction {
//***********************************************************************
    uns index;
    StreamInstructionType whatEnds;
    IsEndDefInstruction(StreamInstructionType what, unsigned indx) :IsInstruction{ sitEndInstruction }, whatEnds{ what }, index{ indx } {}
};


//***********************************************************************
struct IsComponentInstanceInstruction : public IsInstruction {
//***********************************************************************
    uns instanceIndex = 0; // component index or controller index
    uns modelIndex = 0; // in CircuitStorage::models or CircuitStorage::builtInModels

    bool isDefaultRail = false;
    bool isController = false;
    bool isBuiltIn = false;
    uns nNodes = 0;
    uns nParams = 0;
    uns defaultValueRailIndex = 0;

    IsComponentInstanceInstruction(uns index, uns modelIndex_, bool isDefRail, uns defRail, bool isCntrller, bool isBltIn, uns nNode, uns nPar)
        :IsInstruction{ sitComponentInstance }, instanceIndex{ index }, modelIndex{ modelIndex_ }, isDefaultRail{ isDefRail }, 
        isController{ isCntrller }, isBuiltIn{ isBltIn }, nNodes{ nNode }, nParams{ nPar }, defaultValueRailIndex{ defRail } {}
};


//***********************************************************************
struct IsRailNodeRangeInstruction: public IsInstruction {
//***********************************************************************
    ForcedNodeDef forcedNodeRange;
    IsRailNodeRangeInstruction(const ForcedNodeDef& forcedNodeRnge)
        :IsInstruction{ sitRailRange }, forcedNodeRange{ forcedNodeRnge } {}
};


//***********************************************************************
struct IsNodeValueInstruction: public IsInstruction {
//***********************************************************************
    SimpleInterfaceNodeID nodeID;
    IsNodeValueInstruction(const SimpleInterfaceNodeID& node) :IsInstruction{ sitNodeValue }, nodeID{ node }{}
};


//***********************************************************************
struct IsParameterValueInstruction: public IsInstruction {
//***********************************************************************
    ParameterInstance param;
    IsParameterValueInstruction(const ParameterInstance& par) :IsInstruction{ sitParameterValue }, param{ par }{}
};


//***********************************************************************
struct IsFunctionInstruction: public IsInstruction {
//***********************************************************************
    unsigned index, size;
    IsFunctionInstruction(unsigned indx, unsigned siz) :IsInstruction{ sitFunction }, index{ indx }, size{ siz }{}
};


//***********************************************************************
struct IsCreateInstruction: public IsInstruction {
//***********************************************************************
    uns fullCircuitIndex = 0;
    uns modelID = 0;
    uns GND = 0; // Rail ID
    IsCreateInstruction(uns indx, uns model, uns gndRail) :IsInstruction{ sitCreate }, fullCircuitIndex{ indx }, modelID{ model }, GND{ gndRail } {}
};


//***********************************************************************
struct IsProbeInstruction: public IsInstruction {
//***********************************************************************
    uns probeIndex = 0;
    uns probeType = ptV;
    uns fullCircuitID = 0;
    uns nNodes = 0;
    IsProbeInstruction(uns indx, uns type, uns fullCkt, uns nNode) :IsInstruction{ sitProbe }, probeIndex{ indx }, probeType{ type }, fullCircuitID{ fullCkt }, nNodes{ nNode } {}
};


//***********************************************************************
struct IsProbeNodeInstruction: public IsInstruction {
//***********************************************************************
    DeepInterfaceNodeID nodeID;
    IsProbeNodeInstruction(const DeepInterfaceNodeID& node) :IsInstruction{ sitProbeNode }, nodeID{ node }{}
};


//***********************************************************************
struct IsRunInstruction: public IsInstruction {
//***********************************************************************
    RunData data;
    IsRunInstruction(const RunData& runData) :IsInstruction{ sitRun }, data{ runData }{}
};


//***********************************************************************
struct IsSaveInstruction: public IsInstruction {
//***********************************************************************
    bool isRaw = false;
    bool isAppend = false;
    uns maxResultsPerRow = 100;
    char fileName[4000] = { '\0' };
    uns nProbeIDs = 0;
    IsSaveInstruction(bool isRw, bool isAppnd, uns maxResultsPerRow_, std::string& fleName, uns nProbe)
        :IsInstruction{ sitSave }, isRaw{ isRw }, isAppend{ isAppnd }, maxResultsPerRow{ maxResultsPerRow_ }, nProbeIDs{ nProbe } {
        strcpy_s(fileName, fleName.c_str());
    }
};


//***********************************************************************
struct IsUnsInstruction: public IsInstruction {
//***********************************************************************
    uns data;
    IsUnsInstruction(uns Data) :IsInstruction{ sitUns }, data{ Data }{}
};


//***********************************************************************
class InstructionStream {
//***********************************************************************
    IsNothingInstruction listDummy;
    IsInstruction * pListLast;
public:
    //***********************************************************************
    InstructionStream() :pListLast { &listDummy } {}
    //***********************************************************************
    ~InstructionStream() { clear(); }
    //***********************************************************************
    void clear() {
    //***********************************************************************
        IsInstruction* it = listDummy.next;
        while (it != nullptr) {
            IsInstruction* temp = it->next;
            delete it;
            it = temp;
        }
        pListLast = &listDummy;
    }
    //***********************************************************************
    void add(IsInstruction* pNew) {
    //***********************************************************************
        if (pNew != nullptr) {
            pListLast->next = pNew;
            pListLast = pNew;
        }
    }
    //***********************************************************************
    IsInstruction* extractList(IsInstruction*& retLast) {
    //***********************************************************************
        IsInstruction* temp = listDummy.next;
        retLast = temp == nullptr ? nullptr : pListLast;
        listDummy.next = nullptr;
        pListLast = &listDummy;
        return temp;
    }
};


}


#endif
