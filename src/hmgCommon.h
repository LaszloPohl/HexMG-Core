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
inline constexpr uns probeMaxComponentLevel = 3; // if changed, ProbeNodeID::componentID default values must be set
//***********************************************************************


//***********************************************************************
enum NodeVarType {
//***********************************************************************
    nvtNone, nvtIO, nvtIN, nvtCIN, nvtOUT, nvtFWOUT, nvtNInternal, nvtCInternal, 
    nvtVarInternal, nvtVarGlobal, nvtParam, nvtRail, nvtGND, nvtUnconnected,
    nvtTime, nvtDTime, nvtFreq
};


//***********************************************************************
struct SimpleNodeID {
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
enum StreamInstructionType {
//***********************************************************************
    sitNothing = 0, sitCreate, sitSave,

    sitDefModelSubcircuit, sitDefModelController,

    sitComponentInstance, sitSunredTree, sitSunredLevel, sitSunredReduction,

    sitRails, sitRailValue, sitRailRange, sitNodeValue, sitParameterValue,

    sitProbe, sitProbeNode, sitRun, sitFunction, sitExpressionAtom, sitUns,

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
    SimpleNodeID param; // if a parameter is forwarded, this is the index of the parameter in the input param list, otherwise 0
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
inline rvt& getRe(cplx& c) { return reinterpret_cast<rvt(&)[2]>(c)[0]; }
inline rvt& getIm(cplx& c) { return reinterpret_cast<rvt(&)[2]>(c)[1]; }
//***********************************************************************
   

//***********************************************************************
struct ProbeNodeID {
//***********************************************************************
    SimpleNodeID nodeID;
    uns componentID[probeMaxComponentLevel] = { unsMax, unsMax, unsMax };
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
enum CDNodeType { cdntInternal, cdntExternal, cdntRail, cdntGnd, cdntUnconnected }; // unconnected: only for ONodes
struct CDNode { CDNodeType type = cdntExternal; uns index = 0; };
enum CDParamType { cdptValue, cdptGlobalVariable, cdptLocalVariable, cdptParam, cdptInternalNode, cdptExternalNode };
struct CDParam { CDParamType type = CDParamType::cdptValue; uns index = 0; rvt value = rvt0; };
//***********************************************************************


//***********************************************************************
struct ProbeCDNodeID {
//***********************************************************************
    CDNode nodeID;
    uns componentID[probeMaxComponentLevel] = { unsMax, unsMax, unsMax };
};


#define HMG_DEBUGPRINT
//***********************************************************************


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
