#include "hmgOptimization.h"
#include <chrono>
#include <iostream>
namespace o=nsHMG::optimization;
int main(){try{
    unsigned checks=0;
    const auto check=[&](bool value){++checks;if(!value)throw std::runtime_error("Option index contract");};
    const auto rejected=[&](const std::vector<std::string>& lines){bool failed=false;try{o::parse(lines,"test",0);}catch(const nsHMG::hmgExcept&){failed=true;}check(failed);};
    o::parse({"NEWTON_FORWARD_LIMIT c n0 r0 .2","ADAPTIVE_VOLTAGE_NODE c n0"},"test",0);
    check(o::newtonForwardLimits.size()==1&&o::adaptiveVoltageNodes.size()==1);
    rejected({"NEWTON_FORWARD_LIMIT C N0 R0 .3"});
    rejected({"ADAPTIVE_VOLTAGE_NODE C N0"});
    rejected({"NEWTON_FORWARD_LIMIT c n1 r0 .2","NEWTON_FORWARD_LIMIT c n1 r0 .3"});
    rejected({"ADAPTIVE_VOLTAGE_NODE c n1","ADAPTIVE_VOLTAGE_NODE c n1"});
    check(o::newtonForwardLimits.size()==1&&o::adaptiveVoltageNodes.size()==1);
    o::parse({"NEWTON_FORWARD_LIMIT c r0 n0 .4","NEWTON_FORWARD_LIMIT other n0 r0 .5","ADAPTIVE_VOLTAGE_NODE other n0"},"test",0);
    check(o::newtonForwardLimits.size()==3&&o::adaptiveVoltageNodes.size()==2);
    std::vector<std::string> lines;lines.reserve(400000);
    for(unsigned i=1;i<=200000;++i){const auto node="N"+std::to_string(i);lines.push_back("NEWTON_FORWARD_LIMIT C "+node+" R0 .2");lines.push_back("ADAPTIVE_VOLTAGE_NODE C "+node);}
    const auto start=std::chrono::steady_clock::now();o::parse(lines,"large",0);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    check(o::newtonForwardLimits.size()==200003&&o::adaptiveVoltageNodes.size()==200002);
    check(o::newtonForwardLimits.back().anode=="N200000"&&o::adaptiveVoltageNodes.back().node=="N200000");
    rejected({"ADAPTIVE_VOLTAGE_NODE c n200000"});check(o::adaptiveVoltageNodes.size()==200002);
    std::cout<<"PASS "<<checks<<" option index checks; 400000 entries in "<<elapsed<<" s\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
