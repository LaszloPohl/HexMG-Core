#include "hmgConvergenceMetric.h"
#include <iostream>
#include <limits>
#include <string>
using namespace nsHMG::convergence;
int main() { try {
    int checks=0;
    const auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("convergence arithmetic regression");};
    const auto near=[&](double a,double b){check(std::abs(a-b)<1e-12*std::max(1.,std::abs(b)));};
    const auto reject=[&](auto f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected);};
    const double nan=std::numeric_limits<double>::quiet_NaN(), inf=std::numeric_limits<double>::infinity();
    // Independent analytic boundary: limit = .1 + .01*10 = .2.
    near(normalized(.2,10,.1,.01),1);
    check(normalized(.201,10,.1,.01)>1);
    near(normalized(-.2,10,.1,.01),1);
    near(normalized(1e-6,0,1e-6,.005),1);
    // Equivalent units: scaling errors, references and absolute tolerances together.
    for(double scale:{1e-9,1.,1e9}) {
        near(normalized(.2*scale,10*scale,.1*scale,.01),1);
        near(nodeError(.1*scale,10*scale,.3*scale,20*scale,{.01,.1*scale,.1*scale}),1);
    }
    // Balance is scaled by physical branch magnitudes, not their cancellation.
    near(nodeError(0,0,.01,20,{.005,1e-6,1e-12}),.01/(.1+1e-12));
    check(nodeError(.1,1,0,1,{.005,1e-6,1e-12})>1);
    check(nodeError(0,1,.1,1,{.005,1e-6,1e-12})>1);
    // A single bad node cannot be averaged away by a million exact nodes.
    Collector c;check(c.converged());c.add(2);for(int i=0;i<1000000;++i)c.add(0);check(!c.converged());near(c.maximum(),2);
    Collector edge;edge.add(1);check(edge.converged());edge.add(inf);check(!edge.converged());
    reject([&]{c.add(nan);});reject([&]{c.add(-1);});
    for(double bad:{nan,inf,-inf}) {
        reject([&]{normalized(bad,1,1,.01);});reject([&]{normalized(1,bad,1,.01);});
        reject([&]{normalized(1,1,bad,.01);});reject([&]{normalized(1,1,1,bad);});
    }
    for(double bad:{0.,-1.}) {reject([&]{normalized(1,1,bad,.01);});reject([&]{normalized(1,1,1,bad);});}
    reject([&]{normalized(1,-1,1,.01);});
    reject([&]{normalized(1,1e308,1e308,10);});
    check(std::isinf(normalized(1e308,0,1e-308,.01)));
    std::cout<<checks<<" convergence arithmetic checks PASS; not wired into runtime\n";
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }
