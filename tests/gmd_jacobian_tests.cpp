#include "hmgHMGFileReader.h"
#include "hmgInstructionQueue.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

// Compare the assembled Jacobian with the derivative of the actual stamped
// physical currents, including BOTH dissipation outputs and control ports.
int main(int argc,char** argv) {
    using namespace nsHMG;
    try {
        Rails::resize(1);Rails::reset();
        if(argc!=2) throw std::runtime_error("Expected fresh fixture directory");
        const auto root=std::filesystem::absolute(argv[1]);
        if(std::filesystem::exists(root)||!std::filesystem::create_directories(root))
            throw std::runtime_error("Fresh directory required");
        const auto path=root/"input.hmg";
        std::ofstream(path)<<R"(HEXMG 1.0
.FUNCTION LAW P=4 V=3
_SUB V0 P0 P1
_MULC V1 P2 .01
_MULC V2 P3 .02
_ADD V1 V1 V2
_ADDC V1 V1 2
_MUL V1 V1 V0
_POWC V2 V0 3
_MULC V2 V2 .3
_ADD RET V1 V2
_MULC RET RET -1
.END FUNCTION LAW
.MODEL M SUBCIRCUIT N=6
D FCIMD F=LAW(X0 X1 X3 X4) N0 N1 N2 N3 N4 .3 .1
Y FCIMD Y=1 F=LAW(X0 X1 Y0 X4) N0 N1 N2 N3 N4 N5 .7 .1
G GMD N0 N1 N2 N3 N4 .3 2
D4 FCID F=LAW(X0 X1 X2 X3) N0 N1 N2 N3 .3 .1
Y4 FCID Y=1 F=LAW(X0 X1 Y0 X3) N0 N1 N2 N3 N4 .7 .1
G4 GD N0 N1 N2 N3 .3 2
.END MODEL M
.CREATE C M GND=R0
.END
)";
        HgmFunctionStorage::getInstance();HmgFileReader reader;
        reader.ReadFile(path.generic_string());InstructionStream stream;
        reader.convertToInstructionStream(stream);reader.clear();
        InstructionQueue queue;queue.addInstructionStream(stream);InstructionQueue::waitToFinish(queue);
        auto& circuit=*CircuitStorage::getInstance().fullCircuitInstances[0].component;
        unsigned checks=0,failures=0;
        for(unsigned item=0;item<6;++item) {
            auto* part=const_cast<ComponentBase*>(circuit.getContainedComponent(item));
            const unsigned rows=item>=3?4u:5u,columns=rows+(item==1||item==4?1u:0u);
            for(const double voltage:{-.7,.2,1.1})for(const double share:{0.,.3,1.}) {
                part->setParam(0,Param{nullptr,share});
                const std::array<double,6> state=item>=3?std::array<double,6>{voltage,.1,35,40,30,0}:std::array<double,6>{voltage,.1,.3,35,40,30};
                for(unsigned x=0;x<columns;++x)part->getNode(x)->setValueDC(state[x]);
                const auto stamp=[&] {
                    for(unsigned y=0;y<rows;++y)part->getNode(y)->deleteDDC();
                    part->calculateValueDC();part->calculateCurrent(true);
                    std::array<double,5> residual{};
                    for(unsigned y=0;y<rows;++y)residual[y]=part->getNode(y)->getDDC();
                    return residual;
                };
                for(unsigned x=0;x<columns;++x) {
                    const double h=1e-5*std::max(1.,std::abs(state[x]));
                    part->getNode(x)->setValueDC(state[x]+h);const auto plus=stamp();
                    part->getNode(x)->setValueDC(state[x]-h);const auto minus=stamp();
                    part->getNode(x)->setValueDC(state[x]);(void)stamp();
                    for(unsigned y=0;y<rows;++y) {
                        const double expected=-(plus[y]-minus[y])/(2*h),actual=part->getYDC(y,x);++checks;
                        if(!std::isfinite(actual)||std::abs(actual-expected)>2e-6*std::max(1.,std::abs(expected))) {
                            if(failures<15)std::cerr<<"component "<<item<<" share "<<share<<" Y["<<y<<','<<x<<"] "<<actual<<" expected "<<expected<<'\n';
                            ++failures;
                        }
                        SimControl::lagDissipationJacobian=true;
                        const auto lagged=part->getYDC(y,x);const auto physical=stamp();
                        SimControl::lagDissipationJacobian=false;
                        const double lagExpected=y>=rows-2?0:expected;++checks;
                        if(std::abs(lagged-lagExpected)>2e-6*std::max(1.,std::abs(lagExpected)))++failures;
                        const auto exact=stamp();++checks;if(physical!=exact)++failures;
                    }
                }
            }
        }
        std::cout<<checks<<" Jacobian checks; "<<failures<<" failures\n";
        return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
