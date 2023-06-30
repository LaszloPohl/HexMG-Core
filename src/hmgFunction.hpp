//***********************************************************************
// HexMG function (expression) classes
// Creation date:  2023. 03. 02.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_FUNCTION_HEADER
#define	HMG_FUNCTION_HEADER
//***********************************************************************


//***********************************************************************
#include <cmath>
#include "hmgCommon.h"
#include "hmgException.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class ComponentAndControllerBase;
class HmgFunction;
//***********************************************************************


//***********************************************************************
enum ParameterType { ptParam, ptLocalVar, ptPrev }; // prev: index field of the previous function (or the owner function, if no previous)
//***********************************************************************


//***********************************************************************
struct ParameterIdentifier{
//***********************************************************************
    ParameterType parType = ptParam;
    uns parIndex = 0;
};


//***********************************************************************
struct LineDescription {
//***********************************************************************
    const HmgFunction* pFunction = nullptr;
    rvt value = rvt0;
    std::vector<ParameterIdentifier> parameters;    // parameters[0] is the return
    std::vector<rvt> moreValues;                    // _PWL uses
    int jumpValue = 0;
};


//***********************************************************************
class HmgFunction {
//***********************************************************************
protected:
    uns nParam = 0;
    uns nIndexField = 0;
    uns nWorkingField = 0;
public:
    //***********************************************************************
    HmgFunction(uns nParam_, uns nIndexField_, uns nWorkingField_) : nParam{ nParam_ }, nIndexField{ nIndexField_ }, nWorkingField{ nWorkingField_ } {}
    virtual ~HmgFunction() = default;
    //***********************************************************************
    virtual int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept = 0;
    // owner is Component_Function_Controlled_I_with_const_G or Controller
    // return: true if the caller function must be exit (ifr instruction), false: every other cases
    //***********************************************************************
    rvt devive(cuns* index, crvt* cworkField, ComponentAndControllerBase* owner, cuns variableIndex, const LineDescription& line)const noexcept {
    //***********************************************************************
        rvt* workField = const_cast<rvt*>(cworkField); // changes and restores so no change
        crvt value = workField[index[0]];
        crvt var = workField[index[variableIndex]];
        crvt dx = (abs(value) + abs(var)) * rvt(1.0e-9);
        workField[index[variableIndex]] += dx;
        evaluate(index, workField, owner, line);
        crvt ret = (workField[index[0]] - value) / dx;
        // restoring the workfield
        workField[index[0]] = value;
        workField[index[variableIndex]] = var;
        return ret;
    }
    uns getN_Param()const noexcept { return nParam; }
    uns getN_IndexField()const noexcept { return nIndexField; }
    uns getN_WorkingField()const noexcept { return nWorkingField; }
    virtual void fillIndexField(uns* indexField)const noexcept {};
};


//***********************************************************************
class HgmFunctionStorage {
//***********************************************************************
    HgmFunctionStorage();
public:
    HgmFunctionStorage(const HgmFunctionStorage&) = delete;
    HgmFunctionStorage& operator=(const HgmFunctionStorage&) = delete;
    inline static std::vector<std::unique_ptr<HmgFunction>> builtInFunctions;
    inline static std::vector<std::unique_ptr<HmgFunction>> globalCustomFunctions;
    inline static std::vector<std::unique_ptr<HmgFunction>> namelessCustomFunctions;
    //***********************************************************************
    static HgmFunctionStorage& getInstance() { // singleton
    //***********************************************************************
        static HgmFunctionStorage instance;
        return instance;
    }

};


//***********************************************************************
class HmgF_Const final : public HmgFunction{
//***********************************************************************
    crvt value;
public:
    HmgF_Const(rvt value_) : HmgFunction{ 0, 2, 0 }, value{ value_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override { workField[index[0]] = value; return 0; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_If1 final : public HmgFunction{
// if ret is true, caller executes the next function, else it is skipped.
//***********************************************************************
public:
    HmgF_If1() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override { return workField[index[0]] != rvt0 ? 0 : 1; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Else1 final : public HmgFunction{
// if ret is false, caller executes the next function, else it is skipped.
//***********************************************************************
public:
    HmgF_Else1() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override { return workField[index[0]] == rvt0 ? 0 : 1; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_IfValue1 final : public HmgFunction{
// if par1 is true, ret = par2
//***********************************************************************
public:
    HmgF_IfValue1() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] != rvt0)
            workField[index[0]] = workField[index[3]];
        return 0; 
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_IfValue2 final : public HmgFunction{
// if par1 is true, ret = par2 else ret = par3
//***********************************************************************
public:
    HmgF_IfValue2() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = (workField[index[2]] != rvt0) ? workField[index[3]] : workField[index[4]];
        return 0;
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_IfR final : public HmgFunction{
// if par1 is true, ret = par2 and forces the caller to return
//***********************************************************************
public:
    HmgF_IfR() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] != rvt0) {
            workField[index[0]] = workField[index[3]];
            return returnInstructionID;
        }
        return 0; 
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_ControlledI_Node_StepStart final : public HmgFunction{
//***********************************************************************
    uns nodeIndex;
public:
    HmgF_Load_ControlledI_Node_StepStart(uns nodeIndex_) : HmgFunction{ 0, 2, 0 }, nodeIndex{ nodeIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_Controller_Node_StepStart final : public HmgFunction{
//***********************************************************************
    uns nodeIndex;
public:
    HmgF_Load_Controller_Node_StepStart(uns nodeIndex_) : HmgFunction{ 0, 2, 0 }, nodeIndex{ nodeIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_Controller_mVar_StepStart final : public HmgFunction{
//***********************************************************************
    uns varIndex;
public:
    HmgF_Load_Controller_mVar_StepStart(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_Controller_mVar_Value final : public HmgFunction{
//***********************************************************************
    uns varIndex;
public:
    HmgF_Load_Controller_mVar_Value(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Set_Controller_mVar_Value final : public HmgFunction{
// ret => owner->mVars[nodeIndex], not changing ret
//***********************************************************************
    uns varIndex;
public:
    HmgF_Set_Controller_mVar_Value(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Set_Controller_mVar_ValueFromStepStart final : public HmgFunction{
// ret is not used, can be any element of the workField
//***********************************************************************
    uns varIndex;
public:
    HmgF_Set_Controller_mVar_ValueFromStepStart(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_sqrt final : public HmgFunction{
//***********************************************************************
public:
    HmgF_sqrt() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override { workField[index[0]] = sqrt(workField[index[2]]); return 0; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_opPlus final : public HmgFunction{
//***********************************************************************
public:
    HmgF_opPlus() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] + workField[index[3]]; 
        return 0; 
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_opMul final : public HmgFunction{
//***********************************************************************
public:
    HmgF_opMul() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] * workField[index[3]]; 
        return 0;
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//----------------------------------------------------------------------------


//***********************************************************************
class HmgBuiltInFunction_CONST final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CONST() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = line.value;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_C_PI final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_PI() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = hmgPi;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_C_2PI final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_2PI() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = 2 * hmgPi;
        return 0;
    }
};



//***********************************************************************
class HmgBuiltInFunction_C_PI2 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_PI2() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = 0.5 * hmgPi;
        return 0;
    }
};



//***********************************************************************
class HmgBuiltInFunction_C_E final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_E() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = hmgE;
        return 0;
    }
};



//***********************************************************************
class HmgBuiltInFunction_C_T0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_T0() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = hmgT0;
        return 0;
    }
};



//***********************************************************************
class HmgBuiltInFunction_C_K final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_K() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = hmgK;
        return 0;
    }
};



//***********************************************************************
class HmgBuiltInFunction_C_Q final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_C_Q() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = hmgQ;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ADD final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ADD() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] + workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SUB final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SUB() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] - workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_MUL final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_MUL() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] * workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_DIV final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DIV() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] / workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ADDC final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ADDC() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] + line.value;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SUBC final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SUBC() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] - line.value;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_MULC final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_MULC() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] * line.value;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_DIVC final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DIVC() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] / line.value;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CADD final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CADD() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = line.value + workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CSUB final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CSUB() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = line.value - workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CMUL final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CMUL() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = line.value * workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CDIV final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CDIV() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = line.value / workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_NEG final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_NEG() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = -workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_INV final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_INV() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = rvt1 / workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SQRT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SQRT() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = sqrt(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_POW final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_POW() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = pow(workField[index[2]], workField[index[3]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_POWC final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_POWC() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = pow(workField[index[2]], line.value);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CPOW final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CPOW() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = pow(line.value, workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_EXP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_EXP() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = exp(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_NEXP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_NEXP() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = exp(-workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_IEXP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_IEXP() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = exp(rvt1 / workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_INEXP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_INEXP() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = exp(-rvt1 / workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_NIEXP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_NIEXP() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = exp(-rvt1 / workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_LN final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_LN() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = log(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_LOG final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_LOG() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = log(workField[index[3]]) / log(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CLOG final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CLOG() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = log(workField[index[2]]) / log(line.value);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ABS final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ABS() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = abs(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ASIN final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ASIN() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = asin(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ACOS final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ACOS() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = acos(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ATAN final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ATAN() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = atan(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ASINH final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ASINH() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = asinh(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ACOSH final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ACOSH() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = acosh(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ATANH final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ATANH() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = atanh(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SIN final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SIN() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = sin(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_COS final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_COS() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = cos(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TAN final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TAN() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = tan(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SINH final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SINH() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = sinh(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_COSH final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_COSH() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = cosh(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TANH final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TANH() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = tanh(workField[index[2]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_RATIO final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_RATIO() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = (rvt1 - workField[index[2]]) * workField[index[3]] + workField[index[2]] * workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_PWL final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_PWL() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        
        crvt x = workField[index[2]];
        rvt& y = workField[index[0]];
        size_t last = (line.moreValues.size() / 2 - 1) * 2;

        if (line.moreValues.size() < 2) { y = x;                        return 0; }
        if (line.moreValues.size() < 4) { y = line.moreValues[1];       return 0; }
        if (x <= line.moreValues[0])    { y = line.moreValues[1];       return 0; }
        if (x >= line.moreValues[last]) { y = line.moreValues[last + 1];return 0; }
        
        uns second = 2;
        while (line.moreValues[second] > x)
            second += 2;

        crvt x1 = line.moreValues[second - 2];
        crvt y1 = line.moreValues[second - 1];
        crvt x2 = line.moreValues[second];
        crvt y2 = line.moreValues[second + 1];

        y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);

        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_DERIV final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DERIV() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = (workField[index[2]] - workField[index[3]]) / workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_DERIVC final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DERIVC() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = (workField[index[2]] - workField[index[3]]) / line.value;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_VLENGTH2 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_VLENGTH2() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = sqrt(workField[index[2]] * workField[index[2]] + workField[index[3]] * workField[index[3]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_VLENGTH3 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_VLENGTH3() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = sqrt(workField[index[2]] * workField[index[2]] + workField[index[3]] * workField[index[3]] + workField[index[4]] * workField[index[4]]);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_DISTANCE2 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DISTANCE2() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        crvt dx = workField[index[2]] - workField[index[4]];
        crvt dy = workField[index[3]] - workField[index[5]];
        workField[index[0]] = sqrt(dx * dx + dy * dy);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_DISTANCE3 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DISTANCE3() : HmgFunction{ 6, 8, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        crvt dx = workField[index[2]] - workField[index[5]];
        crvt dy = workField[index[3]] - workField[index[6]];
        crvt dz = workField[index[4]] - workField[index[7]];
        workField[index[0]] = sqrt(dx * dx + dy * dy + dz * dz);
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_GT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_GT() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] > workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ST final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ST() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] < workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_GE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_GE() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] >= workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SE() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] <= workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_EQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_EQ() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] == workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_NEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_NEQ() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] != workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_GT0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_GT0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] > rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_ST0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_ST0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] < rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_GE0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_GE0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] >= rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_SE0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SE0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] <= rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_EQ0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_EQ0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] == rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_NEQ0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_NEQ0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] != rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_AND final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_AND() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] != rvt0 && workField[index[3]] != rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_OR final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_OR() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] != rvt0 || workField[index[3]] != rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_NOT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_NOT() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] == rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JMP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_JMP() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return line.jumpValue;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JGT final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JGT() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] > workField[index[3]] ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JST final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_JST() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] < workField[index[3]] ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JGE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_JGE() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] >= workField[index[3]] ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JSE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_JSE() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] <= workField[index[3]] ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_JEQ() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] == workField[index[3]] ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JNEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_JNEQ() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] != workField[index[3]] ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JGT0 final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JGT0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] > rvt0 ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JST0 final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JST0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] < rvt0 ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JGE0 final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JGE0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] >= rvt0 ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JSE0 final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JSE0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] <= rvt0 ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JEQ0 final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JEQ0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] == rvt0 ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_JNEQ0 final : public HmgFunction {
//***********************************************************************
public:
    HmgBuiltInFunction_JNEQ0() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        return workField[index[2]] != rvt0 ? line.jumpValue : 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CPY final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CPY() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CGT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CGT() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] > workField[index[3]])
            workField[index[0]] = workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CST final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CST() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] < workField[index[3]])
            workField[index[0]] = workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CGE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CGE() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] >= workField[index[3]])
            workField[index[0]] = workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CSE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CSE() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] <= workField[index[3]])
            workField[index[0]] = workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CEQ() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] == workField[index[3]])
            workField[index[0]] = workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CNEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CNEQ() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] != workField[index[3]])
            workField[index[0]] = workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CGT0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CGT0() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] > rvt0)
            workField[index[0]] = workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CST0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CST0() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] < rvt0)
            workField[index[0]] = workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CGE0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CGE0() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] >= rvt0)
            workField[index[0]] = workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CSE0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CSE0() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] <= rvt0)
            workField[index[0]] = workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CEQ0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CEQ0() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] == rvt0)
            workField[index[0]] = workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_CNEQ0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_CNEQ0() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        if (workField[index[2]] != rvt0)
            workField[index[0]] = workField[index[3]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TGT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TGT() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] > workField[index[3]] ? workField[index[4]] : workField[index[5]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TST final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TST() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] < workField[index[3]] ? workField[index[4]] : workField[index[5]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TGE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TGE() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] >= workField[index[3]] ? workField[index[4]] : workField[index[5]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TSE final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TSE() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] <= workField[index[3]] ? workField[index[4]] : workField[index[5]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TEQ() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] == workField[index[3]] ? workField[index[4]] : workField[index[5]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TNEQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TNEQ() : HmgFunction{ 4, 6, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] != workField[index[3]] ? workField[index[4]] : workField[index[5]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TGT0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TGT0() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] > rvt0 ? workField[index[3]] : workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TST0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TST0() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] < rvt0 ? workField[index[3]] : workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TGE0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TGE0() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] >= rvt0 ? workField[index[3]] : workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TSE0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TSE0() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] <= rvt0 ? workField[index[3]] : workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TEQ0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TEQ0() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] == rvt0 ? workField[index[3]] : workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TNEQ0 final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TNEQ0() : HmgFunction{ 3, 5, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] != rvt0 ? workField[index[3]] : workField[index[4]];
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_UNIT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_UNIT() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] > rvt0 ? rvt1 : rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_UNITT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_UNITT() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//***********************************************************************
class HmgBuiltInFunction_URAMP final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_URAMP() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override {
        workField[index[0]] = workField[index[2]] > rvt0 ? workField[index[2]] : rvt0;
        return 0;
    }
};


//***********************************************************************
class HmgBuiltInFunction_TIME final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_TIME() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//***********************************************************************
class HmgBuiltInFunction_DT final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_DT() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//***********************************************************************
class HmgBuiltInFunction_FREQ final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_FREQ() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//***********************************************************************
class HmgBuiltInFunction_RAIL final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_RAIL() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//***********************************************************************
class HmgBuiltInFunction_SETVG final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_SETVG() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//***********************************************************************
class HmgBuiltInFunction_GETVG final : public HmgFunction{
//***********************************************************************
public:
    HmgBuiltInFunction_GETVG() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& line)const noexcept override;
};


//----------------------------------------------------------------------------


//***********************************************************************
class HgmCustomFunctionModel {
//***********************************************************************
public:
    //***********************************************************************
    uns nParams = 0;
    uns nLocal = 0;
    std::vector<LineDescription> lines;
    //***********************************************************************
};


//***********************************************************************
class HmgF_CustomFunction final : public HmgFunction {
//***********************************************************************
    HgmCustomFunctionModel model;
    //***********************************************************************
    void init()noexcept {
    //***********************************************************************
        nParam = model.nParams;
        nIndexField = 2 + nParam;
        nWorkingField = model.nLocal;
        for (const auto& line : model.lines) {
            nIndexField += line.pFunction->getN_IndexField();
            nWorkingField += line.pFunction->getN_WorkingField();
        }
    }
public:
    //***********************************************************************
    explicit HmgF_CustomFunction(HgmCustomFunctionModel model_) noexcept : HmgFunction{ 0, 0, 0 }, model(std::move(model_)) { init(); }
    //***********************************************************************
    void fillIndexField(uns* indexField)const noexcept override {
    //***********************************************************************
        // indexField[0] = ret, caller fills
        // indexField[1] = work field starts, caller fills
        // indexField[2...nParam+2-1] = params, caller fills
        // local variables are not in indexField, only in workField => workField[indexField[1] + localVariableIndex]
        uns prev = 0;
        uns base = model.nParams + 2;
        for (const auto& line : model.lines) {
            
            // ret index

            switch (line.parameters[0].parType) {
                case ParameterType::ptParam:
                    indexField[base] = indexField[line.parameters[0].parIndex]; // = params[parIndex]
                    break;
                case ParameterType::ptLocalVar:
                    indexField[base] = indexField[1] + line.parameters[0].parIndex; // = workField[parIndex] a.k.a localvariable[parIndex]
                    break;
                case ParameterType::ptPrev:
                    indexField[base] = indexField[prev + line.parameters[0].parIndex]; // = previous function index field [parIndex]
                    break;
            }
            
            // workField index

            indexField[base + 1] = indexField[1] + model.nLocal;
            
            // params

            cuns nParams = line.pFunction->getN_Param();
            for (uns i = 0; i < nParams; i++) {
                switch (line.parameters[i + 1].parType) {
                    case ParameterType::ptParam:
                        indexField[base + 2 + i] = indexField[line.parameters[i + 1].parIndex]; // = params[parIndex]
                        break;
                    case ParameterType::ptLocalVar:
                        indexField[base + 2 + i] = indexField[1] + line.parameters[i + 1].parIndex; // = workField[parIndex] a.k.a localvariable[parIndex]
                        break;
                    case ParameterType::ptPrev:
                        indexField[base + 2 + i] = indexField[prev + line.parameters[i + 1].parIndex]; // = previous function index field [parIndex]
                        break;
                }
            }
            
            // function fills

            line.pFunction->fillIndexField(indexField + base);
            prev = base;
            base += line.pFunction->getN_IndexField();
        }
    }
    //***********************************************************************
    int evaluate(cuns* indexField, rvt* workField, ComponentAndControllerBase* owner, const LineDescription& ownerLine)const noexcept override {
    //***********************************************************************
        uns base = model.nParams + 2;
        for (uns i = 0; i < model.lines.size(); i++) {
            const auto& line = model.lines[i];
            int jumpValue = line.pFunction->evaluate(indexField + base, workField, owner, line);
            if (jumpValue != 0) { // jump instruction
                if (jumpValue == returnInstructionID) // return instruction
                    return false;
                if (jumpValue < 0) { // jump back
                    while (jumpValue != 0) {
                        i--;
                        base -= model.lines[i].pFunction->getN_IndexField();
                        jumpValue++;
                    }
                }
                else { // jump forward
                    while (jumpValue != 0) {
                        base += model.lines[i].pFunction->getN_IndexField();
                        i++;
                        jumpValue--;
                    }
                }
                i--; // neutralize i++ in for
            }
            else base += line.pFunction->getN_IndexField();
        }
        return 0;
    }
};


//***********************************************************************
inline HgmFunctionStorage::HgmFunctionStorage() {
//***********************************************************************
    builtInFunctions.resize(builtInFunctionType::biftSize);
    builtInFunctions[builtInFunctionType::bift_CONST] = std::make_unique<HmgBuiltInFunction_CONST>();
    builtInFunctions[builtInFunctionType::bift_C_PI] = std::make_unique<HmgBuiltInFunction_C_PI>();
    builtInFunctions[builtInFunctionType::bift_C_2PI] = std::make_unique<HmgBuiltInFunction_C_2PI>();
    builtInFunctions[builtInFunctionType::bift_C_PI2] = std::make_unique<HmgBuiltInFunction_C_PI2>();
    builtInFunctions[builtInFunctionType::bift_C_E] = std::make_unique<HmgBuiltInFunction_C_E>();
    builtInFunctions[builtInFunctionType::bift_C_T0] = std::make_unique<HmgBuiltInFunction_C_T0>();
    builtInFunctions[builtInFunctionType::bift_C_K] = std::make_unique<HmgBuiltInFunction_C_K>();
    builtInFunctions[builtInFunctionType::bift_C_Q] = std::make_unique<HmgBuiltInFunction_C_Q>();
    builtInFunctions[builtInFunctionType::bift_ADD] = std::make_unique<HmgBuiltInFunction_ADD>();
    builtInFunctions[builtInFunctionType::bift_SUB] = std::make_unique<HmgBuiltInFunction_SUB>();
    builtInFunctions[builtInFunctionType::bift_MUL] = std::make_unique<HmgBuiltInFunction_MUL>();
    builtInFunctions[builtInFunctionType::bift_DIV] = std::make_unique<HmgBuiltInFunction_DIV>();
    builtInFunctions[builtInFunctionType::bift_ADDC] = std::make_unique<HmgBuiltInFunction_ADDC>();
    builtInFunctions[builtInFunctionType::bift_SUBC] = std::make_unique<HmgBuiltInFunction_SUBC>();
    builtInFunctions[builtInFunctionType::bift_MULC] = std::make_unique<HmgBuiltInFunction_MULC>();
    builtInFunctions[builtInFunctionType::bift_DIVC] = std::make_unique<HmgBuiltInFunction_DIVC>();
    builtInFunctions[builtInFunctionType::bift_CADD] = std::make_unique<HmgBuiltInFunction_CADD>();
    builtInFunctions[builtInFunctionType::bift_CSUB] = std::make_unique<HmgBuiltInFunction_CSUB>();
    builtInFunctions[builtInFunctionType::bift_CMUL] = std::make_unique<HmgBuiltInFunction_CMUL>();
    builtInFunctions[builtInFunctionType::bift_CDIV] = std::make_unique<HmgBuiltInFunction_CDIV>();
    builtInFunctions[builtInFunctionType::bift_NEG] = std::make_unique<HmgBuiltInFunction_NEG>();
    builtInFunctions[builtInFunctionType::bift_INV] = std::make_unique<HmgBuiltInFunction_INV>();
    builtInFunctions[builtInFunctionType::bift_SQRT] = std::make_unique<HmgBuiltInFunction_SQRT>();
    builtInFunctions[builtInFunctionType::bift_POW] = std::make_unique<HmgBuiltInFunction_POW>();
    builtInFunctions[builtInFunctionType::bift_POWC] = std::make_unique<HmgBuiltInFunction_POWC>();
    builtInFunctions[builtInFunctionType::bift_CPOW] = std::make_unique<HmgBuiltInFunction_CPOW>();
    builtInFunctions[builtInFunctionType::bift_EXP] = std::make_unique<HmgBuiltInFunction_EXP>();
    builtInFunctions[builtInFunctionType::bift_NEXP] = std::make_unique<HmgBuiltInFunction_NEXP>();
    builtInFunctions[builtInFunctionType::bift_IEXP] = std::make_unique<HmgBuiltInFunction_IEXP>();
    builtInFunctions[builtInFunctionType::bift_INEXP] = std::make_unique<HmgBuiltInFunction_INEXP>();
    builtInFunctions[builtInFunctionType::bift_LN] = std::make_unique<HmgBuiltInFunction_LN>();
    builtInFunctions[builtInFunctionType::bift_LOG] = std::make_unique<HmgBuiltInFunction_LOG>();
    builtInFunctions[builtInFunctionType::bift_CLOG] = std::make_unique<HmgBuiltInFunction_CLOG>();
    builtInFunctions[builtInFunctionType::bift_ABS] = std::make_unique<HmgBuiltInFunction_ABS>();
    builtInFunctions[builtInFunctionType::bift_ASIN] = std::make_unique<HmgBuiltInFunction_ASIN>();
    builtInFunctions[builtInFunctionType::bift_ACOS] = std::make_unique<HmgBuiltInFunction_ACOS>();
    builtInFunctions[builtInFunctionType::bift_ATAN] = std::make_unique<HmgBuiltInFunction_ATAN>();
    builtInFunctions[builtInFunctionType::bift_ASINH] = std::make_unique<HmgBuiltInFunction_ASINH>();
    builtInFunctions[builtInFunctionType::bift_ACOSH] = std::make_unique<HmgBuiltInFunction_ACOSH>();
    builtInFunctions[builtInFunctionType::bift_ATANH] = std::make_unique<HmgBuiltInFunction_ATANH>();
    builtInFunctions[builtInFunctionType::bift_SIN] = std::make_unique<HmgBuiltInFunction_SIN>();
    builtInFunctions[builtInFunctionType::bift_COS] = std::make_unique<HmgBuiltInFunction_COS>();
    builtInFunctions[builtInFunctionType::bift_TAN] = std::make_unique<HmgBuiltInFunction_TAN>();
    builtInFunctions[builtInFunctionType::bift_SINH] = std::make_unique<HmgBuiltInFunction_SINH>();
    builtInFunctions[builtInFunctionType::bift_COSH] = std::make_unique<HmgBuiltInFunction_COSH>();
    builtInFunctions[builtInFunctionType::bift_TANH] = std::make_unique<HmgBuiltInFunction_TANH>();
    builtInFunctions[builtInFunctionType::bift_RATIO] = std::make_unique<HmgBuiltInFunction_RATIO>();
    builtInFunctions[builtInFunctionType::bift_PWL] = std::make_unique<HmgBuiltInFunction_PWL>();
    builtInFunctions[builtInFunctionType::bift_DERIV] = std::make_unique<HmgBuiltInFunction_DERIV>();
    builtInFunctions[builtInFunctionType::bift_DERIVC] = std::make_unique<HmgBuiltInFunction_DERIVC>();
    builtInFunctions[builtInFunctionType::bift_VLENGTH2] = std::make_unique<HmgBuiltInFunction_VLENGTH2>();
    builtInFunctions[builtInFunctionType::bift_VLENGTH3] = std::make_unique<HmgBuiltInFunction_VLENGTH3>();
    builtInFunctions[builtInFunctionType::bift_DISTANCE2] = std::make_unique<HmgBuiltInFunction_DISTANCE2>();
    builtInFunctions[builtInFunctionType::bift_DISTANCE3] = std::make_unique<HmgBuiltInFunction_DISTANCE3>();
    builtInFunctions[builtInFunctionType::bift_GT] = std::make_unique<HmgBuiltInFunction_GT>();
    builtInFunctions[builtInFunctionType::bift_ST] = std::make_unique<HmgBuiltInFunction_ST>();
    builtInFunctions[builtInFunctionType::bift_GE] = std::make_unique<HmgBuiltInFunction_GE>();
    builtInFunctions[builtInFunctionType::bift_SE] = std::make_unique<HmgBuiltInFunction_SE>();
    builtInFunctions[builtInFunctionType::bift_EQ] = std::make_unique<HmgBuiltInFunction_EQ>();
    builtInFunctions[builtInFunctionType::bift_NEQ] = std::make_unique<HmgBuiltInFunction_NEQ>();
    builtInFunctions[builtInFunctionType::bift_GT0] = std::make_unique<HmgBuiltInFunction_GT0>();
    builtInFunctions[builtInFunctionType::bift_ST0] = std::make_unique<HmgBuiltInFunction_ST0>();
    builtInFunctions[builtInFunctionType::bift_GE0] = std::make_unique<HmgBuiltInFunction_GE0>();
    builtInFunctions[builtInFunctionType::bift_SE0] = std::make_unique<HmgBuiltInFunction_SE0>();
    builtInFunctions[builtInFunctionType::bift_EQ0] = std::make_unique<HmgBuiltInFunction_EQ0>();
    builtInFunctions[builtInFunctionType::bift_NEQ0] = std::make_unique<HmgBuiltInFunction_NEQ0>();
    builtInFunctions[builtInFunctionType::bift_AND] = std::make_unique<HmgBuiltInFunction_AND>();
    builtInFunctions[builtInFunctionType::bift_OR] = std::make_unique<HmgBuiltInFunction_OR>();
    builtInFunctions[builtInFunctionType::bift_NOT] = std::make_unique<HmgBuiltInFunction_NOT>();
    builtInFunctions[builtInFunctionType::bift_JMP] = std::make_unique<HmgBuiltInFunction_JMP>();
    builtInFunctions[builtInFunctionType::bift_JGT] = std::make_unique<HmgBuiltInFunction_JGT>();
    builtInFunctions[builtInFunctionType::bift_JST] = std::make_unique<HmgBuiltInFunction_JST>();
    builtInFunctions[builtInFunctionType::bift_JGE] = std::make_unique<HmgBuiltInFunction_JGE>();
    builtInFunctions[builtInFunctionType::bift_JSE] = std::make_unique<HmgBuiltInFunction_JSE>();
    builtInFunctions[builtInFunctionType::bift_JEQ] = std::make_unique<HmgBuiltInFunction_JEQ>();
    builtInFunctions[builtInFunctionType::bift_JNEQ] = std::make_unique<HmgBuiltInFunction_JNEQ>();
    builtInFunctions[builtInFunctionType::bift_JGT0] = std::make_unique<HmgBuiltInFunction_JGT0>();
    builtInFunctions[builtInFunctionType::bift_JST0] = std::make_unique<HmgBuiltInFunction_JST0>();
    builtInFunctions[builtInFunctionType::bift_JGE0] = std::make_unique<HmgBuiltInFunction_JGE0>();
    builtInFunctions[builtInFunctionType::bift_JSE0] = std::make_unique<HmgBuiltInFunction_JSE0>();
    builtInFunctions[builtInFunctionType::bift_JEQ0] = std::make_unique<HmgBuiltInFunction_JEQ0>();
    builtInFunctions[builtInFunctionType::bift_JNEQ0] = std::make_unique<HmgBuiltInFunction_JNEQ0>();
    builtInFunctions[builtInFunctionType::bift_CPY] = std::make_unique<HmgBuiltInFunction_CPY>();
    builtInFunctions[builtInFunctionType::bift_CGT] = std::make_unique<HmgBuiltInFunction_CGT>();
    builtInFunctions[builtInFunctionType::bift_CST] = std::make_unique<HmgBuiltInFunction_CST>();
    builtInFunctions[builtInFunctionType::bift_CGE] = std::make_unique<HmgBuiltInFunction_CGE>();
    builtInFunctions[builtInFunctionType::bift_CSE] = std::make_unique<HmgBuiltInFunction_CSE>();
    builtInFunctions[builtInFunctionType::bift_CEQ] = std::make_unique<HmgBuiltInFunction_CEQ>();
    builtInFunctions[builtInFunctionType::bift_CNEQ] = std::make_unique<HmgBuiltInFunction_CNEQ>();
    builtInFunctions[builtInFunctionType::bift_CGT0] = std::make_unique<HmgBuiltInFunction_CGT0>();
    builtInFunctions[builtInFunctionType::bift_CST0] = std::make_unique<HmgBuiltInFunction_CST0>();
    builtInFunctions[builtInFunctionType::bift_CGE0] = std::make_unique<HmgBuiltInFunction_CGE0>();
    builtInFunctions[builtInFunctionType::bift_CSE0] = std::make_unique<HmgBuiltInFunction_CSE0>();
    builtInFunctions[builtInFunctionType::bift_CEQ0] = std::make_unique<HmgBuiltInFunction_CEQ0>();
    builtInFunctions[builtInFunctionType::bift_CNEQ0] = std::make_unique<HmgBuiltInFunction_CNEQ0>();
    builtInFunctions[builtInFunctionType::bift_TGT] = std::make_unique<HmgBuiltInFunction_TGT>();
    builtInFunctions[builtInFunctionType::bift_TST] = std::make_unique<HmgBuiltInFunction_TST>();
    builtInFunctions[builtInFunctionType::bift_TGE] = std::make_unique<HmgBuiltInFunction_TGE>();
    builtInFunctions[builtInFunctionType::bift_TSE] = std::make_unique<HmgBuiltInFunction_TSE>();
    builtInFunctions[builtInFunctionType::bift_TEQ] = std::make_unique<HmgBuiltInFunction_TEQ>();
    builtInFunctions[builtInFunctionType::bift_TNEQ] = std::make_unique<HmgBuiltInFunction_TNEQ>();
    builtInFunctions[builtInFunctionType::bift_TGT0] = std::make_unique<HmgBuiltInFunction_TGT0>();
    builtInFunctions[builtInFunctionType::bift_TST0] = std::make_unique<HmgBuiltInFunction_TST0>();
    builtInFunctions[builtInFunctionType::bift_TGE0] = std::make_unique<HmgBuiltInFunction_TGE0>();
    builtInFunctions[builtInFunctionType::bift_TSE0] = std::make_unique<HmgBuiltInFunction_TSE0>();
    builtInFunctions[builtInFunctionType::bift_TEQ0] = std::make_unique<HmgBuiltInFunction_TEQ0>();
    builtInFunctions[builtInFunctionType::bift_TNEQ0] = std::make_unique<HmgBuiltInFunction_TNEQ0>();
    builtInFunctions[builtInFunctionType::bift_UNIT] = std::make_unique<HmgBuiltInFunction_UNIT>();
    builtInFunctions[builtInFunctionType::bift_UNITT] = std::make_unique<HmgBuiltInFunction_UNITT>();
    builtInFunctions[builtInFunctionType::bift_URAMP] = std::make_unique<HmgBuiltInFunction_URAMP>();
    builtInFunctions[builtInFunctionType::bift_TIME] = std::make_unique<HmgBuiltInFunction_TIME>();
    builtInFunctions[builtInFunctionType::bift_DT] = std::make_unique<HmgBuiltInFunction_DT>();
    builtInFunctions[builtInFunctionType::bift_FREQ] = std::make_unique<HmgBuiltInFunction_FREQ>();
    builtInFunctions[builtInFunctionType::bift_RAIL] = std::make_unique<HmgBuiltInFunction_RAIL>();
    builtInFunctions[builtInFunctionType::bift_SETVG] = std::make_unique<HmgBuiltInFunction_SETVG>();
    builtInFunctions[builtInFunctionType::bift_GETVG] = std::make_unique<HmgBuiltInFunction_GETVG>();
}


//***********************************************************************
struct NodeConnectionInstructions {
//***********************************************************************
    enum SourceType { sExternalNodeValue, sExternalNodeStepstart, sParam, sReturn };
    struct Source {
        SourceType sourceType = sExternalNodeValue;
        uns sourceIndex = 0;
    };
    struct Destination {
        uns srcParamIndex = 0;
        uns destNodeIndex = 0;
    };
    std::vector<Source> sources;
    std::vector<Destination> destinations; // for controllers
};



}
//***********************************************************************


//***********************************************************************
#endif
//***********************************************************************
