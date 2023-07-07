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
    bimtConst_V_Controlled_I, bimtGirator, bimtConstVI, 
    bimFunc_Controlled_IG, bimtSize }; // bimtSize have to be the last one
//***********************************************************************


//***********************************************************************
enum componentModelType { cmtBuiltIn, cmtCustom, cmtFunctionControlledBuiltIn };
//***********************************************************************


//***********************************************************************
enum builtInFunctionType {
//***********************************************************************
    biftInvalid, biftCustom,
    
    bift_CONST, bift_C_PI, bift_C_2PI, bift_C_PI2, bift_C_E, bift_C_T0, 
    bift_C_K, bift_C_Q,
    
    bift_ADD, bift_SUB, bift_MUL, bift_DIV, bift_ADDC, bift_SUBC,
    bift_MULC, bift_DIVC,
    bift_CADD, bift_CSUB, bift_CMUL, bift_CDIV,
    bift_NEG, bift_INV, bift_SQRT, bift_POW, bift_POWC, bift_CPOW,
    bift_EXP, bift_NEXP, bift_IEXP, bift_INEXP, bift_LN, bift_LOG,
    bift_CLOG,
    bift_ABS, bift_ASIN,bift_ACOS, bift_ATAN, bift_ASINH, bift_ACOSH,
    bift_ATANH,
    bift_SIN, bift_COS, bift_TAN, bift_SINH, bift_COSH, bift_TANH,
    bift_RATIO, bift_PWL, bift_DERIV, bift_DERIVC, bift_VLENGTH2,
    bift_VLENGTH3, bift_DISTANCE2, bift_DISTANCE3,
    
    bift_GT, bift_ST, bift_GE, bift_SE, bift_EQ, bift_NEQ, bift_GT0, 
    bift_ST0, bift_GE0, bift_SE0, bift_EQ0, bift_NEQ0, bift_AND,
    bift_OR, bift_NOT,
    
    bift_JMP, bift_JGT, bift_JST, bift_JGE, bift_JSE, bift_JEQ, bift_JNEQ,
    bift_JGT0, bift_JST0, bift_JGE0, bift_JSE0, bift_JEQ0, bift_JNEQ0,
    
    bift_CPY, bift_CGT, bift_CST, bift_CGE, bift_CSE, bift_CEQ, bift_CNEQ,
    bift_CGT0, bift_CST0, bift_CGE0, bift_CSE0, bift_CEQ0, bift_CNEQ0,
    
    bift_TGT, bift_TST, bift_TGE, bift_TSE, bift_TEQ, bift_TNEQ,
    bift_TGT0, bift_TST0, bift_TGE0, bift_TSE0, bift_TEQ0, bift_TNEQ0,
    
    bift_CGTC, bift_CSTC, bift_CGEC, bift_CSEC, bift_CEQC, bift_CNEQC,
    bift_CGT0C, bift_CST0C, bift_CGE0C, bift_CSE0C, bift_CEQ0C, bift_CNEQ0C,

    bift_JMPR, bift_JGTR, bift_JSTR, bift_JGER, bift_JSER, bift_JEQR, bift_JNEQR,
    bift_JGT0R, bift_JST0R, bift_JGE0R, bift_JSE0R, bift_JEQ0R, bift_JNEQ0R,

    bift_CGTR, bift_CSTR, bift_CGER, bift_CSER, bift_CEQR, bift_CNEQR,
    bift_CGT0R, bift_CST0R, bift_CGE0R, bift_CSE0R, bift_CEQ0R, bift_CNEQ0R,

    bift_TGTR, bift_TSTR, bift_TGER, bift_TSER, bift_TEQR, bift_TNEQR,
    bift_TGT0R, bift_TST0R, bift_TGE0R, bift_TSE0R, bift_TEQ0R, bift_TNEQ0R,

    bift_CGTCR, bift_CSTCR, bift_CGECR, bift_CSECR, bift_CEQCR, bift_CNEQCR,
    bift_CGT0CR, bift_CST0CR, bift_CGE0CR, bift_CSE0CR, bift_CEQ0CR, bift_CNEQ0CR,

    bift_UNIT, bift_UNITT, bift_URAMP, bift_TIME, bift_DT, bift_FREQ,
    bift_RAIL,

    bift_SETVG, bift_GETVG,

    biftSize
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

    sitComponentInstance, sitFunctionControlledComponentInstance, 
    sitSunredTree, sitSunredLevel, sitSunredReduction,

    sitMultigrid, sitMgLocals, sitMgLocalSimple, sitMgOneLocalSimple,
    sitMgRecursiveInstr, sitMgOneRecursiveInstr, sitMgFineCoarse,
    sitMgNodeInstruction, sitMgOne, sitMgComponentGroup,

    sitRails, sitRailValue, sitRailRange, sitNodeValue, sitParameterValue,

    sitProbe, sitProbeNode, sitRun, sitUns, sitRvt,
    sitSet,

    sitFunction, sitFunctionCall, sitFunctionParID,

    sitEndInstruction
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
