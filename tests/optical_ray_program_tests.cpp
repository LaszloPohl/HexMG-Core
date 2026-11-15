#include "hmgOpticalRayFile.h"
#include <iostream>
#include <numeric>
#include <functional>
using namespace hexmg::optics;
static unsigned checks=0;
static void near(double a,double b){++checks;if(!std::isfinite(a)||std::abs(a-b)>2e-13*std::max(1.,std::abs(b)))throw std::runtime_error("Optical analytic reference mismatch");}
static void rejects(const std::function<void()>& f){++checks;try{f();}catch(const std::exception&){return;}throw std::runtime_error("Invalid program accepted");}
int main(){try{
    Program p;p.cells=2;
    // A transmitted branch and a reflected branch traverse different cells.
    p.steps={{Op::Branch,0,0},{Op::Segment,0,.5},{Op::Split,1,.25},{Op::Wall,1,.5},{Op::Cutoff,0,0},
        {Op::Branch,1,0},{Op::Segment,1,.25},{Op::Escape,3,0}};
    p.rays={{0,8,0,2,0,1}};
    const std::array<std::array<double,4>,2> c{{{2,3,.6,.2},{4,5,.3,.4}}};
    Executor<double> e(p,1,0);
    for(double power:{2.,0.,-1.,7.}){
        const std::array<double,1> input{power};const auto& o=e.evaluate(c,input,{});
        const double a=power*(1-std::exp(-1.)),b=power-a,y=.6*a;
        const double ab=.75*b*(1-std::exp(-1.)),ay=.75*y*(1-std::exp(-1.25));
        near(o.heat[0],.4*a);near(o.yellow[0],y);
        near(o.wall[1],.125*(b+y));near(o.heat[1],.125*(b+y)+.7*ab+.6*ay);
        near(o.yellow[1],.3*ab+.4*ay);near(o.escape[3][0],.75*b-ab);
        near(o.escape[3][1],.75*y-ay+.3*ab+.4*ay);near(o.cutoff[0],.125*b);near(o.cutoff[1],.125*y);
        double balance=std::accumulate(o.heat.begin(),o.heat.end(),0.);
        for(const auto& side:o.escape)for(double v:side)balance+=v;for(double v:o.cutoff)balance+=v;near(balance,power);
    }
    // Independent yellow emission is returned to the circuit, not transported
    // twice within the same ray. Changing inputs must not retain old scratch.
    p.independentYellow=true;p.rays.push_back({8,3,0,1,1,1});
    p.steps.insert(p.steps.end(),{{Op::Branch,0,0},{Op::Segment,0,.5},{Op::Escape,2,0}});
    Executor<double> independent(p,1,1);const std::array<double,1> blue{2},yellow{3};
    const auto& oi=independent.evaluate(c,blue,yellow);
    const double ya=3*(1-std::exp(-1.5));near(oi.escape[2][1],3-ya);near(oi.yellow[0],1.2*(1-std::exp(-1.))+.2*ya);
    double balance=std::accumulate(oi.heat.begin(),oi.heat.end(),0.)+std::accumulate(oi.yellow.begin(),oi.yellow.end(),0.);
    for(const auto& side:oi.escape)for(double v:side)balance+=v;for(double v:oi.cutoff)balance+=v;near(balance,5);
    auto bad=c;bad[0][2]=1.01;rejects([&]{independent.evaluate(bad,blue,yellow);});
    bad=c;bad[0][0]=-1;rejects([&]{independent.evaluate(bad,blue,yellow);});
    bad=c;bad[0][1]=std::numeric_limits<double>::quiet_NaN();rejects([&]{independent.evaluate(bad,blue,yellow);});
    rejects([&]{independent.evaluate(c,{},yellow);});
    const auto mutate=[&](const std::function<void(Program&)>& f){auto q=p;f(q);rejects([&]{q.validate(1,1);});};
    mutate([](auto& q){q.steps[0].index=1;});mutate([](auto& q){q.steps[1].index=2;});
    mutate([](auto& q){q.steps[1].value=-1;});mutate([](auto& q){q.steps[2].index=0;});
    mutate([](auto& q){q.steps[2].value=1;});mutate([](auto& q){q.steps[4].op=Op::Segment;q.steps[4].value=1;});
    mutate([](auto& q){q.steps[5].index=0;});mutate([](auto& q){q.steps[7].index=6;});
    mutate([](auto& q){q.rays[0].begin=1;});mutate([](auto& q){q.rays[0].count=100;});
    mutate([](auto& q){q.rays[0].source=1;});mutate([](auto& q){q.rays[0].fraction=0;});
    mutate([](auto& q){q.steps.push_back({});});mutate([](auto& q){q.steps[1].op=static_cast<Op>(99);});
    mutate([](auto& q){q.blueDistance=-1;});mutate([](auto& q){q.independentYellow=false;});
    File file;file.program=p;file.bindings={{{},2},{{},3},{{},.6},{{},.2},{"CELL.N0",0},{"CELL.B0",0}};
    file.cells.resize(2);for(auto& cell:file.cells)cell.coefficients={0,1,2,3};file.cells[0].outputs[0]=5;file.blue={4};file.yellow={4};
    const auto bytes=encode(file);const auto roundtrip=decode(bytes);near(static_cast<double>(roundtrip.program.steps.size()),static_cast<double>(p.steps.size()));
    if(encode(roundtrip)!=bytes)throw std::runtime_error("Optical file roundtrip differs");++checks;
    auto corrupt=bytes;corrupt[20]^=1;rejects([&]{decode(corrupt);});
    for(std::size_t size=0;size<bytes.size();size+=13)rejects([&]{decode(std::span(bytes).first(size));});
    auto duplicate=file;duplicate.cells[1].outputs[0]=5;rejects([&]{encode(duplicate);});
    std::cout<<"PASS "<<checks<<" optical program/file checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
