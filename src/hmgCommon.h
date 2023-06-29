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
#include <cstring>
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
    nvtNone, nvtIO, nvtIN, nvtCIN, nvtOUT, nvtFWOUT, nvtNInternal, nvtCInternal, 
    nvtVarInternal, nvtVarGlobal, nvtParam, nvtRail, nvtGND, nvtUnconnected,
    nvtTime, nvtDTime, nvtFreq
};


//***********************************************************************
struct SimpleInterfaceNodeID {
//***********************************************************************
    NodeVarType type = nvtNone;
    uns index = 0;
};


//***********************************************************************
enum SolutionType { stFullMatrix, stSunRed }; // , stMultiGrid
//***********************************************************************


//***********************************************************************
enum builtInModelType { bimtCustom, bimtConstR_1, bimtConstR_2, bimtConstG_1, 
    bimtConstG_2, bimtConstC_1, bimtConstC_2, bimtConstI_1, bimtConstI_2, 
    bimtConst_V_Controlled_I, bimtGirator, bimtConstVI, bimtSize }; // bimtSize have to be the last one
//***********************************************************************


//***********************************************************************
enum builtInFunctionType {
    futInvalid, futCustom,
    futOpPlus, futOpMinus, futOpMul, futOpDiv, futOpNegativeSign,
    futOpGreater, futOpSmaller, futOpGrEq, futOpSmEq, futOpEqual,
    futOpNonEqual, futOpAnd, futOpOr, futOpNot,
    futInv, futSqrt, futPow, futExp, futLn, futLog, futAbs,
    futAsin, futAcos, futAtan, futAsinh, futAcosh, futAtanh,
    futSin, futCos, futTan, futSinh, futCosh, futTanh,
    futRatio, futPwl, futQcnl, futUnit, futUramp,
    futSize // futSize have to be the last one
};
//***********************************************************************


//***********************************************************************
enum fileFunctionType {
//***********************************************************************
    fftInvalid, fftCustom,
    
    fft_CONST, fft_C_PI, fft_C_2PI, fft_C_PI2, fft_C_E, fft_C_T0, fft_C_K,
    fft_C_Q,
    
    fft_ADD, fft_SUB, fft_MUL, fft_DIV, fft_ADDC, fft_SUBC, fft_MULC, fft_DIVC,
    fft_CADD, fft_CSUB, fft_CMUL, fft_CDIV,
    fft_NEG, fft_INV, fft_SQRT, fft_POW, fft_POWC, fft_CPOW,
    fft_EXP, fft_NEXP, fft_IEXP, fft_INEXP, fft_LN, fft_LOG, fft_CLOG,
    fft_ABS, fft_ASIN,fft_ACOS, fft_ATAN, fft_ASINH, fft_ACOSH, fft_ATANH,
    fft_SIN, fft_COS, fft_TAN, fft_SINH, fft_COSH, fft_TANH,
    fft_RATIO, fft_PWL, fft_DERIV, fft_DERIVC, fft_VLENGTH2, fft_VLENGTH3,
    fft_DISTANCE2, fft_DISTANCE3,
    
    fft_GT, fft_ST, fft_GE, fft_SE, fft_EQ, fft_NEQ, fft_GT0, fft_ST0,
    fft_GE0, fft_SE0, fft_EQ0, fft_NEQ0, fft_AND, fft_OR, fft_NOT,
    
    fft_JMP, fft_JGT, fft_JST, fft_JGE, fft_JSE, fft_JEQ, fft_JNEQ,
    fft_JGT0, fft_JST0, fft_JGE0, fft_JSE0, fft_JEQ0, fft_JNEQ0,
    
    fft_CPY, fft_CGT, fft_CST, fft_CGE, fft_CSE, fft_CEQ, fft_CNEQ,
    fft_CGT0, fft_CST0, fft_CGE0, fft_CSE0, fft_CEQ0, fft_CNEQ0,
    
    fft_TGT, fft_TST, fft_TGE, fft_TSE, fft_TEQ, fft_TNEQ,
    fft_TGT0, fft_TST0, fft_TGE0, fft_TSE0, fft_TEQ0, fft_TNEQ0,
    
    fft_UNIT, fft_UNITT, fft_URAMP, fft_TIME, fft_DT, fft_FREQ, fft_GND,
    fft_RAIL,

    fftSize
};


//***********************************************************************
inline rvt cutToPrint(rvt value) noexcept { return abs(value) < rvt(1e-15) ? rvt0 : value; }
inline cplx cutToPrint(cplx value) noexcept { return abs(value) < rvt(1e-15) ? cplx0 : value; }
//***********************************************************************
template<typename T>
inline T square(T value) { return value * value; }
inline rvt absSquare(cplx value) { return value.real() * value.real() + value.imag() * value.imag(); }
//***********************************************************************



//***********************************************************************
enum StreamInstructionType {
//***********************************************************************
    sitNothing = 0, sitCreate, sitSave,

    sitDefModelSubcircuit, sitDefModelController,

    sitComponentInstance, sitSunredTree, sitSunredLevel, sitSunredReduction,

    sitMultigrid, sitMgLocals, sitMgLocalSimple, sitMgOneLocalSimple,
    sitMgRecursiveInstr, sitMgOneRecursiveInstr, sitMgFineCoarse,
    sitMgNodeInstruction, sitMgOne, sitMgComponentGroup,

    sitRails, sitRailValue, sitRailRange, sitNodeValue, sitParameterValue,

    sitProbe, sitProbeNode, sitRun, sitFunction, sitExpressionAtom, sitUns,
    sitSet,

    sitEndInstruction
};


//***********************************************************************
enum ControlInstructionType {
//***********************************************************************
    citNothing, citAnalysis, citRepaceComponetType, citSave
};


//***********************************************************************
enum ProbeType {
//***********************************************************************
    ptV, ptI, ptVSum, ptVAverage, ptISum, ptIAverage
};


//***********************************************************************
struct ExternalConnectionSizePack {
//***********************************************************************
    uns nIONodes = 0;
    uns nNormalINodes = 0;
    uns nControlINodes = 0;
    uns nNormalONodes = 0;
    uns nForwardedONodes = 0;
    uns nParams = 0;
    void zero() { nIONodes = nNormalINodes = nControlINodes = nNormalONodes = nForwardedONodes = nParams = 0; }
};


//***********************************************************************
struct InternalNodeVarSizePack {
//***********************************************************************
    uns nNormalInternalNodes = 0;
    uns nControlInternalNodes = 0;
    uns nInternalVars = 0;
    void zero() { nNormalInternalNodes = nControlInternalNodes = nInternalVars = 0; }
};


//***********************************************************************
struct ReductionInstruction {
//***********************************************************************
    uns cell1Level = 0;
    uns cell1Index = 0;
    uns cell2Level = 0;
    uns cell2Index = 0;
};


//***********************************************************************
struct ParameterInstance {
//***********************************************************************
    SimpleInterfaceNodeID param; // if a parameter is forwarded, this is the index of the parameter in the input param list, otherwise 0
    rvt value = rvt0; // if a value is given, paramIndex=0
};


//***********************************************************************
enum BuiltInComponentTemplateType {
//***********************************************************************
    bicttCustom, bictt_R, bictt_RD, bictt_C, bictt_OPEN, bictt_V, bictt_VR,
    bictt_I, bictt_IR, bictt_HYS, bictt_PCC,
    bictt_RM, bictt_CM // resistor and capacitor with semiconductor model
};


//***********************************************************************
enum AnalysisType {
//***********************************************************************
    atDC, atTimeStep, atAC, atTimeConst
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
template<typename T>
inline T& vectorForcedGet(std::vector<T>& v, size_t index) {
//***********************************************************************
    if (index > v.size()) {
        v.resize(index + 1); // zeros the new elements
    }
    else if (index == v.size()) {
        v.emplace_back(T());
    }
    return v[index];
}


//***********************************************************************
inline int strcmpC(const char* str1, const char* str2, uns code, uns& retCode) {
//***********************************************************************
    int res = strcmp(str1, str2);
    if (res == 0)
        retCode = code;
    return res;
}


//***********************************************************************
inline rvt& getRe(cplx& c) { return reinterpret_cast<rvt(&)[2]>(c)[0]; }
inline rvt& getIm(cplx& c) { return reinterpret_cast<rvt(&)[2]>(c)[1]; }
//***********************************************************************
   

//***********************************************************************
struct DeepInterfaceNodeID {
//***********************************************************************
    SimpleInterfaceNodeID nodeID;
    std::vector<uns> componentID;
};


//***********************************************************************
struct DefaultRailRange {
//***********************************************************************
    uns rail = 0;
    NodeVarType type = nvtNone;
    uns start_index = 0;
    uns stop_index = 0;
};


//***********************************************************************
struct ForcedNodeDef {
//***********************************************************************
    uns defaultRailIndex = 0;
    uns nodeStartIndex = 0;
    uns nodeStopIndex = 0;
    bool isExternal = false;
};


//***********************************************************************
struct RunData {
//***********************************************************************
    uns fullCircuitID = 0;
    AnalysisType analysisType = atDC;
    bool isInitial = false;
    bool isPre = false; // successive approximation
    bool isDT = false;  // TIMESTEP: T or DT
    bool isTau = false; // fTau is f or tau (f=1/(2*PI*TAU))
    bool isMultigrid = false;
    uns iterNumSPD = 0; // DC/TIMESTEP: number of iteration steps, if 0 => until convergence; TIMECONST: STEP PER DECADE
    rvt err = 0.0001;
    rvt fTauDtT = 1;
};


//***********************************************************************
struct SimulationToSaveData {
//***********************************************************************
    bool isRaw = false;
    bool isAppend = false;
    AnalysisType analysisType = atDC;
    uns maxResultsPerRow = 100;
    rvt timeFreqValue = rvt0;
    rvt dtValue = rvt0;
    std::string fileName;
    std::vector<rvt> saveValuesDC;
    std::vector<cplx> saveValuesAC;
    SimulationToSaveData* next = nullptr;
};



//***********************************************************************
enum CDNodeType { cdntNone, cdntInternal, cdntExternal, cdntRail, cdntGnd, cdntUnconnected }; // unconnected: only for ONodes
struct CDNode { CDNodeType type = cdntNone; uns index = 0; }; // ! default type must be cdntNone!
enum CDParamType { cdptValue, cdptGlobalVariable, cdptLocalVariable, cdptParam, cdptInternalNode, cdptExternalNode };
struct CDParam { CDParamType type = CDParamType::cdptValue; uns index = 0; rvt value = rvt0; };
//***********************************************************************


//***********************************************************************
struct DeepCDNodeID {
//***********************************************************************
    CDNode nodeID;
    std::vector<uns> componentID;
};


#define HMG_DEBUGPRINT
//***********************************************************************


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
