//***********************************************************************
// HexMG CircuitStorage Create function definitions cpp
// Creation date:  2023. 01. 27.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#include "hmgComponent.h"
#include "hmgMultigrid.hpp"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
std::unique_ptr<ComponentDefinition> Create_R1(CDNodeType node_0_type, uns node_0_index, CDNodeType node_1_type, uns node_1_index, CDParamType paramType, rvt value, uns paramIndex) {
//***********************************************************************
	std::unique_ptr<ComponentDefinition> cd = std::make_unique<ComponentDefinition>();
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstR_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = node_0_type;
	cd->nodesConnectedTo[0].index = node_0_index;
	cd->nodesConnectedTo[1].type = node_1_type;
	cd->nodesConnectedTo[1].index = node_1_index;
	cd->params.resize(1);
	cd->params[0].type = paramType;
	cd->params[0].value = value;
	cd->params[0].index = paramIndex;
	return cd;
}


//***********************************************************************
std::unique_ptr<ComponentDefinition> Create_C1(CDNodeType node_0_type, uns node_0_index, CDNodeType node_1_type, uns node_1_index, CDParamType paramType, rvt value, uns paramIndex) {
//***********************************************************************
	std::unique_ptr<ComponentDefinition> cd = std::make_unique<ComponentDefinition>();
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstC_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = node_0_type;
	cd->nodesConnectedTo[0].index = node_0_index;
	cd->nodesConnectedTo[1].type = node_1_type;
	cd->nodesConnectedTo[1].index = node_1_index;
	cd->params.resize(1);
	cd->params[0].type = paramType;
	cd->params[0].value = value;
	cd->params[0].index = paramIndex;
	return cd;
}


//***********************************************************************
std::unique_ptr<ComponentDefinition> Create_Gyrator(CDNodeType node_0_type, uns node_0_index, CDNodeType node_1_type, uns node_1_index, CDNodeType node_2_type, uns node_2_index, 
	CDNodeType node_3_type, uns node_3_index, CDParamType param_0_type, rvt param_0_value, uns param_0_index, CDParamType param_1_type, rvt param_1_value, uns param_1_index) {
//***********************************************************************
	std::unique_ptr<ComponentDefinition> cd = std::make_unique<ComponentDefinition>();

	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtGirator;

	cd->nodesConnectedTo.resize(4);

	cd->nodesConnectedTo[0].type = node_0_type;
	cd->nodesConnectedTo[0].index = node_0_index;
	cd->nodesConnectedTo[1].type = node_1_type;
	cd->nodesConnectedTo[1].index = node_1_index;
	cd->nodesConnectedTo[2].type = node_2_type;
	cd->nodesConnectedTo[2].index = node_2_index;
	cd->nodesConnectedTo[3].type = node_3_type;
	cd->nodesConnectedTo[3].index = node_3_index;

	cd->params.resize(2);

	cd->params[0].type = param_0_type;
	cd->params[0].value = param_0_value;
	cd->params[0].index = param_0_index;
	cd->params[1].type = param_1_type;
	cd->params[1].value = param_1_value;
	cd->params[1].index = param_1_index;

	return cd;
}

   
//***********************************************************************
void CircuitStorage::Create_bimtConstL_1(ComponentAndControllerModelBase* dest) {
//***********************************************************************
    ModelSubCircuit& model = *static_cast<ModelSubCircuit*>(dest);

	model.push_back_component(Create_R1(cdntExternal, 0, cdntExternal, 1, cdptValue, 1e10, 0));
	model.push_back_component(Create_Gyrator(cdntExternal, 0, cdntExternal, 1, cdntInternal, 0, cdntExternal, 1, cdptValue, 1.0, 0, cdptValue, 1.0, 0));
	model.push_back_component(Create_R1(cdntInternal, 0, cdntExternal, 1, cdptValue, 1e10, 0));
	model.push_back_component(Create_C1(cdntInternal, 0, cdntExternal, 1, cdptParam, 0, 0));
}


//***********************************************************************
HmgFunction* CircuitStorage::Create_function_XDiodeEq() {
//***********************************************************************
	HgmCustomFunctionModel fvModel;

	fvModel.nComponentParams = 0;
	fvModel.nParams = 2;
	fvModel.nLocal = 1;

	ParameterIdentifier parId;

	// _CONST V0 1e-12

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_CONST].get();
		lineDesc.value = 1.0e-12;
		parId.parType = ParameterType::ptLocalVar; // V0
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	// _TSE RET P0 V0 V0 P0

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_TSE].get();
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // P0
		parId.parIndex = 2;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptLocalVar; // V0
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptLocalVar; // V0
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // P0
		parId.parIndex = 2;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	// _MULC RET RET 1e13

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MULC].get();
		lineDesc.value = 1.0e13;
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	// _ADDC RET RET 1

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_ADDC].get();
		lineDesc.value = 1.0;
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	// _LN RET RET

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_LN].get();
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	// _MULC RET RET 0.026

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MULC].get();
		lineDesc.value = 0.026;
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	// _MUL RET RET P1

	{
		LineDescription lineDesc;
		lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MUL].get();
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // RET
		parId.parIndex = 0;
		lineDesc.parameters.push_back(parId);
		parId.parType = ParameterType::ptParam; // P1
		parId.parIndex = 3;
		lineDesc.parameters.push_back(parId);
		fvModel.lines.push_back(lineDesc);
	}

	internalCustomFunctions.push_back(std::make_unique<HmgF_CustomFunction>(fvModel));

	return internalCustomFunctions.back().get();
}


//***********************************************************************
NodeConnectionInstructions CircuitStorage::Create_ConnectionInstructions_XDiode() {
//***********************************************************************
	NodeConnectionInstructions pack; // Model_Function_Controlled_I_with_const_G ctor moves it !
	NodeConnectionInstructions::ConnectionInstruction ci;

	ci.nodeOrVarType = NodeConnectionInstructions::sExternalNodeValue;
	ci.functionParamIndex = 2;	// in the function: RET = 0, P0 = 2, P1 = 3
	ci.nodeOrVarIndex = 2;		// in the FCI:		N0 = 0, N1 = 1, Y0 = 2
	pack.load.push_back(ci);

	ci.nodeOrVarType = NodeConnectionInstructions::sParam;
	ci.functionParamIndex = 3;	// in the function: RET = 0, P0 = 2, P1 = 3
	ci.nodeOrVarIndex = 0;		// in the FCI:		P0 = 0
	pack.load.push_back(ci);

	return pack;
}



//***********************************************************************
}
//***********************************************************************
