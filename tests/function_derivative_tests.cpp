#include "hmgHMGFileReader.h"
#include "hmgInstructionQueue.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc,char** argv) {
    using namespace nsHMG;
    try {
        Rails::resize(1);Rails::reset();
        if(argc!=2)throw std::runtime_error("Fresh fixture directory required");
        const auto root=std::filesystem::absolute(argv[1]);
        if(std::filesystem::exists(root)||!std::filesystem::create_directories(root))throw std::runtime_error("Fresh directory required");
        const auto path=root/"input.hmg";
        std::ofstream(path)<<R"(HEXMG 1.0
.OPTIONAL HEXMG_OPT 1
FUNCTION_DERIVATIVE LAW P0 DERIVATIVE
FUNCTION_DERIVATIVE EXPONENTIAL P0 EXPONENTIAL
.END OPTIONAL
.FUNCTION LAW P=2 V=1
_POWC V0 P0 2
_ADDC V0 V0 7
_ADD RET V0 P1
.END FUNCTION LAW
.FUNCTION DOUBLE P=1 V=0
_MULC RET P0 2
.END FUNCTION DOUBLE
.FUNCTION DERIVATIVE P=2 V=2
DOUBLE V0 P0
_ADDC V1 P1 0
_ADD RET V0 0
.END FUNCTION DERIVATIVE
.FUNCTION EXPONENTIAL P=1 V=0
_EXP RET P0
.END FUNCTION EXPONENTIAL
.MODEL M SUBCIRCUIT N=1
G G N0 R0 1
F FCI Y=1 F=LAW(X0 Y0) N0 R0 R0 0
E FCI F=EXPONENTIAL(X0) N0 R0 0
.END MODEL M
.CREATE C M GND=R0
.END
)";
        optimization::enabled=true;HgmFunctionStorage::getInstance();HmgFileReader reader;
        reader.ReadFile(path.generic_string());InstructionStream stream;reader.convertToInstructionStream(stream);reader.clear();
        InstructionQueue queue;queue.addInstructionStream(stream);InstructionQueue::waitToFinish(queue);
        unsigned checks=0;
        const auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Derivative or state preservation failed at check "+std::to_string(checks));};
        auto& models=CircuitStorage::getInstance().functionControlledBuiltInModels;
        for(const unsigned id:{0u,1u}) {
            const auto& function=*static_cast<Model_Function_Controlled_I_with_const_G&>(*models[id]).controlFunction;const unsigned params=function.getN_Param();
            std::vector<uns> index(function.getN_IndexField());index[0]=0;index[1]=1+params;
            for(unsigned p=0;p<params;++p)index[2+p]=1+p;
            function.fillIndexField(index.data());
            std::vector<rvt> work(1+params+function.getN_WorkingField());
            for(const double value:{-10.,-1.,-1e-20,0.,1e-20,.3,1.,10.}) {
                work[1]=value;if(params==2)work[2]=.7;
                function.evaluate(index.data(),work.data(),nullptr,LineDescription(),nullptr);
                const auto before=work;
                const double expected=id==0?2*value:std::exp(value);
                const double actual=function.devive(index.data(),work.data(),nullptr,2,LineDescription(),nullptr);
                if(std::abs(actual-expected)>1e-14*std::max(1e-30,std::abs(expected))||!std::isfinite(actual))
                    std::cerr<<"id="<<id<<" x="<<value<<" actual="<<actual<<" expected="<<expected<<'\n';
                check(std::abs(actual-expected)<=1e-14*std::max(1e-30,std::abs(expected)));
                check(work==before);
                // The unchanged one-sided fallback is only accurate to its
                // subtraction roundoff, unlike the exact tiny derivative.
                if(params==2) {
                    const double numeric=function.devive(index.data(),work.data(),nullptr,3,LineDescription(),nullptr);
                    const double roundoff=4*std::numeric_limits<rvt>::epsilon()*std::max(1.,std::abs(double(before[0])))/8e-10;
                    check(std::abs(numeric-1)<1e-8+roundoff);
                }
                // Repeated calls must reuse scratch without retaining old arguments.
                for(unsigned n=0;n<32;++n)check(function.devive(index.data(),work.data(),nullptr,2,LineDescription(),nullptr)==actual);
            }
        }
        std::cout<<checks<<" derivative checks PASS\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
