//***********************************************************************
// HexMG common types and declarations
// Creation date:  2023. 01. 24.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_COMMON_HEADER
#define	HMG_COMMON_HEADER
//***********************************************************************


//***********************************************************************
#include <iostream>
#include <cmath>
#include <complex>
#include <atomic>
#include <vector>
#include <memory>
#include <algorithm>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
using rvt = double;
using uns = unsigned;
using siz = size_t;
using cuns = const uns;
using cint = const int;
using crvt = const rvt;
using csiz = const siz;
using cplx = std::complex<rvt>;
using ccplx = const cplx;
using ush = unsigned short;
//***********************************************************************


//***********************************************************************
inline constexpr rvt hmgPi  = rvt(3.1415926535897932384626433832795);
inline constexpr rvt hmgE   = rvt(2.7182818284590452353602874713526);
inline constexpr rvt hmgK   = rvt(1.38064852e-23);
inline constexpr rvt hmgQ   = rvt(1.602176634e-19);
inline constexpr rvt hmgT0  = rvt(273.15);
inline constexpr rvt rvt0   = rvt(0);
inline constexpr rvt rvt1   = rvt(1);
inline constexpr cplx cplx0 = cplx(0);
inline constexpr siz siz0   = siz(0);
inline constexpr int returnInstructionID = 1'000'000'000;
inline constexpr uns unsMax = ~uns(0);
inline constexpr uns externalNodeFlag = uns(1 << 30);
inline constexpr uns maxRails = 1'048'576;
//***********************************************************************


//***********************************************************************
enum NodeVarType {
//***********************************************************************
    nvtIO, nvtIN, nvtCIN, nvtOUT, nvtFWOUT, nvtNInternal, nvtCInternal, 
    nvtVarInternal, nvtVarGlobal, nvtParam, nvtRail, nvtGND,
    nvtTime, nvtDTime, nvtFreq
};


//***********************************************************************
struct SimpleNodeID {
//***********************************************************************
    NodeVarType type = nvtIO;
    uns index = 0;
};


//***********************************************************************
enum builtInModelType { bimtCustom, bimtConstR_1, bimtConstC_1,
    bimtConstI_1, bimtConst_V_Controlled_I, bimtGirator, bimtConstV, 
    bimtSize }; // bimtSize have to be the last one
//***********************************************************************


//***********************************************************************
enum builtInFunctionType {
    futInvalid, futCustom,
    futOpPlus, futOpMinus, futOpMul, futOpDiv, futOpNegativeSign,
    futOpGreater, futOpSmaller, futOpGrEq, futOpSmEq, futOpEqual,
    futOpNonEqual, futOpAnd, futOpOr, futOpNot,
    futInv, futSqrt, futSquare, futPow, futExp, futLn, futLog, futAbs,
    futAsin, futAcos, futAtan, futAsinh, futAcosh, futAtanh,
    futSin, futCos, futTan, futSinh, futCosh, futTanh,
    futRatio, futPwl, futQcnl, futUnit, futUramp,
    futSize // futSize have to be the last one
};
//***********************************************************************


//***********************************************************************
inline rvt cutToPrint(rvt value) noexcept { return abs(value) < rvt(1e-15) ? rvt0 : value; }
inline cplx cutToPrint(cplx value) noexcept { return abs(value) < rvt(1e-15) ? cplx0 : value; }
//***********************************************************************
template<typename T>
inline T square(T value) { return value * value; }
inline rvt absSquare(cplx value) { return value.real() * value.real() + value.imag() * value.imag(); }
//***********************************************************************



//***********************************************************************
enum ExpressionAndComponentType {
//***********************************************************************
    etInvalid, etConst, etFunction, etListElem, etNodeOrVar, etParameter
};


//***********************************************************************
enum HMGFileInstructionType {
//***********************************************************************
    itNone, itComponentInstance, itModel, itSunredTree, itRail,
    itExpression, itGlobal, itBuiltInComponentType
};


//***********************************************************************
struct NodeID {
//***********************************************************************
    enum NodeType {
        ntGround        = 0x00000001,   // ground=1, everithing else=0
        ntGlobal        = 0x00000002,   // global=1, local=0; in case of static var and expression this bit is ignored, always global

        ntExternal      = 0x00000004,   // external=1, internal=0

        ntVariable      = 0x00000008,   // variable=1, node=0
        ntConst         = 0x00000010,   // const=1, non-const=0
        ntSVariable     = 0x00000020,   // svariable=1, mvariable=0
        ntStaticVar     = 0x00000040,   // static variable=1, instance variable=0 (if node: ignored)

        ntTamb          = 0x00000080,   // the _Tamb built in variable
        ntTime          = 0x00000100,   // the _time built in variable
        ntDTime         = 0x00000200,   // the _dtime built in variable
        ntFreq          = 0x00000400,   // the _freq built in variable

        ntNotActual     = 0x00000800,   // Prev or StepStart = 1, actual = 0
        ntStepStart     = 0x00001000,   // StepStart=1, Prev=0

        ntExpression    = 0x00002000,   // Expression=1, node or variable = 0

        ntI             = 0x00004000,   // I of a component instance
        ntV             = 0x00008000,   // V of a component instance
        ntValue         = 0x00010000,   // value of a component instance

        ntThermal       = 0x00020000,   // Thermal=1, other=0

        ntForwarded     = 0x00040000,   // forwarded=1, other=0

        ntIsComponentId = 0x00080000    // only for serialization
    };
    unsigned nodeType;
    unsigned componentId;    
    unsigned nodeId;
    NodeID() :nodeType{ 0 }, componentId{ 0 }, nodeId{ 0 }{}
};


//***********************************************************************
struct ExpressionAtom {
//***********************************************************************
    ExpressionAndComponentType atomType;
    builtInFunctionType functionType;
    double constValue;
    bool isConst;
    bool isBool; // true: bool, false: double
    // one or two parameter functions and the operators do not use parameter list
    unsigned short par1OrSourceIndex, par2OrPrevIndex; // 0: no source/prev; prev is used only for list members, it points to the prev list member 
    NodeID nodeID;
    ExpressionAtom(ExpressionAndComponentType type = etInvalid) : atomType{ type }, functionType{ futInvalid }, constValue{ 0 }, isConst{ false }, isBool{ false },
        /*, subNode{ sntAct }*/par1OrSourceIndex{ 0 }, par2OrPrevIndex{ 0 }{}
};


//***********************************************************************
enum StreamInstructionType {
//***********************************************************************
    sitNothing = 0, sitEndSimulation,

    sitDefSubckt, sitDefController, sitDefComponentType,

    sitEndDefSubckt, sitEndDefController, sitEndDefComponentType,

    sitReplaceSubckt, sitReplaceController, sitReplaceComponentType,

    sitSubcktInstance, sitControllerInstance, sitSunredTree, 
    sitBuiltInComponentInstance, 

    sitEndComponentInstance,

    sitSetSubcktContainerSize, sitSetControllerContainerSize, 
    sitSetComponentTypeContainerSize, sitSetStaticVarContainerSize, 
    sitSetModelContainerSize, sitSetExpressionContainerSize, sitSetSunredContainerSize,

    sitSetComponentInstanceSize, sitSetVarContainerSize, 
    sitSetInternalNodeContainerSize, sitSetProbeContainerSize, sitSetForwardedContainerSize,

    sitNodeValueContainerSize, sitParameterValueContainerSize,

    sitNodeValue, sitParameterValue,

    sitExpression, sitExpressionAtom, sitEndExpression,

};


//***********************************************************************
enum ProbeUnitType {
//***********************************************************************
    putNodeVar, putText, putInt, putReal, putZeros
};


//***********************************************************************
struct ProbeUnit {
//***********************************************************************
    ProbeUnitType type; // putNodeVar, putText, putInt, putReal, putZeros
    NodeID nodeVar;
    std::string textValue;
    int intValue;
    float realValue;
    unsigned nZeros;
};


//***********************************************************************
struct ParameterInstance {
//***********************************************************************
    unsigned paramIndex; // if a parameter is forwarded, this is the index of the parameter in the input param list, otherwise 0
    double value; // if a value is given, paramIndex=0
    ParameterInstance() :paramIndex{ 0 }, value{ 0 }{}
};


//***********************************************************************
enum BuiltInComponentTemplateType {
//***********************************************************************
    bicttCustom, bictt_R, bictt_RD, bictt_C, bictt_OPEN, bictt_V, bictt_VR,
    bictt_I, bictt_IR, bictt_HYS, bictt_PCC,
    bictt_RM, bictt_CM // resistor and capacitor with semiconductor model
};


//***********************************************************************
inline void most(const std::string & mi_tortent) {
//***********************************************************************
    ;
}


//***********************************************************************
template<typename T>
inline bool vectorForcedSet(std::vector<T>& v, const T& x, size_t index) {
//***********************************************************************
    if (index < v.size()) {
        v[index] = x;
        return true;
    }
    else if (index == v.size()) {
        v.push_back(x);
        return false;
    }
    else {
        v.resize(index + 1); // zeros the new elements
        v[index] = x;
        return false;
    }
}




//***********************************************************************
#define HMG_DEBUGPRINT
//***********************************************************************


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
