#include "hmgHMGFileReader.h"
#include "hmgInstructionQueue.h"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){using namespace nsHMG;try{
    if(argc!=2)throw std::runtime_error("Fresh directory required");const auto root=std::filesystem::absolute(argv[1]);
    if(std::filesystem::exists(root)||!std::filesystem::create_directories(root))throw std::runtime_error("Fresh directory required");
    std::ofstream file(root/"input.hmg");file<<"HEXMG 1.0\n.FUNCTION LEAF P=1 V=3\n_ADDC V0 P0 0\n_MUL V1 V0 V0\n_ADD V2 V1 P0\n_ADDC RET V2 0\n.END FUNCTION LEAF\n"
        ".FUNCTION PARENT P=1 V=2\n_COPY V0 0\n";
    for(unsigned i=0;i<10;++i)file<<"LEAF V1 P0\n_ADD V0 V0 V1\n";
    file<<"_ADDC RET V0 0\n.END FUNCTION PARENT\n.FUNCTION OUTER P=1 V=4\n";
    for(unsigned i=0;i<3;++i)file<<"PARENT V"<<i<<" P0\n";
    file<<"_ADD V3 V0 V1\n_ADD RET V3 V2\n.END FUNCTION OUTER\n.MODEL M SUBCIRCUIT N=1\nG G N0 R0 1\nF FCI F=OUTER(X0) N0 R0 0\n.END MODEL M\n.CREATE C M GND=R0\n.END\n";file.close();
    Rails::resize(1);Rails::reset();optimization::enabled=true;HgmFunctionStorage::getInstance();HmgFileReader reader;reader.ReadFile((root/"input.hmg").generic_string());
    InstructionStream stream;reader.convertToInstructionStream(stream);reader.clear();InstructionQueue queue;queue.addInstructionStream(stream);InstructionQueue::waitToFinish(queue);
    auto& models=CircuitStorage::getInstance().functionControlledBuiltInModels;
    const auto& f=*static_cast<Model_Function_Controlled_I_with_const_G&>(*models.at(0)).controlFunction;
    std::cout<<"workspace="<<f.getN_WorkingField()<<" expected=9\n";
    std::vector<uns> indices(f.getN_IndexField());indices[0]=0;indices[1]=2;indices[2]=1;f.fillIndexField(indices.data());
    std::vector<rvt> work(2+f.getN_WorkingField()+16,1234567);unsigned checks=0;
    for(unsigned n=0;n<100;++n){const double x=static_cast<double>(n)/17-3;work[1]=x;
        f.evaluate(indices.data(),work.data(),nullptr,LineDescription(),nullptr);const double expected=30*(x*x+x);
        if(std::abs(work[0]-expected)>1e-12*std::max(1.,std::abs(expected)))throw std::runtime_error("Nested function value");++checks;
        for(std::size_t i=2+f.getN_WorkingField();i<work.size();++i){if(work[i]!=1234567)throw std::runtime_error("Workspace canary overwritten");++checks;}}
    if(f.getN_WorkingField()!=9)throw std::runtime_error("Sequential children allocate sum instead of reusable maximum scratch");
    std::cout<<"PASS "<<checks<<" nested workspace checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
