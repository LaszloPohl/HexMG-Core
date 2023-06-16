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
inline CDNode SimpleNodeID2CDNode(const SimpleNodeID& src, const ExternalConnectionSizePack& externalNs, const InternalNodeVarSizePack& internalNs) {
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
            throw hmgExcept("SimpleNodeID2CDNode", "not a node type (%u)", src.type);
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
    SimpleNodeID nodeID;
    IsNodeValueInstruction(const SimpleNodeID& node) :IsInstruction{ sitNodeValue }, nodeID{ node }{}
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
    ProbeNodeID nodeID;
    IsProbeNodeInstruction(const ProbeNodeID& node) :IsInstruction{ sitProbeNode }, nodeID{ node }{}
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
