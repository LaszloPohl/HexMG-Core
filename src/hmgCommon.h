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
//***********************************************************************


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
#define HMG_DEBUGPRINT
//***********************************************************************


//***********************************************************************
}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
