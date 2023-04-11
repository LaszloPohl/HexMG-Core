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
//***********************************************************************


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
    virtual int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept = 0;
    // owner is Component_Function_Controlled_I_with_const_G or Controller
    // return: true if the caller function must be exit (ifr instruction), false: every other cases
    //***********************************************************************
    rvt devive(cuns* index, crvt* cworkField, ComponentAndControllerBase* owner, cuns variableIndex)const noexcept {
    //***********************************************************************
        rvt* workField = const_cast<rvt*>(cworkField); // changes and restores so no change
        crvt value = workField[index[0]];
        crvt var = workField[index[variableIndex]];
        crvt dx = (abs(value) + abs(var)) * rvt(1.0e-9);
        workField[index[variableIndex]] += dx;
        evaluate(index, workField, owner);
        crvt ret = (workField[index[0]] - value) / dx;
        // restoring the workfield
        workField[index[0]] = value;
        workField[index[variableIndex]] = var;
        return ret;
    }
    uns getN_Param()const noexcept { return nParam; }
    uns getN_IndexField()const noexcept { return nIndexField; }
    uns getN_WorkingField()const noexcept { return nWorkingField; }
    virtual void fillIndexField(uns* indexField)const noexcept = 0;
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
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { workField[index[0]] = value; return 0; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_If1 final : public HmgFunction{
// if ret is true, caller executes the next function, else it is skipped.
//***********************************************************************
public:
    HmgF_If1() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { return workField[index[0]] != rvt0 ? 0 : 1; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Else1 final : public HmgFunction{
// if ret is false, caller executes the next function, else it is skipped.
//***********************************************************************
public:
    HmgF_Else1() : HmgFunction{ 0, 2, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { return workField[index[0]] == rvt0 ? 0 : 1; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_IfValue1 final : public HmgFunction{
// if par1 is true, ret = par2
//***********************************************************************
public:
    HmgF_IfValue1() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { 
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
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { 
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
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { 
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
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_Controller_Node_StepStart final : public HmgFunction{
//***********************************************************************
    uns nodeIndex;
public:
    HmgF_Load_Controller_Node_StepStart(uns nodeIndex_) : HmgFunction{ 0, 2, 0 }, nodeIndex{ nodeIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_Controller_mVar_StepStart final : public HmgFunction{
//***********************************************************************
    uns varIndex;
public:
    HmgF_Load_Controller_mVar_StepStart(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Load_Controller_mVar_Value final : public HmgFunction{
//***********************************************************************
    uns varIndex;
public:
    HmgF_Load_Controller_mVar_Value(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Set_Controller_mVar_Value final : public HmgFunction{
// ret => owner->mVars[nodeIndex], not changing ret
//***********************************************************************
    uns varIndex;
public:
    HmgF_Set_Controller_mVar_Value(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_Set_Controller_mVar_ValueFromStepStart final : public HmgFunction{
// ret is not used, can be any element of the workField
//***********************************************************************
    uns varIndex;
public:
    HmgF_Set_Controller_mVar_ValueFromStepStart(uns varIndex_) : HmgFunction{ 0, 2, 0 }, varIndex{ varIndex_ } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override; // in hmgComponent.h
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_sqrt final : public HmgFunction{
//***********************************************************************
public:
    HmgF_sqrt() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { workField[index[0]] = sqrt(workField[index[2]]); return 0; }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_sqare final : public HmgFunction{
//***********************************************************************
public:
    HmgF_sqare() : HmgFunction{ 1, 3, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { 
        crvt value = workField[index[2]]; 
        workField[index[0]] = value * value; 
        return 0; 
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HmgF_opPlus final : public HmgFunction{
//***********************************************************************
public:
    HmgF_opPlus() : HmgFunction{ 2, 4, 0 } {}
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override { 
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
    int evaluate(cuns* index, rvt* workField, ComponentAndControllerBase* owner)const noexcept override {
        workField[index[0]] = workField[index[2]] * workField[index[3]]; 
        return 0;
    }
    void fillIndexField(uns* indexField)const noexcept override {}
};


//***********************************************************************
class HgmCustomFunctionModel {
//***********************************************************************
public:
    //***********************************************************************
    //enum FunctionType { ftBuiltInFunction, ftGlobalFunction, ftLocalFunction };
    enum ParameterType { ptParam, ptLocalVar, ptPrev }; // prev: index field of the previous function (or the owner function, if no previous)
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
        std::vector<ParameterIdentifier> parIndex; // parIndex[0] is the return
    };
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

            switch (line.parIndex[0].parType) {
                case HgmCustomFunctionModel::ParameterType::ptParam:
                    indexField[base] = indexField[line.parIndex[0].parIndex]; // = params[parIndex]
                    break;
                case HgmCustomFunctionModel::ParameterType::ptLocalVar:
                    indexField[base] = indexField[1] + line.parIndex[0].parIndex; // = workField[parIndex] a.k.a localvariable[parIndex]
                    break;
                case HgmCustomFunctionModel::ParameterType::ptPrev:
                    indexField[base] = indexField[prev + line.parIndex[0].parIndex]; // = previous function index field [parIndex]
                    break;
            }
            
            // workField index

            indexField[base + 1] = indexField[1] + model.nLocal;
            
            // params

            cuns nParams = line.pFunction->getN_Param();
            for (uns i = 0; i < nParams; i++) {
                switch (line.parIndex[i + 1].parType) {
                    case HgmCustomFunctionModel::ParameterType::ptParam:
                        indexField[base + 2 + i] = indexField[line.parIndex[i + 1].parIndex]; // = params[parIndex]
                        break;
                    case HgmCustomFunctionModel::ParameterType::ptLocalVar:
                        indexField[base + 2 + i] = indexField[1] + line.parIndex[i + 1].parIndex; // = workField[parIndex] a.k.a localvariable[parIndex]
                        break;
                    case HgmCustomFunctionModel::ParameterType::ptPrev:
                        indexField[base + 2 + i] = indexField[prev + line.parIndex[i + 1].parIndex]; // = previous function index field [parIndex]
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
    int evaluate(cuns* indexField, rvt* workField, ComponentAndControllerBase* owner)const noexcept override {
    //***********************************************************************
        uns base = model.nParams + 2;
        for (uns i = 0; i < model.lines.size(); i++) {
            const auto& line = model.lines[i];
            int jumpValue = line.pFunction->evaluate(indexField + base, workField, owner);
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
    builtInFunctions.resize(builtInFunctionType::futSize);
    builtInFunctions[builtInFunctionType::futOpPlus] = std::make_unique<HmgF_opPlus>();
    builtInFunctions[builtInFunctionType::futOpMul] = std::make_unique<HmgF_opMul>();
    builtInFunctions[builtInFunctionType::futSqrt] = std::make_unique<HmgF_sqrt>();
    builtInFunctions[builtInFunctionType::futSquare] = std::make_unique<HmgF_sqare>();
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
