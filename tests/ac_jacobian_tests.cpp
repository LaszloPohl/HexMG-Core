#include "hmgHMGFileReader.h"
#include "hmgInstructionQueue.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

// Independent directional derivative of the physical DC residual. AC must be
// this linear map, including controls, current monitors and both heat ports.
int main(int argc,char** argv) {
    using namespace nsHMG;
    try {
        Rails::resize(1);Rails::reset();
        if(argc!=2)throw std::runtime_error("Expected fresh fixture directory");
        const auto root=std::filesystem::absolute(argv[1]);
        if(std::filesystem::exists(root)||!std::filesystem::create_directories(root))throw std::runtime_error("Fresh directory required");
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
F FCI Y=2 F=LAW(X0 X1 Y0 Y1) N0 N1 N2 N3 .1
M FCIM Y=2 F=LAW(X0 X1 Y0 Y1) N0 N1 N2 N3 N4 .1
.END MODEL M
.CREATE C M GND=R0
.END
)";
        HgmFunctionStorage::getInstance();HmgFileReader reader;reader.ReadFile(path.generic_string());
        InstructionStream stream;reader.convertToInstructionStream(stream);reader.clear();
        InstructionQueue queue;queue.addInstructionStream(stream);InstructionQueue::waitToFinish(queue);
        auto& circuit=*CircuitStorage::getInstance().fullCircuitInstances[0].component;
        unsigned checks=0,failures=0;
        const auto close=[&](cplx actual,cplx expected,const char* what,unsigned item,unsigned y,unsigned x){
            ++checks;if(!std::isfinite(std::abs(actual))||std::abs(actual-expected)>3e-6*std::max(rvt1,std::abs(expected))){
                if(failures<18)std::cerr<<what<<" component "<<item<<" ["<<y<<','<<x<<"] "<<actual<<" expected "<<expected<<'\n';++failures;
            }
        };
        for(unsigned item=0;item<8;++item){
            auto* part=const_cast<ComponentBase*>(circuit.getContainedComponent(item));
            const unsigned rows=item==6?2:item==7?3:item>=3?4:5;
            const unsigned columns=item==6?4:item==7?5:rows+(item==1||item==4?1:0);
            for(const rvt voltage:{rvt(-.7),rvt(.2),rvt(1.1)}){
                std::array<rvt,6> state{voltage,.1,.3,35,40,30};
                if(rows==4)state={voltage,.1,35,40,30,0};
                for(unsigned x=0;x<columns;++x){part->getNode(x)->extend();part->getNode(x)->setValueDC(state[x]);}
                const auto stamp=[&]{for(unsigned y=0;y<rows;++y)part->getNode(y)->deleteDDC();part->calculateValueDC();part->calculateCurrent(true);
                    std::array<rvt,5> d{};for(unsigned y=0;y<rows;++y)d[y]=part->getNode(y)->getDDC();return d;};
                std::array<std::array<rvt,6>,5> jac{};
                for(unsigned x=0;x<columns;++x){const rvt h=1e-5*std::max(rvt1,std::abs(state[x]));
                    part->getNode(x)->setValueDC(state[x]+h);const auto plus=stamp();
                    part->getNode(x)->setValueDC(state[x]-h);const auto minus=stamp();
                    part->getNode(x)->setValueDC(state[x]);(void)stamp();
                    for(unsigned y=0;y<rows;++y){jac[y][x]=-(plus[y]-minus[y])/(2*h);
                        close(part->getYAC(y,x),jac[y][x],"Jacobian",item,y,x);
                        SimControl::lagDissipationJacobian=true;
                        close(part->getYAC(y,x),jac[y][x],"AC independent of DC startup",item,y,x);
                        SimControl::lagDissipationJacobian=false;
                    }
                }
                for(const rvt amplitude:{rvt0,rvt(.03),rvt(3)}){
                    for(unsigned x=0;x<columns;++x){part->getNode(x)->extension->valueAC=amplitude*cplx(rvt(x+1)*.17,rvt(x+2)*-.09);part->getNode(x)->extension->v=cplx0;part->getNode(x)->deleteDAC();}
                    part->calculateCurrent(false);
                    for(unsigned y=0;y<rows;++y){cplx expected=0;for(unsigned x=0;x<columns;++x)expected-=jac[y][x]*part->getNode(x)->getValueAC();
                        close(part->getNode(y)->getDAC(),expected,"Physical AC current",item,y,0);
                    }
                }
                for(unsigned y=0;y<rows;++y)part->getNode(y)->deleteYiiAC();
                part->calculateYiiAC();for(unsigned y=0;y<rows;++y)close(part->getNode(y)->getYiiAC(),jac[y][y],"AC diagonal",item,y,y);
                // A control or heat port may be wired to another terminal.
                // The diagonal then contains all corresponding matrix entries.
                auto* saved=part->getNode(columns-1);auto* shared=part->getNode(0);
                part->setNode(columns-1,shared);
                const rvt original=shared->getValueDC(),h=1e-5*std::max(rvt1,std::abs(original));
                shared->setValueDC(original+h);const auto plus=stamp();
                shared->setValueDC(original-h);const auto minus=stamp();
                shared->setValueDC(original);(void)stamp();
                for(unsigned y=0;y<rows;++y)part->getNode(y)->deleteYiiAC();
                part->calculateYiiAC();close(shared->getYiiAC(),-(plus[0]-minus[0])/(2*h),"Aliased AC diagonal",item,0,columns-1);
                part->setNode(columns-1,saved);
            }
        }
        std::cout<<checks<<" AC Jacobian/current checks; "<<failures<<" failures\n";return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
