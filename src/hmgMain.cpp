//***********************************************************************
// HexMG Core main() function
// Creation date:  2023. 01. 24.
// Creator:        Pohl László
//***********************************************************************



//***********************************************************************
#include "hmgComponent.h"
#include "hmgFunction.hpp"
#include "hmgMultigrid.hpp"
#include <chrono>
#include <ratio>
#include "hmgHMGFileReader.h"
#include "hmgInstructionProcessor.h"
//***********************************************************************



//***********************************************************************
using namespace nsHMG;
using namespace std;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::milli;
//***********************************************************************


//***********************************************************************
void setR(std::unique_ptr<ComponentDefinition>& cd, uns externalIndex, rvt G) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntExternal;
	cd->nodesConnectedTo[0].index = externalIndex;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[1].index = 0;
	cd->params.resize(1);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = G;
}


//***********************************************************************
void setI(std::unique_ptr<ComponentDefinition>& cd, rvt I) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstI_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = 0;
	cd->params.resize(4);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = I;
	cd->params[1].type = CDParamType::cdptValue;
	cd->params[1].value = I;
	cd->params[2].type = CDParamType::cdptValue;
	cd->params[2].value = rvt0;
	cd->params[3].type = CDParamType::cdptValue;
	cd->params[3].value = rvt0;
}


//***********************************************************************
void setV0(std::unique_ptr<ComponentDefinition>& cd, rvt V, rvt G) { // setV0(cd, 1, T0, G0);
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstVI;
	cd->nodesConnectedTo.resize(3);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = 1;
	cd->nodesConnectedTo[2].type = CDNodeType::cdntUnconnected;
	cd->nodesConnectedTo[2].index = 0;
	cd->params.resize(5);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = V;
	cd->params[1].type = CDParamType::cdptValue;
	cd->params[1].value = V;
	cd->params[2].type = CDParamType::cdptValue;
	cd->params[2].value = rvt0; // in AC GND is 0
	cd->params[3].type = CDParamType::cdptValue;
	cd->params[3].value = rvt0;
	cd->params[4].type = CDParamType::cdptValue;
	cd->params[4].value = G;
}


//***********************************************************************
void setIStep(std::unique_ptr<ComponentDefinition>& cd, uns intNode, rvt Idc0, rvt Idc, rvt Iac, rvt Phi) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstI_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = intNode;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = 1;
	cd->params.resize(4);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = Idc0;
	cd->params[1].type = CDParamType::cdptValue;
	cd->params[1].value = Idc;
	cd->params[2].type = CDParamType::cdptValue;
	cd->params[2].value = Iac;
	cd->params[3].type = CDParamType::cdptValue;
	cd->params[3].value = Phi;
}


//***********************************************************************
void setR0(std::unique_ptr<ComponentDefinition>& cd, rvt G, uns nodeIndex, uns groundIndex) {
//***********************************************************************
	cd->setDefaultValueRailIndex(groundIndex);
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = nodeIndex;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = groundIndex;
	cd->params.resize(1);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = G;
}


//***********************************************************************
void setC0(std::unique_ptr<ComponentDefinition>& cd, rvt C) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstC_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = 1;
	cd->params.resize(1);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = C;
}


//***********************************************************************
void setGirator0(std::unique_ptr<ComponentDefinition>& cd) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtGirator;
	cd->nodesConnectedTo.resize(4);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = 1;
	cd->nodesConnectedTo[2].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[2].index = 1;
	cd->nodesConnectedTo[3].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[3].index = 0;
	cd->params.resize(2);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = -1.0;
	cd->params[1].type = CDParamType::cdptValue;
	cd->params[1].value = -1.0;
}


//***********************************************************************
void setDissipator(std::unique_ptr<ComponentDefinition>& cd, uns componentModelIndex, rvt G, rvt Gsrc, uns nodeIndex, uns groundIndex, uns srcIndex) {
//***********************************************************************
	cd->setDefaultValueRailIndex(groundIndex);
	cd->modelType = cmtCustom;
	cd->modelIndex = componentModelIndex;
	cd->nodesConnectedTo.resize(3);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = nodeIndex;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = groundIndex;
	cd->nodesConnectedTo[2].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[2].index = srcIndex;
	cd->params.resize(2);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = G;
	cd->params[1].type = CDParamType::cdptValue;
	cd->params[1].value = Gsrc;
}


//***********************************************************************
void setCell(std::unique_ptr<ComponentDefinition>& cd, uns componentModelIndex, uns nodeNum, uns index1, uns index2 = 0, uns index3 = 0, uns index4 = 0) {
//***********************************************************************
	cd->modelType = cmtCustom;
	cd->modelIndex = componentModelIndex;
	cd->nodesConnectedTo.resize(nodeNum);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = index1;
	if (nodeNum > 1) {
		cd->nodesConnectedTo[1].type = CDNodeType::cdntInternal;
		cd->nodesConnectedTo[1].index = index2;
	}
	if (nodeNum > 2) {
		cd->nodesConnectedTo[2].type = CDNodeType::cdntInternal;
		cd->nodesConnectedTo[2].index = index3;
	}
	if (nodeNum > 3) {
		cd->nodesConnectedTo[3].type = CDNodeType::cdntInternal;
		cd->nodesConnectedTo[3].index = index4;
	}
}


//***********************************************************************
void runStepDC() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
		cout << "\n*********\n" << endl;
	ComponentBase::DefectCollector d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
#ifdef HMG_DEBUGPRINT
		//gc.fullCircuitInstances[0].component->printNodeDefectDC(0);
#endif
		cout << d.sumDefect << " " << d.maxDefect << " " << d.nodeNum << endl;
	ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
#ifdef HMG_DEBUGPRINT
		//gc.fullCircuitInstances[0].component->printNodeErrorDC(0);
#endif
		cout << v.sumDefect << " " << v.maxDefect << " " << v.nodeNum << endl;
		cout << "\n*********\n" << endl;
	gc.fullCircuitInstances[0].component->forwsubs(true);
	gc.fullCircuitInstances[0].component->backsubs(true);
	gc.fullCircuitInstances[0].component->acceptIterationDC(false);
	gc.fullCircuitInstances[0].component->acceptStepDC();
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
		cout << "\n*********\n" << endl;
	d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
#ifdef HMG_DEBUGPRINT
		//gc.fullCircuitInstances[0].component->printNodeDefectDC(0);
#endif
		cout << d.sumDefect << " " << d.maxDefect << " " << d.nodeNum << endl;
	v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
#ifdef HMG_DEBUGPRINT
		//gc.fullCircuitInstances[0].component->printNodeErrorDC(0);
#endif
		cout << v.sumDefect << " " << v.maxDefect << " " << v.nodeNum << endl;
		cout << "\n*********\n" << endl;
#ifdef HMG_DEBUGPRINT
		//gc.fullCircuitInstances[0].component->printNodeValueDC(0);
#endif
}


//***********************************************************************
void runStepLessPrintDC() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
	ComponentBase::DefectCollector d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
	ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
	gc.fullCircuitInstances[0].component->forwsubs(true);
	gc.fullCircuitInstances[0].component->backsubs(true);
	gc.fullCircuitInstances[0].component->acceptIterationDC(false);
	gc.fullCircuitInstances[0].component->acceptStepDC();
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
	d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
	v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
#ifdef HMG_DEBUGPRINT
		gc.fullCircuitInstances[0].component->printNodeValueDC(0);
#endif
}


//***********************************************************************
void runStepLessPrintAC() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();
	gc.fullCircuitInstances[0].component->deleteD(false);
	gc.fullCircuitInstances[0].component->calculateCurrent(false);
	gc.fullCircuitInstances[0].component->forwsubs(false);
	gc.fullCircuitInstances[0].component->backsubs(false);
	gc.fullCircuitInstances[0].component->acceptIterationAndStepAC();
#ifdef HMG_DEBUGPRINT
		gc.fullCircuitInstances[0].component->printNodeValueAC(0);
#endif
}


//***********************************************************************
void runStepNoPrintDC() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();
	for (uns i = 0; i < 10; i++) {
		gc.fullCircuitInstances[0].component->calculateValueDC();
		gc.fullCircuitInstances[0].component->deleteD(true);
		gc.fullCircuitInstances[0].component->calculateCurrent(true);
		ComponentBase::DefectCollector d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
		ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
		gc.fullCircuitInstances[0].component->forwsubs(true);
		gc.fullCircuitInstances[0].component->backsubs(true);
		gc.fullCircuitInstances[0].component->acceptIterationDC(false);
		//std::cout << i << " \tt = " << ComponentBase::SimControl::timeStepStop.getValue() << " sec \t" << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValue() << std::endl;
	}
	gc.fullCircuitInstances[0].component->acceptStepDC();
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
}


//***********************************************************************
void setRinternal(std::unique_ptr<ComponentDefinition>& cd, rvt G, uns internalIndex = 0, uns groundIndex = 1) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = internalIndex;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntRail;
	cd->nodesConnectedTo[1].index = groundIndex;
	cd->params.resize(1);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = G;
}


//***********************************************************************
void setRinternal2(std::unique_ptr<ComponentDefinition>& cd, rvt G, uns internalIndex1, uns internalIndex2) {
//***********************************************************************
	cd->modelType = cmtBuiltIn;
	cd->modelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[0].index = internalIndex1;
	cd->nodesConnectedTo[1].type = CDNodeType::cdntInternal;
	cd->nodesConnectedTo[1].index = internalIndex2;
	cd->params.resize(1);
	cd->params[0].type = CDParamType::cdptValue;
	cd->params[0].value = G;
}


//***********************************************************************
void probaSzimulacio2() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();
	constexpr rvt Cth = rvt(0.00642); // 0.625
	constexpr rvt G = rvt(0.5*1.0 / 375.0); // 0.0025
	constexpr rvt I = rvt(0.01); // 0.125

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nParams_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 0, 0, 0, 0, 0, 0, 0 }, InternalNodeVarSizePack{ 1, 0, 0 }, false, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setRinternal(cd, G);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, I, I, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setV0(cd, 20, 100'000.0);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(0, 1, 0);
	gc.fullCircuitInstances[0].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = true;

	runStepDC();

	rvt steptime = 0.1;
	for (uns i = 0; i < 14; i++, steptime *= 2) {
		std::cout << "******************************************************************" << std::endl;
		SimControl::stepTransientWithDT(steptime);
		runStepNoPrintDC();
#ifdef HMG_DEBUGPRINT
		std::cout << i << " \tt = " << SimControl::timeStepStop.getValueDC() << " sec \t" << gc.fullCircuitInstances[0].component->getInternalNode(0)->getValueDC() << std::endl;
		std::cout << SimControl::timeStepStop.getValueDC() << "    " << gc.fullCircuitInstances[0].component->getInternalNode(0)->getValueDC() << std::endl;
#endif
	}

	SimControl::setFinalDC();
	runStepLessPrintDC();
	runStepLessPrintDC();
	runStepLessPrintDC();

	SimControl::setComplexFrequencyForAC(0.1);

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()
	runStepLessPrintAC();
}


//***********************************************************************
void probaSzimulacio3() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();

	//***********************************************************************
	// Sunred fa építése
	//***********************************************************************

	hmgSunred::ReductionTreeInstructions instr;

	instr.data.resize(1);

	instr.data[0].resize(1);
	instr.data[0][0] = { 0, 0, 0, 1 };

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	// 0

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nParams_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 1, 0, 0, 0, 0, 0, 0 }, InternalNodeVarSizePack{ 2, 0, 0 }, false, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, 0.1);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 600'000.0, 600'000.0);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setR0(cd, 100'000.0, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setGirator0(cd);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR0(cd, 1.0 / 100'000.0, 1, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 1, -6, -6, -6, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 1

	mc = make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 1, 0, 0, 0, 0, 0, 0 }, InternalNodeVarSizePack{ 1, 0, 0 }, false, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, 0.1);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setRinternal(cd, 0.1);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0, 6, 6, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 2

	//mc = make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 0, 0, 0, 0, 0, 0, 0 }, InternalNodeVarSizePack{ 1, 0, 0 }, false, SolutionType::stSunRed, &instr);
	mc = make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 0, 0, 0, 0, 0, 0, 0 }, InternalNodeVarSizePack{ 1, 0, 0 }, false, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 1, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 1, 1, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(2, 1, 0);
	SimControl::setFinalDC();
	gc.fullCircuitInstances[0].component->resetNodes(true);
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
	cout << "\n*********\n" << endl;
	ComponentBase::DefectCollector d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
	cout << d.sumDefect << " " << d.maxDefect << " " << d.nodeNum << endl;
	ComponentBase::DefectCollector v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
	cout << v.sumDefect << " " << v.maxDefect << " " << v.nodeNum << endl;
	cout << "\n*********\n" << endl;
	gc.fullCircuitInstances[0].component->forwsubs(true);
	gc.fullCircuitInstances[0].component->backsubs(true);
	gc.fullCircuitInstances[0].component->acceptIterationDC(false);
	gc.fullCircuitInstances[0].component->acceptStepDC();
	gc.fullCircuitInstances[0].component->calculateValueDC();
	gc.fullCircuitInstances[0].component->deleteD(true);
	gc.fullCircuitInstances[0].component->calculateCurrent(true);
	cout << "\n*********\n" << endl;
	d = gc.fullCircuitInstances[0].component->collectCurrentDefectDC();
	cout << d.sumDefect << " " << d.maxDefect << " " << d.nodeNum << endl;
	v = gc.fullCircuitInstances[0].component->collectVoltageDefectDC();
	cout << v.sumDefect << " " << v.maxDefect << " " << v.nodeNum << endl;
	cout << "\n*********\n" << endl;

	SimControl::setFinalDC();
	cout << "\n********* 1\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 2\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 3\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 4\n" << endl;

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()
	runStepLessPrintAC();

}

//***********************************************************************
void probaSzimulacio4() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();


	HgmCustomFunctionModel funcModel;
	funcModel.nParams = 2;
	funcModel.nLocal = 1;

	ParameterIdentifier parId;
	LineDescription lineDesc;

	// U*U

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MUL].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 2;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 2;
	lineDesc.parameters.push_back(parId);
	funcModel.lines.push_back(lineDesc);

	// (U*U)*G

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MUL].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 3;
	lineDesc.parameters.push_back(parId);
	funcModel.lines.push_back(lineDesc);

	NodeConnectionInstructions functionSources; // Model_Function_Controlled_I_with_const_G ctor moves it !
	NodeConnectionInstructions::ConnectionInstruction src;
	src.nodeOrVarType = NodeConnectionInstructions::sExternalNodeValue;
	src.nodeOrVarIndex = 2; // Usrc
	src.functionParamIndex = 0;
	functionSources.load.push_back(src);
	src.nodeOrVarType = NodeConnectionInstructions::sParam;
	src.nodeOrVarIndex = 1; // Gsrc
	src.functionParamIndex = 2;
	functionSources.load.push_back(src);

	std::unique_ptr<Model_Function_Controlled_I_with_const_G> mf;

	// (uns nNormalINodes_, uns nControlINodes_, uns nParams_, NodeConnectionInstructions functionSources_, const HmgFunction& controlFunction_)

	std::unique_ptr<HmgF_CustomFunction> fp = std::make_unique<HmgF_CustomFunction>(funcModel);
	mf = make_unique<Model_Function_Controlled_I_with_const_G>(1, 0, 2, functionSources, std::vector<uns>(), fp.get());
	cuns funcIIndex = (uns)gc.models.size();
	gc.models.push_back(std::move(mf)); // 0

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(ExternalConnectionSizePack{ 0, 0, 0, 0, 0, 0, 0 }, InternalNodeVarSizePack{ 2, 0, 0 }, false, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setRinternal(cd, 0.1, 0, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setI(cd, 0.25);					mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setDissipator(cd, funcIIndex, 0.5, 0.1, 1, 1, 0);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, I, I, 0);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);			mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setV0(cd, 20, 100'000.0);		mc->push_back_component(std::move(cd));

	cuns cellIndex = (uns)gc.models.size();
	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(cellIndex, 1, 0);
	gc.fullCircuitInstances[0].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = true;
/*
	runStepDC();

	rvt steptime = 0.1;
	for (uns i = 0; i < 14; i++, steptime *= 2) {
		std::cout << "******************************************************************" << std::endl;
		SimControl::stepTransient(steptime);
		runStepNoPrintDC();
#ifdef HMG_DEBUGPRINT
		std::cout << i << " \tt = " << SimControl::timeStepStop.getValueDC() << " sec \t" << gc.fullCircuitInstances[0].component->getInternalNode(0)->getValueDC() << std::endl;
		std::cout << SimControl::timeStepStop.getValueDC() << "    " << gc.fullCircuitInstances[0].component->getInternalNode(0)->getValueDC() << std::endl;
#endif
	}
*/
	SimControl::setFinalDC();
	runStepLessPrintDC();
	runStepLessPrintDC();
	runStepLessPrintDC();

	SimControl::setComplexFrequencyForAC(0.1);

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()
	runStepLessPrintAC();
}


// Be kell állítani a CircuitNodeData::defaultNodeValue vektort (méret és értékek) az áramkördefiníciós adatok alapján, utána lehet létrehozni az áramkört


//***********************************************************************
int main(int n, const char** params) {
//***********************************************************************
	Rails::resize(1);
	Rails::reset();
	try {
		HmgFileReader reader;

		//most("start");
		//reader.ReadFile(params[1]);
		reader.ReadFile("c:/!D/Kutatás/cikkek/tanszeki/HexMG/proba_9.hmg");
		InstructionStream is;
		reader.convertToInstructionStream(is);
		//most("convertToInstructionStream");
		reader.clear();
		//most("clear");

		InstructionProcessor ip;
		ip.addInstructionStream(is);
		InstructionProcessor::waitToFinish(ip);
		//most("process instructions");
		//esemenyek_kiirasa();
	}
	catch (const hmgExcept& err) {
		std::cerr << err.what() << std::endl;
	}
	//cout << "\n\n" << sizeof(NodeVariable) << endl;

	return 0;
}
/*
int main() {
	NodeVariable a;
	//a.turnIntoNode(0, false);
	//a.createAC();

	int isAdd = 1;

//	printf("isAdd? 0 or 1: ");
//	scanf_s("%d", &isAdd);

//	a.nodePtr->d.setIsGnd(isAdd == 0);
	//a.acNodePtr->d.setIsGnd(isAdd == 0);

	const auto startTime = high_resolution_clock::now();

	constexpr siz N = 500'000'000;
	cplx val = { 1.0, 0.2 };

	//for (siz i = 0; i < N; i++) {
	//	a.fetchAdd(1.5);
	//	a.fetchSub(1.4);
	//}
	a.setIsConcurrent(true);
	for (siz i = siz0; i < N; i++) {
		a.d.fetch_add(val, a.isConcurrent, a.isGnd);
		a.d.fetch_sub(val, a.isConcurrent, a.isGnd);
		a.d.fetch_add(val, a.isConcurrent, a.isGnd);
	}
	//a.acNodePtr->d.setIsConcurrent(false);
	//for (siz i = siz0; i < N; i++) {
	//	a.acNodePtr->d.fetch_add(val);
	//	a.acNodePtr->d.fetch_sub(val);
	//	a.acNodePtr->d.fetch_add(val);
	//}


	const auto endTime = high_resolution_clock::now();

	double durr = duration_cast<duration<double>>(endTime - startTime).count();
	double M_op_per_sec = 1e-6 * 3 * N / durr;

	printf("Time: %fs\nM_op_per_sec = %f\n", durr, M_op_per_sec);

	cout << a.d.loadCplx(a.isConcurrent) << endl;
	cout << sizeof(a.d) << endl;
}
*/


//***********************************************************************
int main_1() {
//***********************************************************************

	//SpiceExpression se;
	//se.buildFromString("PUKI(T)+.SQRT(_Q)+.INV(.RATIO(-.PWL(T,25,-20,58+3,-54,25,65K),65K,H))*MUL + V(2) + PUKI() + V(@3,4) - 2 * _PI * I(88) * T");


	Rails::resize(2);
	// Rails::V[0]->defaultNodeValue is mandatory 0 !!!
	Rails::SetVoltage(1, 20.0);
	Rails::reset();

	CircuitStorage& gc = CircuitStorage::getInstance();

	HmgBuiltInFunction_SQRT fsqr;
	rvt wf[3] = { 0.0, 1.0, 3.5 };
	uns par[4] = { 0, 3, 1, 2 };
	fsqr.evaluate(par, wf, nullptr, LineDescription(), nullptr);
	std::cout << "\nDERIVED: " << fsqr.devive(par, wf, nullptr, 2, LineDescription(), nullptr) << std::endl;
	std::cout << "RET: " << wf[0] << "\n" << std::endl;

	HgmCustomFunctionModel fvModel;
	fvModel.nParams = 4;
	fvModel.nLocal = 3;

	ParameterIdentifier parId;
	LineDescription lineDesc;

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MUL].get();
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 2;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 3;
	lineDesc.parameters.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_SQRT].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 1;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 4;
	lineDesc.parameters.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_ADD].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 2;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 1;
	lineDesc.parameters.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MUL].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 2;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 5;
	lineDesc.parameters.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	HmgF_CustomFunction fuggveny(fvModel);
	cuns ifSize = fuggveny.getN_IndexField();
	cuns wfSize = fuggveny.getN_WorkingField(); // local vars (including nested function calls)
	std::vector<uns> indexField(ifSize);
	std::vector<rvt> workField(wfSize + 5); // 1 ret + 4 param

	indexField[0] = 0;
	indexField[1] = 5; // workField index
	indexField[2] = 1;
	indexField[3] = 2;
	indexField[4] = 3;
	indexField[5] = 4;

	fuggveny.fillIndexField(&indexField[0]);

	workField[0] = 0.0;
	workField[1] = 3.0;
	workField[2] = 4.0;
	workField[3] = 9.0;
	workField[4] = 2.0;

	fuggveny.evaluate(&indexField[0], &workField[0], nullptr, LineDescription(), nullptr);

	std::cout << "\nDERIVED: " << fuggveny.devive(&indexField[0], &workField[0], nullptr, 2, LineDescription(), nullptr) << std::endl;
	std::cout << "RET: " << workField[0] << "\n" << std::endl;

	std::vector<std::unique_ptr<HmgFunction>> namelessCustomFunctions;
	namelessCustomFunctions.push_back(std::make_unique<HmgF_CustomFunction>(fvModel));

	HgmCustomFunctionModel fvModel2;
	fvModel2.nParams = 5;
	fvModel2.nLocal = 2;

	lineDesc.pFunction = namelessCustomFunctions[0].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 2;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 3;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 4;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 5;
	lineDesc.parameters.push_back(parId);
	fvModel2.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_MUL].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 1;
	lineDesc.parameters.push_back(parId);
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 6;
	lineDesc.parameters.push_back(parId);
	fvModel2.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[bift_SQRT].get();
	lineDesc.parameters.clear();
	parId.parType = ParameterType::ptParam;
	parId.parIndex = 0;
	lineDesc.parameters.push_back(parId);
	parId.parType = ParameterType::ptLocalVar;
	parId.parIndex = 1;
	lineDesc.parameters.push_back(parId);
	fvModel2.lines.push_back(lineDesc);

	HmgF_CustomFunction fuggveny2(fvModel2);
	cuns ifSize2 = fuggveny2.getN_IndexField();
	cuns wfSize2 = fuggveny2.getN_WorkingField(); // local vars (including nested function calls)
	indexField.resize(ifSize2);
	workField.resize(wfSize + 6); // 1 ret + 5 param

	indexField[0] = 0;
	indexField[1] = 6;
	indexField[2] = 1;
	indexField[3] = 2;
	indexField[4] = 3;
	indexField[5] = 4;
	indexField[6] = 5;

	fuggveny2.fillIndexField(&indexField[0]);

	workField[0] = 0.0;
	workField[1] = 3.0;
	workField[2] = 4.0;
	workField[3] = 9.0;
	workField[4] = 2.0;
	workField[5] = 5.0;

	fuggveny2.evaluate(&indexField[0], &workField[0], nullptr, LineDescription(), nullptr);

	std::cout << "\nvvvvvvvvv\n\n" << fuggveny2.devive(&indexField[0], &workField[0], nullptr, 2, LineDescription(), nullptr) << std::endl;
	std::cout << workField[0] << "\n" << std::endl;


/*
	CircuitNodeData a(0);
	a.createACIfItDoesNotExist();

	int isAdd = 1;

	printf("isAdd? 0 or 1: ");
	scanf_s("%d", &isAdd);

	a.d.setIsGnd(isAdd == 0);
	//a.acNodePtr->d.setIsGnd(isAdd == 0);

	const auto startTime = high_resolution_clock::now();

	constexpr siz N = 500'000'000;
	rvt val = 1.0;

	//for (siz i = 0; i < N; i++) {
	//	a.fetchAdd(1.5);
	//	a.fetchSub(1.4);
	//}
	a.d.setIsConcurrent(false);
	for (siz i = siz0; i < N; i++) {
		a.d.fetch_add(val);
		a.d.fetch_sub(val);
		a.d.fetch_add(val);
	}
	//a.acNodePtr->d.setIsConcurrent(false);
	//for (siz i = siz0; i < N; i++) {
	//	a.acNodePtr->d.fetch_add(val);
	//	a.acNodePtr->d.fetch_sub(val);
	//	a.acNodePtr->d.fetch_add(val);
	//}


	const auto endTime = high_resolution_clock::now();

	double durr = duration_cast<duration<double>>(endTime - startTime).count();
	double M_op_per_sec = 1e-6 * 3 * N / durr;

	printf("Time: %fs\nM_op_per_sec = %f\n", durr, M_op_per_sec);

	cout << a.d.load() << endl;
	cout << sizeof(a.d) << endl;
*/

	const auto startTime_proba = high_resolution_clock::now();
	probaSzimulacio4();
	const auto endTime_proba = high_resolution_clock::now();

	duration<double> time_span = duration_cast<duration<double>>(endTime_proba - startTime_proba);

	cout << "proba time: " << time_span.count() << " seconds\n" << endl;

	//cout << sizeof(NodeData) << '\n' << sizeof(CircuitNodeDataAC) << endl;
	cout << sizeof(ComponentConstR_1) << '\n' << sizeof(ComponentSubCircuit) << endl;
	cout << noexcept(is_true_error(false, "const char* who_threw", "const char* what")) << endl;
	cout << sizeof(matrix<cplx>) << endl;


	return 0;
}

