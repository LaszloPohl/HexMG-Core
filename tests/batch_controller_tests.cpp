#include "hmgHMGFileReader.h"
#include "hmgInstructionQueue.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace nsHMG;using namespace hexmg::optics;
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("Fresh directory required");const auto root=std::filesystem::absolute(argv[1]);
    if(std::filesystem::exists(root)||!std::filesystem::create_directories(root))throw std::runtime_error("Fresh directory required");
    File file;file.program.cells=1;file.program.steps={{Op::Branch,0,0},{Op::Segment,0,.5},{Op::Escape,1,0}};file.program.rays={{0,3,0,1,0,1}};
    file.bindings={{"B0",0},{{},0},{"N0",0},{"B1",0},{"B2",0}};file.cells.resize(1);file.cells[0].coefficients={0,1,1,1};file.cells[0].outputs[0]=3;file.blue={2};file.exits[2]=4;
    if(opticalAttenuationCacheBudget(file)!=0)throw std::runtime_error("Dynamic absorption should not allocate attenuation cache");
    const auto bytes=encode(file);std::ofstream binary(root/"paths.hxray",std::ios::binary);binary.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));binary.close();
    std::ofstream text(root/"input.hmg");text<<"HEXMG 1.0\n.FUNCTION CONST P=0 V=0\n_COPY RET 2\n.END FUNCTION CONST\n.FUNCTION COPY P=1 V=0\n_ADDC RET P0 0\n.END FUNCTION COPY\n"
        ".MODEL MAT CONTROLLER A=1\nFUNCTION CONST\nLOAD -\nSTORE A0\n.END MODEL MAT\n.MODEL READ CONTROLLER A=2\nFUNCTION COPY\nLOAD - A0\nSTORE A1\n.END MODEL READ\n"
        ".MODEL M SUBCIRCUIT N=1 B=5\nG G N0 R0 1\nC0 MODEL MAT CTRL_LEVEL=0 B0\nC1 MODEL READ CTRL_LEVEL=1 B1 B3\nC2 MODEL READ CTRL_LEVEL=2 B1 B4\n.END MODEL M\n.CREATE C M GND=R0\n"
        ".BATCHCONTROLLER LIGHT C TYPE=OPTICAL_RAYS CTRL_LEVEL=1 FILE=\"paths.hxray\"\n.END\n";text.close();
    Rails::resize(1);Rails::reset();optimization::enabled=true;HgmFunctionStorage::getInstance();HmgFileReader reader;reader.ReadFile((root/"input.hmg").generic_string());
    InstructionStream stream;reader.convertToInstructionStream(stream);reader.clear();InstructionQueue queue;queue.addInstructionStream(stream);InstructionQueue::waitToFinish(queue);
    const auto node=[](NodeVarType type,uns index)->NodeVariable&{DeepInterfaceNodeID id{};id.nodeID.type=type;id.nodeID.index=index;return const_cast<NodeVariable&>(CircuitStorage::limitNode(0,id));};
    unsigned checks=0;const auto near=[&](double a,double b){++checks;if(!std::isfinite(a)||std::abs(a-b)>1e-12*std::max(1.,std::abs(b)))throw std::runtime_error("Batch controller schedule or value mismatch");};
    double old=0;for(double power:{2.,5.,0.,-1.}){node(nvtN,0).setValueDC(power);CircuitStorage::CalculateControllersDC(0);const double heat=power*(1-std::exp(-1.));
        near(node(nvtB,0).getValueDC(),2);near(node(nvtB,1).getValueDC(),heat);near(node(nvtB,2).getValueDC(),power-heat);
        near(node(nvtB,3).getValueDC(),old);near(node(nvtB,4).getValueDC(),heat);old=heat;}
    std::cout<<"PASS "<<checks<<" batch-controller load/store checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
