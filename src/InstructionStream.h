//***********************************************************************
// Hex Open Instruction Stream Header
// Creation date:  2021. 07. 26.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HO_INSTRUCTION_STREAM_HEADER
#define HO_INSTRUCTION_STREAM_HEADER
//***********************************************************************


//***********************************************************************
#define _CRT_SECURE_NO_WARNINGS
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
struct IsDefSubcktInstruction: public IsInstruction {
//***********************************************************************
    unsigned index, nNormalNode, nControlNode, nParams;
    IsDefSubcktInstruction() :IsInstruction{ sitDefSubckt }, index{ 0 }, nNormalNode{ 0 }, nControlNode{ 0 }, nParams{ 0 }{}
    IsDefSubcktInstruction(unsigned indx, unsigned nNormal, unsigned nControl, unsigned nPar)
        :IsInstruction{ sitDefSubckt }, index{ indx }, nNormalNode{ nNormal }, nControlNode{ nControl }, nParams{ nPar }{}
};


//***********************************************************************
struct IsDefControllerInstruction: public IsInstruction {
//***********************************************************************
    unsigned index, nControlNode, nParams;
    IsDefControllerInstruction() :IsInstruction{ sitDefController }, index{ 0 }, nControlNode{ 0 }, nParams{ 0 }{}
    IsDefControllerInstruction(unsigned indx, unsigned nControl, unsigned nPar)
        :IsInstruction{ sitDefController }, index{ indx }, nControlNode{ nControl }, nParams{ nPar }{}
};


//***********************************************************************
struct IsEndDefInstruction: public IsInstruction {
//***********************************************************************
    unsigned index;
    IsEndDefInstruction(StreamInstructionType what) :IsInstruction{ what }, index{ 0 } {}
    IsEndDefInstruction(StreamInstructionType what, unsigned indx) :IsInstruction{ what }, index{ indx } {}
};


//***********************************************************************
struct IsReplaceInstruction: public IsInstruction {
//***********************************************************************
    unsigned index;
    IsReplaceInstruction(StreamInstructionType what) :IsInstruction{ what }, index{ 0 } {}
    IsReplaceInstruction(StreamInstructionType what, unsigned indx)
        :IsInstruction{ what }, index{ indx }{}
};


//***********************************************************************
struct IsSetContainerSizeInstruction : public IsInstruction {
//***********************************************************************
    unsigned newSize;
    IsSetContainerSizeInstruction(StreamInstructionType what) :IsInstruction{ what }, newSize{ 0 }{}
    IsSetContainerSizeInstruction(StreamInstructionType what, unsigned size) :IsInstruction{ what }, newSize{ size }{}
};


//***********************************************************************
struct IsComponentInstanceInstruction : public IsInstruction {
//***********************************************************************
    unsigned index, classIndex;
    IsComponentInstanceInstruction(StreamInstructionType what) :IsInstruction{ what }, index{ 0 }, classIndex{ 0 }{}
    IsComponentInstanceInstruction(StreamInstructionType what, unsigned indx, unsigned classIndx)
        :IsInstruction{ what }, index{ indx }, classIndex{ classIndx }{}
};


//***********************************************************************
struct IsNodeValueInstruction: public IsInstruction {
//***********************************************************************
    SimpleNodeID nodeID;
    IsNodeValueInstruction() :IsInstruction{ sitNodeValue }{}
    IsNodeValueInstruction(const SimpleNodeID& node) :IsInstruction{ sitNodeValue }, nodeID{ node }{}
};


//***********************************************************************
struct IsParameterValueInstruction: public IsInstruction {
//***********************************************************************
    ParameterInstance param;
    IsParameterValueInstruction() :IsInstruction{ sitParameterValue }{}
    IsParameterValueInstruction(const ParameterInstance& par) :IsInstruction{ sitParameterValue }, param{ par }{}
};


//***********************************************************************
struct IsExpressionInstruction: public IsInstruction {
//***********************************************************************
    unsigned index, size;
    IsExpressionInstruction() :IsInstruction{ sitExpression }, index{ 0 }, size{ 0 }{}
    IsExpressionInstruction(unsigned indx, unsigned siz) :IsInstruction{ sitExpression }, index{ indx }, size{ siz }{}
};


//***********************************************************************
struct IsExpressionAtomInstruction: public IsInstruction {
//***********************************************************************
    ExpressionAtom atom;
    IsExpressionAtomInstruction() :IsInstruction{ sitExpressionAtom }{}
    IsExpressionAtomInstruction(const ExpressionAtom& ant) :IsInstruction{ sitExpressionAtom }, atom{ ant }{}
};


//***********************************************************************
struct IsDefComponentTypeInstruction : public IsInstruction {
//***********************************************************************
    unsigned index, nNormalNode, nControlNode, nParams, nInternalNodes;
    IsDefComponentTypeInstruction() :IsInstruction{ sitDefComponentType }, index{ 0 }, nNormalNode{ 0 }, nControlNode{ 0 }, nParams{ 0 }, nInternalNodes{ 0 }{}
    IsDefComponentTypeInstruction(unsigned indx, unsigned nNormal, unsigned nControl, unsigned nPar, unsigned nInternalNode)
        :IsInstruction{ sitDefComponentType }, index{ indx }, nNormalNode{ nNormal }, nControlNode{ nControl }, nParams{ nPar }, nInternalNodes{ nInternalNode }{}
};


//***********************************************************************
inline IsInstruction* instantiateIsInstruction(StreamInstructionType instruction) {
//***********************************************************************
    switch (instruction) {
        case sitNothing:                        return new IsNothingInstruction;
        case sitEndSimulation:                  return new IsEndDefInstruction(sitEndSimulation);
        case sitDefSubckt:                      return new IsDefSubcktInstruction;
        case sitDefController:                  return new IsDefControllerInstruction;
        case sitDefComponentType:               return new IsDefComponentTypeInstruction;
        case sitEndDefSubckt:                   return new IsEndDefInstruction(sitEndDefSubckt);
        case sitEndDefController:               return new IsEndDefInstruction(sitEndDefController);
        case sitEndDefComponentType:            return new IsEndDefInstruction(sitEndDefComponentType);
        case sitReplaceSubckt:                  return new IsReplaceInstruction(sitReplaceSubckt);
        case sitReplaceController:              return new IsReplaceInstruction(sitReplaceController);
        case sitReplaceComponentType:           return new IsReplaceInstruction(sitReplaceComponentType);
        case sitSubcktInstance:                 return new IsComponentInstanceInstruction(sitSubcktInstance);
        case sitControllerInstance:             return new IsComponentInstanceInstruction(sitControllerInstance);
        case sitSunredTree:                     return new IsComponentInstanceInstruction(sitSunredTree);
        case sitBuiltInComponentInstance:       return new IsComponentInstanceInstruction(sitBuiltInComponentInstance);
        case sitEndComponentInstance:           return new IsEndDefInstruction(sitEndComponentInstance);
        case sitSetSubcktContainerSize:         return new IsSetContainerSizeInstruction(sitSetSubcktContainerSize);
        case sitSetControllerContainerSize:     return new IsSetContainerSizeInstruction(sitSetControllerContainerSize);
        case sitSetComponentTypeContainerSize:  return new IsSetContainerSizeInstruction(sitSetComponentTypeContainerSize);
        case sitSetStaticVarContainerSize:      return new IsSetContainerSizeInstruction(sitSetStaticVarContainerSize);
        case sitSetModelContainerSize:          return new IsSetContainerSizeInstruction(sitSetModelContainerSize);
        case sitSetExpressionContainerSize:     return new IsSetContainerSizeInstruction(sitSetExpressionContainerSize);
        case sitSetSunredContainerSize:         return new IsSetContainerSizeInstruction(sitSetSunredContainerSize);
        case sitSetComponentInstanceSize:       return new IsSetContainerSizeInstruction(sitSetComponentInstanceSize);
        case sitSetVarContainerSize:            return new IsSetContainerSizeInstruction(sitSetVarContainerSize);
        case sitSetInternalNodeContainerSize:   return new IsSetContainerSizeInstruction(sitSetInternalNodeContainerSize);
        case sitSetProbeContainerSize:          return new IsSetContainerSizeInstruction(sitSetProbeContainerSize);
        case sitSetForwardedContainerSize:      return new IsSetContainerSizeInstruction(sitSetForwardedContainerSize);
        case sitNodeValueContainerSize:         return new IsSetContainerSizeInstruction(sitNodeValueContainerSize);
        case sitParameterValueContainerSize:    return new IsSetContainerSizeInstruction(sitParameterValueContainerSize);
        case sitNodeValue:                      return new IsNodeValueInstruction;
        case sitParameterValue:                 return new IsParameterValueInstruction;
        case sitExpression:                     return new IsExpressionInstruction;
        case sitExpressionAtom:                 return new IsExpressionAtomInstruction;
        case sitEndExpression:                  return new IsEndDefInstruction(sitEndExpression);
    default:
        throw hmgExcept("instantiateIsInstruction", "unknown instruction type (%u)", instruction);
    }
}


//***********************************************************************
class InstructionStream {
//***********************************************************************
    IsNothingInstruction listDummy;
    IsInstruction * pListLast;
    static const unsigned binBlockSize = 2*65536; // 1048576; // 
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
