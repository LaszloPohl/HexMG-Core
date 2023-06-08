//***********************************************************************
// HexMG Simulation Controller Header
// Creation date:  2021. 08. 09.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HMG_SIMULATION_CONTROLLER_HEADER
#define HMG_SIMULATION_CONTROLLER_HEADER
//***********************************************************************


//***********************************************************************
#include <vector>
#include <list>
#include <string>
#include "hmgException.h"
#include "hmgCommon.h"
#include "hmgInstructionStream.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class ControlInstructionBase {
//***********************************************************************
public:
    virtual ~ControlInstructionBase() {}
    virtual ControlInstructionType getType()const = 0;
};


//***********************************************************************
class ControlAnalysisInstruction : public ControlInstructionBase {
//***********************************************************************
public:
    AnalysisType type;
    double value;
    ControlAnalysisInstruction(AnalysisType typ, double val) :type{ typ }, value{ val } {}
    ControlInstructionType getType()const { return citAnalysis; }
};


//***********************************************************************
class ControlSaveToInstruction : public ControlInstructionBase {
//***********************************************************************
public:
    unsigned probeIndex;
    bool isBinary, isAppend;
    std::string fileName;
    ControlSaveToInstruction(unsigned indx, bool isBin, bool isApp, const std::string& filNam) 
        :probeIndex{ indx }, isBinary{ isBin }, isAppend{ isApp }, fileName{ filNam } {}
    ControlInstructionType getType()const { return citSave; }
};


}


#endif
