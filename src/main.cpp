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
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::external;
	cd->nodesConnectedTo[0].index = externalIndex;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[1].index = 0;
	cd->params.resize(1);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = G;
}


//***********************************************************************
void setI(std::unique_ptr<ComponentDefinition>& cd, rvt I) {
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstI_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = 0;
	cd->params.resize(4);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = I;
	cd->params[1].type = ComponentDefinition::CDParamType::value;
	cd->params[1].value = I;
	cd->params[2].type = ComponentDefinition::CDParamType::value;
	cd->params[2].value = rvt0;
	cd->params[3].type = ComponentDefinition::CDParamType::value;
	cd->params[3].value = rvt0;
}


//***********************************************************************
void setV0(std::unique_ptr<ComponentDefinition>& cd, rvt V, rvt G) { // setV0(cd, 1, T0, G0);
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstV;
	cd->nodesConnectedTo.resize(3);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = 1;
	cd->nodesConnectedTo[2].type = ComponentDefinition::CDNodeType::unconnected;
	cd->nodesConnectedTo[2].index = 0;
	cd->params.resize(5);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = V;
	cd->params[1].type = ComponentDefinition::CDParamType::value;
	cd->params[1].value = V;
	cd->params[2].type = ComponentDefinition::CDParamType::value;
	cd->params[2].value = rvt0; // in AC GND is 0
	cd->params[3].type = ComponentDefinition::CDParamType::value;
	cd->params[3].value = rvt0;
	cd->params[4].type = ComponentDefinition::CDParamType::value;
	cd->params[4].value = G;
}


//***********************************************************************
void setIStep(std::unique_ptr<ComponentDefinition>& cd, uns intNode, rvt Idc0, rvt Idc, rvt Iac, rvt Phi) {
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstI_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = intNode;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = 1;
	cd->params.resize(4);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = Idc0;
	cd->params[1].type = ComponentDefinition::CDParamType::value;
	cd->params[1].value = Idc;
	cd->params[2].type = ComponentDefinition::CDParamType::value;
	cd->params[2].value = Iac;
	cd->params[3].type = ComponentDefinition::CDParamType::value;
	cd->params[3].value = Phi;
}


//***********************************************************************
void setR0(std::unique_ptr<ComponentDefinition>& cd, rvt G, uns nodeIndex, uns groundIndex) {
//***********************************************************************
	cd->setNodesDefValueIndex(groundIndex);
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = nodeIndex;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = groundIndex;
	cd->params.resize(1);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = G;
}


//***********************************************************************
void setC0(std::unique_ptr<ComponentDefinition>& cd, rvt C) {
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstC_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = 1;
	cd->params.resize(1);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = C;
}


//***********************************************************************
void setGirator0(std::unique_ptr<ComponentDefinition>& cd) {
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtGirator;
	cd->nodesConnectedTo.resize(4);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = 0;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = 1;
	cd->nodesConnectedTo[2].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[2].index = 1;
	cd->nodesConnectedTo[3].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[3].index = 0;
	cd->params.resize(2);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = -1.0;
	cd->params[1].type = ComponentDefinition::CDParamType::value;
	cd->params[1].value = -1.0;
}


//***********************************************************************
void setDissipator(std::unique_ptr<ComponentDefinition>& cd, uns componentModelIndex, rvt G, rvt Gsrc, uns nodeIndex, uns groundIndex, uns srcIndex) {
//***********************************************************************
	cd->setNodesDefValueIndex(groundIndex);
	cd->isBuiltIn = false;
	cd->componentModelIndex = componentModelIndex;
	cd->nodesConnectedTo.resize(3);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = nodeIndex;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = groundIndex;
	cd->nodesConnectedTo[2].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[2].index = srcIndex;
	cd->params.resize(2);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = G;
	cd->params[1].type = ComponentDefinition::CDParamType::value;
	cd->params[1].value = Gsrc;
}


//***********************************************************************
void setCell(std::unique_ptr<ComponentDefinition>& cd, uns componentModelIndex, uns nodeNum, uns index1, uns index2 = 0, uns index3 = 0, uns index4 = 0) {
//***********************************************************************
	cd->isBuiltIn = false;
	cd->componentModelIndex = componentModelIndex;
	cd->nodesConnectedTo.resize(nodeNum);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = index1;
	if (nodeNum > 1) {
		cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::internal;
		cd->nodesConnectedTo[1].index = index2;
	}
	if (nodeNum > 2) {
		cd->nodesConnectedTo[2].type = ComponentDefinition::CDNodeType::internal;
		cd->nodesConnectedTo[2].index = index3;
	}
	if (nodeNum > 3) {
		cd->nodesConnectedTo[3].type = ComponentDefinition::CDNodeType::internal;
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
void probaSzimulacio1() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();

	//***********************************************************************
	// Sunred fa építése
	//***********************************************************************

	hmgSunred::ReductionTreeInstructions instr;

	instr.data.resize(4);

	instr.data[0].resize(8);
	instr.data[0][0] = { 0, 0, 0, 1 };
	instr.data[0][1] = { 0, 2, 0, 3 };
	instr.data[0][2] = { 0, 4, 0, 5 };
	instr.data[0][3] = { 0, 6, 0, 7 };
	instr.data[0][4] = { 0, 8, 0, 9 };
	instr.data[0][5] = { 0, 10, 0, 11 };
	instr.data[0][6] = { 0, 12, 0, 13 };
	instr.data[0][7] = { 0, 14, 0, 15 };

	instr.data[1].resize(4);
	instr.data[1][0] = { 1, 0, 1, 2 };
	instr.data[1][1] = { 1, 1, 1, 3 };
	instr.data[1][2] = { 1, 4, 1, 6 };
	instr.data[1][3] = { 1, 5, 1, 7 };

	instr.data[2].resize(2);
	instr.data[2][0] = { 2, 0, 2, 1 };
	instr.data[2][1] = { 2, 2, 2, 3 };

	instr.data[3].resize(1);
	instr.data[3][0] = { 3, 0, 3, 1 };

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	constexpr rvt Cth = rvt(0.00642);
	constexpr rvt Gx = rvt(1.0 / 3750.0);
	constexpr rvt Gy = rvt(3.0 / 1250.0);
	constexpr rvt G0 = rvt(100'000.0);
	const rvt T0 = (25.0 - Rails::V[1]->getDefaultNodeValue());
	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	// 0

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 1

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0.0, 0.01, 0.01, 0.0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 2

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 3

	//mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 2, 2, false, 0, 0, SolutionType::stFullMatrix);
	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setV0(cd, T0, G0);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setGirator0(cd);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setR0(cd, 1.0 / G0, 1, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 1, 5, 5, 0, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR0(cd, G0, 0, 1);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 5 * G0, 5 * G0, 0, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 4

	mc = make_unique<ModelSubCircuit>(4, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 3, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 5

	//mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 24, 24, true, 0, 0, SolutionType::stFullMatrix);
	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 24, 24, true, 0, 0, SolutionType::stSunRed, &instr);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 0, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 0, 1, 4);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 1, 2, 5);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 2, 6);			mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 15, 3, 7);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 15, 16, 4, 8);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 16, 17, 5, 9);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 3, 3, 17, 6, 10);		mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 18, 7, 11);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 18, 19, 8, 12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 19, 20, 9, 13);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 20, 10, 14);		mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 1, 2, 21, 11);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 21, 22, 12);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 22, 23, 13);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 23, 14);			mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 6

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(5, 1);
	//ComponentBase::SimControl::setFinalDC();
	gc.fullCircuitInstances[0].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = false;
	std::cout << "runStepDC();" << std::endl;
	cout << "\n********* 1\n" << endl;
	//runStepDC();
	runStepLessPrintDC();

	//hmgSunred sunred;
	//sunred.buildTree(instr, gc.fullCircuitInstances[0].component.get());
	//sunred.allocDC();
	//sunred.forwsubsDC();
	//sunred.backsubsDC();

/*
	std::cout << "for (uns i = 0; i < 100; i++)" << std::endl;

	for (uns i = 0; i < 100; i++) {
//		std::cout << "******************************************************************" << std::endl;
		SimControl::stepTransient(10);
		runStepNoPrintDC();
//		std::cout << i << " \tt = " << ComponentBase::SimControl::timeStepStop.getValue() << " sec \t" << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValue() << std::endl;
		std::cout << SimControl::timeStepStop.getValueDC() << "    " << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValueDC() << std::endl;
	}
*/
	SimControl::setFinalDC();
	cout << "\n********* 2\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 3\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 4\n" << endl;

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()

	SimControl::setComplexFrequencyForAC(0.1);

	runStepLessPrintAC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[0].component->testPrint();
#endif

	SimControl::setComplexFrequencyForTimeConst(1e-2, 5);

	runStepLessPrintAC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[0].component->testPrint();
#endif

}


//***********************************************************************
void setRinternal(std::unique_ptr<ComponentDefinition>& cd, rvt G, uns internalIndex = 0, uns groundIndex = 1) {
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = internalIndex;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::ground;
	cd->nodesConnectedTo[1].index = groundIndex;
	cd->params.resize(1);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
	cd->params[0].value = G;
}


//***********************************************************************
void setRinternal2(std::unique_ptr<ComponentDefinition>& cd, rvt G, uns internalIndex1, uns internalIndex2) {
//***********************************************************************
	cd->isBuiltIn = true;
	cd->componentModelIndex = builtInModelType::bimtConstG_1;
	cd->nodesConnectedTo.resize(2);
	cd->nodesConnectedTo[0].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[0].index = internalIndex1;
	cd->nodesConnectedTo[1].type = ComponentDefinition::CDNodeType::internal;
	cd->nodesConnectedTo[1].index = internalIndex2;
	cd->params.resize(1);
	cd->params[0].type = ComponentDefinition::CDParamType::value;
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

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setRinternal(cd, G);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, I, I, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setV0(cd, 20, 100'000.0);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(0, 1);
	gc.fullCircuitInstances[0].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = true;

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

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(1, 0, 0, 0, 0, 2, 2, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, 0.1);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 600'000.0, 600'000.0);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setR0(cd, 100'000.0, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setGirator0(cd);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR0(cd, 1.0 / 100'000.0, 1, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 1, -6, -6, -6, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 1

	mc = make_unique<ModelSubCircuit>(1, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, 0.1);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setRinternal(cd, 0.1);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0, 6, 6, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 2

	//mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stSunRed, &instr);
	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 1, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 1, 1, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(2, 1);
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

	HgmCustomFunctionModel::ParameterIdentifier parId;
	HgmCustomFunctionModel::LineDescription lineDesc;

	// U*U

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futOpMul].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 2;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 2;
	lineDesc.parIndex.push_back(parId);
	funcModel.lines.push_back(lineDesc);

	// (U*U)*G

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futOpMul].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 3;
	lineDesc.parIndex.push_back(parId);
	funcModel.lines.push_back(lineDesc);

	NodeConnectionInstructions functionSources; // Model_Function_Controlled_I_with_const_G ctor moves it !
	NodeConnectionInstructions::Source src;
	src.sourceType = NodeConnectionInstructions::SourceType::sExternalNodeValue;
	src.sourceIndex = 2; // Usrc
	functionSources.sources.push_back(src);
	src.sourceType = NodeConnectionInstructions::SourceType::sParam;
	src.sourceIndex = 1; // Gsrc
	functionSources.sources.push_back(src);

	std::unique_ptr<Model_Function_Controlled_I_with_const_G> mf;

	// (uns nNormalINodes_, uns nControlINodes_, uns nParams_, NodeConnectionInstructions functionSources_, const HmgFunction& controlFunction_)

	mf = make_unique<Model_Function_Controlled_I_with_const_G>(1, 0, 2, functionSources, std::make_unique<HmgF_CustomFunction>(funcModel));
	cuns funcIIndex = (uns)gc.models.size();
	gc.models.push_back(std::move(mf)); // 0

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 2, 2, false, 0, 0, SolutionType::stFullMatrix);

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

	gc.createFullCircuit(cellIndex, 1);
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


//***********************************************************************
void probaSzimulacioMg1() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();

	//***********************************************************************
	// Sunred fa építése
	//***********************************************************************

	hmgSunred::ReductionTreeInstructions instr;

	instr.data.resize(3);

	instr.data[0].resize(3);
	instr.data[0][0] = { 0, 0, 0, 1 };
	instr.data[0][1] = { 0, 2, 0, 3 };
	instr.data[0][2] = { 0, 4, 0, 5 };

	instr.data[1].resize(1);
	instr.data[1][0] = { 1, 0, 1, 1 };

	instr.data[2].resize(1);
	instr.data[2][0] = { 2, 0, 1, 2 };

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	constexpr rvt Cth = rvt(0.00642);
	constexpr rvt Gx = rvt(1.0 / 3750.0);
	constexpr rvt Gy = rvt(3.0 / 1250.0);
	constexpr rvt G0 = rvt(100'000.0);
	const rvt T0 = (25.0 - Rails::V[1]->getDefaultNodeValue());
	constexpr rvt Gx22 = Gx;
	constexpr rvt Gy22 = Gy;
	constexpr rvt Cth22 = 4 * Cth;
	constexpr rvt Gx12 = Gx / 2;
	constexpr rvt Gy12 = Gy * 2;
	constexpr rvt Cth12 = 2 * Cth;
	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	// 0

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 1

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0.0, 0.01, 0.01, 0.0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 2

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 3

	//mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 2, 2, false, 0, 0, SolutionType::stFullMatrix);
	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setV0(cd, T0, G0);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setGirator0(cd);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setR0(cd, 1.0 / G0, 1, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 1, 5, 5, 0, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR0(cd, G0, 0, 1);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 5 * G0, 5 * G0, 0, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 4

	mc = make_unique<ModelSubCircuit>(4, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 3, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 5

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 24, 24, true, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 0, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 0, 1, 4);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 1, 2, 5);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 2, 6);			mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 15, 3, 7);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 15, 16, 4, 8);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 16, 17, 5, 9);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 3, 3, 17, 6, 10);		mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 18, 7, 11);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 18, 19, 8, 12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 19, 20, 9, 13);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 20, 10, 14);		mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 1, 2, 21, 11);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 21, 22, 12);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 22, 23, 13);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 23, 14);			mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 6

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 7

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth22);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 8

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0.0, 0.01, 0.01, 0.0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth22);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 9

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth12);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 10

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 6, 6, true, 0, 0, SolutionType::stFullMatrix);
	//mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 6, 6, true, 0, 0, SolutionType::stSunRed, &instr);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 7, 2, 0, 2);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 9, 3, 0, 1, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 1, 5);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 3, 3, 1, 5, 3);			mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 8, 2, 4, 2);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 7, 2, 4, 3);			mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));


	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(10, 1);
	gc.createFullCircuit(5, 1);
	//ComponentBase::SimControl::setFinalDC();
	gc.fullCircuitInstances[0].component->resetNodes(true);
	gc.fullCircuitInstances[1].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = false;
	std::cout << "runStepDC();" << std::endl;
	cout << "\n********* 1\n" << endl;
	//runStepDC();
	runStepLessPrintDC();

	//hmgSunred sunred;
	//sunred.buildTree(instr, gc.fullCircuitInstances[0].component.get());
	//sunred.allocDC();
	//sunred.forwsubsDC();
	//sunred.backsubsDC();

/*
	std::cout << "for (uns i = 0; i < 100; i++)" << std::endl;

	for (uns i = 0; i < 100; i++) {
//		std::cout << "******************************************************************" << std::endl;
		SimControl::stepTransient(10);
		runStepNoPrintDC();
//		std::cout << i << " \tt = " << ComponentBase::SimControl::timeStepStop.getValue() << " sec \t" << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValue() << std::endl;
		std::cout << SimControl::timeStepStop.getValueDC() << "    " << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValueDC() << std::endl;
	}
*/
	SimControl::setFinalDC();
	cout << "\n********* 2\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 3\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 4\n" << endl;

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()

	SimControl::setComplexFrequencyForAC(0.001);

	runStepLessPrintAC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[0].component->testPrint();
#endif

	SimControl::setComplexFrequencyForTimeConst(1e-2, 5);

	runStepLessPrintAC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[0].component->testPrint();
#endif

	// multigrid

	hmgMultigrid mg;

	FineCoarseConnectionDescription level1;
	NodeInstruction nodeInstruction;
	NodeInstruction::OneInstruction oneInstruction;

	level1.indexCoarseFullCircuit = 0;
	level1.indexFineFullCircuit = 1;

	// global node restrictions

	nodeInstruction.destNodeIndex = 0;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 16;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 1;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 17;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 2;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 8;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 3;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 9;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 10;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 4;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 19;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 22;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 5;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	// global node prolongations

	nodeInstruction.destNodeIndex = 0;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 1;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));


	nodeInstruction.destNodeIndex = 2;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 3;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 4;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 5;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 6;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 7;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 8;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 9;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 10;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 11;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 12;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 13;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 14;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 15;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 16;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 17;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 18;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 19;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 20;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 21;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 22;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 23;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	// localNodeRestrictionTypes

	LocalProlongationOrRestrictionInstructions cellInstructions;
	LocalProlongationOrRestrictionInstructions::LocalNodeInstruction destCellNode;
	LocalProlongationOrRestrictionInstructions::OneLocalInstruction srcCellNode;
	LocalProlongationOrRestrictionInstructions::RecursiveInstruction destDeepCellNode;
	LocalProlongationOrRestrictionInstructions::OneRecursiveInstruction srcDeepCellNode;

	// coarse = group 0, 4, 5
	
	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 2;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 3;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 1

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 2

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 3

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	//destDeepCellNode.destComponentIndex.clear();
	//destDeepCellNode.instr.clear();
	//destDeepCellNode.isExternal = false;	destDeepCellNode.destComponentIndex.push_back(0);	destDeepCellNode.destComponentIndex.push_back(3);	destDeepCellNode.nodeIndex = 0;
	//srcDeepCellNode.srcComponentIndex.clear();
	//srcDeepCellNode.isDestLevel = false;	srcDeepCellNode.isExternal = false;	srcDeepCellNode.srcComponentIndex.push_back(0); 
	//	srcDeepCellNode.srcComponentIndex.push_back(3); srcDeepCellNode.nodeIndex = 0;	srcDeepCellNode.weight = 1.0;	destDeepCellNode.instr.push_back(srcDeepCellNode);

	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));


	// localNodeProlongationTypes

	// fine 0, 1, 4, 5      group = 0

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	
	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 2;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 3;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.34;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 2, 6      group = 1

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 2;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 3      group = 2

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 7      group = 3

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	//destDeepCellNode.destComponentIndex.clear();
	//destDeepCellNode.instr.clear();
	//destDeepCellNode.isExternal = false;	destDeepCellNode.destComponentIndex.push_back(0);	destDeepCellNode.destComponentIndex.push_back(3);	destDeepCellNode.nodeIndex = 0;
	//srcDeepCellNode.srcComponentIndex.clear();
	//srcDeepCellNode.isDestLevel = false;	srcDeepCellNode.isExternal = false;	srcDeepCellNode.srcComponentIndex.push_back(0); 
	//	srcDeepCellNode.srcComponentIndex.push_back(3); srcDeepCellNode.nodeIndex = 0;	srcDeepCellNode.weight = 1.0;	destDeepCellNode.instr.push_back(srcDeepCellNode);

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 8, 9, 12, 13      group = 4

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.34;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 2;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 3;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 10, 11, 14, 15      group = 5

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.34;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 2;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 3;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// componentGroups

	ComponentGroup componentGroup;

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 0;
	componentGroup.localProlongationIndex = 0;
	componentGroup.fineCells.push_back(0);
	componentGroup.fineCells.push_back(1);
	componentGroup.fineCells.push_back(4);
	componentGroup.fineCells.push_back(5);
	componentGroup.coarseCells.push_back(0);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 1;
	componentGroup.localProlongationIndex = 1;
	componentGroup.fineCells.push_back(2);
	componentGroup.fineCells.push_back(6);
	componentGroup.coarseCells.push_back(1);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 2;
	componentGroup.localProlongationIndex = 2;
	componentGroup.fineCells.push_back(3);
	componentGroup.coarseCells.push_back(2);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 3;
	componentGroup.localProlongationIndex = 3;
	componentGroup.fineCells.push_back(7);
	componentGroup.coarseCells.push_back(3);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 0;
	componentGroup.localProlongationIndex = 4;
	componentGroup.fineCells.push_back(8);
	componentGroup.fineCells.push_back(9);
	componentGroup.fineCells.push_back(12);
	componentGroup.fineCells.push_back(13);
	componentGroup.coarseCells.push_back(4);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 0;
	componentGroup.localProlongationIndex = 5;
	componentGroup.fineCells.push_back(10);
	componentGroup.fineCells.push_back(11);
	componentGroup.fineCells.push_back(14);
	componentGroup.fineCells.push_back(15);
	componentGroup.coarseCells.push_back(5);
	level1.componentGroups.push_back(std::move(componentGroup));

	mg.levels.push_back(std::move(level1));

	mg.solveDC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[1].component->printNodeValueDC(0);
	std::cout << std::endl;
	gc.fullCircuitInstances[0].component->printNodeValueDC(0);
#endif
}


//***********************************************************************
void probaSzimulacioMg2() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();

	//***********************************************************************
	// Sunred fa építése
	//***********************************************************************

	hmgSunred::ReductionTreeInstructions instr;

	instr.data.resize(3);

	instr.data[0].resize(3);
	instr.data[0][0] = { 0, 0, 0, 1 };
	instr.data[0][1] = { 0, 2, 0, 3 };
	instr.data[0][2] = { 0, 4, 0, 5 };

	instr.data[1].resize(1);
	instr.data[1][0] = { 1, 0, 1, 1 };

	instr.data[2].resize(1);
	instr.data[2][0] = { 2, 0, 1, 2 };

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	constexpr rvt Cth = rvt(0.00642);
	constexpr rvt Gx = rvt(1.0 / 3750.0);
	constexpr rvt Gy = rvt(3.0 / 1250.0);
	constexpr rvt G0 = rvt(100'000.0);
	const rvt T0 = (25.0 - Rails::V[1]->getDefaultNodeValue());
	constexpr rvt Gx22 = Gx;
	constexpr rvt Gy22 = Gy;
	constexpr rvt Cth22 = 4 * Cth;
	constexpr rvt Gx12 = Gx * 2;
	constexpr rvt Gy12 = Gy / 2;
	constexpr rvt Cth12 = 2 * Cth;
	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	// 0

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 1

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0.0, 0.01, 0.01, 0.0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 2

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 3

	//mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 2, 2, false, 0, 0, SolutionType::stFullMatrix);
	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setV0(cd, T0, G0);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setGirator0(cd);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setR0(cd, 1.0 / G0, 1, 0);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));
	//cd = make_unique<ComponentDefinition>();	setIStep(cd, 1, 5, 5, 0, 0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR0(cd, G0, 0, 1);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 5 * G0, 5 * G0, 0, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 4

	mc = make_unique<ModelSubCircuit>(4, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 3, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 5

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 24, 24, true, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 0, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 0, 1, 4);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 1, 2, 5);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 2, 6);			mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 15, 3, 7);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 15, 16, 4, 8);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 16, 17, 5, 9);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 3, 3, 17, 6, 10);		mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 18, 7, 11);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 18, 19, 8, 12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 4, 19, 20, 9, 13);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 6, 3, 20, 10, 14);		mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 1, 2, 21, 11);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 21, 22, 12);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 22, 23, 13);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 23, 14);			mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 6

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//*********************************
	
	// 7

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth22);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 8

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0.0, 0.01, 0.01, 0.0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth22);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 9

	mc = make_unique<ModelSubCircuit>(4, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 3, Gy12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth12);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 10

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx22);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy12);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth22);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 11

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 8, 8, true, 0, 0, SolutionType::stFullMatrix);
	//mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 8, 8, true, 0, 0, SolutionType::stSunRed, &instr);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 7, 2, 0, 4);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 9, 4, 0, 1, 2, 5);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 2, 1, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 3, 3, 2, 3, 6);			mc->push_back_component(std::move(cd));

	cd = make_unique<ComponentDefinition>();	setCell(cd, 8, 2, 7, 4);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 10, 3, 7, 5, 6);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));


	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(11, 1);
	gc.createFullCircuit(5, 1);
	//ComponentBase::SimControl::setFinalDC();
	gc.fullCircuitInstances[0].component->resetNodes(true);
	gc.fullCircuitInstances[1].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = false;
	std::cout << "runStepDC();" << std::endl;
	cout << "\n********* 1\n" << endl;
	//runStepDC();
	runStepLessPrintDC();

	//hmgSunred sunred;
	//sunred.buildTree(instr, gc.fullCircuitInstances[0].component.get());
	//sunred.allocDC();
	//sunred.forwsubsDC();
	//sunred.backsubsDC();

/*
	std::cout << "for (uns i = 0; i < 100; i++)" << std::endl;

	for (uns i = 0; i < 100; i++) {
//		std::cout << "******************************************************************" << std::endl;
		SimControl::stepTransient(10);
		runStepNoPrintDC();
//		std::cout << i << " \tt = " << ComponentBase::SimControl::timeStepStop.getValue() << " sec \t" << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValue() << std::endl;
		std::cout << SimControl::timeStepStop.getValueDC() << "    " << gc.fullCircuitInstances[0].component->getStoredComponent(12)->getInternalNode(0)->getValueDC() << std::endl;
	}
*/
	SimControl::setFinalDC();
	cout << "\n********* 2\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 3\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 4\n" << endl;

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()

	SimControl::setComplexFrequencyForAC(0.001);

	runStepLessPrintAC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[0].component->testPrint();
#endif

	SimControl::setComplexFrequencyForTimeConst(1e-2, 5);

	runStepLessPrintAC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[0].component->testPrint();
#endif

	// multigrid

	hmgMultigrid mg;

	FineCoarseConnectionDescription level1;
	NodeInstruction nodeInstruction;
	NodeInstruction::OneInstruction oneInstruction;

	level1.indexCoarseFullCircuit = 0;
	level1.indexFineFullCircuit = 1;

	// global node restrictions
/*
	nodeInstruction.destNodeIndex = 0;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 16;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 1;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 2;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 17;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 3;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 4;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 8;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 5;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 9;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 6;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 10;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 7;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 19;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 22;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));
*/

	constexpr rvt div2 = 1.0 / 2.0;
	constexpr rvt div3 = 1.0 / 3.0;
	constexpr rvt div4 = 1.0 / 4.0;
	constexpr rvt div6 = 1.0 / 6.0;
	constexpr rvt div8 = 1.0 / 8.0;
	constexpr rvt div12 = 1.0 / 12.0;
	constexpr rvt m5p8 = 5.0 / 8.0;

	nodeInstruction.destNodeIndex = 0;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 16;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 1;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 2;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 5;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 6;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 1;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.4;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 2;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.2;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 3;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.2;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 2;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 17;	oneInstruction.weight = div3;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 9;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 10;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 6;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 7;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 3;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = 0.4;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 17;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 3;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.2;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 7;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.2;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 4;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 8;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 15;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 18;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 4;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 5;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 8;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 9;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 5;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 9;	oneInstruction.weight = div3;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 16;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 17;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 19;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 20;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 6;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 10;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 6;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 10;	oneInstruction.weight = 0.4;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 17;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 20;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 7;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.2;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 11;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.2;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 7;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 12;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 13;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 19;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 22;	oneInstruction.weight = div6;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 9;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 10;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 13;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 14;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = div12;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeRestrictions.push_back(std::move(nodeInstruction));

	// global node prolongations

	nodeInstruction.destNodeIndex = 0;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 0;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 1;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 2;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 1;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 3;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 0;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 4;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 0;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 5;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 1;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 6;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 3;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 7;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 8;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.667;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = 0.333;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 9;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 10;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 11;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 4;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 12;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 4;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 13;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 5;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 14;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 5;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 15;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 0;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 16;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 17;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 2;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 18;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 4;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 4;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.5;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 19;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.25;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 0.75;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 20;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 5;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 6;	oneInstruction.weight = 0.1;	nodeInstruction.instr.push_back(oneInstruction);
	oneInstruction.srcComponentIndex = 5;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 0.8;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 21;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 4;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 22;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = unsMax;	oneInstruction.srcNodeIndex = 7;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	nodeInstruction.destNodeIndex = 23;
	nodeInstruction.instr.clear();
	oneInstruction.srcComponentIndex = 5;		oneInstruction.srcNodeIndex = 0;	oneInstruction.weight = 1.0;	nodeInstruction.instr.push_back(oneInstruction);
	level1.globalNodeProlongations.push_back(std::move(nodeInstruction));

	// localNodeRestrictionTypes

	LocalProlongationOrRestrictionInstructions cellInstructions;
	LocalProlongationOrRestrictionInstructions::LocalNodeInstruction destCellNode;
	LocalProlongationOrRestrictionInstructions::OneLocalInstruction srcCellNode;
	LocalProlongationOrRestrictionInstructions::RecursiveInstruction destDeepCellNode;
	LocalProlongationOrRestrictionInstructions::OneRecursiveInstruction srcDeepCellNode;

/*
	// coarse = group 0, 4, 5
	
	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 2;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 3;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 1

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 2

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 3

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	//destDeepCellNode.destComponentIndex.clear();
	//destDeepCellNode.instr.clear();
	//destDeepCellNode.isExternal = false;	destDeepCellNode.destComponentIndex.push_back(0);	destDeepCellNode.destComponentIndex.push_back(3);	destDeepCellNode.nodeIndex = 0;
	//srcDeepCellNode.srcComponentIndex.clear();
	//srcDeepCellNode.isDestLevel = false;	srcDeepCellNode.isExternal = false;	srcDeepCellNode.srcComponentIndex.push_back(0); 
	//	srcDeepCellNode.srcComponentIndex.push_back(3); srcDeepCellNode.nodeIndex = 0;	srcDeepCellNode.weight = 1.0;	destDeepCellNode.instr.push_back(srcDeepCellNode);

	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));
*/

// coarse = group 0, 4, 5

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div12; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div12; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 2;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div12; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 3;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div12; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div6; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = div6; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 3;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div6; destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 3;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = div6; destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));


	// coarse = group 1

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div6;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div6;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div12;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = div12;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 2;	srcCellNode.weight = div3;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = div12;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 1;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = div12;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 2

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.25;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.25;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// coarse = group 3

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.4;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.2;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.2;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 2;	srcCellNode.weight = 0.2;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	//destDeepCellNode.destComponentIndex.clear();
	//destDeepCellNode.instr.clear();
	//destDeepCellNode.isExternal = false;	destDeepCellNode.destComponentIndex.push_back(0);	destDeepCellNode.destComponentIndex.push_back(3);	destDeepCellNode.nodeIndex = 0;
	//srcDeepCellNode.srcComponentIndex.clear();
	//srcDeepCellNode.isDestLevel = false;	srcDeepCellNode.isExternal = false;	srcDeepCellNode.srcComponentIndex.push_back(0); 
	//	srcDeepCellNode.srcComponentIndex.push_back(3); srcDeepCellNode.nodeIndex = 0;	srcDeepCellNode.weight = 1.0;	destDeepCellNode.instr.push_back(srcDeepCellNode);

	mg.localNodeRestrictionTypes.push_back(std::move(cellInstructions));

	// localNodeProlongationTypes

	// fine 0, 1, 4, 5      group = 0

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();
	
	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));
	
	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 2;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 3;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.34;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 2, 6      group = 1

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 3;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 3      group = 2

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 7      group = 3

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	//destDeepCellNode.destComponentIndex.clear();
	//destDeepCellNode.instr.clear();
	//destDeepCellNode.isExternal = false;	destDeepCellNode.destComponentIndex.push_back(0);	destDeepCellNode.destComponentIndex.push_back(3);	destDeepCellNode.nodeIndex = 0;
	//srcDeepCellNode.srcComponentIndex.clear();
	//srcDeepCellNode.isDestLevel = false;	srcDeepCellNode.isExternal = false;	srcDeepCellNode.srcComponentIndex.push_back(0); 
	//	srcDeepCellNode.srcComponentIndex.push_back(3); srcDeepCellNode.nodeIndex = 0;	srcDeepCellNode.weight = 1.0;	destDeepCellNode.instr.push_back(srcDeepCellNode);

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 8, 9, 12, 13      group = 4

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.34;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.33;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 2;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 3;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// fine 10, 11, 14, 15      group = 5

	cellInstructions.destComponentsNodes.clear();
	cellInstructions.deepDestComponentNodes.clear();

	destCellNode.isExternal = false;	destCellNode.destIndex = 0;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.3;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.3;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 1;	srcCellNode.weight = 0.4;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 1;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.4;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 2;	srcCellNode.weight = 0.6;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 2;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = true;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 0.5;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	destCellNode.isExternal = false;	destCellNode.destIndex = 3;	destCellNode.nodeIndex = 0;
	destCellNode.instr.clear();
	srcCellNode.isDestLevel = false;	srcCellNode.isExternal = false;	srcCellNode.srcIndex = 0;	srcCellNode.nodeIndex = 0;	srcCellNode.weight = 1.0;	destCellNode.instr.push_back(srcCellNode);
	cellInstructions.destComponentsNodes.push_back(std::move(destCellNode));

	mg.localNodeProlongationTypes.push_back(std::move(cellInstructions));

	// componentGroups

	ComponentGroup componentGroup;

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 0;
	componentGroup.localProlongationIndex = 0;
	componentGroup.fineCells.push_back(0);
	componentGroup.fineCells.push_back(1);
	componentGroup.fineCells.push_back(4);
	componentGroup.fineCells.push_back(5);
	componentGroup.coarseCells.push_back(0);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 1;
	componentGroup.localProlongationIndex = 1;
	componentGroup.fineCells.push_back(2);
	componentGroup.fineCells.push_back(6);
	componentGroup.coarseCells.push_back(1);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 2;
	componentGroup.localProlongationIndex = 2;
	componentGroup.fineCells.push_back(3);
	componentGroup.coarseCells.push_back(2);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 3;
	componentGroup.localProlongationIndex = 3;
	componentGroup.fineCells.push_back(7);
	componentGroup.coarseCells.push_back(3);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 0;
	componentGroup.localProlongationIndex = 4;
	componentGroup.fineCells.push_back(8);
	componentGroup.fineCells.push_back(9);
	componentGroup.fineCells.push_back(12);
	componentGroup.fineCells.push_back(13);
	componentGroup.coarseCells.push_back(4);
	level1.componentGroups.push_back(std::move(componentGroup));

	componentGroup.fineCells.clear();
	componentGroup.coarseCells.clear();
	componentGroup.isCopy = false;
	componentGroup.localRestrictionIndex = 0;
	componentGroup.localProlongationIndex = 5;
	componentGroup.fineCells.push_back(10);
	componentGroup.fineCells.push_back(11);
	componentGroup.fineCells.push_back(14);
	componentGroup.fineCells.push_back(15);
	componentGroup.coarseCells.push_back(5);
	level1.componentGroups.push_back(std::move(componentGroup));

	mg.levels.push_back(std::move(level1));

	mg.solveDC();

#ifdef HMG_DEBUGPRINT
	gc.fullCircuitInstances[1].component->printNodeValueDC(0);
	std::cout << std::endl;
	gc.fullCircuitInstances[0].component->printNodeValueDC(0);
#endif
}


//***********************************************************************
void probaSzimulacioMg3() {
//***********************************************************************
	CircuitStorage& gc = CircuitStorage::getInstance();

	//***********************************************************************
	// Sunred fa építése
	//***********************************************************************

	hmgSunred::ReductionTreeInstructions instr;

	instr.data.resize(3);

	instr.data[0].resize(2);
	instr.data[0][0] = { 0, 1, 0, 2 };
	instr.data[0][1] = { 0, 0, 0, 4 };

	instr.data[1].resize(1);
	instr.data[1][0] = { 1, 0, 0, 3 };

	instr.data[2].resize(1);
	instr.data[2][0] = { 1, 1, 2, 0 };

	//***********************************************************************
	// cellamodellek
	//***********************************************************************

	constexpr rvt Cth = rvt(1.0);
	constexpr rvt Gx = rvt(1.0);
	constexpr rvt Gy = rvt(0.5);
	constexpr rvt G0 = rvt(100'000.0);
	const rvt T0 = (25.0 - Rails::V[1]->getDefaultNodeValue());
	std::unique_ptr<ModelSubCircuit> mc;
	std::unique_ptr<ComponentDefinition> cd;

	//uns nIONodes_, uns nNormalINodes_, uns nControlINodes_, uns nNormalONodes_, uns nForwardedONodes_, uns nInternalNodes_, uns nNormalInternalNodes_, bool defaultInternalNodeIsConcurrent_, uns nInternalVars_, uns nParams_, SolutionType solutionType_

	// 0

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 0.0, 100.0, 1000.0 / hmgPi, 0.0);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, 2*Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 1

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 2

	mc = make_unique<ModelSubCircuit>(3, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 2, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR0(cd, G0, 0, 1);		mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setIStep(cd, 0, 5 * G0, 5 * G0, 0, 0);	mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 3

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 4

	mc = make_unique<ModelSubCircuit>(2, 0, 0, 0, 0, 1, 1, false, 0, 0, SolutionType::stFullMatrix);

	cd = make_unique<ComponentDefinition>();	setR(cd, 0, Gy);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setR(cd, 1, Gx);	mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setC0(cd, Cth);		mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	// 5

	mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 5, 5, true, 0, 0, SolutionType::stFullMatrix);
	//mc = make_unique<ModelSubCircuit>(0, 0, 0, 0, 0, 5, 5, true, 0, 0, SolutionType::stSunRed, &instr);

	cd = make_unique<ComponentDefinition>();	setCell(cd, 0, 3, 0, 3, 4);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 1, 2, 0, 1);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 2, 3, 0, 1, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 3, 2, 2, 3);			mc->push_back_component(std::move(cd));
	cd = make_unique<ComponentDefinition>();	setCell(cd, 4, 2, 4, 2);			mc->push_back_component(std::move(cd));

	gc.models.push_back(std::move(mc));

	//***********************************************************************
	// példányosítás
	//***********************************************************************

	gc.createFullCircuit(5, 1);
	gc.fullCircuitInstances[0].component->resetNodes(true);

	ComponentConstC_1::isTrapezoid = false;
	std::cout << "runStepDC();" << std::endl;
	cout << "\n********* 1\n" << endl;
	runStepLessPrintDC();

	SimControl::setFinalDC();
	cout << "\n********* 2\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 3\n" << endl;
	runStepLessPrintDC();
	cout << "\n********* 4\n" << endl;

	gc.fullCircuitInstances[0].component->buildForAC(); // DC runs in gc.createFullCircuit() => buildOrReplace()

	SimControl::setComplexFrequencyForAC(1.0);

	runStepLessPrintAC();
}



// Be kell állítani a CircuitNodeData::defaultNodeValue vektort (méret és értékek) az áramkördefiníciós adatok alapján, utána lehet létrehozni az áramkört


//***********************************************************************
int main(int n, const char** params) {
//***********************************************************************
	try {
		HmgFileReader reader;

		//most("start");
		//reader.ReadFile(params[1]);
		reader.ReadFile("c:/!D/Kutatás/cikkek/tanszeki/HexMG/proba.hmg");
		InstructionStream is;
		reader.convertToInstructionStream(is);
		//most("convertToInstructionStream");
		reader.clear();
		//most("clear");

		//InstructionProcessor ip;
		//ip.addInstructionStream(is);
		//InstructionProcessor::waitToFinish(ip);
		//most("process instructions");
		//esemenyek_kiirasa();
	}
	catch (const hmgExcept& err) {
		std::cerr << err.what() << std::endl;
	}

	return 0;
}

//***********************************************************************
int main_1() {
//***********************************************************************

	//SpiceExpression se;
	//se.buildFromString("PUKI(T)+.SQRT(_Q)+.INV(.RATIO(-.PWL(T,25,-20,58+3,-54,25,65K),65K,H))*MUL + V(2) + PUKI() + V(@3,4) - 2 * _PI * I(88) * T");


	Rails::resize(2);
	// FixVoltages::V[0]->defaultNodeValue is mandatory 0 !!!
	Rails::SetVoltage(1, 20.0);
	Rails::reset();

	CircuitStorage& gc = CircuitStorage::getInstance();

	HmgF_opMul fsqr;
	rvt wf[3] = { 0.0, 1.0, 3.5 };
	uns par[4] = { 0, 3, 1, 2 };
	fsqr.evaluate(par, wf, nullptr);
	std::cout << "\n" << fsqr.devive(par, wf, nullptr, 2) << std::endl;
	std::cout << wf[0] << "\n" << std::endl;

	HgmCustomFunctionModel fvModel;
	fvModel.nParams = 4;
	fvModel.nLocal = 3;

	HgmCustomFunctionModel::ParameterIdentifier parId;
	HgmCustomFunctionModel::LineDescription lineDesc;

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futOpMul].get();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 2;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 3;
	lineDesc.parIndex.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futSqrt].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 1;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 4;
	lineDesc.parIndex.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futOpPlus].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 2;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 1;
	lineDesc.parIndex.push_back(parId);
	fvModel.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futOpMul].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 2;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 5;
	lineDesc.parIndex.push_back(parId);
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

	fuggveny.evaluate(&indexField[0], &workField[0], nullptr);

	std::cout << "\n" << fuggveny.devive(&indexField[0], &workField[0], nullptr, 2) << std::endl;
	std::cout << workField[0] << "\n" << std::endl;

	HgmFunctionStorage::namelessCustomFunctions.push_back(std::make_unique<HmgF_CustomFunction>(fvModel));

	HgmCustomFunctionModel fvModel2;
	fvModel2.nParams = 5;
	fvModel2.nLocal = 2;

	lineDesc.pFunction = HgmFunctionStorage::namelessCustomFunctions[0].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 2;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 3;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 4;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 5;
	lineDesc.parIndex.push_back(parId);
	fvModel2.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futOpMul].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 1;
	lineDesc.parIndex.push_back(parId);
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 6;
	lineDesc.parIndex.push_back(parId);
	fvModel2.lines.push_back(lineDesc);

	lineDesc.pFunction = HgmFunctionStorage::builtInFunctions[futSqrt].get();
	lineDesc.parIndex.clear();
	parId.parType = HgmCustomFunctionModel::ParameterType::ptParam;
	parId.parIndex = 0;
	lineDesc.parIndex.push_back(parId);
	parId.parType = HgmCustomFunctionModel::ParameterType::ptLocalVar;
	parId.parIndex = 1;
	lineDesc.parIndex.push_back(parId);
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

	fuggveny2.evaluate(&indexField[0], &workField[0], nullptr);

	std::cout << "\nvvvvvvvvv\n\n" << fuggveny2.devive(&indexField[0], &workField[0], nullptr, 2) << std::endl;
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
	probaSzimulacioMg2();
	const auto endTime_proba = high_resolution_clock::now();

	duration<double> time_span = duration_cast<duration<double>>(endTime_proba - startTime_proba);

	cout << "proba time: " << time_span.count() << " seconds\n" << endl;

	cout << sizeof(NodeData) << '\n' << sizeof(CircuitNodeDataAC) << endl;
	cout << sizeof(ComponentConstR_1) << '\n' << sizeof(ComponentSubCircuit) << endl;
	cout << noexcept(is_true_error(false, "const char* who_threw", "const char* what")) << endl;
	cout << sizeof(matrix<cplx>) << endl;


	return 0;
}

