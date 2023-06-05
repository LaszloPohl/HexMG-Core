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
    IsDefSunredInstruction(uns indx, uns lvls)
        :IsInstruction{ sitSunredTree }, index{ indx }, levels{ lvls } {}
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
    IsDefRailsInstruction(uns rails_)
        :IsInstruction{ sitRails }, rails{ rails_ } {}
};


//***********************************************************************
struct IsDefRailValueInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    double value = 0.0;
    IsDefRailValueInstruction(uns indx, double val)
        :IsInstruction{ sitRails }, index{ indx }, value{ val } {}
};


//***********************************************************************
struct IsDefModelSubcircuitInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    ExternalConnectionSizePack externalNs;
    uns sumExternalNodes = 0;
    InternalNodeVarSizePack internalNs;
    uns sumInternalNodes = 0;
    SolutionType solutionType = stFullMatrix;
    uns solutionDescriptionIndex = 0; // for sunred and multigrid 
    IsDefModelSubcircuitInstruction(uns index_, ExternalConnectionSizePack externalNs_, uns sX, InternalNodeVarSizePack internalNs_, uns sI, SolutionType solutionType_, uns sdi)
        :IsInstruction{ sitDefModelSubcircuit }, index{ index_ }, externalNs{ externalNs_ }, sumExternalNodes{ sX }, 
        internalNs{ internalNs_ }, sumInternalNodes{ sI }, solutionType{ solutionType_ }, solutionDescriptionIndex{ sdi } {}
};


//***********************************************************************
struct IsDefModelControllerInstruction: public IsInstruction {
//***********************************************************************
    uns index = 0;
    IsDefModelControllerInstruction(uns indx)
        :IsInstruction{ sitDefModelController }, index{ indx } {}
};


//***********************************************************************
struct IsEndDefInstruction: public IsInstruction {
//***********************************************************************
    unsigned index;
    IsEndDefInstruction(StreamInstructionType what, unsigned indx) :IsInstruction{ what }, index{ indx } {}
};


//***********************************************************************
struct IsReplaceInstruction: public IsInstruction {
//***********************************************************************
    uns index;
    uns data;
    IsReplaceInstruction(StreamInstructionType what, uns indx, uns data_ = unsMax)
        :IsInstruction{ what }, index{ indx }, data{ data_ } {}
};


//***********************************************************************
struct IsSetContainerSizeInstruction : public IsInstruction {
//***********************************************************************
    unsigned newSize;
    IsSetContainerSizeInstruction(StreamInstructionType what, unsigned size) :IsInstruction{ what }, newSize{ size }{}
};


//***********************************************************************
struct IsComponentInstanceInstruction : public IsInstruction {
//***********************************************************************
    unsigned index, classIndex;
    IsComponentInstanceInstruction(StreamInstructionType what, unsigned indx, unsigned classIndx)
        :IsInstruction{ what }, index{ indx }, classIndex{ classIndx }{}
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
struct IsExpressionAtomInstruction: public IsInstruction {
//***********************************************************************
    ExpressionAtom atom;
    IsExpressionAtomInstruction(const ExpressionAtom& ant) :IsInstruction{ sitExpressionAtom }, atom{ ant }{}
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
