#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

// Contiguous, immutable ray instructions. This module has no circuit, parser
// or GUI dependency. Geometry is prepared once; an Executor reuses its output
// and branch buffers for every nonlinear iteration.
namespace hexmg::optics {
enum class Op : std::uint32_t { Branch, Segment, Split, Wall, Escape, Cutoff, Observe };
struct Step { Op op{}; std::uint32_t index=0; double value=0; };
struct Ray {
    std::uint64_t begin=0;
    std::uint32_t count=0,source=0,branches=0,band=0;
    double fraction=0;
};
static_assert(sizeof(Step)==16 && sizeof(Ray)==32);

struct Program {
    std::vector<Step> steps;
    std::vector<Ray> rays;
    std::size_t cells=0;
    double blueDistance=1,yellowDistance=1;
    bool independentYellow=false;

    // Validate once before executing any externally loaded instructions.
    // Return the largest live branch count, for a single reusable buffer.
    std::size_t validate(std::size_t blueSources,std::size_t yellowSources) const {
        const auto require=[](bool ok,const char* why){if(!ok)throw std::invalid_argument(why);};
        require(cells>0 && cells<=UINT32_MAX,"Invalid optical cell count");
        require(std::isfinite(blueDistance)&&blueDistance>=0&&std::isfinite(yellowDistance)&&yellowDistance>=0,
            "Invalid optical distance multiplier");
        require(independentYellow||yellowSources==0,"Unexpected independent yellow sources");
        std::size_t maximumBranches=1,expectedBegin=0;
        for(const auto& ray:rays){
            require(ray.begin==expectedBegin&&ray.begin<=steps.size()&&ray.count<=steps.size()-ray.begin,
                "Invalid optical instruction range");
            require(ray.count>0&&ray.branches>0&&ray.branches<=ray.count,"Invalid optical branch count");
            require(ray.band<2&&ray.source<(ray.band?yellowSources:blueSources),"Invalid optical source index");
            require(std::isfinite(ray.fraction)&&ray.fraction>0&&ray.fraction<=1,"Invalid optical fraction");
            std::vector<bool> connected(ray.branches);connected[0]=true;
            std::size_t nextBranch=0,currentBranch=0;bool ended=true;
            for(std::size_t i=static_cast<std::size_t>(ray.begin);i<ray.begin+ray.count;++i){
                const auto& step=steps[i];require(std::isfinite(step.value),"Nonfinite optical instruction");
                if(step.op==Op::Branch){
                    require(ended&&step.index==nextBranch&&nextBranch<ray.branches&&connected[nextBranch]&&step.value==0,
                        "Disconnected or reordered optical branch");
                    currentBranch=nextBranch++;ended=false;continue;
                }
                require(nextBranch>0&&!ended,"Optical instruction after termination");
                switch(step.op){
                case Op::Segment:require(step.index<cells&&step.value>0,"Invalid optical segment");break;
                case Op::Observe:require(step.index<cells&&(step.value==.5||step.value==1),"Invalid optical observation");break;
                case Op::Split:
                    require(step.index>currentBranch&&step.index<ray.branches&&!connected[step.index]&&step.value>0&&step.value<1,
                        "Invalid optical split");connected[step.index]=true;break;
                case Op::Wall:require(step.index<cells&&step.value>=0&&step.value<=1,"Invalid optical wall");ended=step.value==0;break;
                case Op::Escape:require(step.index<6&&step.value==0,"Invalid optical exit");ended=true;break;
                case Op::Cutoff:require(step.index==0&&step.value==0,"Invalid optical cutoff");ended=true;break;
                default:throw std::invalid_argument("Unknown optical operation");
                }
            }
            require(ended&&nextBranch==ray.branches,"Unterminated optical ray");
            expectedBegin+=ray.count;maximumBranches=std::max(maximumBranches,std::size_t(ray.branches));
        }
        require(expectedBegin==steps.size(),"Unreferenced optical instructions");
        return maximumBranches;
    }
};

template<class Number> struct Output {
    std::vector<Number> heat,wall,yellow;
    std::vector<Number> blueImage,yellowImage;
    std::array<std::array<Number,2>,6> escape{};
    std::array<Number,2> cutoff{};
};

template<class Number> class Executor {
    const Program& program_;
    std::size_t blueCount_,yellowCount_;
    Output<Number> output_;
    std::vector<std::array<Number,2>> branches_;
public:
    // Program storage must outlive this executor and remain immutable.
    Executor(const Program& program,std::size_t blueCount,std::size_t yellowCount)
        :program_(program),blueCount_(blueCount),yellowCount_(yellowCount),
        branches_(program.validate(blueCount,yellowCount)) {
        output_.heat.resize(program.cells);output_.wall.resize(program.cells);output_.yellow.resize(program.cells);
        if(std::ranges::any_of(program.steps,[](const auto& step){return step.op==Op::Observe;})){output_.blueImage.resize(program.cells);output_.yellowImage.resize(program.cells);}
    }
    const Output<Number>& evaluate(std::span<const std::array<Number,4>> coefficients,
        std::span<const Number> blue,std::span<const Number> yellow) {
        if(coefficients.size()!=program_.cells||blue.size()!=blueCount_||yellow.size()!=yellowCount_)
            throw std::invalid_argument("Optical runtime input count mismatch");
        for(const auto& cell:coefficients)for(unsigned k=0;k<4;++k)
            if(!std::isfinite(cell[k])||cell[k]<0||(k>=2&&cell[k]>1))throw std::invalid_argument("Invalid optical material coefficient");
        for(const auto values:{blue,yellow})for(const auto value:values)
            if(!std::isfinite(value))throw std::invalid_argument("Nonfinite optical source power");
        for(auto* values:{&output_.heat,&output_.wall,&output_.yellow,&output_.blueImage,&output_.yellowImage})std::fill(values->begin(),values->end(),Number(0));
        output_.escape={};output_.cutoff={};
        for(const auto& ray:program_.rays){
            branches_[0]={};branches_[0][ray.band]=(ray.band?yellow:blue)[ray.source]*Number(ray.fraction);
            auto* state=&branches_[0];
            for(std::size_t i=static_cast<std::size_t>(ray.begin);i<ray.begin+ray.count;++i){const auto& step=program_.steps[i];
                switch(step.op){
                case Op::Branch:state=&branches_[step.index];break;
                case Op::Observe:output_.blueImage[step.index]+=(*state)[0]*Number(step.value);output_.yellowImage[step.index]+=(*state)[1]*Number(step.value);break;
                case Op::Segment:{
                    const auto& c=coefficients[step.index];
                    const Number absorbedBlue=(*state)[0]*-std::expm1(-c[0]*Number(step.value)*Number(program_.blueDistance));
                    const Number absorbedYellow=(*state)[1]*-std::expm1(-c[1]*Number(step.value)*Number(program_.yellowDistance));
                    const Number converted=absorbedBlue*c[2]+absorbedYellow*c[3];
                    output_.heat[step.index]+=absorbedBlue*(Number(1)-c[2])+absorbedYellow*(Number(1)-c[3]);
                    output_.yellow[step.index]+=converted;(*state)[0]-=absorbedBlue;(*state)[1]-=absorbedYellow;
                    if(!program_.independentYellow)(*state)[1]+=converted;break;}
                case Op::Split:
                    for(unsigned k=0;k<2;++k){branches_[step.index][k]=(*state)[k]*Number(1-step.value);(*state)[k]*=Number(step.value);}break;
                case Op::Wall:{
                    const Number q=((*state)[0]+(*state)[1])*Number(1-step.value);
                    output_.wall[step.index]+=q;output_.heat[step.index]+=q;
                    for(auto& value:*state)value*=Number(step.value);break;}
                case Op::Escape:for(unsigned k=0;k<2;++k)output_.escape[step.index][k]+=(*state)[k];break;
                case Op::Cutoff:for(unsigned k=0;k<2;++k)output_.cutoff[k]+=(*state)[k];break;
                }
            }
        }
        const auto finite=[](Number value){if(!std::isfinite(value))throw std::overflow_error("Nonfinite optical output");};
        for(const auto* values:{&output_.heat,&output_.wall,&output_.yellow,&output_.blueImage,&output_.yellowImage})for(const auto value:*values)finite(value);
        for(const auto& side:output_.escape)for(const auto value:side)finite(value);
        for(const auto value:output_.cutoff)finite(value);
        return output_;
    }
};
}
